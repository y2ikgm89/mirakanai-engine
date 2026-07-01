// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_directstorage_gdeflate_execution.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mirakana::runtime_rhi {

std::string_view runtime_mavg_directstorage_gdeflate_execution_status_label(
    RuntimeMavgDirectStorageGDeflateExecutionStatus status) noexcept {
    switch (status) {
    case RuntimeMavgDirectStorageGDeflateExecutionStatus::host_evidence_required:
        return "host_evidence_required";
    case RuntimeMavgDirectStorageGDeflateExecutionStatus::blocked:
        return "blocked";
    case RuntimeMavgDirectStorageGDeflateExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

namespace {

void add_diagnostic(RuntimeMavgDirectStorageGDeflateExecutionResult& result,
                    RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode code, std::string message) {
    result.diagnostics.push_back(
        RuntimeMavgDirectStorageGDeflateExecutionDiagnostic{.code = code, .message = std::move(message)});
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

[[nodiscard]] bool is_range_destination(RuntimeMavgDirectStorageGDeflateDestinationKind destination) noexcept {
    return destination == RuntimeMavgDirectStorageGDeflateDestinationKind::d3d12_multiple_subresources_range;
}

[[nodiscard]] bool has_gpu_compression_support(const RuntimeMavgDirectStorageGDeflateExecutionRow& row) noexcept {
    return row.compression_support_gpu_optimized || row.compression_support_gpu_fallback;
}

[[nodiscard]] bool has_gpu_compression_queue(const RuntimeMavgDirectStorageGDeflateExecutionRow& row) noexcept {
    return row.compression_support_uses_compute_queue || row.compression_support_uses_copy_queue;
}

[[nodiscard]] bool has_forbidden_execution_claim(const RuntimeMavgDirectStorageGDeflateExecutionRow& row) noexcept {
    return row.native_handles_exposed || row.directstorage_14_preview_selected || row.claims_zstd_preview_ready ||
           row.claims_performance_gain || row.claims_package_visible_backend_readiness ||
           row.claims_broad_mavg_backend_ready || row.claims_broad_optimization_ready ||
           row.claims_nanite_compatibility || row.claims_nanite_equivalence || row.claims_nanite_superiority ||
           row.claims_unity_unreal_godot_compatibility;
}

void validate_forbidden_claims(RuntimeMavgDirectStorageGDeflateExecutionResult& result,
                               const RuntimeMavgDirectStorageGDeflateExecutionRow& row) {
    if (row.directstorage_14_preview_selected) {
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::directstorage_preview_selected,
                       "MAVG DirectStorage GDeflate execution evidence must use stable DirectStorage GDeflate and "
                       "1.3 queue/destination rows; DirectStorage 1.4/Zstd/GACL preview rows are future review input");
    }
    if (row.native_handles_exposed) {
        result.mavg_directstorage_native_handles_exposed = true;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::native_handle_exposure,
                       "MAVG DirectStorage GDeflate execution evidence must not expose native handles or SDK objects");
    }
    if (row.claims_zstd_preview_ready) {
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::zstd_preview_claim_not_allowed,
                       "MAVG DirectStorage GDeflate execution evidence must not claim DirectStorage 1.4/Zstd preview "
                       "readiness");
    }
    if (row.claims_performance_gain) {
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::performance_claim_not_allowed,
                       "MAVG DirectStorage GDeflate execution evidence must not claim performance readiness");
    }
    if (row.claims_package_visible_backend_readiness) {
        add_diagnostic(
            result,
            RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::package_backend_readiness_claim_not_allowed,
            "MAVG DirectStorage GDeflate execution evidence must not promote package-visible backend readiness");
    }
    if (row.claims_broad_mavg_backend_ready) {
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::broad_backend_claim_not_allowed,
                       "MAVG DirectStorage GDeflate execution evidence must not claim broad MAVG backend readiness");
    }
    if (row.claims_broad_optimization_ready) {
        add_diagnostic(
            result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::broad_optimization_claim_not_allowed,
            "MAVG DirectStorage GDeflate execution evidence must not claim broad CPU/GPU/memory optimization");
    }
    if (row.claims_nanite_compatibility || row.claims_nanite_equivalence || row.claims_nanite_superiority) {
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::nanite_claim_not_allowed,
                       "MAVG DirectStorage GDeflate execution evidence must not claim Nanite compatibility, "
                       "equivalence, or superiority");
    }
    if (row.claims_unity_unreal_godot_compatibility) {
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::external_engine_claim_not_allowed,
                       "MAVG DirectStorage GDeflate execution evidence must not claim Unity, Unreal, or Godot "
                       "compatibility");
    }
}

