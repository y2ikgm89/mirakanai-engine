// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/mavg_page_streaming.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
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

void add_diagnostic(RuntimeMavgResidentPageUseGenerationResult& result, RuntimeMavgPageStreamingDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPageStreamingDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimeMavgResidentPageFrequencyResult& result, RuntimeMavgPageStreamingDiagnosticCode code,
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

[[nodiscard]] bool contains_page_index(const std::vector<std::uint32_t>& page_indices,
                                       std::uint32_t page_index) noexcept {
    return std::ranges::find(page_indices, page_index) != page_indices.end();
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

[[nodiscard]] bool matches_page_mount(const RuntimeMavgResidentPageMountRow& page_mount,
                                      const RuntimeMavgPageStreamingFrequencyRow& frequency_row) noexcept {
    return page_mount.graph_asset == frequency_row.graph_asset && page_mount.page_index == frequency_row.page_index &&
           page_mount.mount_id == frequency_row.mount_id;
}

[[nodiscard]] bool matches_page_mount(const RuntimeMavgResidentPageMountRow& page_mount,
                                      const RuntimeMavgPageStreamingGpuMemoryPressureRow& pressure_row) noexcept {
    return page_mount.graph_asset == pressure_row.graph_asset && page_mount.page_index == pressure_row.page_index &&
           page_mount.mount_id == pressure_row.mount_id;
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

[[nodiscard]] const RuntimeMavgPageStreamingFrequencyRow*
find_frequency_row(std::span<const RuntimeMavgPageStreamingFrequencyRow> frequency_rows,
                   const RuntimeMavgResidentPageMountRow& page_mount) noexcept {
    const auto found =
        std::ranges::find_if(frequency_rows, [&page_mount](const RuntimeMavgPageStreamingFrequencyRow& row) {
            return matches_page_mount(page_mount, row);
        });
    if (found == frequency_rows.end()) {
        return nullptr;
    }
    return &*found;
}

[[nodiscard]] const RuntimeMavgPageStreamingGpuMemoryPressureRow*
find_gpu_memory_pressure_row(std::span<const RuntimeMavgPageStreamingGpuMemoryPressureRow> pressure_rows,
                             const RuntimeMavgResidentPageMountRow& page_mount) noexcept {
    const auto found =
        std::ranges::find_if(pressure_rows, [&page_mount](const RuntimeMavgPageStreamingGpuMemoryPressureRow& row) {
            return matches_page_mount(page_mount, row);
        });
    if (found == pressure_rows.end()) {
        return nullptr;
    }
    return &*found;
}

[[nodiscard]] bool
uses_recency_eviction_order(RuntimeMavgPageStreamingAutomaticEvictionPolicyKind policy_kind) noexcept {
    return policy_kind == RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency ||
           policy_kind == RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru;
}

[[nodiscard]] bool
uses_frequency_eviction_order(RuntimeMavgPageStreamingAutomaticEvictionPolicyKind policy_kind) noexcept {
    return policy_kind == RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency;
}

[[nodiscard]] bool
uses_gpu_memory_pressure_eviction_order(RuntimeMavgPageStreamingAutomaticEvictionPolicyKind policy_kind) noexcept {
    return policy_kind == RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_gpu_memory_pressure;
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

void copy_use_generation_evidence(RuntimeMavgPageStreamingEvictionReviewResult& result,
                                  const RuntimeMavgResidentPageUseGenerationResult& use_generations) {
    result.touched_resident_page_count = use_generations.touched_resident_page_count;
    result.carried_recency_row_count = use_generations.carried_recency_row_count;
    result.new_resident_page_count = use_generations.new_resident_page_count;
    result.dropped_nonresident_recency_row_count = use_generations.dropped_nonresident_recency_row_count;
    result.duplicate_recency_row_count = use_generations.duplicate_recency_row_count;
    result.missing_recency_row_count = use_generations.missing_page_mount_count;
    result.inferred_resident_page_use_generation = use_generations.inferred_resident_page_use_generation;
    result.diagnostics.insert(result.diagnostics.end(), use_generations.diagnostics.begin(),
                              use_generations.diagnostics.end());
}

void copy_frequency_evidence(RuntimeMavgPageStreamingEvictionReviewResult& result,
                             const RuntimeMavgResidentPageFrequencyResult& frequencies) {
    result.touched_resident_page_count = frequencies.touched_resident_page_count;
    result.carried_frequency_row_count = frequencies.carried_frequency_row_count;
    result.new_resident_page_count = frequencies.new_resident_page_count;
    result.dropped_nonresident_frequency_row_count = frequencies.dropped_nonresident_frequency_row_count;
    result.duplicate_frequency_row_count = frequencies.duplicate_frequency_row_count;
    result.missing_frequency_row_count = frequencies.missing_page_mount_count;
    result.inferred_resident_page_frequency = frequencies.inferred_resident_page_frequency;
    result.diagnostics.insert(result.diagnostics.end(), frequencies.diagnostics.begin(), frequencies.diagnostics.end());
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

RuntimeMavgResidentPageUseGenerationResult
infer_runtime_mavg_resident_page_use_generations(const RuntimeResidentPackageMountSetV2& mount_set,
                                                 const RuntimeMavgResidentPageUseGenerationDesc& desc) {
    RuntimeMavgResidentPageUseGenerationResult result;
    result.input_resident_page_mount_count = desc.resident_page_mounts.size();
    result.input_selected_cluster_count = desc.selected_clusters.size();

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_graph_asset, desc.graph_asset, 0,
                       "MAVG resident page use generation graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_graph, desc.graph_asset, 0,
                       "MAVG resident page use generation graph document is required");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto& graph = *desc.graph;
    if (graph.asset != desc.graph_asset) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::graph_asset_mismatch, desc.graph_asset, 0,
                       "MAVG resident page use generation graph document asset must match the requested graph asset");
        invalid_inputs = true;
    }
    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_graph, desc.graph_asset, 0,
                       "MAVG resident page use generation graph validation failed");
        invalid_inputs = true;
    }

    std::vector<std::uint32_t> resident_page_indices;
    std::vector<RuntimeResidentPackageMountIdV2> resident_mount_ids;
    for (const auto& row : desc.resident_page_mounts) {
        if (row.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::page_mount_graph_mismatch, row.graph_asset,
                           row.page_index,
                           "MAVG resident page use generation mount graph asset does not match the requested graph");
            invalid_inputs = true;
            continue;
        }
        if (!has_page(graph, row.page_index)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_page, desc.graph_asset,
                           row.page_index, "MAVG resident page use generation row references an unknown graph page");
            invalid_inputs = true;
            continue;
        }
        if (row.mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_page_mount, desc.graph_asset,
                           row.page_index, "MAVG resident page use generation mount id must be non-zero");
            invalid_inputs = true;
            continue;
        }
        if (contains_page_index(resident_page_indices, row.page_index) ||
            contains_mount_id(resident_mount_ids, row.mount_id)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_page_mount, desc.graph_asset,
                           row.page_index,
                           "MAVG resident page use generation mount rows must be unique by page and mount id");
            invalid_inputs = true;
            continue;
        }
        if (!contains_mount_id(mount_set, row.mount_id)) {
            ++result.missing_page_mount_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_page_mount, desc.graph_asset,
                           row.page_index, "MAVG resident page use generation mount id is not mounted");
            invalid_inputs = true;
            continue;
        }
        resident_page_indices.push_back(row.page_index);
        resident_mount_ids.push_back(row.mount_id);
    }

    std::vector<std::uint32_t> selected_page_indices;
    for (const auto& selected : desc.selected_clusters) {
        if (selected.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::selected_graph_mismatch,
                           selected.graph_asset, 0,
                           "MAVG resident page use generation selected cluster graph asset does not match");
            invalid_inputs = true;
            continue;
        }
        const auto* const cluster = find_cluster(graph, selected.cluster_index);
        if (cluster == nullptr) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_cluster, desc.graph_asset, 0,
                           "MAVG resident page use generation selected cluster references an unknown graph cluster");
            invalid_inputs = true;
            continue;
        }
        if (!contains_page_index(resident_page_indices, cluster->page_index)) {
            ++result.missing_page_mount_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_page_mount, desc.graph_asset,
                           cluster->page_index,
                           "MAVG resident page use generation selected cluster page is not resident");
            invalid_inputs = true;
            continue;
        }
        if (!contains_page_index(selected_page_indices, cluster->page_index)) {
            selected_page_indices.push_back(cluster->page_index);
        }
    }
    if (invalid_inputs) {
        return result;
    }

    std::vector<std::uint32_t> previous_page_indices;
    std::vector<RuntimeResidentPackageMountIdV2> previous_mount_ids;
    bool invalid_recency = false;
    for (const auto& row : desc.previous_recency_rows) {
        if (row.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::recency_graph_mismatch, row.graph_asset,
                           row.page_index,
                           "MAVG resident page use generation previous recency graph asset does not match");
            invalid_recency = true;
            continue;
        }
        if (row.mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_recency_row, desc.graph_asset,
                           row.page_index,
                           "MAVG resident page use generation previous recency mount id must be non-zero");
            invalid_recency = true;
            continue;
        }
        if (contains_page_index(previous_page_indices, row.page_index) ||
            contains_mount_id(previous_mount_ids, row.mount_id)) {
            ++result.duplicate_recency_row_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_recency_row, desc.graph_asset,
                           row.page_index, "MAVG resident page use generation previous recency rows must be unique");
            invalid_recency = true;
            continue;
        }
        previous_page_indices.push_back(row.page_index);
        previous_mount_ids.push_back(row.mount_id);

        const auto* const current_mount = find_page_mount(desc.resident_page_mounts, row.page_index);
        if (current_mount == nullptr || !matches_page_mount(*current_mount, row)) {
            ++result.dropped_nonresident_recency_row_count;
            continue;
        }
        if (row.resident_page_last_used_generation > desc.current_use_generation) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::non_monotonic_use_generation,
                           desc.graph_asset, row.page_index,
                           "MAVG resident page use generation must be monotonic for retained resident pages");
            invalid_recency = true;
        }
    }
    if (invalid_recency) {
        return result;
    }

    std::vector<RuntimeMavgResidentPageMountRow> ordered_mounts(desc.resident_page_mounts.begin(),
                                                                desc.resident_page_mounts.end());
    std::ranges::sort(ordered_mounts,
                      [](const RuntimeMavgResidentPageMountRow& lhs, const RuntimeMavgResidentPageMountRow& rhs) {
                          if (lhs.page_index != rhs.page_index) {
                              return lhs.page_index < rhs.page_index;
                          }
                          return lhs.mount_id.value < rhs.mount_id.value;
                      });

    result.recency_rows.reserve(ordered_mounts.size());
    for (const auto& mount : ordered_mounts) {
        auto last_used_generation = std::uint64_t{0};
        if (contains_page_index(selected_page_indices, mount.page_index)) {
            last_used_generation = desc.current_use_generation;
            ++result.touched_resident_page_count;
        } else if (const auto* const previous = find_recency_row(desc.previous_recency_rows, mount);
                   previous != nullptr) {
            last_used_generation = previous->resident_page_last_used_generation;
            ++result.carried_recency_row_count;
        } else {
            ++result.new_resident_page_count;
        }

        result.recency_rows.push_back(RuntimeMavgPageStreamingRecencyRow{
            .graph_asset = mount.graph_asset,
            .page_index = mount.page_index,
            .mount_id = mount.mount_id,
            .resident_page_last_used_generation = last_used_generation,
        });
    }
    result.output_recency_row_count = result.recency_rows.size();
    result.inferred_resident_page_use_generation = true;
    return result;
}

