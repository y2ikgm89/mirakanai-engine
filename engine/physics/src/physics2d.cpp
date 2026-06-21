// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/physics/physics2d.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_vec(Vec2 value) noexcept {
    return finite(value.x) && finite(value.y);
}

[[nodiscard]] float inverse_mass_for(const PhysicsBody2DDesc& desc) noexcept {
    return desc.dynamic ? 1.0F / desc.mass : 0.0F;
}

[[nodiscard]] bool valid_shape(PhysicsShape2DKind shape) noexcept {
    switch (shape) {
    case PhysicsShape2DKind::aabb:
    case PhysicsShape2DKind::circle:
        return true;
    }
    return false;
}

struct PhysicsBodyBounds2D {
    Vec2 minimum;
    Vec2 maximum;
};

[[nodiscard]] float clamp_value(float value, float minimum, float maximum) noexcept {
    return std::max(minimum, std::min(value, maximum));
}

[[nodiscard]] Vec2 bounds_extents_for(const PhysicsBody2D& body) noexcept {
    if (body.shape == PhysicsShape2DKind::circle) {
        return Vec2{.x = body.radius, .y = body.radius};
    }
    return body.half_extents;
}

[[nodiscard]] PhysicsBodyBounds2D bounds_for(const PhysicsBody2D& body) noexcept {
    const auto extents = bounds_extents_for(body);
    return PhysicsBodyBounds2D{
        .minimum = body.position - extents,
        .maximum = body.position + extents,
    };
}

[[nodiscard]] Vec2 bounds_extents_for(PhysicsShape2DKind shape, Vec2 half_extents, float radius) noexcept {
    if (shape == PhysicsShape2DKind::circle) {
        return Vec2{.x = radius, .y = radius};
    }
    return half_extents;
}

[[nodiscard]] PhysicsBodyBounds2D bounds_for(Vec2 center, Vec2 extents) noexcept {
    return PhysicsBodyBounds2D{
        .minimum = center - extents,
        .maximum = center + extents,
    };
}

[[nodiscard]] PhysicsBodyBounds2D expand_bounds(const PhysicsBodyBounds2D& bounds, Vec2 extents) noexcept {
    return PhysicsBodyBounds2D{
        .minimum = bounds.minimum - extents,
        .maximum = bounds.maximum + extents,
    };
}

[[nodiscard]] bool bounds_overlap(const PhysicsBodyBounds2D& first, const PhysicsBodyBounds2D& second) noexcept {
    return first.minimum.x <= second.maximum.x && first.maximum.x >= second.minimum.x &&
           first.minimum.y <= second.maximum.y && first.maximum.y >= second.minimum.y;
}

[[nodiscard]] bool collision_filters_match(const PhysicsBody2D& first, const PhysicsBody2D& second) noexcept {
    return (first.collision_mask & second.collision_layer) != 0U &&
           (second.collision_mask & first.collision_layer) != 0U;
}

struct BoundsRaycast2D {
    Vec2 point;
    Vec2 normal;
    float distance{0.0F};
};

[[nodiscard]] bool update_ray_interval(float origin, float direction, float minimum, float maximum, Vec2 min_normal,
                                       Vec2 max_normal, float& entry_distance, float& exit_distance,
                                       Vec2& entry_normal) noexcept {
    constexpr auto parallel_epsilon = 0.000001F;
    if (std::fabs(direction) <= parallel_epsilon) {
        return origin >= minimum && origin <= maximum;
    }

    float near_distance = 0.0F;
    float far_distance = 0.0F;
    Vec2 near_normal{};
    if (direction > 0.0F) {
        near_distance = (minimum - origin) / direction;
        far_distance = (maximum - origin) / direction;
        near_normal = min_normal;
    } else {
        near_distance = (maximum - origin) / direction;
        far_distance = (minimum - origin) / direction;
        near_normal = max_normal;
    }

    if (near_distance > entry_distance) {
        entry_distance = near_distance;
        entry_normal = near_normal;
    }
    exit_distance = std::min(exit_distance, far_distance);
    return entry_distance <= exit_distance;
}

[[nodiscard]] std::optional<BoundsRaycast2D> raycast_bounds(Vec2 origin, Vec2 direction, float max_distance,
                                                            const PhysicsBodyBounds2D& bounds) noexcept {
    float entry_distance = 0.0F;
    auto exit_distance = max_distance;
    Vec2 entry_normal{.x = direction.x * -1.0F, .y = direction.y * -1.0F};

    if (!update_ray_interval(origin.x, direction.x, bounds.minimum.x, bounds.maximum.x, Vec2{.x = -1.0F, .y = 0.0F},
                             Vec2{.x = 1.0F, .y = 0.0F}, entry_distance, exit_distance, entry_normal)) {
        return std::nullopt;
    }
    if (!update_ray_interval(origin.y, direction.y, bounds.minimum.y, bounds.maximum.y, Vec2{.x = 0.0F, .y = -1.0F},
                             Vec2{.x = 0.0F, .y = 1.0F}, entry_distance, exit_distance, entry_normal)) {
        return std::nullopt;
    }
    if (entry_distance < 0.0F || entry_distance > max_distance) {
        return std::nullopt;
    }

    return BoundsRaycast2D{
        .point = origin + direction * entry_distance, .normal = entry_normal, .distance = entry_distance};
}

[[nodiscard]] bool is_valid_raycast_desc(const PhysicsRaycast2DDesc& desc) noexcept {
    return finite_vec(desc.origin) && finite_vec(desc.direction) && finite(desc.max_distance) &&
           desc.max_distance >= 0.0F && length(desc.direction) > 0.000001F;
}

[[nodiscard]] Vec2 fallback_normal(Vec2 delta) noexcept {
    if (std::fabs(delta.x) >= std::fabs(delta.y)) {
        return Vec2{.x = delta.x >= 0.0F ? 1.0F : -1.0F, .y = 0.0F};
    }
    return Vec2{.x = 0.0F, .y = delta.y >= 0.0F ? 1.0F : -1.0F};
}

