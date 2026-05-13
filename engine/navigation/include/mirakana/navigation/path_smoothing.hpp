// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/navigation/navigation_replan.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace mirakana {

enum class NavigationGridPathSmoothingStatus : std::uint8_t {
    success,
    invalid_source_path,
    unsupported_adjacency,
};

struct NavigationGridPathSmoothingRequest {
    NavigationGridCoord start;
    NavigationGridCoord goal;
    std::span<const NavigationGridCoord> path;
    NavigationAdjacency adjacency{NavigationAdjacency::cardinal4};
};

struct NavigationGridPathSmoothingResult {
    NavigationGridPathSmoothingStatus status{NavigationGridPathSmoothingStatus::invalid_source_path};
    NavigationGridPathValidationResult validation{};
    std::vector<NavigationGridCoord> path;
    std::size_t removed_point_count{0};
};

[[nodiscard]] NavigationGridPathSmoothingResult
smooth_navigation_grid_path(const NavigationGrid& grid, const NavigationGridPathSmoothingRequest& request);

} // namespace mirakana