RuntimeMavgResidentPageFrequencyResult
infer_runtime_mavg_resident_page_frequencies(const RuntimeResidentPackageMountSetV2& mount_set,
                                             const RuntimeMavgResidentPageFrequencyDesc& desc) {
    RuntimeMavgResidentPageFrequencyResult result;
    result.input_resident_page_mount_count = desc.resident_page_mounts.size();
    result.input_selected_cluster_count = desc.selected_clusters.size();

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_graph_asset, desc.graph_asset, 0,
                       "MAVG resident page frequency graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_graph, desc.graph_asset, 0,
                       "MAVG resident page frequency graph document is required");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto& graph = *desc.graph;
    if (graph.asset != desc.graph_asset) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::graph_asset_mismatch, desc.graph_asset, 0,
                       "MAVG resident page frequency graph document asset must match the requested graph asset");
        invalid_inputs = true;
    }
    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_graph, desc.graph_asset, 0,
                       "MAVG resident page frequency graph validation failed");
        invalid_inputs = true;
    }

    std::vector<std::uint32_t> resident_page_indices;
    std::vector<RuntimeResidentPackageMountIdV2> resident_mount_ids;
    for (const auto& row : desc.resident_page_mounts) {
        if (row.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::page_mount_graph_mismatch, row.graph_asset,
                           row.page_index,
                           "MAVG resident page frequency mount graph asset does not match the requested graph");
            invalid_inputs = true;
            continue;
        }
        if (!has_page(graph, row.page_index)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_page, desc.graph_asset,
                           row.page_index, "MAVG resident page frequency row references an unknown graph page");
            invalid_inputs = true;
            continue;
        }
        if (row.mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_page_mount, desc.graph_asset,
                           row.page_index, "MAVG resident page frequency mount id must be non-zero");
            invalid_inputs = true;
            continue;
        }
        if (contains_page_index(resident_page_indices, row.page_index) ||
            contains_mount_id(resident_mount_ids, row.mount_id)) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_page_mount, desc.graph_asset,
                           row.page_index,
                           "MAVG resident page frequency mount rows must be unique by page and mount id");
            invalid_inputs = true;
            continue;
        }
        if (!contains_mount_id(mount_set, row.mount_id)) {
            ++result.missing_page_mount_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_page_mount, desc.graph_asset,
                           row.page_index, "MAVG resident page frequency mount id is not mounted");
            invalid_inputs = true;
            continue;
        }
        resident_page_indices.push_back(row.page_index);
        resident_mount_ids.push_back(row.mount_id);
    }

    std::vector<std::uint32_t> selected_page_indices;
    for (const auto& selected : desc.selected_clusters) {
        if (selected.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::selected_graph_mismatch,
                           selected.graph_asset, 0,
                           "MAVG resident page frequency selected cluster graph asset does not match");
            invalid_inputs = true;
            continue;
        }
        const auto* const cluster = find_cluster(graph, selected.cluster_index);
        if (cluster == nullptr) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::unknown_cluster, desc.graph_asset, 0,
                           "MAVG resident page frequency selected cluster references an unknown graph cluster");
            invalid_inputs = true;
            continue;
        }
        if (!contains_page_index(resident_page_indices, cluster->page_index)) {
            ++result.missing_page_mount_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_page_mount, desc.graph_asset,
                           cluster->page_index, "MAVG resident page frequency selected cluster page is not resident");
            invalid_inputs = true;
            continue;
        }
        if (!contains_page_index(selected_page_indices, cluster->page_index)) {
            selected_page_indices.push_back(cluster->page_index);
        }
    }
    if (invalid_inputs) {
        return result;
    }

    std::vector<std::uint32_t> previous_page_indices;
    std::vector<RuntimeResidentPackageMountIdV2> previous_mount_ids;
    bool invalid_frequency = false;
    for (const auto& row : desc.previous_frequency_rows) {
        if (row.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::frequency_graph_mismatch, row.graph_asset,
                           row.page_index, "MAVG resident page frequency previous row graph asset does not match");
            invalid_frequency = true;
            continue;
        }
        if (row.mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_frequency_row, desc.graph_asset,
                           row.page_index, "MAVG resident page frequency previous row mount id must be non-zero");
            invalid_frequency = true;
            continue;
        }
        if (contains_page_index(previous_page_indices, row.page_index) ||
            contains_mount_id(previous_mount_ids, row.mount_id)) {
            ++result.duplicate_frequency_row_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_frequency_row, desc.graph_asset,
                           row.page_index, "MAVG resident page frequency previous rows must be unique");
            invalid_frequency = true;
            continue;
        }
        previous_page_indices.push_back(row.page_index);
        previous_mount_ids.push_back(row.mount_id);

        const auto* const current_mount = find_page_mount(desc.resident_page_mounts, row.page_index);
        if (current_mount == nullptr || !matches_page_mount(*current_mount, row)) {
            ++result.dropped_nonresident_frequency_row_count;
            continue;
        }
        if (contains_page_index(selected_page_indices, row.page_index) &&
            row.resident_page_selection_count == std::numeric_limits<std::uint64_t>::max()) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::frequency_counter_overflow, desc.graph_asset,
                           row.page_index,
                           "MAVG resident page frequency cannot increment a saturated selection counter");
            invalid_frequency = true;
        }
    }
    if (invalid_frequency) {
        return result;
    }

    std::vector<RuntimeMavgResidentPageMountRow> ordered_mounts(desc.resident_page_mounts.begin(),
                                                                desc.resident_page_mounts.end());
    std::ranges::sort(ordered_mounts,
                      [](const RuntimeMavgResidentPageMountRow& lhs, const RuntimeMavgResidentPageMountRow& rhs) {
                          if (lhs.page_index != rhs.page_index) {
                              return lhs.page_index < rhs.page_index;
                          }
                          return lhs.mount_id.value < rhs.mount_id.value;
                      });

    result.frequency_rows.reserve(ordered_mounts.size());
    for (const auto& mount : ordered_mounts) {
        auto selection_count = std::uint64_t{0};
        const auto* const previous = find_frequency_row(desc.previous_frequency_rows, mount);
        if (contains_page_index(selected_page_indices, mount.page_index)) {
            selection_count = previous == nullptr ? std::uint64_t{1} : previous->resident_page_selection_count + 1;
            ++result.touched_resident_page_count;
            if (previous == nullptr) {
                ++result.new_resident_page_count;
            }
        } else if (previous != nullptr) {
            selection_count = previous->resident_page_selection_count;
            ++result.carried_frequency_row_count;
        } else {
            ++result.new_resident_page_count;
        }

        result.frequency_rows.push_back(RuntimeMavgPageStreamingFrequencyRow{
            .graph_asset = mount.graph_asset,
            .page_index = mount.page_index,
            .mount_id = mount.mount_id,
            .resident_page_selection_count = selection_count,
        });
    }
    result.output_frequency_row_count = result.frequency_rows.size();
    result.inferred_resident_page_frequency = true;
    return result;
}

