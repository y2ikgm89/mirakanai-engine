// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_autonomous_streaming_scheduler.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgAutonomousStreamingSchedulerResult& result,
                    RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode code, AssetId graph_asset,
                    std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgAutonomousStreamingSchedulerDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_page_index(std::span<const std::uint32_t> page_indices, std::uint32_t page_index) noexcept {
    return std::ranges::find(page_indices, page_index) != page_indices.end();
}

[[nodiscard]] bool has_pending_page(std::span<const runtime::RuntimeMavgPageStreamingPlanRow> rows, AssetId graph_asset,
                                    std::uint32_t page_index) noexcept {
    return std::ranges::any_of(rows, [graph_asset, page_index](const auto& row) {
        return row.graph_asset == graph_asset && row.page_index == page_index;
    });
}

void sort_plan_rows(std::vector<runtime::RuntimeMavgPageStreamingPlanRow>& rows) {
    std::ranges::sort(rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.priority != rhs.priority) {
            return lhs.priority > rhs.priority;
        }
        return lhs.page_index < rhs.page_index;
    });
}

[[nodiscard]] std::vector<runtime::RuntimeMavgPageStreamingSelectedClusterRow>
selected_cluster_rows(const MavgLodSelectionResult& selection) {
    std::vector<runtime::RuntimeMavgPageStreamingSelectedClusterRow> rows;
    rows.reserve(selection.selected_clusters.size());
    for (const auto& selected : selection.selected_clusters) {
        rows.push_back(runtime::RuntimeMavgPageStreamingSelectedClusterRow{
            .graph_asset = selected.graph_asset,
            .cluster_index = selected.cluster_index,
        });
    }
    return rows;
}

