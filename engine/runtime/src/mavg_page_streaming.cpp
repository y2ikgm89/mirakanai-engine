// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/mavg_page_streaming.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

void add_diagnostic(RuntimeMavgPageStreamingPlanResult& result, RuntimeMavgPageStreamingDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPageStreamingDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimeMavgPageStreamingDrainResult& result, RuntimeMavgPageStreamingDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPageStreamingDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimeMavgPageStreamingEvictionReviewResult& result, RuntimeMavgPageStreamingDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPageStreamingDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimeMavgPageStreamingDispatchPlan& result, RuntimeMavgPageStreamingDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPageStreamingDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimeMavgPageStreamingWorkerResult& result, RuntimeMavgPageStreamingDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPageStreamingDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool has_page(const MavgClusterGraphDocument& graph, std::uint32_t page_index) noexcept {
    return std::ranges::any_of(
        graph.pages, [page_index](const MavgClusterGraphPage& page) { return page.page_index == page_index; });
}

[[nodiscard]] bool has_resident_page(const MavgLodResidentPageSet& resident_pages, std::uint32_t page_index) noexcept {
    return std::ranges::find(resident_pages.page_indices, page_index) != resident_pages.page_indices.end();
}

[[nodiscard]] bool valid_candidate(const RuntimePackageIndexDiscoveryCandidateV2& candidate) noexcept {
    return !candidate.package_index_path.empty() && !candidate.content_root.empty();
}

[[nodiscard]] bool valid_reason(const std::string& reason) noexcept {
    return !reason.empty() && reason.find('\0') == std::string::npos && reason.find('\n') == std::string::npos &&
           reason.find('\r') == std::string::npos;
}

[[nodiscard]] bool valid_dispatch_mode(RuntimeMavgPageStreamingDispatchMode mode) noexcept {
    return mode == RuntimeMavgPageStreamingDispatchMode::caller_owned_safe_point ||
           mode == RuntimeMavgPageStreamingDispatchMode::caller_owned_background_queue ||
           mode == RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker;
}

[[nodiscard]] bool valid_dispatch_row(const RuntimeMavgPageStreamingPlanRow& row) noexcept {
    return row.graph_asset.value != 0 && std::isfinite(row.priority) && valid_reason(row.reason) &&
           valid_candidate(row.candidate);
}

[[nodiscard]] const RuntimeMavgPageStreamingCandidateRow*
find_candidate_for_page(std::span<const RuntimeMavgPageStreamingCandidateRow> candidates, AssetId graph_asset,
                        std::uint32_t page_index, RuntimeMavgPageStreamingPlanResult& result) {
    const RuntimeMavgPageStreamingCandidateRow* match = nullptr;
    for (const auto& candidate : candidates) {
        if (candidate.graph_asset != graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::candidate_graph_mismatch,
                           candidate.graph_asset, candidate.page_index,
                           "MAVG page streaming candidate graph asset does not match the requested graph asset");
            continue;
        }
        if (candidate.page_index != page_index) {
            continue;
        }
        if (!valid_candidate(candidate.candidate)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_candidate, graph_asset, page_index,
                           "MAVG page streaming candidate must include a package index path and content root");
            continue;
        }
        if (match != nullptr) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_candidate, graph_asset, page_index,
                           "MAVG page streaming candidate pages must be unique");
            continue;
        }
        match = &candidate;
    }
    return match;
}

void sort_plan_rows(std::vector<RuntimeMavgPageStreamingPlanRow>& rows) {
    std::ranges::sort(rows, [](const RuntimeMavgPageStreamingPlanRow& lhs, const RuntimeMavgPageStreamingPlanRow& rhs) {
        if (lhs.priority != rhs.priority) {
            return lhs.priority > rhs.priority;
        }
        return lhs.page_index < rhs.page_index;
    });
}

[[nodiscard]] const MavgClusterGraphCluster* find_cluster(const MavgClusterGraphDocument& graph,
                                                          std::uint32_t cluster_index) noexcept {
    const auto found = std::ranges::find_if(graph.clusters, [cluster_index](const MavgClusterGraphCluster& cluster) {
        return cluster.cluster_index == cluster_index;
    });
    if (found == graph.clusters.end()) {
        return nullptr;
    }
    return &*found;
}

