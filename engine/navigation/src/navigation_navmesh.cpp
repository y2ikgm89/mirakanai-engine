// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/navigation_navmesh.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace mirakana {
namespace {

struct NavmeshEdge {
    std::size_t from{0};
    std::size_t to{0};
    std::uint32_t cost{0};
};

[[nodiscard]] bool finite_point(NavigationPoint2 point) noexcept {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

[[nodiscard]] std::optional<std::size_t> find_polygon_index(std::span<const NavigationNavmeshPolygon> polygons,
                                                            std::uint32_t id) noexcept {
    for (std::size_t index = 0; index < polygons.size(); ++index) {
        if (polygons[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

[[nodiscard]] NavigationNavmeshPathResult invalid_navmesh(NavigationNavmeshPathDiagnostic diagnostic,
                                                          std::uint32_t polygon = 0U, std::uint32_t obstacle = 0U) {
    NavigationNavmeshPathResult result;
    result.status = NavigationNavmeshPathStatus::invalid_navmesh;
    result.diagnostic = diagnostic;
    result.failing_polygon = polygon;
    result.failing_obstacle = obstacle;
    return result;
}

[[nodiscard]] bool has_duplicate_scene_ref(std::span<const NavigationNavmeshPolygon> polygons, std::size_t index) {
    for (std::size_t previous = 0; previous < index; ++previous) {
        if (polygons[previous].scene_ref == polygons[index].scene_ref) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_duplicate_polygon_id(std::span<const NavigationNavmeshPolygon> polygons, std::size_t index) {
    for (std::size_t previous = 0; previous < index; ++previous) {
        if (polygons[previous].id == polygons[index].id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_duplicate_obstacle_id(std::span<const NavigationNavmeshDynamicObstacle> obstacles,
                                             std::size_t index) {
    if (obstacles[index].id == 0U) {
        return false;
    }
    for (std::size_t previous = 0; previous < index; ++previous) {
        if (obstacles[previous].enabled && obstacles[previous].id == obstacles[index].id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::vector<NavmeshEdge> build_edges(std::span<const NavigationNavmeshPolygon> polygons,
                                                   std::span<const NavigationNavmeshPortal> portals,
                                                   NavigationNavmeshPathResult& result) {
    std::vector<NavmeshEdge> edges;
    edges.reserve(portals.size() * 2U);
    for (const auto& portal : portals) {
        const auto from = find_polygon_index(polygons, portal.from);
        const auto to = find_polygon_index(polygons, portal.to);
        if (!from.has_value() || !to.has_value() || portal.from == portal.to) {
            result = invalid_navmesh(NavigationNavmeshPathDiagnostic::invalid_portal_endpoint, portal.from);
            return {};
        }
        if (portal.cost == 0U) {
            result = invalid_navmesh(NavigationNavmeshPathDiagnostic::invalid_portal_cost, portal.from);
            return {};
        }
        edges.push_back(NavmeshEdge{.from = *from, .to = *to, .cost = portal.cost});
        if (portal.bidirectional) {
            edges.push_back(NavmeshEdge{.from = *to, .to = *from, .cost = portal.cost});
        }
    }
    std::ranges::sort(edges, [](const NavmeshEdge& lhs, const NavmeshEdge& rhs) {
        if (lhs.from != rhs.from) {
            return lhs.from < rhs.from;
        }
        if (lhs.to != rhs.to) {
            return lhs.to < rhs.to;
        }
        return lhs.cost < rhs.cost;
    });
    return edges;
}

void append_readiness_diagnostic(NavigationNavmeshReadinessReport& report,
                                 NavigationNavmeshReadinessDiagnostic diagnostic) {
    if (report.diagnostics.empty()) {
        report.diagnostic = diagnostic;
    }
    report.diagnostics.push_back(diagnostic);
}

} // namespace

NavigationNavmeshPathResult plan_navigation_navmesh_path(const NavigationNavmeshPathRequest& request) {
    NavigationNavmeshPathResult result;

    if (request.polygons.empty()) {
        result.status = NavigationNavmeshPathStatus::invalid_request;
        result.diagnostic = NavigationNavmeshPathDiagnostic::empty_navmesh;
        return result;
    }

    for (std::size_t index = 0; index < request.polygons.size(); ++index) {
        const auto& polygon = request.polygons[index];
        if (polygon.id == 0U) {
            return invalid_navmesh(NavigationNavmeshPathDiagnostic::invalid_polygon_id);
        }
        if (has_duplicate_polygon_id(request.polygons, index)) {
            return invalid_navmesh(NavigationNavmeshPathDiagnostic::duplicate_polygon_id, polygon.id);
        }
        if (polygon.scene_ref.empty()) {
            return invalid_navmesh(NavigationNavmeshPathDiagnostic::invalid_scene_ref, polygon.id);
        }
        if (has_duplicate_scene_ref(request.polygons, index)) {
            return invalid_navmesh(NavigationNavmeshPathDiagnostic::duplicate_scene_ref, polygon.id);
        }
        if (!finite_point(polygon.center)) {
            return invalid_navmesh(NavigationNavmeshPathDiagnostic::invalid_polygon_center, polygon.id);
        }
        if (polygon.traversal_cost == 0U) {
            return invalid_navmesh(NavigationNavmeshPathDiagnostic::invalid_traversal_cost, polygon.id);
        }
    }

    const auto start = find_polygon_index(request.polygons, request.start);
    const auto goal = find_polygon_index(request.polygons, request.goal);
    if (!start.has_value() || !goal.has_value()) {
        result.status = NavigationNavmeshPathStatus::invalid_endpoint;
        result.diagnostic = NavigationNavmeshPathDiagnostic::invalid_polygon_id;
        result.failing_polygon = !start.has_value() ? request.start : request.goal;
        return result;
    }

    std::vector<bool> blocked(request.polygons.size(), false);
    for (std::size_t index = 0; index < request.dynamic_obstacles.size(); ++index) {
        const auto& obstacle = request.dynamic_obstacles[index];
        if (!obstacle.enabled) {
            continue;
        }
        if (obstacle.id == 0U || obstacle.scene_ref.empty()) {
            return invalid_navmesh(NavigationNavmeshPathDiagnostic::invalid_obstacle, obstacle.blocked_polygon,
                                   obstacle.id);
        }
        if (has_duplicate_obstacle_id(request.dynamic_obstacles, index)) {
            return invalid_navmesh(NavigationNavmeshPathDiagnostic::duplicate_obstacle_id, obstacle.blocked_polygon,
                                   obstacle.id);
        }
        const auto blocked_index = find_polygon_index(request.polygons, obstacle.blocked_polygon);
        if (!blocked_index.has_value()) {
            return invalid_navmesh(NavigationNavmeshPathDiagnostic::invalid_obstacle, obstacle.blocked_polygon,
                                   obstacle.id);
        }
        blocked[*blocked_index] = true;
        ++result.dynamic_obstacle_count;
    }

    if (blocked[*start]) {
        result.status = NavigationNavmeshPathStatus::blocked_endpoint;
        result.diagnostic = NavigationNavmeshPathDiagnostic::blocked_start;
        result.failing_polygon = request.start;
        return result;
    }
    if (blocked[*goal]) {
        result.status = NavigationNavmeshPathStatus::blocked_endpoint;
        result.diagnostic = NavigationNavmeshPathDiagnostic::blocked_goal;
        result.failing_polygon = request.goal;
        return result;
    }

    auto edge_validation = NavigationNavmeshPathResult{};
    auto edges = build_edges(request.polygons, request.portals, edge_validation);
    if (edge_validation.status == NavigationNavmeshPathStatus::invalid_navmesh) {
        edge_validation.dynamic_obstacle_count = result.dynamic_obstacle_count;
        return edge_validation;
    }

    constexpr auto infinity = std::numeric_limits<std::uint64_t>::max();
    std::vector<std::uint64_t> distances(request.polygons.size(), infinity);
    std::vector<std::optional<std::size_t>> previous(request.polygons.size());
    std::vector<bool> visited(request.polygons.size(), false);
    distances[*start] = 0U;

    while (true) {
        std::optional<std::size_t> current;
        for (std::size_t index = 0; index < request.polygons.size(); ++index) {
            if (visited[index] || blocked[index] || distances[index] == infinity) {
                continue;
            }
            if (!current.has_value() || distances[index] < distances[*current] ||
                (distances[index] == distances[*current] &&
                 request.polygons[index].id < request.polygons[*current].id)) {
                current = index;
            }
        }
        if (!current.has_value()) {
            break;
        }
        visited[*current] = true;
        ++result.visited_polygon_count;
        if (*current == *goal) {
            break;
        }

        for (const auto& edge : edges) {
            if (edge.from != *current || blocked[edge.to]) {
                continue;
            }
            const auto step_cost = static_cast<std::uint64_t>(edge.cost) + request.polygons[edge.to].traversal_cost;
            if (distances[*current] > infinity - step_cost) {
                result.status = NavigationNavmeshPathStatus::invalid_navmesh;
                result.diagnostic = NavigationNavmeshPathDiagnostic::cost_overflow;
                result.failing_polygon = request.polygons[edge.to].id;
                return result;
            }
            const auto candidate = distances[*current] + step_cost;
            if (candidate < distances[edge.to]) {
                distances[edge.to] = candidate;
                previous[edge.to] = current;
            }
        }
    }

    if (distances[*goal] == infinity) {
        result.status = NavigationNavmeshPathStatus::no_path;
        result.diagnostic = NavigationNavmeshPathDiagnostic::none;
        return result;
    }
    if (distances[*goal] > std::numeric_limits<std::uint32_t>::max()) {
        result.status = NavigationNavmeshPathStatus::invalid_navmesh;
        result.diagnostic = NavigationNavmeshPathDiagnostic::cost_overflow;
        result.failing_polygon = request.goal;
        return result;
    }

    std::vector<std::size_t> reversed_indices;
    for (std::optional<std::size_t> cursor = goal; cursor.has_value(); cursor = previous[*cursor]) {
        reversed_indices.push_back(*cursor);
        if (*cursor == *start) {
            break;
        }
    }
    std::ranges::reverse(reversed_indices);
    for (const auto index : reversed_indices) {
        result.polygon_path.push_back(request.polygons[index].id);
        result.scene_refs.push_back(request.polygons[index].scene_ref);
        result.point_path.push_back(request.polygons[index].center);
    }
    result.status = NavigationNavmeshPathStatus::success;
    result.diagnostic = NavigationNavmeshPathDiagnostic::none;
    result.total_cost = static_cast<std::uint32_t>(distances[*goal]);
    return result;
}

NavigationNavmeshReadinessReport evaluate_navigation_navmesh_readiness(const NavigationNavmeshPathResult& route,
                                                                       const NavigationNavmeshReadinessConfig& config) {
    NavigationNavmeshReadinessReport report;
    report.polygon_path_rows = route.polygon_path.size();
    report.scene_ref_rows = route.scene_refs.size();
    report.point_path_rows = route.point_path.size();
    report.dynamic_obstacle_count = route.dynamic_obstacle_count;
    report.visited_polygon_count = route.visited_polygon_count;
    report.total_cost = route.total_cost;

    if (route.status != NavigationNavmeshPathStatus::success ||
        route.diagnostic != NavigationNavmeshPathDiagnostic::none) {
        append_readiness_diagnostic(report, NavigationNavmeshReadinessDiagnostic::invalid_path_result);
        report.status = NavigationNavmeshReadinessStatus::invalid_result;
        return report;
    }

    if (config.require_scene_refs && route.scene_refs.empty()) {
        append_readiness_diagnostic(report, NavigationNavmeshReadinessDiagnostic::missing_scene_refs);
    }
    if (config.require_scene_refs && (route.scene_refs.size() != route.polygon_path.size() ||
                                      route.point_path.size() != route.polygon_path.size())) {
        append_readiness_diagnostic(report, NavigationNavmeshReadinessDiagnostic::scene_ref_count_mismatch);
    }
    if (config.require_dynamic_obstacle_route && route.dynamic_obstacle_count == 0U) {
        append_readiness_diagnostic(report, NavigationNavmeshReadinessDiagnostic::missing_dynamic_obstacle_route);
    }
    if (route.polygon_path.size() < config.min_polygon_path_rows) {
        append_readiness_diagnostic(report, NavigationNavmeshReadinessDiagnostic::insufficient_polygon_path);
    }
    if (route.visited_polygon_count < config.min_visited_polygons) {
        append_readiness_diagnostic(report, NavigationNavmeshReadinessDiagnostic::insufficient_visited_polygons);
    }
    if (route.total_cost > config.max_total_cost) {
        append_readiness_diagnostic(report, NavigationNavmeshReadinessDiagnostic::total_cost_exceeded);
    }

    report.status = report.diagnostics.empty() ? NavigationNavmeshReadinessStatus::ready
                                               : NavigationNavmeshReadinessStatus::diagnostics;
    return report;
}

} // namespace mirakana
