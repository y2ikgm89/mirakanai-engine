// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/navigation_hierarchical_world.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

struct RegionEdge {
    std::size_t from{0U};
    std::size_t to{0U};
    std::string portal_id;
    NavigationHierarchicalWorldPortalEndpoint from_endpoint;
    NavigationHierarchicalWorldPortalEndpoint to_endpoint;
    std::uint32_t cost{0U};
};

[[nodiscard]] bool is_path_safe_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool is_safe_ref(std::string_view ref) {
    return !ref.empty() && std::ranges::none_of(ref, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == ':';
    });
}

[[nodiscard]] std::optional<std::size_t>
find_region_index(const std::vector<NavigationHierarchicalWorldRegion>& regions, std::string_view id) {
    for (std::size_t index = 0; index < regions.size(); ++index) {
        if (regions[index].region_id == id) {
            return index;
        }
    }
    return std::nullopt;
}

[[nodiscard]] bool has_duplicate_region_id(const std::vector<NavigationHierarchicalWorldRegion>& regions,
                                           std::size_t index) {
    for (std::size_t previous = 0; previous < index; ++previous) {
        if (regions[previous].region_id == regions[index].region_id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_duplicate_world_region_ref(const std::vector<NavigationHierarchicalWorldRegion>& regions,
                                                  std::size_t index) {
    for (std::size_t previous = 0; previous < index; ++previous) {
        if (regions[previous].world_region_ref == regions[index].world_region_ref) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_duplicate_nav_data_ref(const std::vector<NavigationHierarchicalWorldRegion>& regions,
                                              std::size_t index) {
    for (std::size_t previous = 0; previous < index; ++previous) {
        if (regions[previous].nav_data_ref == regions[index].nav_data_ref) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_duplicate_portal_id(const std::vector<NavigationHierarchicalWorldPortal>& portals,
                                           std::size_t index) {
    for (std::size_t previous = 0; previous < index; ++previous) {
        if (portals[previous].portal_id == portals[index].portal_id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] NavigationHierarchicalWorldPathResult
make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic diagnostic, std::string region_id = {},
                         std::string portal_id = {}) {
    NavigationHierarchicalWorldPathResult result;
    result.status = NavigationHierarchicalWorldPathStatus::invalid_world_graph;
    result.diagnostic = diagnostic;
    result.failing_region_id = std::move(region_id);
    result.failing_portal_id = std::move(portal_id);
    return result;
}

[[nodiscard]] bool endpoint_matches_region(const NavigationHierarchicalWorldPortalEndpoint& endpoint,
                                           const NavigationHierarchicalWorldRegion& region) {
    return endpoint.region_id == region.region_id && endpoint.nav_data_ref == region.nav_data_ref;
}

[[nodiscard]] std::optional<NavigationHierarchicalWorldPathResult>
validate_regions(const std::vector<NavigationHierarchicalWorldRegion>& regions) {
    if (regions.empty()) {
        NavigationHierarchicalWorldPathResult result;
        result.status = NavigationHierarchicalWorldPathStatus::invalid_request;
        result.diagnostic = NavigationHierarchicalWorldPathDiagnostic::empty_regions;
        return result;
    }

    for (std::size_t index = 0; index < regions.size(); ++index) {
        const auto& region = regions[index];
        if (!is_path_safe_id(region.region_id)) {
            return make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::invalid_region_id,
                                            region.region_id);
        }
        if (has_duplicate_region_id(regions, index)) {
            return make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::duplicate_region_id,
                                            region.region_id);
        }
        if (!is_safe_ref(region.world_region_ref)) {
            return make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::invalid_world_region_ref,
                                            region.region_id);
        }
        if (has_duplicate_world_region_ref(regions, index)) {
            return make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::duplicate_world_region_ref,
                                            region.region_id);
        }
        if (!is_safe_ref(region.nav_data_ref)) {
            return make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::invalid_nav_data_ref,
                                            region.region_id);
        }
        if (has_duplicate_nav_data_ref(regions, index)) {
            return make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::duplicate_nav_data_ref,
                                            region.region_id);
        }
        if (region.traversal_cost == 0U) {
            return make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::invalid_traversal_cost,
                                            region.region_id);
        }
    }
    return std::nullopt;
}

[[nodiscard]] NavigationHierarchicalWorldPortalPathRow
make_portal_row(const RegionEdge& edge, const std::vector<NavigationHierarchicalWorldRegion>& regions) {
    return NavigationHierarchicalWorldPortalPathRow{
        .portal_id = edge.portal_id,
        .from_region_id = regions[edge.from].region_id,
        .to_region_id = regions[edge.to].region_id,
        .from_nav_data_ref = edge.from_endpoint.nav_data_ref,
        .to_nav_data_ref = edge.to_endpoint.nav_data_ref,
        .from_polygon_id = edge.from_endpoint.polygon_id,
        .to_polygon_id = edge.to_endpoint.polygon_id,
        .from_scene_ref = edge.from_endpoint.scene_ref,
        .to_scene_ref = edge.to_endpoint.scene_ref,
        .cost = edge.cost,
    };
}

void append_edge(std::vector<RegionEdge>& edges, std::size_t from, std::size_t to,
                 const NavigationHierarchicalWorldPortal& portal) {
    edges.push_back(RegionEdge{
        .from = from,
        .to = to,
        .portal_id = portal.portal_id,
        .from_endpoint = portal.from,
        .to_endpoint = portal.to,
        .cost = portal.cost,
    });
}

[[nodiscard]] std::vector<RegionEdge> build_edges(const std::vector<NavigationHierarchicalWorldRegion>& regions,
                                                  const std::vector<NavigationHierarchicalWorldPortal>& portals,
                                                  std::optional<NavigationHierarchicalWorldPathResult>& failure) {
    std::vector<RegionEdge> edges;
    edges.reserve(portals.size() * 2U);

    for (std::size_t index = 0; index < portals.size(); ++index) {
        const auto& portal = portals[index];
        if (!is_path_safe_id(portal.portal_id)) {
            failure = make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::invalid_portal_id, {},
                                               portal.portal_id);
            return {};
        }
        if (has_duplicate_portal_id(portals, index)) {
            failure = make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::duplicate_portal_id, {},
                                               portal.portal_id);
            return {};
        }
        if (portal.cost == 0U) {
            failure = make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::invalid_portal_cost, {},
                                               portal.portal_id);
            return {};
        }
        if (portal.from.polygon_id == 0U || portal.to.polygon_id == 0U || !is_safe_ref(portal.from.scene_ref) ||
            !is_safe_ref(portal.to.scene_ref)) {
            failure = make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::invalid_portal_endpoint, {},
                                               portal.portal_id);
            return {};
        }

        const auto from = find_region_index(regions, portal.from.region_id);
        const auto to = find_region_index(regions, portal.to.region_id);
        if (!from.has_value() || !to.has_value() || *from == *to) {
            failure = make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::missing_portal_region,
                                               !from.has_value() ? portal.from.region_id : portal.to.region_id,
                                               portal.portal_id);
            return {};
        }
        if (!endpoint_matches_region(portal.from, regions[*from]) ||
            !endpoint_matches_region(portal.to, regions[*to])) {
            failure = make_world_graph_failure(
                NavigationHierarchicalWorldPathDiagnostic::portal_nav_data_region_mismatch, {}, portal.portal_id);
            return {};
        }

        append_edge(edges, *from, *to, portal);
        if (portal.bidirectional) {
            const auto reversed = NavigationHierarchicalWorldPortal{
                .portal_id = portal.portal_id,
                .from = portal.to,
                .to = portal.from,
                .cost = portal.cost,
                .bidirectional = false,
            };
            append_edge(edges, *to, *from, reversed);
        }
    }

    std::ranges::sort(edges, [&regions](const RegionEdge& lhs, const RegionEdge& rhs) {
        if (lhs.from != rhs.from) {
            return regions[lhs.from].region_id < regions[rhs.from].region_id;
        }
        if (lhs.cost != rhs.cost) {
            return lhs.cost < rhs.cost;
        }
        if (lhs.to != rhs.to) {
            return regions[lhs.to].region_id < regions[rhs.to].region_id;
        }
        return lhs.portal_id < rhs.portal_id;
    });
    return edges;
}

