// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/navigation/navigation_grid.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace mirakana {

enum class NavigationGridPathValidationStatus {
    valid,
    empty_path,
    start_mismatch,
    goal_mismatch,
    out_of_bounds_cell,
    blocked_cell,
    non_cardinal_step,
    unsupported_adjacency,
    cost_overflow,
};

struct NavigationGridPathValidationRequest {
    NavigationGridCoord start;
    NavigationGridCoord goal;
    std::span<const NavigationGridCoord> path;
    NavigationAdjacency adjacency{NavigationAdjacency::cardinal4};
};

struct NavigationGridPathValidationResult {
    NavigationGridPathValidationStatus status{NavigationGridPathValidationStatus::empty_path};
    std::size_t failing_index{0};
    NavigationGridCoord coord{};
    std::uint32_t total_cost{0};
};

enum class NavigationGridReplanStatus {
    reused_existing_path,
    replanned,
    unsupported_adjacency,
    invalid_endpoint,
    blocked_endpoint,
    no_path,
};

struct NavigationGridReplanRequest {
    NavigationGridCoord current;
    NavigationGridCoord goal;
    std::span<const NavigationGridCoord> remaining_path;
    NavigationAdjacency adjacency{NavigationAdjacency::cardinal4};
    bool reuse_valid_path{true};
};

struct NavigationGridReplanResult {
    NavigationGridReplanStatus status{NavigationGridReplanStatus::no_path};
    NavigationGridPathValidationResult validation{};
    std::vector<NavigationGridCoord> path;
    std::uint32_t total_cost{0};
    bool reused_existing_path{false};
    bool replanned{false};
};

[[nodiscard]] NavigationGridPathValidationResult
validate_navigation_grid_path(const NavigationGrid& grid, const NavigationGridPathValidationRequest& request);

[[nodiscard]] NavigationGridReplanResult replan_navigation_grid_path(const NavigationGrid& grid,
                                                                     const NavigationGridReplanRequest& request);

} // namespace mirakana
