// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/navigation_crowd.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_point(NavigationPoint2 point) noexcept {
    return finite(point.x) && finite(point.y);
}

[[nodiscard]] bool finite_path(const std::vector<NavigationPoint2>& path) noexcept {
    return std::ranges::all_of(path, [](NavigationPoint2 point) { return finite_point(point); });
}

[[nodiscard]] bool valid_agent_config(NavigationAgentConfig config) noexcept {
    return finite(config.max_speed) && config.max_speed > 0.0F && finite(config.slowing_radius) &&
           config.slowing_radius > 0.0F && finite(config.arrival_radius) && config.arrival_radius >= 0.0F;
}

[[nodiscard]] bool valid_agent_status(NavigationAgentStatus status) noexcept {
    switch (status) {
    case NavigationAgentStatus::idle:
    case NavigationAgentStatus::moving:
    case NavigationAgentStatus::reached_destination:
    case NavigationAgentStatus::cancelled:
        return true;
    case NavigationAgentStatus::invalid_request:
        return false;
    }
    return false;
}

[[nodiscard]] bool valid_agent_state(const NavigationAgentState& state) noexcept {
    if (!finite_point(state.position) || !finite_path(state.path) || !valid_agent_status(state.status)) {
        return false;
    }
    if (state.path.empty()) {
        return state.target_index == 0U &&
               (state.status == NavigationAgentStatus::idle || state.status == NavigationAgentStatus::cancelled);
    }
    return state.target_index < state.path.size() && (state.status == NavigationAgentStatus::moving ||
                                                      state.status == NavigationAgentStatus::reached_destination);
}

[[nodiscard]] NavigationCrowdPlanResult invalid_result(NavigationCrowdPlanStatus status,
                                                       NavigationCrowdPlanDiagnostic diagnostic,
                                                       NavigationLocalAvoidanceAgentId failing_agent,
                                                       std::size_t failing_source_index) {
    return NavigationCrowdPlanResult{
        .status = status,
        .diagnostic = diagnostic,
        .rows = {},
        .planned_agent_count = 0U,
        .route_success_count = 0U,
        .avoidance_success_count = 0U,
        .applied_neighbor_count = 0U,
        .dynamic_obstacle_count = 0U,
        .failing_agent = failing_agent,
        .failing_source_index = failing_source_index,
    };
}

[[nodiscard]] NavigationCrowdPlanDiagnostic validate_agent(const NavigationCrowdAgentDesc& agent) noexcept {
    if (agent.id == 0U) {
        return NavigationCrowdPlanDiagnostic::invalid_agent_id;
    }
    if (!finite_point(agent.position)) {
        return NavigationCrowdPlanDiagnostic::invalid_agent_position;
    }
    if (!finite_point(agent.desired_velocity)) {
        return NavigationCrowdPlanDiagnostic::invalid_agent_velocity;
    }
    if (!finite(agent.radius) || agent.radius < 0.0F) {
        return NavigationCrowdPlanDiagnostic::invalid_agent_radius;
    }
    if (!valid_agent_config(agent.config)) {
        return NavigationCrowdPlanDiagnostic::invalid_agent_speed;
    }
    if (agent.start_polygon == 0U || agent.goal_polygon == 0U) {
        return NavigationCrowdPlanDiagnostic::invalid_agent_goal;
    }
    if (agent.use_existing_state && !valid_agent_state(agent.state)) {
        return NavigationCrowdPlanDiagnostic::invalid_agent_state;
    }
    return NavigationCrowdPlanDiagnostic::none;
}

[[nodiscard]] std::vector<std::size_t> sorted_agent_indices(std::span<const NavigationCrowdAgentDesc> agents) {
    std::vector<std::size_t> indices(agents.size());
    // NOLINTNEXTLINE(modernize-use-ranges): hosted Clang/AppleClang CI lacks std::ranges::iota.
    std::iota(indices.begin(), indices.end(), std::size_t{0});
    std::ranges::stable_sort(indices, [&agents](std::size_t lhs, std::size_t rhs) {
        if (agents[lhs].id == agents[rhs].id) {
            return lhs < rhs;
        }
        return agents[lhs].id < agents[rhs].id;
    });
    return indices;
}

[[nodiscard]] bool is_zero_velocity(NavigationPoint2 velocity) noexcept {
    return velocity.x == 0.0F && velocity.y == 0.0F;
}