[[nodiscard]] bool contains_mount_id(std::span<const RuntimeResidentPackageMountIdV2> mount_ids,
                                     RuntimeResidentPackageMountIdV2 mount_id) noexcept {
    return std::ranges::find(mount_ids, mount_id) != mount_ids.end();
}

[[nodiscard]] bool contains_mount_id(const RuntimeResidentPackageMountSetV2& mount_set,
                                     RuntimeResidentPackageMountIdV2 mount_id) noexcept {
    return std::ranges::any_of(mount_set.mounts(), [mount_id](const RuntimeResidentPackageMountRecordV2& mount) {
        return mount.id == mount_id;
    });
}

[[nodiscard]] const RuntimeMavgResidentPageMountRow*
find_page_mount(std::span<const RuntimeMavgResidentPageMountRow> page_mounts, std::uint32_t page_index) noexcept {
    const auto found = std::ranges::find_if(
        page_mounts, [page_index](const RuntimeMavgResidentPageMountRow& row) { return row.page_index == page_index; });
    if (found == page_mounts.end()) {
        return nullptr;
    }
    return &*found;
}

void add_unique_protected_mount(RuntimeMavgPageStreamingEvictionReviewResult& result,
                                RuntimeResidentPackageMountIdV2 mount_id) {
    if (contains_mount_id(result.protected_mount_ids, mount_id)) {
        ++result.duplicate_protected_mount_count;
        return;
    }
    result.protected_mount_ids.push_back(mount_id);
}

void copy_eviction_plan_diagnostics(RuntimeMavgPageStreamingEvictionReviewResult& result, AssetId graph_asset) {
    for (const auto& diagnostic : result.eviction_plan.diagnostics) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::eviction_plan_failed, graph_asset, 0,
                       diagnostic.message.empty() ? diagnostic.code : diagnostic.message);
    }
    if (!result.eviction_plan.succeeded() && result.diagnostics.empty()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::eviction_plan_failed, graph_asset, 0,
                       "MAVG page streaming eviction review plan failed");
    }
}

void copy_mount_diagnostics(RuntimeMavgPageStreamingDrainResult& result) {
    for (const auto& diagnostic : result.mount_result.diagnostics) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::safe_point_failed, result.row.graph_asset,
                       result.row.page_index, diagnostic.message.empty() ? diagnostic.code : diagnostic.message);
    }
    if (!result.mount_result.succeeded() && result.diagnostics.empty()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::safe_point_failed, result.row.graph_asset,
                       result.row.page_index, "MAVG page streaming safe point failed");
    }
}

} // namespace

RuntimeMavgPageStreamingPlanResult
plan_runtime_mavg_page_streaming_requests(const RuntimeMavgPageStreamingPlanDesc& desc,
                                          std::span<const MavgLodPageRequest> page_requests,
                                          std::span<const RuntimeMavgPageStreamingCandidateRow> candidates) {
    RuntimeMavgPageStreamingPlanResult result;
    result.input_request_count = page_requests.size();

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_graph_asset, desc.graph_asset, 0,
                       "MAVG page streaming graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_graph, desc.graph_asset, 0,
                       "MAVG page streaming graph document is required");
        invalid_inputs = true;
    }
    if (desc.resident_pages == nullptr) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_resident_pages, desc.graph_asset, 0,
                       "MAVG page streaming resident page set is required");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto& graph = *desc.graph;
    const auto& resident_pages = *desc.resident_pages;
    if (graph.asset != desc.graph_asset) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::graph_asset_mismatch, desc.graph_asset, 0,
                       "MAVG page streaming graph document asset must match the requested graph asset");
    }
    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_graph, desc.graph_asset, 0,
                       "MAVG page streaming graph validation failed");
    }

    std::vector<RuntimeMavgPageStreamingPlanRow> coalesced;
    for (const auto& request : page_requests) {
        if (request.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::request_graph_mismatch, request.graph_asset,
                           request.page_index,
                           "MAVG page request graph asset does not match the streaming graph asset");
            continue;
        }
        if (!has_page(graph, request.page_index)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_page, request.graph_asset,
                           request.page_index, "MAVG page request references an unknown graph page");
            continue;
        }
        if (!std::isfinite(request.priority)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_priority, request.graph_asset,
                           request.page_index, "MAVG page request priority must be finite");
            continue;
        }
        if (!valid_reason(request.reason)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_reason, request.graph_asset,
                           request.page_index, "MAVG page request reason must be non-empty single-line text");
            continue;
        }
        if (has_resident_page(resident_pages, request.page_index)) {
            ++result.resident_skip_count;
            continue;
        }

        const auto existing = std::ranges::find_if(coalesced, [&request](const RuntimeMavgPageStreamingPlanRow& row) {
            return row.page_index == request.page_index;
        });
        if (existing != coalesced.end()) {
            ++result.duplicate_request_count;
            ++existing->duplicate_count;
            if (request.priority > existing->priority) {
                existing->priority = request.priority;
                existing->reason = request.reason;
            }
            continue;
        }

        coalesced.push_back(RuntimeMavgPageStreamingPlanRow{
            .graph_asset = desc.graph_asset,
            .page_index = request.page_index,
            .priority = request.priority,
            .reason = request.reason,
            .candidate = {},
            .duplicate_count = 0,
        });
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    for (auto& row : coalesced) {
        const auto* const candidate = find_candidate_for_page(candidates, desc.graph_asset, row.page_index, result);
        if (candidate == nullptr) {
            ++result.missing_candidate_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_candidate, desc.graph_asset,
                           row.page_index, "MAVG page streaming candidate is missing for requested page");
            continue;
        }
        row.candidate = candidate->candidate;
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    sort_plan_rows(coalesced);
    if (desc.max_queued_pages > 0 && coalesced.size() > desc.max_queued_pages) {
        result.budget_degraded = true;
        result.budget_dropped_request_count = coalesced.size() - desc.max_queued_pages;
        coalesced.resize(desc.max_queued_pages);
    }
    result.queued_page_requests = std::move(coalesced);
    return result;
}

