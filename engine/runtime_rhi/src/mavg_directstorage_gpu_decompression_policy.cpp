// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_directstorage_gpu_decompression_policy.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mirakana::runtime_rhi {

std::string_view runtime_mavg_directstorage_gpu_decompression_status_label(
    RuntimeMavgDirectStorageGpuDecompressionStatus status) noexcept {
    switch (status) {
    case RuntimeMavgDirectStorageGpuDecompressionStatus::review_required:
        return "review_required";
    case RuntimeMavgDirectStorageGpuDecompressionStatus::blocked:
        return "blocked";
    case RuntimeMavgDirectStorageGpuDecompressionStatus::policy_ready:
        return "policy_ready";
    }
    return "unknown";
}

namespace {

void add_diagnostic(RuntimeMavgDirectStorageGpuDecompressionPolicyResult& result,
                    RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode code, std::string message) {
    result.diagnostics.push_back(
        RuntimeMavgDirectStorageGpuDecompressionDiagnostic{.code = code, .message = std::move(message)});
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

[[nodiscard]] bool is_gpu_destination(RuntimeMavgDirectStorageGpuDecompressionDestination destination) noexcept {
    switch (destination) {
    case RuntimeMavgDirectStorageGpuDecompressionDestination::d3d12_buffer:
    case RuntimeMavgDirectStorageGpuDecompressionDestination::d3d12_texture:
    case RuntimeMavgDirectStorageGpuDecompressionDestination::d3d12_multiple_subresources_range:
        return true;
    }
    return false;
}

[[nodiscard]] bool has_blocked_claims(const RuntimeMavgDirectStorageGpuDecompressionPolicyRow& row) noexcept {
    return row.native_handles_exposed || row.claims_performance_gain || row.claims_package_visible_backend_readiness ||
           row.claims_broad_mavg_backend_ready || row.claims_broad_optimization_ready ||
           row.claims_nanite_compatibility || row.claims_nanite_equivalence || row.claims_nanite_superiority;
}

void validate_blocked_claims(RuntimeMavgDirectStorageGpuDecompressionPolicyResult& result,
                             const RuntimeMavgDirectStorageGpuDecompressionPolicyRow& row) {
    if (row.native_handles_exposed) {
        result.mavg_directstorage_native_handles_exposed = true;
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::native_handle_exposure,
                       "MAVG DirectStorage/GDeflate policy rows must not expose native handles or SDK objects");
    }
    if (row.claims_performance_gain) {
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::performance_claim_not_allowed,
                       "MAVG DirectStorage/GDeflate policy rows must not claim performance readiness");
    }
    if (row.claims_package_visible_backend_readiness) {
        add_diagnostic(
            result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::package_backend_readiness_claim_not_allowed,
            "MAVG DirectStorage/GDeflate policy rows must not promote package-visible backend readiness");
    }
    if (row.claims_broad_mavg_backend_ready) {
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::broad_backend_claim_not_allowed,
                       "MAVG DirectStorage/GDeflate policy rows must not promote broad MAVG backend readiness");
    }
    if (row.claims_broad_optimization_ready) {
        add_diagnostic(result,
                       RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::broad_optimization_claim_not_allowed,
                       "MAVG DirectStorage/GDeflate policy rows must not promote broad CPU/GPU/memory optimization");
    }
    if (row.claims_nanite_compatibility || row.claims_nanite_equivalence || row.claims_nanite_superiority) {
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::nanite_claim_not_allowed,
                       "MAVG DirectStorage/GDeflate policy rows must not claim Nanite compatibility, equivalence, or "
                       "superiority");
    }
}

[[nodiscard]] bool has_execution_claim(const RuntimeMavgDirectStorageGpuDecompressionPolicyRow& row) noexcept {
    return row.claims_gpu_destination_ready || row.claims_gdeflate_ready || row.execution_artifact_ready;
}

void validate_execution_claim_artifact(RuntimeMavgDirectStorageGpuDecompressionPolicyResult& result,
                                       const RuntimeMavgDirectStorageGpuDecompressionPolicyRow& row) {
    if (!has_execution_claim(row)) {
        return;
    }
    if (blank(row.retained_artifact_id)) {
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::execution_claim_without_artifact,
                       "MAVG DirectStorage/GDeflate execution claims require retained artifact identity before future "
                       "execution review");
    }
    if (!is_lower_sha256(row.retained_artifact_sha256)) {
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::invalid_retained_artifact_hash,
                       "MAVG DirectStorage/GDeflate retained artifact hashes must be lowercase SHA-256");
    }
}

} // namespace

