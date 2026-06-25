// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/backend_renderer_parity_policy.hpp"
#include "mirakana/renderer/production_vfx_profiling.hpp"
#include "mirakana/renderer/renderer_quality_matrix.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class RendererCommercialQualityCloseoutStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    dependency_evidence_required,
    unsupported,
    no_rows,
    invalid_request,
};

enum class RendererCommercialQualityEvidenceKind : std::uint8_t {
    backend_parity = 0,
    renderer_quality_matrix,
    production_vfx_profiling,
    metal_memory_profiling,
    visible_3d_package,
    runtime_ui_package,
    environment_package,
    generated_game_package,
    claim_control,
    clean_room,
};

enum class RendererCommercialQualityEvidenceStatus : std::uint8_t {
    ready = 0,
    host_gated,
    dependency_gated,
    unsupported,
};

enum class RendererCommercialQualityCloseoutDiagnosticCode : std::uint8_t {
    none = 0,
    missing_backend_parity,
    missing_quality_matrix,
    missing_production_vfx_profiling,
    missing_metal_memory_profiling_evidence,
    missing_required_package_row,
    missing_claim_control_evidence,
    missing_clean_room_evidence,
    duplicate_evidence_row,
    invalid_evidence_row,
    unreviewed_host_validation_recipe,
    unsupported_native_handle_claim,
    cross_backend_inference,
    external_engine_parity_claim,
    unsupported_subjective_quality_claim,
    unsupported_crash_upload,
    external_engine_code_used,
    external_engine_sample_used,
    external_engine_asset_used,
    external_engine_trademark_used,
    external_engine_compatibility_claim,
    missing_third_party_notices,
    row_budget_exceeded,
};

struct RendererCommercialQualityEvidenceRow {
    std::string row_id;
    RendererCommercialQualityEvidenceKind kind{RendererCommercialQualityEvidenceKind::backend_parity};
    rhi::BackendKind backend{rhi::BackendKind::null};
    RendererCommercialQualityEvidenceStatus status{RendererCommercialQualityEvidenceStatus::ready};
    bool reviewed{false};
    bool ready{false};
    bool host_gated{false};
    bool dependency_gated{false};
    std::string host_validation_recipe_id;
    std::string package_counter_id;
    bool request_native_handle_access{false};
    bool request_cross_backend_inference{false};
    bool request_external_engine_parity{false};
    bool request_subjective_quality_claim{false};
    bool request_crash_upload{false};
    bool external_engine_code_used{false};
    bool external_engine_sample_used{false};
    bool external_engine_asset_used{false};
    bool external_engine_trademark_used{false};
    bool external_engine_compatibility_claim{false};
    bool approved_external_material{false};
    bool third_party_notices_complete{true};
    std::uint32_t source_index{0U};
};

struct RendererCommercialQualityCloseoutDesc {
    BackendRendererParityPolicyPlan backend_parity;
    RendererQualityMatrixPlan quality_matrix;
    RendererProductionVfxProfilingPlan vfx_profiling;
    std::vector<RendererCommercialQualityEvidenceRow> evidence_rows;
    std::size_t row_budget{256U};
    std::uint64_t seed{0U};
};

struct RendererCommercialQualityCloseoutDiagnostic {
    RendererCommercialQualityCloseoutDiagnosticCode code{RendererCommercialQualityCloseoutDiagnosticCode::none};
    RendererCommercialQualityEvidenceKind kind{RendererCommercialQualityEvidenceKind::backend_parity};
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RendererCommercialQualityCloseoutPlan {
    RendererCommercialQualityCloseoutStatus status{RendererCommercialQualityCloseoutStatus::invalid_request};
    std::vector<RendererCommercialQualityCloseoutDiagnostic> diagnostics;
    std::vector<RendererCommercialQualityEvidenceRow> evidence_rows;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t host_gated_row_count{0U};
    std::size_t dependency_gated_row_count{0U};
    std::size_t unsupported_row_count{0U};
    std::size_t rejected_unsafe_row_count{0U};
    std::size_t native_handle_access_count{0U};
    std::size_t cross_backend_inference_count{0U};
    std::size_t external_engine_parity_count{0U};
    std::size_t external_engine_code_used_count{0U};
    std::size_t external_engine_sample_used_count{0U};
    std::size_t external_engine_asset_used_count{0U};
    std::size_t external_engine_trademark_used_count{0U};
    std::size_t external_engine_compatibility_claim_count{0U};
    std::uint64_t replay_hash{0U};
    bool renderer_backend_parity_ready{false};
    bool renderer_metal_broad_readiness{false};
    bool renderer_broad_quality_ready{false};
    bool renderer_commercial_readiness{false};
    bool d3d12_renderer_quality_ready{false};
    bool vulkan_strict_renderer_quality_ready{false};
    bool metal_renderer_quality_ready{false};
    bool production_vfx_profiling_ready{false};
    bool metal_memory_profiling_ready{false};
    bool visible_3d_package_ready{false};
    bool runtime_ui_package_ready{false};
    bool environment_package_ready{false};
    bool generated_game_package_ready{false};
    bool claim_control_ready{false};
    bool clean_room_source_review_ready{false};
    bool third_party_notices_complete{true};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Aggregates renderer readiness evidence without executing GPU commands, native captures, package scripts,
/// external tools, crash uploads, or backend-native handle access. Backend readiness remains backend-local.
[[nodiscard]] RendererCommercialQualityCloseoutPlan
plan_renderer_commercial_quality_closeout(const RendererCommercialQualityCloseoutDesc& desc);

} // namespace mirakana