RuntimeMavgPageStreamingDispatchPlan
plan_runtime_mavg_page_streaming_dispatches(const RuntimeMavgPageStreamingDispatchDesc& desc) {
    RuntimeMavgPageStreamingDispatchPlan result;
    result.input_request_count = desc.queued_page_requests.size();
    result.dispatch_mount_id_count = desc.mount_ids.size();
    result.requires_safe_point = desc.require_safe_point;
    result.caller_owned_background_queue =
        desc.mode == RuntimeMavgPageStreamingDispatchMode::caller_owned_background_queue;
    result.engine_owned_background_worker =
        desc.mode == RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker;
    result.invoked_file_io = false;
    result.mutated_mount_set = false;
    result.executed_streaming = false;
    result.executed_background_worker = false;
    result.touched_renderer_or_rhi_handles = false;

    bool invalid_inputs = false;
    if (!valid_dispatch_mode(desc.mode)) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_dispatch_mode, {}, 0,
                       "MAVG page streaming dispatch mode is not supported");
        invalid_inputs = true;
    }
    if (!desc.require_safe_point) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unsafe_dispatch_mode, {}, 0,
                       "MAVG page streaming dispatch requires a caller-owned safe point for residency mutation");
        invalid_inputs = true;
    }
    if (!desc.queued_page_requests.empty() && desc.mount_ids.empty()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_dispatch_mount_ids, {}, 0,
                       "MAVG page streaming dispatch requires caller-assigned mount ids");
        invalid_inputs = true;
    }
    if (desc.mount_ids.size() != desc.queued_page_requests.size()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::dispatch_mount_id_count_mismatch, {}, 0,
                       "MAVG page streaming dispatch mount ids must match queued page request rows");
        invalid_inputs = true;
    }

    for (const auto& row : desc.queued_page_requests) {
        if (!valid_dispatch_row(row)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_dispatch_row, row.graph_asset,
                           row.page_index,
                           "MAVG page streaming dispatch rows must be validated queued page request rows");
            invalid_inputs = true;
        }
    }

    std::vector<RuntimeResidentPackageMountIdV2> seen_mount_ids;
    for (const auto mount_id : desc.mount_ids) {
        if (mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_dispatch_mount_id, {}, 0,
                           "MAVG page streaming dispatch mount ids must be non-zero");
            invalid_inputs = true;
            continue;
        }
        if (contains_mount_id(seen_mount_ids, mount_id)) {
            ++result.duplicate_dispatch_mount_id_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_dispatch_mount_id, {}, 0,
                           "MAVG page streaming dispatch mount ids must be unique");
            invalid_inputs = true;
            continue;
        }
        seen_mount_ids.push_back(mount_id);
    }

    if (invalid_inputs) {
        return result;
    }

    auto dispatch_count = desc.queued_page_requests.size();
    if (desc.max_dispatch_rows > 0 && dispatch_count > desc.max_dispatch_rows) {
        result.budget_degraded = true;
        result.budget_dropped_request_count = dispatch_count - desc.max_dispatch_rows;
        dispatch_count = desc.max_dispatch_rows;
    }

    result.dispatch_rows.reserve(dispatch_count);
    for (std::size_t row_index = 0; row_index < dispatch_count; ++row_index) {
        std::vector<RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order(
            desc.eviction_candidate_unmount_order.begin(), desc.eviction_candidate_unmount_order.end());
        std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids(desc.protected_mount_ids.begin(),
                                                                         desc.protected_mount_ids.end());
        result.dispatch_rows.push_back(RuntimeMavgPageStreamingDispatchRow{
            .drain_desc =
                RuntimeMavgPageStreamingDrainDesc{
                    .row = desc.queued_page_requests[row_index],
                    .mount_id = desc.mount_ids[row_index],
                    .overlay = desc.overlay,
                    .budget = desc.budget,
                    .eviction_candidate_unmount_order = std::move(eviction_candidate_unmount_order),
                    .protected_mount_ids = std::move(protected_mount_ids),
                },
            .mode = desc.mode,
            .dispatch_index = row_index,
            .safe_point_required = desc.require_safe_point,
            .background_worker_owned_by_caller =
                desc.mode == RuntimeMavgPageStreamingDispatchMode::caller_owned_background_queue,
            .background_worker_owned_by_engine =
                desc.mode == RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker,
        });
    }
    return result;
}

