// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/audio/wasapi/wasapi_audio_device.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <audioclient.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

constexpr std::int64_t reference_times_per_second = 10'000'000;

[[nodiscard]] std::runtime_error hresult_error(const char* operation, HRESULT result) {
    std::ostringstream stream;
    stream << operation << " failed with HRESULT 0x" << std::hex << std::uppercase
           << static_cast<std::uint32_t>(result);
    return std::runtime_error(stream.str());
}

void throw_if_failed(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        throw hresult_error(operation, result);
    }
}

[[nodiscard]] std::uint64_t checked_sample_count(std::uint32_t frame_count, std::uint32_t channel_count) noexcept {
    return static_cast<std::uint64_t>(frame_count) * static_cast<std::uint64_t>(channel_count);
}

[[nodiscard]] REFERENCE_TIME duration_from_frames(std::uint32_t frames, std::uint32_t sample_rate) noexcept {
    if (frames == 0U || sample_rate == 0U) {
        return 0;
    }
    const auto numerator = static_cast<std::uint64_t>(reference_times_per_second) * frames;
    return static_cast<REFERENCE_TIME>((numerator + sample_rate - 1U) / sample_rate);
}

[[nodiscard]] AudioSampleFormat sample_format_from_wave_format(const WAVEFORMATEX& format) noexcept {
    if (format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT && format.wBitsPerSample == 32U) {
        return AudioSampleFormat::float32;
    }
    if (format.wFormatTag == WAVE_FORMAT_PCM && format.wBitsPerSample == 16U) {
        return AudioSampleFormat::pcm16;
    }
    if (format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
        format.cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
        const auto& extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE&>(format);
        if (format.wBitsPerSample == 32U && IsEqualGUID(extensible.SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) != 0) {
            return AudioSampleFormat::float32;
        }
        if (format.wBitsPerSample == 16U && IsEqualGUID(extensible.SubFormat, KSDATAFORMAT_SUBTYPE_PCM) != 0) {
            return AudioSampleFormat::pcm16;
        }
    }
    return AudioSampleFormat::unknown;
}

[[nodiscard]] AudioDeviceFormat audio_device_format_from_wave_format(const WAVEFORMATEX& format) noexcept {
    return AudioDeviceFormat{
        .sample_rate = format.nSamplesPerSec,
        .channel_count = format.nChannels,
        .sample_format = sample_format_from_wave_format(format),
    };
}

class CoTaskMemWaveFormat final {
  public:
    CoTaskMemWaveFormat() = default;
    ~CoTaskMemWaveFormat() {
        if (format_ != nullptr) {
            CoTaskMemFree(format_);
        }
    }

    CoTaskMemWaveFormat(const CoTaskMemWaveFormat&) = delete;
    CoTaskMemWaveFormat& operator=(const CoTaskMemWaveFormat&) = delete;
    CoTaskMemWaveFormat(CoTaskMemWaveFormat&&) = delete;
    CoTaskMemWaveFormat& operator=(CoTaskMemWaveFormat&&) = delete;

    [[nodiscard]] WAVEFORMATEX** put() noexcept {
        return &format_;
    }

    [[nodiscard]] const WAVEFORMATEX& get() const {
        if (format_ == nullptr) {
            throw std::runtime_error("WASAPI mix format was not initialized");
        }
        return *format_;
    }

  private:
    WAVEFORMATEX* format_{nullptr};
};

[[nodiscard]] std::int16_t float_to_pcm16(float sample) noexcept {
    const auto clamped = std::clamp(sample, -1.0F, 1.0F);
    const auto scaled = clamped < 0.0F ? clamped * 32768.0F : clamped * 32767.0F;
    return static_cast<std::int16_t>(std::lrint(scaled));
}

} // namespace

bool WasapiAudioRuntimePlan::succeeded() const noexcept {
    return use_multimedia_device_enumerator && use_default_render_endpoint && use_console_role &&
           !native_handle_exposed;
}

