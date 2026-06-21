// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_async_overlap_performance_proof.hpp"

#include <array>

namespace {

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceResult make_ready_overlap() {
    mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceResult overlap;
    overlap.background_loaded_row_count = 2;
    overlap.adopted_page_count = 2;
    overlap.uploaded_page_count = 2;
    overlap.uploaded_cluster_count = 2;
    overlap.uploaded_bytes = 4096;
    overlap.background_load_tick_count = 80;
    overlap.gpu_upload_tick_count = 70;
    overlap.overlap_tick_count = 30;
    overlap.graph_asset = mirakana::AssetId::from_name("mavg/async-overlap-proof");
    overlap.timeline_id = 42;
    overlap.recorded_temporal_overlap_evidence = true;
    overlap.executed_background_worker = true;
    overlap.invoked_gpu_upload = true;
    return overlap;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgAsyncOverlapPerformanceSample
make_sample(std::uint64_t total_ticks, std::uint64_t overlap_ticks = 0) {
    return mirakana::runtime_rhi::RuntimeMavgAsyncOverlapPerformanceSample{
        .workload_id = "mavg.selected.directstorage.upload",
        .measurement_artifact_id = "trace://mavg/async-overlap-proof/reference",
        .completed_row_count = 2,
        .background_load_ticks = 80,
        .gpu_upload_ticks = 70,
        .total_ticks = total_ticks,
        .overlap_ticks = overlap_ticks,
    };
}

} // namespace

MK_TEST("runtime rhi mavg async overlap performance proof promotes selected measured samples only") {
    const auto overlap = make_ready_overlap();
    const std::array serial_samples{make_sample(100), make_sample(110), make_sample(120), make_sample(130),
                                    make_sample(140)};
    const std::array overlapped_samples{make_sample(70, 30), make_sample(80, 32), make_sample(90, 30),
                                        make_sample(100, 34), make_sample(105, 36)};

    const auto result = mirakana::runtime_rhi::prove_runtime_mavg_async_overlap_performance(
        mirakana::runtime_rhi::RuntimeMavgAsyncOverlapPerformanceProofDesc{
            .graph_asset = mirakana::AssetId::from_name("mavg/async-overlap-proof"),
            .overlap_evidence = &overlap,
            .workload_id = "mavg.selected.directstorage.upload",
            .measurement_artifact_id = "trace://mavg/async-overlap-proof/reference",
            .serial_samples = serial_samples,
            .overlapped_samples = overlapped_samples,
            .minimum_sample_count = 5,
            .minimum_speedup_basis_points = 1000,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.recorded_temporal_overlap_evidence);
    MK_REQUIRE(result.claimed_speedup);
    MK_REQUIRE(result.proved_async_overlap_performance);
    MK_REQUIRE(result.serial_p95_tick_count == 140U);
    MK_REQUIRE(result.overlapped_p95_tick_count == 105U);
    MK_REQUIRE(result.speedup_basis_points == 2500U);
    MK_REQUIRE(result.sample_count == 10U);
    MK_REQUIRE(result.overlapped_sample_count == 5U);
    MK_REQUIRE(result.overlap_tick_count == 30U);
    MK_REQUIRE(result.completed_row_count == 2U);
    MK_REQUIRE(!result.touched_native_handles);
    MK_REQUIRE(!result.used_gpu_directstorage_destination);
    MK_REQUIRE(!result.used_gdeflate);
    MK_REQUIRE(!result.executed_mesh_shader);
    MK_REQUIRE(!result.claimed_metal_readiness);
    MK_REQUIRE(!result.claimed_nanite_equivalence);
    MK_REQUIRE(!result.claimed_broad_optimization);
}

MK_TEST("runtime rhi mavg async overlap performance proof fails closed without ready overlap evidence") {
    mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceResult overlap;
    const std::array serial_samples{make_sample(100), make_sample(110), make_sample(120), make_sample(130),
                                    make_sample(140)};
    const std::array overlapped_samples{make_sample(70, 30), make_sample(80, 32), make_sample(90, 30),
                                        make_sample(100, 34), make_sample(105, 36)};

    const auto result = mirakana::runtime_rhi::prove_runtime_mavg_async_overlap_performance(
        mirakana::runtime_rhi::RuntimeMavgAsyncOverlapPerformanceProofDesc{
            .graph_asset = mirakana::AssetId::from_name("mavg/async-overlap-proof"),
            .overlap_evidence = &overlap,
            .workload_id = "mavg.selected.directstorage.upload",
            .measurement_artifact_id = "trace://mavg/async-overlap-proof/reference",
            .serial_samples = serial_samples,
            .overlapped_samples = overlapped_samples,
            .minimum_sample_count = 5,
            .minimum_speedup_basis_points = 1000,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.claimed_speedup);
    MK_REQUIRE(!result.proved_async_overlap_performance);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_performance_proof_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::overlap_evidence_not_ready));
}

MK_TEST("runtime rhi mavg async overlap performance proof rejects invalid sample evidence") {
    const auto overlap = make_ready_overlap();
    auto mismatched = make_sample(80, 30);
    mismatched.workload_id = "mavg.other.workload";
    auto zero_total = make_sample(0, 30);
    const std::array serial_samples{make_sample(100), mismatched, make_sample(120), make_sample(130), make_sample(140)};
    const std::array overlapped_samples{make_sample(70, 30), zero_total, make_sample(90, 30), make_sample(100, 34),
                                        make_sample(105, 36)};

    const auto result = mirakana::runtime_rhi::prove_runtime_mavg_async_overlap_performance(
        mirakana::runtime_rhi::RuntimeMavgAsyncOverlapPerformanceProofDesc{
            .graph_asset = mirakana::AssetId::from_name("mavg/async-overlap-proof"),
            .overlap_evidence = &overlap,
            .workload_id = "mavg.selected.directstorage.upload",
            .measurement_artifact_id = "trace://mavg/async-overlap-proof/reference",
            .serial_samples = serial_samples,
            .overlapped_samples = overlapped_samples,
            .minimum_sample_count = 5,
            .minimum_speedup_basis_points = 1000,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.proved_async_overlap_performance);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_performance_proof_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::sample_workload_mismatch));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_performance_proof_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::invalid_sample_ticks));
}

MK_TEST("runtime rhi mavg async overlap performance proof rejects insufficient speedup") {
    const auto overlap = make_ready_overlap();
    const std::array serial_samples{make_sample(100), make_sample(110), make_sample(120), make_sample(130),
                                    make_sample(140)};
    const std::array overlapped_samples{make_sample(98, 30), make_sample(108, 32), make_sample(118, 30),
                                        make_sample(128, 34), make_sample(138, 36)};

    const auto result = mirakana::runtime_rhi::prove_runtime_mavg_async_overlap_performance(
        mirakana::runtime_rhi::RuntimeMavgAsyncOverlapPerformanceProofDesc{
            .graph_asset = mirakana::AssetId::from_name("mavg/async-overlap-proof"),
            .overlap_evidence = &overlap,
            .workload_id = "mavg.selected.directstorage.upload",
            .measurement_artifact_id = "trace://mavg/async-overlap-proof/reference",
            .serial_samples = serial_samples,
            .overlapped_samples = overlapped_samples,
            .minimum_sample_count = 5,
            .minimum_speedup_basis_points = 1000,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.claimed_speedup);
    MK_REQUIRE(!result.proved_async_overlap_performance);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_async_overlap_performance_proof_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::insufficient_speedup));
}

int main() {
    return mirakana::test::run_all();
}
