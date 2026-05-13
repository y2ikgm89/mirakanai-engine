// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/navigation/local_avoidance.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool is_finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool is_finite(NavigationPoint2 point) noexcept {
    return is_finite(point.x) && is_finite(point.y);
}

struct AvoidanceVector2 {
    double x{0.0};
    double y{0.0};
};

[[nodiscard]] constexpr double max_float_value() noexcept {
    return static_cast<double>(std::numeric_limits<float>::max());
}

[[nodiscard]] bool fits_float(double value) noexcept {
    return std::isfinite(value) && std::abs(value) <= max_float_value();
}

[[nodiscard]] bool fits_point(AvoidanceVector2 point) noexcept {
    return fits_float(point.x) && fits_float(point.y);
}

[[nodiscard]] AvoidanceVector2 to_vector(NavigationPoint2 point) noexcept {
    return AvoidanceVector2{.x = static_cast<double>(point.x), .y = static_cast<double>(point.y)};
}

[[nodiscard]] AvoidanceVector2 add(AvoidanceVector2 lhs, AvoidanceVector2 rhs) noexcept {
    return AvoidanceVector2{.x = lhs.x + rhs.x, .y = lhs.y + rhs.y};
}

[[nodiscard]] AvoidanceVector2 subtract(AvoidanceVector2 lhs, AvoidanceVector2 rhs) noexcept {
    return AvoidanceVector2{.x = lhs.x - rhs.x, .y = lhs.y - rhs.y};
}

[[nodiscard]] AvoidanceVector2 scale(AvoidanceVector2 point, double value) noexcept {
    return AvoidanceVector2{.x = point.x * value, .y = point.y * value};
}

