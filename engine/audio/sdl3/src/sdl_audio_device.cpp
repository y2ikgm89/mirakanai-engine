// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/audio/sdl3/sdl_audio_device.hpp"

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] std::runtime_error sdl_error(const char* operation) {
    return std::runtime_error(std::string(operation) + " failed: " + SDL_GetError());
}

[[nodiscard]] SDL_AudioFormat to_sdl_audio_format(AudioSampleFormat format) {
    switch (format) {
    case AudioSampleFormat::float32:
        return SDL_AUDIO_F32;
    case AudioSampleFormat::pcm16:
        return SDL_AUDIO_S16;
    case AudioSampleFormat::unknown:
        break;
    }
    throw std::invalid_argument("audio sample format is not supported by SDL3 adapter");
}

[[nodiscard]] AudioSampleFormat from_sdl_audio_format(SDL_AudioFormat format) noexcept {
    if (format == SDL_AUDIO_F32) {
        return AudioSampleFormat::float32;
    }
    if (format == SDL_AUDIO_S16) {
        return AudioSampleFormat::pcm16;
    }
    return AudioSampleFormat::unknown;
}

[[nodiscard]] SDL_AudioSpec to_sdl_audio_spec(AudioDeviceFormat format) {
    if (!sdl3_audio_device_format_supported(format)) {
        throw std::invalid_argument("SDL3 audio device format is invalid");
    }
    return SDL_AudioSpec{
        to_sdl_audio_format(format.sample_format),
        static_cast<int>(format.channel_count),
        static_cast<int>(format.sample_rate),
    };
}

[[nodiscard]] AudioDeviceFormat from_sdl_audio_spec(const SDL_AudioSpec& spec) noexcept {
    return AudioDeviceFormat{
        .sample_rate = spec.freq > 0 ? static_cast<std::uint32_t>(spec.freq) : 0U,
        .channel_count = spec.channels > 0 ? static_cast<std::uint32_t>(spec.channels) : 0U,
        .sample_format = from_sdl_audio_format(spec.format),
    };
}

[[nodiscard]] std::uint32_t frame_size_bytes(AudioDeviceFormat format) {
    const auto sample_size = format.sample_format == AudioSampleFormat::float32 ? sizeof(float) : sizeof(std::int16_t);
    const auto channel_count = static_cast<std::uint64_t>(format.channel_count);
    const auto result = channel_count * static_cast<std::uint64_t>(sample_size);
    if (result == 0 || result > std::numeric_limits<std::uint32_t>::max()) {
        throw std::overflow_error("audio frame size overflows");
    }
    return static_cast<std::uint32_t>(result);
}

[[nodiscard]] int checked_byte_count(std::size_t byte_count) {
    if (byte_count > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::overflow_error("SDL3 audio queue byte count overflows");
    }
    return static_cast<int>(byte_count);
}

} // namespace

bool sdl3_audio_device_format_supported(AudioDeviceFormat format) noexcept {
    return is_valid_audio_device_format(format) &&
           (format.sample_format == AudioSampleFormat::float32 || format.sample_format == AudioSampleFormat::pcm16);
}

SdlAudioRuntime::SdlAudioRuntime(SdlAudioRuntimeDesc desc) {
    if (!desc.audio_driver_hint.empty()) {
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, desc.audio_driver_hint.c_str());
    }

    flags_ = SDL_INIT_AUDIO;
    if (!SDL_InitSubSystem(flags_)) {
        throw sdl_error("SDL_InitSubSystem");
    }
}

SdlAudioRuntime::~SdlAudioRuntime() {
    SDL_QuitSubSystem(flags_);
}

struct SdlAudioDevice::Impl {
    SDL_AudioStream* stream{nullptr};
    AudioDeviceFormat format;

    explicit Impl(SdlAudioDeviceDesc desc) {
        auto requested = to_sdl_audio_spec(desc.format);
        if (!desc.stream_name.empty()) {
            SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, desc.stream_name.c_str());
        }

        stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &requested, nullptr, nullptr);
        if (stream == nullptr) {
            throw sdl_error("SDL_OpenAudioDeviceStream");
        }

        SDL_AudioSpec source{};
        SDL_AudioSpec destination{};
        if (!SDL_GetAudioStreamFormat(stream, &source, &destination)) {
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
            throw sdl_error("SDL_GetAudioStreamFormat");
        }
        format = from_sdl_audio_spec(source);
        if (!sdl3_audio_device_format_supported(format)) {
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
            throw std::runtime_error("SDL3 audio stream format is unsupported");
        }

        if (!desc.start_paused) {
            resume();
        }
    }

    ~Impl() {
        if (stream != nullptr) {
            SDL_DestroyAudioStream(stream);
        }
    }

    void queue_bytes(const void* data, std::size_t byte_count) {
        if (stream == nullptr) {
            throw std::runtime_error("SDL3 audio stream is not active");
        }
        if (byte_count == 0) {
            return;
        }
        if (!SDL_PutAudioStreamData(stream, data, checked_byte_count(byte_count))) {
            throw sdl_error("SDL_PutAudioStreamData");
        }
    }

    void clear() {
        if (stream != nullptr && !SDL_ClearAudioStream(stream)) {
            throw sdl_error("SDL_ClearAudioStream");
        }
    }

    void pause() {
        if (stream != nullptr && !SDL_PauseAudioStreamDevice(stream)) {
            throw sdl_error("SDL_PauseAudioStreamDevice");
        }
    }

    void resume() {
        if (stream != nullptr && !SDL_ResumeAudioStreamDevice(stream)) {
            throw sdl_error("SDL_ResumeAudioStreamDevice");
        }
    }
};

SdlAudioDevice::SdlAudioDevice(SdlAudioDeviceDesc desc) : impl_(std::make_unique<Impl>(std::move(desc))) {}

SdlAudioDevice::~SdlAudioDevice() = default;

SdlAudioDevice::SdlAudioDevice(SdlAudioDevice&&) noexcept = default;

SdlAudioDevice& SdlAudioDevice::operator=(SdlAudioDevice&&) noexcept = default;

bool SdlAudioDevice::active() const noexcept {
    return impl_ != nullptr && impl_->stream != nullptr;
}

bool SdlAudioDevice::paused() const {
    if (!active()) {
        return false;
    }
    return SDL_AudioStreamDevicePaused(impl_->stream);
}

std::uint32_t SdlAudioDevice::queued_frames() const {
    if (!active()) {
        return 0;
    }
    const auto queued_bytes = SDL_GetAudioStreamQueued(impl_->stream);
    if (queued_bytes <= 0) {
        return 0;
    }
    return static_cast<std::uint32_t>(queued_bytes) / frame_size_bytes(impl_->format);
}

const AudioDeviceFormat& SdlAudioDevice::format() const noexcept {
    return impl_->format;
}

std::string_view SdlAudioDevice::backend_name() const noexcept {
    return "sdl3";
}

void SdlAudioDevice::queue_silence_frames(std::uint32_t frame_count) {
    if (frame_count == 0U) {
        return;
    }
    const auto bytes = static_cast<std::size_t>(frame_count) * frame_size_bytes(impl_->format);
    std::vector<std::byte> silence(bytes);
    impl_->queue_bytes(silence.data(), silence.size());
}

void SdlAudioDevice::queue_interleaved_float_frames(std::span<const float> samples) {
    if (impl_->format.sample_format != AudioSampleFormat::float32) {
        throw std::invalid_argument("SDL3 audio device is not using float32 samples");
    }
    if (samples.empty()) {
        return;
    }
    if (samples.size() % impl_->format.channel_count != 0U) {
        throw std::invalid_argument("interleaved audio sample count must align to channel count");
    }
    impl_->queue_bytes(samples.data(), samples.size_bytes());
}

void SdlAudioDevice::clear() {
    impl_->clear();
}

void SdlAudioDevice::pause() {
    impl_->pause();
}

void SdlAudioDevice::resume() {
    impl_->resume();
}

} // namespace mirakana
