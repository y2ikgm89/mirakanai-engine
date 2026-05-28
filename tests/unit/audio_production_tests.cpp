// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/audio/audio_mixer.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

[[nodiscard]] mirakana::AudioDeviceFormat package_format() noexcept {
    return mirakana::AudioDeviceFormat{
        .sample_rate = 48000,
        .channel_count = 2,
        .sample_format = mirakana::AudioSampleFormat::float32,
    };
}

[[nodiscard]] mirakana::AudioProductionDecodedSourceEvidenceRow decoded_source(mirakana::AssetId clip) {
    return mirakana::AudioProductionDecodedSourceEvidenceRow{
        .clip = clip,
        .format = package_format(),
        .frame_count = 4U,
        .decoded_byte_count = 32U,
        .reviewed = true,
        .source_index = 1U,
    };
}

[[nodiscard]] mirakana::AudioProductionStreamingChunkEvidenceRow streaming_chunk(mirakana::AssetId clip) {
    return mirakana::AudioProductionStreamingChunkEvidenceRow{
        .chunk =
            mirakana::AudioStreamingChunkDesc{
                .clip = clip,
                .format = package_format(),
                .start_frame = 0U,
                .frame_count = 4U,
            },
        .queued_frame_count = 4U,
        .reviewed = true,
        .source_index = 2U,
    };
}

[[nodiscard]] mirakana::AudioProductionFormatConversionPolicyRow conversion_policy(mirakana::AssetId clip) {
    return mirakana::AudioProductionFormatConversionPolicyRow{
        .clip = clip,
        .source_format = package_format(),
        .device_format = package_format(),
        .resampling_quality = mirakana::AudioResamplingQuality::linear,
        .reviewed = true,
        .source_index = 3U,
    };
}

[[nodiscard]] mirakana::AudioProductionDspGraphRow dsp_row() {
    return mirakana::AudioProductionDspGraphRow{
        .node_id = "limiter.master",
        .kind = mirakana::AudioProductionDspNodeKind::limiter,
        .input_count = 1U,
        .output_count = 1U,
        .deterministic = true,
        .reviewed = true,
        .source_index = 4U,
    };
}

[[nodiscard]] mirakana::AudioProductionDeviceLifecycleRow device_lifecycle(bool host_evidence) {
    return mirakana::AudioProductionDeviceLifecycleRow{
        .backend_id = "wasapi",
        .uses_logical_device = true,
        .uses_audio_stream = true,
        .uses_queueing = true,
        .uses_callback = false,
        .can_pause_resume = true,
        .can_clear = true,
        .host_evidence_available = host_evidence,
        .native_handle_exposed = false,
        .source_index = 5U,
    };
}

[[nodiscard]] mirakana::AudioProductionReviewRequest make_request(bool host_evidence) {
    const auto clip = mirakana::AssetId::from_name("audio/production/jump");
    return mirakana::AudioProductionReviewRequest{
        .decoded_sources = {decoded_source(clip)},
        .streaming_chunks = {streaming_chunk(clip)},
        .format_conversion_policies = {conversion_policy(clip)},
        .dsp_graph_rows = {dsp_row()},
        .listener =
            mirakana::AudioSpatialListenerDesc{
                .position = mirakana::AudioPoint3{},
                .right = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
            },
        .spatial_voices =
            {
                mirakana::AudioSpatialVoiceDesc{
                    .voice = mirakana::AudioVoiceId{1U},
                    .position = mirakana::AudioPoint3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
                    .min_distance = 1.0F,
                    .max_distance = 8.0F,
                    .spatialized = true,
                },
            },
        .device_lifecycle_rows = {device_lifecycle(host_evidence)},
        .unsupported_claim_rows = {},
        .max_voice_budget = 8U,
        .active_voice_count = 1U,
        .max_bus_budget = 4U,
        .active_bus_count = 2U,
        .row_budget = 32U,
        .official_sources_reviewed = true,
        .hrtf_host_evidence_available = host_evidence,
        .request_native_device_handles = false,
        .invoked_codec_decode = false,
        .invoked_background_streaming = false,
        .invoked_middleware = false,
        .invoked_hrtf = false,
        .invoked_device_callback = false,
        .invoked_device_io = false,
        .seed = 77U,
    };
}

} // namespace

