// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/mavg_page_streaming.hpp"

#include <algorithm>
#include <cmath>
#include <string>
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
