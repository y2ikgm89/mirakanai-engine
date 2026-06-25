// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/renderer_commercial_readiness_evidence.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace {

using mirakana::RendererCommercialReadinessEvidenceDiagnosticCode;
using mirakana::RendererCommercialReadinessEvidenceKind;
using mirakana::RendererCommercialReadinessPromotionStatus;

[[nodiscard]] mirakana::RendererCommercialQualityCloseoutPlan make_ready_closeout_plan() {
    return mirakana::RendererCommercialQualityCloseoutPlan{
        .status = mirakana::RendererCommercialQualityCloseoutStatus::ready,
        .diagnostics = {},
        .evidence_rows = {},
        .row_count = 12U,
        .ready_row_count = 12U,
        .host_gated_row_count = 0U,
        .dependency_gated_row_count = 0U,
        .unsupported_row_count = 0U,
        .rejected_unsafe_row_count = 0U,
        .native_handle_access_count = 0U,
        .cross_backend_inference_count = 0U,
        .external_engine_parity_count = 0U,
        .external_engine_code_used_count = 0U,
        .external_engine_sample_used_count = 0U,
        .external_engine_asset_used_count = 0U,
        .external_engine_trademark_used_count = 0U,
        .external_engine_compatibility_claim_count = 0U,
        .replay_hash = 4242U,
        .renderer_backend_parity_ready = true,
        .renderer_metal_broad_readiness = true,
        .renderer_broad_quality_ready = true,
        .renderer_commercial_readiness = true,
        .d3d12_renderer_quality_ready = true,
        .vulkan_strict_renderer_quality_ready = true,
        .metal_renderer_quality_ready = true,
        .production_vfx_profiling_ready = true,
        .metal_memory_profiling_ready = true,
        .visible_3d_package_ready = true,
        .runtime_ui_package_ready = true,
        .environment_package_ready = true,
        .generated_game_package_ready = true,
        .claim_control_ready = true,
        .clean_room_source_review_ready = true,
        .third_party_notices_complete = true,
    };
}

[[nodiscard]] std::vector<mirakana::RendererCommercialReadinessSourceRow> make_source_rows() {
    return {
        {.source_id = "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25",
         .reviewed = true,
         .official_or_context7 = true,
         .stale = false},
        {.source_id = "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25",
         .reviewed = true,
         .official_or_context7 = true,
         .stale = false},
        {.source_id = "Apple-Metal-Framework-Memory-Capture-2026-06-25",
         .reviewed = true,
         .official_or_context7 = true,
         .stale = false},
        {.source_id = "Apple-Metal-Shading-Language-Specification-2026-06-25",
         .reviewed = true,
         .official_or_context7 = true,
         .stale = false},
        {.source_id = "Unity-Legal-Terms-2026-06-25", .reviewed = true, .official_or_context7 = true, .stale = false},
        {.source_id = "Epic-Unreal-Engine-EULA-Trademark-2026-06-25",
         .reviewed = true,
         .official_or_context7 = true,
         .stale = false},
        {.source_id = "Godot-Trademark-Licensing-2026-06-25",
         .reviewed = true,
         .official_or_context7 = true,
         .stale = false},
    };
}

