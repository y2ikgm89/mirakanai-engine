// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_directstorage_gpu_decompression_policy.hpp"

#include <string_view>
#include <vector>

namespace {

using mirakana::runtime_rhi::RuntimeMavgDirectStorageCompressionFormat;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGpuDecompressionDestination;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGpuDecompressionPolicyRow;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageGpuDecompressionStatus;

constexpr std::string_view kHash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

[[nodiscard]] RuntimeMavgDirectStorageGpuDecompressionPolicyRow make_gpu_destination_row() {
    return RuntimeMavgDirectStorageGpuDecompressionPolicyRow{
        .destination = RuntimeMavgDirectStorageGpuDecompressionDestination::d3d12_multiple_subresources_range,
        .compression = RuntimeMavgDirectStorageCompressionFormat::none,
        .row_id = "directstorage.gpu.destination.policy",
        .official_source_id = "microsoft-directstorage-1.3-release",
        .sdk_package_version = "1.3.0",
        .reviewed = true,
        .stable_directstorage_13_selected = true,
        .directstorage_13_feature_reviewed = true,
        .gpu_resource_destination_reviewed = true,
        .d3d12_fence_synchronization_reviewed = true,
    };
}

[[nodiscard]] RuntimeMavgDirectStorageGpuDecompressionPolicyRow make_gdeflate_row() {
    return RuntimeMavgDirectStorageGpuDecompressionPolicyRow{
        .destination = RuntimeMavgDirectStorageGpuDecompressionDestination::d3d12_buffer,
        .compression = RuntimeMavgDirectStorageCompressionFormat::gdeflate,
        .row_id = "directstorage.gdeflate.policy",
        .official_source_id = "microsoft-directstorage-developer-guidance",
        .sdk_package_version = "1.3.0",
        .reviewed = true,
        .stable_directstorage_13_selected = true,
        .directstorage_13_feature_reviewed = true,
        .gpu_resource_destination_reviewed = true,
        .d3d12_fence_synchronization_reviewed = true,
        .gdeflate_compression_reviewed = true,
    };
}

[[nodiscard]] bool has_code(const mirakana::runtime_rhi::RuntimeMavgDirectStorageGpuDecompressionPolicyResult& result,
                            RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode code) noexcept {
    return mirakana::runtime_rhi::has_runtime_mavg_directstorage_gpu_decompression_diagnostic(result, code);
}

} // namespace

MK_TEST(
    "runtime rhi mavg directstorage gpu decompression policy accepts reviewed 1.3 gpu destination and gdeflate rows") {
    const std::vector<RuntimeMavgDirectStorageGpuDecompressionPolicyRow> rows{
        make_gpu_destination_row(),
        make_gdeflate_row(),
    };

    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_decompression_policy({.rows = rows});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDecompressionStatus::policy_ready);
    MK_REQUIRE(result.reviewed_rows == 2U);
    MK_REQUIRE(result.gpu_destination_policy_rows == 2U);
    MK_REQUIRE(result.gdeflate_policy_rows == 1U);
    MK_REQUIRE(result.mavg_directstorage_gpu_decompression_policy_ready);
    MK_REQUIRE(result.mavg_directstorage_gpu_destination_policy_ready);
    MK_REQUIRE(result.mavg_directstorage_gdeflate_policy_ready);
    MK_REQUIRE(!result.mavg_directstorage_gpu_destination_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_gdeflate_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_zstd_preview_ready);
    MK_REQUIRE(!result.mavg_directstorage_zstd_preview_selected);
    MK_REQUIRE(!result.mavg_directstorage_native_handles_exposed);
    MK_REQUIRE(!result.mavg_directstorage_performance_ready);
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
}

MK_TEST("runtime rhi mavg directstorage gpu decompression policy host gates missing official review rows") {
    auto row = make_gpu_destination_row();
    row.official_source_id = "";
    row.gpu_resource_destination_reviewed = false;
    row.d3d12_fence_synchronization_reviewed = false;

    const std::vector<RuntimeMavgDirectStorageGpuDecompressionPolicyRow> rows{row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_decompression_policy({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDecompressionStatus::review_required);
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_official_source));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_gpu_destination_review));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_d3d12_synchronization_review));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::missing_gdeflate_review));
}

MK_TEST("runtime rhi mavg directstorage gpu decompression policy leaves zstd preview host gated") {
    auto row = make_gpu_destination_row();
    row.compression = RuntimeMavgDirectStorageCompressionFormat::zstd_preview;
    row.sdk_package_version = "1.4.0-preview1-2603.504";
    row.stable_directstorage_13_selected = false;
    row.directstorage_14_preview_selected = true;
    row.zstd_preview_reviewed = true;

    const std::vector<RuntimeMavgDirectStorageGpuDecompressionPolicyRow> rows{row, make_gdeflate_row()};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_decompression_policy({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDecompressionStatus::review_required);
    MK_REQUIRE(result.preview_rows == 1U);
    MK_REQUIRE(result.mavg_directstorage_zstd_preview_selected);
    MK_REQUIRE(!result.mavg_directstorage_zstd_preview_ready);
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::directstorage_preview_selected));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::zstd_preview_not_ready));
}

MK_TEST("runtime rhi mavg directstorage gpu decompression policy blocks native handles and external claims") {
    auto row = make_gdeflate_row();
    row.native_handles_exposed = true;
    row.claims_performance_gain = true;
    row.claims_broad_mavg_backend_ready = true;
    row.claims_nanite_compatibility = true;

    const std::vector<RuntimeMavgDirectStorageGpuDecompressionPolicyRow> rows{make_gpu_destination_row(), row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_decompression_policy({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDecompressionStatus::blocked);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.blocked_rows == 1U);
    MK_REQUIRE(result.mavg_directstorage_native_handles_exposed);
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::native_handle_exposure));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::performance_claim_not_allowed));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::broad_backend_claim_not_allowed));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::nanite_claim_not_allowed));
}

MK_TEST("runtime rhi mavg directstorage gpu decompression policy blocks execution claims without retained artifact") {
    auto row = make_gdeflate_row();
    row.claims_gdeflate_ready = true;
    row.execution_artifact_ready = true;
    row.retained_artifact_id = "gdeflate-execution-artifact";
    row.retained_artifact_sha256 = "not-a-sha256";

    const std::vector<RuntimeMavgDirectStorageGpuDecompressionPolicyRow> rows{make_gpu_destination_row(), row};
    const auto result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_decompression_policy({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDecompressionStatus::blocked);
    MK_REQUIRE(!result.mavg_directstorage_gdeflate_execution_ready);
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::invalid_retained_artifact_hash));

    row.retained_artifact_sha256 = kHash;
    row.retained_artifact_id = "";
    const std::vector<RuntimeMavgDirectStorageGpuDecompressionPolicyRow> missing_id_rows{make_gpu_destination_row(),
                                                                                         row};
    const auto missing_id_result =
        mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_gpu_decompression_policy({.rows = missing_id_rows});

    MK_REQUIRE(missing_id_result.status == RuntimeMavgDirectStorageGpuDecompressionStatus::blocked);
    MK_REQUIRE(has_code(missing_id_result,
                        RuntimeMavgDirectStorageGpuDecompressionDiagnosticCode::execution_claim_without_artifact));
}

int main() {
    return mirakana::test::run_all();
}