[[nodiscard]] std::vector<MavgLodPageRequest>
filter_page_requests(const RuntimeMavgAutonomousStreamingSchedulerDesc& desc,
                     const std::vector<MavgLodPageRequest>& page_requests,
                     RuntimeMavgAutonomousStreamingSchedulerResult& result) {
    const auto page_heat = [&desc](std::uint32_t page_index) noexcept {
        auto heat = 0.0F;
        for (const auto& row : desc.page_heat) {
            if (row.graph_asset == desc.graph_asset && row.page_index == page_index && std::isfinite(row.heat) &&
                row.heat > heat) {
                heat = row.heat;
            }
        }
        return heat;
    };

    std::vector<MavgLodPageRequest> filtered;
    filtered.reserve(page_requests.size());
    for (const auto& request : page_requests) {
        if (contains_page_index(desc.cancelled_page_indices, request.page_index)) {
            ++result.cancelled_page_count;
            continue;
        }
        if (desc.max_candidate_pages_per_frame > 0U && filtered.size() >= desc.max_candidate_pages_per_frame) {
            continue;
        }
        auto adjusted_request = request;
        adjusted_request.priority += page_heat(request.page_index);
        filtered.push_back(std::move(adjusted_request));
    }
    return filtered;
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

void publish_no_claim_flags(RuntimeMavgAutonomousStreamingSchedulerResult& result) {
    result.selected_page_request_count = result.generated_page_requests.size();
    result.queued_page_count = result.page_streaming_plan.queued_page_requests.size();
    result.dispatched_page_count = result.background_service.dispatched_row_count;
    result.adopted_page_count = result.safe_point_adoption.adopted_page_count;
    result.eviction_candidate_count = result.gpu_memory_residency.eviction_candidate_count;
    result.pending_page_count_before = result.background_service.pending_row_count_before;
    result.pending_page_count_after = result.background_service.pending_row_count_after;
    result.duplicate_pending_page_count = result.background_service.duplicate_pending_row_count;
    result.payload_loaded_page_count = result.payload_page_load.loaded_page_count;
    result.executed_filesystem_payload_io = result.payload_page_load.invoked_file_io;
    result.executed_direct_storage_payload_io = result.payload_page_load.executed_direct_storage;
    result.invoked_file_io = result.executed_filesystem_payload_io || result.background_service.invoked_file_io;
    result.executed_background_worker = result.background_service.executed_background_worker;
    result.applied_gpu_memory_pressure_policy = result.gpu_memory_residency.applied_gpu_memory_pressure_policy;
    result.mutated_mount_set = result.safe_point_adoption.mutated_mount_set;
    result.invoked_catalog_refresh = result.safe_point_adoption.invoked_catalog_refresh;
    result.touched_renderer_or_rhi_handles =
        result.lod_selection.succeeded() && (result.page_streaming_plan.touched_renderer_or_rhi_handles ||
                                             result.payload_page_load.touched_renderer_or_rhi_handles ||
                                             result.background_service.touched_renderer_or_rhi_handles ||
                                             result.gpu_memory_residency.touched_renderer_or_rhi_handles ||
                                             result.safe_point_adoption.touched_renderer_or_rhi_handles);
    result.exposed_native_handles = false;
    result.proved_async_overlap_performance = false;
    result.invoked_gpu_upload = false;
    result.executed_backend = false;
}

void tick_pending_without_dispatch(runtime::RuntimeMavgPageStreamingBackgroundServiceState& state,
                                   RuntimeMavgAutonomousStreamingSchedulerResult& result, AssetId graph_asset,
                                   std::span<const runtime::RuntimeMavgPageStreamingPlanRow> reviewed_rows,
                                   std::size_t max_pending_pages) {
    result.background_service.input_row_count = reviewed_rows.size();
    result.background_service.pending_row_count_before = state.pending_rows.size();

    auto next_pending = state.pending_rows;
    for (const auto& row : reviewed_rows) {
        if (has_pending_page(next_pending, row.graph_asset, row.page_index)) {
            ++result.background_service.duplicate_pending_row_count;
            for (auto& pending : next_pending) {
                if (pending.graph_asset == row.graph_asset && pending.page_index == row.page_index &&
                    row.priority > pending.priority) {
                    pending.priority = row.priority;
                    pending.reason = row.reason;
                    pending.candidate = row.candidate;
                    break;
                }
            }
            continue;
        }
        if (max_pending_pages > 0U && next_pending.size() >= max_pending_pages) {
            result.background_service.budget_degraded = true;
            ++result.background_service.budget_dropped_request_count;
            continue;
        }
        next_pending.push_back(row);
        ++result.background_service.accepted_row_count;
    }

    sort_plan_rows(next_pending);
    state.graph_asset = graph_asset;
    state.pending_rows = std::move(next_pending);
    result.background_service.pending_row_count_after = state.pending_rows.size();
}

[[nodiscard]] RuntimeMavgClusterStreamingResidencyCloseoutResult
make_closeout(const RuntimeMavgAutonomousStreamingSchedulerResult& result) {
    auto closeout = RuntimeMavgClusterStreamingResidencyCloseoutResult{};
    closeout.page_streaming_plan = result.page_streaming_plan;
    closeout.payload_page_load = result.payload_page_load;
    closeout.background_load = result.background_service.dispatch;
    closeout.background_load.loaded_rows = result.background_service.loaded_rows;
    closeout.background_load.loaded_row_count = result.background_service.loaded_row_count;
    closeout.background_load.dispatched_row_count = result.background_service.dispatched_row_count;
    closeout.background_load.failed_row_count = result.background_service.failed_row_count;
    closeout.background_load.invoked_file_io = result.background_service.invoked_file_io;
    closeout.background_load.invoked_candidate_load = result.background_service.invoked_candidate_load;
    closeout.background_load.executed_streaming = result.background_service.executed_streaming;
    closeout.background_load.executed_background_worker = result.background_service.executed_background_worker;
    closeout.background_load.committed = result.background_service.committed;
    closeout.gpu_memory_residency = result.gpu_memory_residency;
    closeout.queued_page_request_count = result.queued_page_count;
    closeout.payload_loaded_page_count = result.payload_loaded_page_count;
    closeout.background_loaded_row_count = result.background_service.loaded_row_count;
    closeout.protected_visible_page_count = result.gpu_memory_residency.eviction_review.protected_visible_page_count;
    closeout.protected_fallback_page_count = result.gpu_memory_residency.eviction_review.protected_fallback_page_count;
    closeout.deterministic_degradation_row_count =
        std::max(result.page_streaming_plan.budget_dropped_request_count,
                 result.queued_page_count - result.background_service.loaded_row_count);
    closeout.applied_gpu_memory_pressure_policy = result.applied_gpu_memory_pressure_policy;
    closeout.preserved_visible_geometry_without_holes = result.applied_gpu_memory_pressure_policy &&
                                                        closeout.protected_visible_page_count > 0U &&
                                                        closeout.protected_fallback_page_count > 0U;
    closeout.invoked_file_io = result.invoked_file_io;
    closeout.executed_background_worker = result.executed_background_worker;
    closeout.mutated_mount_set = false;
    closeout.invoked_catalog_refresh = false;
    closeout.invoked_direct_storage = result.executed_direct_storage_payload_io;
    closeout.invoked_gpu_upload = false;
    closeout.executed_backend = false;
    closeout.touched_renderer_or_rhi_handles = false;
    closeout.proved_async_overlap_performance = false;
    return closeout;
}

[[nodiscard]] std::vector<RuntimeMavgClusterStreamingSafePointAdoptionMountRow>
make_mount_rows(const RuntimeMavgAutonomousStreamingSchedulerDesc& desc,
                const runtime::RuntimeMavgPageStreamingBackgroundServiceTickResult& background_service,
                RuntimeMavgAutonomousStreamingSchedulerState& state) {
    const auto adopt_count = desc.max_adopt_pages > 0U
                                 ? std::min(desc.max_adopt_pages, background_service.loaded_rows.size())
                                 : background_service.loaded_rows.size();
    std::vector<RuntimeMavgClusterStreamingSafePointAdoptionMountRow> rows;
    rows.reserve(adopt_count);
    auto next_mount_id = state.next_mount_id;
    for (std::size_t index = 0; index < adopt_count; ++index) {
        const auto& loaded = background_service.loaded_rows[index];
        rows.push_back(RuntimeMavgClusterStreamingSafePointAdoptionMountRow{
            .graph_asset = desc.graph_asset,
            .page_index = loaded.row.page_index,
            .mount_id = runtime::RuntimeResidentPackageMountIdV2{.value = next_mount_id},
        });
        if (next_mount_id != 0U) {
            ++next_mount_id;
        }
    }
    return rows;
}

void publish_ready(RuntimeMavgAutonomousStreamingSchedulerResult& result,
                   RuntimeMavgAutonomousStreamingSafePointPolicy safe_point_policy) {
    result.mavg_autonomous_streaming_scheduler_ready =
        result.succeeded() && safe_point_policy == RuntimeMavgAutonomousStreamingSafePointPolicy::adopt_when_loaded &&
        result.selected_without_caller_precomputed_requests && result.selected_page_request_count > 0U &&
        result.queued_page_count > 0U && result.payload_loaded_page_count > 0U && result.dispatched_page_count > 0U &&
        result.adopted_page_count > 0U && result.applied_gpu_memory_pressure_policy && result.mutated_mount_set &&
        result.invoked_catalog_refresh && !result.touched_renderer_or_rhi_handles && !result.exposed_native_handles &&
        !result.proved_async_overlap_performance;
}

} // namespace