[[nodiscard]] std::optional<PhysicsContact2D> aabb_contact(const PhysicsBody2D& first, const PhysicsBody2D& second) {
    const auto delta = second.position - first.position;
    const auto overlap_x = (first.half_extents.x + second.half_extents.x) - std::fabs(delta.x);
    const auto overlap_y = (first.half_extents.y + second.half_extents.y) - std::fabs(delta.y);
    if (overlap_x < 0.0F || overlap_y < 0.0F) {
        return std::nullopt;
    }

    if (overlap_x <= overlap_y) {
        return PhysicsContact2D{.first = first.id,
                                .second = second.id,
                                .normal = Vec2{.x = delta.x >= 0.0F ? 1.0F : -1.0F, .y = 0.0F},
                                .penetration_depth = overlap_x};
    }
    return PhysicsContact2D{.first = first.id,
                            .second = second.id,
                            .normal = Vec2{.x = 0.0F, .y = delta.y >= 0.0F ? 1.0F : -1.0F},
                            .penetration_depth = overlap_y};
}

[[nodiscard]] std::optional<PhysicsContact2D> circle_contact(const PhysicsBody2D& first, const PhysicsBody2D& second) {
    const auto delta = second.position - first.position;
    const auto radius_sum = first.radius + second.radius;
    const auto distance_squared = dot(delta, delta);
    if (distance_squared > radius_sum * radius_sum) {
        return std::nullopt;
    }

    const auto distance = std::sqrt(distance_squared);
    const auto normal = distance > 0.000001F ? delta * (1.0F / distance) : Vec2{.x = 1.0F, .y = 0.0F};
    return PhysicsContact2D{
        .first = first.id, .second = second.id, .normal = normal, .penetration_depth = radius_sum - distance};
}

[[nodiscard]] std::optional<PhysicsContact2D> circle_aabb_contact(const PhysicsBody2D& circle,
                                                                  const PhysicsBody2D& aabb, bool circle_is_first) {
    const auto bounds = bounds_for(aabb);
    const auto closest = Vec2{
        .x = clamp_value(circle.position.x, bounds.minimum.x, bounds.maximum.x),
        .y = clamp_value(circle.position.y, bounds.minimum.y, bounds.maximum.y),
    };
    auto delta = closest - circle.position;
    const auto distance_squared = dot(delta, delta);

    if (distance_squared > circle.radius * circle.radius) {
        return std::nullopt;
    }

    struct CircleAabbPenetration {
        Vec2 normal_from_circle_to_aabb;
        float depth;
    };
    const auto penetration = [&]() -> CircleAabbPenetration {
        if (distance_squared > 0.000001F) {
            const auto distance = std::sqrt(distance_squared);
            return CircleAabbPenetration{
                .normal_from_circle_to_aabb = delta * (1.0F / distance),
                .depth = circle.radius - distance,
            };
        }
        const auto local = circle.position - aabb.position;
        const auto distance_to_x = aabb.half_extents.x - std::fabs(local.x);
        const auto distance_to_y = aabb.half_extents.y - std::fabs(local.y);
        return CircleAabbPenetration{
            .normal_from_circle_to_aabb = fallback_normal(local),
            .depth = circle.radius + std::min(distance_to_x, distance_to_y),
        };
    }();

    if (circle_is_first) {
        return PhysicsContact2D{.first = circle.id,
                                .second = aabb.id,
                                .normal = penetration.normal_from_circle_to_aabb,
                                .penetration_depth = penetration.depth};
    }
    return PhysicsContact2D{.first = aabb.id,
                            .second = circle.id,
                            .normal = penetration.normal_from_circle_to_aabb * -1.0F,
                            .penetration_depth = penetration.depth};
}

[[nodiscard]] std::optional<PhysicsContact2D> contact_for(const PhysicsBody2D& first, const PhysicsBody2D& second) {
    if (first.shape == PhysicsShape2DKind::aabb && second.shape == PhysicsShape2DKind::aabb) {
        return aabb_contact(first, second);
    }
    if (first.shape == PhysicsShape2DKind::circle && second.shape == PhysicsShape2DKind::circle) {
        return circle_contact(first, second);
    }
    if (first.shape == PhysicsShape2DKind::circle && second.shape == PhysicsShape2DKind::aabb) {
        return circle_aabb_contact(first, second, true);
    }
    if (first.shape == PhysicsShape2DKind::aabb && second.shape == PhysicsShape2DKind::circle) {
        return circle_aabb_contact(second, first, false);
    }
    return std::nullopt;
}

[[nodiscard]] bool is_valid_shape_sweep_desc(const PhysicsShapeSweep2DDesc& desc) noexcept {
    if (!finite_vec(desc.origin) || !finite_vec(desc.direction) || !finite(desc.max_distance) ||
        desc.max_distance < 0.0F || !valid_shape(desc.shape)) {
        return false;
    }
    if (desc.shape == PhysicsShape2DKind::circle) {
        return finite(desc.radius) && desc.radius > 0.0F;
    }
    return finite_vec(desc.half_extents) && desc.half_extents.x > 0.0F && desc.half_extents.y > 0.0F;
}

[[nodiscard]] Vec2 normalized_or_zero(Vec2 value) noexcept {
    const auto value_length = length(value);
    if (value_length <= 0.000001F) {
        return Vec2{.x = 0.0F, .y = 0.0F};
    }
    return value * (1.0F / value_length);
}

struct ExactSweepHit2D {
    PhysicsBody2DId body;
    Vec2 position{.x = 0.0F, .y = 0.0F};
    Vec2 normal{.x = 1.0F, .y = 0.0F};
    float distance{0.0F};
    bool initial_overlap{false};
};

[[nodiscard]] PhysicsBody2D query_body_for(Vec2 position, PhysicsShape2DKind shape, Vec2 half_extents,
                                           float radius) noexcept {
    return PhysicsBody2D{
        .id = null_physics_body_2d,
        .position = position,
        .velocity = Vec2{.x = 0.0F, .y = 0.0F},
        .accumulated_force = Vec2{.x = 0.0F, .y = 0.0F},
        .inverse_mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = half_extents,
        .collision_enabled = true,
        .shape = shape,
        .radius = radius,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .trigger = false,
    };
}

[[nodiscard]] bool shape_overlaps_body(Vec2 position, PhysicsShape2DKind shape, Vec2 half_extents, float radius,
                                       const PhysicsBody2D& body) {
    const auto query = query_body_for(position, shape, half_extents, radius);
    return contact_for(query, body).has_value();
}

