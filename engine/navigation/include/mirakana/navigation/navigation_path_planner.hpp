// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/navigation/navigation_agent.hpp"
#include "mirakana/navigation/path_smoothing.hpp"

#include <cstddef>
#include <cstdint>

namespace mirakana {

enum class NavigationGridAgentPathStatus {
    ready,
    invalid_request,
    invalid_mapping,
    unsupported_adjacency,
    invalid_endpoint,
    blocked_endpoint,
    no_path,
    invalid_source_path,
    agent_path_invalid,
};

enum class NavigationGridAgentPathDiagnostic {
    none,
    invalid_mapping,
    unsupported_adjacency,
    path_invalid_endpoint,
    path_blocked_endpoint,
    path_not_found,
    smoothing_invalid_source_path,
    smoothing_unsupported_adjacency,
    point_mapping_failed,
    agent_path_rejected,
};

struct NavigationGridAgentPathRequest {
    NavigationGridCoord start;
    NavigationGridCoord goal;
    NavigationGridPointMapping mapping{};
    NavigationAdjacency adjacency{NavigationAdjacency::cardinal4};
    bool smooth_path{true};
};

struct NavigationGridAgentPathPlan {
    NavigationGridAgentPathStatus status{NavigationGridAgentPathStatus::invalid_request};
    NavigationGridAgentPathDiagnostic diagnostic{NavigationGridAgentPathDiagnostic::none};
    NavigationPathResult path_result{};
    NavigationGridPathSmoothingResult smoothing_result{};
    NavigationPointPathBuildResult point_path_result{};
    NavigationAgentState agent_state{};
    std::size_t raw_grid_point_count{0};
    std::size_t planned_grid_point_count{0};
    std::uint32_t total_cost{0};
    bool used_smoothing{false};
};

[[nodiscard]] NavigationGridAgentPathPlan
plan_navigation_grid_agent_path(const NavigationGrid& grid, const NavigationGridAgentPathRequest& request);

} // namespace mirakana