bool WasapiAudioStreamPlan::succeeded() const noexcept {
    return diagnostic == WasapiAudioStreamDiagnostic::none && wasapi_audio_device_format_supported(format) &&
           target_latency_frames > 0U && use_default_render_endpoint && use_shared_mode && use_render_client &&
           !use_event_callback && !use_exclusive_mode && !use_capture && !native_handle_exposed;
}

bool WasapiAudioQueuePlan::succeeded() const noexcept {
    return diagnostic == WasapiAudioQueueDiagnostic::none;
}

bool wasapi_audio_device_format_supported(AudioDeviceFormat format) noexcept {
    return is_valid_audio_device_format(format) &&
           (format.sample_format == AudioSampleFormat::float32 || format.sample_format == AudioSampleFormat::pcm16);
}

WasapiAudioRuntimePlan plan_wasapi_audio_runtime(WasapiAudioRuntimeDesc desc) noexcept {
    return WasapiAudioRuntimePlan{
        .initialize_com = desc.initialize_com,
        .use_multimedia_device_enumerator = true,
        .use_default_render_endpoint = true,
        .use_console_role = true,
        .native_handle_exposed = false,
    };
}

WasapiAudioStreamPlan plan_wasapi_shared_mode_stream(const WasapiAudioDeviceDesc& desc) noexcept {
    auto plan = WasapiAudioStreamPlan{
        .format = desc.format,
        .target_latency_frames = desc.target_latency_frames,
        .diagnostic = WasapiAudioStreamDiagnostic::none,
        .use_default_render_endpoint = true,
        .use_shared_mode = true,
        .use_render_client = true,
        .use_event_callback = false,
        .use_exclusive_mode = false,
        .use_capture = false,
        .queue_silence_before_start = desc.start_paused,
        .native_handle_exposed = false,
    };
    if (!wasapi_audio_device_format_supported(desc.format)) {
        plan.diagnostic = WasapiAudioStreamDiagnostic::invalid_format;
        return plan;
    }
    if (desc.target_latency_frames == 0U) {
        plan.diagnostic = WasapiAudioStreamDiagnostic::invalid_latency_frames;
        return plan;
    }
    return plan;
}

WasapiAudioQueuePlan plan_wasapi_render_queue(WasapiAudioQueueRequest request) noexcept {
    auto plan = WasapiAudioQueuePlan{
        .diagnostic = WasapiAudioQueueDiagnostic::none,
        .available_frames = 0,
        .frames_to_write = 0,
        .queued_frames_after = request.current_padding_frames,
        .submit_silence = request.submit_silence,
    };
    if (!wasapi_audio_device_format_supported(request.format)) {
        plan.diagnostic = WasapiAudioQueueDiagnostic::invalid_format;
        return plan;
    }
    if (request.buffer_frames == 0U) {
        plan.diagnostic = WasapiAudioQueueDiagnostic::invalid_buffer_frames;
        return plan;
    }
    if (request.current_padding_frames > request.buffer_frames) {
        plan.diagnostic = WasapiAudioQueueDiagnostic::padding_exceeds_buffer;
        return plan;
    }

    plan.available_frames = request.buffer_frames - request.current_padding_frames;
    plan.frames_to_write = std::min(request.requested_frames, plan.available_frames);
    plan.queued_frames_after = request.current_padding_frames + plan.frames_to_write;
    if (!request.submit_silence) {
        const auto expected_samples = checked_sample_count(plan.frames_to_write, request.format.channel_count);
        if (request.interleaved_sample_count < expected_samples) {
            plan.diagnostic = WasapiAudioQueueDiagnostic::sample_count_mismatch;
        }
    }
    return plan;
}

AudioProductionDeviceLifecycleRow wasapi_audio_device_lifecycle_evidence(bool host_evidence_available) {
    return AudioProductionDeviceLifecycleRow{
        .backend_id = "wasapi",
        .uses_logical_device = true,
        .uses_audio_stream = true,
        .uses_queueing = true,
        .uses_callback = false,
        .can_pause_resume = true,
        .can_clear = true,
        .host_evidence_available = host_evidence_available,
        .native_handle_exposed = false,
        .source_index = 0U,
    };
}

