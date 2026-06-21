// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_scene_rhi/mavg_backend_readiness_closeout.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana::runtime_scene_rhi {
namespace {

using Kind = MavgBackendReadinessEvidenceKind;
using Status = MavgBackendReadinessEvidenceStatus;
using Code = MavgBackendReadinessDiagnosticCode;

constexpr std::array<Kind, 9> kRequiredRows{
    Kind::graph_cook_package,
    Kind::runtime_lod_selection,
    Kind::resident_page,
    Kind::safe_point_adoption,
    Kind::streamed_gpu_upload,
    Kind::streamed_backend_draw,
    Kind::d3d12_compute_generated_indirect_consumption,
    Kind::vulkan_compute_generated_indirect_consumption,
    Kind::package_smoke_counter,
};

[[nodiscard]] constexpr bool requires_execution(const Kind kind) noexcept {
    return kind == Kind::safe_point_adoption || kind == Kind::streamed_gpu_upload ||
           kind == Kind::streamed_backend_draw || kind == Kind::d3d12_compute_generated_indirect_consumption ||
           kind == Kind::vulkan_compute_generated_indirect_consumption;
}

[[nodiscard]] constexpr std::string_view canonical_row_id(const Kind kind) noexcept {
    switch (kind) {
    case Kind::graph_cook_package:
        return "mavg.backend.graph_cook_package";
    case Kind::runtime_lod_selection:
        return "mavg.backend.runtime_lod_selection";
    case Kind::resident_page:
        return "mavg.backend.resident_page";
    case Kind::safe_point_adoption:
        return "mavg.backend.safe_point_adoption";
    case Kind::streamed_gpu_upload:
        return "mavg.backend.streamed_gpu_upload";
    case Kind::streamed_backend_draw:
        return "mavg.backend.streamed_backend_draw";
    case Kind::d3d12_compute_generated_indirect_consumption:
        return "mavg.backend.d3d12_compute_generated_indirect_consumption";
    case Kind::vulkan_compute_generated_indirect_consumption:
        return "mavg.backend.vulkan_compute_generated_indirect_consumption";
    case Kind::package_smoke_counter:
        return "mavg.backend.package_smoke_counter";
    case Kind::metal_host_gate:
        return "mavg.backend.metal_host_gate";
    }
    return "mavg.backend.unknown";
}

[[nodiscard]] std::string row_id_for(const MavgBackendReadinessEvidenceRow& row) {
    if (!row.row_id.empty()) {
        return std::string(row.row_id);
    }
    return std::string(canonical_row_id(row.kind));
}

void add_diagnostic(MavgBackendReadinessCloseoutResult& result, const Code code, const std::string_view row_id,
                    std::string message) {
    result.diagnostics.push_back(MavgBackendReadinessDiagnostic{
        .code = code,
        .row_id = std::string(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] std::vector<const MavgBackendReadinessEvidenceRow*>
matching_rows(const std::span<const MavgBackendReadinessEvidenceRow> rows, const Kind kind) {
    std::vector<const MavgBackendReadinessEvidenceRow*> matches;
    for (const auto& row : rows) {
        if (row.kind == kind) {
            matches.push_back(&row);
        }
    }
    return matches;
}

} // namespace

MavgBackendReadinessCloseoutResult
evaluate_mavg_backend_readiness_closeout(const MavgBackendReadinessCloseoutDesc& desc) {
    MavgBackendReadinessCloseoutResult result;
    result.required_row_count = kRequiredRows.size();

    for (const auto& row : desc.rows) {
        if (row.kind == Kind::metal_host_gate && row.status == Status::host_gated) {
            ++result.metal_host_gated_row_count;
        }
        if (row.touched_native_handles) {
            result.native_handles_exposed = true;
        }
        if (row.inferred_from_metal || row.inferred_from_other_backend) {
            add_diagnostic(result, Code::metal_inference_rejected, row_id_for(row),
                           "cross-backend or Metal-host inference cannot satisfy MAVG backend readiness evidence");
        }
    }

    if (desc.request_metal_inference) {
        add_diagnostic(result, Code::metal_inference_rejected, "mavg.backend.metal_inference",
                       "Metal host-gated rows are not D3D12 or Vulkan execution evidence");
    }
    if (desc.request_broad_backend_readiness) {
        add_diagnostic(result, Code::broad_backend_readiness_not_promoted, "mavg.backend.broad_readiness",
                       "package-visible MAVG backend readiness does not promote broad backend readiness");
    }

    for (const auto kind : kRequiredRows) {
        const auto matches = matching_rows(desc.rows, kind);
        const auto row_id = canonical_row_id(kind);
        if (matches.empty()) {
            ++result.missing_required_row_count;
            add_diagnostic(result, Code::missing_required_row, row_id,
                           "required MAVG backend readiness row is missing");
            continue;
        }

        if (matches.size() > 1U) {
            add_diagnostic(result, Code::duplicate_required_row, row_id,
                           "required MAVG backend readiness row is duplicated");
            continue;
        }

        const auto& row = *matches.front();
        const auto diagnostics_before = result.diagnostics.size();
        if (row.status != Status::ready) {
            add_diagnostic(result, Code::row_not_ready, row_id_for(row), "required row is not ready");
        }
        if (!row.package_visible) {
            add_diagnostic(result, Code::row_not_package_visible, row_id_for(row),
                           "required row is not package-visible");
        }
        if (requires_execution(kind) && (!row.execution_evidence || row.value_only)) {
            add_diagnostic(result, Code::execution_evidence_required, row_id_for(row),
                           "required row needs executed backend evidence, not value-only planner evidence");
        }
        if (row.touched_native_handles) {
            add_diagnostic(result, Code::native_handle_access, row_id_for(row), "required row exposed native handles");
        }
        if (row.diagnostic_count > 0U) {
            add_diagnostic(result, Code::diagnostic_row_present, row_id_for(row), "required row contains diagnostics");
        }

        if (result.diagnostics.size() == diagnostics_before) {
            ++result.ready_required_row_count;
        }
    }

    result.metal_inference_used = false;
    result.mavg_broad_backend_readiness_ready = false;
    result.mavg_package_visible_backend_readiness_ready =
        result.diagnostics.empty() && result.ready_required_row_count == result.required_row_count &&
        result.missing_required_row_count == 0U;
    return result;
}

bool has_mavg_backend_readiness_closeout_diagnostic(const MavgBackendReadinessCloseoutResult& result,
                                                    const MavgBackendReadinessDiagnosticCode code) noexcept {
    return std::any_of(result.diagnostics.begin(), result.diagnostics.end(),
                       [code](const auto& diagnostic) { return diagnostic.code == code; });
}

bool has_mavg_backend_readiness_closeout_row_diagnostic(const MavgBackendReadinessCloseoutResult& result,
                                                        const std::string_view row_id,
                                                        const MavgBackendReadinessDiagnosticCode code) noexcept {
    return std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [row_id, code](const auto& diagnostic) {
        return diagnostic.code == code && diagnostic.row_id == row_id;
    });
}

} // namespace mirakana::runtime_scene_rhi