[[nodiscard]] double dot(AvoidanceVector2 lhs, AvoidanceVector2 rhs) noexcept {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

[[nodiscard]] double length_squared(AvoidanceVector2 point) noexcept {
    return dot(point, point);
}

[[nodiscard]] double length(AvoidanceVector2 point) noexcept {
    return std::hypot(point.x, point.y);
}

[[nodiscard]] bool make_point(AvoidanceVector2 point, NavigationPoint2& out) noexcept {
    if (!fits_point(point)) {
        return false;
    }
    out = NavigationPoint2{.x = static_cast<float>(point.x), .y = static_cast<float>(point.y)};
    return is_finite(out);
}

[[nodiscard]] bool normalize_or(AvoidanceVector2 point, AvoidanceVector2 fallback, double epsilon,
                                AvoidanceVector2& out) noexcept {
    const double point_length = length(point);
    if (!fits_float(point_length)) {
        return false;
    }
    if (point_length > epsilon) {
        out = scale(point, 1.0 / point_length);
        return fits_point(out);
    }

    const double fallback_length = length(fallback);
    if (!fits_float(fallback_length)) {
        return false;
    }
    if (fallback_length > epsilon) {
        out = scale(fallback, 1.0 / fallback_length);
        return fits_point(out);
    }

    out = AvoidanceVector2{.x = 1.0, .y = 0.0};
    return true;
}

[[nodiscard]] NavigationLocalAvoidanceResult invalid_result(NavigationLocalAvoidanceDiagnostic diagnostic) noexcept {
    return NavigationLocalAvoidanceResult{
        .status = NavigationLocalAvoidanceStatus::invalid_request,
        .diagnostic = diagnostic,
        .adjusted_velocity = NavigationPoint2{},
        .applied_neighbor_count = 0U,
        .clamped_to_max_speed = false,
    };
}

[[nodiscard]] NavigationLocalAvoidanceDiagnostic
validate_agent(const NavigationLocalAvoidanceAgentDesc& agent) noexcept {
    if (agent.id == 0U) {
        return NavigationLocalAvoidanceDiagnostic::invalid_agent_id;
    }
    if (!is_finite(agent.position)) {
        return NavigationLocalAvoidanceDiagnostic::invalid_position;
    }
    if (!is_finite(agent.desired_velocity)) {
        return NavigationLocalAvoidanceDiagnostic::invalid_desired_velocity;
    }
    if (!is_finite(agent.radius) || agent.radius < 0.0F) {
        return NavigationLocalAvoidanceDiagnostic::invalid_radius;
    }
    if (!is_finite(agent.max_speed) || agent.max_speed < 0.0F) {
        return NavigationLocalAvoidanceDiagnostic::invalid_max_speed;
    }
    if (!fits_float(length(to_vector(agent.desired_velocity)))) {
        return NavigationLocalAvoidanceDiagnostic::calculation_overflow;
    }

    return NavigationLocalAvoidanceDiagnostic::none;
}

[[nodiscard]] NavigationLocalAvoidanceDiagnostic
validate_neighbor(const NavigationLocalAvoidanceNeighborDesc& neighbor) noexcept {
    if (neighbor.id == 0U) {
        return NavigationLocalAvoidanceDiagnostic::invalid_neighbor_id;
    }
    if (!is_finite(neighbor.position)) {
        return NavigationLocalAvoidanceDiagnostic::invalid_neighbor_position;
    }
    if (!is_finite(neighbor.velocity)) {
        return NavigationLocalAvoidanceDiagnostic::invalid_neighbor_velocity;
    }
    if (!is_finite(neighbor.radius) || neighbor.radius < 0.0F) {
        return NavigationLocalAvoidanceDiagnostic::invalid_neighbor_radius;
    }
    if (!fits_float(length(to_vector(neighbor.velocity)))) {
        return NavigationLocalAvoidanceDiagnostic::calculation_overflow;
    }

    return NavigationLocalAvoidanceDiagnostic::none;
}

[[nodiscard]] NavigationLocalAvoidanceDiagnostic
validate_request(const NavigationLocalAvoidanceRequest& request) noexcept {
    if (const auto agent_diagnostic = validate_agent(request.agent);
        agent_diagnostic != NavigationLocalAvoidanceDiagnostic::none) {
        return agent_diagnostic;
    }
    if (!is_finite(request.separation_weight) || request.separation_weight < 0.0F) {
        return NavigationLocalAvoidanceDiagnostic::invalid_separation_weight;
    }
    if (!is_finite(request.prediction_time_seconds) || request.prediction_time_seconds < 0.0F) {
        return NavigationLocalAvoidanceDiagnostic::invalid_prediction_time_seconds;
    }
    if (!is_finite(request.epsilon) || request.epsilon <= 0.0F) {
        return NavigationLocalAvoidanceDiagnostic::invalid_epsilon;
    }

    for (const NavigationLocalAvoidanceNeighborDesc& neighbor : request.neighbors) {
        if (const auto neighbor_diagnostic = validate_neighbor(neighbor);
            neighbor_diagnostic != NavigationLocalAvoidanceDiagnostic::none) {
            return neighbor_diagnostic;
        }
    }
    for (std::size_t outer = 0U; outer < request.neighbors.size(); ++outer) {
        for (std::size_t inner = outer + 1U; inner < request.neighbors.size(); ++inner) {
            if (request.neighbors[outer].id == request.neighbors[inner].id) {
                return NavigationLocalAvoidanceDiagnostic::duplicate_neighbor_id;
            }
        }
    }

    return NavigationLocalAvoidanceDiagnostic::none;
}

[[nodiscard]] std::vector<std::size_t>
sorted_neighbor_indices(std::span<const NavigationLocalAvoidanceNeighborDesc> neighbors) {
    std::vector<std::size_t> indices(neighbors.size());
    // NOLINTNEXTLINE(modernize-use-ranges): hosted Clang/AppleClang CI lacks std::ranges::iota.
    std::iota(indices.begin(), indices.end(), std::size_t{0});
    std::ranges::stable_sort(indices, [&neighbors](std::size_t lhs, std::size_t rhs) {
        if (neighbors[lhs].id == neighbors[rhs].id) {
            return lhs < rhs;
        }
        return neighbors[lhs].id < neighbors[rhs].id;
    });
    return indices;
}

} // namespace

