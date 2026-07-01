// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_directstorage_gdeflate_execution.hpp"

#include <string_view>
#include <vector>

namespace {

using mirakana::runtime_rhi::RuntimeMavgDirectStorageGDeflateDestinationKind;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGDeflateExecutionRow;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGDeflateExecutionSourceKind;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGDeflateExecutionStatus;

constexpr std::string_view kHash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

[[nodiscard]] RuntimeMavgDirectStorageGDeflateExecutionRow make_range_row() {
    return RuntimeMavgDirectStorageGDeflateExecutionRow{
        .destination = RuntimeMavgDirectStorageGDeflateDestinationKind::d3d12_multiple_subresources_range,
        .source = RuntimeMavgDirectStorageGDeflateExecutionSourceKind::file,
        .row_id = "directstorage.gdeflate.range.execution",
        .official_source_id = "microsoft-directstorage-compression-format",
        .sdk_package_version = "1.3.0",
        .retained_artifact_id = "artifacts/mavg/directstorage/gdeflate/range-evidence.json",
        .retained_artifact_sha256 = kHash,
        .compressed_bytes = 2048,
        .decompressed_bytes = 4096,
        .completed_bytes = 4096,
        .first_subresource = 2,
        .num_subresources = 3,
        .reviewed = true,
        .ready = true,
        .stable_directstorage_11_gdeflate_selected = true,
        .stable_directstorage_13_selected = true,
        .request_options_reviewed = true,
        .compression_format_gdeflate = true,
        .source_type_reviewed = true,
        .destination_type_reviewed = true,
        .reserved_request_bits_zero = true,
        .get_compression_support_queried = true,
        .compression_support_gpu_optimized = true,
        .compression_support_gpu_fallback = false,
        .compression_support_cpu_fallback = false,
        .compression_support_uses_compute_queue = true,
        .compression_support_uses_copy_queue = false,
        .gpu_decompression_path_selected = true,
        .staging_buffer_configured = true,
        .queue_device_bound = true,
        .destination_resource_device_matches_queue_device = true,
        .enqueue_requests_used = true,
        .d3d12_fence_synchronization_used = true,
        .destination_resource_state_common = true,
        .request_completed = true,
        .status_array_success = true,
        .expected_decompressed_hash_ready = true,
        .readback_hash_ready = true,
        .readback_hash_matches_expected = true,
        .package_visible_output_ready = true,
    };
}

[[nodiscard]] bool has_code(const mirakana::runtime_rhi::RuntimeMavgDirectStorageGDeflateExecutionResult& result,
                            RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode code) noexcept {
    return mirakana::runtime_rhi::has_runtime_mavg_directstorage_gdeflate_execution_diagnostic(result, code);
}

} // namespace

MK_TEST("runtime rhi mavg directstorage gdeflate execution accepts reviewed gdeflate range evidence") {
    const std::vector<RuntimeMavgDirectStorageGDeflateExecutionRow> rows{make_range_row()};

    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gdeflate_execution_evidence({.rows = rows});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGDeflateExecutionStatus::ready);
    MK_REQUIRE(result.reviewed_rows == 1U);
    MK_REQUIRE(result.ready_rows == 1U);
    MK_REQUIRE(result.gdeflate_execution_rows == 1U);
    MK_REQUIRE(result.gpu_destination_execution_rows == 1U);
    MK_REQUIRE(result.multiple_subresources_range_rows == 1U);
    MK_REQUIRE(result.file_source_rows == 1U);
    MK_REQUIRE(result.memory_source_rows == 0U);
    MK_REQUIRE(result.mavg_directstorage_gdeflate_execution_ready);
    MK_REQUIRE(result.mavg_directstorage_gpu_destination_execution_ready);
    MK_REQUIRE(result.mavg_directstorage_multiple_subresources_range_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_zstd_preview_ready);
    MK_REQUIRE(!result.mavg_directstorage_native_handles_exposed);
    MK_REQUIRE(!result.mavg_directstorage_performance_ready);
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
    MK_REQUIRE(!result.mavg_external_engine_compatibility);
}