WasapiAudioRuntime::WasapiAudioRuntime(WasapiAudioRuntimeDesc desc) {
    if (!desc.initialize_com) {
        return;
    }
    const auto result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(result)) {
        throw hresult_error("CoInitializeEx", result);
    }
    com_initialized_ = true;
}

WasapiAudioRuntime::~WasapiAudioRuntime() {
    if (com_initialized_) {
        CoUninitialize();
    }
}

struct WasapiAudioDevice::Impl {
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    Microsoft::WRL::ComPtr<IMMDevice> endpoint;
    Microsoft::WRL::ComPtr<IAudioClient> audio_client;
    Microsoft::WRL::ComPtr<IAudioRenderClient> render_client;
    AudioDeviceFormat format;
    std::uint32_t buffer_frames{0};
    bool paused{true};

    explicit Impl(WasapiAudioDeviceDesc desc) {
        const auto stream_plan = plan_wasapi_shared_mode_stream(desc);
        if (!stream_plan.succeeded()) {
            throw std::invalid_argument("WASAPI audio stream descriptor is invalid");
        }

        throw_if_failed(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                         IID_PPV_ARGS(enumerator.GetAddressOf())),
                        "CoCreateInstance(MMDeviceEnumerator)");
        throw_if_failed(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, endpoint.GetAddressOf()),
                        "IMMDeviceEnumerator::GetDefaultAudioEndpoint");
        throw_if_failed(endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                           reinterpret_cast<void**>(audio_client.GetAddressOf())),
                        "IMMDevice::Activate(IAudioClient)");

        CoTaskMemWaveFormat mix_format;
        throw_if_failed(audio_client->GetMixFormat(mix_format.put()), "IAudioClient::GetMixFormat");
        format = audio_device_format_from_wave_format(mix_format.get());
        if (!wasapi_audio_device_format_supported(format)) {
            throw std::runtime_error("WASAPI shared-mode mix format is unsupported");
        }

        const auto buffer_duration = duration_from_frames(desc.target_latency_frames, format.sample_rate);
        throw_if_failed(
            audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, buffer_duration, 0, &mix_format.get(), nullptr),
            "IAudioClient::Initialize");
        throw_if_failed(audio_client->GetBufferSize(&buffer_frames), "IAudioClient::GetBufferSize");
        throw_if_failed(audio_client->GetService(IID_PPV_ARGS(render_client.GetAddressOf())),
                        "IAudioClient::GetService(IAudioRenderClient)");
        if (!desc.start_paused) {
            resume();
        }
    }

    ~Impl() {
        if (audio_client.Get() != nullptr) {
            (void)audio_client->Stop();
        }
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    [[nodiscard]] std::uint32_t queued_frames() const {
        if (audio_client.Get() == nullptr) {
            return 0;
        }
        std::uint32_t padding = 0;
        throw_if_failed(audio_client->GetCurrentPadding(&padding), "IAudioClient::GetCurrentPadding");
        return padding;
    }

    void queue_render_frames(std::span<const float> samples, std::uint32_t requested_frames, bool submit_silence) {
        if (audio_client.Get() == nullptr || render_client.Get() == nullptr) {
            throw std::runtime_error("WASAPI audio stream is not active");
        }
        if (requested_frames == 0U) {
            return;
        }

        const auto queue_plan = plan_wasapi_render_queue(WasapiAudioQueueRequest{
            .format = format,
            .buffer_frames = buffer_frames,
            .current_padding_frames = queued_frames(),
            .requested_frames = requested_frames,
            .interleaved_sample_count = samples.size(),
            .submit_silence = submit_silence,
        });
        if (!queue_plan.succeeded()) {
            throw std::invalid_argument("WASAPI render queue request is invalid");
        }
        if (queue_plan.frames_to_write == 0U) {
            return;
        }

        BYTE* data = nullptr;
        throw_if_failed(render_client->GetBuffer(queue_plan.frames_to_write, &data), "IAudioRenderClient::GetBuffer");
        if (!queue_plan.submit_silence) {
            copy_interleaved_float_samples(data, samples, queue_plan.frames_to_write);
        }
        throw_if_failed(render_client->ReleaseBuffer(queue_plan.frames_to_write,
                                                     queue_plan.submit_silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0),
                        "IAudioRenderClient::ReleaseBuffer");
    }

    void copy_interleaved_float_samples(BYTE* destination, std::span<const float> samples,
                                        std::uint32_t frame_count) const {
        if (destination == nullptr) {
            throw std::runtime_error("WASAPI render buffer is null");
        }
        const auto sample_count = static_cast<std::size_t>(checked_sample_count(frame_count, format.channel_count));
        if (format.sample_format == AudioSampleFormat::float32) {
            std::memcpy(destination, samples.data(), sample_count * sizeof(float));
            return;
        }
        if (format.sample_format == AudioSampleFormat::pcm16) {
            auto* pcm = reinterpret_cast<std::int16_t*>(destination);
            for (std::size_t sample_index = 0; sample_index < sample_count; ++sample_index) {
                pcm[sample_index] = float_to_pcm16(samples[sample_index]);
            }
            return;
        }
        throw std::invalid_argument("WASAPI audio stream format is unsupported");
    }

    void clear() {
        if (audio_client.Get() == nullptr) {
            return;
        }
        const auto was_running = !paused;
        throw_if_failed(audio_client->Stop(), "IAudioClient::Stop");
        paused = true;
        throw_if_failed(audio_client->Reset(), "IAudioClient::Reset");
        if (was_running) {
            resume();
        }
    }

    void pause() {
        if (audio_client.Get() == nullptr || paused) {
            return;
        }
        throw_if_failed(audio_client->Stop(), "IAudioClient::Stop");
        paused = true;
    }

    void resume() {
        if (audio_client.Get() == nullptr || !paused) {
            return;
        }
        throw_if_failed(audio_client->Start(), "IAudioClient::Start");
        paused = false;
    }
};

