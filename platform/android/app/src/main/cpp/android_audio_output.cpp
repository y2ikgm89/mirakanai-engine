// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "android_audio_output.hpp"

#include <aaudio/AAudio.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mirakana_android {
namespace {

[[nodiscard]] std::runtime_error aaudio_error(const char* operation, aaudio_result_t result) {
    return std::runtime_error(std::string(operation) + " failed: " + AAudio_convertResultToText(result));
}

[[nodiscard]] std::size_t checked_sample_count(std::uint32_t frame_count, std::uint32_t channel_count) {
    const auto samples = static_cast<std::uint64_t>(frame_count) * static_cast<std::uint64_t>(channel_count);
    if (samples == 0 || samples > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::overflow_error("Android audio sample count overflows");
    }
    return static_cast<std::size_t>(samples);
}

[[nodiscard]] float sanitize_output_sample(float sample) noexcept {
    if (!std::isfinite(sample)) {
        return 0.0F;
    }
    return std::clamp(sample, -1.0F, 1.0F);
}

} // namespace

bool android_audio_output_format_supported(mirakana::AudioDeviceFormat format) noexcept {
    return mirakana::is_valid_audio_device_format(format) && format.sample_format == mirakana::AudioSampleFormat::float32;
}

struct AndroidAudioOutput::Impl {
    AAudioStream* stream{nullptr};
    mirakana::AudioDeviceFormat format{};
    std::vector<float> ring;
    std::atomic<std::uint64_t> read_sample{0};
    std::atomic<std::uint64_t> write_sample{0};
    std::atomic<aaudio_result_t> last_error{AAUDIO_OK};
    std::atomic<bool> running{false};

    explicit Impl(AndroidAudioOutputDesc desc) {
        if (!android_audio_output_format_supported(desc.format)) {
            throw std::invalid_argument("Android audio output requires a valid float32 device format");
        }

        AAudioStreamBuilder* builder = nullptr;
        auto result = AAudio_createStreamBuilder(&builder);
        if (result != AAUDIO_OK) {
            throw aaudio_error("AAudio_createStreamBuilder", result);
        }

        AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
        AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
        AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
        AAudioStreamBuilder_setSampleRate(builder, static_cast<std::int32_t>(desc.format.sample_rate));
        AAudioStreamBuilder_setChannelCount(builder, static_cast<std::int32_t>(desc.format.channel_count));
        AAudioStreamBuilder_setDataCallback(builder, data_callback, this);
        AAudioStreamBuilder_setErrorCallback(builder, error_callback, this);

        result = AAudioStreamBuilder_openStream(builder, &stream);
        AAudioStreamBuilder_delete(builder);
        if (result != AAUDIO_OK) {
            stream = nullptr;
            throw aaudio_error("AAudioStreamBuilder_openStream", result);
        }

        format = mirakana::AudioDeviceFormat{
            static_cast<std::uint32_t>(std::max(AAudioStream_getSampleRate(stream), 0)),
            static_cast<std::uint32_t>(std::max(AAudioStream_getChannelCount(stream), 0)),
            AAudioStream_getFormat(stream) == AAUDIO_FORMAT_PCM_FLOAT ? mirakana::AudioSampleFormat::float32
                                                                      : mirakana::AudioSampleFormat::unknown,
        };
        if (!android_audio_output_format_supported(format)) {
            close_stream();
            throw std::runtime_error("Android audio stream negotiated an unsupported format");
        }

        const auto frames_per_burst = AAudioStream_getFramesPerBurst(stream);
        if (frames_per_burst > 0) {
            (void)AAudioStream_setBufferSizeInFrames(stream, frames_per_burst * 2);
        }

        ring.assign(checked_sample_count(desc.queue_capacity_frames, format.channel_count), 0.0F);
        if (desc.start_immediately) {
            start();
        }
    }

    ~Impl() {
        close_stream();
    }

    [[nodiscard]] bool active() const noexcept {
        return stream != nullptr;
    }

    [[nodiscard]] std::uint32_t queued_frames() const noexcept {
        if (format.channel_count == 0U) {
            return 0;
        }
        const auto read = read_sample.load(std::memory_order_acquire);
        const auto write = write_sample.load(std::memory_order_acquire);
        if (write <= read) {
            return 0;
        }
        const auto queued_samples = write - read;
        const auto queued = queued_samples / format.channel_count;
        return queued > std::numeric_limits<std::uint32_t>::max() ? std::numeric_limits<std::uint32_t>::max()
                                                                  : static_cast<std::uint32_t>(queued);
    }

