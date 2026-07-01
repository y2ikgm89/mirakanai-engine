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

enum class RuntimeMavgDirectStorageGpuDestinationKind : std::uint8_t {
    d3d12_buffer,
    d3d12_texture_region,
    d3d12_multiple_subresources_range,
};

enum class RuntimeMavgDirectStorageGpuDestinationExecutionStatus : std::uint8_t {
    host_evidence_required,
    blocked,
    ready,
};

enum class RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode : std::uint8_t {
    none,
    missing_official_source,
    missing_sdk_version,
    missing_stable_directstorage_13,
    directstorage_preview_selected,
    missing_queue_device_binding,
    destination_device_mismatch,
    missing_enqueue_requests,
    missing_d3d12_fence_synchronization,
    missing_destination_state_common,
    missing_completed_request,
    missing_status_success,
    missing_readback_hash,
    missing_package_visible_output,
    invalid_subresource_range,
    invalid_retained_artifact_hash,
    native_handle_exposure,
    gdeflate_execution_claim_not_allowed,
    zstd_preview_claim_not_allowed,
    performance_claim_not_allowed,
    package_backend_readiness_claim_not_allowed,
    broad_backend_claim_not_allowed,
    broad_optimization_claim_not_allowed,
    nanite_claim_not_allowed,
    external_engine_claim_not_allowed,
};

struct RuntimeMavgDirectStorageGpuDestinationExecutionDiagnostic {
    RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode code{
        RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::none};
    std::string message;
};

struct RuntimeMavgDirectStorageGpuDestinationExecutionRow {
    RuntimeMavgDirectStorageGpuDestinationKind destination{RuntimeMavgDirectStorageGpuDestinationKind::d3d12_buffer};
    std::string_view row_id;
    std::string_view official_source_id;
    std::string_view sdk_package_version;
    std::string_view retained_artifact_id;
    std::string_view retained_artifact_sha256;
    std::uint64_t requested_bytes{0};
    std::uint64_t completed_bytes{0};
    std::uint32_t first_subresource{0};
    std::uint32_t num_subresources{0};
    bool reviewed{false};
    bool ready{false};
    bool stable_directstorage_13_selected{false};
    bool directstorage_14_preview_selected{false};
    bool queue_device_bound{false};
    bool destination_resource_device_matches_queue_device{false};
    bool enqueue_requests_used{false};
    bool d3d12_fence_synchronization_used{false};
    bool destination_resource_state_common{false};
    bool request_completed{false};
    bool status_array_success{false};
    bool readback_hash_ready{false};
    bool package_visible_output_ready{false};
    bool native_handles_exposed{false};
    bool claims_gdeflate_execution_ready{false};
    bool claims_zstd_preview_ready{false};
    bool claims_performance_gain{false};
    bool claims_package_visible_backend_readiness{false};
    bool claims_broad_mavg_backend_ready{false};
    bool claims_broad_optimization_ready{false};
    bool claims_nanite_compatibility{false};
    bool claims_nanite_equivalence{false};
    bool claims_nanite_superiority{false};
    bool claims_unity_unreal_godot_compatibility{false};
};

struct RuntimeMavgDirectStorageGpuDestinationExecutionDesc {
    std::span<const RuntimeMavgDirectStorageGpuDestinationExecutionRow> rows;
};

struct RuntimeMavgDirectStorageGpuDestinationExecutionResult {
    RuntimeMavgDirectStorageGpuDestinationExecutionStatus status{
        RuntimeMavgDirectStorageGpuDestinationExecutionStatus::host_evidence_required};
    std::vector<RuntimeMavgDirectStorageGpuDestinationExecutionDiagnostic> diagnostics;
    std::size_t reviewed_rows{0};
    std::size_t ready_rows{0};
    std::size_t gpu_destination_execution_rows{0};
    std::size_t multiple_subresources_range_rows{0};
    std::size_t blocked_rows{0};
    std::size_t host_evidence_required_rows{0};
    bool mavg_directstorage_gpu_destination_execution_ready{false};
    bool mavg_directstorage_multiple_subresources_range_execution_ready{false};
    bool mavg_directstorage_gdeflate_execution_ready{false};
    bool mavg_directstorage_zstd_preview_ready{false};
    bool mavg_directstorage_native_handles_exposed{false};
    bool mavg_directstorage_performance_ready{false};
    bool mavg_package_visible_backend_readiness_ready{false};
    bool mavg_broad_cpu_gpu_memory_optimization_ready{false};
    bool mavg_nanite_compatible{false};
    bool mavg_nanite_equivalent{false};
    bool mavg_nanite_superior{false};
    bool mavg_external_engine_compatibility{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == RuntimeMavgDirectStorageGpuDestinationExecutionStatus::ready &&
               mavg_directstorage_gpu_destination_execution_ready;
    }
};

[[nodiscard]] std::string_view runtime_mavg_directstorage_gpu_destination_execution_status_label(
    RuntimeMavgDirectStorageGpuDestinationExecutionStatus status) noexcept;
[[nodiscard]] RuntimeMavgDirectStorageGpuDestinationExecutionResult
evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence(
    const RuntimeMavgDirectStorageGpuDestinationExecutionDesc& desc);
[[nodiscard]] bool has_runtime_mavg_directstorage_gpu_destination_execution_diagnostic(
    const RuntimeMavgDirectStorageGpuDestinationExecutionResult& result,
    RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
