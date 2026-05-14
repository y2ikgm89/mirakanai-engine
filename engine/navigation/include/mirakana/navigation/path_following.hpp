// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/navigation/navigation_grid.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace mirakana {

struct NavigationPoint2 {
    float x{0.0F};
    float y{0.0F};

    // Exact equality is intended for deterministic snapshots and grid-mapped points.
    friend constexpr bool operator==(NavigationPoint2 lhs, NavigationPoint2 rhs) noexcept {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    friend constexpr bool operator!=(NavigationPoint2 lhs, NavigationPoint2 rhs) noexcept {
        return !(lhs == rhs);
    }
};

struct NavigationGridPointMapping {
    NavigationPoint2 origin{};
    float cell_size{1.0F};
    bool use_cell_centers{true};
};

enum class NavigationPointPathBuildStatus : std::uint8_t {
    success,
    invalid_mapping,
};

struct NavigationPointPathBuildResult {
    NavigationPointPathBuildStatus status{NavigationPointPathBuildStatus::success};
    std::vector<NavigationPoint2> points;
};

enum class NavigationPathFollowStatus : std::uint8_t {
    advanced,
    reached_destination,
    invalid_path,
    invalid_request,
};

struct NavigationPathFollowRequest {
    std::vector<NavigationPoint2> points;
    NavigationPoint2 position{};
    std::size_t target_index{0};
    float max_step_distance{1.0F};
    float arrival_radius{0.01F};
};

struct NavigationPathFollowResult {
    NavigationPathFollowStatus status{NavigationPathFollowStatus::invalid_path};
    NavigationPoint2 position{};
    NavigationPoint2 step_delta{};
    std::size_t target_index{0};
    float advanced_distance{0.0F};
    float remaining_distance{0.0F};
    bool reached_destination{false};
};

enum class NavigationArriveSteeringStatus : std::uint8_t {
    moving,
    reached_target,
    invalid_request,
};

struct NavigationArriveSteeringRequest {
    NavigationPoint2 position{};
    NavigationPoint2 target{};
    float max_speed{1.0F};
    float slowing_radius{1.0F};
    float arrival_radius{0.01F};
};

struct NavigationArriveSteeringResult {
    NavigationArriveSteeringStatus status{NavigationArriveSteeringStatus::invalid_request};
    NavigationPoint2 desired_velocity{};
    float desired_speed{0.0F};
    float distance_to_target{0.0F};
    bool reached_target{false};
};

[[nodiscard]] std::vector<NavigationPoint2> build_navigation_point_path(std::span<const NavigationGridCoord> path,
                                                                        NavigationGridPointMapping mapping);

[[nodiscard]] NavigationPointPathBuildResult
build_navigation_point_path_result(std::span<const NavigationGridCoord> path, NavigationGridPointMapping mapping);

[[nodiscard]] NavigationPathFollowResult advance_navigation_path_following(const NavigationPathFollowRequest& request);

[[nodiscard]] NavigationArriveSteeringResult
calculate_navigation_arrive_steering(const NavigationArriveSteeringRequest& request);

} // namespace mirakana
