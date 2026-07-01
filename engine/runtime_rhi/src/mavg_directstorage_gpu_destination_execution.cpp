// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_directstorage_gpu_destination_execution.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mirakana::runtime_rhi {

std::string_view runtime_mavg_directstorage_gpu_destination_execution_status_label(
    RuntimeMavgDirectStorageGpuDestinationExecutionStatus status) noexcept {
    switch (status) {
    case RuntimeMavgDirectStorageGpuDestinationExecutionStatus::host_evidence_required:
        return "host_evidence_required";
    case RuntimeMavgDirectStorageGpuDestinationExecutionStatus::blocked:
        return "blocked";
    case RuntimeMavgDirectStorageGpuDestinationExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

namespace {

void add_diagnostic(RuntimeMavgDirectStorageGpuDestinationExecutionResult& result,
                    RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode code, std::string message) {
    result.diagnostics.push_back(
        RuntimeMavgDirectStorageGpuDestinationExecutionDiagnostic{.code = code, .message = std::move(message)});
}

[[nodiscard]] bool blank(std::string_view value) noexcept {
    return value.empty();
}

[[nodiscard]] bool is_lower_sha256(std::string_view value) noexcept {
    if (value.size() != 64U) {
        return false;
    }
    return std::ranges::all_of(value, [](char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'); });
}

[[nodiscard]] bool is_range_destination(RuntimeMavgDirectStorageGpuDestinationKind destination) noexcept {
    return destination == RuntimeMavgDirectStorageGpuDestinationKind::d3d12_multiple_subresources_range;
}

[[nodiscard]] bool
has_forbidden_execution_claim(const RuntimeMavgDirectStorageGpuDestinationExecutionRow& row) noexcept {
    return row.native_handles_exposed || row.directstorage_14_preview_selected || row.claims_gdeflate_execution_ready ||
           row.claims_zstd_preview_ready || row.claims_performance_gain ||
           row.claims_package_visible_backend_readiness || row.claims_broad_mavg_backend_ready ||
           row.claims_broad_optimization_ready || row.claims_nanite_compatibility || row.claims_nanite_equivalence ||
           row.claims_nanite_superiority || row.claims_unity_unreal_godot_compatibility;
}

void validate_forbidden_claims(RuntimeMavgDirectStorageGpuDestinationExecutionResult& result,
                               const RuntimeMavgDirectStorageGpuDestinationExecutionRow& row) {
    if (row.directstorage_14_preview_selected) {
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::directstorage_preview_selected,
                       "MAVG DirectStorage GPU destination execution evidence must use stable DirectStorage 1.3; "
                       "DirectStorage 1.4/Zstd/GACL preview rows are future review input only");
    }
    if (row.native_handles_exposed) {
        result.mavg_directstorage_native_handles_exposed = true;
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::native_handle_exposure,
                       "MAVG DirectStorage GPU destination execution evidence must not expose native handles or SDK "
                       "objects");
    }
    if (row.claims_gdeflate_execution_ready) {
        add_diagnostic(
            result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::gdeflate_execution_claim_not_allowed,
            "MAVG DirectStorage GPU destination execution evidence must not claim GDeflate execution readiness");
    }
    if (row.claims_zstd_preview_ready) {
        add_diagnostic(
            result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::zstd_preview_claim_not_allowed,
            "MAVG DirectStorage GPU destination execution evidence must not claim DirectStorage 1.4/Zstd preview "
            "readiness");
    }
    if (row.claims_performance_gain) {
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::performance_claim_not_allowed,
                       "MAVG DirectStorage GPU destination execution evidence must not claim performance readiness");
    }
    if (row.claims_package_visible_backend_readiness) {
        add_diagnostic(
            result,
            RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::package_backend_readiness_claim_not_allowed,
            "MAVG DirectStorage GPU destination execution evidence must not promote package-visible backend "
            "readiness");
    }
    if (row.claims_broad_mavg_backend_ready) {
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::broad_backend_claim_not_allowed,
                       "MAVG DirectStorage GPU destination execution evidence must not claim broad MAVG backend "
                       "readiness");
    }
    if (row.claims_broad_optimization_ready) {
        add_diagnostic(
            result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::broad_optimization_claim_not_allowed,
            "MAVG DirectStorage GPU destination execution evidence must not claim broad CPU/GPU/memory "
            "optimization");
    }
    if (row.claims_nanite_compatibility || row.claims_nanite_equivalence || row.claims_nanite_superiority) {
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::nanite_claim_not_allowed,
                       "MAVG DirectStorage GPU destination execution evidence must not claim Nanite compatibility, "
                       "equivalence, or superiority");
    }
    if (row.claims_unity_unreal_godot_compatibility) {
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::external_engine_claim_not_allowed,
                       "MAVG DirectStorage GPU destination execution evidence must not claim Unity, Unreal, or Godot "
                       "compatibility");
    }
}