[[nodiscard]] bool validate_required_evidence(RuntimeMavgDirectStorageGDeflateExecutionResult& result,
                                              const RuntimeMavgDirectStorageGDeflateExecutionRow& row) {
    bool valid = true;

    if (blank(row.official_source_id)) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_official_source,
                       "MAVG DirectStorage GDeflate execution rows require Microsoft official source ids");
    }
    if (blank(row.sdk_package_version)) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_sdk_version,
                       "MAVG DirectStorage GDeflate execution rows require a reviewed DirectStorage SDK version");
    }
    if (!row.reviewed || !row.ready || !row.stable_directstorage_11_gdeflate_selected) {
        valid = false;
        add_diagnostic(
            result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_stable_directstorage_11_gdeflate,
            "MAVG DirectStorage GDeflate execution requires reviewed stable DirectStorage 1.1 GDeflate evidence");
    }
    if (!row.stable_directstorage_13_selected) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_stable_directstorage_13,
                       "MAVG DirectStorage GDeflate execution requires reviewed stable DirectStorage 1.3 queue and "
                       "destination evidence");
    }
    if (!row.request_options_reviewed) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_request_options_review,
                       "MAVG DirectStorage GDeflate execution requires reviewed DirectStorage request options");
    }
    if (!row.compression_format_gdeflate) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_gdeflate_compression_format,
                       "MAVG DirectStorage GDeflate execution requires DSTORAGE_COMPRESSION_FORMAT_GDEFLATE evidence");
    }
    if (!row.source_type_reviewed) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_source_type_review,
                       "MAVG DirectStorage GDeflate execution requires reviewed request source type evidence");
    }
    if (!row.destination_type_reviewed) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_destination_type_review,
                       "MAVG DirectStorage GDeflate execution requires reviewed request destination type evidence");
    }
    if (!row.reserved_request_bits_zero) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::reserved_request_bits_nonzero,
                       "MAVG DirectStorage GDeflate execution request option reserved bits must be zero");
    }
    if (!row.get_compression_support_queried) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_get_compression_support,
                       "MAVG DirectStorage GDeflate execution requires reviewed GetCompressionSupport evidence");
    }
    if (!has_gpu_compression_support(row)) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_gpu_compression_support,
                       "MAVG DirectStorage GDeflate execution requires GPU optimized or GPU fallback support");
    }
    if (row.compression_support_cpu_fallback) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::cpu_fallback_not_gpu_execution,
                       "MAVG DirectStorage GDeflate execution cannot treat CPU fallback as GPU execution evidence");
    }
    if (!has_gpu_compression_queue(row)) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_gpu_compression_queue,
                       "MAVG DirectStorage GDeflate execution requires compute or copy queue compression support");
    }
    if (!row.gpu_decompression_path_selected) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_gpu_decompression_path,
                       "MAVG DirectStorage GDeflate execution requires selected GPU decompression path evidence");
    }
    if (!row.staging_buffer_configured) {
        valid = false;
        add_diagnostic(
            result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_staging_buffer_configuration,
            "MAVG DirectStorage GDeflate execution requires reviewed GPU decompression staging buffer configuration");
    }
    if (!row.queue_device_bound) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_queue_device_binding,
                       "MAVG DirectStorage GDeflate execution requires a queue bound to the D3D12 destination device");
    }
    if (!row.destination_resource_device_matches_queue_device) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::destination_device_mismatch,
                       "MAVG DirectStorage GDeflate destination resources must match the queue device");
    }
    if (!row.enqueue_requests_used) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_enqueue_requests,
                       "MAVG DirectStorage GDeflate execution requires the EnqueueRequests API");
    }
    if (!row.d3d12_fence_synchronization_used) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_d3d12_fence_synchronization,
                       "MAVG DirectStorage GDeflate execution requires D3D12 fence synchronization");
    }
    if (!row.destination_resource_state_common) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_destination_state_common,
                       "MAVG DirectStorage GDeflate destination resources must start in D3D12_RESOURCE_STATE_COMMON");
    }
    if (!row.request_completed || row.compressed_bytes == 0U || row.decompressed_bytes == 0U ||
        row.completed_bytes == 0U || row.completed_bytes != row.decompressed_bytes) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_completed_request,
                       "MAVG DirectStorage GDeflate execution requires completed decompression with matching "
                       "decompressed byte counts");
    }
    if (!row.status_array_success) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_status_success,
                       "MAVG DirectStorage GDeflate execution requires a successful status array row");
    }
    if (!row.expected_decompressed_hash_ready) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_expected_hash,
                       "MAVG DirectStorage GDeflate execution requires expected decompressed hash evidence");
    }
    if (!row.readback_hash_ready) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_readback_hash,
                       "MAVG DirectStorage GDeflate execution requires deterministic readback hash evidence");
    }
    if (!row.readback_hash_matches_expected) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::readback_hash_mismatch,
                       "MAVG DirectStorage GDeflate execution readback hash must match expected decompressed output");
    }
    if (!row.package_visible_output_ready) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_package_visible_output,
                       "MAVG DirectStorage GDeflate execution requires package-visible output evidence");
    }
    if (blank(row.retained_artifact_id) || !is_lower_sha256(row.retained_artifact_sha256)) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::invalid_retained_artifact_hash,
                       "MAVG DirectStorage GDeflate execution evidence requires retained artifact id and lowercase "
                       "SHA-256 hash");
    }
    if (is_range_destination(row.destination) && row.num_subresources == 0U) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::invalid_subresource_range,
                       "MAVG DirectStorage GDeflate multiple-subresources-range evidence requires at least one "
                       "subresource");
    }

    return valid;
}