[[nodiscard]] std::optional<BoundsRaycast2D> raycast_circle(Vec2 origin, Vec2 direction, float max_distance,
                                                            Vec2 center, float radius) noexcept {
    const auto origin_to_center = origin - center;
    const auto c = dot(origin_to_center, origin_to_center) - (radius * radius);
    if (c <= 0.0F) {
        return BoundsRaycast2D{
            .point = origin,
            .normal = normalized_or_zero(origin - center),
            .distance = 0.0F,
        };
    }

    const auto b = dot(origin_to_center, direction);
    if (b > 0.0F) {
        return std::nullopt;
    }

    const auto discriminant = (b * b) - c;
    if (discriminant < 0.0F) {
        return std::nullopt;
    }

    const auto distance = -b - std::sqrt(discriminant);
    if (distance < 0.0F || distance > max_distance) {
        return std::nullopt;
    }
    const auto point = origin + (direction * distance);
    return BoundsRaycast2D{.point = point, .normal = normalized_or_zero(point - center), .distance = distance};
}

[[nodiscard]] std::optional<BoundsRaycast2D> choose_nearest(std::optional<BoundsRaycast2D> current,
                                                            BoundsRaycast2D candidate) noexcept {
    if (!current.has_value() || candidate.distance < current->distance) {
        return candidate;
    }
    return current;
}

[[nodiscard]] std::optional<BoundsRaycast2D> raycast_rounded_rect(Vec2 origin, Vec2 direction, float max_distance,
                                                                  const PhysicsBodyBounds2D& core,
                                                                  float radius) noexcept {
    if (radius <= 0.0F) {
        return raycast_bounds(origin, direction, max_distance, core);
    }

    std::optional<BoundsRaycast2D> closest;
    auto add_side = [&](float distance, Vec2 normal, bool inside_span) {
        if (distance < 0.0F || distance > max_distance || !inside_span) {
            return;
        }
        closest = choose_nearest(
            closest, BoundsRaycast2D{.point = origin + (direction * distance), .normal = normal, .distance = distance});
    };

    if (std::fabs(direction.x) > 0.000001F) {
        const auto left_distance = ((core.minimum.x - radius) - origin.x) / direction.x;
        const auto left_y = origin.y + (direction.y * left_distance);
        add_side(left_distance, Vec2{.x = -1.0F, .y = 0.0F}, left_y >= core.minimum.y && left_y <= core.maximum.y);

        const auto right_distance = ((core.maximum.x + radius) - origin.x) / direction.x;
        const auto right_y = origin.y + (direction.y * right_distance);
        add_side(right_distance, Vec2{.x = 1.0F, .y = 0.0F}, right_y >= core.minimum.y && right_y <= core.maximum.y);
    }

    if (std::fabs(direction.y) > 0.000001F) {
        const auto bottom_distance = ((core.minimum.y - radius) - origin.y) / direction.y;
        const auto bottom_x = origin.x + (direction.x * bottom_distance);
        add_side(bottom_distance, Vec2{.x = 0.0F, .y = -1.0F},
                 bottom_x >= core.minimum.x && bottom_x <= core.maximum.x);

        const auto top_distance = ((core.maximum.y + radius) - origin.y) / direction.y;
        const auto top_x = origin.x + (direction.x * top_distance);
        add_side(top_distance, Vec2{.x = 0.0F, .y = 1.0F}, top_x >= core.minimum.x && top_x <= core.maximum.x);
    }

    const Vec2 corners[] = {
        Vec2{.x = core.minimum.x, .y = core.minimum.y},
        Vec2{.x = core.minimum.x, .y = core.maximum.y},
        Vec2{.x = core.maximum.x, .y = core.minimum.y},
        Vec2{.x = core.maximum.x, .y = core.maximum.y},
    };
    for (const auto corner : corners) {
        if (auto hit = raycast_circle(origin, direction, max_distance, corner, radius); hit.has_value()) {
            closest = choose_nearest(closest, *hit);
        }
    }
    return closest;
}

[[nodiscard]] std::optional<ExactSweepHit2D> exact_sweep_body(Vec2 origin, Vec2 displacement, PhysicsShape2DKind shape,
                                                              Vec2 half_extents, float radius,
                                                              const PhysicsBody2D& target) {
    if (shape_overlaps_body(origin, shape, half_extents, radius, target)) {
        return ExactSweepHit2D{
            .body = target.id,
            .position = origin,
            .normal = fallback_normal(origin - target.position),
            .distance = 0.0F,
            .initial_overlap = true,
        };
    }

    const auto max_distance = length(displacement);
    if (max_distance <= 0.000001F) {
        return std::nullopt;
    }
    const auto direction = displacement * (1.0F / max_distance);
    std::optional<BoundsRaycast2D> hit;
    if (shape == PhysicsShape2DKind::circle && target.shape == PhysicsShape2DKind::circle) {
        hit = raycast_circle(origin, direction, max_distance, target.position, radius + target.radius);
    } else if (shape == PhysicsShape2DKind::circle && target.shape == PhysicsShape2DKind::aabb) {
        hit = raycast_rounded_rect(origin, direction, max_distance, bounds_for(target), radius);
    } else if (shape == PhysicsShape2DKind::aabb && target.shape == PhysicsShape2DKind::circle) {
        hit = raycast_rounded_rect(origin, direction, max_distance, bounds_for(target.position, half_extents),
                                   target.radius);
    } else {
        hit = raycast_bounds(origin, direction, max_distance, expand_bounds(bounds_for(target), half_extents));
    }

    if (!hit.has_value()) {
        return std::nullopt;
    }
    return ExactSweepHit2D{
        .body = target.id,
        .position = hit->point,
        .normal = hit->normal,
        .distance = hit->distance,
        .initial_overlap = false,
    };
}

[[nodiscard]] bool target_matches_runtime_filter(const PhysicsBody2D& source, const PhysicsBody2D& target,
                                                 std::uint32_t collision_mask, bool include_triggers,
                                                 bool static_only) noexcept {
    if (!target.collision_enabled || target.id == source.id || (collision_mask & target.collision_layer) == 0U ||
        !collision_filters_match(source, target) || (!include_triggers && target.trigger)) {
        return false;
    }
    return !static_only || !target.dynamic;
}

