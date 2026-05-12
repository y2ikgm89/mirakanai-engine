// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/path_following.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_point(NavigationPoint2 value) noexcept {
    return finite(value.x) && finite(value.y);
}

[[nodiscard]] constexpr double max_float_value() noexcept {
    return static_cast<double>(std::numeric_limits<float>::max());
}

[[nodiscard]] bool fits_float(double value) noexcept {
    return std::isfinite(value) && std::abs(value) <= max_float_value();
}

[[nodiscard]] bool make_point(double x, double y, NavigationPoint2& out) noexcept {
    if (!fits_float(x) || !fits_float(y)) {
        return false;
    }
    out = NavigationPoint2{.x = static_cast<float>(x), .y = static_cast<float>(y)};
    return finite_point(out);
}

struct SegmentMeasure {
    double dx{0.0};
    double dy{0.0};
    double distance{0.0};
};

[[nodiscard]] bool measure_segment(NavigationPoint2 from, NavigationPoint2 to, SegmentMeasure& out) noexcept {
    const auto dx = static_cast<double>(to.x) - static_cast<double>(from.x);
    const auto dy = static_cast<double>(to.y) - static_cast<double>(from.y);
    const auto distance = std::hypot(dx, dy);
    if (!fits_float(dx) || !fits_float(dy) || !fits_float(distance)) {
        return false;
    }
    out = SegmentMeasure{.dx = dx, .dy = dy, .distance = distance};
    return true;
}

[[nodiscard]] bool subtract_point(NavigationPoint2 lhs, NavigationPoint2 rhs, NavigationPoint2& out) noexcept {
    return make_point(static_cast<double>(lhs.x) - static_cast<double>(rhs.x),
                      static_cast<double>(lhs.y) - static_cast<double>(rhs.y), out);
}

[[nodiscard]] bool move_along_segment(NavigationPoint2 position, SegmentMeasure segment, double distance,
                                      NavigationPoint2& out) noexcept {
    if (segment.distance <= 0.0) {
        out = position;
        return true;
    }

    const auto ratio = distance / segment.distance;
    return make_point(static_cast<double>(position.x) + (segment.dx * ratio),
                      static_cast<double>(position.y) + (segment.dy * ratio), out);
}

[[nodiscard]] bool valid_path_points(const std::vector<NavigationPoint2>& points) noexcept {
    return std::ranges::all_of(points, [](NavigationPoint2 point) { return finite_point(point); });
}

struct DistanceResult {
    bool valid{false};
    float value{0.0F};
};

[[nodiscard]] DistanceResult remaining_path_distance(const std::vector<NavigationPoint2>& points,
                                                     NavigationPoint2 position, std::size_t target_index) noexcept {
    if (points.empty() || target_index >= points.size()) {
        return DistanceResult{.valid = true, .value = 0.0F};
    }

    SegmentMeasure segment;
    if (!measure_segment(position, points[target_index], segment)) {
        return {};
    }

    auto remaining = segment.distance;
    for (std::size_t index = target_index; index + 1U < points.size(); ++index) {
        if (!measure_segment(points[index], points[index + 1U], segment)) {
            return {};
        }
        remaining += segment.distance;
        if (!fits_float(remaining)) {
            return {};
        }
    }
    return DistanceResult{.valid = true, .value = static_cast<float>(remaining)};
}

[[nodiscard]] bool valid_follow_geometry(const NavigationPathFollowRequest& request) noexcept {
    return remaining_path_distance(request.points, request.position, request.target_index).valid;
}

[[nodiscard]] bool set_step_delta(NavigationPathFollowResult& result, NavigationPoint2 start) noexcept {
    return subtract_point(result.position, start, result.step_delta);
}

[[nodiscard]] NavigationPathFollowResult make_follow_result(NavigationPathFollowStatus status,
                                                            const NavigationPathFollowRequest& request) noexcept {
    NavigationPathFollowResult result;
    result.status = status;
    result.position = request.position;
    result.target_index = request.target_index;
    return result;
}

[[nodiscard]] NavigationPathFollowResult make_reached_result(const NavigationPathFollowRequest& request,
                                                             NavigationPoint2 position, std::size_t target_index,
                                                             double advanced_distance) noexcept {
    NavigationPathFollowResult result;
    result.status = NavigationPathFollowStatus::reached_destination;
    result.position = position;
    result.target_index = target_index;
    result.advanced_distance = static_cast<float>(advanced_distance);
    result.reached_destination = true;
    if (!set_step_delta(result, request.position)) {
        return make_follow_result(NavigationPathFollowStatus::invalid_request, request);
    }
    return result;
}

[[nodiscard]] NavigationPathFollowResult make_advanced_result(const NavigationPathFollowRequest& request,
                                                              NavigationPoint2 position, std::size_t target_index,
                                                              double advanced_distance) noexcept {
    const auto remaining = remaining_path_distance(request.points, position, target_index);
    if (!remaining.valid || !fits_float(advanced_distance)) {
        return make_follow_result(NavigationPathFollowStatus::invalid_request, request);
    }

    NavigationPathFollowResult result;
    result.status = NavigationPathFollowStatus::advanced;
    result.position = position;
    result.target_index = target_index;
    result.advanced_distance = static_cast<float>(advanced_distance);
    result.remaining_distance = remaining.value;
    if (!set_step_delta(result, request.position)) {
        return make_follow_result(NavigationPathFollowStatus::invalid_request, request);
    }
    return result;
}

} // namespace