RuntimeMavgPageStreamingWorkerResult
execute_runtime_mavg_page_streaming_worker(const RuntimeMavgPageStreamingWorkerDesc& desc,
                                           const RuntimeMavgPageStreamingDispatchPlan& dispatch_plan) {
    RuntimeMavgPageStreamingWorkerResult result;
    result.input_dispatch_row_count = dispatch_plan.dispatch_rows.size();

    bool invalid_inputs = false;
    if (desc.filesystem == nullptr) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_worker_filesystem, {}, 0,
                       "MAVG page streaming worker requires a filesystem");
        invalid_inputs = true;
    }
    if (desc.mount_set == nullptr) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_worker_mount_set, {}, 0,
                       "MAVG page streaming worker requires a resident package mount set");
        invalid_inputs = true;
    }
    if (desc.catalog_cache == nullptr) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_worker_catalog_cache, {}, 0,
                       "MAVG page streaming worker requires a resident catalog cache");
        invalid_inputs = true;
    }
    if (!dispatch_plan.succeeded()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_worker_dispatch_plan, {}, 0,
                       "MAVG page streaming worker requires a successful dispatch plan");
        invalid_inputs = true;
    }
    if (dispatch_plan.dispatch_rows.empty()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::empty_worker_dispatch_plan, {}, 0,
                       "MAVG page streaming worker requires at least one dispatch row");
        invalid_inputs = true;
    }
    if (!dispatch_plan.engine_owned_background_worker) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unsupported_worker_dispatch_mode, {}, 0,
                       "MAVG page streaming worker only executes engine-owned background worker dispatch plans");
        invalid_inputs = true;
    }
    for (const auto& row : dispatch_plan.dispatch_rows) {
        if (row.mode != RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker ||
            !row.background_worker_owned_by_engine || !row.safe_point_required) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unsupported_worker_dispatch_mode,
                           row.drain_desc.row.graph_asset, row.drain_desc.row.page_index,
                           "MAVG page streaming worker dispatch rows must be engine-owned safe-point rows");
            invalid_inputs = true;
        }
    }
    if (invalid_inputs) {
        return result;
    }

    auto worker_rows = dispatch_plan.dispatch_rows;
    auto row_count = worker_rows.size();
    if (desc.max_worker_rows > 0 && row_count > desc.max_worker_rows) {
        result.budget_degraded = true;
        result.budget_dropped_row_count = row_count - desc.max_worker_rows;
        row_count = desc.max_worker_rows;
        worker_rows.resize(row_count);
    }

    try {
        std::thread worker([&result, &worker_rows, row_count, &desc]() {
            result.executed_background_worker = true;
            result.drain_results.reserve(row_count);
            for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
                auto drain = execute_runtime_mavg_page_streaming_request_safe_point(
                    *desc.filesystem, *desc.mount_set, *desc.catalog_cache,
                    std::move(worker_rows[row_index].drain_desc));
                drain.executed_background_worker = true;
                result.invoked_file_io = result.invoked_file_io || drain.invoked_candidate_load;
                result.mutated_mount_set = result.mutated_mount_set || drain.committed;
                result.executed_streaming = result.executed_streaming || drain.committed;
                result.touched_renderer_or_rhi_handles =
                    result.touched_renderer_or_rhi_handles || drain.touched_renderer_or_rhi_handles;
                ++result.executed_row_count;
                if (drain.succeeded() && drain.committed) {
                    ++result.committed_row_count;
                } else {
                    ++result.failed_row_count;
                    add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::worker_row_failed,
                                   drain.row.graph_asset, drain.row.page_index,
                                   drain.diagnostics.empty() ? "MAVG page streaming worker row failed"
                                                             : drain.diagnostics.front().message);
                }
                result.drain_results.push_back(std::move(drain));
            }
        });
        worker.join();
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::worker_start_failed, {}, 0, error.what());
    }

    return result;
}

