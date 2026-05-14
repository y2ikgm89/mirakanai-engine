// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mirakana {

struct AudioVoiceId {
    std::uint32_t value{0};

    friend constexpr bool operator==(AudioVoiceId lhs, AudioVoiceId rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

inline constexpr AudioVoiceId null_audio_voice{};

struct AudioBusDesc {
    std::string name{"master"};
    float gain{1.0F};
    bool muted{false};
};

struct AudioVoiceDesc {
    AssetId clip;
    std::string bus{"master"};
    float gain{1.0F};
    bool looping{false};
    std::uint64_t start_frame{0};
};

struct AudioPoint3 {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct AudioSpatialListenerDesc {
    AudioPoint3 position{};
    AudioPoint3 right{.x = 1.0F, .y = 0.0F, .z = 0.0F};
};

struct AudioSpatialVoiceDesc {
    AudioVoiceId voice;
    AudioPoint3 position{};
    float min_distance{1.0F};
    float max_distance{64.0F};
    // When false, spatial_mix_plan ignores position and distance fields and uses the base voice gain.
    bool spatialized{true};
};

struct AudioMixCommand {
    AudioVoiceId voice;
    AssetId clip;
    std::string bus;
    float gain{1.0F};
    bool looping{false};
};

struct AudioSpatialMixCommand {
    AudioVoiceId voice;
    AssetId clip;
    std::string bus;
    bool spatialized{false};
    float distance{0.0F};
    float attenuation{1.0F};
    float pan{0.0F};
    float center_gain{1.0F};
    float left_gain{1.0F};
    float right_gain{1.0F};
    bool looping{false};
};

enum class AudioSampleFormat : std::uint8_t { unknown, float32, pcm16 };

enum class AudioResamplingQuality : std::uint8_t { nearest, linear };

enum class AudioFormatConversion : std::uint8_t {
    none = 0,
    resample = 1U << 0U,
    channel_convert = 1U << 1U,
    sample_format_convert = 1U << 2U,
};

[[nodiscard]] constexpr AudioFormatConversion operator|(AudioFormatConversion lhs, AudioFormatConversion rhs) noexcept {
    return static_cast<AudioFormatConversion>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] constexpr AudioFormatConversion operator&(AudioFormatConversion lhs, AudioFormatConversion rhs) noexcept {
    return static_cast<AudioFormatConversion>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
}

constexpr AudioFormatConversion& operator|=(AudioFormatConversion& lhs, AudioFormatConversion rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

struct AudioClipDesc {
    AssetId clip;
    std::uint32_t sample_rate{48000};
    std::uint32_t channel_count{2};
    std::uint64_t frame_count{0};
    AudioSampleFormat sample_format{AudioSampleFormat::float32};
    bool streaming{false};
    std::uint64_t buffered_frame_count{0};
};

struct AudioDeviceFormat {
    std::uint32_t sample_rate{48000};
    std::uint32_t channel_count{2};
    AudioSampleFormat sample_format{AudioSampleFormat::float32};
};

struct AudioRenderRequest {
    AudioDeviceFormat format;
    std::uint32_t frame_count{0};
    std::uint64_t device_frame{0};
    std::uint32_t underrun_warning_threshold_frames{0};
    AudioResamplingQuality resampling_quality{AudioResamplingQuality::linear};
};

struct AudioRenderCommand {
    AudioVoiceId voice;
    AssetId clip;
    std::string bus;
    float gain{1.0F};
    std::uint64_t output_offset_frames{0};
    std::uint64_t output_frame_count{0};
    std::uint64_t source_start_frame{0};
    std::uint64_t source_frame_count{0};
    AudioFormatConversion conversion{AudioFormatConversion::none};
    AudioResamplingQuality resampling_quality{AudioResamplingQuality::linear};
    bool looping{false};
};

struct AudioUnderrunDiagnostic {
    AudioVoiceId voice;
    AssetId clip;
    std::uint64_t requested_source_frames{0};
    std::uint64_t buffered_source_frames{0};
    std::uint64_t missing_source_frames{0};
};

struct AudioRenderPlan {
    AudioDeviceFormat format;
    std::uint32_t requested_output_frames{0};
    std::uint64_t device_frame{0};
    std::vector<AudioRenderCommand> commands;
    std::vector<AudioUnderrunDiagnostic> underruns;
};

struct AudioClipSampleData {
    AssetId clip;
    AudioDeviceFormat format;
    std::uint64_t frame_count{0};
    std::vector<float> interleaved_float_samples;
};

struct AudioRenderBuffer {
    AudioDeviceFormat format;
    std::uint32_t frame_count{0};
    std::uint64_t device_frame{0};
    std::vector<float> interleaved_float_samples;
    AudioRenderPlan plan;
};

enum class AudioDeviceStreamStatus : std::uint8_t { ready, no_work, invalid_request };

enum class AudioDeviceStreamDiagnostic : std::uint8_t {
    none,
    invalid_format,
    invalid_queue_target,
    invalid_render_budget,
    invalid_resampling_quality,
    device_frame_overflow,
};

struct AudioDeviceStreamRequest {
    AudioDeviceFormat format;
    std::uint64_t device_frame{0};
    std::uint32_t queued_frames{0};
    std::uint32_t target_queued_frames{1024};
    std::uint32_t max_render_frames{512};
    std::uint32_t underrun_warning_threshold_frames{0};
    AudioResamplingQuality resampling_quality{AudioResamplingQuality::linear};
};

struct AudioDeviceStreamPlan {
    AudioDeviceStreamStatus status{AudioDeviceStreamStatus::invalid_request};
    AudioDeviceStreamDiagnostic diagnostic{AudioDeviceStreamDiagnostic::none};
    AudioRenderRequest render_request;
    std::uint32_t queued_frames_before{0};
    std::uint32_t target_queued_frames{0};
    std::uint32_t frames_to_render{0};
    std::uint32_t queued_frames_after{0};
};

struct AudioDeviceStreamRenderResult {
    AudioDeviceStreamPlan plan;
    AudioRenderBuffer buffer;
};

struct AudioStreamingChunkDesc {
    AssetId clip;
    AudioDeviceFormat format;
    std::uint64_t start_frame{0};
    std::uint64_t frame_count{0};
};

struct AudioStreamingConsumeResult {
    std::uint64_t requested_frames{0};
    std::uint64_t consumed_frames{0};
    std::uint64_t missing_frames{0};
    std::uint64_t next_read_cursor_frame{0};
    bool starved{false};
};

class AudioMixer;

[[nodiscard]] bool is_valid_audio_bus_desc(const AudioBusDesc& bus) noexcept;
[[nodiscard]] bool is_valid_audio_voice_desc(const AudioVoiceDesc& voice) noexcept;
[[nodiscard]] bool is_valid_audio_spatial_listener_desc(const AudioSpatialListenerDesc& listener) noexcept;
[[nodiscard]] bool is_valid_audio_spatial_voice_desc(const AudioSpatialVoiceDesc& voice) noexcept;
[[nodiscard]] bool is_valid_audio_clip_desc(const AudioClipDesc& clip) noexcept;
[[nodiscard]] bool is_valid_audio_device_format(const AudioDeviceFormat& format) noexcept;
[[nodiscard]] bool is_valid_audio_render_request(const AudioRenderRequest& request) noexcept;
[[nodiscard]] bool is_valid_audio_clip_sample_data(const AudioClipSampleData& samples) noexcept;
[[nodiscard]] bool is_valid_audio_device_stream_request(const AudioDeviceStreamRequest& request) noexcept;
[[nodiscard]] AudioDeviceStreamPlan plan_audio_device_stream(AudioDeviceStreamRequest request) noexcept;
[[nodiscard]] AudioDeviceStreamRenderResult
render_audio_device_stream_interleaved_float(const AudioMixer& mixer, AudioDeviceStreamRequest request,
                                             std::span<const AudioClipSampleData> samples);

class AudioMixer {
  public:
    AudioMixer();

    [[nodiscard]] bool try_add_bus(AudioBusDesc bus);
    void add_bus(AudioBusDesc bus);

    void set_bus_gain(std::string_view bus, float gain);
    void set_bus_muted(std::string_view bus, bool muted);

    [[nodiscard]] AudioVoiceId play(AudioVoiceDesc voice);
    [[nodiscard]] std::vector<AudioMixCommand> mix_plan() const;
    [[nodiscard]] std::vector<AudioSpatialMixCommand>
    spatial_mix_plan(AudioSpatialListenerDesc listener, std::span<const AudioSpatialVoiceDesc> spatial_voices) const;

    [[nodiscard]] bool register_clip(AudioClipDesc clip);
    void add_clip(AudioClipDesc clip);
    void set_clip_buffered_frames(AssetId clip, std::uint64_t buffered_frame_count);
    [[nodiscard]] const AudioClipDesc* find_clip(AssetId clip) const noexcept;
    [[nodiscard]] AudioRenderPlan render_plan(AudioRenderRequest request) const;
    [[nodiscard]] AudioRenderBuffer render_interleaved_float(AudioRenderRequest request,
                                                             std::span<const AudioClipSampleData> samples) const;

  private:
    struct AudioBusState {
        AudioBusDesc desc;
    };

    struct AudioVoiceState {
        AudioVoiceId id;
        AudioVoiceDesc desc;
    };

    [[nodiscard]] AudioBusState* find_bus(std::string_view bus) noexcept;
    [[nodiscard]] const AudioBusState* find_bus(std::string_view bus) const noexcept;

    std::vector<AudioBusState> buses_;
    std::unordered_map<std::string, std::size_t> bus_indices_;
    std::vector<AudioVoiceState> voices_;
    std::unordered_map<AssetId, AudioClipDesc, AssetIdHash> clips_;
};

class AudioStreamingQueue {
  public:
    explicit AudioStreamingQueue(AudioClipDesc clip);

    [[nodiscard]] const AudioClipDesc& clip() const noexcept;
    [[nodiscard]] std::uint64_t read_cursor_frame() const noexcept;
    [[nodiscard]] std::uint64_t buffered_end_frame() const noexcept;
    [[nodiscard]] std::uint64_t buffered_frame_count() const noexcept;

    [[nodiscard]] bool append(AudioStreamingChunkDesc chunk) noexcept;
    [[nodiscard]] AudioStreamingConsumeResult consume(std::uint64_t requested_frames) noexcept;

  private:
    AudioClipDesc clip_;
    std::uint64_t read_cursor_frame_{0};
    std::uint64_t buffered_end_frame_{0};
};

} // namespace mirakana
