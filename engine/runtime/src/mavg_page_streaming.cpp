// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/mavg_page_streaming.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

constexpr std::string_view package_index_extension = ".geindex";

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

[[nodiscard]] bool has_page(const MavgClusterGraphDocument& graph, std::uint32_t page_index) noexcept {
    return std::ranges::any_of(
        graph.pages, [page_index](const MavgClusterGraphPage& page) { return page.page_index == page_index; });
}

[[nodiscard]] bool has_resident_page(const MavgLodResidentPageSet& resident_pages, std::uint32_t page_index) noexcept {
    return std::ranges::find(resident_pages.page_indices, page_index) != resident_pages.page_indices.end();
}

[[nodiscard]] bool contains_control_character(std::string_view text) noexcept {
    return std::ranges::any_of(text, [](unsigned char character) { return character < 0x20U; });
}

[[nodiscard]] bool valid_relative_vfs_path(std::string_view path) noexcept {
    if (path.empty() || contains_control_character(path)) {
        return false;
    }
    if (path.front() == '/' || path.front() == '\\') {
        return false;
    }
    if (path.find('\\') != std::string_view::npos) {
        return false;
    }
    if (path.size() >= 2U && path[1] == ':') {
        return false;
    }

    std::size_t segment_begin = 0;
    while (segment_begin <= path.size()) {
        const auto segment_end = path.find('/', segment_begin);
        const auto segment = segment_end == std::string_view::npos
                                 ? path.substr(segment_begin)
                                 : path.substr(segment_begin, segment_end - segment_begin);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }

    return true;
}

[[nodiscard]] bool ends_with_package_index_extension(std::string_view path) noexcept {
    return path.size() > package_index_extension.size() && path.ends_with(package_index_extension);
}

[[nodiscard]] bool valid_candidate(const RuntimePackageIndexDiscoveryCandidateV2& candidate) noexcept {
    return valid_relative_vfs_path(candidate.package_index_path) &&
           ends_with_package_index_extension(candidate.package_index_path) &&
           (candidate.content_root.empty() || valid_relative_vfs_path(candidate.content_root)) &&
           valid_relative_vfs_path(candidate.label);
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
            continue;
        }
        if (candidate.page_index != page_index) {
            continue;
        }
        if (!valid_candidate(candidate.candidate)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_candidate, graph_asset, page_index,
                           "MAVG page streaming candidate must be a relative .geindex package row with valid roots");
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

