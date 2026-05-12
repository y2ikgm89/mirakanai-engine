// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/navigation_grid.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>

namespace mirakana {

namespace {

struct FrontierNode {
    std::uint32_t estimated_total{0};
    std::uint32_t cost_so_far{0};
    std::size_t cell_index{0};
    std::uint32_t sequence{0};
};

struct FrontierNodeGreater {
    bool operator()(const FrontierNode& lhs, const FrontierNode& rhs) const noexcept {
        if (lhs.estimated_total != rhs.estimated_total) {
            return lhs.estimated_total > rhs.estimated_total;
        }
        if (lhs.cost_so_far != rhs.cost_so_far) {
            return lhs.cost_so_far > rhs.cost_so_far;
        }
        return lhs.sequence > rhs.sequence;
    }
};

[[nodiscard]] std::uint32_t cardinal_heuristic(NavigationGridCoord from, NavigationGridCoord to) noexcept {
    const auto dx = static_cast<std::uint32_t>(std::abs(from.x - to.x));
    const auto dy = static_cast<std::uint32_t>(std::abs(from.y - to.y));
    return dx + dy;
}

[[nodiscard]] std::vector<NavigationGridCoord> cardinal_neighbors(NavigationGridCoord coord) {
    return {
        NavigationGridCoord{.x = coord.x + 1, .y = coord.y},
        NavigationGridCoord{.x = coord.x, .y = coord.y + 1},
        NavigationGridCoord{.x = coord.x - 1, .y = coord.y},
        NavigationGridCoord{.x = coord.x, .y = coord.y - 1},
    };
}

[[nodiscard]] NavigationGridCoord coord_from_index(NavigationGridSize size, std::size_t index) noexcept {
    return NavigationGridCoord{
        .x = static_cast<int>(index % size.width),
        .y = static_cast<int>(index / size.width),
    };
}

} // namespace

NavigationGrid::NavigationGrid(NavigationGridSize size) : size_(size) {
    if (size.width == 0 || size.height == 0) {
        throw std::invalid_argument("navigation grid dimensions must be non-zero");
    }

    cells_.resize(static_cast<std::size_t>(size.width) * static_cast<std::size_t>(size.height));
}

NavigationGridSize NavigationGrid::size() const noexcept {
    return size_;
}

bool NavigationGrid::in_bounds(NavigationGridCoord coord) const noexcept {
    return coord.x >= 0 && coord.y >= 0 && static_cast<std::uint32_t>(coord.x) < size_.width &&
           static_cast<std::uint32_t>(coord.y) < size_.height;
}

const NavigationCell& NavigationGrid::cell(NavigationGridCoord coord) const {
    return cells_.at(index_for(coord));
}

void NavigationGrid::set_walkable(NavigationGridCoord coord, bool walkable) {
    cells_.at(index_for(coord)).walkable = walkable;
}

void NavigationGrid::set_traversal_cost(NavigationGridCoord coord, std::uint32_t traversal_cost) {
    if (traversal_cost == 0) {
        throw std::invalid_argument("navigation traversal cost must be greater than zero");
    }

    cells_.at(index_for(coord)).traversal_cost = traversal_cost;
}

std::size_t NavigationGrid::index_for(NavigationGridCoord coord) const {
    if (!in_bounds(coord)) {
        throw std::out_of_range("navigation grid coordinate is out of bounds");
    }

    return (static_cast<std::size_t>(coord.y) * static_cast<std::size_t>(size_.width)) +
           static_cast<std::size_t>(coord.x);
}

NavigationPathResult find_navigation_path(const NavigationGrid& grid, const NavigationPathRequest& request) {
    NavigationPathResult result;

    if (!grid.in_bounds(request.start) || !grid.in_bounds(request.goal)) {
        result.status = NavigationPathStatus::invalid_endpoint;
        return result;
    }

    if (!grid.cell(request.start).walkable || !grid.cell(request.goal).walkable) {
        result.status = NavigationPathStatus::blocked_endpoint;
        return result;
    }

    const auto size = grid.size();
    const auto cell_count = static_cast<std::size_t>(size.width) * static_cast<std::size_t>(size.height);
    const auto start_index = (static_cast<std::size_t>(request.start.y) * static_cast<std::size_t>(size.width)) +
                             static_cast<std::size_t>(request.start.x);
    const auto goal_index = (static_cast<std::size_t>(request.goal.y) * static_cast<std::size_t>(size.width)) +
                            static_cast<std::size_t>(request.goal.x);

    if (start_index == goal_index) {
        result.status = NavigationPathStatus::success;
        result.points.push_back(request.start);
        return result;
    }

    constexpr auto unreachable_cost = std::numeric_limits<std::uint32_t>::max();
    std::vector<std::uint32_t> best_cost(cell_count, unreachable_cost);
    std::vector<std::size_t> came_from(cell_count, cell_count);
    std::vector<bool> closed(cell_count, false);

    std::priority_queue<FrontierNode, std::vector<FrontierNode>, FrontierNodeGreater> frontier;
    std::uint32_t sequence = 0;
    best_cost[start_index] = 0;
    frontier.push(FrontierNode{.estimated_total = cardinal_heuristic(request.start, request.goal),
                               .cost_so_far = 0,
                               .cell_index = start_index,
                               .sequence = sequence++});

    while (!frontier.empty()) {
        const auto current = frontier.top();
        frontier.pop();

        if (closed[current.cell_index]) {
            continue;
        }

        closed[current.cell_index] = true;
        ++result.visited_node_count;

        if (current.cell_index == goal_index) {
            result.status = NavigationPathStatus::success;
            result.total_cost = best_cost[goal_index];

            std::vector<NavigationGridCoord> reversed;
            for (auto cursor = goal_index; cursor != cell_count; cursor = came_from[cursor]) {
                reversed.push_back(coord_from_index(size, cursor));
                if (cursor == start_index) {
                    break;
                }
            }

            result.points.assign(reversed.rbegin(), reversed.rend());
            return result;
        }

        const auto current_coord = coord_from_index(size, current.cell_index);
        for (const auto neighbor : cardinal_neighbors(current_coord)) {
            if (!grid.in_bounds(neighbor)) {
                continue;
            }

            const auto& neighbor_cell = grid.cell(neighbor);
            if (!neighbor_cell.walkable) {
                continue;
            }

            const auto neighbor_index = (static_cast<std::size_t>(neighbor.y) * static_cast<std::size_t>(size.width)) +
                                        static_cast<std::size_t>(neighbor.x);
            if (closed[neighbor_index]) {
                continue;
            }

            if (best_cost[current.cell_index] > unreachable_cost - neighbor_cell.traversal_cost) {
                continue;
            }

            const auto next_cost = best_cost[current.cell_index] + neighbor_cell.traversal_cost;
            if (next_cost >= best_cost[neighbor_index]) {
                continue;
            }

            best_cost[neighbor_index] = next_cost;
            came_from[neighbor_index] = current.cell_index;
            const auto estimated_total = next_cost + cardinal_heuristic(neighbor, request.goal);
            frontier.push(FrontierNode{.estimated_total = estimated_total,
                                       .cost_so_far = next_cost,
                                       .cell_index = neighbor_index,
                                       .sequence = sequence++});
        }
    }

    result.status = NavigationPathStatus::no_path;
    return result;
}

} // namespace mirakana
