// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/mavg_lod_selection.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr float minimum_cluster_distance = 0.001F;

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool finite_mat4(const Mat4& value) noexcept {
    for (const auto entry : value.values()) {
        if (!std::isfinite(entry)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool valid_view(const MavgLodViewDesc& view) noexcept {
    return finite_mat4(view.clip_from_world) && finite_vec3(view.camera_world_position) &&
           std::isfinite(view.viewport_height_pixels) && view.viewport_height_pixels > 0.0F &&
           std::isfinite(view.target_error_pixels) && view.target_error_pixels > 0.0F &&
           std::isfinite(view.hysteresis_pixels) && view.hysteresis_pixels >= 0.0F;
}

[[nodiscard]] Vec3 bounds_center(const MavgBounds3f& bounds) noexcept {
    return Vec3{
        .x = (bounds.min.x + bounds.max.x) * 0.5F,
        .y = (bounds.min.y + bounds.max.y) * 0.5F,
        .z = (bounds.min.z + bounds.max.z) * 0.5F,
    };
}

[[nodiscard]] float screen_error_pixels(const MavgClusterGraphCluster& cluster, const MavgLodViewDesc& view) noexcept {
    const auto center = bounds_center(cluster.bounds);
    const auto distance = std::max(length(view.camera_world_position - center), minimum_cluster_distance);
    return cluster.geometric_error * view.viewport_height_pixels / distance;
}

[[nodiscard]] std::vector<std::uint32_t> sorted_unique(std::vector<std::uint32_t> values) {
    std::ranges::sort(values);
    const auto duplicates = std::ranges::unique(values);
    values.erase(duplicates.begin(), duplicates.end());
    return values;
}

[[nodiscard]] bool contains_sorted(const std::vector<std::uint32_t>& values, std::uint32_t value) {
    return std::ranges::binary_search(values, value);
}

[[nodiscard]] std::unordered_map<std::uint32_t, std::size_t>
build_cluster_offsets(const MavgClusterGraphDocument& graph) {
    std::unordered_map<std::uint32_t, std::size_t> offsets;
    offsets.reserve(graph.clusters.size());
    for (std::size_t index = 0; index < graph.clusters.size(); ++index) {
        offsets.emplace(graph.clusters[index].cluster_index, index);
    }
    return offsets;
}

[[nodiscard]] const MavgClusterGraphCluster*
find_cluster(const MavgClusterGraphDocument& graph,
             const std::unordered_map<std::uint32_t, std::size_t>& cluster_offsets, std::uint32_t cluster_index) {
    const auto found = cluster_offsets.find(cluster_index);
    if (found == cluster_offsets.end()) {
        return nullptr;
    }
    return &graph.clusters[found->second];
}

void append_diagnostic(MavgLodSelectionResult& result, MavgLodSelectionDiagnosticCode code, std::uint32_t cluster_index,
                       std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(MavgLodSelectionDiagnostic{
        .code = code,
        .cluster_index = cluster_index,
        .page_index = page_index,
        .message = std::move(message),
    });
}

[[nodiscard]] MavgLodSelectionDiagnosticCode map_graph_diagnostic(MavgClusterGraphDiagnosticCode code) noexcept {
    switch (code) {
    case MavgClusterGraphDiagnosticCode::missing_root_cluster:
        return MavgLodSelectionDiagnosticCode::missing_root_cluster;
    case MavgClusterGraphDiagnosticCode::missing_resident_fallback:
    case MavgClusterGraphDiagnosticCode::fallback_not_ancestor:
        return MavgLodSelectionDiagnosticCode::missing_resident_fallback;
    default:
        return MavgLodSelectionDiagnosticCode::invalid_graph;
    }
}

void append_graph_diagnostics(MavgLodSelectionResult& result, const MavgClusterGraphValidationResult& validation) {
    bool has_invalid_graph = false;
    for (const auto& diagnostic : validation.diagnostics) {
        const auto code = map_graph_diagnostic(diagnostic.code);
        has_invalid_graph = has_invalid_graph || code == MavgLodSelectionDiagnosticCode::invalid_graph;
        append_diagnostic(result, code, 0, 0, diagnostic.message);
    }
    if (!has_invalid_graph) {
        append_diagnostic(result, MavgLodSelectionDiagnosticCode::invalid_graph, 0, 0, "invalid MAVG cluster graph");
    }
}

void append_page_request(MavgLodSelectionResult& result, AssetId graph_asset, std::uint32_t page_index, float priority,
                         std::string reason) {
    for (auto& request : result.page_requests) {
        if (request.page_index == page_index) {
            request.priority = std::max(request.priority, priority);
            return;
        }
    }
    result.page_requests.push_back(MavgLodPageRequest{
        .graph_asset = graph_asset,
        .page_index = page_index,
        .priority = priority,
        .reason = std::move(reason),
    });
}

void append_selected_cluster(MavgLodSelectionResult& result, AssetId graph_asset,
                             const MavgClusterGraphCluster& cluster, bool fallback_substitution) {
    for (auto& selected : result.selected_clusters) {
        if (selected.cluster_index == cluster.cluster_index) {
            selected.fallback_substitution = selected.fallback_substitution || fallback_substitution;
            return;
        }
    }
    result.selected_clusters.push_back(MavgLodSelectedCluster{
        .graph_asset = graph_asset,
        .cluster_index = cluster.cluster_index,
        .page_index = cluster.page_index,
        .lod_level = cluster.lod_level,
        .material_partition = cluster.material_partition,
        .first_index = cluster.first_index,
        .index_count = cluster.index_count,
        .vertex_base = cluster.vertex_base,
        .fallback_substitution = fallback_substitution,
    });
}

[[nodiscard]] bool previous_selection_valid(const MavgLodPreviousSelection& previous,
                                            const std::unordered_map<std::uint32_t, std::size_t>& cluster_offsets) {
    if (previous.cluster_indices.empty()) {
        return false;
    }
    for (const auto cluster_index : previous.cluster_indices) {
        if (!cluster_offsets.contains(cluster_index)) {
            return false;
        }
    }
    return true;
}

void sort_selection_result(MavgLodSelectionResult& result) {
    std::ranges::sort(result.selected_clusters,
                      [](const MavgLodSelectedCluster& lhs, const MavgLodSelectedCluster& rhs) {
                          if (lhs.material_partition != rhs.material_partition) {
                              return lhs.material_partition < rhs.material_partition;
                          }
                          if (lhs.page_index != rhs.page_index) {
                              return lhs.page_index < rhs.page_index;
                          }
                          return lhs.cluster_index < rhs.cluster_index;
                      });
    std::ranges::sort(result.page_requests, [](const MavgLodPageRequest& lhs, const MavgLodPageRequest& rhs) {
        if (lhs.page_index != rhs.page_index) {
            return lhs.page_index < rhs.page_index;
        }
        return lhs.graph_asset.value < rhs.graph_asset.value;
    });
}

[[nodiscard]] std::uint32_t selected_parent_score(const MavgLodSelectionResult& result,
                                                  const MavgClusterGraphDocument& graph,
                                                  const std::unordered_map<std::uint32_t, std::size_t>& cluster_offsets,
                                                  std::uint32_t parent_cluster_index) {
    std::uint32_t count = 0;
    for (const auto& selected : result.selected_clusters) {
        const auto* cluster = find_cluster(graph, cluster_offsets, selected.cluster_index);
        if (cluster != nullptr && cluster->has_parent && cluster->parent_cluster_index == parent_cluster_index) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::uint32_t choose_budget_parent(const MavgLodSelectionResult& result,
                                                 const MavgClusterGraphDocument& graph,
                                                 const std::unordered_map<std::uint32_t, std::size_t>& cluster_offsets,
                                                 const std::vector<std::uint32_t>& resident_page_indices) {
    std::uint32_t best_parent = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t best_score = 0;
    float best_error = -1.0F;

    for (const auto& selected : result.selected_clusters) {
        const auto* cluster = find_cluster(graph, cluster_offsets, selected.cluster_index);
        if (cluster == nullptr || !cluster->has_parent) {
            continue;
        }
        const auto* parent = find_cluster(graph, cluster_offsets, cluster->parent_cluster_index);
        if (parent == nullptr || !contains_sorted(resident_page_indices, parent->page_index)) {
            continue;
        }
        const auto score = selected_parent_score(result, graph, cluster_offsets, parent->cluster_index);
        if (score > best_score || (score == best_score && parent->geometric_error > best_error) ||
            (score == best_score && parent->geometric_error == best_error && parent->cluster_index < best_parent)) {
            best_parent = parent->cluster_index;
            best_score = score;
            best_error = parent->geometric_error;
        }
    }
    return best_parent;
}

void degrade_selection_budget(MavgLodSelectionResult& result, const MavgClusterGraphDocument& graph,
                              const std::unordered_map<std::uint32_t, std::size_t>& cluster_offsets,
                              const std::vector<std::uint32_t>& resident_page_indices,
                              std::uint32_t max_selected_clusters) {
    if (max_selected_clusters == 0 || result.selected_clusters.size() <= max_selected_clusters) {
        return;
    }

    bool emitted_diagnostic = false;
    while (result.selected_clusters.size() > max_selected_clusters) {
        const auto parent_index = choose_budget_parent(result, graph, cluster_offsets, resident_page_indices);
        if (parent_index == std::numeric_limits<std::uint32_t>::max()) {
            break;
        }
        const auto* parent = find_cluster(graph, cluster_offsets, parent_index);
        if (parent == nullptr) {
            break;
        }

        std::erase_if(result.selected_clusters, [parent_index, &graph, &cluster_offsets](const auto& selected) {
            const auto* cluster = find_cluster(graph, cluster_offsets, selected.cluster_index);
            return cluster != nullptr && cluster->has_parent && cluster->parent_cluster_index == parent_index;
        });
        append_selected_cluster(result, graph.asset, *parent, true);
        result.budget_degraded = true;
        if (!emitted_diagnostic) {
            append_diagnostic(result, MavgLodSelectionDiagnosticCode::budget_degraded, parent_index, parent->page_index,
                              "MAVG LOD selection degraded to an ancestor to satisfy max_selected_clusters");
            emitted_diagnostic = true;
        }
    }
}

} // namespace

bool MavgLodSelectionResult::succeeded() const noexcept {
    for (const auto& diagnostic : diagnostics) {
        switch (diagnostic.code) {
        case MavgLodSelectionDiagnosticCode::invalid_graph:
        case MavgLodSelectionDiagnosticCode::invalid_view:
        case MavgLodSelectionDiagnosticCode::missing_root_cluster:
        case MavgLodSelectionDiagnosticCode::missing_resident_fallback:
            return false;
        case MavgLodSelectionDiagnosticCode::missing_page:
        case MavgLodSelectionDiagnosticCode::budget_degraded:
        case MavgLodSelectionDiagnosticCode::hysteresis_reused_previous:
            break;
        }
    }
    return true;
}

MavgLodSelectionResult select_mavg_lod_clusters(const MavgClusterGraphDocument& graph, const MavgLodViewDesc& view,
                                                const MavgLodResidentPageSet& resident_pages,
                                                const MavgLodPreviousSelection& previous) {
    MavgLodSelectionResult result;
    if (!valid_view(view)) {
        append_diagnostic(result, MavgLodSelectionDiagnosticCode::invalid_view, 0, 0, "invalid MAVG LOD view");
        return result;
    }

    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        append_graph_diagnostics(result, validation);
        return result;
    }

    const auto canonical = canonicalize_mavg_cluster_graph(graph);
    const auto cluster_offsets = build_cluster_offsets(canonical);
    const auto resident_page_indices = sorted_unique(resident_pages.page_indices);
    const auto previous_valid = previous_selection_valid(previous, cluster_offsets);
    const auto previous_cluster_indices =
        previous_valid ? sorted_unique(previous.cluster_indices) : std::vector<std::uint32_t>{};

    auto select_cluster = [&](auto&& self, const MavgClusterGraphCluster& cluster) -> void {
        if (!contains_sorted(resident_page_indices, cluster.page_index)) {
            ++result.missing_page_count;
            const auto priority = screen_error_pixels(cluster, view);
            append_page_request(result, canonical.asset, cluster.page_index, priority, "missing_cluster_page");
            append_diagnostic(result, MavgLodSelectionDiagnosticCode::missing_page, cluster.cluster_index,
                              cluster.page_index, "MAVG cluster page is not resident");

            const auto* fallback = find_cluster(canonical, cluster_offsets, cluster.resident_fallback_cluster_index);
            if (fallback == nullptr || !contains_sorted(resident_page_indices, fallback->page_index)) {
                append_diagnostic(result, MavgLodSelectionDiagnosticCode::missing_resident_fallback,
                                  cluster.cluster_index, cluster.page_index,
                                  "MAVG cluster page is missing and no resident fallback cluster is available");
                return;
            }
            ++result.fallback_substitution_count;
            append_selected_cluster(result, canonical.asset, *fallback, true);
            return;
        }

        const auto error_pixels = screen_error_pixels(cluster, view);
        const auto keep_previous = previous_valid && contains_sorted(previous_cluster_indices, cluster.cluster_index) &&
                                   error_pixels <= (view.target_error_pixels + view.hysteresis_pixels);
        if (!cluster.children.empty() && !keep_previous && error_pixels > view.target_error_pixels) {
            auto child_indices = sorted_unique(cluster.children);
            for (const auto child_index : child_indices) {
                const auto* child = find_cluster(canonical, cluster_offsets, child_index);
                if (child != nullptr) {
                    self(self, *child);
                }
            }
            return;
        }

        if (keep_previous && !cluster.children.empty() && error_pixels > view.target_error_pixels) {
            append_diagnostic(result, MavgLodSelectionDiagnosticCode::hysteresis_reused_previous, cluster.cluster_index,
                              cluster.page_index,
                              "MAVG LOD hysteresis reused a previous cluster within the hysteresis band");
        }
        append_selected_cluster(result, canonical.asset, cluster, false);
    };

    std::vector<std::uint32_t> root_indices;
    for (const auto& cluster : canonical.clusters) {
        if (!cluster.has_parent) {
            root_indices.push_back(cluster.cluster_index);
        }
    }
    root_indices = sorted_unique(std::move(root_indices));
    if (root_indices.empty()) {
        append_diagnostic(result, MavgLodSelectionDiagnosticCode::missing_root_cluster, 0, 0,
                          "MAVG cluster graph has no root cluster");
        return result;
    }

    for (const auto root_index : root_indices) {
        const auto* root = find_cluster(canonical, cluster_offsets, root_index);
        if (root != nullptr) {
            select_cluster(select_cluster, *root);
        }
    }

    degrade_selection_budget(result, canonical, cluster_offsets, resident_page_indices, view.max_selected_clusters);
    sort_selection_result(result);
    return result;
}

} // namespace mirakana
