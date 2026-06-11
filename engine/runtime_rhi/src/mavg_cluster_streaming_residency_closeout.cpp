// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_cluster_streaming_residency_closeout.hpp"

#include <algorithm>

namespace mirakana::runtime_rhi {

namespace {

void add_diagnostic(RuntimeMavgClusterStreamingResidencyCloseoutResult& result,
                    RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode code, AssetId graph_asset,
                    std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgClusterStreamingResidencyCloseoutDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

[[nodiscard]] std::vector<std::uint32_t>
queued_page_indices(std::span<const runtime::RuntimeMavgPageStreamingPlanRow> rows) {
    std::vector<std::uint32_t> indices;
    indices.reserve(rows.size());
    for (const auto& row : rows) {
        indices.push_back(row.page_index);
    }
    return indices;
}

void publish_no_claim_flags(RuntimeMavgClusterStreamingResidencyCloseoutResult& result) {
    result.invoked_file_io = result.payload_page_load.invoked_file_io || result.background_load.invoked_file_io;
    result.executed_background_worker = result.background_load.executed_background_worker;
    result.mutated_mount_set = result.page_streaming_plan.mutated_mount_set ||
                               result.payload_page_load.mutated_mount_set || result.background_load.mutated_mount_set ||
                               result.gpu_memory_residency.mutated_mount_set;
    result.invoked_catalog_refresh =
        result.background_load.invoked_catalog_refresh || result.gpu_memory_residency.invoked_catalog_refresh;
    result.invoked_direct_storage = result.background_load.invoked_direct_storage ||
                                    result.gpu_memory_residency.invoked_direct_storage ||
                                    result.payload_page_load.executed_direct_storage;
    result.touched_renderer_or_rhi_handles = result.page_streaming_plan.touched_renderer_or_rhi_handles ||
                                             result.payload_page_load.touched_renderer_or_rhi_handles ||
                                             result.background_load.touched_renderer_or_rhi_handles ||
                                             result.gpu_memory_residency.touched_renderer_or_rhi_handles;
    result.proved_async_overlap_performance = result.background_load.proved_async_overlap_performance ||
                                              result.gpu_memory_residency.proved_async_overlap_performance;
}

void publish_residency_counters(RuntimeMavgClusterStreamingResidencyCloseoutResult& result) {
    result.protected_visible_page_count = result.gpu_memory_residency.eviction_review.protected_visible_page_count;
    result.protected_fallback_page_count = result.gpu_memory_residency.eviction_review.protected_fallback_page_count;
    result.deterministic_degradation_row_count =
        std::max(result.deterministic_degradation_row_count,
                 result.gpu_memory_residency.eviction_review.eviction_plan.steps.size());
    result.applied_gpu_memory_pressure_policy = result.gpu_memory_residency.applied_gpu_memory_pressure_policy;
    result.preserved_visible_geometry_without_holes =
        result.applied_gpu_memory_pressure_policy && result.protected_visible_page_count > 0U &&
        result.protected_fallback_page_count > 0U && result.diagnostics.empty();
}

} // namespace

RuntimeMavgClusterStreamingResidencyCloseoutResult
plan_runtime_mavg_cluster_streaming_residency_closeout(const runtime::RuntimeResidentPackageMountSetV2& mount_set,
                                                       const RuntimeMavgClusterStreamingResidencyCloseoutDesc& desc) {
    RuntimeMavgClusterStreamingResidencyCloseoutResult result;

    if (desc.graph_asset.value == 0U) {
        add_diagnostic(result, RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::invalid_graph_asset,
                       desc.graph_asset, 0,
                       "MAVG cluster streaming residency closeout requires a non-zero graph asset");
        return result;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::missing_graph,
                       desc.graph_asset, 0, "MAVG cluster streaming residency closeout requires a graph document");
        return result;
    }
    if (desc.filesystem == nullptr) {
        add_diagnostic(result, RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::missing_filesystem,
                       desc.graph_asset, 0, "MAVG cluster streaming residency closeout requires a filesystem");
        return result;
    }
    if (desc.execution_pool == nullptr) {
        add_diagnostic(result, RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::missing_execution_pool,
                       desc.graph_asset, 0, "MAVG cluster streaming residency closeout requires a job execution pool");
        return result;
    }

