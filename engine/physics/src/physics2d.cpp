// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/physics/physics2d.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

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

} // namespace mirakana