[[nodiscard]] std::optional<ExactSweepHit2D> closest_exact_sweep(const std::vector<PhysicsBody2D>& bodies,
                                                                 const PhysicsBody2D& source, Vec2 origin,
                                                                 Vec2 displacement, std::uint32_t collision_mask,
                                                                 bool include_triggers, bool static_only) {
    std::optional<ExactSweepHit2D> closest;
    for (const auto& target : bodies) {
        if (!target_matches_runtime_filter(source, target, collision_mask, include_triggers, static_only)) {
            continue;
        }
        const auto hit =
            exact_sweep_body(origin, displacement, source.shape, source.half_extents, source.radius, target);
        if (!hit.has_value()) {
            continue;
        }
        if (!closest.has_value() || hit->distance < closest->distance ||
            (hit->distance == closest->distance && hit->body.value < closest->body.value)) {
            closest = *hit;
        }
    }
    return closest;
}

[[nodiscard]] Physics2DSimulateStepResult invalid_simulate_result(Physics2DRuntimeDiagnostic diagnostic) {
    return Physics2DSimulateStepResult{
        .status = Physics2DSimulateStepStatus::invalid_request,
        .diagnostic = diagnostic,
    };
}

[[nodiscard]] bool valid_runtime_request(const Physics2DContinuousCollisionRequest& request) noexcept {
    return finite(request.delta_seconds) && request.delta_seconds >= 0.0F && finite(request.skin_width) &&
           request.skin_width >= 0.0F;
}

[[nodiscard]] std::size_t dynamic_body_count(const PhysicsWorld2D& world) noexcept {
    std::size_t count = 0;
    for (const auto& body : world.bodies()) {
        if (body.dynamic && body.collision_enabled) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] PhysicsTriggerOverlap2D normalize_overlap(PhysicsTriggerOverlap2D overlap) noexcept {
    if (overlap.second.value < overlap.first.value) {
        std::swap(overlap.first, overlap.second);
    }
    return overlap;
}

[[nodiscard]] bool same_overlap(PhysicsTriggerOverlap2D lhs, PhysicsTriggerOverlap2D rhs) noexcept {
    lhs = normalize_overlap(lhs);
    rhs = normalize_overlap(rhs);
    return lhs.first == rhs.first && lhs.second == rhs.second;
}

[[nodiscard]] bool contains_overlap(const std::vector<PhysicsTriggerOverlap2D>& overlaps,
                                    PhysicsTriggerOverlap2D query) noexcept {
    return std::ranges::any_of(overlaps,
                               [query](PhysicsTriggerOverlap2D overlap) { return same_overlap(overlap, query); });
}

[[nodiscard]] Physics2DAreaTriggerEventRow make_trigger_event_row(const PhysicsWorld2D& world, std::size_t source_index,
                                                                  Physics2DAreaTriggerEventKind kind,
                                                                  PhysicsTriggerOverlap2D overlap) {
    overlap = normalize_overlap(overlap);
    const auto* first = world.find_body(overlap.first);
    const auto* second = world.find_body(overlap.second);
    if (first != nullptr && first->trigger) {
        return Physics2DAreaTriggerEventRow{
            .source_index = source_index, .kind = kind, .trigger_body = first->id, .other_body = overlap.second};
    }
    if (second != nullptr && second->trigger) {
        return Physics2DAreaTriggerEventRow{
            .source_index = source_index, .kind = kind, .trigger_body = second->id, .other_body = overlap.first};
    }
    return Physics2DAreaTriggerEventRow{
        .source_index = source_index, .kind = kind, .trigger_body = overlap.first, .other_body = overlap.second};
}

[[nodiscard]] std::vector<Physics2DAreaTriggerEventRow>
collect_trigger_event_rows(const PhysicsWorld2D& world, const std::vector<PhysicsTriggerOverlap2D>& previous_overlaps) {
    auto current_overlaps = world.trigger_overlaps();
    for (auto& overlap : current_overlaps) {
        overlap = normalize_overlap(overlap);
    }

    std::vector<Physics2DAreaTriggerEventRow> rows;
    for (const auto overlap : current_overlaps) {
        const auto kind = contains_overlap(previous_overlaps, overlap) ? Physics2DAreaTriggerEventKind::stay
                                                                       : Physics2DAreaTriggerEventKind::enter;
        rows.push_back(make_trigger_event_row(world, rows.size(), kind, overlap));
    }
    for (auto overlap : previous_overlaps) {
        overlap = normalize_overlap(overlap);
        if (!contains_overlap(current_overlaps, overlap)) {
            rows.push_back(make_trigger_event_row(world, rows.size(), Physics2DAreaTriggerEventKind::exit, overlap));
        }
    }
    return rows;
}

} // namespace

bool is_valid_physics_body_desc(const PhysicsBody2DDesc& desc) noexcept {
    if (!finite_vec(desc.position) || !finite_vec(desc.velocity) || !finite(desc.mass) ||
        !finite(desc.linear_damping) || desc.linear_damping < 0.0F || !finite_vec(desc.half_extents) ||
        desc.half_extents.x <= 0.0F || desc.half_extents.y <= 0.0F || !valid_shape(desc.shape) ||
        !finite(desc.radius) || desc.radius <= 0.0F || desc.collision_layer == 0U) {
        return false;
    }
    return desc.dynamic ? desc.mass > 0.0F : desc.mass >= 0.0F;
}

PhysicsWorld2D::PhysicsWorld2D(PhysicsWorld2DConfig config) : config_(config) {
    if (!finite_vec(config_.gravity)) {
        throw std::invalid_argument("physics world gravity is invalid");
    }
}

PhysicsBody2DId PhysicsWorld2D::create_body(const PhysicsBody2DDesc& desc) {
    if (!is_valid_physics_body_desc(desc)) {
        throw std::invalid_argument("physics body description is invalid");
    }

    const auto id = PhysicsBody2DId{static_cast<std::uint32_t>(bodies_.size() + 1U)};
    bodies_.push_back(PhysicsBody2D{
        .id = id,
        .position = desc.position,
        .velocity = desc.velocity,
        .accumulated_force = Vec2{.x = 0.0F, .y = 0.0F},
        .inverse_mass = inverse_mass_for(desc),
        .linear_damping = desc.linear_damping,
        .dynamic = desc.dynamic,
        .half_extents = desc.half_extents,
        .collision_enabled = desc.collision_enabled,
        .shape = desc.shape,
        .radius = desc.radius,
        .collision_layer = desc.collision_layer,
        .collision_mask = desc.collision_mask,
        .trigger = desc.trigger,
    });
    return id;
}