[[nodiscard]] bool is_better_tie(const RegionEdge& candidate, const RegionEdge& current,
                                 const std::vector<NavigationHierarchicalWorldRegion>& regions) {
    if (candidate.portal_id != current.portal_id) {
        return candidate.portal_id < current.portal_id;
    }
    if (regions[candidate.from].region_id != regions[current.from].region_id) {
        return regions[candidate.from].region_id < regions[current.from].region_id;
    }
    return regions[candidate.to].region_id < regions[current.to].region_id;
}

} // namespace

bool NavigationHierarchicalWorldPathResult::succeeded() const noexcept {
    return status == NavigationHierarchicalWorldPathStatus::success;
}

NavigationHierarchicalWorldPathResult
plan_navigation_hierarchical_world_path(const NavigationHierarchicalWorldPathRequest& request) {
    if (auto invalid_regions = validate_regions(request.regions)) {
        return *invalid_regions;
    }

    const auto start = find_region_index(request.regions, request.start_region_id);
    const auto goal = find_region_index(request.regions, request.goal_region_id);
    if (!start.has_value() || !goal.has_value()) {
        NavigationHierarchicalWorldPathResult result;
        result.status = NavigationHierarchicalWorldPathStatus::invalid_endpoint;
        result.diagnostic = NavigationHierarchicalWorldPathDiagnostic::invalid_region_id;
        result.failing_region_id = !start.has_value() ? request.start_region_id : request.goal_region_id;
        return result;
    }

    std::optional<NavigationHierarchicalWorldPathResult> edge_failure;
    const auto edges = build_edges(request.regions, request.portals, edge_failure);
    if (edge_failure.has_value()) {
        return *edge_failure;
    }

    NavigationHierarchicalWorldPathResult result;
    constexpr auto infinity = std::numeric_limits<std::uint64_t>::max();
    std::vector<std::uint64_t> distances(request.regions.size(), infinity);
    std::vector<std::optional<std::size_t>> previous_region(request.regions.size());
    std::vector<std::optional<std::size_t>> previous_edge(request.regions.size());
    std::vector<bool> visited(request.regions.size(), false);
    distances[*start] = 0U;

    while (true) {
        std::optional<std::size_t> current;
        for (std::size_t index = 0; index < request.regions.size(); ++index) {
            if (visited[index] || distances[index] == infinity) {
                continue;
            }
            if (!current.has_value() || distances[index] < distances[*current] ||
                (distances[index] == distances[*current] &&
                 request.regions[index].region_id < request.regions[*current].region_id)) {
                current = index;
            }
        }
        if (!current.has_value()) {
            break;
        }
        visited[*current] = true;
        ++result.visited_region_count;
        if (*current == *goal) {
            break;
        }

        for (std::size_t edge_index = 0; edge_index < edges.size(); ++edge_index) {
            const auto& edge = edges[edge_index];
            if (edge.from != *current || visited[edge.to]) {
                continue;
            }
            const auto step_cost = static_cast<std::uint64_t>(edge.cost) + request.regions[edge.to].traversal_cost;
            if (distances[*current] > infinity - step_cost) {
                return make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::cost_overflow,
                                                request.regions[edge.to].region_id, edge.portal_id);
            }
            const auto candidate = distances[*current] + step_cost;
            const auto better_tie = previous_edge[edge.to].has_value() &&
                                    is_better_tie(edge, edges[*previous_edge[edge.to]], request.regions);
            if (candidate < distances[edge.to] || (candidate == distances[edge.to] && better_tie)) {
                distances[edge.to] = candidate;
                previous_region[edge.to] = current;
                previous_edge[edge.to] = edge_index;
            }
        }
    }

    if (distances[*goal] == infinity) {
        result.status = NavigationHierarchicalWorldPathStatus::no_path;
        result.diagnostic = NavigationHierarchicalWorldPathDiagnostic::none;
        return result;
    }
    if (distances[*goal] > std::numeric_limits<std::uint32_t>::max()) {
        return make_world_graph_failure(NavigationHierarchicalWorldPathDiagnostic::cost_overflow,
                                        request.regions[*goal].region_id);
    }

    std::vector<std::size_t> reversed_regions;
    std::vector<std::size_t> reversed_edges;
    for (std::optional<std::size_t> cursor = goal; cursor.has_value(); cursor = previous_region[*cursor]) {
        reversed_regions.push_back(*cursor);
        if (*cursor == *start) {
            break;
        }
        if (!previous_edge[*cursor].has_value()) {
            result.status = NavigationHierarchicalWorldPathStatus::no_path;
            result.diagnostic = NavigationHierarchicalWorldPathDiagnostic::none;
            return result;
        }
        reversed_edges.push_back(*previous_edge[*cursor]);
    }
    std::ranges::reverse(reversed_regions);
    std::ranges::reverse(reversed_edges);

    for (const auto index : reversed_regions) {
        result.region_path.push_back(request.regions[index].region_id);
        result.world_region_refs.push_back(request.regions[index].world_region_ref);
        result.nav_data_refs.push_back(request.regions[index].nav_data_ref);
    }
    for (const auto edge_index : reversed_edges) {
        result.portal_path.push_back(edges[edge_index].portal_id);
        result.portal_rows.push_back(make_portal_row(edges[edge_index], request.regions));
    }
    result.status = NavigationHierarchicalWorldPathStatus::success;
    result.diagnostic = NavigationHierarchicalWorldPathDiagnostic::none;
    result.total_cost = static_cast<std::uint32_t>(distances[*goal]);
    return result;
}

} // namespace mirakana
