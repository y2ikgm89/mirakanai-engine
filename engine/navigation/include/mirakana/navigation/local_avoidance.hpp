// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/navigation/path_following.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace mirakana {

using NavigationLocalAvoidanceAgentId = std::uint64_t;

enum class NavigationLocalAvoidanceStatus {
    success,
    invalid_request,
};

enum class NavigationLocalAvoidanceDiagnostic {
    none,
    invalid_agent_id,
    invalid_position,
    invalid_desired_velocity,
    invalid_radius,
    invalid_max_speed,
    invalid_neighbor_id,
    invalid_neighbor_position,
    invalid_neighbor_velocity,
    invalid_neighbor_radius,
    duplicate_neighbor_id,
    invalid_separation_weight,
    invalid_prediction_time_seconds,
    invalid_epsilon,
    calculation_overflow,
};

struct NavigationLocalAvoidanceAgentDesc {
    NavigationLocalAvoidanceAgentId id{0};
    NavigationPoint2 position{};
    NavigationPoint2 desired_velocity{};
    float radius{0.0F};
    float max_speed{0.0F};
};

struct NavigationLocalAvoidanceNeighborDesc {
    NavigationLocalAvoidanceAgentId id{0};
    NavigationPoint2 position{};
    NavigationPoint2 velocity{};
    float radius{0.0F};
};

struct NavigationLocalAvoidanceRequest {
    NavigationLocalAvoidanceAgentDesc agent{};
    std::span<const NavigationLocalAvoidanceNeighborDesc> neighbors;
    float separation_weight{1.0F};
    float prediction_time_seconds{1.0F};
    float epsilon{0.0001F};
};

struct NavigationLocalAvoidanceResult {
    NavigationLocalAvoidanceStatus status{NavigationLocalAvoidanceStatus::invalid_request};
    NavigationLocalAvoidanceDiagnostic diagnostic{NavigationLocalAvoidanceDiagnostic::none};
    NavigationPoint2 adjusted_velocity{};
    std::size_t applied_neighbor_count{0};
    bool clamped_to_max_speed{false};
};

[[nodiscard]] NavigationLocalAvoidanceResult
calculate_navigation_local_avoidance(const NavigationLocalAvoidanceRequest& request);

} // namespace mirakana
