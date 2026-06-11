// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_streaming_upload_overlap_evidence.hpp"

#include <string>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord
make_record(mirakana::AssetId asset, mirakana::runtime::RuntimeAssetHandle handle, std::string content) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = mirakana::AssetKind::mesh,
        .path = "runtime/mavg/overlap/" + std::to_string(handle.value) + ".geasset",
        .content_hash = asset.value + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = std::move(content),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_package(mirakana::runtime::RuntimeAssetRecord record) {
    return mirakana::runtime::RuntimeAssetPackage({std::move(record)});
}

[[nodiscard]] mirakana::runtime::RuntimePackageCandidateLoadResultV2 make_loaded_candidate(std::uint32_t page_index) {
    const auto page_asset = mirakana::AssetId::from_name("mavg/overlap-page-" + std::to_string(page_index));
    auto loaded = mirakana::runtime::RuntimePackageCandidateLoadResultV2{
        .status = mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded,
        .candidate =
            mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                .package_index_path = "runtime/mavg/overlap/page" + std::to_string(page_index) + ".geindex",
                .content_root = "runtime/mavg/overlap",
                .label = "mavg-overlap-page-" + std::to_string(page_index),
            },
        .loaded_package =
            mirakana::runtime::RuntimeAssetPackageLoadResult{
                .package = make_package(make_record(
                    page_asset, mirakana::runtime::RuntimeAssetHandle{.value = page_index + 1U}, "overlap-page")),
                .failures = {},
            },
        .loaded_record_count = 1,
        .estimated_resident_bytes = 8,
        .invoked_load = true,
    };
    loaded.package_desc.index_path = loaded.candidate.package_index_path;
    loaded.package_desc.content_root = loaded.candidate.content_root;
    return loaded;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutResult
make_successful_closeout(mirakana::AssetId graph_asset, std::size_t loaded_rows) {
    mirakana::runtime_rhi::RuntimeMavgClusterStreamingResidencyCloseoutResult closeout;
    closeout.background_load.execution.status = mirakana::JobExecutionRunStatus::ready;
    closeout.background_load.loaded_row_count = loaded_rows;
    closeout.background_load.dispatched_row_count = loaded_rows;
    closeout.background_load.executed_background_worker = true;
    closeout.background_load.executed_streaming = true;
    for (std::size_t index = 0; index < loaded_rows; ++index) {
        closeout.background_load.loaded_rows.push_back(mirakana::runtime::RuntimeMavgPageStreamingBackgroundLoadedRow{
            .row =
                mirakana::runtime::RuntimeMavgPageStreamingPlanRow{
                    .graph_asset = graph_asset,
                    .page_index = static_cast<std::uint32_t>(index),
                },
            .candidate_load = make_loaded_candidate(static_cast<std::uint32_t>(index)),
            .worker_id = static_cast<std::uint32_t>(index),
            .invoked_candidate_load = true,
        });
    }
    closeout.gpu_memory_residency.eviction_review.eviction_plan.status =
        mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required;
    closeout.gpu_memory_residency.applied_gpu_memory_pressure_policy = true;
    closeout.background_loaded_row_count = loaded_rows;
    closeout.executed_background_worker = true;
    return closeout;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionResult
make_successful_adoption(mirakana::AssetId graph_asset, std::size_t adopted_pages) {
    mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionResult adoption;
    for (std::size_t index = 0; index < adopted_pages; ++index) {
        adoption.adopted_rows.push_back(mirakana::runtime_rhi::RuntimeMavgClusterStreamingSafePointAdoptionRow{
            .graph_asset = graph_asset,
            .page_index = static_cast<std::uint32_t>(index),
        });
    }
    adoption.committed = true;
    adoption.adopted_page_count = adopted_pages;
    adoption.mounted_package_count = adopted_pages;
    return adoption;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadResult
make_successful_upload(mirakana::AssetId graph_asset, std::size_t uploaded_pages, std::uint64_t uploaded_bytes) {
    mirakana::runtime_rhi::RuntimeMavgStreamedClusterGpuUploadResult upload;
    for (std::size_t index = 0; index < uploaded_pages; ++index) {
        upload.page_bindings.push_back(mirakana::runtime_rhi::RuntimeMavgStreamedClusterPageBindingRow{
            .graph_asset = graph_asset,
            .page_index = static_cast<std::uint32_t>(index),
            .uploaded_cluster_count = 1,
            .uploaded_bytes = uploaded_bytes / uploaded_pages,
        });
    }
    upload.package_visible = true;
    upload.streamed_cluster_pages_ready = true;
    upload.invoked_gpu_upload = true;
    upload.uploaded_page_count = uploaded_pages;
    upload.uploaded_cluster_count = uploaded_pages;
    upload.uploaded_bytes = uploaded_bytes;
    return upload;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgStreamingUploadEvidenceWindow
make_window(std::uint64_t begin_tick, std::uint64_t end_tick, std::size_t completed_row_count) {
    return mirakana::runtime_rhi::RuntimeMavgStreamingUploadEvidenceWindow{
        .clock_domain = mirakana::runtime_rhi::RuntimeMavgStreamingUploadEvidenceClockDomain::caller_monotonic_tick,
        .timeline_id = 42,
        .begin_tick = begin_tick,
        .end_tick = end_tick,
        .completed_row_count = completed_row_count,
    };
}

} // namespace

MK_TEST("runtime rhi mavg streaming upload overlap evidence records caller supplied overlap without broad claims") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streaming-upload-overlap");
    const auto closeout = make_successful_closeout(graph_asset, 2);
    const auto adoption = make_successful_adoption(graph_asset, 2);
    const auto upload = make_successful_upload(graph_asset, 2, 4096);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_streaming_upload_overlap_evidence(
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .safe_point_adoption = &adoption,
            .gpu_upload = &upload,
            .background_load_window = make_window(100, 180, 2),
            .gpu_upload_window = make_window(150, 220, 2),
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.recorded_temporal_overlap_evidence);
    MK_REQUIRE(result.background_load_tick_count == 80U);
    MK_REQUIRE(result.gpu_upload_tick_count == 70U);
    MK_REQUIRE(result.overlap_tick_count == 30U);
    MK_REQUIRE(result.background_loaded_row_count == 2U);
    MK_REQUIRE(result.uploaded_page_count == 2U);
    MK_REQUIRE(result.uploaded_bytes == 4096U);
    MK_REQUIRE(result.executed_background_worker);
    MK_REQUIRE(result.invoked_gpu_upload);
    MK_REQUIRE(!result.invoked_persistent_streaming_service);
    MK_REQUIRE(!result.invoked_direct_storage);
    MK_REQUIRE(!result.executed_backend);
    MK_REQUIRE(!result.executed_mesh_shader);
    MK_REQUIRE(!result.touched_native_handles);
    MK_REQUIRE(!result.claimed_speedup);
    MK_REQUIRE(!result.proved_async_overlap_performance);
}

MK_TEST("runtime rhi mavg streaming upload overlap evidence rejects missing input rows") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streaming-upload-overlap");

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_streaming_upload_overlap_evidence(
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDesc{
            .graph_asset = graph_asset,
            .closeout = nullptr,
            .safe_point_adoption = nullptr,
            .gpu_upload = nullptr,
            .background_load_window = make_window(10, 20, 1),
            .gpu_upload_window = make_window(15, 25, 1),
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.recorded_temporal_overlap_evidence);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::missing_closeout));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::missing_safe_point_adoption));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::missing_gpu_upload));
    MK_REQUIRE(!result.claimed_speedup);
    MK_REQUIRE(!result.proved_async_overlap_performance);
}

MK_TEST("runtime rhi mavg streaming upload overlap evidence rejects non overlapping windows") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streaming-upload-overlap");
    const auto closeout = make_successful_closeout(graph_asset, 1);
    const auto adoption = make_successful_adoption(graph_asset, 1);
    const auto upload = make_successful_upload(graph_asset, 1, 2048);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_streaming_upload_overlap_evidence(
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .safe_point_adoption = &adoption,
            .gpu_upload = &upload,
            .background_load_window = make_window(10, 20, 1),
            .gpu_upload_window = make_window(20, 30, 1),
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.overlap_tick_count == 0U);
    MK_REQUIRE(!result.recorded_temporal_overlap_evidence);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::no_temporal_overlap));
    MK_REQUIRE(!result.executed_backend);
    MK_REQUIRE(!result.invoked_direct_storage);
    MK_REQUIRE(!result.proved_async_overlap_performance);
}

MK_TEST("runtime rhi mavg streaming upload overlap evidence rejects invalid windows and missing row counters") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streaming-upload-overlap");
    const auto closeout = make_successful_closeout(graph_asset, 1);
    const auto adoption = make_successful_adoption(graph_asset, 1);
    const auto upload = make_successful_upload(graph_asset, 1, 1024);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_streaming_upload_overlap_evidence(
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .safe_point_adoption = &adoption,
            .gpu_upload = &upload,
            .background_load_window =
                mirakana::runtime_rhi::RuntimeMavgStreamingUploadEvidenceWindow{
                    .clock_domain =
                        mirakana::runtime_rhi::RuntimeMavgStreamingUploadEvidenceClockDomain::caller_monotonic_tick,
                    .timeline_id = 42,
                    .begin_tick = 30,
                    .end_tick = 30,
                    .completed_row_count = 0,
                },
            .gpu_upload_window =
                mirakana::runtime_rhi::RuntimeMavgStreamingUploadEvidenceWindow{
                    .clock_domain =
                        mirakana::runtime_rhi::RuntimeMavgStreamingUploadEvidenceClockDomain::caller_monotonic_tick,
                    .timeline_id = 42,
                    .begin_tick = 20,
                    .end_tick = 10,
                    .completed_row_count = 0,
                },
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::invalid_background_window));
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::invalid_gpu_upload_window));
    MK_REQUIRE(!result.recorded_temporal_overlap_evidence);
}

