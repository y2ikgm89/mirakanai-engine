// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/navigation_replan.hpp"

#include <limits>

namespace mirakana {

namespace {

[[nodiscard]] bool supports_adjacency(NavigationAdjacency adjacency) noexcept {
    switch (adjacency) {
    case NavigationAdjacency::cardinal4:
        return true;
    }

    return false;
}

[[nodiscard]] bool is_cardinal_step(NavigationGridCoord from, NavigationGridCoord to) noexcept {
    const auto dx = static_cast<std::int64_t>(to.x) - static_cast<std::int64_t>(from.x);
    const auto dy = static_cast<std::int64_t>(to.y) - static_cast<std::int64_t>(from.y);
    return ((dx == 1 || dx == -1) && dy == 0) || (dx == 0 && (dy == 1 || dy == -1));
}

[[nodiscard]] NavigationGridReplanStatus map_path_status(NavigationPathStatus status) noexcept {
    switch (status) {
    case NavigationPathStatus::success:
        return NavigationGridReplanStatus::replanned;
    case NavigationPathStatus::invalid_endpoint:
        return NavigationGridReplanStatus::invalid_endpoint;
    case NavigationPathStatus::blocked_endpoint:
        return NavigationGridReplanStatus::blocked_endpoint;
    case NavigationPathStatus::no_path:
        return NavigationGridReplanStatus::no_path;
    }

    return NavigationGridReplanStatus::no_path;
}

} // namespace

NavigationGridPathValidationResult validate_navigation_grid_path(const NavigationGrid& grid,
                                                                 const NavigationGridPathValidationRequest& request) {
    NavigationGridPathValidationResult result;

    if (!supports_adjacency(request.adjacency)) {
        result.status = NavigationGridPathValidationStatus::unsupported_adjacency;
        result.coord = request.start;
        return result;
    }

    if (request.path.empty()) {
        result.status = NavigationGridPathValidationStatus::empty_path;
        result.coord = request.start;
        return result;
    }

    if (request.path.front() != request.start) {
        result.status = NavigationGridPathValidationStatus::start_mismatch;
        result.coord = request.path.front();
        return result;
    }

    if (request.path.back() != request.goal) {
        result.status = NavigationGridPathValidationStatus::goal_mismatch;
        result.failing_index = request.path.size() - 1U;
        result.coord = request.path.back();
        return result;
    }

    constexpr auto max_cost = std::numeric_limits<std::uint32_t>::max();
    for (std::size_t index = 0; index < request.path.size(); ++index) {
        const auto coord = request.path[index];
        if (!grid.in_bounds(coord)) {
            result.status = NavigationGridPathValidationStatus::out_of_bounds_cell;
            result.failing_index = index;
            result.coord = coord;
            return result;
        }

        if (!grid.cell(coord).walkable) {
            result.status = NavigationGridPathValidationStatus::blocked_cell;
            result.failing_index = index;
            result.coord = coord;
            return result;
        }

        if (index == 0U) {
            continue;
        }

        if (!is_cardinal_step(request.path[index - 1U], coord)) {
            result.status = NavigationGridPathValidationStatus::non_cardinal_step;
            result.failing_index = index;
            result.coord = coord;
            return result;
        }

        const auto step_cost = grid.cell(coord).traversal_cost;
        if (result.total_cost > max_cost - step_cost) {
            result.status = NavigationGridPathValidationStatus::cost_overflow;
            result.failing_index = index;
            result.coord = coord;
            return result;
        }

        result.total_cost += step_cost;
    }

    result.status = NavigationGridPathValidationStatus::valid;
    result.failing_index = 0;
    result.coord = request.path.front();
    return result;
}

NavigationGridReplanResult replan_navigation_grid_path(const NavigationGrid& grid,
                                                       const NavigationGridReplanRequest& request) {
    NavigationGridReplanResult result;
    result.validation =
        validate_navigation_grid_path(grid, NavigationGridPathValidationRequest{.start = request.current,
                                                                                .goal = request.goal,
                                                                                .path = request.remaining_path,
                                                                                .adjacency = request.adjacency});

    if (result.validation.status == NavigationGridPathValidationStatus::unsupported_adjacency) {
        result.status = NavigationGridReplanStatus::unsupported_adjacency;
        return result;
    }

    if (result.validation.status == NavigationGridPathValidationStatus::valid && request.reuse_valid_path) {
        result.status = NavigationGridReplanStatus::reused_existing_path;
        result.path.assign(request.remaining_path.begin(), request.remaining_path.end());
        result.total_cost = result.validation.total_cost;
        result.reused_existing_path = true;
        return result;
    }

    const auto path = find_navigation_path(
        grid, NavigationPathRequest{.start = request.current, .goal = request.goal, .adjacency = request.adjacency});
    result.status = map_path_status(path.status);
    result.path = path.points;
    result.total_cost = path.total_cost;
    result.replanned = true;
    return result;
}

} // namespace mirakana
