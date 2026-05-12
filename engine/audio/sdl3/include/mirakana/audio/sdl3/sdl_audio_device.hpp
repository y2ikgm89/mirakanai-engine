// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/audio/audio_mixer.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace mirakana {

struct SdlAudioRuntimeDesc {
    std::string audio_driver_hint;
};

class SdlAudioRuntime final {
  public:
    explicit SdlAudioRuntime(SdlAudioRuntimeDesc desc = {});
    ~SdlAudioRuntime();

    SdlAudioRuntime(const SdlAudioRuntime&) = delete;
    SdlAudioRuntime& operator=(const SdlAudioRuntime&) = delete;
    SdlAudioRuntime(SdlAudioRuntime&&) = delete;
    SdlAudioRuntime& operator=(SdlAudioRuntime&&) = delete;

  private:
    std::uint32_t flags_{0};
};

struct SdlAudioDeviceDesc {
    AudioDeviceFormat format;
    bool start_paused{true};
    std::string stream_name;
};

class SdlAudioDevice final {
  public:
    explicit SdlAudioDevice(SdlAudioDeviceDesc desc);
    ~SdlAudioDevice();

    SdlAudioDevice(const SdlAudioDevice&) = delete;
    SdlAudioDevice& operator=(const SdlAudioDevice&) = delete;
    SdlAudioDevice(SdlAudioDevice&&) noexcept;
    SdlAudioDevice& operator=(SdlAudioDevice&&) noexcept;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] bool paused() const;
    [[nodiscard]] std::uint32_t queued_frames() const;
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

[[nodiscard]] bool sdl3_audio_device_format_supported(AudioDeviceFormat format) noexcept;

} // namespace mirakana
