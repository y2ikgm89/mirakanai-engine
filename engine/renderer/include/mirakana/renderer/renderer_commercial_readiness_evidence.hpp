// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer_commercial_quality_closeout.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class RendererCommercialReadinessPromotionStatus : std::uint8_t {
    ready = 0,
    evidence_required,
    no_rows,
    invalid_request,
};

enum class RendererCommercialReadinessEvidenceKind : std::uint8_t {
    d3d12 = 0,
    vulkan_strict,
    apple_metal,
    renderer_quality_matrix,
    production_vfx_profiling,
    metal_memory_residency,
    metal_profiling_capture,
    visible_3d_package,
    runtime_ui_package,
    environment_package,
    generated_game_package,
    claim_control,
    clean_room,
    third_party_notice,
    forbidden_material,
};

enum class RendererCommercialReadinessEvidenceDiagnosticCode : std::uint8_t {
    none = 0,
    missing_source_id,
    stale_source_id,
    invalid_evidence_row,
    missing_quality_closeout,
    missing_d3d12_evidence,
    missing_vulkan_strict_evidence,
    missing_apple_metal_evidence,
    missing_renderer_quality_matrix,
    missing_production_vfx_profiling,
    missing_metal_memory_profiling,
    missing_required_package_row,
    missing_claim_control,
    missing_clean_room,
    missing_third_party_notice,
    native_handle_access,
    cross_backend_inference,
    external_engine_material_used,
    external_engine_claim,
    forbidden_material_rejected,
    row_budget_exceeded,
};

struct RendererCommercialReadinessSourceRow {
    std::string source_id;
    bool reviewed{false};
    bool official_or_context7{false};
    bool stale{false};
};

struct RendererCommercialReadinessEvidenceRow {
    std::string row_id;
    RendererCommercialReadinessEvidenceKind kind{RendererCommercialReadinessEvidenceKind::d3d12};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool selected{false};
    bool ready{false};
    std::string source_id;
    std::string host_validation_recipe_id;
    std::string validation_counter_id;
    std::string package_counter_id;
    std::string artifact_hash_sha256;
    std::string artifact_schema_version;
    bool apple_host_metal_artifact{false};
    bool request_native_handle_access{false};
    bool request_cross_backend_inference{false};
    bool external_engine_code_used{false};
    bool external_engine_sample_used{false};
    bool external_engine_asset_used{false};
    bool external_engine_trademark_used{false};
    bool external_engine_ui_expression_used{false};
    bool external_engine_api_used{false};
    bool external_engine_compatibility_claim{false};
    bool external_engine_equivalence_claim{false};
    bool external_engine_parity_claim{false};
    bool forbidden_material_diagnostic{false};
    bool forbidden_material_rejected{false};
    bool third_party_notices_complete{true};
    std::uint32_t source_index{0U};
};

struct RendererCommercialReadinessPromotionDesc {
    RendererCommercialQualityCloseoutPlan quality_closeout;
    std::vector<RendererCommercialReadinessSourceRow> source_rows;
    std::vector<RendererCommercialReadinessEvidenceRow> evidence_rows;
    std::size_t row_budget{256U};
    std::uint64_t seed{0U};
};

struct RendererCommercialReadinessEvidenceDiagnostic {
    RendererCommercialReadinessEvidenceDiagnosticCode code{RendererCommercialReadinessEvidenceDiagnosticCode::none};
    RendererCommercialReadinessEvidenceKind kind{RendererCommercialReadinessEvidenceKind::d3d12};
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RendererCommercialReadinessPromotionPlan {
    RendererCommercialReadinessPromotionStatus status{RendererCommercialReadinessPromotionStatus::invalid_request};
    std::vector<RendererCommercialReadinessEvidenceDiagnostic> diagnostics;
    std::vector<RendererCommercialReadinessEvidenceRow> evidence_rows;
    std::vector<RendererCommercialReadinessSourceRow> source_rows;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t rejected_forbidden_material_count{0U};
    std::size_t native_handle_access_count{0U};
    std::size_t cross_backend_inference_count{0U};
    std::size_t external_engine_material_used_count{0U};
    std::size_t external_engine_claim_count{0U};
    std::uint64_t replay_hash{0U};
    bool renderer_backend_parity_ready{false};
    bool renderer_metal_broad_readiness{false};
    bool renderer_broad_quality_ready{false};
    bool renderer_commercial_readiness{false};
    bool d3d12_evidence_ready{false};
    bool vulkan_strict_evidence_ready{false};
    bool apple_metal_evidence_ready{false};
    bool renderer_quality_matrix_ready{false};
    bool production_vfx_profiling_ready{false};
    bool metal_memory_residency_ready{false};
    bool metal_profiling_capture_ready{false};
    bool visible_3d_package_ready{false};
    bool runtime_ui_package_ready{false};
    bool environment_package_ready{false};
    bool generated_game_package_ready{false};
    bool claim_control_ready{false};
    bool clean_room_ready{false};
    bool third_party_notice_ready{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Promotes retained renderer commercial evidence rows without reading files, executing recipes,
/// invoking GPU work, opening native handles, running package scripts, or making network requests.
[[nodiscard]] RendererCommercialReadinessPromotionPlan
plan_renderer_commercial_readiness_promotion(const RendererCommercialReadinessPromotionDesc& desc);

} // namespace mirakana