[[nodiscard]] bool has_blocking_invalid_evidence(const RuntimeMavgDirectStorageGDeflateExecutionRow& row) noexcept {
    return blank(row.retained_artifact_id) || !is_lower_sha256(row.retained_artifact_sha256) ||
           (is_range_destination(row.destination) && row.num_subresources == 0U) ||
           (row.completed_bytes != 0U && row.decompressed_bytes != 0U && row.completed_bytes != row.decompressed_bytes);
}

void count_ready_row(RuntimeMavgDirectStorageGDeflateExecutionResult& result,
                     const RuntimeMavgDirectStorageGDeflateExecutionRow& row) {
    ++result.ready_rows;
    ++result.gdeflate_execution_rows;
    ++result.gpu_destination_execution_rows;
    if (is_range_destination(row.destination)) {
        ++result.multiple_subresources_range_rows;
    }
    switch (row.source) {
    case RuntimeMavgDirectStorageGDeflateExecutionSourceKind::file:
        ++result.file_source_rows;
        break;
    case RuntimeMavgDirectStorageGDeflateExecutionSourceKind::memory:
        ++result.memory_source_rows;
        break;
    }
}

} // namespace

RuntimeMavgDirectStorageGDeflateExecutionResult evaluate_runtime_mavg_directstorage_gdeflate_execution_evidence(
    const RuntimeMavgDirectStorageGDeflateExecutionDesc& desc) {
    RuntimeMavgDirectStorageGDeflateExecutionResult result;

    bool has_ready_row = false;
    bool has_ready_range_row = false;
    bool blocked = false;

    for (const auto& row : desc.rows) {
        if (row.reviewed) {
            ++result.reviewed_rows;
        }

        validate_forbidden_claims(result, row);
        const bool row_forbidden = has_forbidden_execution_claim(row);
        const bool row_valid = validate_required_evidence(result, row);

        if (row_forbidden || has_blocking_invalid_evidence(row)) {
            ++result.blocked_rows;
            blocked = true;
            continue;
        }

        if (!row_valid) {
            ++result.host_evidence_required_rows;
            continue;
        }

        count_ready_row(result, row);
        has_ready_row = true;
        has_ready_range_row = has_ready_range_row || is_range_destination(row.destination);
    }

    if (blocked) {
        result.status = RuntimeMavgDirectStorageGDeflateExecutionStatus::blocked;
        return result;
    }
    if (!has_ready_row || !result.diagnostics.empty()) {
        result.status = RuntimeMavgDirectStorageGDeflateExecutionStatus::host_evidence_required;
        return result;
    }

    result.status = RuntimeMavgDirectStorageGDeflateExecutionStatus::ready;
    result.mavg_directstorage_gdeflate_execution_ready = true;
    result.mavg_directstorage_gpu_destination_execution_ready = true;
    result.mavg_directstorage_multiple_subresources_range_execution_ready = has_ready_range_row;
    return result;
}

bool has_runtime_mavg_directstorage_gdeflate_execution_diagnostic(
    const RuntimeMavgDirectStorageGDeflateExecutionResult& result,
    RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics,
                               [code](const RuntimeMavgDirectStorageGDeflateExecutionDiagnostic& diagnostic) {
                                   return diagnostic.code == code;
                               });
}

} // namespace mirakana::runtime_rhi