[[nodiscard]] bool derive_desired_velocity(const NavigationCrowdAgentDesc& agent,
                                           const NavigationNavmeshPathResult& route, NavigationPoint2& out) noexcept {
    if (!is_zero_velocity(agent.desired_velocity)) {
        out = agent.desired_velocity;
        return true;
    }
    if (route.point_path.size() < 2U) {
        out = {};
        return true;
    }

    const auto steering = calculate_navigation_arrive_steering(NavigationArriveSteeringRequest{
        .position = agent.position,
        .target = route.point_path[1U],
        .max_speed = agent.config.max_speed,
        .slowing_radius = agent.config.slowing_radius,
        .arrival_radius = agent.config.arrival_radius,
    });
    if (steering.status == NavigationArriveSteeringStatus::invalid_request) {
        return false;
    }
    out = steering.desired_velocity;
    return true;
}

[[nodiscard]] std::vector<NavigationLocalAvoidanceNeighborDesc>
build_neighbors(std::span<const NavigationCrowdAgentDesc> agents, std::span<const std::size_t> sorted_indices,
                std::size_t source_index) {
    std::vector<NavigationLocalAvoidanceNeighborDesc> neighbors;
    neighbors.reserve(agents.size() > 0U ? agents.size() - 1U : 0U);
    for (const std::size_t neighbor_index : sorted_indices) {
        if (neighbor_index == source_index) {
            continue;
        }
        const NavigationCrowdAgentDesc& neighbor = agents[neighbor_index];
        neighbors.push_back(NavigationLocalAvoidanceNeighborDesc{
            .id = neighbor.id,
            .position = neighbor.position,
            .velocity = neighbor.desired_velocity,
            .radius = neighbor.radius,
        });
    }
    return neighbors;
}

void set_result_failure(NavigationCrowdPlanResult& result, NavigationCrowdPlanStatus status,
                        NavigationCrowdPlanDiagnostic diagnostic, NavigationLocalAvoidanceAgentId agent_id,
                        std::size_t source_index) {
    if (result.status == NavigationCrowdPlanStatus::success) {
        result.status = status;
        result.diagnostic = diagnostic;
        result.failing_agent = agent_id;
        result.failing_source_index = source_index;
    }
}

[[nodiscard]] NavigationCrowdAgentPlanRow make_base_row(const NavigationCrowdAgentDesc& agent,
                                                        std::size_t source_index) {
    return NavigationCrowdAgentPlanRow{
        .agent_id = agent.id,
        .status = NavigationCrowdPlanStatus::invalid_request,
        .diagnostic = NavigationCrowdPlanDiagnostic::none,
        .route_status = NavigationNavmeshPathStatus::invalid_request,
        .route_diagnostic = NavigationNavmeshPathDiagnostic::none,
        .avoidance_status = NavigationLocalAvoidanceStatus::invalid_request,
        .avoidance_diagnostic = NavigationLocalAvoidanceDiagnostic::none,
        .source_index = source_index,
        .neighbor_count = 0U,
        .applied_neighbor_count = 0U,
        .dynamic_obstacle_count = 0U,
        .route_cost = 0U,
        .desired_velocity = {},
        .adjusted_velocity = {},
        .planned_state = {},
    };
}

} // namespace