RuntimeMavgDirectStorageGpuDecompressionPolicyResult evaluate_runtime_mavg_directstorage_gpu_decompression_policy(
    const RuntimeMavgDirectStorageGpuDecompressionPolicyDesc& desc) {
    RuntimeMavgDirectStorageGpuDecompressionPolicyResult result;

    bool has_stable_gpu_destination = false;
    bool has_gdeflate_policy = false;
    bool blocked = false;

    for (const auto& row : desc.rows) {
        bool row_blocked = false;
        const bool is_preview = row.directstorage_14_preview_selected ||
                                row.compression == RuntimeMavgDirectStorageCompressionFormat::zstd_preview;
        const bool is_gdeflate = row.compression == RuntimeMavgDirectStorageCompressionFormat::gdeflate;

        if (row.reviewed) {
            ++result.reviewed_rows;
        }
        if (row.gpu_resource_destination_reviewed && is_gpu_destination(row.destination)) {
            ++result.gpu_destination_policy_rows;
        }
        if (is_gdeflate && row.gdeflate_compression_reviewed) {
            ++result.gdeflate_policy_rows;
        }
        if (is_preview) {
            ++result.preview_rows;
            result.mavg_directstorage_zstd_preview_selected = true;
            add_diagnostic(result,
                           RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::directstorage_preview_selected,
                           "DirectStorage 1.4/Zstd/GACL rows are public preview and cannot promote stable policy or "
                           "execution readiness");
            add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::zstd_preview_not_ready,
                           "DirectStorage Zstd preview requires explicitly authored Zstd/GACL assets and explicit API "
                           "usage before any future promotion");
        }

        if (blank(row.official_source_id)) {
            add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_official_source,
                           "MAVG DirectStorage/GDeflate policy rows require Microsoft official source ids");
        }
        if (blank(row.sdk_package_version)) {
            add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_sdk_version,
                           "MAVG DirectStorage/GDeflate policy rows require a reviewed DirectStorage SDK version");
        }
        if (!is_preview &&
            (!row.reviewed || !row.stable_directstorage_13_selected || !row.directstorage_13_feature_reviewed)) {
            add_diagnostic(result,
                           RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_directstorage_13_review,
                           "Stable MAVG DirectStorage/GDeflate policy rows require reviewed DirectStorage 1.3 "
                           "destination/API evidence");
        }
        if (!row.gpu_resource_destination_reviewed) {
            add_diagnostic(result,
                           RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_gpu_destination_review,
                           "MAVG DirectStorage GPU destination policy requires reviewed D3D12 destination evidence");
        }
        if (!row.d3d12_fence_synchronization_reviewed) {
            add_diagnostic(
                result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_d3d12_synchronization_review,
                "MAVG DirectStorage GPU destination policy requires reviewed D3D12 fence/resource synchronization");
        }
        if (is_gdeflate && !row.gdeflate_compression_reviewed) {
            add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_gdeflate_review,
                           "MAVG GDeflate policy rows require reviewed DirectStorage GPU decompression guidance");
        }

        if (!is_preview && row.reviewed && row.stable_directstorage_13_selected &&
            row.directstorage_13_feature_reviewed && row.gpu_resource_destination_reviewed &&
            row.d3d12_fence_synchronization_reviewed) {
            has_stable_gpu_destination = true;
        }
        if (is_gdeflate && row.reviewed && row.gdeflate_compression_reviewed) {
            has_gdeflate_policy = true;
        }

        validate_blocked_claims(result, row);
        validate_execution_claim_artifact(result, row);
        row_blocked = has_blocked_claims(row) || has_execution_claim(row);
        if (row_blocked) {
            ++result.blocked_rows;
            blocked = true;
        }
    }

    if (!has_gdeflate_policy) {
        add_diagnostic(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_gdeflate_review,
                       "MAVG DirectStorage GPU decompression policy requires one reviewed GDeflate policy row");
    }

    result.mavg_directstorage_gpu_destination_policy_ready = has_stable_gpu_destination;
    result.mavg_directstorage_gdeflate_policy_ready = has_gdeflate_policy;

    if (blocked) {
        result.status = RuntimeMavgDirectStorageGpuDecompressionStatus::blocked;
        return result;
    }
    if (!result.diagnostics.empty() || !has_stable_gpu_destination || !has_gdeflate_policy) {
        result.status = RuntimeMavgDirectStorageGpuDecompressionStatus::review_required;
        return result;
    }

    result.status = RuntimeMavgDirectStorageGpuDecompressionStatus::policy_ready;
    result.mavg_directstorage_gpu_decompression_policy_ready = true;
    return result;
}

bool has_runtime_mavg_directstorage_gpu_decompression_diagnostic(
    const RuntimeMavgDirectStorageGpuDecompressionPolicyResult& result,
    RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics,
                               [code](const RuntimeMavgDirectStorageGpuDecompressionDiagnostic& diagnostic) {
                                   return diagnostic.code == code;
                               });
}

} // namespace mirakana::runtime_rhi
