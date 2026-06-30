// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/two_d_commercial_renderer_rhi_quality.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace {

using mirakana::TwoDCommercialRendererRhiQualityDiagnosticCode;
using mirakana::TwoDCommercialRendererRhiQualityEvidenceKind;
using mirakana::TwoDCommercialRendererRhiQualityEvidenceRow;
using mirakana::TwoDCommercialRendererRhiQualityEvidenceStatus;
using mirakana::TwoDCommercialRendererRhiQualityOfficialSourceKind;
using mirakana::TwoDCommercialRendererRhiQualityOfficialSourceRow;
using mirakana::TwoDCommercialRendererRhiQualityStatus;

[[nodiscard]] bool has_diagnostic(const std::vector<mirakana::TwoDCommercialRendererRhiQualityDiagnostic>& diagnostics,
                                  TwoDCommercialRendererRhiQualityDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] std::array<TwoDCommercialRendererRhiQualityOfficialSourceRow, 4U> make_source_rows() {
    return {
        TwoDCommercialRendererRhiQualityOfficialSourceRow{
            .id = "Microsoft-D3D12-Direct3D12-Command-Fence-Barrier-Descriptor-2026-07-01",
            .kind = TwoDCommercialRendererRhiQualityOfficialSourceKind::microsoft_direct3d12,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        TwoDCommercialRendererRhiQualityOfficialSourceRow{
            .id = "Khronos-Vulkan-Synchronization2-Timestamp-Validation-2026-07-01",
            .kind = TwoDCommercialRendererRhiQualityOfficialSourceKind::khronos_vulkan,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        TwoDCommercialRendererRhiQualityOfficialSourceRow{
            .id = "Apple-Metal-Shading-Language-Resource-Binding-2026-07-01",
            .kind = TwoDCommercialRendererRhiQualityOfficialSourceKind::apple_metal,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        TwoDCommercialRendererRhiQualityOfficialSourceRow{
            .id = "MIRAIKANAI-Legal-Clean-Room-Policy-2026-07-01",
            .kind = TwoDCommercialRendererRhiQualityOfficialSourceKind::repository_legal_policy,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
    };
}

[[nodiscard]] mirakana::SpriteBatchProductionThroughputPlan make_sprite_throughput() {
    return mirakana::SpriteBatchProductionThroughputPlan{
        .status = mirakana::SpriteBatchProductionThroughputStatus::ready,
        .diagnostics = {},
        .total_draw_rows = 12U,
        .total_instance_rows = 4096U,
        .total_upload_bytes = 262144U,
        .measured_workload_rows = 1U,
        .host_gated_workload_rows = 1U,
        .claimed_broad_optimization = false,
        .claimed_cross_backend_parity = false,
        .claimed_metal_readiness = false,
    };
}

[[nodiscard]] TwoDCommercialRendererRhiQualityEvidenceRow
make_ready_row(const char* row_id, TwoDCommercialRendererRhiQualityEvidenceKind kind,
               mirakana::rhi::BackendKind backend) {
    return TwoDCommercialRendererRhiQualityEvidenceRow{
        .row_id = row_id,
        .kind = kind,
        .backend = backend,
        .status = TwoDCommercialRendererRhiQualityEvidenceStatus::ready,
        .selected = true,
        .reviewed = true,
        .ready = true,
        .host_gated = false,
        .validation_recipe_id = "2d-commercial-renderer-rhi-quality",
        .package_counter_id = "2d_commercial_renderer_rhi_quality.ready",
        .command_allocator_fence_ready = true,
        .command_list_close_execute_reset_ready = true,
        .descriptor_heap_binding_ready = true,
        .pipeline_state_reuse_ready = true,
        .resource_barrier_ready = true,
        .debug_validation_clean = true,
        .timestamp_or_pix_evidence_ready = true,
        .package_visible_readback_ready = true,
        .sprite_batch_budget_ready = true,
        .atlas_residency_budget_ready = true,
        .texture_upload_scheduling_ready = true,
        .frame_pacing_budget_ready = true,
        .source_index = 1U,
    };
}

[[nodiscard]] mirakana::TwoDCommercialRendererRhiQualityDesc make_ready_desc() {
    return mirakana::TwoDCommercialRendererRhiQualityDesc{
        .sprite_throughput = make_sprite_throughput(),
        .official_source_rows = make_source_rows(),
        .evidence_rows =
            {
                make_ready_row("renderer_rhi.d3d12.official_patterns",
                               TwoDCommercialRendererRhiQualityEvidenceKind::backend_host_evidence,
                               mirakana::rhi::BackendKind::d3d12),
                make_ready_row("renderer_rhi.sprite_batch.package",
                               TwoDCommercialRendererRhiQualityEvidenceKind::sprite_batch_package,
                               mirakana::rhi::BackendKind::null),
                make_ready_row("renderer_rhi.atlas_upload.package",
                               TwoDCommercialRendererRhiQualityEvidenceKind::atlas_residency_upload,
                               mirakana::rhi::BackendKind::null),
                make_ready_row("renderer_rhi.frame_pacing.package",
                               TwoDCommercialRendererRhiQualityEvidenceKind::frame_pacing_budget,
                               mirakana::rhi::BackendKind::null),
                make_ready_row("renderer_rhi.claim_control",
                               TwoDCommercialRendererRhiQualityEvidenceKind::claim_control,
                               mirakana::rhi::BackendKind::null),
                make_ready_row("renderer_rhi.clean_room", TwoDCommercialRendererRhiQualityEvidenceKind::clean_room,
                               mirakana::rhi::BackendKind::null),
            },
        .selected_d3d12_ready_claim = true,
        .seed = 7301U,
    };
}

} // namespace

MK_TEST("2d commercial renderer rhi quality accepts selected D3D12 proof without broad backend claims") {
    const auto result = mirakana::evaluate_2d_commercial_renderer_rhi_quality(make_ready_desc());

    MK_REQUIRE(result.status == TwoDCommercialRendererRhiQualityStatus::ready);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.selected_d3d12_renderer_rhi_quality_ready);
    MK_REQUIRE(!result.vulkan_strict_renderer_rhi_quality_ready);
    MK_REQUIRE(!result.metal_renderer_rhi_quality_ready);
    MK_REQUIRE(!result.broad_backend_parity_ready);
    MK_REQUIRE(!result.broad_renderer_rhi_quality_ready);
    MK_REQUIRE(result.official_source_ledger_ready);
    MK_REQUIRE(result.sprite_batch_budget_ready);
    MK_REQUIRE(result.d3d12_command_allocator_fence_ready);
    MK_REQUIRE(result.d3d12_descriptor_heap_binding_ready);
    MK_REQUIRE(result.d3d12_pipeline_state_reuse_ready);
    MK_REQUIRE(result.d3d12_resource_barrier_ready);
    MK_REQUIRE(result.d3d12_debug_validation_clean);
    MK_REQUIRE(result.d3d12_timestamp_or_pix_evidence_ready);
    MK_REQUIRE(result.package_visible_readback_ready);
    MK_REQUIRE(result.host_gated_row_count == 0U);
    MK_REQUIRE(result.public_native_handle_rows == 0U);
    MK_REQUIRE(result.cross_backend_inference_rows == 0U);
    MK_REQUIRE(result.external_engine_claim_rows == 0U);
    MK_REQUIRE(result.legal_approval_claim_rows == 0U);
    MK_REQUIRE(result.replay_hash != 0U);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("2d commercial renderer rhi quality keeps missing D3D12 official patterns host gated") {
    auto desc = make_ready_desc();
    desc.evidence_rows[0].command_allocator_fence_ready = false;
    desc.evidence_rows[0].resource_barrier_ready = false;
    desc.evidence_rows[0].debug_validation_clean = false;
    desc.evidence_rows[0].timestamp_or_pix_evidence_ready = false;

    const auto result = mirakana::evaluate_2d_commercial_renderer_rhi_quality(desc);

    MK_REQUIRE(result.status == TwoDCommercialRendererRhiQualityStatus::host_evidence_required);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.selected_d3d12_renderer_rhi_quality_ready);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRendererRhiQualityDiagnosticCode::missing_d3d12_evidence));
}