MK_TEST("audio production readiness accepts decoded streaming dsp spatial and device evidence") {
    const auto plan = mirakana::review_audio_production_readiness(make_request(true));

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::AudioProductionReadinessStatus::ready);
    MK_REQUIRE(plan.production_audio_ready);
    MK_REQUIRE(plan.reviewed);
    MK_REQUIRE(plan.decoded_source_rows == 1U);
    MK_REQUIRE(plan.streaming_chunk_rows == 1U);
    MK_REQUIRE(plan.format_conversion_policy_rows == 1U);
    MK_REQUIRE(plan.bus_budget_rows == 1U);
    MK_REQUIRE(plan.voice_budget_rows == 1U);
    MK_REQUIRE(plan.dsp_graph_rows == 1U);
    MK_REQUIRE(plan.listener_rows == 1U);
    MK_REQUIRE(plan.spatial_source_rows == 1U);
    MK_REQUIRE(plan.hrtf_host_gate_rows == 1U);
    MK_REQUIRE(plan.device_lifecycle_rows == 1U);
    MK_REQUIRE(plan.device_host_evidence_available);
    MK_REQUIRE(plan.hrtf_host_evidence_available);
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("audio production readiness separates missing device and hrtf host evidence") {
    const auto plan = mirakana::review_audio_production_readiness(make_request(false));

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::AudioProductionReadinessStatus::host_evidence_required);
    MK_REQUIRE(!plan.production_audio_ready);
    MK_REQUIRE(plan.selected_package_evidence_ready);
    MK_REQUIRE(plan.decoded_source_rows == 1U);
    MK_REQUIRE(plan.streaming_chunk_rows == 1U);
    MK_REQUIRE(plan.dsp_graph_rows == 1U);
    MK_REQUIRE(plan.spatial_source_rows == 1U);
    MK_REQUIRE(plan.hrtf_host_gate_rows == 1U);
    MK_REQUIRE(plan.device_lifecycle_rows == 1U);
    MK_REQUIRE(!plan.device_host_evidence_available);
    MK_REQUIRE(!plan.hrtf_host_evidence_available);
    MK_REQUIRE(plan.diagnostics.size() == 2U);
}

MK_TEST("audio production readiness rejects unsupported codec middleware native handle and background claims") {
    auto request = make_request(true);
    request.unsupported_claim_rows = {
        mirakana::AudioProductionUnsupportedClaimRow{
            .kind = mirakana::AudioProductionUnsupportedClaimKind::broad_codec_support,
            .claim_id = "decode-any-codec",
            .requested = true,
            .source_index = 10U,
        },
        mirakana::AudioProductionUnsupportedClaimRow{
            .kind = mirakana::AudioProductionUnsupportedClaimKind::middleware_parity,
            .claim_id = "middleware-parity",
            .requested = true,
            .source_index = 11U,
        },
        mirakana::AudioProductionUnsupportedClaimRow{
            .kind = mirakana::AudioProductionUnsupportedClaimKind::background_streaming_execution,
            .claim_id = "background-streaming",
            .requested = true,
            .source_index = 12U,
        },
    };
    request.request_native_device_handles = true;
    request.invoked_codec_decode = true;
    request.invoked_background_streaming = true;
    request.invoked_middleware = true;

    const auto plan = mirakana::review_audio_production_readiness(request);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::AudioProductionReadinessStatus::invalid_request);
    MK_REQUIRE(!plan.production_audio_ready);
    MK_REQUIRE(plan.unsupported_claim_rows == 3U);
    MK_REQUIRE(plan.requested_native_device_handles);
    MK_REQUIRE(plan.invoked_codec_decode);
    MK_REQUIRE(plan.invoked_background_streaming);
    MK_REQUIRE(plan.invoked_middleware);
    MK_REQUIRE(plan.diagnostics.size() >= 5U);
}

MK_TEST("audio production readiness rejects incomplete rows budgets and side effects") {
    auto request = make_request(true);
    request.official_sources_reviewed = false;
    request.decoded_sources[0].decoded_byte_count = 0U;
    request.streaming_chunks[0].chunk.frame_count = 0U;
    request.format_conversion_policies[0].reviewed = false;
    request.dsp_graph_rows[0].deterministic = false;
    request.max_voice_budget = 1U;
    request.active_voice_count = 2U;
    request.max_bus_budget = 1U;
    request.active_bus_count = 2U;
    request.spatial_voices[0].max_distance = 0.5F;
    request.device_lifecycle_rows[0].native_handle_exposed = true;
    request.invoked_device_callback = true;
    request.invoked_device_io = true;

    const auto plan = mirakana::review_audio_production_readiness(request);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::AudioProductionReadinessStatus::invalid_request);
    MK_REQUIRE(!plan.reviewed);
    MK_REQUIRE(!plan.selected_package_evidence_ready);
    MK_REQUIRE(plan.diagnostics.size() >= 9U);
    MK_REQUIRE(plan.invoked_device_callback);
    MK_REQUIRE(plan.invoked_device_io);
}

int main() {
    return mirakana::test::run_all();
}