PhysicsWorld2DConfig PhysicsWorld2D::config() const noexcept {
    return config_;
}

PhysicsBody2D* PhysicsWorld2D::find_body(PhysicsBody2DId id) noexcept {
    if (id == null_physics_body_2d || id.value > bodies_.size()) {
        return nullptr;
    }
    return &bodies_[id.value - 1U];
}

const PhysicsBody2D* PhysicsWorld2D::find_body(PhysicsBody2DId id) const noexcept {
    if (id == null_physics_body_2d || id.value > bodies_.size()) {
        return nullptr;
    }
    return &bodies_[id.value - 1U];
}

const std::vector<PhysicsBody2D>& PhysicsWorld2D::bodies() const noexcept {
    return bodies_;
}

std::vector<PhysicsBroadphasePair2D> PhysicsWorld2D::broadphase_pairs() const {
    struct Candidate {
        std::size_t body_index{0};
        PhysicsBodyBounds2D bounds{};
    };

    std::vector<Candidate> candidates;
    candidates.reserve(bodies_.size());

    for (std::size_t index = 0; index < bodies_.size(); ++index) {
        if (!bodies_[index].collision_enabled) {
            continue;
        }
        candidates.push_back(Candidate{.body_index = index, .bounds = bounds_for(bodies_[index])});
    }

    std::ranges::sort(candidates, [this](const Candidate& lhs, const Candidate& rhs) {
        if (lhs.bounds.minimum.x != rhs.bounds.minimum.x) {
            return lhs.bounds.minimum.x < rhs.bounds.minimum.x;
        }
        return bodies_[lhs.body_index].id.value < bodies_[rhs.body_index].id.value;
    });

    std::vector<PhysicsBroadphasePair2D> result;
    for (std::size_t first_index = 0; first_index < candidates.size(); ++first_index) {
        const auto& first_candidate = candidates[first_index];
        const auto& first = bodies_[first_candidate.body_index];
        for (std::size_t second_index = first_index + 1U; second_index < candidates.size(); ++second_index) {
            const auto& second_candidate = candidates[second_index];
            if (second_candidate.bounds.minimum.x > first_candidate.bounds.maximum.x) {
                break;
            }

            const auto& second = bodies_[second_candidate.body_index];
            if (!collision_filters_match(first, second) ||
                !bounds_overlap(first_candidate.bounds, second_candidate.bounds)) {
                continue;
            }

            if (first_candidate.body_index < second_candidate.body_index) {
                result.push_back(PhysicsBroadphasePair2D{.first = first.id, .second = second.id});
            } else {
                result.push_back(PhysicsBroadphasePair2D{.first = second.id, .second = first.id});
            }
        }
    }

    std::ranges::sort(result, [](const PhysicsBroadphasePair2D& lhs, const PhysicsBroadphasePair2D& rhs) {
        if (lhs.first.value != rhs.first.value) {
            return lhs.first.value < rhs.first.value;
        }
        return lhs.second.value < rhs.second.value;
    });
    const auto unique_end =
        std::ranges::unique(result, [](const PhysicsBroadphasePair2D& lhs, const PhysicsBroadphasePair2D& rhs) {
            return lhs.first == rhs.first && lhs.second == rhs.second;
        });
    result.erase(unique_end.begin(), result.end());
    return result;
}

std::vector<PhysicsContact2D> PhysicsWorld2D::contacts() const {
    std::vector<PhysicsContact2D> result;

    for (const auto& pair : broadphase_pairs()) {
        const auto* first = find_body(pair.first);
        const auto* second = find_body(pair.second);
        if (first == nullptr || second == nullptr) {
            continue;
        }
        if (first->trigger || second->trigger) {
            continue;
        }
        if (const auto contact = contact_for(*first, *second); contact.has_value()) {
            result.push_back(*contact);
        }
    }

    return result;
}

std::vector<PhysicsTriggerOverlap2D> PhysicsWorld2D::trigger_overlaps() const {
    std::vector<PhysicsTriggerOverlap2D> result;

    for (const auto& pair : broadphase_pairs()) {
        const auto* first = find_body(pair.first);
        const auto* second = find_body(pair.second);
        if (first == nullptr || second == nullptr) {
            continue;
        }
        if (!first->trigger && !second->trigger) {
            continue;
        }
        if (contact_for(*first, *second).has_value()) {
            result.push_back(PhysicsTriggerOverlap2D{.first = pair.first, .second = pair.second});
        }
    }

    return result;
}

std::optional<PhysicsRaycastHit2D> PhysicsWorld2D::raycast(PhysicsRaycast2DDesc desc) const {
    if (!finite_vec(desc.origin) || !finite_vec(desc.direction) || !finite(desc.max_distance) ||
        desc.max_distance < 0.0F) {
        return std::nullopt;
    }

    const auto direction_length = length(desc.direction);
    if (direction_length <= 0.000001F) {
        return std::nullopt;
    }

    const auto direction = desc.direction * (1.0F / direction_length);
    std::optional<PhysicsRaycastHit2D> closest;
    for (const auto& body : bodies_) {
        if (!body.collision_enabled || (desc.collision_mask & body.collision_layer) == 0U) {
            continue;
        }

        const auto hit = raycast_bounds(desc.origin, direction, desc.max_distance, bounds_for(body));
        if (!hit.has_value()) {
            continue;
        }

        if (!closest.has_value() || hit->distance < closest->distance ||
            (hit->distance == closest->distance && body.id.value < closest->body.value)) {
            closest = PhysicsRaycastHit2D{
                .body = body.id, .point = hit->point, .normal = hit->normal, .distance = hit->distance};
        }
    }

    return closest;
}