[[nodiscard]] static bool validate_recency_rows(RuntimeMavgPageStreamingEvictionReviewResult& result,
                                                AssetId graph_asset,
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

[[nodiscard]] static bool add_gpu_memory_pressure_estimated_bytes(RuntimeMavgPageStreamingEvictionReviewResult& result,
                                                                  AssetId graph_asset, std::uint32_t page_index,
                                                                  std::uint64_t bytes, std::uint64_t& total) {
    if (total > std::numeric_limits<std::uint64_t>::max() - bytes) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::gpu_memory_pressure_counter_overflow,
                       graph_asset, page_index, "MAVG GPU memory pressure estimated byte aggregation overflowed");
        return false;
    }
    total += bytes;
    return true;
}

[[nodiscard]] static bool
validate_gpu_memory_pressure_rows(RuntimeMavgPageStreamingEvictionReviewResult& result, AssetId graph_asset,
                                  std::span<const RuntimeMavgResidentPageMountRow> page_mounts,
                                  std::span<const RuntimeMavgPageStreamingGpuMemoryPressureRow> pressure_rows) {
    std::vector<std::uint32_t> seen_page_indices;
    std::vector<RuntimeResidentPackageMountIdV2> seen_mount_ids;
    for (const auto& row : pressure_rows) {
        if (row.graph_asset != graph_asset) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::gpu_memory_pressure_graph_mismatch,
                           row.graph_asset, row.page_index,
                           "MAVG GPU memory pressure row graph asset does not match the review graph");
            continue;
        }
        if (row.mount_id.value == 0) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_gpu_memory_pressure_row, graph_asset,
                           row.page_index, "MAVG GPU memory pressure row mount id must be non-zero");
            continue;
        }
        if (std::ranges::find(seen_page_indices, row.page_index) != seen_page_indices.end() ||
            contains_mount_id(seen_mount_ids, row.mount_id)) {
            ++result.duplicate_gpu_memory_pressure_row_count;
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::duplicate_gpu_memory_pressure_row,
                           graph_asset, row.page_index,
                           "MAVG GPU memory pressure rows must be unique by page and mount id");
            continue;
        }
        const auto page_mount_matches =
            std::ranges::any_of(page_mounts, [&row](const RuntimeMavgResidentPageMountRow& page_mount) {
                return matches_page_mount(page_mount, row);
            });
        if (!page_mount_matches) {
            add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_gpu_memory_pressure_row, graph_asset,
                           row.page_index, "MAVG GPU memory pressure row must match a resident page mount row");
            continue;
        }
        seen_page_indices.push_back(row.page_index);
        seen_mount_ids.push_back(row.mount_id);
    }
    return result.diagnostics.empty();
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
        std::uint64_t resident_page_selection_count{0};
        std::uint64_t gpu_memory_eviction_pressure_score{0};
        std::uint64_t gpu_memory_estimated_resident_bytes{0};
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
    } else if (desc.policy_kind == RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru) {
        auto use_generations = infer_runtime_mavg_resident_page_use_generations(
            mount_set, RuntimeMavgResidentPageUseGenerationDesc{
                           .graph_asset = desc.graph_asset,
                           .graph = desc.graph,
                           .selected_clusters = desc.selected_clusters,
                           .resident_page_mounts = desc.resident_page_mounts,
                           .previous_recency_rows = desc.previous_recency_rows,
                           .current_use_generation = desc.current_use_generation,
                       });
        copy_use_generation_evidence(result, use_generations);
        if (!use_generations.succeeded()) {
            return result;
        }
        for (auto& candidate : eviction_candidates) {
            const auto* const recency = find_recency_row(use_generations.recency_rows, candidate.page_mount);
            if (recency == nullptr) {
                ++result.missing_recency_row_count;
                add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_recency_row, desc.graph_asset,
                               candidate.page_mount.page_index,
                               "MAVG runtime-inferred LRU policy requires inferred recency for each candidate");
                continue;
            }
            candidate.resident_page_last_used_generation = recency->resident_page_last_used_generation;
        }
        if (!result.diagnostics.empty()) {
            return result;
        }
        result.inferred_eviction_policy = true;
        result.inferred_lru_eviction_policy = true;
        result.recency_eviction_candidate_count = eviction_candidates.size();
        result.runtime_inferred_lru_eviction_candidate_count = eviction_candidates.size();
    } else if (desc.policy_kind == RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency) {
        auto frequencies = infer_runtime_mavg_resident_page_frequencies(
            mount_set, RuntimeMavgResidentPageFrequencyDesc{
                           .graph_asset = desc.graph_asset,
                           .graph = desc.graph,
                           .selected_clusters = desc.selected_clusters,
                           .resident_page_mounts = desc.resident_page_mounts,
                           .previous_frequency_rows = desc.previous_frequency_rows,
                       });
        copy_frequency_evidence(result, frequencies);
        if (!frequencies.succeeded()) {
            return result;
        }
        for (auto& candidate : eviction_candidates) {
            const auto* const frequency = find_frequency_row(frequencies.frequency_rows, candidate.page_mount);
            if (frequency == nullptr) {
                ++result.missing_frequency_row_count;
                add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_frequency_row, desc.graph_asset,
                               candidate.page_mount.page_index,
                               "MAVG runtime-inferred frequency policy requires inferred frequency for each candidate");
                continue;
            }
            candidate.resident_page_selection_count = frequency->resident_page_selection_count;
        }
        if (!result.diagnostics.empty()) {
            return result;
        }
        result.inferred_eviction_policy = true;
        result.inferred_frequency_eviction_policy = true;
        result.runtime_inferred_frequency_eviction_candidate_count = eviction_candidates.size();
    } else if (desc.policy_kind ==
               RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_gpu_memory_pressure) {
        if (!validate_gpu_memory_pressure_rows(result, desc.graph_asset, desc.resident_page_mounts,
                                               desc.gpu_memory_pressure_rows)) {
            return result;
        }
        for (const auto& row : desc.gpu_memory_pressure_rows) {
            if (contains_mount_id(result.protected_mount_ids, row.mount_id) &&
                !add_gpu_memory_pressure_estimated_bytes(result, desc.graph_asset, row.page_index,
                                                         row.estimated_gpu_resident_bytes,
                                                         result.gpu_memory_pressure_protected_estimated_bytes)) {
                return result;
            }
        }
        for (auto& candidate : eviction_candidates) {
            const auto* const pressure =
                find_gpu_memory_pressure_row(desc.gpu_memory_pressure_rows, candidate.page_mount);
            if (pressure == nullptr) {
                ++result.missing_gpu_memory_pressure_row_count;
                add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::missing_gpu_memory_pressure_row,
                               desc.graph_asset, candidate.page_mount.page_index,
                               "MAVG GPU memory pressure policy requires one row for each candidate");
                continue;
            }
            candidate.gpu_memory_eviction_pressure_score = pressure->eviction_pressure_score;
            candidate.gpu_memory_estimated_resident_bytes = pressure->estimated_gpu_resident_bytes;
            if (!add_gpu_memory_pressure_estimated_bytes(result, desc.graph_asset, candidate.page_mount.page_index,
                                                         pressure->estimated_gpu_resident_bytes,
                                                         result.gpu_memory_pressure_candidate_estimated_bytes)) {
                continue;
            }
        }
        if (!result.diagnostics.empty()) {
            return result;
        }
        result.applied_caller_supplied_gpu_memory_pressure_policy = true;
        result.planned_gpu_memory_pressure_eviction_policy = true;
        result.gpu_memory_pressure_eviction_candidate_count = eviction_candidates.size();
    } else if (desc.policy_kind != RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::deterministic_page_index) {
        add_diagnostic(result, RuntimeMavgPageStreamingDiagnosticCode::invalid_recency_row, desc.graph_asset, 0,
                       "MAVG automatic eviction policy kind is not supported");
        return result;
    }

    std::ranges::sort(eviction_candidates, [&desc](const EvictionCandidate& lhs, const EvictionCandidate& rhs) {
        if (uses_gpu_memory_pressure_eviction_order(desc.policy_kind) &&
            lhs.gpu_memory_eviction_pressure_score != rhs.gpu_memory_eviction_pressure_score) {
            return lhs.gpu_memory_eviction_pressure_score > rhs.gpu_memory_eviction_pressure_score;
        }
        if (uses_gpu_memory_pressure_eviction_order(desc.policy_kind) &&
            lhs.gpu_memory_estimated_resident_bytes != rhs.gpu_memory_estimated_resident_bytes) {
            return lhs.gpu_memory_estimated_resident_bytes > rhs.gpu_memory_estimated_resident_bytes;
        }
        if (uses_frequency_eviction_order(desc.policy_kind) &&
            lhs.resident_page_selection_count != rhs.resident_page_selection_count) {
            return lhs.resident_page_selection_count < rhs.resident_page_selection_count;
        }
        if (uses_recency_eviction_order(desc.policy_kind) &&
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
