// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_residency.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgPageResidencyActionResult& result, RuntimeMavgPageResidencyActionDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t page_index, runtime::RuntimeResidentPackageMountIdV2 mount_id,
                    std::string message) {
    result.diagnostics.push_back(RuntimeMavgPageResidencyActionDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .mount_id = mount_id,
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_page_index(const std::vector<std::uint32_t>& page_indices,
                                       std::uint32_t page_index) noexcept {
    return std::ranges::find(page_indices, page_index) != page_indices.end();
}

[[nodiscard]] bool contains_mount_id(const std::vector<runtime::RuntimeResidentPackageMountIdV2>& mount_ids,
                                     runtime::RuntimeResidentPackageMountIdV2 mount_id) noexcept {
    return std::ranges::find(mount_ids, mount_id) != mount_ids.end();
}

[[nodiscard]] bool graph_contains_page(const MavgClusterGraphDocument& graph, std::uint32_t page_index) noexcept {
    return std::ranges::any_of(
        graph.pages, [page_index](const MavgClusterGraphPage& page) { return page.page_index == page_index; });
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

[[nodiscard]] bool resource_ref_shape_valid(const rhi::RhiResidencyResourceRef& resource) noexcept {
    switch (resource.kind) {
    case rhi::RhiResidencyResourceKind::buffer:
        return resource.buffer.value != 0 && resource.texture.value == 0;
    case rhi::RhiResidencyResourceKind::texture:
        return resource.texture.value != 0 && resource.buffer.value == 0;
    }
    return false;
}

[[nodiscard]] const RuntimeMavgResidentPageResourceRow*
find_resource_by_page(std::span<const RuntimeMavgResidentPageResourceRow> resources,
                      std::uint32_t page_index) noexcept {
    const auto found = std::ranges::find_if(resources, [page_index](const RuntimeMavgResidentPageResourceRow& row) {
        return row.page_index == page_index;
    });
    if (found == resources.end()) {
        return nullptr;
    }
    return &*found;
}

[[nodiscard]] const RuntimeMavgResidentPageResourceRow*
find_resource_by_mount_id(std::span<const RuntimeMavgResidentPageResourceRow> resources,
                          runtime::RuntimeResidentPackageMountIdV2 mount_id) noexcept {
    const auto found = std::ranges::find_if(
        resources, [mount_id](const RuntimeMavgResidentPageResourceRow& row) { return row.mount_id == mount_id; });
    if (found == resources.end()) {
        return nullptr;
    }
    return &*found;
}

void append_unique_resource(std::vector<rhi::RhiResidencyResourceRef>& resources,
                            const rhi::RhiResidencyResourceRef& resource) {
    const auto already_present = std::ranges::any_of(resources, [&resource](const rhi::RhiResidencyResourceRef& row) {
        return row.kind == resource.kind && row.buffer.value == resource.buffer.value &&
               row.texture.value == resource.texture.value;
    });
    if (!already_present) {
        resources.push_back(resource);
    }
}

void append_action_row(RuntimeMavgPageResidencyActionResult& result, const RuntimeMavgResidentPageResourceRow& row,
                       rhi::RhiResidencyActionKind action, bool selected_page, bool protected_page,
                       bool reviewed_eviction_candidate, bool skipped_protected_eviction_candidate) {
    result.action_rows.push_back(RuntimeMavgPageResidencyActionRow{
        .graph_asset = row.graph_asset,
        .page_index = row.page_index,
        .mount_id = row.mount_id,
        .action = action,
        .resource = row.resource,
        .selected_page = selected_page,
        .protected_page = protected_page,
        .reviewed_eviction_candidate = reviewed_eviction_candidate,
        .skipped_protected_eviction_candidate = skipped_protected_eviction_candidate,
    });
}

void merge_rhi_evidence(RuntimeMavgPageResidencyActionResult& result, const rhi::RhiResidencyActionResult& action) {
    result.invoked_native_make_resident = result.invoked_native_make_resident || action.invoked_native_make_resident;
    result.invoked_native_evict = result.invoked_native_evict || action.invoked_native_evict;
    result.exposed_native_handles = result.exposed_native_handles || action.exposed_native_handles;
    result.enforced_allocator_budget = result.enforced_allocator_budget || action.enforced_allocator_budget;
}

} // namespace

RuntimeMavgPageResidencyActionResult
execute_runtime_mavg_page_residency_actions(rhi::IRhiDevice& device, const RuntimeMavgPageResidencyActionDesc& desc) {
    RuntimeMavgPageResidencyActionResult result;
    result.input_resident_page_resource_count = desc.resident_page_resources.size();

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::invalid_graph_asset, desc.graph_asset, 0,
                       {}, "MAVG page residency graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::missing_graph, desc.graph_asset, 0, {},
                       "MAVG page residency requires a graph document");
        invalid_inputs = true;
    } else {
        const auto validation = validate_mavg_cluster_graph(*desc.graph);
        if (desc.graph->asset != desc.graph_asset || !validation.valid()) {
            add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::invalid_graph, desc.graph->asset, 0,
                           {}, "MAVG page residency graph must match the requested asset and validate successfully");
            invalid_inputs = true;
        }
    }

    std::vector<std::uint32_t> resident_page_indices;
    std::vector<runtime::RuntimeResidentPackageMountIdV2> resident_mount_ids;
    if (desc.graph != nullptr) {
        for (const auto& row : desc.resident_page_resources) {
            if (row.graph_asset != desc.graph_asset || row.mount_id.value == 0 ||
                row.estimated_gpu_resident_bytes == 0 || !graph_contains_page(*desc.graph, row.page_index) ||
                !resource_ref_shape_valid(row.resource)) {
                add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::invalid_resident_page_resource,
                               row.graph_asset, row.page_index, row.mount_id,
                               "MAVG page residency resource rows must match the graph, reference a known page, use a "
                               "non-zero mount id, carry positive estimated bytes, and contain one valid RHI resource");
                invalid_inputs = true;
                continue;
            }
            if (contains_page_index(resident_page_indices, row.page_index) ||
                contains_mount_id(resident_mount_ids, row.mount_id)) {
                add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::duplicate_resident_page_resource,
                               row.graph_asset, row.page_index, row.mount_id,
                               "MAVG page residency resource rows must be unique by page index and mount id");
                invalid_inputs = true;
                continue;
            }
            resident_page_indices.push_back(row.page_index);
            resident_mount_ids.push_back(row.mount_id);
        }
    }

    std::vector<std::uint32_t> selected_pages;
    if (desc.graph != nullptr) {
        for (const auto& selected : desc.selected_clusters) {
            const auto* const cluster = find_cluster(*desc.graph, selected.cluster_index);
            if (selected.graph_asset != desc.graph_asset || cluster == nullptr) {
                add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::invalid_selected_cluster,
                               selected.graph_asset, 0, {},
                               "MAVG page residency selected cluster rows must match the graph and reference a known "
                               "cluster");
                invalid_inputs = true;
                continue;
            }
            if (!contains_page_index(selected_pages, cluster->page_index)) {
                selected_pages.push_back(cluster->page_index);
            }
        }
    }

    std::vector<runtime::RuntimeResidentPackageMountIdV2> protected_mounts;
    for (const auto mount_id : desc.protected_mount_ids) {
        if (mount_id.value == 0 || find_resource_by_mount_id(desc.resident_page_resources, mount_id) == nullptr) {
            add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::invalid_protected_mount,
                           desc.graph_asset, 0, mount_id,
                           "MAVG page residency protected mount ids must be non-zero and reference resident resources");
            invalid_inputs = true;
            continue;
        }
        if (contains_mount_id(protected_mounts, mount_id)) {
            add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::duplicate_protected_mount,
                           desc.graph_asset, 0, mount_id, "MAVG page residency protected mount ids must be unique");
            invalid_inputs = true;
            continue;
        }
        protected_mounts.push_back(mount_id);
    }

    std::vector<runtime::RuntimeResidentPackageMountIdV2> eviction_mounts;
    for (const auto mount_id : desc.eviction_candidate_unmount_order) {
        if (mount_id.value == 0 || find_resource_by_mount_id(desc.resident_page_resources, mount_id) == nullptr) {
            add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::invalid_eviction_candidate,
                           desc.graph_asset, 0, mount_id,
                           "MAVG page residency eviction candidates must be non-zero and reference resident resources");
            invalid_inputs = true;
            continue;
        }
        if (contains_mount_id(eviction_mounts, mount_id)) {
            add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::duplicate_eviction_candidate,
                           desc.graph_asset, 0, mount_id,
                           "MAVG page residency eviction candidate mount ids must be unique");
            invalid_inputs = true;
            continue;
        }
        eviction_mounts.push_back(mount_id);
    }

    if (invalid_inputs) {
        return result;
    }

    std::vector<rhi::RhiResidencyResourceRef> make_resident_resources;
    if (desc.make_selected_pages_resident) {
        for (const auto page_index : selected_pages) {
            const auto* const resource = find_resource_by_page(desc.resident_page_resources, page_index);
            if (resource == nullptr) {
                add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::missing_resident_page_resource,
                               desc.graph_asset, page_index, {},
                               "MAVG page residency selected pages must have resident resource rows");
                continue;
            }
            ++result.selected_page_resource_count;
            append_unique_resource(make_resident_resources, resource->resource);
            append_action_row(result, *resource, rhi::RhiResidencyActionKind::make_resident, true,
                              contains_mount_id(protected_mounts, resource->mount_id), false, false);
        }
        for (const auto mount_id : protected_mounts) {
            const auto* const resource = find_resource_by_mount_id(desc.resident_page_resources, mount_id);
            if (resource == nullptr) {
                add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::missing_resident_page_resource,
                               desc.graph_asset, 0, mount_id,
                               "MAVG page residency protected mounts must have resident resource rows");
                continue;
            }
            ++result.protected_page_resource_count;
            append_unique_resource(make_resident_resources, resource->resource);
            if (!contains_page_index(selected_pages, resource->page_index)) {
                append_action_row(result, *resource, rhi::RhiResidencyActionKind::make_resident, false, true, false,
                                  false);
            }
        }
    }

    std::vector<rhi::RhiResidencyResourceRef> evict_resources;
    if (desc.evict_reviewed_candidates) {
        for (const auto mount_id : eviction_mounts) {
            const auto* const resource = find_resource_by_mount_id(desc.resident_page_resources, mount_id);
            if (resource == nullptr) {
                add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::missing_resident_page_resource,
                               desc.graph_asset, 0, mount_id,
                               "MAVG page residency eviction candidates must have resident resource rows");
                continue;
            }
            if (contains_mount_id(protected_mounts, mount_id)) {
                ++result.protected_skip_count;
                append_action_row(result, *resource, rhi::RhiResidencyActionKind::evict,
                                  contains_page_index(selected_pages, resource->page_index), true, true, true);
                continue;
            }
            ++result.eviction_candidate_resource_count;
            append_unique_resource(evict_resources, resource->resource);
            append_action_row(result, *resource, rhi::RhiResidencyActionKind::evict,
                              contains_page_index(selected_pages, resource->page_index), false, true, false);
        }
    }

    if (!result.diagnostics.empty()) {
        result.action_rows.clear();
        return result;
    }

    if (!make_resident_resources.empty()) {
        result.invoked_rhi_residency_action = true;
        result.invoked_make_resident_action = true;
        result.make_resident_result = device.execute_residency_action(rhi::RhiResidencyActionDesc{
            .action = rhi::RhiResidencyActionKind::make_resident,
            .resources = make_resident_resources,
        });
        merge_rhi_evidence(result, result.make_resident_result);
        if (result.make_resident_result.status != rhi::RhiResidencyActionStatus::succeeded) {
            add_diagnostic(
                result, RuntimeMavgPageResidencyActionDiagnosticCode::rhi_make_resident_failed, desc.graph_asset, 0, {},
                "MAVG page residency make-resident RHI action failed: " + result.make_resident_result.diagnostic);
            return result;
        }
        result.made_resident_count = result.make_resident_result.acted_resource_count;
    }

    if (!evict_resources.empty()) {
        result.invoked_rhi_residency_action = true;
        result.invoked_evict_action = true;
        result.evict_result = device.execute_residency_action(rhi::RhiResidencyActionDesc{
            .action = rhi::RhiResidencyActionKind::evict,
            .resources = evict_resources,
        });
        merge_rhi_evidence(result, result.evict_result);
        if (result.evict_result.status != rhi::RhiResidencyActionStatus::succeeded) {
            add_diagnostic(result, RuntimeMavgPageResidencyActionDiagnosticCode::rhi_evict_failed, desc.graph_asset, 0,
                           {}, "MAVG page residency evict RHI action failed: " + result.evict_result.diagnostic);
            return result;
        }
        result.evicted_count = result.evict_result.acted_resource_count;
    }

    return result;
}

} // namespace mirakana::runtime_rhi