RuntimeMavgPageStreamingEvictionReviewResult
review_runtime_mavg_page_streaming_evictions(const RuntimeResidentPackageMountSetV2& mount_set,
                                             const RuntimeMavgPageStreamingEvictionReviewDesc& desc) {
    RuntimeMavgPageStreamingEvictionReviewResult result;
    result.eviction_candidate_unmount_order.assign(desc.reviewed_candidate_unmount_order.begin(),
                                                   desc.reviewed_candidate_unmount_order.end());

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_graph_asset, desc.graph_asset, 0,
                       "MAVG page streaming eviction review graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_graph, desc.graph_asset, 0,
                       "MAVG page streaming eviction review graph document is required");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto& graph = *desc.graph;
    if (graph.asset != desc.graph_asset) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::graph_asset_mismatch, desc.graph_asset, 0,
                       "MAVG page streaming eviction review graph document asset must match the requested graph asset");
    }
    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_graph, desc.graph_asset, 0,
                       "MAVG page streaming eviction review graph validation failed");
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    std::vector<std::uint32_t> seen_page_indices;
    std::vector<RuntimeResidentPackageMountIdV2> seen_page_mount_ids;
    for (const auto& row : desc.resident_page_mounts) {
        if (row.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::page_mount_graph_mismatch, row.graph_asset,
                           row.page_index, "MAVG resident page mount graph asset does not match the review graph");
            continue;
        }
        if (!has_page(graph, row.page_index)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_page, desc.graph_asset,
                           row.page_index, "MAVG resident page mount references an unknown graph page");
            continue;
        }
        if (row.mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_page_mount, desc.graph_asset,
                           row.page_index, "MAVG resident page mount id must be non-zero");
            continue;
        }
        if (std::ranges::find(seen_page_indices, row.page_index) != seen_page_indices.end() ||
            contains_mount_id(seen_page_mount_ids, row.mount_id)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_page_mount, desc.graph_asset,
                           row.page_index, "MAVG resident page mount rows must be unique by page and mount id");
            continue;
        }
        if (!contains_mount_id(mount_set, row.mount_id)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_page_mount, desc.graph_asset,
                           row.page_index, "MAVG resident page mount id is not mounted");
            continue;
        }
        seen_page_indices.push_back(row.page_index);
        seen_page_mount_ids.push_back(row.mount_id);
    }

    std::vector<RuntimeResidentPackageMountIdV2> seen_caller_protected_mounts;
    for (const auto mount_id : desc.caller_protected_mount_ids) {
        if (mount_id.value == 0 || !contains_mount_id(mount_set, mount_id)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_protected_mount, desc.graph_asset, 0,
                           "MAVG caller-protected mount ids must be non-zero mounted ids");
            continue;
        }
        if (contains_mount_id(seen_caller_protected_mounts, mount_id)) {
            ++result.duplicate_protected_mount_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_protected_mount, desc.graph_asset,
                           0, "MAVG caller-protected mount id appears more than once");
            continue;
        }
        seen_caller_protected_mounts.push_back(mount_id);
        add_unique_protected_mount(result, mount_id);
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    std::vector<std::uint32_t> visible_page_indices;
    std::vector<std::uint32_t> fallback_page_indices;
    const auto contains_page = [](const std::vector<std::uint32_t>& pages, std::uint32_t page_index) noexcept {
        return std::ranges::find(pages, page_index) != pages.end();
    };
    const auto protect_page = [&](std::uint32_t page_index, bool visible) {
        const auto* const page_mount = find_page_mount(desc.resident_page_mounts, page_index);
        if (page_mount == nullptr) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_page_mount, desc.graph_asset,
                           page_index, "MAVG selected cluster page must have a resident page mount");
            return;
        }
        if (visible) {
            if (contains_page(visible_page_indices, page_index)) {
                return;
            }
            visible_page_indices.push_back(page_index);
            ++result.protected_visible_page_count;
            add_unique_protected_mount(result, page_mount->mount_id);
            return;
        }
        if (contains_page(visible_page_indices, page_index) || contains_page(fallback_page_indices, page_index)) {
            return;
        }
        fallback_page_indices.push_back(page_index);
        ++result.protected_fallback_page_count;
        add_unique_protected_mount(result, page_mount->mount_id);
    };

    for (const auto& selected : desc.selected_clusters) {
        if (selected.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::selected_graph_mismatch,
                           selected.graph_asset, 0,
                           "MAVG selected cluster graph asset does not match the eviction review graph");
            continue;
        }

        const auto* cluster = find_cluster(graph, selected.cluster_index);
        if (cluster == nullptr) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_cluster, desc.graph_asset, 0,
                           "MAVG selected cluster references an unknown graph cluster");
            continue;
        }

        protect_page(cluster->page_index, true);

        const auto* fallback_cluster = find_cluster(graph, cluster->resident_fallback_cluster_index);
        if (fallback_cluster == nullptr) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_cluster, desc.graph_asset, 0,
                           "MAVG selected cluster resident fallback references an unknown graph cluster");
        } else {
            protect_page(fallback_cluster->page_index, false);
        }

        const auto* cursor = cluster;
        for (std::size_t guard = 0; cursor != nullptr && cursor->has_parent && guard < graph.clusters.size(); ++guard) {
            const auto* const parent = find_cluster(graph, cursor->parent_cluster_index);
            if (parent == nullptr) {
                add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_cluster, desc.graph_asset, 0,
                               "MAVG selected cluster parent references an unknown graph cluster");
                break;
            }
            protect_page(parent->page_index, false);
            cursor = parent;
        }
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.invoked_eviction_plan = true;
    result.eviction_plan = plan_runtime_resident_package_evictions_v2(
        mount_set, RuntimeResidentPackageEvictionPlanDescV2{
                       .target_budget = desc.target_budget,
                       .overlay = desc.overlay,
                       .candidate_unmount_order = result.eviction_candidate_unmount_order,
                       .protected_mount_ids = result.protected_mount_ids,
                   });
    copy_eviction_plan_diagnostics(result, desc.graph_asset);
    return result;
}

RuntimeMavgPageStreamingDrainResult execute_runtime_mavg_page_streaming_request_safe_point(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeMavgPageStreamingDrainDesc desc) {
    RuntimeMavgPageStreamingDrainResult result;
    result.row = desc.row;
    result.executed_safe_point = true;
    result.mount_result = commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        RuntimePackageCandidateResidentMountReviewedEvictionsDescV2{
            .candidate = desc.row.candidate,
            .mount_id = desc.mount_id,
            .overlay = desc.overlay,
            .budget = desc.budget,
            .eviction_candidate_unmount_order = std::move(desc.eviction_candidate_unmount_order),
            .protected_mount_ids = std::move(desc.protected_mount_ids),
        });
    result.invoked_candidate_load = result.mount_result.invoked_candidate_load;
    result.invoked_eviction_plan = result.mount_result.invoked_eviction_plan;
    result.invoked_catalog_refresh = result.mount_result.invoked_catalog_refresh;
    result.committed = result.mount_result.committed;
    copy_mount_diagnostics(result);
    return result;
}

} // namespace mirakana::runtime