MK_TEST("runtime rhi mavg streaming upload overlap evidence refuses speedup proof without measured budget rows") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streaming-upload-overlap");
    const auto closeout = make_successful_closeout(graph_asset, 1);
    const auto adoption = make_successful_adoption(graph_asset, 1);
    const auto upload = make_successful_upload(graph_asset, 1, 512);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_streaming_upload_overlap_evidence(
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .safe_point_adoption = &adoption,
            .gpu_upload = &upload,
            .background_load_window = make_window(100, 200, 1),
            .gpu_upload_window = make_window(150, 190, 1),
            .require_measured_budget_evidence = true,
            .measured_budget_evidence_ready = false,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.claimed_speedup);
    MK_REQUIRE(!result.proved_async_overlap_performance);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::
                    missing_measured_budget_evidence));
}

MK_TEST("runtime rhi mavg streaming upload overlap evidence rejects failed closeout even with background rows") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streaming-upload-overlap");
    auto closeout = make_successful_closeout(graph_asset, 1);
    closeout.gpu_memory_residency.applied_gpu_memory_pressure_policy = false;
    const auto adoption = make_successful_adoption(graph_asset, 1);
    const auto upload = make_successful_upload(graph_asset, 1, 512);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_streaming_upload_overlap_evidence(
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .safe_point_adoption = &adoption,
            .gpu_upload = &upload,
            .background_load_window = make_window(100, 200, 1),
            .gpu_upload_window = make_window(150, 190, 1),
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.recorded_temporal_overlap_evidence);
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::closeout_failed));
}

MK_TEST("runtime rhi mavg streaming upload overlap evidence rejects source row graph mismatch") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streaming-upload-overlap");
    const auto other_graph_asset = mirakana::AssetId::from_name("mavg/other-graph");
    auto closeout = make_successful_closeout(graph_asset, 1);
    closeout.background_load.loaded_rows[0].row.graph_asset = other_graph_asset;
    const auto adoption = make_successful_adoption(graph_asset, 1);
    const auto upload = make_successful_upload(graph_asset, 1, 512);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_streaming_upload_overlap_evidence(
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .safe_point_adoption = &adoption,
            .gpu_upload = &upload,
            .background_load_window = make_window(100, 200, 1),
            .gpu_upload_window = make_window(150, 190, 1),
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result, mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::
                    source_row_graph_asset_mismatch));
}

MK_TEST("runtime rhi mavg streaming upload overlap evidence rejects mismatched window clock domains") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streaming-upload-overlap");
    const auto closeout = make_successful_closeout(graph_asset, 1);
    const auto adoption = make_successful_adoption(graph_asset, 1);
    const auto upload = make_successful_upload(graph_asset, 1, 512);

    auto gpu_window = make_window(150, 190, 1);
    gpu_window.timeline_id = 7;

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_streaming_upload_overlap_evidence(
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .safe_point_adoption = &adoption,
            .gpu_upload = &upload,
            .background_load_window = make_window(100, 200, 1),
            .gpu_upload_window = gpu_window,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::window_clock_domain_mismatch));
}

MK_TEST("runtime rhi mavg streaming upload overlap evidence rejects missing window row counters") {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/streaming-upload-overlap");
    const auto closeout = make_successful_closeout(graph_asset, 1);
    const auto adoption = make_successful_adoption(graph_asset, 1);
    const auto upload = make_successful_upload(graph_asset, 1, 512);

    const auto result = mirakana::runtime_rhi::plan_runtime_mavg_streaming_upload_overlap_evidence(
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDesc{
            .graph_asset = graph_asset,
            .closeout = &closeout,
            .safe_point_adoption = &adoption,
            .gpu_upload = &upload,
            .background_load_window = make_window(100, 200, 0),
            .gpu_upload_window = make_window(150, 190, 0),
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(mirakana::runtime_rhi::has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
        result,
        mirakana::runtime_rhi::RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::missing_window_row_evidence));
}

int main() {
    return mirakana::test::run_all();
}
