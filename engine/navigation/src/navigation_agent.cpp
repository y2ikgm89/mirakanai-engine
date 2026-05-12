// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/navigation_agent.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_point(NavigationPoint2 point) noexcept {
    return finite(point.x) && finite(point.y);
}

[[nodiscard]] bool valid_path(const std::vector<NavigationPoint2>& path) noexcept {
    return std::ranges::all_of(path, [](NavigationPoint2 point) { return finite_point(point); });
}

[[nodiscard]] bool valid_config(NavigationAgentConfig config) noexcept {
    return finite(config.max_speed) && config.max_speed > 0.0F && finite(config.slowing_radius) &&
           config.slowing_radius > 0.0F && finite(config.arrival_radius) && config.arrival_radius >= 0.0F;
}

[[nodiscard]] bool valid_status(NavigationAgentStatus status) noexcept {
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

[[nodiscard]] bool valid_state(const NavigationAgentState& state) noexcept {
    if (!finite_point(state.position) || !valid_path(state.path) || !valid_status(state.status)) {
        return false;
    }
    if (state.path.empty()) {
        return state.target_index == 0 &&
               (state.status == NavigationAgentStatus::idle || state.status == NavigationAgentStatus::cancelled);
    }
    return state.target_index < state.path.size() && (state.status == NavigationAgentStatus::moving ||
                                                      state.status == NavigationAgentStatus::reached_destination);
}

[[nodiscard]] bool valid_step_distance(float max_speed, float delta_seconds, float& out) noexcept {
    const auto step = static_cast<double>(max_speed) * static_cast<double>(delta_seconds);
    if (!std::isfinite(step) || step <= 0.0 || step > static_cast<double>(std::numeric_limits<float>::max())) {
        return false;
    }
    out = static_cast<float>(step);
    return finite(out) && out > 0.0F;
}

struct SegmentMeasure {
    double distance{0.0};
};

struct DistanceResult {
    bool valid{false};
    float value{0.0F};
};

[[nodiscard]] bool measure_segment(NavigationPoint2 from, NavigationPoint2 to, SegmentMeasure& out) noexcept {
    const auto dx = static_cast<double>(to.x) - static_cast<double>(from.x);
    const auto dy = static_cast<double>(to.y) - static_cast<double>(from.y);
    const auto distance = std::hypot(dx, dy);
    if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(distance) ||
        std::abs(dx) > static_cast<double>(std::numeric_limits<float>::max()) ||
        std::abs(dy) > static_cast<double>(std::numeric_limits<float>::max()) ||
        distance > static_cast<double>(std::numeric_limits<float>::max())) {
        return false;
    }
    out = SegmentMeasure{distance};
    return true;
}

[[nodiscard]] DistanceResult remaining_path_distance(const NavigationAgentState& state) noexcept {
    if (state.path.empty() || state.target_index >= state.path.size()) {
        return DistanceResult{.valid = true, .value = 0.0F};
    }

    SegmentMeasure segment;
    if (!measure_segment(state.position, state.path[state.target_index], segment)) {
        return {};
    }

    auto remaining = segment.distance;
    for (std::size_t index = state.target_index; index + 1U < state.path.size(); ++index) {
        if (!measure_segment(state.path[index], state.path[index + 1U], segment)) {
            return {};
        }
        remaining += segment.distance;
        if (!std::isfinite(remaining) || remaining > static_cast<double>(std::numeric_limits<float>::max())) {
            return {};
        }
    }

    return DistanceResult{.valid = true, .value = static_cast<float>(remaining)};
}

[[nodiscard]] bool movement_speed(NavigationAgentConfig config, float remaining_distance, float& out) noexcept {
    if (!finite(remaining_distance)) {
        return false;
    }
    if (remaining_distance <= config.arrival_radius || remaining_distance >= config.slowing_radius) {
        out = config.max_speed;
        return true;
    }

    const auto speed =
        static_cast<double>(config.max_speed) *
        std::clamp(static_cast<double>(remaining_distance) / static_cast<double>(config.slowing_radius), 0.0, 1.0);
    if (!std::isfinite(speed) || speed <= 0.0 || speed > static_cast<double>(std::numeric_limits<float>::max())) {
        return false;
    }
    out = static_cast<float>(speed);
    return finite(out) && out > 0.0F;
}

