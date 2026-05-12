// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mirakana {

struct NavigationGridSize {
    std::uint32_t width{0};
    std::uint32_t height{0};
};

struct NavigationGridCoord {
    int x{0};
    int y{0};

    friend constexpr bool operator==(NavigationGridCoord lhs, NavigationGridCoord rhs) noexcept {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    friend constexpr bool operator!=(NavigationGridCoord lhs, NavigationGridCoord rhs) noexcept {
        return !(lhs == rhs);
    }
};

enum class NavigationAdjacency { cardinal4 };

struct NavigationCell {
    bool walkable{true};
    std::uint32_t traversal_cost{1};
};

enum class NavigationPathStatus {
    success,
    invalid_endpoint,
    blocked_endpoint,
    no_path,
};

struct NavigationPathRequest {
    NavigationGridCoord start;
    NavigationGridCoord goal;
    NavigationAdjacency adjacency{NavigationAdjacency::cardinal4};
};

struct NavigationPathResult {
    NavigationPathStatus status{NavigationPathStatus::no_path};
    std::vector<NavigationGridCoord> points;
    std::uint32_t total_cost{0};
    std::size_t visited_node_count{0};
};

class NavigationGrid {
  public:
    explicit NavigationGrid(NavigationGridSize size);

    [[nodiscard]] NavigationGridSize size() const noexcept;
    [[nodiscard]] bool in_bounds(NavigationGridCoord coord) const noexcept;
    [[nodiscard]] const NavigationCell& cell(NavigationGridCoord coord) const;

    void set_walkable(NavigationGridCoord coord, bool walkable);
    void set_traversal_cost(NavigationGridCoord coord, std::uint32_t traversal_cost);

  private:
    [[nodiscard]] std::size_t index_for(NavigationGridCoord coord) const;

    NavigationGridSize size_;
    std::vector<NavigationCell> cells_;
};

[[nodiscard]] NavigationPathResult find_navigation_path(const NavigationGrid& grid,
                                                        const NavigationPathRequest& request);

} // namespace mirakana
