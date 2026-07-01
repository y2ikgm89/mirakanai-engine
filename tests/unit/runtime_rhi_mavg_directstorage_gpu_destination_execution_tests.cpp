// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_directstorage_gpu_destination_execution.hpp"

#include <string_view>
#include <vector>

namespace {

using mirakana::runtime_rhi::RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGpuDestinationExecutionRow;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGpuDestinationExecutionStatus;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGpuDestinationKind;

constexpr std::string_view kHash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

[[nodiscard]] RuntimeMavgDirectStorageGpuDestinationExecutionRow make_range_row() {
    return RuntimeMavgDirectStorageGpuDestinationExecutionRow{
        .destination = RuntimeMavgDirectStorageGpuDestinationKind::d3d12_multiple_subresources_range,
        .row_id = "directstorage.gpu.destination.range.execution",
        .official_source_id = "microsoft-directstorage-1.3-enqueue-requests",
        .sdk_package_version = "1.3.0",
        .retained_artifact_id = "artifacts/mavg/directstorage/gpu-destination/range-evidence.json",
        .retained_artifact_sha256 = kHash,
        .requested_bytes = 4096,
        .completed_bytes = 4096,
        .first_subresource = 2,
        .num_subresources = 3,
        .reviewed = true,
        .ready = true,
        .stable_directstorage_13_selected = true,
        .queue_device_bound = true,
        .destination_resource_device_matches_queue_device = true,
        .enqueue_requests_used = true,
        .d3d12_fence_synchronization_used = true,
        .destination_resource_state_common = true,
        .request_completed = true,
        .status_array_success = true,
        .readback_hash_ready = true,
        .package_visible_output_ready = true,
    };
}

[[nodiscard]] bool has_code(const mirakana::runtime_rhi::RuntimeMavgDirectStorageGpuDestinationExecutionResult& result,
                            RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode code) noexcept {
    return mirakana::runtime_rhi::has_runtime_mavg_directstorage_gpu_destination_execution_diagnostic(result, code);
}

} // namespace

MK_TEST("runtime rhi mavg directstorage gpu destination execution accepts reviewed 1.3 range evidence") {
    const std::vector<RuntimeMavgDirectStorageGpuDestinationExecutionRow> rows{make_range_row()};

    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence({.rows = rows});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDestinationExecutionStatus::ready);
    MK_REQUIRE(result.reviewed_rows == 1U);
    MK_REQUIRE(result.ready_rows == 1U);
    MK_REQUIRE(result.gpu_destination_execution_rows == 1U);
    MK_REQUIRE(result.multiple_subresources_range_rows == 1U);
    MK_REQUIRE(result.mavg_directstorage_gpu_destination_execution_ready);
    MK_REQUIRE(result.mavg_directstorage_multiple_subresources_range_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_gdeflate_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_zstd_preview_ready);
    MK_REQUIRE(!result.mavg_directstorage_performance_ready);
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
    MK_REQUIRE(!result.mavg_external_engine_compatibility);
}

MK_TEST("runtime rhi mavg directstorage gpu destination execution host gates missing execution evidence") {
    auto row = make_range_row();
    row.queue_device_bound = false;
    row.enqueue_requests_used = false;
    row.d3d12_fence_synchronization_used = false;
    row.destination_resource_state_common = false;
    row.request_completed = false;
    row.status_array_success = false;
    row.readback_hash_ready = false;
    row.package_visible_output_ready = false;

    const std::vector<RuntimeMavgDirectStorageGpuDestinationExecutionRow> rows{row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDestinationExecutionStatus::host_evidence_required);
    MK_REQUIRE(result.host_evidence_required_rows == 1U);
    MK_REQUIRE(!result.mavg_directstorage_gpu_destination_execution_ready);
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_queue_device_binding));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_enqueue_requests));
    MK_REQUIRE(has_code(
        result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_d3d12_fence_synchronization));
    MK_REQUIRE(has_code(
        result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_destination_state_common));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_completed_request));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_status_success));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_readback_hash));
    MK_REQUIRE(has_code(result,
                        RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_package_visible_output));
}

MK_TEST("runtime rhi mavg directstorage gpu destination execution blocks invalid artifacts and ranges") {
    auto row = make_range_row();
    row.retained_artifact_sha256 = "not-a-sha256";
    row.completed_bytes = 1024;
    row.num_subresources = 0;

    const std::vector<RuntimeMavgDirectStorageGpuDestinationExecutionRow> rows{row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDestinationExecutionStatus::blocked);
    MK_REQUIRE(result.blocked_rows == 1U);
    MK_REQUIRE(has_code(result,
                        RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::invalid_retained_artifact_hash));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::invalid_subresource_range));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::missing_completed_request));
}

MK_TEST("runtime rhi mavg directstorage gpu destination execution keeps preview and decompression claims blocked") {
    auto row = make_range_row();
    row.directstorage_14_preview_selected = true;
    row.claims_gdeflate_execution_ready = true;
    row.claims_zstd_preview_ready = true;
    row.claims_performance_gain = true;
    row.claims_package_visible_backend_readiness = true;
    row.claims_broad_mavg_backend_ready = true;
    row.claims_broad_optimization_ready = true;
    row.claims_nanite_compatibility = true;
    row.claims_nanite_equivalence = true;
    row.claims_nanite_superiority = true;
    row.claims_unity_unreal_godot_compatibility = true;

    const std::vector<RuntimeMavgDirectStorageGpuDestinationExecutionRow> rows{row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDestinationExecutionStatus::blocked);
    MK_REQUIRE(!result.mavg_directstorage_gpu_destination_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_gdeflate_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_zstd_preview_ready);
    MK_REQUIRE(!result.mavg_directstorage_performance_ready);
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
    MK_REQUIRE(!result.mavg_external_engine_compatibility);
    MK_REQUIRE(has_code(result,
                        RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::directstorage_preview_selected));
    MK_REQUIRE(has_code(
        result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::gdeflate_execution_claim_not_allowed));
    MK_REQUIRE(has_code(result,
                        RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::zstd_preview_claim_not_allowed));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::performance_claim_not_allowed));
    MK_REQUIRE(has_code(
        result,
        RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::package_backend_readiness_claim_not_allowed));
    MK_REQUIRE(has_code(
        result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::broad_backend_claim_not_allowed));
    MK_REQUIRE(has_code(
        result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::broad_optimization_claim_not_allowed));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::nanite_claim_not_allowed));
    MK_REQUIRE(has_code(
        result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::external_engine_claim_not_allowed));
}

MK_TEST("runtime rhi mavg directstorage gpu destination execution accepts buffer evidence without range readiness") {
    auto row = make_range_row();
    row.destination = RuntimeMavgDirectStorageGpuDestinationKind::d3d12_buffer;
    row.first_subresource = 0;
    row.num_subresources = 0;

    const std::vector<RuntimeMavgDirectStorageGpuDestinationExecutionRow> rows{row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence({.rows = rows});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.mavg_directstorage_gpu_destination_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_multiple_subresources_range_execution_ready);
}

MK_TEST("runtime rhi mavg directstorage gpu destination execution blocks native handle exposure") {
    auto row = make_range_row();
    row.native_handles_exposed = true;

    const std::vector<RuntimeMavgDirectStorageGpuDestinationExecutionRow> rows{row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDestinationExecutionStatus::blocked);
    MK_REQUIRE(result.mavg_directstorage_native_handles_exposed);
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGpuDestinationExecutionDiagnosticCode::native_handle_exposure));
}

int main() {
    return mirakana::test::run_all();
}
