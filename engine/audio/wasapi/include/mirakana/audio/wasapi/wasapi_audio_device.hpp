// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/audio/audio_mixer.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace mirakana {

struct WasapiAudioRuntimeDesc {
    bool initialize_com{true};
};

struct WasapiAudioRuntimePlan {
    bool initialize_com{true};
    bool use_multimedia_device_enumerator{true};
    bool use_default_render_endpoint{true};
    bool use_console_role{true};
    bool native_handle_exposed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

class WasapiAudioRuntime final {
  public:
    explicit WasapiAudioRuntime(WasapiAudioRuntimeDesc desc = {});
    ~WasapiAudioRuntime();

    WasapiAudioRuntime(const WasapiAudioRuntime&) = delete;
    WasapiAudioRuntime& operator=(const WasapiAudioRuntime&) = delete;
    WasapiAudioRuntime(WasapiAudioRuntime&&) = delete;
    WasapiAudioRuntime& operator=(WasapiAudioRuntime&&) = delete;

  private:
    bool com_initialized_{false};
};

struct WasapiAudioDeviceDesc {
    AudioDeviceFormat format;
    std::uint32_t target_latency_frames{1024};
    bool start_paused{true};
    std::string stream_name;
};

enum class WasapiAudioStreamDiagnostic : std::uint8_t {
    none,
    invalid_format,
    invalid_latency_frames,
};

struct WasapiAudioStreamPlan {
    AudioDeviceFormat format;
    std::uint32_t target_latency_frames{0};
    WasapiAudioStreamDiagnostic diagnostic{WasapiAudioStreamDiagnostic::none};
    bool use_default_render_endpoint{true};
    bool use_shared_mode{true};
    bool use_render_client{true};
    bool use_event_callback{false};
    bool use_exclusive_mode{false};
    bool use_capture{false};
    bool queue_silence_before_start{true};
    bool native_handle_exposed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

enum class WasapiAudioQueueDiagnostic : std::uint8_t {
    none,
    invalid_format,
    invalid_buffer_frames,
    padding_exceeds_buffer,
    sample_count_mismatch,
};

struct WasapiAudioQueueRequest {
    AudioDeviceFormat format;
    std::uint32_t buffer_frames{0};
    std::uint32_t current_padding_frames{0};
    std::uint32_t requested_frames{0};
    std::size_t interleaved_sample_count{0};
    bool submit_silence{false};
};

struct WasapiAudioQueuePlan {
    WasapiAudioQueueDiagnostic diagnostic{WasapiAudioQueueDiagnostic::none};
    std::uint32_t available_frames{0};
    std::uint32_t frames_to_write{0};
    std::uint32_t queued_frames_after{0};
    bool submit_silence{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

class WasapiAudioDevice final {
  public:
    explicit WasapiAudioDevice(WasapiAudioDeviceDesc desc);
    ~WasapiAudioDevice();

    WasapiAudioDevice(const WasapiAudioDevice&) = delete;
    WasapiAudioDevice& operator=(const WasapiAudioDevice&) = delete;
    WasapiAudioDevice(WasapiAudioDevice&&) noexcept;
    WasapiAudioDevice& operator=(WasapiAudioDevice&&) noexcept;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] bool paused() const noexcept;
    [[nodiscard]] std::uint32_t queued_frames() const;
    [[nodiscard]] std::uint32_t buffer_frames() const noexcept;
    [[nodiscard]] const AudioDeviceFormat& format() const noexcept;
    [[nodiscard]] std::string_view backend_name() const noexcept;

    void queue_silence_frames(std::uint32_t frame_count);
    void queue_interleaved_float_frames(std::span<const float> samples);
    void clear();
    void pause();
    void resume();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] bool wasapi_audio_device_format_supported(AudioDeviceFormat format) noexcept;
[[nodiscard]] WasapiAudioRuntimePlan plan_wasapi_audio_runtime(WasapiAudioRuntimeDesc desc = {}) noexcept;
[[nodiscard]] WasapiAudioStreamPlan plan_wasapi_shared_mode_stream(const WasapiAudioDeviceDesc& desc) noexcept;
[[nodiscard]] WasapiAudioQueuePlan plan_wasapi_render_queue(WasapiAudioQueueRequest request) noexcept;
[[nodiscard]] AudioProductionDeviceLifecycleRow
wasapi_audio_device_lifecycle_evidence(bool host_evidence_available = false);

} // namespace mirakana