MK_TEST("2d commercial renderer rhi quality requires sprite throughput budget evidence") {
    auto desc = make_ready_desc();
    desc.sprite_throughput.status = mirakana::SpriteBatchProductionThroughputStatus::budget_exceeded;
    desc.sprite_throughput.rows.push_back(mirakana::SpriteBatchProductionWorkloadRow{.over_budget = true});
    desc.evidence_rows[1].sprite_batch_budget_ready = false;

    const auto result = mirakana::evaluate_2d_commercial_renderer_rhi_quality(desc);

    MK_REQUIRE(result.status == TwoDCommercialRendererRhiQualityStatus::host_evidence_required);
    MK_REQUIRE(!result.sprite_batch_budget_ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              TwoDCommercialRendererRhiQualityDiagnosticCode::missing_sprite_batch_budget));
}

MK_TEST("2d commercial renderer rhi quality rejects native handle and cross backend inference rows") {
    auto desc = make_ready_desc();
    desc.evidence_rows[0].public_native_handles_exposed = true;
    desc.evidence_rows[0].request_cross_backend_inference = true;
    desc.metal_ready_claim = true;

    const auto result = mirakana::evaluate_2d_commercial_renderer_rhi_quality(desc);

    MK_REQUIRE(result.status == TwoDCommercialRendererRhiQualityStatus::invalid_request);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.public_native_handle_rows == 1U);
    MK_REQUIRE(result.cross_backend_inference_rows == 1U);
    MK_REQUIRE(result.replay_hash == 0U);
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              TwoDCommercialRendererRhiQualityDiagnosticCode::public_native_handle_access));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRendererRhiQualityDiagnosticCode::cross_backend_inference));
}

MK_TEST("2d commercial renderer rhi quality rejects external engine and legal approval claims") {
    auto desc = make_ready_desc();
    desc.evidence_rows[5].external_engine_code_used = true;
    desc.evidence_rows[5].external_engine_asset_used = true;
    desc.evidence_rows[5].external_engine_compatibility_claim = true;
    desc.evidence_rows[5].legal_approval_claim = true;

    const auto result = mirakana::evaluate_2d_commercial_renderer_rhi_quality(desc);

    MK_REQUIRE(result.status == TwoDCommercialRendererRhiQualityStatus::invalid_request);
    MK_REQUIRE(result.external_engine_claim_rows == 3U);
    MK_REQUIRE(result.legal_approval_claim_rows == 1U);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRendererRhiQualityDiagnosticCode::external_engine_claim));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRendererRhiQualityDiagnosticCode::legal_approval_claim));
}

MK_TEST("2d commercial renderer rhi quality rejects stale official source rows") {
    auto desc = make_ready_desc();
    desc.official_source_rows[0].id = "Microsoft-D3D12-Direct3D12-Command-Fence-Barrier-Descriptor-2026-06-01";

    const auto result = mirakana::evaluate_2d_commercial_renderer_rhi_quality(desc);

    MK_REQUIRE(result.status == TwoDCommercialRendererRhiQualityStatus::invalid_request);
    MK_REQUIRE(!result.official_source_ledger_ready);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRendererRhiQualityDiagnosticCode::stale_official_source));
}

int main() {
    return mirakana::test::run_all();
}