NavigationPointPathBuildResult build_navigation_point_path_result(std::span<const NavigationGridCoord> path,
                                                                  NavigationGridPointMapping mapping) {
    NavigationPointPathBuildResult result;
    if (!finite_point(mapping.origin) || !finite(mapping.cell_size) || mapping.cell_size <= 0.0F) {
        result.status = NavigationPointPathBuildStatus::invalid_mapping;
        return result;
    }

    result.points.reserve(path.size());
    const auto offset = mapping.use_cell_centers ? 0.5 : 0.0;
    for (const auto coord : path) {
        NavigationPoint2 point;
        const auto x =
            static_cast<double>(mapping.origin.x) + ((static_cast<double>(coord.x) + offset) * mapping.cell_size);
        const auto y =
            static_cast<double>(mapping.origin.y) + ((static_cast<double>(coord.y) + offset) * mapping.cell_size);
        if (!make_point(x, y, point)) {
            result.status = NavigationPointPathBuildStatus::invalid_mapping;
            result.points.clear();
            return result;
        }
        result.points.push_back(point);
    }

    return result;
}

std::vector<NavigationPoint2> build_navigation_point_path(std::span<const NavigationGridCoord> path,
                                                          NavigationGridPointMapping mapping) {
    return build_navigation_point_path_result(path, mapping).points;
}

NavigationPathFollowResult advance_navigation_path_following(const NavigationPathFollowRequest& request) {
    if (request.points.empty()) {
        return make_follow_result(NavigationPathFollowStatus::invalid_path, request);
    }
    if (request.target_index >= request.points.size() || !finite_point(request.position) ||
        !finite(request.max_step_distance) || request.max_step_distance <= 0.0F || !finite(request.arrival_radius) ||
        request.arrival_radius < 0.0F || !valid_path_points(request.points) || !valid_follow_geometry(request)) {
        return make_follow_result(NavigationPathFollowStatus::invalid_request, request);
    }

    auto position = request.position;
    auto target_index = request.target_index;
    auto step_remaining = static_cast<double>(request.max_step_distance);
    double advanced_distance = 0.0;

    while (target_index < request.points.size()) {
        const auto target = request.points[target_index];
        SegmentMeasure to_target;
        if (!measure_segment(position, target, to_target)) {
            return make_follow_result(NavigationPathFollowStatus::invalid_request, request);
        }
        const auto target_distance = to_target.distance;

        if (target_distance <= request.arrival_radius && target_distance <= step_remaining) {
            position = target;
            advanced_distance += target_distance;
            step_remaining -= target_distance;
            if (target_index + 1U >= request.points.size()) {
                return make_reached_result(request, position, target_index, advanced_distance);
            }
            ++target_index;
            continue;
        }

        if (target_distance <= step_remaining) {
            position = target;
            advanced_distance += target_distance;
            step_remaining -= target_distance;
            if (target_index + 1U >= request.points.size()) {
                return make_reached_result(request, position, target_index, advanced_distance);
            }
            ++target_index;
            continue;
        }

        if (!move_along_segment(position, to_target, step_remaining, position)) {
            return make_follow_result(NavigationPathFollowStatus::invalid_request, request);
        }
        advanced_distance += step_remaining;
        return make_advanced_result(request, position, target_index, advanced_distance);
    }

    return make_reached_result(request, position, request.points.size() - 1U, advanced_distance);
}

NavigationArriveSteeringResult calculate_navigation_arrive_steering(const NavigationArriveSteeringRequest& request) {
    NavigationArriveSteeringResult result;
    if (!finite_point(request.position) || !finite_point(request.target) || !finite(request.max_speed) ||
        request.max_speed <= 0.0F || !finite(request.slowing_radius) || request.slowing_radius <= 0.0F ||
        !finite(request.arrival_radius) || request.arrival_radius < 0.0F) {
        result.status = NavigationArriveSteeringStatus::invalid_request;
        return result;
    }

    SegmentMeasure to_target;
    if (!measure_segment(request.position, request.target, to_target)) {
        result.status = NavigationArriveSteeringStatus::invalid_request;
        return result;
    }

    const auto target_distance = to_target.distance;
    result.distance_to_target = static_cast<float>(target_distance);
    if (target_distance <= request.arrival_radius) {
        result.status = NavigationArriveSteeringStatus::reached_target;
        result.reached_target = true;
        return result;
    }

    result.status = NavigationArriveSteeringStatus::moving;
    const auto desired_speed =
        target_distance >= request.slowing_radius
            ? static_cast<double>(request.max_speed)
            : static_cast<double>(request.max_speed) *
                  std::clamp(target_distance / static_cast<double>(request.slowing_radius), 0.0, 1.0);
    if (!fits_float(desired_speed)) {
        result.status = NavigationArriveSteeringStatus::invalid_request;
        result.distance_to_target = 0.0F;
        return result;
    }
    result.desired_speed = static_cast<float>(desired_speed);
    if (!make_point((to_target.dx / target_distance) * desired_speed, (to_target.dy / target_distance) * desired_speed,
                    result.desired_velocity)) {
        result.status = NavigationArriveSteeringStatus::invalid_request;
        result.desired_speed = 0.0F;
        result.distance_to_target = 0.0F;
        return result;
    }
    return result;
}

} // namespace mirakana