PhysicsRaycastBatch2DResult PhysicsWorld2D::raycast_batch(const PhysicsRaycastBatch2DDesc& desc) const {
    PhysicsRaycastBatch2DResult result;
    if (desc.max_queries == 0U || desc.queries.size() > desc.max_queries) {
        result.status = PhysicsCollisionQueryBatchStatus::invalid_request;
        result.diagnostic = PhysicsCollisionQueryBatchDiagnostic::query_budget_exceeded;
        return result;
    }

    result.status = PhysicsCollisionQueryBatchStatus::completed;
    result.diagnostic = PhysicsCollisionQueryBatchDiagnostic::none;
    result.rows.reserve(desc.queries.size());

    for (std::size_t index = 0; index < desc.queries.size(); ++index) {
        const auto& query = desc.queries[index];
        if (!is_valid_raycast_desc(query)) {
            result.rows.push_back(PhysicsRaycastBatch2DRow{
                .source_index = index,
                .status = PhysicsCollisionQueryRowStatus::invalid_request,
                .diagnostic = PhysicsCollisionQueryRowDiagnostic::invalid_request,
            });
            continue;
        }

        auto row = PhysicsRaycastBatch2DRow{
            .source_index = index,
            .status = PhysicsCollisionQueryRowStatus::no_hit,
            .diagnostic = PhysicsCollisionQueryRowDiagnostic::none,
        };
        if (auto hit = raycast(query); hit.has_value()) {
            row.status = PhysicsCollisionQueryRowStatus::hit;
            row.hit = *hit;
        }
        result.rows.push_back(row);
    }

    return result;
}

std::optional<PhysicsShapeSweepHit2D> PhysicsWorld2D::shape_sweep(PhysicsShapeSweep2DDesc desc) const {
    if (!is_valid_shape_sweep_desc(desc)) {
        return std::nullopt;
    }

    const auto direction_length = length(desc.direction);
    const auto query_extents = bounds_extents_for(desc.shape, desc.half_extents, desc.radius);
    const auto query_bounds = bounds_for(desc.origin, query_extents);
    std::optional<PhysicsShapeSweepHit2D> closest_initial;

    for (const auto& body : bodies_) {
        if (!body.collision_enabled || body.id == desc.ignored_body ||
            (desc.collision_mask & body.collision_layer) == 0U || (!desc.include_triggers && body.trigger)) {
            continue;
        }

        const auto target_bounds = bounds_for(body);
        if (bounds_overlap(query_bounds, target_bounds)) {
            const auto candidate = PhysicsShapeSweepHit2D{.body = body.id,
                                                          .position = desc.origin,
                                                          .normal = fallback_normal(desc.origin - body.position),
                                                          .distance = 0.0F,
                                                          .initial_overlap = true};
            if (!closest_initial.has_value() || body.id.value < closest_initial->body.value) {
                closest_initial = candidate;
            }
        }
    }

    if (closest_initial.has_value()) {
        return closest_initial;
    }

    if (direction_length <= 0.000001F) {
        return std::nullopt;
    }

    const auto direction = desc.direction * (1.0F / direction_length);
    std::optional<PhysicsShapeSweepHit2D> closest;
    for (const auto& body : bodies_) {
        if (!body.collision_enabled || body.id == desc.ignored_body ||
            (desc.collision_mask & body.collision_layer) == 0U || (!desc.include_triggers && body.trigger)) {
            continue;
        }

        const auto hit =
            raycast_bounds(desc.origin, direction, desc.max_distance, expand_bounds(bounds_for(body), query_extents));
        if (!hit.has_value()) {
            continue;
        }

        const auto candidate = PhysicsShapeSweepHit2D{.body = body.id,
                                                      .position = desc.origin + direction * hit->distance,
                                                      .normal = hit->normal,
                                                      .distance = hit->distance,
                                                      .initial_overlap = false};
        if (!closest.has_value() || candidate.distance < closest->distance ||
            (candidate.distance == closest->distance && body.id.value < closest->body.value)) {
            closest = candidate;
        }
    }

    return closest;
}

PhysicsShapeSweepBatch2DResult PhysicsWorld2D::shape_sweep_batch(const PhysicsShapeSweepBatch2DDesc& desc) const {
    PhysicsShapeSweepBatch2DResult result;
    if (desc.max_queries == 0U || desc.queries.size() > desc.max_queries) {
        result.status = PhysicsCollisionQueryBatchStatus::invalid_request;
        result.diagnostic = PhysicsCollisionQueryBatchDiagnostic::query_budget_exceeded;
        return result;
    }

    result.status = PhysicsCollisionQueryBatchStatus::completed;
    result.diagnostic = PhysicsCollisionQueryBatchDiagnostic::none;
    result.rows.reserve(desc.queries.size());

    for (std::size_t index = 0; index < desc.queries.size(); ++index) {
        const auto& query = desc.queries[index];
        if (!is_valid_shape_sweep_desc(query)) {
            result.rows.push_back(PhysicsShapeSweepBatch2DRow{
                .source_index = index,
                .status = PhysicsCollisionQueryRowStatus::invalid_request,
                .diagnostic = PhysicsCollisionQueryRowDiagnostic::invalid_request,
            });
            continue;
        }

        auto row = PhysicsShapeSweepBatch2DRow{
            .source_index = index,
            .status = PhysicsCollisionQueryRowStatus::no_hit,
            .diagnostic = PhysicsCollisionQueryRowDiagnostic::none,
        };
        if (auto hit = shape_sweep(query); hit.has_value()) {
            row.status = PhysicsCollisionQueryRowStatus::hit;
            row.hit = *hit;
        }
        result.rows.push_back(row);
    }

    return result;
}

void PhysicsWorld2D::apply_force(PhysicsBody2DId id, Vec2 force) {
    if (!finite_vec(force)) {
        throw std::invalid_argument("physics force is invalid");
    }
    auto* body = find_body(id);
    if (body == nullptr) {
        throw std::invalid_argument("physics body does not exist");
    }
    if (!body->dynamic) {
        return;
    }
    body->accumulated_force = body->accumulated_force + force;
}

void PhysicsWorld2D::step(float delta_seconds) {
    if (!finite(delta_seconds) || delta_seconds < 0.0F) {
        throw std::invalid_argument("physics delta seconds is invalid");
    }

    for (auto& body : bodies_) {
        if (!body.dynamic) {
            body.accumulated_force = Vec2{.x = 0.0F, .y = 0.0F};
            continue;
        }

        const auto acceleration = config_.gravity + (body.accumulated_force * body.inverse_mass);
        body.velocity = body.velocity + (acceleration * delta_seconds);
        const auto damping = std::max(0.0F, 1.0F - (body.linear_damping * delta_seconds));
        body.velocity = body.velocity * damping;
        body.position = body.position + (body.velocity * delta_seconds);
        body.accumulated_force = Vec2{.x = 0.0F, .y = 0.0F};
    }
}

void PhysicsWorld2D::resolve_contacts(float restitution) {
    resolve_contacts(PhysicsContactSolver2DConfig{.restitution = restitution, .iterations = 1U});
}

