// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/navigation/path_following.hpp"

#include <cstdint>
#include <vector>

namespace mirakana {

enum class NavigationAgentStatus : std::uint8_t {
    idle,
    moving,
    reached_destination,
    cancelled,
    invalid_request,
};

struct NavigationAgentConfig {
    float max_speed{1.0F};
    float slowing_radius{1.0F};
    float arrival_radius{0.01F};
};

struct NavigationAgentState {
    NavigationPoint2 position{};
    std::vector<NavigationPoint2> path;
    std::size_t target_index{0};
    NavigationAgentStatus status{NavigationAgentStatus::idle};
};

struct NavigationAgentUpdateRequest {
    NavigationAgentState state;
    NavigationAgentConfig config;
    float delta_seconds{0.0F};
};

struct NavigationAgentUpdateResult {
    NavigationAgentStatus status{NavigationAgentStatus::invalid_request};
    NavigationAgentState state;
    NavigationPoint2 desired_velocity{};
    NavigationPoint2 step_delta{};
    float step_distance{0.0F};
    float remaining_distance{0.0F};
    bool reached_destination{false};
};

[[nodiscard]] NavigationAgentState make_navigation_agent_state(NavigationPoint2 position);

[[nodiscard]] NavigationAgentState replace_navigation_agent_path(NavigationAgentState state,
                                                                 std::vector<NavigationPoint2> path);

[[nodiscard]] NavigationAgentState cancel_navigation_agent_move(NavigationAgentState state);

[[nodiscard]] NavigationAgentUpdateResult update_navigation_agent(const NavigationAgentUpdateRequest& request);

} // namespace mirakana
