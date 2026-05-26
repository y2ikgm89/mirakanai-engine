// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <cstddef>
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

enum class AudioGameplayCueKind : std::uint8_t {
    music,
    sfx,
    ambience,
    ui,
    voice,
    custom,
};

struct AudioGameplayBusMixDesc {
    std::string name{"master"};
    float gain{1.0F};
    bool muted{false};
    bool paused{false};
    float fade_from_gain{1.0F};
    float fade_to_gain{1.0F};
    float fade_elapsed_seconds{0.0F};
    float fade_duration_seconds{0.0F};
};

struct AudioGameplayCueDesc {
    std::string id;
    AudioGameplayCueKind kind{AudioGameplayCueKind::sfx};
    AssetId clip;
    std::string bus{"master"};
    float gain{1.0F};
    bool looping{false};
    bool spatialized{false};
    AudioPoint3 position{};
    float min_distance{1.0F};
    float max_distance{64.0F};
};

struct AudioGameplayCueTrigger {
    std::string cue_id;
    std::uint64_t start_frame{0};
    float gain_scale{1.0F};
};

struct AudioGameplayMixRequest {
    std::vector<AudioGameplayBusMixDesc> buses;
    std::vector<AudioGameplayCueDesc> cues;
    std::vector<AudioGameplayCueTrigger> triggers;
};

enum class AudioGameplayMixDiagnosticCode : std::uint8_t {
    missing_bus_name,
    duplicate_bus,
    invalid_bus_gain,
    invalid_bus_fade,
    missing_cue_id,
    duplicate_cue_id,
    unsupported_cue_kind,
    missing_clip,
    unknown_bus,
    invalid_cue_gain,
    invalid_spatial_range,
    missing_trigger_cue_id,
    unknown_cue,
    invalid_trigger_gain,
};

struct AudioGameplayMixDiagnostic {
    AudioGameplayMixDiagnosticCode code{AudioGameplayMixDiagnosticCode::missing_cue_id};
    std::string bus;
    std::string cue_id;
    std::string message;
};

struct AudioGameplayMixCommand {
    std::string cue_id;
    AudioGameplayCueKind kind{AudioGameplayCueKind::sfx};
    AudioVoiceDesc voice;
    bool spatialized{false};
    AudioPoint3 position{};
    float min_distance{1.0F};
    float max_distance{64.0F};
};