void PhysicsWorld2D::resolve_contacts(PhysicsContactSolver2DConfig config) {
    if (!finite(config.restitution) || config.restitution < 0.0F || config.restitution > 1.0F ||
        config.iterations == 0U || config.iterations > 64U) {
        throw std::invalid_argument("2d physics contact solver config is invalid");
    }

    for (std::uint32_t iteration = 0; iteration < config.iterations; ++iteration) {
        const auto pending_contacts = contacts();
        if (pending_contacts.empty()) {
            return;
        }

        for (const auto& contact : pending_contacts) {
            auto* first = find_body(contact.first);
            auto* second = find_body(contact.second);
            if (first == nullptr || second == nullptr) {
                continue;
            }

            const auto total_inverse_mass = first->inverse_mass + second->inverse_mass;
            if (total_inverse_mass <= 0.0F) {
                continue;
            }

            const auto first_position_share = first->inverse_mass / total_inverse_mass;
            const auto second_position_share = second->inverse_mass / total_inverse_mass;
            first->position = first->position - (contact.normal * (contact.penetration_depth * first_position_share));
            second->position =
                second->position + (contact.normal * (contact.penetration_depth * second_position_share));

            const auto relative_velocity = second->velocity - first->velocity;
            const auto contact_velocity = dot(relative_velocity, contact.normal);
            if (contact_velocity >= 0.0F) {
                continue;
            }

            const auto impulse_magnitude = -(1.0F + config.restitution) * contact_velocity / total_inverse_mass;
            const auto impulse = contact.normal * impulse_magnitude;
            first->velocity = first->velocity - (impulse * first->inverse_mass);
            second->velocity = second->velocity + (impulse * second->inverse_mass);
        }
    }
}

