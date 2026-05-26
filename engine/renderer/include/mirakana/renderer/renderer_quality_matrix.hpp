// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class RendererQualityMatrixStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    no_rows,
    invalid_request,
};

enum class RendererQualityFeatureKind : std::uint8_t {
    materials = 0,
    lighting_shadows,
    postprocess,
    sprite_ui,
    scene_scale,
    gpu_memory_residency,
    profiling_capture,
};

enum class RendererQualityProofKind : std::uint8_t {
    selected_package = 0,
    host_backend,
    tool_validation,
    reviewed_handoff,
    host_gate,
};

enum class RendererQualityMatrixDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_required_backend,
    duplicate_required_backend,
    unsupported_backend,
    missing_required_quality_row,
    duplicate_quality_row,
    invalid_quality_row,
    missing_backend_local_evidence,
    missing_resource_synchronization_evidence,
    missing_backend_validation_evidence,
    missing_shader_tool_validation_evidence,
    missing_package_counter_evidence,
    missing_timing_budget_evidence,
    missing_gpu_memory_evidence,
    missing_backend_parity_evidence,
    missing_metal_host_evidence,
    unsupported_native_handle_claim,
    unsupported_capture_execution,
    unsupported_crash_upload_execution,
    unsupported_inferred_backend_parity,
    unsupported_subjective_visual_quality_claim,
    row_budget_exceeded,
};

struct RendererQualityMatrixRow {
    std::string feature_id;
    RendererQualityFeatureKind feature{RendererQualityFeatureKind::materials};
    rhi::BackendKind backend{rhi::BackendKind::null};
    RendererQualityProofKind proof{RendererQualityProofKind::selected_package};
    bool reviewed{false};
    bool backend_local_evidence{false};
    bool d3d12_resource_state_barrier_evidence{false};
    bool d3d12_fence_evidence{false};
    bool vulkan_synchronization2_evidence{false};
    bool vulkan_layout_transition_evidence{false};
    bool vulkan_validation_layer_evidence{false};
    bool vulkan_spirv_validation_evidence{false};
    bool metal_resource_synchronization_evidence{false};
    bool metal_feature_set_evidence{false};
    bool shader_tool_validation_evidence{false};
    std::vector<std::string> package_counter_ids;
    std::uint32_t timing_budget_us{0U};
    bool gpu_memory_evidence{false};
    bool backend_parity_evidence{false};
    bool host_validated{false};
    bool host_gate_required{false};
    bool request_native_handle_access{false};
    bool request_capture_execution{false};
    bool request_crash_upload_execution{false};
    bool request_inferred_backend_parity{false};
    bool request_subjective_visual_quality_claim{false};
    std::uint32_t source_index{0U};
};

struct RendererQualityMatrixRequest {
    std::vector<rhi::BackendKind> required_backends;
    std::vector<RendererQualityMatrixRow> rows;
    std::size_t row_budget{512U};
    std::uint64_t seed{0U};
};

struct RendererQualityMatrixDiagnostic {
    RendererQualityMatrixDiagnosticCode code{RendererQualityMatrixDiagnosticCode::none};
    rhi::BackendKind backend{rhi::BackendKind::null};
    RendererQualityFeatureKind feature{RendererQualityFeatureKind::materials};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RendererQualityMatrixPlan {
    RendererQualityMatrixStatus status{RendererQualityMatrixStatus::invalid_request};
    std::vector<RendererQualityMatrixDiagnostic> diagnostics;
    std::vector<rhi::BackendKind> required_backends;
    std::vector<RendererQualityMatrixRow> rows;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t host_gated_row_count{0U};
    std::size_t host_validated_backend_count{0U};
    std::uint64_t replay_hash{0U};
    bool d3d12_quality_matrix_ready{false};
    bool vulkan_strict_quality_matrix_ready{false};
    bool metal_quality_matrix_ready{false};
    bool requires_metal_host_evidence{false};
    bool has_metal_host_evidence{false};
    bool selected_package_quality_evidence_ready{false};
    bool general_renderer_quality_ready{false};
    bool invoked_gpu_commands{false};
    bool invoked_native_capture{false};
    bool invoked_crash_upload{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews renderer production-quality evidence rows without executing GPU commands, native captures,
/// crash uploads, external tools, or backend handle access. Evidence is backend-local and never inferred.
[[nodiscard]] RendererQualityMatrixPlan plan_renderer_quality_matrix(const RendererQualityMatrixRequest& request);

} // namespace mirakana