    result.page_streaming_plan = runtime::plan_runtime_mavg_page_streaming_requests(
        runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = desc.graph_asset,
            .graph = desc.graph,
            .resident_pages = desc.resident_pages,
            .max_queued_pages = desc.max_queued_pages,
        },
        desc.page_requests, desc.candidates);
    result.queued_page_request_count = result.page_streaming_plan.queued_page_requests.size();
    result.deterministic_degradation_row_count = result.page_streaming_plan.budget_dropped_request_count;
    publish_no_claim_flags(result);
    if (!result.page_streaming_plan.succeeded()) {
        add_diagnostic(result, RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::page_streaming_plan_failed,
                       desc.graph_asset, 0, "MAVG page streaming request planning failed");
        return result;
    }

    const auto page_indices = queued_page_indices(result.page_streaming_plan.queued_page_requests);
    const auto payload_path =
        desc.payload_path.empty() ? std::string_view(desc.graph->cluster_payload_uri) : desc.payload_path;
    result.payload_page_load =
        runtime::load_runtime_mavg_payload_pages_from_filesystem(runtime::RuntimeMavgPayloadFilesystemPageLoadDesc{
            .graph_asset = desc.graph_asset,
            .graph = desc.graph,
            .filesystem = desc.filesystem,
            .payload_path = payload_path,
            .page_indices = page_indices,
        });
    result.payload_loaded_page_count = result.payload_page_load.loaded_page_count;
    result.deterministic_degradation_row_count =
        std::max(result.deterministic_degradation_row_count,
                 result.queued_page_request_count - result.payload_loaded_page_count);
    publish_no_claim_flags(result);
    if (!result.payload_page_load.succeeded()) {
        add_diagnostic(result, RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::payload_page_load_failed,
                       desc.graph_asset, 0, "MAVG payload byte-range page load failed");
        return result;
    }

    result.background_load = runtime::dispatch_runtime_mavg_page_streaming_background_loads(
        *desc.filesystem, *desc.execution_pool,
        runtime::RuntimeMavgPageStreamingBackgroundLoadDesc{
            .rows = result.page_streaming_plan.queued_page_requests,
            .frame_index = desc.frame_index,
            .scratch_bytes_per_task = desc.scratch_bytes_per_task,
        });
    result.background_loaded_row_count = result.background_load.loaded_row_count;
    result.deterministic_degradation_row_count =
        std::max(result.deterministic_degradation_row_count,
                 result.queued_page_request_count - result.background_loaded_row_count);
    publish_no_claim_flags(result);
    if (!result.background_load.succeeded()) {
        add_diagnostic(result, RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::background_load_failed,
                       desc.graph_asset, 0, "MAVG background package candidate load failed");
        return result;
    }

    result.gpu_memory_residency = plan_runtime_mavg_gpu_memory_pressure_residency(
        mount_set, RuntimeMavgGpuMemoryResidencyDesc{
                       .graph_asset = desc.graph_asset,
                       .graph = desc.graph,
                       .gpu_memory_policy = desc.gpu_memory_policy,
                       .selected_clusters = desc.selected_clusters,
                       .resident_page_mounts = desc.resident_page_mounts,
                       .recency_rows = desc.recency_rows,
                       .caller_protected_mount_ids = desc.caller_protected_mount_ids,
                       .overlay = desc.overlay,
                       .policy_kind = desc.eviction_policy_kind,
                   });
    publish_residency_counters(result);
    publish_no_claim_flags(result);
    if (!result.gpu_memory_residency.succeeded()) {
        add_diagnostic(result, RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode::gpu_memory_residency_failed,
                       desc.graph_asset, 0, "MAVG GPU memory pressure residency planning failed");
        result.preserved_visible_geometry_without_holes = false;
    }

    return result;
}

bool has_runtime_mavg_cluster_streaming_residency_closeout_diagnostic(
    const RuntimeMavgClusterStreamingResidencyCloseoutResult& result,
    RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana::runtime_rhi
