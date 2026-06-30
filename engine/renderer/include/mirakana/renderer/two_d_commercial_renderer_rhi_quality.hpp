// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/sprite_batch.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class TwoDCommercialRendererRhiQualityStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    dependency_evidence_required,
    unsupported,
    no_rows,
    invalid_request,
};

enum class TwoDCommercialRendererRhiQualityEvidenceKind : std::uint8_t {
    backend_host_evidence = 0,
    sprite_batch_package,
    atlas_residency_upload,
    frame_pacing_budget,
    claim_control,
    clean_room,
};

enum class TwoDCommercialRendererRhiQualityEvidenceStatus : std::uint8_t {
    ready = 0,
    host_gated,
    dependency_gated,
    unsupported,
};

enum class TwoDCommercialRendererRhiQualityOfficialSourceKind : std::uint8_t {
    microsoft_direct3d12 = 0,
    khronos_vulkan,
    apple_metal,
    repository_legal_policy,
};

enum class TwoDCommercialRendererRhiQualityDiagnosticCode : std::uint8_t {
    none = 0,
    missing_official_source,
    invalid_official_source,
    stale_official_source,
    invalid_evidence_row,
    selected_d3d12_claim_missing,
    missing_d3d12_evidence,
    missing_sprite_batch_budget,
    missing_atlas_upload_evidence,
    missing_frame_pacing_budget,
    missing_claim_control_evidence,
    missing_clean_room_evidence,
    public_native_handle_access,
    cross_backend_inference,
    external_engine_claim,
    legal_approval_claim,
    broad_backend_claim,
};

struct TwoDCommercialRendererRhiQualityOfficialSourceRow {
    std::string id;
    TwoDCommercialRendererRhiQualityOfficialSourceKind kind{
        TwoDCommercialRendererRhiQualityOfficialSourceKind::microsoft_direct3d12};
    bool ready{false};
    bool official{false};
    bool public_docs_only{false};
};

struct TwoDCommercialRendererRhiQualityEvidenceRow {
    std::string row_id;
    TwoDCommercialRendererRhiQualityEvidenceKind kind{
        TwoDCommercialRendererRhiQualityEvidenceKind::backend_host_evidence};
    rhi::BackendKind backend{rhi::BackendKind::null};
    TwoDCommercialRendererRhiQualityEvidenceStatus status{TwoDCommercialRendererRhiQualityEvidenceStatus::ready};
    bool selected{false};
    bool reviewed{false};
    bool ready{false};
    bool host_gated{false};
    bool dependency_gated{false};
    std::string validation_recipe_id;
    std::string package_counter_id;

    bool command_allocator_fence_ready{false};
    bool command_list_close_execute_reset_ready{false};
    bool descriptor_heap_binding_ready{false};
    bool pipeline_state_reuse_ready{false};
    bool resource_barrier_ready{false};
    bool debug_validation_clean{false};
    bool timestamp_or_pix_evidence_ready{false};
    bool package_visible_readback_ready{false};
    bool sprite_batch_budget_ready{false};
    bool atlas_residency_budget_ready{false};
    bool texture_upload_scheduling_ready{false};
    bool frame_pacing_budget_ready{false};

    bool public_native_handles_exposed{false};
    bool request_cross_backend_inference{false};
    bool external_engine_code_used{false};
    bool external_engine_sample_used{false};
    bool external_engine_asset_used{false};
    bool external_engine_trademark_used{false};
    bool external_engine_compatibility_claim{false};
    bool legal_approval_claim{false};
    std::uint32_t source_index{0U};
};

struct TwoDCommercialRendererRhiQualityDesc {
    SpriteBatchProductionThroughputPlan sprite_throughput;
    std::array<TwoDCommercialRendererRhiQualityOfficialSourceRow, 4U> official_source_rows{};
    std::vector<TwoDCommercialRendererRhiQualityEvidenceRow> evidence_rows;
    bool selected_d3d12_ready_claim{false};
    bool vulkan_ready_claim{false};
    bool metal_ready_claim{false};
    bool broad_backend_parity_claim{false};
    bool broad_renderer_rhi_quality_claim{false};
    std::uint64_t seed{0U};
};

struct TwoDCommercialRendererRhiQualityDiagnostic {
    TwoDCommercialRendererRhiQualityDiagnosticCode code{TwoDCommercialRendererRhiQualityDiagnosticCode::none};
    TwoDCommercialRendererRhiQualityEvidenceKind kind{
        TwoDCommercialRendererRhiQualityEvidenceKind::backend_host_evidence};
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::string row_id;
    std::string message;
};

struct TwoDCommercialRendererRhiQualityResult {
    TwoDCommercialRendererRhiQualityStatus status{TwoDCommercialRendererRhiQualityStatus::invalid_request};
    std::vector<TwoDCommercialRendererRhiQualityDiagnostic> diagnostics;
    std::size_t official_source_rows{0U};
    std::size_t evidence_row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t host_gated_row_count{0U};
    std::size_t dependency_gated_row_count{0U};
    std::size_t unsupported_row_count{0U};
    std::size_t public_native_handle_rows{0U};
    std::size_t cross_backend_inference_rows{0U};
    std::size_t external_engine_claim_rows{0U};
    std::size_t legal_approval_claim_rows{0U};
    std::uint64_t replay_hash{0U};

    bool selected_d3d12_renderer_rhi_quality_ready{false};
    bool vulkan_strict_renderer_rhi_quality_ready{false};
    bool metal_renderer_rhi_quality_ready{false};
    bool broad_backend_parity_ready{false};
    bool broad_renderer_rhi_quality_ready{false};
    bool official_source_ledger_ready{false};
    bool sprite_batch_budget_ready{false};
    bool atlas_residency_upload_ready{false};
    bool frame_pacing_budget_ready{false};
    bool claim_control_ready{false};
    bool clean_room_source_review_ready{false};
    bool d3d12_command_allocator_fence_ready{false};
    bool d3d12_command_list_close_execute_reset_ready{false};
    bool d3d12_descriptor_heap_binding_ready{false};
    bool d3d12_pipeline_state_reuse_ready{false};
    bool d3d12_resource_barrier_ready{false};
    bool d3d12_debug_validation_clean{false};
    bool d3d12_timestamp_or_pix_evidence_ready{false};
    bool package_visible_readback_ready{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] std::string_view two_d_commercial_renderer_rhi_quality_official_source_kind_name(
    TwoDCommercialRendererRhiQualityOfficialSourceKind kind) noexcept;
[[nodiscard]] TwoDCommercialRendererRhiQualityResult
evaluate_2d_commercial_renderer_rhi_quality(const TwoDCommercialRendererRhiQualityDesc& desc);

} // namespace mirakana