MK_TEST("runtime rhi mavg directstorage gdeflate execution host gates missing request and hash evidence") {
    auto row = make_range_row();
    row.request_options_reviewed = false;
    row.compression_format_gdeflate = false;
    row.source_type_reviewed = false;
    row.destination_type_reviewed = false;
    row.reserved_request_bits_zero = false;
    row.get_compression_support_queried = false;
    row.compression_support_gpu_optimized = false;
    row.compression_support_cpu_fallback = true;
    row.compression_support_uses_compute_queue = false;
    row.gpu_decompression_path_selected = false;
    row.staging_buffer_configured = false;
    row.queue_device_bound = false;
    row.enqueue_requests_used = false;
    row.d3d12_fence_synchronization_used = false;
    row.expected_decompressed_hash_ready = false;
    row.readback_hash_ready = false;
    row.readback_hash_matches_expected = false;

    const std::vector<RuntimeMavgDirectStorageGDeflateExecutionRow> rows{row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gdeflate_execution_evidence({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGDeflateExecutionStatus::host_evidence_required);
    MK_REQUIRE(result.host_evidence_required_rows == 1U);
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_request_options_review));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_gdeflate_compression_format));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_source_type_review));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_destination_type_review));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::reserved_request_bits_nonzero));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_get_compression_support));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_gpu_compression_support));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::cpu_fallback_not_gpu_execution));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_gpu_compression_queue));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_gpu_decompression_path));
    MK_REQUIRE(has_code(result,
                        RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_staging_buffer_configuration));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_queue_device_binding));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_enqueue_requests));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_d3d12_fence_synchronization));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_expected_hash));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_readback_hash));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::readback_hash_mismatch));
}

MK_TEST("runtime rhi mavg directstorage gdeflate execution blocks invalid artifacts ranges and byte counts") {
    auto row = make_range_row();
    row.retained_artifact_sha256 = "not-a-sha256";
    row.completed_bytes = 1024;
    row.num_subresources = 0;

    const std::vector<RuntimeMavgDirectStorageGDeflateExecutionRow> rows{row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gdeflate_execution_evidence({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGDeflateExecutionStatus::blocked);
    MK_REQUIRE(result.blocked_rows == 1U);
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::invalid_retained_artifact_hash));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::invalid_subresource_range));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::missing_completed_request));
}

MK_TEST("runtime rhi mavg directstorage gdeflate execution blocks preview and unsupported claims") {
    auto row = make_range_row();
    row.directstorage_14_preview_selected = true;
    row.claims_zstd_preview_ready = true;
    row.native_handles_exposed = true;
    row.claims_performance_gain = true;
    row.claims_package_visible_backend_readiness = true;
    row.claims_broad_mavg_backend_ready = true;
    row.claims_broad_optimization_ready = true;
    row.claims_nanite_compatibility = true;
    row.claims_nanite_equivalence = true;
    row.claims_nanite_superiority = true;
    row.claims_unity_unreal_godot_compatibility = true;

    const std::vector<RuntimeMavgDirectStorageGDeflateExecutionRow> rows{row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gdeflate_execution_evidence({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGDeflateExecutionStatus::blocked);
    MK_REQUIRE(!result.mavg_directstorage_gdeflate_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_zstd_preview_ready);
    MK_REQUIRE(result.mavg_directstorage_native_handles_exposed);
    MK_REQUIRE(!result.mavg_directstorage_performance_ready);
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
    MK_REQUIRE(!result.mavg_external_engine_compatibility);
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::directstorage_preview_selected));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::zstd_preview_claim_not_allowed));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::native_handle_exposure));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::performance_claim_not_allowed));
    MK_REQUIRE(has_code(
        result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::package_backend_readiness_claim_not_allowed));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::broad_backend_claim_not_allowed));
    MK_REQUIRE(has_code(result,
                        RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::broad_optimization_claim_not_allowed));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::nanite_claim_not_allowed));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGDeflateExecutionDiagnosticCode::external_engine_claim_not_allowed));
}

MK_TEST("runtime rhi mavg directstorage gdeflate execution accepts buffer memory evidence without range readiness") {
    auto row = make_range_row();
    row.destination = RuntimeMavgDirectStorageGDeflateDestinationKind::d3d12_buffer;
    row.source = RuntimeMavgDirectStorageGDeflateExecutionSourceKind::memory;
    row.first_subresource = 0;
    row.num_subresources = 0;

    const std::vector<RuntimeMavgDirectStorageGDeflateExecutionRow> rows{row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gdeflate_execution_evidence({.rows = rows});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.memory_source_rows == 1U);
    MK_REQUIRE(result.file_source_rows == 0U);
    MK_REQUIRE(result.mavg_directstorage_gdeflate_execution_ready);
    MK_REQUIRE(result.mavg_directstorage_gpu_destination_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_multiple_subresources_range_execution_ready);
}

int main() {
    return mirakana::test::run_all();
}