[[nodiscard]] bool contains_mount_id(const std::vector<RuntimeResidentPackageMountIdV2>& mount_ids,
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

[[nodiscard]] bool matches_page_mount(const RuntimeMavgResidentPageMountRow& page_mount,
                                      const RuntimeMavgPageStreamingRecencyRow& recency_row) noexcept {
    return page_mount.graph_asset == recency_row.graph_asset && page_mount.page_index == recency_row.page_index &&
           page_mount.mount_id == recency_row.mount_id;
}

[[nodiscard]] const RuntimeMavgPageStreamingRecencyRow*
find_recency_row(std::span<const RuntimeMavgPageStreamingRecencyRow> recency_rows,
                 const RuntimeMavgResidentPageMountRow& page_mount) noexcept {
    const auto found = std::ranges::find_if(recency_rows, [&page_mount](const RuntimeMavgPageStreamingRecencyRow& row) {
        return matches_page_mount(page_mount, row);
    });
    if (found == recency_rows.end()) {
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

[[nodiscard]] bool
prepare_eviction_review_protection(RuntimeMavgPageStreamingEvictionReviewResult& result,
                                   const RuntimeResidentPackageMountSetV2& mount_set, AssetId graph_asset,
                                   const MavgClusterGraphDocument* graph_document,
                                   std::span<const RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters,
                                   std::span<const RuntimeMavgResidentPageMountRow> page_mounts,
                                   std::span<const RuntimeResidentPackageMountIdV2> caller_protected_mount_ids) {
    bool invalid_inputs = false;
    if (graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_graph_asset, graph_asset, 0,
                       "MAVG page streaming eviction review graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (graph_document == nullptr) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_graph, graph_asset, 0,
                       "MAVG page streaming eviction review graph document is required");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return false;
    }

    const auto& graph = *graph_document;
    if (graph.asset != graph_asset) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::graph_asset_mismatch, graph_asset, 0,
                       "MAVG page streaming eviction review graph document asset must match the requested graph asset");
    }
    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_graph, graph_asset, 0,
                       "MAVG page streaming eviction review graph validation failed");
    }
    if (!result.diagnostics.empty()) {
        return false;
    }

    std::vector<std::uint32_t> seen_page_indices;
    std::vector<RuntimeResidentPackageMountIdV2> seen_page_mount_ids;
    for (const auto& row : page_mounts) {
        if (row.graph_asset != graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::page_mount_graph_mismatch, row.graph_asset,
                           row.page_index, "MAVG resident page mount graph asset does not match the review graph");
            continue;
        }
        if (!has_page(graph, row.page_index)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_page, graph_asset, row.page_index,
                           "MAVG resident page mount references an unknown graph page");
            continue;
        }
        if (row.mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_page_mount, graph_asset,
                           row.page_index, "MAVG resident page mount id must be non-zero");
            continue;
        }
        if (std::ranges::find(seen_page_indices, row.page_index) != seen_page_indices.end() ||
            contains_mount_id(seen_page_mount_ids, row.mount_id)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_page_mount, graph_asset,
                           row.page_index, "MAVG resident page mount rows must be unique by page and mount id");
            continue;
        }
        if (!contains_mount_id(mount_set, row.mount_id)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_page_mount, graph_asset,
                           row.page_index, "MAVG resident page mount id is not mounted");
            continue;
        }
        seen_page_indices.push_back(row.page_index);
        seen_page_mount_ids.push_back(row.mount_id);
    }

    std::vector<RuntimeResidentPackageMountIdV2> seen_caller_protected_mounts;
    for (const auto mount_id : caller_protected_mount_ids) {
        if (mount_id.value == 0 || !contains_mount_id(mount_set, mount_id)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_protected_mount, graph_asset, 0,
                           "MAVG caller-protected mount ids must be non-zero mounted ids");
            continue;
        }
        if (contains_mount_id(seen_caller_protected_mounts, mount_id)) {
            ++result.duplicate_protected_mount_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_protected_mount, graph_asset, 0,
                           "MAVG caller-protected mount id appears more than once");
            continue;
        }
        seen_caller_protected_mounts.push_back(mount_id);
        add_unique_protected_mount(result, mount_id);
    }
    if (!result.diagnostics.empty()) {
        return false;
    }

    std::vector<std::uint32_t> visible_page_indices;
    std::vector<std::uint32_t> fallback_page_indices;
    const auto contains_page = [](const std::vector<std::uint32_t>& pages, std::uint32_t page_index) noexcept {
        return std::ranges::find(pages, page_index) != pages.end();
    };
    const auto protect_page = [&](std::uint32_t page_index, bool visible) {
        const auto* const page_mount = find_page_mount(page_mounts, page_index);
        if (page_mount == nullptr) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_page_mount, graph_asset, page_index,
                           "MAVG selected cluster page must have a resident page mount");
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

    for (const auto& selected : selected_clusters) {
        if (selected.graph_asset != graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::selected_graph_mismatch,
                           selected.graph_asset, 0,
                           "MAVG selected cluster graph asset does not match the eviction review graph");
            continue;
        }

        const auto* cluster = find_cluster(graph, selected.cluster_index);
        if (cluster == nullptr) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_cluster, graph_asset, 0,
                           "MAVG selected cluster references an unknown graph cluster");
            continue;
        }

        protect_page(cluster->page_index, true);

        const auto* fallback_cluster = find_cluster(graph, cluster->resident_fallback_cluster_index);
        if (fallback_cluster == nullptr) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_cluster, graph_asset, 0,
                           "MAVG selected cluster resident fallback references an unknown graph cluster");
        } else {
            protect_page(fallback_cluster->page_index, false);
        }

        const auto* cursor = cluster;
        for (std::size_t guard = 0; cursor != nullptr && cursor->has_parent && guard < graph.clusters.size(); ++guard) {
            const auto* const parent = find_cluster(graph, cursor->parent_cluster_index);
            if (parent == nullptr) {
                add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_cluster, graph_asset, 0,
                               "MAVG selected cluster parent references an unknown graph cluster");
                break;
            }
            protect_page(parent->page_index, false);
            cursor = parent;
        }
    }
    return result.diagnostics.empty();
}

[[nodiscard]] bool validate_recency_rows(RuntimeMavgPageStreamingEvictionReviewResult& result, AssetId graph_asset,
                                         std::span<const RuntimeMavgResidentPageMountRow> page_mounts,
                                         std::span<const RuntimeMavgPageStreamingRecencyRow> recency_rows) {
    std::vector<std::uint32_t> seen_page_indices;
    std::vector<RuntimeResidentPackageMountIdV2> seen_mount_ids;
    for (const auto& row : recency_rows) {
        if (row.graph_asset != graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::recency_graph_mismatch, row.graph_asset,
                           row.page_index, "MAVG recency row graph asset does not match the review graph");
            continue;
        }
        if (row.mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_recency_row, graph_asset,
                           row.page_index, "MAVG recency row mount id must be non-zero");
            continue;
        }
        if (std::ranges::find(seen_page_indices, row.page_index) != seen_page_indices.end() ||
            contains_mount_id(seen_mount_ids, row.mount_id)) {
            ++result.duplicate_recency_row_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_recency_row, graph_asset,
                           row.page_index, "MAVG recency rows must be unique by page and mount id");
            continue;
        }
        const auto page_mount_matches =
            std::ranges::any_of(page_mounts, [&row](const RuntimeMavgResidentPageMountRow& page_mount) {
                return matches_page_mount(page_mount, row);
            });
        if (!page_mount_matches) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_recency_row, graph_asset,
                           row.page_index, "MAVG recency row must match a resident page mount row");
            continue;
        }
        seen_page_indices.push_back(row.page_index);
        seen_mount_ids.push_back(row.mount_id);
    }
    return result.diagnostics.empty();
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