[[nodiscard]] mirakana::RendererCommercialReadinessEvidenceRow
make_ready_row(std::string row_id, RendererCommercialReadinessEvidenceKind kind, mirakana::rhi::BackendKind backend,
               std::string source_id, std::uint32_t source_index) {
    const auto metal_memory = kind == RendererCommercialReadinessEvidenceKind::metal_memory_residency ||
                              kind == RendererCommercialReadinessEvidenceKind::metal_profiling_capture;
    return mirakana::RendererCommercialReadinessEvidenceRow{
        .row_id = std::move(row_id),
        .kind = kind,
        .backend = backend,
        .selected = true,
        .ready = true,
        .source_id = std::move(source_id),
        .host_validation_recipe_id =
            metal_memory ? "renderer-metal-memory-profiling-host-evidence" : "renderer-commercial-readiness-evidence",
        .validation_counter_id = "renderer_commercial_readiness.row_ready",
        .package_counter_id = "renderer_commercial_readiness.package_row",
        .artifact_hash_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        .artifact_schema_version = metal_memory ? "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1"
                                                : "GameEngine.RendererCommercialReadinessEvidence.v1",
        .apple_host_metal_artifact = metal_memory,
        .request_native_handle_access = false,
        .request_cross_backend_inference = false,
        .external_engine_code_used = false,
        .external_engine_sample_used = false,
        .external_engine_asset_used = false,
        .external_engine_trademark_used = false,
        .external_engine_ui_expression_used = false,
        .external_engine_api_used = false,
        .external_engine_compatibility_claim = false,
        .external_engine_equivalence_claim = false,
        .external_engine_parity_claim = false,
        .forbidden_material_diagnostic = false,
        .forbidden_material_rejected = false,
        .third_party_notices_complete = true,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::RendererCommercialReadinessPromotionDesc make_ready_desc() {
    constexpr auto d3d12_source = "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25";
    constexpr auto vulkan_source = "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25";
    constexpr auto metal_source = "Apple-Metal-Framework-Memory-Capture-2026-06-25";
    return mirakana::RendererCommercialReadinessPromotionDesc{
        .quality_closeout = make_ready_closeout_plan(),
        .source_rows = make_source_rows(),
        .evidence_rows =
            {
                make_ready_row("backend.d3d12", RendererCommercialReadinessEvidenceKind::d3d12,
                               mirakana::rhi::BackendKind::d3d12, d3d12_source, 1U),
                make_ready_row("backend.vulkan_strict", RendererCommercialReadinessEvidenceKind::vulkan_strict,
                               mirakana::rhi::BackendKind::vulkan, vulkan_source, 2U),
                make_ready_row("backend.apple_metal", RendererCommercialReadinessEvidenceKind::apple_metal,
                               mirakana::rhi::BackendKind::metal, metal_source, 3U),
                make_ready_row("quality.matrix", RendererCommercialReadinessEvidenceKind::renderer_quality_matrix,
                               mirakana::rhi::BackendKind::null, d3d12_source, 4U),
                make_ready_row("quality.vfx", RendererCommercialReadinessEvidenceKind::production_vfx_profiling,
                               mirakana::rhi::BackendKind::null, vulkan_source, 5U),
                make_ready_row("metal.memory", RendererCommercialReadinessEvidenceKind::metal_memory_residency,
                               mirakana::rhi::BackendKind::metal, metal_source, 6U),
                make_ready_row("metal.capture", RendererCommercialReadinessEvidenceKind::metal_profiling_capture,
                               mirakana::rhi::BackendKind::metal, metal_source, 7U),
                make_ready_row("package.visible_3d", RendererCommercialReadinessEvidenceKind::visible_3d_package,
                               mirakana::rhi::BackendKind::null, d3d12_source, 8U),
                make_ready_row("package.runtime_ui", RendererCommercialReadinessEvidenceKind::runtime_ui_package,
                               mirakana::rhi::BackendKind::null, d3d12_source, 9U),
                make_ready_row("package.environment", RendererCommercialReadinessEvidenceKind::environment_package,
                               mirakana::rhi::BackendKind::null, vulkan_source, 10U),
                make_ready_row("package.generated_game",
                               RendererCommercialReadinessEvidenceKind::generated_game_package,
                               mirakana::rhi::BackendKind::null, metal_source, 11U),
                make_ready_row("claim_control", RendererCommercialReadinessEvidenceKind::claim_control,
                               mirakana::rhi::BackendKind::null, d3d12_source, 12U),
                make_ready_row("clean_room", RendererCommercialReadinessEvidenceKind::clean_room,
                               mirakana::rhi::BackendKind::null, "Unity-Legal-Terms-2026-06-25", 13U),
                make_ready_row("third_party_notice", RendererCommercialReadinessEvidenceKind::third_party_notice,
                               mirakana::rhi::BackendKind::null, "Godot-Trademark-Licensing-2026-06-25", 14U),
            },
        .row_budget = 64U,
        .seed = 5150U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::RendererCommercialReadinessPromotionPlan& plan,
                                           RendererCommercialReadinessEvidenceDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

void erase_kind(std::vector<mirakana::RendererCommercialReadinessEvidenceRow>& rows,
                RendererCommercialReadinessEvidenceKind kind) {
    std::erase_if(rows, [kind](const auto& row) { return row.kind == kind; });
}

} // namespace

MK_TEST("renderer commercial readiness promotion accepts complete retained evidence rows") {
    const auto plan = mirakana::plan_renderer_commercial_readiness_promotion(make_ready_desc());

    MK_REQUIRE(plan.status == RendererCommercialReadinessPromotionStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.renderer_backend_parity_ready);
    MK_REQUIRE(plan.renderer_metal_broad_readiness);
    MK_REQUIRE(plan.renderer_broad_quality_ready);
    MK_REQUIRE(plan.renderer_commercial_readiness);
    MK_REQUIRE(plan.native_handle_access_count == 0U);
    MK_REQUIRE(plan.cross_backend_inference_count == 0U);
    MK_REQUIRE(plan.external_engine_material_used_count == 0U);
    MK_REQUIRE(plan.ready_row_count == 14U);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("renderer commercial readiness promotion requires Apple-host Metal memory and profiling rows") {
    auto desc = make_ready_desc();
    erase_kind(desc.evidence_rows, RendererCommercialReadinessEvidenceKind::metal_profiling_capture);

    const auto plan = mirakana::plan_renderer_commercial_readiness_promotion(desc);

    MK_REQUIRE(plan.status == RendererCommercialReadinessPromotionStatus::evidence_required);
    MK_REQUIRE(!plan.renderer_metal_broad_readiness);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(diagnostic_count(
                   plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_metal_memory_profiling) == 1U);
}

MK_TEST("renderer commercial readiness promotion keeps missing strict Vulkan backend parity non-ready") {
    auto desc = make_ready_desc();
    erase_kind(desc.evidence_rows, RendererCommercialReadinessEvidenceKind::vulkan_strict);

    const auto plan = mirakana::plan_renderer_commercial_readiness_promotion(desc);

    MK_REQUIRE(plan.status == RendererCommercialReadinessPromotionStatus::evidence_required);
    MK_REQUIRE(!plan.renderer_backend_parity_ready);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(diagnostic_count(
                   plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_vulkan_strict_evidence) == 1U);
}

MK_TEST("renderer commercial readiness promotion rejects stale source ids") {
    auto desc = make_ready_desc();
    desc.source_rows[1].stale = true;

    const auto plan = mirakana::plan_renderer_commercial_readiness_promotion(desc);

    MK_REQUIRE(plan.status == RendererCommercialReadinessPromotionStatus::invalid_request);
    MK_REQUIRE(!plan.renderer_backend_parity_ready);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialReadinessEvidenceDiagnosticCode::stale_source_id) == 1U);
}

MK_TEST("renderer commercial readiness promotion rejects cross-backend Metal inference and native handles") {
    auto desc = make_ready_desc();
    for (auto& row : desc.evidence_rows) {
        if (row.kind == RendererCommercialReadinessEvidenceKind::metal_memory_residency) {
            row.backend = mirakana::rhi::BackendKind::d3d12;
            row.apple_host_metal_artifact = false;
            row.request_cross_backend_inference = true;
        }
    }
    desc.evidence_rows[0].request_native_handle_access = true;

    const auto plan = mirakana::plan_renderer_commercial_readiness_promotion(desc);

    MK_REQUIRE(plan.status == RendererCommercialReadinessPromotionStatus::invalid_request);
    MK_REQUIRE(!plan.renderer_metal_broad_readiness);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(plan.native_handle_access_count == 1U);
    MK_REQUIRE(plan.cross_backend_inference_count >= 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialReadinessEvidenceDiagnosticCode::native_handle_access) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialReadinessEvidenceDiagnosticCode::cross_backend_inference) >=
               1U);
}