Physics2DSimulateStepResult simulate_physics2d_step(PhysicsWorld2D& world,
                                                    const Physics2DContinuousCollisionRequest& request) {
    if (!valid_runtime_request(request)) {
        return invalid_simulate_result(!finite(request.delta_seconds) || request.delta_seconds < 0.0F
                                           ? Physics2DRuntimeDiagnostic::invalid_delta_seconds
                                           : Physics2DRuntimeDiagnostic::invalid_config);
    }

    const auto time_of_impact_row_count = dynamic_body_count(world);
    const auto trigger_event_rows = collect_trigger_event_rows(world, request.previous_trigger_overlaps);
    if (time_of_impact_row_count > request.max_time_of_impact_rows ||
        request.kinematic_requests.size() > request.max_kinematic_contact_rows ||
        request.joints.size() > request.max_joint_rows || trigger_event_rows.size() > request.max_trigger_event_rows) {
        return invalid_simulate_result(Physics2DRuntimeDiagnostic::row_budget_exceeded);
    }

    Physics2DSimulateStepResult result;
    result.status = Physics2DSimulateStepStatus::simulated;
    result.diagnostic = Physics2DRuntimeDiagnostic::none;
    result.time_of_impact_rows.reserve(time_of_impact_row_count);
    result.kinematic_contact_rows.reserve(request.kinematic_requests.size());
    result.joint_rows.reserve(request.joints.size());
    result.trigger_event_rows = trigger_event_rows;

    std::vector<PhysicsBody2DId> dynamic_body_ids;
    dynamic_body_ids.reserve(time_of_impact_row_count);
    for (const auto& body : world.bodies()) {
        if (body.dynamic && body.collision_enabled) {
            dynamic_body_ids.push_back(body.id);
        }
    }

    for (std::size_t source_index = 0; source_index < dynamic_body_ids.size(); ++source_index) {
        auto* body = world.find_body(dynamic_body_ids[source_index]);
        if (body == nullptr) {
            continue;
        }

        const auto previous_position = body->position;
        const auto acceleration = world.config().gravity + (body->accumulated_force * body->inverse_mass);
        auto integrated_velocity = body->velocity + (acceleration * request.delta_seconds);
        const auto damping = std::max(0.0F, 1.0F - (body->linear_damping * request.delta_seconds));
        integrated_velocity = integrated_velocity * damping;
        const auto attempted_displacement = integrated_velocity * request.delta_seconds;

        Physics2DTimeOfImpactRow row{
            .source_index = source_index,
            .body = body->id,
            .previous_position = previous_position,
            .attempted_displacement = attempted_displacement,
            .applied_displacement = attempted_displacement,
            .remaining_displacement = Vec2{.x = 0.0F, .y = 0.0F},
            .hit_body = null_physics_body_2d,
            .normal = Vec2{.x = 1.0F, .y = 0.0F},
            .distance = 0.0F,
            .time_of_impact = 1.0F,
            .initial_overlap = false,
            .hit = false,
            .diagnostic = Physics2DRuntimeDiagnostic::none,
        };

        if (const auto hit = closest_exact_sweep(world.bodies(), *body, previous_position, attempted_displacement,
                                                 body->collision_mask, request.include_triggers, true);
            hit.has_value()) {
            const auto attempted_distance = length(attempted_displacement);
            const auto direction = normalized_or_zero(attempted_displacement);
            const auto applied_distance =
                hit->initial_overlap ? 0.0F : std::max(0.0F, hit->distance - request.skin_width);
            row.hit_body = hit->body;
            row.normal = hit->normal;
            row.distance = hit->distance;
            row.time_of_impact = attempted_distance > 0.000001F ? hit->distance / attempted_distance : 0.0F;
            row.initial_overlap = hit->initial_overlap;
            row.hit = true;
            row.applied_displacement = direction * applied_distance;
            row.remaining_displacement = attempted_displacement - row.applied_displacement;
        }

        body->position = previous_position + row.applied_displacement;
        if (row.hit && !row.initial_overlap) {
            const auto normal_component = std::min(0.0F, dot(integrated_velocity, row.normal));
            body->velocity = integrated_velocity - (row.normal * normal_component);
        } else {
            body->velocity = integrated_velocity;
        }
        body->accumulated_force = Vec2{.x = 0.0F, .y = 0.0F};
        result.time_of_impact_rows.push_back(row);
    }

    for (std::size_t source_index = 0; source_index < request.kinematic_requests.size(); ++source_index) {
        const auto& kinematic_request = request.kinematic_requests[source_index];
        auto row = Physics2DKinematicContactResolutionRow{
            .source_index = source_index,
            .body = kinematic_request.body,
            .hit_body = null_physics_body_2d,
            .attempted_displacement = kinematic_request.attempted_displacement,
            .applied_displacement = kinematic_request.attempted_displacement,
            .remaining_displacement = Vec2{.x = 0.0F, .y = 0.0F},
            .normal = Vec2{.x = 1.0F, .y = 0.0F},
            .initial_overlap = false,
            .diagnostic = Physics2DRuntimeDiagnostic::none,
        };

        auto* body = world.find_body(kinematic_request.body);
        if (body == nullptr || !finite_vec(kinematic_request.attempted_displacement) ||
            !finite(kinematic_request.skin_width) || kinematic_request.skin_width < 0.0F ||
            kinematic_request.max_iterations == 0U) {
            row.diagnostic = Physics2DRuntimeDiagnostic::invalid_request;
            result.kinematic_contact_rows.push_back(row);
            continue;
        }

        const auto hit =
            closest_exact_sweep(world.bodies(), *body, body->position, kinematic_request.attempted_displacement,
                                kinematic_request.collision_mask, kinematic_request.include_triggers, true);
        if (hit.has_value()) {
            const auto direction = normalized_or_zero(kinematic_request.attempted_displacement);
            const auto applied_distance =
                hit->initial_overlap ? 0.0F : std::max(0.0F, hit->distance - kinematic_request.skin_width);
            row.hit_body = hit->body;
            row.normal = hit->normal;
            row.initial_overlap = hit->initial_overlap;
            row.applied_displacement = direction * applied_distance;
            const auto leftover = kinematic_request.attempted_displacement - row.applied_displacement;
            const auto normal_component = std::min(0.0F, dot(leftover, row.normal));
            row.remaining_displacement = leftover - (row.normal * normal_component);
            body->position = body->position + row.applied_displacement + row.remaining_displacement;
        } else {
            body->position = body->position + row.applied_displacement;
        }
        result.kinematic_contact_rows.push_back(row);
    }

    for (std::size_t source_index = 0; source_index < request.joints.size(); ++source_index) {
        const auto& joint = request.joints[source_index];
        auto row = Physics2DJointRow{
            .source_index = source_index,
            .kind = joint.kind,
            .first = joint.first,
            .second = joint.second,
            .previous_distance = 0.0F,
            .target_distance = joint.target_distance,
            .residual_distance = 0.0F,
            .first_correction = Vec2{.x = 0.0F, .y = 0.0F},
            .second_correction = Vec2{.x = 0.0F, .y = 0.0F},
            .axis_limit_clamped = false,
            .diagnostic = Physics2DRuntimeDiagnostic::none,
        };

        auto* first = world.find_body(joint.first);
        auto* second = world.find_body(joint.second);
        if (first == nullptr || second == nullptr) {
            row.diagnostic = Physics2DRuntimeDiagnostic::missing_body;
            result.joint_rows.push_back(row);
            continue;
        }
        if (!joint.enabled) {
            row.diagnostic = Physics2DRuntimeDiagnostic::disabled_joint;
            result.joint_rows.push_back(row);
            continue;
        }
        if (joint.first == joint.second || !finite_vec(joint.local_anchor_first) ||
            !finite_vec(joint.local_anchor_second) || !finite(joint.target_distance) || !finite_vec(joint.axis) ||
            !finite(joint.minimum_translation) || !finite(joint.maximum_translation) || !finite(joint.stiffness)) {
            row.diagnostic = Physics2DRuntimeDiagnostic::invalid_joint;
            result.joint_rows.push_back(row);
            continue;
        }
        if (first->inverse_mass + second->inverse_mass <= 0.0F) {
            row.diagnostic = Physics2DRuntimeDiagnostic::static_pair;
            result.joint_rows.push_back(row);
            continue;
        }

        const auto first_anchor = first->position + joint.local_anchor_first;
        const auto second_anchor = second->position + joint.local_anchor_second;
        const auto delta = second_anchor - first_anchor;
        const auto previous_distance = length(delta);
        row.previous_distance = previous_distance;

        Vec2 correction_error{.x = 0.0F, .y = 0.0F};
        if (joint.kind == Physics2DJointKind::prismatic) {
            const auto axis_length = length(joint.axis);
            if (axis_length <= 0.000001F || joint.minimum_translation > joint.maximum_translation) {
                row.diagnostic = Physics2DRuntimeDiagnostic::invalid_joint;
                result.joint_rows.push_back(row);
                continue;
            }
            const auto axis = joint.axis * (1.0F / axis_length);
            const auto axis_distance = dot(delta, axis);
            const auto clamped_distance =
                clamp_value(axis_distance, joint.minimum_translation, joint.maximum_translation);
            row.axis_limit_clamped = clamped_distance != axis_distance;
            row.target_distance = clamped_distance;
            correction_error = delta - (axis * clamped_distance);
        } else {
            float target_distance = joint.target_distance;
            float scale = 1.0F;
            if (joint.kind == Physics2DJointKind::hinge) {
                target_distance = 0.0F;
            } else if (joint.kind == Physics2DJointKind::spring) {
                scale = clamp_value(joint.stiffness, 0.0F, 1.0F);
            }
            if (target_distance < 0.0F) {
                row.diagnostic = Physics2DRuntimeDiagnostic::invalid_joint;
                result.joint_rows.push_back(row);
                continue;
            }
            row.target_distance = target_distance;
            const auto normal =
                previous_distance > 0.000001F ? delta * (1.0F / previous_distance) : Vec2{.x = 1.0F, .y = 0.0F};
            correction_error = normal * ((previous_distance - target_distance) * scale);
        }

        const auto total_inverse_mass = first->inverse_mass + second->inverse_mass;
        row.first_correction = correction_error * (first->inverse_mass / total_inverse_mass);
        row.second_correction = correction_error * (-second->inverse_mass / total_inverse_mass);
        first->position = first->position + row.first_correction;
        second->position = second->position + row.second_correction;

        const auto resolved_first_anchor = first->position + joint.local_anchor_first;
        const auto resolved_second_anchor = second->position + joint.local_anchor_second;
        const auto resolved_delta = resolved_second_anchor - resolved_first_anchor;
        row.residual_distance = std::fabs(length(resolved_delta) - row.target_distance);
        result.joint_rows.push_back(row);
    }

    return result;
}

} // namespace mirakana