NavigationCrowdPlanResult plan_navigation_navmesh_crowd(const NavigationCrowdPlanRequest& request) {
    if (request.agents.empty()) {
        return invalid_result(NavigationCrowdPlanStatus::invalid_request, NavigationCrowdPlanDiagnostic::empty_agents,
                              NavigationLocalAvoidanceAgentId{}, 0U);
    }
    if (request.max_agents > 0U && request.agents.size() > request.max_agents) {
        const std::size_t failing_source_index = request.max_agents;
        return invalid_result(NavigationCrowdPlanStatus::agent_budget_exceeded,
                              NavigationCrowdPlanDiagnostic::agent_budget_exceeded,
                              request.agents[failing_source_index].id, failing_source_index);
    }

    for (std::size_t index = 0U; index < request.agents.size(); ++index) {
        const NavigationCrowdAgentDesc& agent = request.agents[index];
        if (const auto diagnostic = validate_agent(agent); diagnostic != NavigationCrowdPlanDiagnostic::none) {
            return invalid_result(NavigationCrowdPlanStatus::invalid_agent, diagnostic, agent.id, index);
        }
        for (std::size_t previous = 0U; previous < index; ++previous) {
            if (request.agents[previous].id == agent.id) {
                return invalid_result(NavigationCrowdPlanStatus::duplicate_agent,
                                      NavigationCrowdPlanDiagnostic::duplicate_agent_id, agent.id, index);
            }
        }
    }

    const std::vector<std::size_t> sorted_indices = sorted_agent_indices(request.agents);
    NavigationCrowdPlanResult result{
        .status = NavigationCrowdPlanStatus::success,
        .diagnostic = NavigationCrowdPlanDiagnostic::none,
        .rows = {},
        .planned_agent_count = 0U,
        .route_success_count = 0U,
        .avoidance_success_count = 0U,
        .applied_neighbor_count = 0U,
        .dynamic_obstacle_count = 0U,
        .failing_agent = 0U,
        .failing_source_index = 0U,
    };
    result.rows.reserve(request.agents.size());

    for (const std::size_t source_index : sorted_indices) {
        const NavigationCrowdAgentDesc& agent = request.agents[source_index];
        NavigationCrowdAgentPlanRow row = make_base_row(agent, source_index);
        const auto route = plan_navigation_navmesh_path(NavigationNavmeshPathRequest{
            .polygons = request.polygons,
            .portals = request.portals,
            .dynamic_obstacles = request.dynamic_obstacles,
            .start = agent.start_polygon,
            .goal = agent.goal_polygon,
        });
        row.route_status = route.status;
        row.route_diagnostic = route.diagnostic;
        row.dynamic_obstacle_count = route.dynamic_obstacle_count;
        row.route_cost = route.total_cost;
        result.dynamic_obstacle_count += route.dynamic_obstacle_count;
        if (route.status != NavigationNavmeshPathStatus::success) {
            row.status = NavigationCrowdPlanStatus::route_failed;
            row.diagnostic = NavigationCrowdPlanDiagnostic::route_failed;
            set_result_failure(result, NavigationCrowdPlanStatus::route_failed,
                               NavigationCrowdPlanDiagnostic::route_failed, agent.id, source_index);
            result.rows.push_back(std::move(row));
            continue;
        }
        ++result.route_success_count;

        if (!derive_desired_velocity(agent, route, row.desired_velocity)) {
            row.status = NavigationCrowdPlanStatus::avoidance_failed;
            row.diagnostic = NavigationCrowdPlanDiagnostic::avoidance_failed;
            set_result_failure(result, NavigationCrowdPlanStatus::avoidance_failed,
                               NavigationCrowdPlanDiagnostic::avoidance_failed, agent.id, source_index);
            result.rows.push_back(std::move(row));
            continue;
        }

        NavigationAgentState planned_state =
            agent.use_existing_state ? agent.state : make_navigation_agent_state(agent.position);
        row.planned_state = replace_navigation_agent_path(std::move(planned_state), route.point_path);

        const std::vector<NavigationLocalAvoidanceNeighborDesc> neighbors =
            build_neighbors(request.agents, sorted_indices, source_index);
        row.neighbor_count = neighbors.size();
        const auto avoidance = calculate_navigation_local_avoidance(NavigationLocalAvoidanceRequest{
            .agent =
                NavigationLocalAvoidanceAgentDesc{
                    .id = agent.id,
                    .position = agent.position,
                    .desired_velocity = row.desired_velocity,
                    .radius = agent.radius,
                    .max_speed = agent.config.max_speed,
                },
            .neighbors = neighbors,
            .separation_weight = request.separation_weight,
            .prediction_time_seconds = request.prediction_time_seconds,
            .epsilon = request.epsilon,
        });
        row.avoidance_status = avoidance.status;
        row.avoidance_diagnostic = avoidance.diagnostic;
        row.adjusted_velocity = avoidance.adjusted_velocity;
        row.applied_neighbor_count = avoidance.applied_neighbor_count;
        if (avoidance.status != NavigationLocalAvoidanceStatus::success) {
            row.status = NavigationCrowdPlanStatus::avoidance_failed;
            row.diagnostic = NavigationCrowdPlanDiagnostic::avoidance_failed;
            set_result_failure(result, NavigationCrowdPlanStatus::avoidance_failed,
                               NavigationCrowdPlanDiagnostic::avoidance_failed, agent.id, source_index);
            result.rows.push_back(std::move(row));
            continue;
        }

        row.status = NavigationCrowdPlanStatus::success;
        row.diagnostic = NavigationCrowdPlanDiagnostic::none;
        ++result.planned_agent_count;
        ++result.avoidance_success_count;
        result.applied_neighbor_count += avoidance.applied_neighbor_count;
        result.rows.push_back(std::move(row));
    }

    return result;
}

} // namespace mirakana