[[nodiscard]] bool validate_required_evidence(RuntimeMavgDirectStorageGpuDestinationExecutionResult& result,
                                              const RuntimeMavgDirectStorageGpuDestinationExecutionRow& row) {
    bool valid = true;

    if (blank(row.official_source_id)) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_official_source,
                       "MAVG DirectStorage GPU destination execution rows require Microsoft official source ids");
    }
    if (blank(row.sdk_package_version)) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_sdk_version,
                       "MAVG DirectStorage GPU destination execution rows require a reviewed DirectStorage SDK "
                       "version");
    }
    if (!row.reviewed || !row.ready || !row.stable_directstorage_13_selected) {
        valid = false;
        add_diagnostic(
            result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_stable_directstorage_13,
            "MAVG DirectStorage GPU destination execution requires reviewed stable DirectStorage 1.3 evidence");
    }
    if (!row.queue_device_bound) {
        valid = false;
        add_diagnostic(
            result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_queue_device_binding,
            "MAVG DirectStorage GPU destination execution requires a queue bound to the D3D12 destination device");
    }
    if (!row.destination_resource_device_matches_queue_device) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::destination_device_mismatch,
                       "MAVG DirectStorage GPU destination resources must match the DirectStorage queue device");
    }
    if (!row.enqueue_requests_used) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_enqueue_requests,
                       "MAVG DirectStorage GPU destination execution requires the EnqueueRequests API");
    }
    if (!row.d3d12_fence_synchronization_used) {
        valid = false;
        add_diagnostic(
            result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_d3d12_fence_synchronization,
            "MAVG DirectStorage GPU destination execution requires D3D12 fence synchronization");
    }
    if (!row.destination_resource_state_common) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_destination_state_common,
                       "MAVG DirectStorage GPU destination resources must start in D3D12_RESOURCE_STATE_COMMON");
    }
    if (!row.request_completed || row.requested_bytes == 0U || row.completed_bytes == 0U ||
        row.completed_bytes != row.requested_bytes) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_completed_request,
                       "MAVG DirectStorage GPU destination execution requires a completed request with matching byte "
                       "counts");
    }
    if (!row.status_array_success) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_status_success,
                       "MAVG DirectStorage GPU destination execution requires a successful status array row");
    }
    if (!row.readback_hash_ready) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_readback_hash,
                       "MAVG DirectStorage GPU destination execution requires deterministic readback hash evidence");
    }
    if (!row.package_visible_output_ready) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_package_visible_output,
                       "MAVG DirectStorage GPU destination execution requires package-visible output evidence");
    }
    if (blank(row.retained_artifact_id) || !is_lower_sha256(row.retained_artifact_sha256)) {
        valid = false;
        add_diagnostic(
            result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::invalid_retained_artifact_hash,
            "MAVG DirectStorage GPU destination execution evidence requires retained artifact id and lowercase "
            "SHA-256 hash");
    }
    if (is_range_destination(row.destination) && row.num_subresources == 0U) {
        valid = false;
        add_diagnostic(
            result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::invalid_subresource_range,
            "MAVG DirectStorage multiple-subresources-range destination evidence requires at least one subresource");
    }

    return valid;
}

} // namespace

RuntimeMavgDirectStorageGpuDestinationExecutionResult
evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence(
    const RuntimeMavgDirectStorageGpuDestinationExecutionDesc& desc) {
    RuntimeMavgDirectStorageGpuDestinationExecutionResult result;

    bool has_ready_row = false;
    bool has_ready_range_row = false;
    bool blocked = false;

    for (const auto& row : desc.rows) {
        if (row.reviewed) {
            ++result.reviewed_rows;
        }

        const std::size_t diagnostics_before = result.diagnostics.size();
        validate_forbidden_claims(result, row);
        const bool row_forbidden = has_forbidden_execution_claim(row);
        const bool row_valid = validate_required_evidence(result, row);
        const bool row_has_diagnostic = result.diagnostics.size() != diagnostics_before;

        if (row_forbidden || (!row_valid && row_has_diagnostic &&
                              (blank(row.retained_artifact_id) || !is_lower_sha256(row.retained_artifact_sha256) ||
                               (is_range_destination(row.destination) && row.num_subresources == 0U) ||
                               (row.requested_bytes != 0U && row.completed_bytes != row.requested_bytes)))) {
            ++result.blocked_rows;
            blocked = true;
            continue;
        }

        if (!row_valid) {
            ++result.host_evidence_required_rows;
            continue;
        }

        ++result.ready_rows;
        ++result.gpu_destination_execution_rows;
        has_ready_row = true;
        if (is_range_destination(row.destination)) {
            ++result.multiple_subresources_range_rows;
            has_ready_range_row = true;
        }
    }

    if (blocked) {
        result.status = RuntimeMavgDirectStorageGpuDestinationExecutionStatus::blocked;
        return result;
    }
    if (!has_ready_row || !result.diagnostics.empty()) {
        result.status = RuntimeMavgDirectStorageGpuDestinationExecutionStatus::host_evidence_required;
        return result;
    }

    result.status = RuntimeMavgDirectStorageGpuDestinationExecutionStatus::ready;
    result.mavg_directstorage_gpu_destination_execution_ready = true;
    result.mavg_directstorage_multiple_subresources_range_execution_ready = has_ready_range_row;
    return result;
}

bool has_runtime_mavg_directstorage_gpu_destination_execution_diagnostic(
    const RuntimeMavgDirectStorageGpuDestinationExecutionResult& result,
    RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics,
                               [code](const RuntimeMavgDirectStorageGpuDestinationExecutionDiagnostic& diagnostic) {
                                   return diagnostic.code == code;
                               });
}

} // namespace mirakana::runtime_rhi
