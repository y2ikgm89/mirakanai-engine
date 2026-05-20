// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/navigation/local_avoidance.hpp"
#include "mirakana/navigation/navigation_agent.hpp"
#include "mirakana/navigation/navigation_navmesh.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace mirakana {

enum class NavigationCrowdPlanStatus : std::uint8_t {
    success,
    invalid_request,
    invalid_agent,
    duplicate_agent,
    agent_budget_exceeded,
    route_failed,
    avoidance_failed,
};

enum class NavigationCrowdPlanDiagnostic : std::uint8_t {
    none,
    empty_agents,
    invalid_agent_id,
    duplicate_agent_id,
    invalid_agent_position,
    invalid_agent_velocity,
    invalid_agent_radius,
    invalid_agent_speed,
    invalid_agent_goal,
    invalid_agent_state,
    agent_budget_exceeded,
    route_failed,
    avoidance_failed,
};

struct NavigationCrowdAgentDesc {
    NavigationLocalAvoidanceAgentId id{0};
    NavigationPoint2 position{};
    std::uint32_t start_polygon{0};
    std::uint32_t goal_polygon{0};
    float radius{0.0F};
    NavigationAgentConfig config{};
    NavigationPoint2 desired_velocity{};
    NavigationAgentState state{};
    bool use_existing_state{false};
};

struct NavigationCrowdAgentPlanRow {
    NavigationLocalAvoidanceAgentId agent_id{0};
    NavigationCrowdPlanStatus status{NavigationCrowdPlanStatus::invalid_request};
    NavigationCrowdPlanDiagnostic diagnostic{NavigationCrowdPlanDiagnostic::none};
    NavigationNavmeshPathStatus route_status{NavigationNavmeshPathStatus::invalid_request};
    NavigationNavmeshPathDiagnostic route_diagnostic{NavigationNavmeshPathDiagnostic::none};
    NavigationLocalAvoidanceStatus avoidance_status{NavigationLocalAvoidanceStatus::invalid_request};
    NavigationLocalAvoidanceDiagnostic avoidance_diagnostic{NavigationLocalAvoidanceDiagnostic::none};
    std::size_t source_index{0};
    std::size_t neighbor_count{0};
    std::size_t applied_neighbor_count{0};
    std::size_t dynamic_obstacle_count{0};
    std::uint32_t route_cost{0};
    NavigationPoint2 desired_velocity{};
    NavigationPoint2 adjusted_velocity{};
    NavigationAgentState planned_state{};
};

struct NavigationCrowdPlanRequest {
    std::span<const NavigationNavmeshPolygon> polygons;
    std::span<const NavigationNavmeshPortal> portals;
    std::span<const NavigationNavmeshDynamicObstacle> dynamic_obstacles;
    std::span<const NavigationCrowdAgentDesc> agents;
    std::size_t max_agents{0};
    float separation_weight{1.0F};
    float prediction_time_seconds{1.0F};
    float epsilon{0.0001F};
};

struct NavigationCrowdPlanResult {
    NavigationCrowdPlanStatus status{NavigationCrowdPlanStatus::invalid_request};
    NavigationCrowdPlanDiagnostic diagnostic{NavigationCrowdPlanDiagnostic::none};
    std::vector<NavigationCrowdAgentPlanRow> rows;
    std::size_t planned_agent_count{0};
    std::size_t route_success_count{0};
    std::size_t avoidance_success_count{0};
    std::size_t applied_neighbor_count{0};
    std::size_t dynamic_obstacle_count{0};
    NavigationLocalAvoidanceAgentId failing_agent{0};
    std::size_t failing_source_index{0};
};

[[nodiscard]] NavigationCrowdPlanResult plan_navigation_navmesh_crowd(const NavigationCrowdPlanRequest& request);

} // namespace mirakana