[[nodiscard]] NavigationPoint2 velocity_from_delta(NavigationPoint2 step_delta, float delta_seconds) noexcept {
    return NavigationPoint2{.x = step_delta.x / delta_seconds, .y = step_delta.y / delta_seconds};
}

[[nodiscard]] NavigationAgentUpdateResult make_result(NavigationAgentStatus status, NavigationAgentState state) {
    state.status = status;
    NavigationAgentUpdateResult result;
    result.status = status;
    result.state = std::move(state);
    result.reached_destination = status == NavigationAgentStatus::reached_destination;
    return result;
}

[[nodiscard]] NavigationAgentUpdateResult make_invalid(const NavigationAgentUpdateRequest& request) {
    return make_result(NavigationAgentStatus::invalid_request, request.state);
}

} // namespace

NavigationAgentState make_navigation_agent_state(NavigationPoint2 position) {
    NavigationAgentState state;
    state.position = position;
    state.status = NavigationAgentStatus::idle;
    return state;
}

NavigationAgentState replace_navigation_agent_path(NavigationAgentState state, std::vector<NavigationPoint2> path) {
    state.path = std::move(path);
    state.target_index = 0;
    if (!valid_path(state.path)) {
        state.status = NavigationAgentStatus::invalid_request;
    } else {
        state.status = state.path.empty() ? NavigationAgentStatus::idle : NavigationAgentStatus::moving;
    }
    return state;
}

NavigationAgentState cancel_navigation_agent_move(NavigationAgentState state) {
    state.path.clear();
    state.target_index = 0;
    state.status = NavigationAgentStatus::cancelled;
    return state;
}

NavigationAgentUpdateResult update_navigation_agent(const NavigationAgentUpdateRequest& request) {
    if (!valid_config(request.config) || !valid_state(request.state) || !finite(request.delta_seconds) ||
        request.delta_seconds <= 0.0F) {
        return make_invalid(request);
    }

    if (request.state.path.empty() || request.state.status == NavigationAgentStatus::idle ||
        request.state.status == NavigationAgentStatus::cancelled ||
        request.state.status == NavigationAgentStatus::reached_destination) {
        const auto status = request.state.status == NavigationAgentStatus::cancelled
                                ? NavigationAgentStatus::cancelled
                                : (request.state.path.empty() ? NavigationAgentStatus::idle : request.state.status);
        return make_result(status, request.state);
    }

    const auto remaining_before = remaining_path_distance(request.state);
    if (!remaining_before.valid) {
        return make_invalid(request);
    }

    float speed = 0.0F;
    if (!movement_speed(request.config, remaining_before.value, speed)) {
        return make_invalid(request);
    }

    float max_step_distance = 0.0F;
    if (!valid_step_distance(speed, request.delta_seconds, max_step_distance)) {
        return make_invalid(request);
    }

    const auto follow = advance_navigation_path_following(NavigationPathFollowRequest{
        .points = request.state.path,
        .position = request.state.position,
        .target_index = request.state.target_index,
        .max_step_distance = max_step_distance,
        .arrival_radius = request.config.arrival_radius,
    });
    if (follow.status == NavigationPathFollowStatus::invalid_path ||
        follow.status == NavigationPathFollowStatus::invalid_request) {
        return make_invalid(request);
    }

    auto next_state = request.state;
    next_state.position = follow.position;
    next_state.target_index = follow.target_index;
    next_state.status =
        follow.reached_destination ? NavigationAgentStatus::reached_destination : NavigationAgentStatus::moving;

    NavigationAgentUpdateResult result;
    result.status = next_state.status;
    result.state = std::move(next_state);
    result.step_delta = follow.step_delta;
    result.step_distance = follow.advanced_distance;
    result.remaining_distance = follow.remaining_distance;
    result.reached_destination = follow.reached_destination;
    result.desired_velocity = velocity_from_delta(result.step_delta, request.delta_seconds);
    if (!finite_point(result.desired_velocity)) {
        return make_invalid(request);
    }

    return result;
}

} // namespace mirakana