RuntimeMavgAutonomousStreamingSchedulerResult tick_runtime_mavg_autonomous_streaming_scheduler(
    runtime::RuntimeResidentPackageMountSetV2& mount_set, runtime::RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeMavgAutonomousStreamingSchedulerState& state, const RuntimeMavgAutonomousStreamingSchedulerDesc& desc) {
    RuntimeMavgAutonomousStreamingSchedulerResult result;
    result.selected_without_caller_precomputed_requests = true;
    result.used_filesystem_backend_row =
        desc.io_backend == RuntimeMavgAutonomousStreamingIoBackendKind::filesystem_byte_range;
    result.used_direct_storage_backend_row =
        desc.io_backend == RuntimeMavgAutonomousStreamingIoBackendKind::direct_storage_byte_range;
    result.bounded_per_frame_work = desc.max_candidate_pages_per_frame > 0U || desc.max_queued_pages > 0U ||
                                    desc.max_pending_pages > 0U || desc.max_dispatch_pages > 0U ||
                                    desc.max_adopt_pages > 0U;

    if (desc.graph_asset.value == 0U) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::invalid_graph_asset,
                       desc.graph_asset, 0, "MAVG autonomous scheduler requires a non-zero graph asset");
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::missing_graph, desc.graph_asset,
                       0, "MAVG autonomous scheduler requires a graph document");
    }
    if (desc.resident_pages == nullptr) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::missing_resident_pages,
                       desc.graph_asset, 0, "MAVG autonomous scheduler requires resident page state");
    }
    if (desc.filesystem == nullptr) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::missing_filesystem,
                       desc.graph_asset, 0, "MAVG autonomous scheduler requires a filesystem");
    }
    if (desc.execution_pool == nullptr) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::missing_execution_pool,
                       desc.graph_asset, 0, "MAVG autonomous scheduler requires a job execution pool");
    }
    if (desc.gpu_memory_policy == nullptr) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::missing_gpu_memory_policy,
                       desc.graph_asset, 0, "MAVG autonomous scheduler requires a GPU memory policy plan");
    }
    if (desc.io_backend == RuntimeMavgAutonomousStreamingIoBackendKind::direct_storage_byte_range &&
        desc.direct_storage == nullptr) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::missing_direct_storage_executor,
                       desc.graph_asset, 0, "MAVG autonomous scheduler DirectStorage backend requires an executor");
    }
    if (state.graph_asset.value == 0U) {
        state.graph_asset = desc.graph_asset;
    }
    if (state.graph_asset != desc.graph_asset) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::state_graph_mismatch,
                       state.graph_asset, 0, "MAVG autonomous scheduler state graph must match the tick graph");
    }
    if (!result.succeeded()) {
        publish_no_claim_flags(result);
        return result;
    }

    if (state.background_service.graph_asset.value == 0U) {
        state.background_service.graph_asset = desc.graph_asset;
    }
    result.lod_selection = select_mavg_lod_clusters(*desc.graph, desc.view.lod_view, *desc.resident_pages);
    result.generated_page_requests = result.lod_selection.page_requests;
    publish_no_claim_flags(result);
    if (!result.lod_selection.succeeded()) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::lod_selection_failed,
                       desc.graph_asset, 0, "MAVG autonomous scheduler LOD selection failed");
        publish_no_claim_flags(result);
        return result;
    }

    const auto selected_clusters = selected_cluster_rows(result.lod_selection);
    result.resident_page_use_generations = runtime::infer_runtime_mavg_resident_page_use_generations(
        mount_set, runtime::RuntimeMavgResidentPageUseGenerationDesc{
                       .graph_asset = desc.graph_asset,
                       .graph = desc.graph,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = desc.resident_page_mounts,
                       .previous_recency_rows = state.recency_rows,
                       .current_use_generation = desc.frame_index,
                   });
    if (!result.resident_page_use_generations.succeeded()) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::recency_inference_failed,
                       desc.graph_asset, 0, "MAVG autonomous scheduler resident page recency inference failed");
        publish_no_claim_flags(result);
        return result;
    }
    state.recency_rows = result.resident_page_use_generations.recency_rows;

    const auto filtered_requests = filter_page_requests(desc, result.generated_page_requests, result);
    result.page_streaming_plan = runtime::plan_runtime_mavg_page_streaming_requests(
        runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = desc.graph_asset,
            .graph = desc.graph,
            .resident_pages = desc.resident_pages,
            .max_queued_pages = desc.max_queued_pages,
        },
        filtered_requests, desc.candidates);
    publish_no_claim_flags(result);
    if (!result.page_streaming_plan.succeeded()) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::page_streaming_plan_failed,
                       desc.graph_asset, 0, "MAVG autonomous scheduler page streaming planning failed");
        publish_no_claim_flags(result);
        return result;
    }
    if (result.page_streaming_plan.queued_page_requests.empty()) {
        state.last_frame_index = desc.frame_index;
        publish_ready(result, desc.safe_point_policy);
        return result;
    }

    const auto page_indices = queued_page_indices(result.page_streaming_plan.queued_page_requests);
    const auto payload_path =
        desc.payload_path.empty() ? std::string_view(desc.graph->cluster_payload_uri) : desc.payload_path;
    if (desc.io_backend == RuntimeMavgAutonomousStreamingIoBackendKind::direct_storage_byte_range) {
        result.payload_page_load = runtime::load_runtime_mavg_payload_pages_from_direct_storage(
            runtime::RuntimeMavgPayloadDirectStoragePageLoadDesc{
                .graph_asset = desc.graph_asset,
                .graph = desc.graph,
                .direct_storage = desc.direct_storage,
                .payload_path = payload_path,
                .page_indices = page_indices,
            });
    } else {
        result.payload_page_load =
            runtime::load_runtime_mavg_payload_pages_from_filesystem(runtime::RuntimeMavgPayloadFilesystemPageLoadDesc{
                .graph_asset = desc.graph_asset,
                .graph = desc.graph,
                .filesystem = desc.filesystem,
                .payload_path = payload_path,
                .page_indices = page_indices,
            });
    }
    publish_no_claim_flags(result);
    if (!result.payload_page_load.succeeded()) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::payload_page_load_failed,
                       desc.graph_asset, 0, "MAVG autonomous scheduler payload page load failed");
        publish_no_claim_flags(result);
        return result;
    }

    if (desc.max_dispatch_pages == 0U &&
        desc.safe_point_policy == RuntimeMavgAutonomousStreamingSafePointPolicy::defer) {
        tick_pending_without_dispatch(state.background_service, result, desc.graph_asset,
                                      result.page_streaming_plan.queued_page_requests, desc.max_pending_pages);
    } else {
        result.background_service = runtime::tick_runtime_mavg_page_streaming_background_service(
            *desc.filesystem, *desc.execution_pool, state.background_service,
            runtime::RuntimeMavgPageStreamingBackgroundServiceTickDesc{
                .graph_asset = desc.graph_asset,
                .reviewed_rows = result.page_streaming_plan.queued_page_requests,
                .max_pending_pages = desc.max_pending_pages,
                .max_dispatch_pages = desc.max_dispatch_pages,
                .frame_index = desc.frame_index,
                .scratch_bytes_per_task = desc.scratch_bytes_per_task,
            });
    }
    publish_no_claim_flags(result);
    if (!result.background_service.succeeded()) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::background_service_failed,
                       desc.graph_asset, 0, "MAVG autonomous scheduler background streaming service failed");
        publish_no_claim_flags(result);
        return result;
    }

    result.gpu_memory_residency = plan_runtime_mavg_gpu_memory_pressure_residency(
        mount_set,
        RuntimeMavgGpuMemoryResidencyDesc{
            .graph_asset = desc.graph_asset,
            .graph = desc.graph,
            .gpu_memory_policy = desc.gpu_memory_policy,
            .selected_clusters = selected_clusters,
            .resident_page_mounts = desc.resident_page_mounts,
            .recency_rows = state.recency_rows,
            .caller_protected_mount_ids = desc.caller_protected_mount_ids,
            .overlay = desc.overlay,
            .policy_kind = runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency,
        });
    publish_no_claim_flags(result);
    if (!result.gpu_memory_residency.succeeded()) {
        add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::gpu_memory_residency_failed,
                       desc.graph_asset, 0, "MAVG autonomous scheduler GPU memory residency planning failed");
        publish_no_claim_flags(result);
        return result;
    }

    if (desc.safe_point_policy == RuntimeMavgAutonomousStreamingSafePointPolicy::adopt_when_loaded &&
        !result.background_service.loaded_rows.empty()) {
        auto closeout = make_closeout(result);
        auto mount_rows = make_mount_rows(desc, result.background_service, state);
        result.safe_point_adoption =
            execute_runtime_mavg_cluster_streaming_safe_point_adoption(mount_set, catalog_cache,
                                                                       RuntimeMavgClusterStreamingSafePointAdoptionDesc{
                                                                           .graph_asset = desc.graph_asset,
                                                                           .closeout = &closeout,
                                                                           .mount_rows = mount_rows,
                                                                           .overlay = desc.overlay,
                                                                       });
        if (!result.safe_point_adoption.succeeded()) {
            add_diagnostic(result, RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode::safe_point_adoption_failed,
                           desc.graph_asset, 0, "MAVG autonomous scheduler safe-point adoption failed");
            publish_no_claim_flags(result);
            return result;
        }
        state.next_mount_id += static_cast<std::uint32_t>(result.safe_point_adoption.adopted_page_count);
    }

    state.last_frame_index = desc.frame_index;
    publish_no_claim_flags(result);
    publish_ready(result, desc.safe_point_policy);
    return result;
}

bool has_runtime_mavg_autonomous_streaming_scheduler_diagnostic(
    const RuntimeMavgAutonomousStreamingSchedulerResult& result,
    RuntimeMavgAutonomousStreamingSchedulerDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana::runtime_rhi