    void queue(std::span<const float> samples) {
        if (!active()) {
            throw std::runtime_error("Android audio stream is not active");
        }
        if (samples.empty()) {
            return;
        }
        if (samples.size() % format.channel_count != 0U) {
            throw std::invalid_argument("interleaved Android audio sample count must align to channel count");
        }

        const auto capacity = static_cast<std::uint64_t>(ring.size());
        const auto read = read_sample.load(std::memory_order_acquire);
        const auto write = write_sample.load(std::memory_order_relaxed);
        if (write < read || static_cast<std::uint64_t>(samples.size()) > capacity - (write - read)) {
            throw std::overflow_error("Android audio queue capacity exceeded");
        }

        for (std::size_t index = 0; index < samples.size(); ++index) {
            ring[static_cast<std::size_t>((write + index) % capacity)] = sanitize_output_sample(samples[index]);
        }
        write_sample.store(write + samples.size(), std::memory_order_release);
    }

    void clear() noexcept {
        read_sample.store(write_sample.load(std::memory_order_acquire), std::memory_order_release);
    }

    void start() {
        if (!active() || running.load(std::memory_order_acquire)) {
            return;
        }
        const auto result = AAudioStream_requestStart(stream);
        if (result != AAUDIO_OK) {
            throw aaudio_error("AAudioStream_requestStart", result);
        }
        running.store(true, std::memory_order_release);
    }

    void stop() noexcept {
        if (!active() || !running.load(std::memory_order_acquire)) {
            return;
        }
        (void)AAudioStream_requestStop(stream);
        running.store(false, std::memory_order_release);
        clear();
    }

    void close_stream() noexcept {
        if (stream == nullptr) {
            return;
        }
        stop();
        (void)AAudioStream_close(stream);
        stream = nullptr;
    }

    [[nodiscard]] aaudio_data_callback_result_t fill_output(float* output, std::int32_t frame_count) noexcept {
        if (output == nullptr || frame_count <= 0 || ring.empty()) {
            return AAUDIO_CALLBACK_RESULT_CONTINUE;
        }

        const auto sample_count =
            static_cast<std::uint64_t>(frame_count) * static_cast<std::uint64_t>(format.channel_count);
        const auto capacity = static_cast<std::uint64_t>(ring.size());
        auto read = read_sample.load(std::memory_order_relaxed);
        const auto write = write_sample.load(std::memory_order_acquire);

        for (std::uint64_t index = 0; index < sample_count; ++index) {
            if (read < write) {
                output[index] = ring[static_cast<std::size_t>(read % capacity)];
                ++read;
            } else {
                output[index] = 0.0F;
            }
        }

        read_sample.store(read, std::memory_order_release);
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }

    static aaudio_data_callback_result_t data_callback(AAudioStream*, void* user_data, void* audio_data,
                                                       std::int32_t frame_count) {
        auto* self = static_cast<Impl*>(user_data);
        return self == nullptr ? AAUDIO_CALLBACK_RESULT_STOP
                               : self->fill_output(static_cast<float*>(audio_data), frame_count);
    }

    static void error_callback(AAudioStream*, void* user_data, aaudio_result_t error) {
        auto* self = static_cast<Impl*>(user_data);
        if (self != nullptr) {
            self->last_error.store(error, std::memory_order_release);
        }
    }
};

AndroidAudioOutput::AndroidAudioOutput(AndroidAudioOutputDesc desc) : impl_(std::make_unique<Impl>(std::move(desc))) {}

AndroidAudioOutput::~AndroidAudioOutput() = default;

AndroidAudioOutput::AndroidAudioOutput(AndroidAudioOutput&&) noexcept = default;

AndroidAudioOutput& AndroidAudioOutput::operator=(AndroidAudioOutput&&) noexcept = default;

bool AndroidAudioOutput::active() const noexcept {
    return impl_ != nullptr && impl_->active();
}

bool AndroidAudioOutput::started() const noexcept {
    return impl_ != nullptr && impl_->running.load(std::memory_order_acquire);
}

std::uint32_t AndroidAudioOutput::queued_frames() const noexcept {
    return impl_ == nullptr ? 0U : impl_->queued_frames();
}

const mirakana::AudioDeviceFormat& AndroidAudioOutput::format() const noexcept {
    return impl_->format;
}

std::string_view AndroidAudioOutput::backend_name() const noexcept {
    return "aaudio";
}

void AndroidAudioOutput::queue_interleaved_float_frames(std::span<const float> samples) {
    impl_->queue(samples);
}

void AndroidAudioOutput::clear() noexcept {
    if (impl_ != nullptr) {
        impl_->clear();
    }
}

void AndroidAudioOutput::start() {
    impl_->start();
}

void AndroidAudioOutput::stop() noexcept {
    if (impl_ != nullptr) {
        impl_->stop();
    }
}

} // namespace mirakana_android