NavigationLocalAvoidanceResult calculate_navigation_local_avoidance(const NavigationLocalAvoidanceRequest& request) {
    if (const auto diagnostic = validate_request(request); diagnostic != NavigationLocalAvoidanceDiagnostic::none) {
        return invalid_result(diagnostic);
    }

    const NavigationLocalAvoidanceAgentDesc& agent = request.agent;
    AvoidanceVector2 adjusted_velocity = to_vector(agent.desired_velocity);
    const double desired_speed = length(to_vector(agent.desired_velocity));
    if (!fits_float(desired_speed)) {
        return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
    }
    std::size_t applied_neighbor_count = 0U;

    const auto prediction_time = static_cast<double>(request.prediction_time_seconds);
    const auto epsilon = static_cast<double>(request.epsilon);
    const double base_speed = std::max(static_cast<double>(agent.max_speed), desired_speed);
    if (!fits_float(base_speed)) {
        return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
    }

    for (const std::size_t index : sorted_neighbor_indices(request.neighbors)) {
        const NavigationLocalAvoidanceNeighborDesc& neighbor = request.neighbors[index];
        if (neighbor.id == agent.id) {
            continue;
        }

        const double influence_radius =
            static_cast<double>(agent.radius) + static_cast<double>(neighbor.radius) + epsilon;
        if (!fits_float(influence_radius)) {
            return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
        }

        const AvoidanceVector2 current_offset = subtract(to_vector(agent.position), to_vector(neighbor.position));
        const AvoidanceVector2 relative_velocity =
            subtract(to_vector(agent.desired_velocity), to_vector(neighbor.velocity));
        if (!fits_point(current_offset) || !fits_point(relative_velocity)) {
            return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
        }

        const double relative_speed_squared = length_squared(relative_velocity);
        if (!fits_float(relative_speed_squared)) {
            return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
        }

        double closest_time = 0.0;
        if (relative_speed_squared > epsilon * epsilon) {
            closest_time =
                std::clamp(-dot(current_offset, relative_velocity) / relative_speed_squared, 0.0, prediction_time);
            if (!fits_float(closest_time)) {
                return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
            }
        }

        const AvoidanceVector2 closest_offset = add(current_offset, scale(relative_velocity, closest_time));
        if (!fits_point(closest_offset)) {
            return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
        }

        const double closest_distance = length(closest_offset);
        if (!fits_float(closest_distance)) {
            return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
        }
        if (closest_distance > influence_radius) {
            continue;
        }

        AvoidanceVector2 separation_direction;
        if (!normalize_or(closest_offset, current_offset, epsilon, separation_direction) &&
            !normalize_or(closest_offset, scale(to_vector(agent.desired_velocity), -1.0), epsilon,
                          separation_direction)) {
            return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
        }

        if (length(closest_offset) <= epsilon) {
            const AvoidanceVector2 fallback = scale(to_vector(agent.desired_velocity), -1.0);
            if (!normalize_or(current_offset, fallback, epsilon, separation_direction)) {
                return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
            }
        }

        const double normalized_penetration =
            std::clamp((influence_radius - closest_distance) / influence_radius, 0.0, 1.0);
        const double separation_amount =
            static_cast<double>(request.separation_weight) * normalized_penetration * base_speed;
        if (!fits_float(separation_amount)) {
            return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
        }

        adjusted_velocity = add(adjusted_velocity, scale(separation_direction, separation_amount));
        if (!fits_point(adjusted_velocity)) {
            return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
        }
        ++applied_neighbor_count;
    }

    bool clamped_to_max_speed = false;
    const double adjusted_speed = length(adjusted_velocity);
    if (!fits_float(adjusted_speed)) {
        return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
    }
    if (agent.max_speed == 0.0F) {
        clamped_to_max_speed = adjusted_speed > epsilon;
        adjusted_velocity = AvoidanceVector2{};
    } else if (adjusted_speed > static_cast<double>(agent.max_speed)) {
        adjusted_velocity = scale(adjusted_velocity, static_cast<double>(agent.max_speed) / adjusted_speed);
        clamped_to_max_speed = true;
    }

    NavigationPoint2 adjusted_point;
    if (!make_point(adjusted_velocity, adjusted_point)) {
        return invalid_result(NavigationLocalAvoidanceDiagnostic::calculation_overflow);
    }

    return NavigationLocalAvoidanceResult{
        .status = NavigationLocalAvoidanceStatus::success,
        .diagnostic = NavigationLocalAvoidanceDiagnostic::none,
        .adjusted_velocity = adjusted_point,
        .applied_neighbor_count = applied_neighbor_count,
        .clamped_to_max_speed = clamped_to_max_speed,
    };
}

} // namespace mirakana
