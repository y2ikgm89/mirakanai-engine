// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgDirectStorageGpuDecompressionDestination : std::uint8_t {
    d3d12_buffer,
    d3d12_texture,
    d3d12_multiple_subresources_range,
};

enum class RuntimeMavgDirectStorageCompressionFormat : std::uint8_t {
    none,
    gdeflate,
    zstd_preview,
};

enum class RuntimeMavgDirectStorageGpuDecompressionStatus : std::uint8_t {
    review_required,
    blocked,
    policy_ready,
};

enum class RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode : std::uint8_t {
    none,
    missing_official_source,
    missing_sdk_version,
    missing_directstorage_13_review,
    missing_gpu_destination_review,
    missing_d3d12_synchronization_review,
    missing_gdeflate_review,
    directstorage_preview_selected,
    zstd_preview_not_ready,
    native_handle_exposure,
    performance_claim_not_allowed,
    broad_backend_claim_not_allowed,
    broad_optimization_claim_not_allowed,
    package_backend_readiness_claim_not_allowed,
    nanite_claim_not_allowed,
    execution_claim_without_artifact,
    invalid_retained_artifact_hash,
};

struct RuntimeMavgDirectStorageGpuDecompressionDiagnostic {
    RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode code{
        RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::none};
    std::string message;
};

struct RuntimeMavgDirectStorageGpuDecompressionPolicyRow {
    RuntimeMavgDirectStorageGpuDecompressionDestination destination{
        RuntimeMavgDirectStorageGpuDecompressionDestination::d3d12_buffer};
    RuntimeMavgDirectStorageCompressionFormat compression{RuntimeMavgDirectStorageCompressionFormat::none};
    std::string_view row_id;
    std::string_view official_source_id;
    std::string_view sdk_package_version;
    std::string_view retained_artifact_id;
    std::string_view retained_artifact_sha256;
    bool reviewed{false};
    bool stable_directstorage_13_selected{false};
    bool directstorage_13_feature_reviewed{false};
    bool directstorage_14_preview_selected{false};
    bool gpu_resource_destination_reviewed{false};
    bool d3d12_fence_synchronization_reviewed{false};
    bool gdeflate_compression_reviewed{false};
    bool zstd_preview_reviewed{false};
    bool execution_artifact_ready{false};
    bool claims_gpu_destination_ready{false};
    bool claims_gdeflate_ready{false};
    bool native_handles_exposed{false};
    bool claims_performance_gain{false};
    bool claims_package_visible_backend_readiness{false};
    bool claims_broad_mavg_backend_ready{false};
    bool claims_broad_optimization_ready{false};
    bool claims_nanite_compatibility{false};
    bool claims_nanite_equivalence{false};
    bool claims_nanite_superiority{false};
};

struct RuntimeMavgDirectStorageGpuDecompressionPolicyDesc {
    std::span<const RuntimeMavgDirectStorageGpuDecompressionPolicyRow> rows;
};

struct RuntimeMavgDirectStorageGpuDecompressionPolicyResult {
    RuntimeMavgDirectStorageGpuDecompressionStatus status{
        RuntimeMavgDirectStorageGpuDecompressionStatus::review_required};
    std::vector<RuntimeMavgDirectStorageGpuDecompressionDiagnostic> diagnostics;
    std::size_t reviewed_rows{0};
    std::size_t gpu_destination_policy_rows{0};
    std::size_t gdeflate_policy_rows{0};
    std::size_t preview_rows{0};
    std::size_t blocked_rows{0};
    bool mavg_directstorage_gpu_decompression_policy_ready{false};
    bool mavg_directstorage_gpu_destination_policy_ready{false};
    bool mavg_directstorage_gdeflate_policy_ready{false};
    bool mavg_directstorage_gpu_destination_execution_ready{false};
    bool mavg_directstorage_gdeflate_execution_ready{false};
    bool mavg_directstorage_zstd_preview_ready{false};
    bool mavg_directstorage_zstd_preview_selected{false};
    bool mavg_directstorage_native_handles_exposed{false};
    bool mavg_directstorage_performance_ready{false};
    bool mavg_package_visible_backend_readiness_ready{false};
    bool mavg_broad_cpu_gpu_memory_optimization_ready{false};
    bool mavg_nanite_compatible{false};
    bool mavg_nanite_equivalent{false};
    bool mavg_nanite_superior{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == RuntimeMavgDirectStorageGpuDecompressionStatus::policy_ready &&
               mavg_directstorage_gpu_decompression_policy_ready;
    }
};

[[nodiscard]] std::string_view runtime_mavg_directstorage_gpu_decompression_status_label(
    RuntimeMavgDirectStorageGpuDecompressionStatus status) noexcept;
[[nodiscard]] RuntimeMavgDirectStorageGpuDecompressionPolicyResult
evaluate_runtime_mavg_directstorage_gpu_decompression_policy(
    const RuntimeMavgDirectStorageGpuDecompressionPolicyDesc& desc);
[[nodiscard]] bool has_runtime_mavg_directstorage_gpu_decompression_diagnostic(
    const RuntimeMavgDirectStorageGpuDecompressionPolicyResult& result,
    RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
