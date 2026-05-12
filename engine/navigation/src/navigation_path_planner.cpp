// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/navigation_path_planner.hpp"

#include <cmath>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_point(NavigationPoint2 point) noexcept {
    return finite(point.x) && finite(point.y);
}

[[nodiscard]] bool valid_mapping(NavigationGridPointMapping mapping) noexcept {
    return finite_point(mapping.origin) && finite(mapping.cell_size) && mapping.cell_size > 0.0F;
}

[[nodiscard]] bool supports_adjacency(NavigationAdjacency adjacency) noexcept {
    switch (adjacency) {
    case NavigationAdjacency::cardinal4:
        return true;
    }

    return false;
}

[[nodiscard]] NavigationGridAgentPathPlan make_failure(NavigationGridAgentPathStatus status,
                                                       NavigationGridAgentPathDiagnostic diagnostic) {
    NavigationGridAgentPathPlan result;
    result.status = status;
    result.diagnostic = diagnostic;
    return result;
}

[[nodiscard]] NavigationGridAgentPathPlan make_failure(NavigationGridAgentPathPlan result,
                                                       NavigationGridAgentPathStatus status,
                                                       NavigationGridAgentPathDiagnostic diagnostic) {
    result.status = status;
    result.diagnostic = diagnostic;
    return result;
}

[[nodiscard]] NavigationGridAgentPathStatus map_path_status(NavigationPathStatus status) noexcept {
    switch (status) {
    case NavigationPathStatus::success:
        return NavigationGridAgentPathStatus::ready;
    case NavigationPathStatus::invalid_endpoint:
        return NavigationGridAgentPathStatus::invalid_endpoint;
    case NavigationPathStatus::blocked_endpoint:
        return NavigationGridAgentPathStatus::blocked_endpoint;
    case NavigationPathStatus::no_path:
        return NavigationGridAgentPathStatus::no_path;
    }

    return NavigationGridAgentPathStatus::no_path;
}

[[nodiscard]] NavigationGridAgentPathDiagnostic map_path_diagnostic(NavigationPathStatus status) noexcept {
    switch (status) {
    case NavigationPathStatus::success:
        return NavigationGridAgentPathDiagnostic::none;
    case NavigationPathStatus::invalid_endpoint:
        return NavigationGridAgentPathDiagnostic::path_invalid_endpoint;
    case NavigationPathStatus::blocked_endpoint:
        return NavigationGridAgentPathDiagnostic::path_blocked_endpoint;
    case NavigationPathStatus::no_path:
        return NavigationGridAgentPathDiagnostic::path_not_found;
    }

    return NavigationGridAgentPathDiagnostic::path_not_found;
}

[[nodiscard]] NavigationGridAgentPathDiagnostic
map_smoothing_diagnostic(NavigationGridPathSmoothingStatus status) noexcept {
    switch (status) {
    case NavigationGridPathSmoothingStatus::success:
        return NavigationGridAgentPathDiagnostic::none;
    case NavigationGridPathSmoothingStatus::invalid_source_path:
        return NavigationGridAgentPathDiagnostic::smoothing_invalid_source_path;
    case NavigationGridPathSmoothingStatus::unsupported_adjacency:
        return NavigationGridAgentPathDiagnostic::smoothing_unsupported_adjacency;
    }

    return NavigationGridAgentPathDiagnostic::smoothing_invalid_source_path;
}

} // namespace

NavigationGridAgentPathPlan plan_navigation_grid_agent_path(const NavigationGrid& grid,
                                                            const NavigationGridAgentPathRequest& request) {
    if (!valid_mapping(request.mapping)) {
        NavigationGridAgentPathPlan result;
        result.point_path_result.status = NavigationPointPathBuildStatus::invalid_mapping;
        return make_failure(std::move(result), NavigationGridAgentPathStatus::invalid_mapping,
                            NavigationGridAgentPathDiagnostic::invalid_mapping);
    }

    if (!supports_adjacency(request.adjacency)) {
        return make_failure(NavigationGridAgentPathStatus::unsupported_adjacency,
                            NavigationGridAgentPathDiagnostic::unsupported_adjacency);
    }

    NavigationGridAgentPathPlan result;
    result.path_result = find_navigation_path(
        grid, NavigationPathRequest{.start = request.start, .goal = request.goal, .adjacency = request.adjacency});
    if (result.path_result.status != NavigationPathStatus::success) {
        const auto status = result.path_result.status;
        return make_failure(std::move(result), map_path_status(status), map_path_diagnostic(status));
    }

    result.raw_grid_point_count = result.path_result.points.size();
    result.total_cost = result.path_result.total_cost;

    if (request.smooth_path) {
        result.smoothing_result =
            smooth_navigation_grid_path(grid, NavigationGridPathSmoothingRequest{.start = request.start,
                                                                                 .goal = request.goal,
                                                                                 .path = result.path_result.points,
                                                                                 .adjacency = request.adjacency});
        result.used_smoothing = true;
    } else {
        result.smoothing_result.status = NavigationGridPathSmoothingStatus::success;
        result.smoothing_result.validation =
            validate_navigation_grid_path(grid, NavigationGridPathValidationRequest{.start = request.start,
                                                                                    .goal = request.goal,
                                                                                    .path = result.path_result.points,
                                                                                    .adjacency = request.adjacency});
        result.smoothing_result.path = result.path_result.points;
        result.used_smoothing = false;
    }

    if (result.smoothing_result.status != NavigationGridPathSmoothingStatus::success) {
        const auto status = result.smoothing_result.status;
        return make_failure(std::move(result), NavigationGridAgentPathStatus::invalid_source_path,
                            map_smoothing_diagnostic(status));
    }
    if (result.smoothing_result.validation.status == NavigationGridPathValidationStatus::unsupported_adjacency) {
        return make_failure(std::move(result), NavigationGridAgentPathStatus::unsupported_adjacency,
                            NavigationGridAgentPathDiagnostic::smoothing_unsupported_adjacency);
    }
    if (result.smoothing_result.validation.status != NavigationGridPathValidationStatus::valid) {
        return make_failure(std::move(result), NavigationGridAgentPathStatus::invalid_source_path,
                            NavigationGridAgentPathDiagnostic::smoothing_invalid_source_path);
    }

    result.planned_grid_point_count = result.smoothing_result.path.size();
    result.point_path_result = build_navigation_point_path_result(result.smoothing_result.path, request.mapping);
    if (result.point_path_result.status != NavigationPointPathBuildStatus::success) {
        return make_failure(std::move(result), NavigationGridAgentPathStatus::invalid_mapping,
                            NavigationGridAgentPathDiagnostic::point_mapping_failed);
    }

    const auto start_position =
        result.point_path_result.points.empty() ? NavigationPoint2{} : result.point_path_result.points.front();
    auto agent_state = make_navigation_agent_state(start_position);
    agent_state = replace_navigation_agent_path(std::move(agent_state), result.point_path_result.points);
    if (agent_state.status == NavigationAgentStatus::invalid_request) {
        result.agent_state = std::move(agent_state);
        return make_failure(std::move(result), NavigationGridAgentPathStatus::agent_path_invalid,
                            NavigationGridAgentPathDiagnostic::agent_path_rejected);
    }

    result.agent_state = std::move(agent_state);
    result.status = NavigationGridAgentPathStatus::ready;
    result.diagnostic = NavigationGridAgentPathDiagnostic::none;
    return result;
}

} // namespace mirakana
