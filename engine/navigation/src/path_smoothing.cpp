// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/path_smoothing.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool same_coord(NavigationGridCoord lhs, NavigationGridCoord rhs) noexcept {
    return lhs == rhs;
}

[[nodiscard]] bool is_walkable(const NavigationGrid& grid, NavigationGridCoord coord) {
    return grid.in_bounds(coord) && grid.cell(coord).walkable;
}

[[nodiscard]] bool append_if_walkable(const NavigationGrid& grid, NavigationGridCoord coord,
                                      std::vector<NavigationGridCoord>& checked) {
    if (!is_walkable(grid, coord)) {
        return false;
    }
    if (std::ranges::none_of(checked, [coord](NavigationGridCoord existing) { return same_coord(existing, coord); })) {
        checked.push_back(coord);
    }
    return true;
}

[[nodiscard]] bool has_grid_line_of_sight(const NavigationGrid& grid, NavigationGridCoord from,
                                          NavigationGridCoord to) {
    std::vector<NavigationGridCoord> checked;
    if (!append_if_walkable(grid, from, checked)) {
        return false;
    }

    const std::int64_t delta_x = static_cast<std::int64_t>(to.x) - static_cast<std::int64_t>(from.x);
    const std::int64_t delta_y = static_cast<std::int64_t>(to.y) - static_cast<std::int64_t>(from.y);
    const std::int64_t dx = (delta_x < 0) ? -delta_x : delta_x;
    const std::int64_t dy = (delta_y < 0) ? -delta_y : delta_y;
    const int step_x = (to.x > from.x) ? 1 : ((to.x < from.x) ? -1 : 0);
    const int step_y = (to.y > from.y) ? 1 : ((to.y < from.y) ? -1 : 0);

    int current_x = from.x;
    int current_y = from.y;
    std::int64_t progress_x = 0;
    std::int64_t progress_y = 0;

    while (progress_x < dx || progress_y < dy) {
        const std::int64_t decision = ((1 + (2 * progress_x)) * dy) - ((1 + (2 * progress_y)) * dx);
        if (decision == 0) {
            const NavigationGridCoord horizontal{.x = current_x + step_x, .y = current_y};
            const NavigationGridCoord vertical{.x = current_x, .y = current_y + step_y};
            if (step_x != 0 && !append_if_walkable(grid, horizontal, checked)) {
                return false;
            }
            if (step_y != 0 && !append_if_walkable(grid, vertical, checked)) {
                return false;
            }
            current_x += step_x;
            current_y += step_y;
            ++progress_x;
            ++progress_y;
        } else if (decision < 0) {
            current_x += step_x;
            ++progress_x;
        } else {
            current_y += step_y;
            ++progress_y;
        }

        if (!append_if_walkable(grid, NavigationGridCoord{.x = current_x, .y = current_y}, checked)) {
            return false;
        }
    }

    return append_if_walkable(grid, to, checked);
}

} // namespace

NavigationGridPathSmoothingResult smooth_navigation_grid_path(const NavigationGrid& grid,
                                                              const NavigationGridPathSmoothingRequest& request) {
    NavigationGridPathSmoothingResult result;
    result.validation = validate_navigation_grid_path(grid, NavigationGridPathValidationRequest{
                                                                .start = request.start,
                                                                .goal = request.goal,
                                                                .path = request.path,
                                                                .adjacency = request.adjacency,
                                                            });
    if (result.validation.status == NavigationGridPathValidationStatus::unsupported_adjacency) {
        result.status = NavigationGridPathSmoothingStatus::unsupported_adjacency;
        return result;
    }
    if (result.validation.status != NavigationGridPathValidationStatus::valid) {
        result.status = NavigationGridPathSmoothingStatus::invalid_source_path;
        return result;
    }

    result.status = NavigationGridPathSmoothingStatus::success;
    if (request.path.size() <= 2U) {
        result.path.assign(request.path.begin(), request.path.end());
        return result;
    }

    result.path.push_back(request.path.front());
    std::size_t current_index = 0U;
    while (current_index + 1U < request.path.size()) {
        std::size_t next_index = request.path.size() - 1U;
        while (next_index > current_index + 1U &&
               !has_grid_line_of_sight(grid, request.path[current_index], request.path[next_index])) {
            --next_index;
        }

        result.path.push_back(request.path[next_index]);
        current_index = next_index;
    }

    result.removed_point_count = request.path.size() - result.path.size();
    return result;
}

} // namespace mirakana