struct AudioGameplayMixPlan {
    std::vector<AudioBusDesc> buses;
    std::vector<AudioGameplayMixCommand> commands;
    std::vector<AudioGameplayMixDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
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

enum class AudioProductionReadinessStatus : std::uint8_t {
    ready,
    host_evidence_required,
    invalid_request,
};

enum class AudioProductionDspNodeKind : std::uint8_t {
    gain,
    limiter,
    filter,
    send,
};

enum class AudioProductionUnsupportedClaimKind : std::uint8_t {
    broad_codec_support,
    middleware_parity,
    native_device_handle_access,
    background_streaming_execution,
    subjective_mix_quality,
    hrtf_execution,
    platform_device_parity,
};

enum class AudioProductionDiagnosticCode : std::uint8_t {
    missing_official_source_review,
    missing_decoded_source,
    invalid_decoded_source,
    missing_streaming_chunk,
    invalid_streaming_chunk,
    missing_format_conversion_policy,
    invalid_format_conversion_policy,
    invalid_voice_budget,
    invalid_bus_budget,
    row_budget_exceeded,
    missing_dsp_graph,
    invalid_dsp_graph,
    missing_spatial_listener,
    invalid_spatial_evidence,
    missing_spatial_source,
    missing_hrtf_host_gate,
    missing_device_lifecycle,
    invalid_device_lifecycle,
    missing_device_host_evidence,
    unsupported_audio_claim,
    native_handle_exposure,
    side_effect_claim,
};

struct AudioProductionDecodedSourceEvidenceRow {
    AssetId clip;
    AudioDeviceFormat format;
    std::uint64_t frame_count{0};
    std::uint64_t decoded_byte_count{0};
    bool reviewed{false};
    std::uint32_t source_index{0};
};

struct AudioProductionStreamingChunkEvidenceRow {
    AudioStreamingChunkDesc chunk;
    std::uint64_t queued_frame_count{0};
    bool reviewed{false};
    std::uint32_t source_index{0};
};

struct AudioProductionFormatConversionPolicyRow {
    AssetId clip;
    AudioDeviceFormat source_format;
    AudioDeviceFormat device_format;
    AudioResamplingQuality resampling_quality{AudioResamplingQuality::linear};
    bool reviewed{false};
    std::uint32_t source_index{0};
};

struct AudioProductionDspGraphRow {
    std::string node_id;
    AudioProductionDspNodeKind kind{AudioProductionDspNodeKind::gain};
    std::uint32_t input_count{0};
    std::uint32_t output_count{0};
    bool deterministic{false};
    bool reviewed{false};
    std::uint32_t source_index{0};
};

struct AudioProductionDeviceLifecycleRow {
    std::string backend_id;
    bool uses_logical_device{false};
    bool uses_audio_stream{false};
    bool uses_queueing{false};
    bool uses_callback{false};
    bool can_pause_resume{false};
    bool can_clear{false};
    bool host_evidence_available{false};
    bool native_handle_exposed{false};
    std::uint32_t source_index{0};
};

struct AudioProductionUnsupportedClaimRow {
    AudioProductionUnsupportedClaimKind kind{AudioProductionUnsupportedClaimKind::broad_codec_support};
    std::string claim_id;
    bool requested{false};
    std::uint32_t source_index{0};
};

struct AudioProductionReviewRequest {
    std::vector<AudioProductionDecodedSourceEvidenceRow> decoded_sources;
    std::vector<AudioProductionStreamingChunkEvidenceRow> streaming_chunks;
    std::vector<AudioProductionFormatConversionPolicyRow> format_conversion_policies;
    std::vector<AudioProductionDspGraphRow> dsp_graph_rows;
    AudioSpatialListenerDesc listener;
    std::vector<AudioSpatialVoiceDesc> spatial_voices;
    std::vector<AudioProductionDeviceLifecycleRow> device_lifecycle_rows;
    std::vector<AudioProductionUnsupportedClaimRow> unsupported_claim_rows;
    std::size_t max_voice_budget{0};
    std::size_t active_voice_count{0};
    std::size_t max_bus_budget{0};
    std::size_t active_bus_count{0};
    std::size_t row_budget{0};
    bool official_sources_reviewed{false};
    bool hrtf_host_evidence_available{false};
    bool request_native_device_handles{false};
    bool invoked_codec_decode{false};
    bool invoked_background_streaming{false};
    bool invoked_middleware{false};
    bool invoked_hrtf{false};
    bool invoked_device_callback{false};
    bool invoked_device_io{false};
    std::uint64_t seed{0};
};

struct AudioProductionDiagnostic {
    AudioProductionDiagnosticCode code{AudioProductionDiagnosticCode::missing_official_source_review};
    std::string evidence_id;
    std::string message;
    std::uint32_t source_index{0};
};

struct AudioProductionReadinessPlan {
    AudioProductionReadinessStatus status{AudioProductionReadinessStatus::invalid_request};
    bool production_audio_ready{false};
    bool selected_package_evidence_ready{false};
    bool reviewed{false};
    std::size_t decoded_source_rows{0};
    std::size_t streaming_chunk_rows{0};
    std::size_t format_conversion_policy_rows{0};
    std::size_t bus_budget_rows{0};
    std::size_t voice_budget_rows{0};
    std::size_t dsp_graph_rows{0};
    std::size_t listener_rows{0};
    std::size_t spatial_source_rows{0};
    std::size_t hrtf_host_gate_rows{0};
    std::size_t device_lifecycle_rows{0};
    bool device_host_evidence_available{false};
    bool hrtf_host_evidence_available{false};
    std::size_t unsupported_claim_rows{0};
    bool requested_native_device_handles{false};
    bool invoked_codec_decode{false};
    bool invoked_background_streaming{false};
    bool invoked_middleware{false};
    bool invoked_hrtf{false};
    bool invoked_device_callback{false};
    bool invoked_device_io{false};
    std::vector<AudioProductionDiagnostic> diagnostics;
    std::uint64_t replay_hash{0};

    [[nodiscard]] bool succeeded() const noexcept;
};

class AudioMixer;

[[nodiscard]] bool is_valid_audio_bus_desc(const AudioBusDesc& bus) noexcept;
[[nodiscard]] bool is_valid_audio_voice_desc(const AudioVoiceDesc& voice) noexcept;
[[nodiscard]] bool is_valid_audio_spatial_listener_desc(const AudioSpatialListenerDesc& listener) noexcept;
[[nodiscard]] bool is_valid_audio_spatial_voice_desc(const AudioSpatialVoiceDesc& voice) noexcept;
[[nodiscard]] bool is_valid_audio_gameplay_cue_kind(AudioGameplayCueKind kind) noexcept;
[[nodiscard]] bool is_valid_audio_clip_desc(const AudioClipDesc& clip) noexcept;
[[nodiscard]] bool is_valid_audio_device_format(const AudioDeviceFormat& format) noexcept;
[[nodiscard]] bool is_valid_audio_render_request(const AudioRenderRequest& request) noexcept;
[[nodiscard]] bool is_valid_audio_clip_sample_data(const AudioClipSampleData& samples) noexcept;
[[nodiscard]] bool is_valid_audio_device_stream_request(const AudioDeviceStreamRequest& request) noexcept;
[[nodiscard]] AudioGameplayMixPlan plan_gameplay_audio_mix(const AudioGameplayMixRequest& request);
[[nodiscard]] AudioDeviceStreamPlan plan_audio_device_stream(AudioDeviceStreamRequest request) noexcept;
[[nodiscard]] AudioProductionReadinessPlan
review_audio_production_readiness(const AudioProductionReviewRequest& request);
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
