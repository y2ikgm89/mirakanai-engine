// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_scene_rhi {

enum class MavgBackendReadinessEvidenceKind : std::uint8_t {
    graph_cook_package = 0,
    runtime_lod_selection,
    resident_page,
    safe_point_adoption,
    streamed_gpu_upload,
    streamed_backend_draw,
    d3d12_compute_generated_indirect_consumption,
    vulkan_compute_generated_indirect_consumption,
    package_smoke_counter,
    metal_host_gate,
};

enum class MavgBackendReadinessEvidenceStatus : std::uint8_t {
    missing = 0,
    ready,
    host_gated,
    blocked,
    value_only,
};

enum class MavgBackendReadinessDiagnosticCode : std::uint8_t {
    none = 0,
    missing_required_row,
    duplicate_required_row,
    row_not_ready,
    row_not_package_visible,
    execution_evidence_required,
    native_handle_access,
    metal_inference_rejected,
    broad_backend_readiness_not_promoted,
    diagnostic_row_present,
};

struct MavgBackendReadinessEvidenceRow {
    MavgBackendReadinessEvidenceKind kind{MavgBackendReadinessEvidenceKind::graph_cook_package};
    std::string_view row_id;
    MavgBackendReadinessEvidenceStatus status{MavgBackendReadinessEvidenceStatus::missing};
    bool package_visible{false};
    bool execution_evidence{false};
    bool value_only{false};
    bool touched_native_handles{false};
    bool inferred_from_metal{false};
    bool inferred_from_other_backend{false};
    std::uint32_t diagnostic_count{0};
};

struct MavgBackendReadinessCloseoutDesc {
    std::span<const MavgBackendReadinessEvidenceRow> rows;
    bool request_metal_inference{false};
    bool request_broad_backend_readiness{false};
};

struct MavgBackendReadinessDiagnostic {
    MavgBackendReadinessDiagnosticCode code{MavgBackendReadinessDiagnosticCode::none};
    std::string row_id;
    std::string message;
};

struct MavgBackendReadinessCloseoutResult {
    std::vector<MavgBackendReadinessDiagnostic> diagnostics;
    std::size_t required_row_count{0};
    std::size_t ready_required_row_count{0};
    std::size_t missing_required_row_count{0};
    std::size_t metal_host_gated_row_count{0};
    bool native_handles_exposed{false};
    bool metal_inference_used{false};
    bool mavg_package_visible_backend_readiness_ready{false};
    bool mavg_broad_backend_readiness_ready{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return mavg_package_visible_backend_readiness_ready && diagnostics.empty();
    }
};

[[nodiscard]] MavgBackendReadinessCloseoutResult
evaluate_mavg_backend_readiness_closeout(const MavgBackendReadinessCloseoutDesc& desc);

[[nodiscard]] bool has_mavg_backend_readiness_closeout_diagnostic(const MavgBackendReadinessCloseoutResult& result,
                                                                  MavgBackendReadinessDiagnosticCode code) noexcept;

[[nodiscard]] bool has_mavg_backend_readiness_closeout_row_diagnostic(const MavgBackendReadinessCloseoutResult& result,
                                                                      std::string_view row_id,
                                                                      MavgBackendReadinessDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_scene_rhi