WasapiAudioDevice::WasapiAudioDevice(WasapiAudioDeviceDesc desc) : impl_(std::make_unique<Impl>(std::move(desc))) {}

WasapiAudioDevice::~WasapiAudioDevice() = default;

WasapiAudioDevice::WasapiAudioDevice(WasapiAudioDevice&&) noexcept = default;

WasapiAudioDevice& WasapiAudioDevice::operator=(WasapiAudioDevice&&) noexcept = default;

bool WasapiAudioDevice::active() const noexcept {
    return impl_ != nullptr && impl_->audio_client.Get() != nullptr && impl_->render_client.Get() != nullptr;
}

bool WasapiAudioDevice::paused() const noexcept {
    return impl_ == nullptr || impl_->paused;
}

std::uint32_t WasapiAudioDevice::queued_frames() const {
    if (!active()) {
        return 0;
    }
    return impl_->queued_frames();
}

std::uint32_t WasapiAudioDevice::buffer_frames() const noexcept {
    if (!active()) {
        return 0;
    }
    return impl_->buffer_frames;
}

const AudioDeviceFormat& WasapiAudioDevice::format() const noexcept {
    return impl_->format;
}

std::string_view WasapiAudioDevice::backend_name() const noexcept {
    return "wasapi";
}

void WasapiAudioDevice::queue_silence_frames(std::uint32_t frame_count) {
    if (frame_count == 0U) {
        return;
    }
    impl_->queue_render_frames({}, frame_count, true);
}

void WasapiAudioDevice::queue_interleaved_float_frames(std::span<const float> samples) {
    if (samples.empty()) {
        return;
    }
    if (samples.size() % impl_->format.channel_count != 0U) {
        throw std::invalid_argument("interleaved audio sample count must align to channel count");
    }
    const auto requested_frames = static_cast<std::uint64_t>(samples.size()) / impl_->format.channel_count;
    if (requested_frames > std::numeric_limits<std::uint32_t>::max()) {
        throw std::overflow_error("WASAPI audio queue frame count overflows");
    }
    impl_->queue_render_frames(samples, static_cast<std::uint32_t>(requested_frames), false);
}

void WasapiAudioDevice::clear() {
    impl_->clear();
}

void WasapiAudioDevice::pause() {
    impl_->pause();
}

void WasapiAudioDevice::resume() {
    impl_->resume();
}

} // namespace mirakana