RuntimeMavgPageStreamingEvictionReviewResult
review_runtime_mavg_page_streaming_evictions(const RuntimeResidentPackageMountSetV2& mount_set,
                                             const RuntimeMavgPageStreamingEvictionReviewDesc& desc) {
    RuntimeMavgPageStreamingEvictionReviewResult result;
    result.eviction_candidate_unmount_order.assign(desc.reviewed_candidate_unmount_order.begin(),
                                                   desc.reviewed_candidate_unmount_order.end());

    if (!prepare_eviction_review_protection(result, mount_set, desc.graph_asset, desc.graph, desc.selected_clusters,
                                            desc.resident_page_mounts, desc.caller_protected_mount_ids)) {
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

RuntimeMavgPageStreamingEvictionReviewResult
plan_runtime_mavg_page_streaming_automatic_evictions(const RuntimeResidentPackageMountSetV2& mount_set,
                                                     const RuntimeMavgPageStreamingAutomaticEvictionPlanDesc& desc) {
    RuntimeMavgPageStreamingEvictionReviewResult result;
    if (!prepare_eviction_review_protection(result, mount_set, desc.graph_asset, desc.graph, desc.selected_clusters,
                                            desc.resident_page_mounts, desc.caller_protected_mount_ids)) {
        return result;
    }

    struct EvictionCandidate {
        RuntimeMavgResidentPageMountRow page_mount;
        std::uint64_t resident_page_last_used_generation{0};
    };

    std::vector<EvictionCandidate> eviction_candidates;
    eviction_candidates.reserve(desc.resident_page_mounts.size());
    for (const auto& row : desc.resident_page_mounts) {
        if (contains_mount_id(result.protected_mount_ids, row.mount_id)) {
            ++result.protected_eviction_candidate_skip_count;
            continue;
        }
        eviction_candidates.push_back(EvictionCandidate{.page_mount = row});
    }

    if (desc.policy_kind == RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency) {
        if (!validate_recency_rows(result, desc.graph_asset, desc.resident_page_mounts, desc.recency_rows)) {
            return result;
        }
        for (auto& candidate : eviction_candidates) {
            const auto* const recency = find_recency_row(desc.recency_rows, candidate.page_mount);
            if (recency == nullptr) {
                ++result.missing_recency_row_count;
                add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_recency_row, desc.graph_asset,
                               candidate.page_mount.page_index,
                               "MAVG recency eviction policy requires one recency row for each candidate");
                continue;
            }
            candidate.resident_page_last_used_generation = recency->resident_page_last_used_generation;
        }
        if (!result.diagnostics.empty()) {
            return result;
        }
        result.applied_caller_supplied_recency_policy = true;
        result.recency_eviction_candidate_count = eviction_candidates.size();
    } else if (desc.policy_kind != RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::deterministic_page_index) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_recency_row, desc.graph_asset, 0,
                       "MAVG automatic eviction policy kind is not supported");
        return result;
    }

    std::ranges::sort(eviction_candidates, [&desc](const EvictionCandidate& lhs, const EvictionCandidate& rhs) {
        if (desc.policy_kind == RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency &&
            lhs.resident_page_last_used_generation != rhs.resident_page_last_used_generation) {
            return lhs.resident_page_last_used_generation < rhs.resident_page_last_used_generation;
        }
        if (lhs.page_mount.page_index != rhs.page_mount.page_index) {
            return lhs.page_mount.page_index > rhs.page_mount.page_index;
        }
        return lhs.page_mount.mount_id.value < rhs.page_mount.mount_id.value;
    });

    result.eviction_candidate_unmount_order.reserve(eviction_candidates.size());
    for (const auto& candidate : eviction_candidates) {
        result.eviction_candidate_unmount_order.push_back(candidate.page_mount.mount_id);
    }
    result.automatic_eviction_candidate_count = result.eviction_candidate_unmount_order.size();
    result.planned_automatic_eviction_policy = true;

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