MK_TEST("renderer commercial readiness promotion blocks external engine material and compatibility claims") {
    auto desc = make_ready_desc();
    desc.evidence_rows[12].external_engine_code_used = true;
    desc.evidence_rows[12].external_engine_sample_used = true;
    desc.evidence_rows[12].external_engine_ui_expression_used = true;
    desc.evidence_rows[12].external_engine_compatibility_claim = true;
    desc.evidence_rows[12].external_engine_equivalence_claim = true;

    const auto plan = mirakana::plan_renderer_commercial_readiness_promotion(desc);

    MK_REQUIRE(plan.status == RendererCommercialReadinessPromotionStatus::invalid_request);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(plan.external_engine_material_used_count == 3U);
    MK_REQUIRE(plan.external_engine_claim_count == 2U);
    MK_REQUIRE(
        diagnostic_count(plan, RendererCommercialReadinessEvidenceDiagnosticCode::external_engine_material_used) >= 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialReadinessEvidenceDiagnosticCode::external_engine_claim) >= 1U);
}

MK_TEST("renderer commercial readiness promotion keeps rejected forbidden-material diagnostics non-ready") {
    auto desc = make_ready_desc();
    auto row = make_ready_row("rejected.external_shader", RendererCommercialReadinessEvidenceKind::forbidden_material,
                              mirakana::rhi::BackendKind::null, "Unity-Legal-Terms-2026-06-25", 15U);
    row.ready = false;
    row.forbidden_material_diagnostic = true;
    row.forbidden_material_rejected = true;
    desc.evidence_rows.push_back(std::move(row));

    const auto plan = mirakana::plan_renderer_commercial_readiness_promotion(desc);

    MK_REQUIRE(plan.status == RendererCommercialReadinessPromotionStatus::evidence_required);
    MK_REQUIRE(!plan.renderer_commercial_readiness);
    MK_REQUIRE(plan.rejected_forbidden_material_count == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererCommercialReadinessEvidenceDiagnosticCode::forbidden_material_rejected) ==
               1U);
}

MK_TEST("renderer commercial readiness promotion replay hash changes with accepted artifact details") {
    const auto first = mirakana::plan_renderer_commercial_readiness_promotion(make_ready_desc());

    auto desc = make_ready_desc();
    desc.evidence_rows[7].artifact_hash_sha256 = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    const auto second = mirakana::plan_renderer_commercial_readiness_promotion(desc);

    MK_REQUIRE(first.status == RendererCommercialReadinessPromotionStatus::ready);
    MK_REQUIRE(second.status == RendererCommercialReadinessPromotionStatus::ready);
    MK_REQUIRE(first.replay_hash != 0U);
    MK_REQUIRE(second.replay_hash != 0U);
    MK_REQUIRE(first.replay_hash != second.replay_hash);
}

int main() {
    return mirakana::test::run_all();
}
