// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/audio/audio_mixer.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace mirakana_android {

struct AndroidAudioOutputDesc {
    mirakana::AudioDeviceFormat format;
    std::uint32_t queue_capacity_frames{48000};
    bool start_immediately{false};
};

class AndroidAudioOutput final {
  public:
    explicit AndroidAudioOutput(AndroidAudioOutputDesc desc);
    ~AndroidAudioOutput();

    AndroidAudioOutput(const AndroidAudioOutput&) = delete;
    AndroidAudioOutput& operator=(const AndroidAudioOutput&) = delete;
    AndroidAudioOutput(AndroidAudioOutput&&) noexcept;
    AndroidAudioOutput& operator=(AndroidAudioOutput&&) noexcept;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] bool started() const noexcept;
    [[nodiscard]] std::uint32_t queued_frames() const noexcept;
    [[nodiscard]] const mirakana::AudioDeviceFormat& format() const noexcept;
    [[nodiscard]] std::string_view backend_name() const noexcept;

    void queue_interleaved_float_frames(std::span<const float> samples);
    void clear() noexcept;
    void start();
    void stop() noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] bool android_audio_output_format_supported(mirakana::AudioDeviceFormat format) noexcept;

} // namespace mirakana_android
