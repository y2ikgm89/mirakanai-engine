// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/navigation/path_following.hpp"

#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class NavigationNavmeshPathStatus : std::uint8_t {
    success,
    invalid_request,
    invalid_navmesh,
    invalid_endpoint,
    blocked_endpoint,
    no_path,
};

enum class NavigationNavmeshPathDiagnostic : std::uint8_t {
    none,
    empty_navmesh,
    invalid_polygon_id,
    duplicate_polygon_id,
    invalid_scene_ref,
    duplicate_scene_ref,
    invalid_polygon_center,
    invalid_traversal_cost,
    invalid_portal_endpoint,
    invalid_portal_cost,
    invalid_obstacle,
    duplicate_obstacle_id,
    blocked_start,
    blocked_goal,
    cost_overflow,
};

enum class NavigationNavmeshReadinessStatus : std::uint8_t {
    ready,
    diagnostics,
    invalid_result,
};

enum class NavigationNavmeshReadinessDiagnostic : std::uint8_t {
    none,
    invalid_path_result,
    missing_scene_refs,
    scene_ref_count_mismatch,
    missing_dynamic_obstacle_route,
    insufficient_polygon_path,
    insufficient_visited_polygons,
    total_cost_exceeded,
};

struct NavigationNavmeshPolygon {
    std::uint32_t id{0};
    std::string scene_ref;
    NavigationPoint2 center{};
    std::uint32_t traversal_cost{1U};
};

struct NavigationNavmeshPortal {
    std::uint32_t from{0};
    std::uint32_t to{0};
    std::uint32_t cost{1U};
    bool bidirectional{true};
};

struct NavigationNavmeshDynamicObstacle {
    std::uint32_t id{0};
    std::uint32_t blocked_polygon{0};
    std::string scene_ref;
    bool enabled{true};
};

struct NavigationNavmeshPathRequest {
    std::span<const NavigationNavmeshPolygon> polygons;
    std::span<const NavigationNavmeshPortal> portals;
    std::span<const NavigationNavmeshDynamicObstacle> dynamic_obstacles;
    std::uint32_t start{0};
    std::uint32_t goal{0};
};

struct NavigationNavmeshPathResult {
    NavigationNavmeshPathStatus status{NavigationNavmeshPathStatus::invalid_request};
    NavigationNavmeshPathDiagnostic diagnostic{NavigationNavmeshPathDiagnostic::none};
    std::vector<std::uint32_t> polygon_path;
    std::vector<std::string> scene_refs;
    std::vector<NavigationPoint2> point_path;
    std::uint32_t total_cost{0};
    std::size_t visited_polygon_count{0};
    std::size_t dynamic_obstacle_count{0};
    std::uint32_t failing_polygon{0};
    std::uint32_t failing_obstacle{0};
};

struct NavigationNavmeshReadinessConfig {
    bool require_scene_refs{false};
    bool require_dynamic_obstacle_route{false};
    std::size_t min_polygon_path_rows{0};
    std::size_t min_visited_polygons{0};
    std::uint32_t max_total_cost{std::numeric_limits<std::uint32_t>::max()};
};

struct NavigationNavmeshReadinessReport {
    NavigationNavmeshReadinessStatus status{NavigationNavmeshReadinessStatus::invalid_result};
    NavigationNavmeshReadinessDiagnostic diagnostic{NavigationNavmeshReadinessDiagnostic::none};
    std::size_t polygon_path_rows{0};
    std::size_t scene_ref_rows{0};
    std::size_t point_path_rows{0};
    std::size_t dynamic_obstacle_count{0};
    std::size_t visited_polygon_count{0};
    std::uint32_t total_cost{0};
    std::vector<NavigationNavmeshReadinessDiagnostic> diagnostics;
};

[[nodiscard]] NavigationNavmeshPathResult plan_navigation_navmesh_path(const NavigationNavmeshPathRequest& request);
[[nodiscard]] NavigationNavmeshReadinessReport
evaluate_navigation_navmesh_readiness(const NavigationNavmeshPathResult& route,
                                      const NavigationNavmeshReadinessConfig& config = {});

} // namespace mirakana
