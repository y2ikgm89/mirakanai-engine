// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/physics/physics3d.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_vec(Vec3 value) noexcept {
    return finite(value.x) && finite(value.y) && finite(value.z);
}

[[nodiscard]] float inverse_mass_for(const PhysicsBody3DDesc& desc) noexcept {
    return desc.dynamic ? 1.0F / desc.mass : 0.0F;
}

[[nodiscard]] bool valid_shape(PhysicsShape3DKind shape) noexcept {
    switch (shape) {
    case PhysicsShape3DKind::aabb:
    case PhysicsShape3DKind::sphere:
    case PhysicsShape3DKind::capsule:
        return true;
    }
    return false;
}

struct PhysicsBodyBounds3D {
    Vec3 minimum;
    Vec3 maximum;
};

struct CapsuleAxisPair3D {
    float first_y;
    float second_y;
};

struct ContactPenetration3D {
    Vec3 normal;
    float depth;
};

[[nodiscard]] float clamp_value(float value, float minimum, float maximum) noexcept {
    return std::max(minimum, std::min(value, maximum));
}

[[nodiscard]] float capsule_axis_y_for_bounds(const PhysicsBody3D& capsule,
                                              const PhysicsBodyBounds3D& bounds) noexcept {
    const auto capsule_min_y = capsule.position.y - capsule.half_height;
    const auto capsule_max_y = capsule.position.y + capsule.half_height;
    const auto overlapping_min_y = std::max(capsule_min_y, bounds.minimum.y);
    const auto overlapping_max_y = std::min(capsule_max_y, bounds.maximum.y);

    if (overlapping_min_y <= overlapping_max_y) {
        return clamp_value(capsule.position.y, overlapping_min_y, overlapping_max_y);
    }
    if (capsule_max_y < bounds.minimum.y) {
        return capsule_max_y;
    }
    return capsule_min_y;
}

[[nodiscard]] CapsuleAxisPair3D capsule_axis_pair(const PhysicsBody3D& first, const PhysicsBody3D& second) noexcept {
    const auto first_min_y = first.position.y - first.half_height;
    const auto first_max_y = first.position.y + first.half_height;
    const auto second_min_y = second.position.y - second.half_height;
    const auto second_max_y = second.position.y + second.half_height;
    const auto overlapping_min_y = std::max(first_min_y, second_min_y);
    const auto overlapping_max_y = std::min(first_max_y, second_max_y);

    if (overlapping_min_y <= overlapping_max_y) {
        const auto overlap_y = clamp_value(first.position.y, overlapping_min_y, overlapping_max_y);
        return CapsuleAxisPair3D{.first_y = overlap_y, .second_y = overlap_y};
    }
    if (first_max_y < second_min_y) {
        return CapsuleAxisPair3D{.first_y = first_max_y, .second_y = second_min_y};
    }
    return CapsuleAxisPair3D{.first_y = first_min_y, .second_y = second_max_y};
}

[[nodiscard]] ContactPenetration3D contact_penetration_from_distance(Vec3 delta, float distance_squared, float radius,
                                                                     Vec3 fallback_normal,
                                                                     float fallback_depth) noexcept {
    if (distance_squared > 0.000001F) {
        const auto distance = std::sqrt(distance_squared);
        return ContactPenetration3D{.normal = delta * (1.0F / distance), .depth = radius - distance};
    }
    return ContactPenetration3D{.normal = fallback_normal, .depth = fallback_depth};
}

[[nodiscard]] Vec3 bounds_extents_for(const PhysicsBody3D& body) noexcept {
    if (body.shape == PhysicsShape3DKind::sphere) {
        return Vec3{.x = body.radius, .y = body.radius, .z = body.radius};
    }
    if (body.shape == PhysicsShape3DKind::capsule) {
        return Vec3{.x = body.radius, .y = body.half_height + body.radius, .z = body.radius};
    }
    return body.half_extents;
}

[[nodiscard]] PhysicsBodyBounds3D bounds_for(const PhysicsBody3D& body) noexcept {
    const auto extents = bounds_extents_for(body);
    return PhysicsBodyBounds3D{
        .minimum = body.position - extents,
        .maximum = body.position + extents,
    };
}

[[nodiscard]] Vec3 bounds_extents_for(PhysicsShape3DKind shape, Vec3 half_extents, float radius,
                                      float half_height) noexcept {
    if (shape == PhysicsShape3DKind::sphere) {
        return Vec3{.x = radius, .y = radius, .z = radius};
    }
    if (shape == PhysicsShape3DKind::capsule) {
        return Vec3{.x = radius, .y = half_height + radius, .z = radius};
    }
    return half_extents;
}

[[nodiscard]] PhysicsBodyBounds3D bounds_for(Vec3 center, Vec3 extents) noexcept {
    return PhysicsBodyBounds3D{
        .minimum = center - extents,
        .maximum = center + extents,
    };
}

[[nodiscard]] PhysicsBodyBounds3D expand_bounds(const PhysicsBodyBounds3D& bounds, Vec3 extents) noexcept {
    return PhysicsBodyBounds3D{
        .minimum = bounds.minimum - extents,
        .maximum = bounds.maximum + extents,
    };
}

[[nodiscard]] bool bounds_overlap(const PhysicsBodyBounds3D& first, const PhysicsBodyBounds3D& second) noexcept {
    return first.minimum.x <= second.maximum.x && first.maximum.x >= second.minimum.x &&
           first.minimum.y <= second.maximum.y && first.maximum.y >= second.minimum.y &&
           first.minimum.z <= second.maximum.z && first.maximum.z >= second.minimum.z;
}

[[nodiscard]] bool collision_filters_match(const PhysicsBody3D& first, const PhysicsBody3D& second) noexcept {
    return (first.collision_mask & second.collision_layer) != 0U &&
           (second.collision_mask & first.collision_layer) != 0U;
}

struct BoundsRaycast3D {
    Vec3 point;
    Vec3 normal;
    float distance{0.0F};
};

[[nodiscard]] bool update_ray_interval(float origin, float direction, float minimum, float maximum, Vec3 min_normal,
                                       Vec3 max_normal, float& entry_distance, float& exit_distance,
                                       Vec3& entry_normal) noexcept {
    constexpr auto parallel_epsilon = 0.000001F;
    if (std::fabs(direction) <= parallel_epsilon) {
        return origin >= minimum && origin <= maximum;
    }

    float near_distance = 0.0F;
    float far_distance = 0.0F;
    Vec3 near_normal{};
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

[[nodiscard]] std::optional<BoundsRaycast3D> raycast_bounds(Vec3 origin, Vec3 direction, float max_distance,
                                                            const PhysicsBodyBounds3D& bounds) noexcept {
    float entry_distance = 0.0F;
    auto exit_distance = max_distance;
    Vec3 entry_normal{.x = direction.x * -1.0F, .y = direction.y * -1.0F, .z = direction.z * -1.0F};

    if (!update_ray_interval(origin.x, direction.x, bounds.minimum.x, bounds.maximum.x,
                             Vec3{.x = -1.0F, .y = 0.0F, .z = 0.0F}, Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                             entry_distance, exit_distance, entry_normal)) {
        return std::nullopt;
    }
    if (!update_ray_interval(origin.y, direction.y, bounds.minimum.y, bounds.maximum.y,
                             Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F}, Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
                             entry_distance, exit_distance, entry_normal)) {
        return std::nullopt;
    }
    if (!update_ray_interval(origin.z, direction.z, bounds.minimum.z, bounds.maximum.z,
                             Vec3{.x = 0.0F, .y = 0.0F, .z = -1.0F}, Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F},
                             entry_distance, exit_distance, entry_normal)) {
        return std::nullopt;
    }
    if (entry_distance < 0.0F || entry_distance > max_distance) {
        return std::nullopt;
    }

    return BoundsRaycast3D{
        .point = origin + direction * entry_distance, .normal = entry_normal, .distance = entry_distance};
}

[[nodiscard]] Vec3 fallback_normal(Vec3 delta) noexcept {
    if (std::fabs(delta.x) >= std::fabs(delta.y) && std::fabs(delta.x) >= std::fabs(delta.z)) {
        return Vec3{.x = delta.x >= 0.0F ? 1.0F : -1.0F, .y = 0.0F, .z = 0.0F};
    }
    if (std::fabs(delta.y) >= std::fabs(delta.z)) {
        return Vec3{.x = 0.0F, .y = delta.y >= 0.0F ? 1.0F : -1.0F, .z = 0.0F};
    }
    return Vec3{.x = 0.0F, .y = 0.0F, .z = delta.z >= 0.0F ? 1.0F : -1.0F};
}

[[nodiscard]] Vec3 normalize_or_fallback(Vec3 value, Vec3 fallback) noexcept {
    const auto value_length = length(value);
    if (value_length <= 0.000001F) {
        return fallback;
    }
    return value * (1.0F / value_length);
}

[[nodiscard]] float axis_support(float extent, float direction) noexcept {
    constexpr auto epsilon = 0.000001F;
    if (direction > epsilon) {
        return extent;
    }
    if (direction < -epsilon) {
        return -extent;
    }
    return 0.0F;
}

[[nodiscard]] Vec3 support_point_for(const PhysicsBody3D& body, Vec3 direction) noexcept {
    if (body.shape == PhysicsShape3DKind::sphere) {
        return body.position + normalize_or_fallback(direction, Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}) * body.radius;
    }

    if (body.shape == PhysicsShape3DKind::capsule) {
        const auto normal = normalize_or_fallback(direction, Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
        const auto axis_y = body.position.y + axis_support(body.half_height, normal.y);
        return Vec3{.x = body.position.x, .y = axis_y, .z = body.position.z} + normal * body.radius;
    }

    return body.position + Vec3{
                               .x = axis_support(body.half_extents.x, direction.x),
                               .y = axis_support(body.half_extents.y, direction.y),
                               .z = axis_support(body.half_extents.z, direction.z),
                           };
}

[[nodiscard]] float overlap_midpoint(float first_minimum, float first_maximum, float second_minimum,
                                     float second_maximum) noexcept {
    return (std::max(first_minimum, second_minimum) + std::min(first_maximum, second_maximum)) * 0.5F;
}

[[nodiscard]] Vec3 aabb_aabb_contact_point(const PhysicsBody3D& first, const PhysicsBody3D& second) noexcept {
    const auto first_bounds = bounds_for(first);
    const auto second_bounds = bounds_for(second);
    return Vec3{
        .x = overlap_midpoint(first_bounds.minimum.x, first_bounds.maximum.x, second_bounds.minimum.x,
                              second_bounds.maximum.x),
        .y = overlap_midpoint(first_bounds.minimum.y, first_bounds.maximum.y, second_bounds.minimum.y,
                              second_bounds.maximum.y),
        .z = overlap_midpoint(first_bounds.minimum.z, first_bounds.maximum.z, second_bounds.minimum.z,
                              second_bounds.maximum.z),
    };
}

[[nodiscard]] Vec3 sphere_aabb_contact_point(const PhysicsBody3D& sphere, const PhysicsBody3D& aabb,
                                             Vec3 normal_from_sphere_to_aabb) noexcept {
    const auto bounds = bounds_for(aabb);
    const auto closest = Vec3{
        .x = clamp_value(sphere.position.x, bounds.minimum.x, bounds.maximum.x),
        .y = clamp_value(sphere.position.y, bounds.minimum.y, bounds.maximum.y),
        .z = clamp_value(sphere.position.z, bounds.minimum.z, bounds.maximum.z),
    };
    if (dot(closest - sphere.position, closest - sphere.position) > 0.000001F) {
        return closest;
    }
    return support_point_for(aabb, normal_from_sphere_to_aabb * -1.0F);
}

[[nodiscard]] Vec3 closest_point_on_capsule_axis(const PhysicsBody3D& capsule, Vec3 point) noexcept;

[[nodiscard]] Vec3 capsule_sphere_contact_point(const PhysicsBody3D& capsule, const PhysicsBody3D& sphere,
                                                Vec3 normal_from_capsule_to_sphere) noexcept {
    const auto axis_point = closest_point_on_capsule_axis(capsule, sphere.position);
    const auto capsule_surface = axis_point + normal_from_capsule_to_sphere * capsule.radius;
    const auto sphere_surface = sphere.position - normal_from_capsule_to_sphere * sphere.radius;
    return (capsule_surface + sphere_surface) * 0.5F;
}

[[nodiscard]] Vec3 capsule_aabb_contact_point(const PhysicsBody3D& capsule, const PhysicsBody3D& aabb,
                                              Vec3 normal_from_capsule_to_aabb) noexcept {
    const auto bounds = bounds_for(aabb);
    const auto axis_y = capsule_axis_y_for_bounds(capsule, bounds);
    const auto axis_point = Vec3{.x = capsule.position.x, .y = axis_y, .z = capsule.position.z};
    const auto closest = Vec3{
        .x = clamp_value(axis_point.x, bounds.minimum.x, bounds.maximum.x),
        .y = clamp_value(axis_point.y, bounds.minimum.y, bounds.maximum.y),
        .z = clamp_value(axis_point.z, bounds.minimum.z, bounds.maximum.z),
    };
    if (dot(closest - axis_point, closest - axis_point) > 0.000001F) {
        return closest;
    }
    return support_point_for(aabb, normal_from_capsule_to_aabb * -1.0F);
}

[[nodiscard]] Vec3 capsule_capsule_contact_point(const PhysicsBody3D& first, const PhysicsBody3D& second,
                                                 Vec3 normal) noexcept {
    const auto axes = capsule_axis_pair(first, second);
    const auto first_surface =
        Vec3{.x = first.position.x, .y = axes.first_y, .z = first.position.z} + normal * first.radius;
    const auto second_surface =
        Vec3{.x = second.position.x, .y = axes.second_y, .z = second.position.z} - normal * second.radius;
    return (first_surface + second_surface) * 0.5F;
}

[[nodiscard]] Vec3 contact_point_for(const PhysicsBody3D& first, const PhysicsBody3D& second,
                                     const PhysicsContact3D& contact) noexcept {
    if (first.shape == PhysicsShape3DKind::aabb && second.shape == PhysicsShape3DKind::aabb) {
        return aabb_aabb_contact_point(first, second);
    }

    if (first.shape == PhysicsShape3DKind::sphere && second.shape == PhysicsShape3DKind::aabb) {
        return sphere_aabb_contact_point(first, second, contact.normal);
    }

    if (first.shape == PhysicsShape3DKind::aabb && second.shape == PhysicsShape3DKind::sphere) {
        return sphere_aabb_contact_point(second, first, contact.normal * -1.0F);
    }

    if (first.shape == PhysicsShape3DKind::capsule && second.shape == PhysicsShape3DKind::sphere) {
        return capsule_sphere_contact_point(first, second, contact.normal);
    }

    if (first.shape == PhysicsShape3DKind::sphere && second.shape == PhysicsShape3DKind::capsule) {
        return capsule_sphere_contact_point(second, first, contact.normal * -1.0F);
    }

    if (first.shape == PhysicsShape3DKind::capsule && second.shape == PhysicsShape3DKind::aabb) {
        return capsule_aabb_contact_point(first, second, contact.normal);
    }

    if (first.shape == PhysicsShape3DKind::aabb && second.shape == PhysicsShape3DKind::capsule) {
        return capsule_aabb_contact_point(second, first, contact.normal * -1.0F);
    }

    if (first.shape == PhysicsShape3DKind::capsule && second.shape == PhysicsShape3DKind::capsule) {
        return capsule_capsule_contact_point(first, second, contact.normal);
    }

    const auto first_point = support_point_for(first, contact.normal);
    const auto second_point = support_point_for(second, contact.normal * -1.0F);
    return (first_point + second_point) * 0.5F;
}

[[nodiscard]] std::uint32_t feature_id_for_contact(const PhysicsBody3D& first, const PhysicsBody3D& second,
                                                   Vec3 normal) noexcept {
    constexpr std::uint32_t aabb_aabb_x_feature = 0xA001U;
    constexpr std::uint32_t aabb_aabb_y_feature = 0xA002U;
    constexpr std::uint32_t aabb_aabb_z_feature = 0xA003U;
    constexpr std::uint32_t sphere_sphere_feature = 0xB001U;
    constexpr std::uint32_t sphere_aabb_feature = 0xB002U;
    constexpr std::uint32_t capsule_sphere_feature = 0xC001U;
    constexpr std::uint32_t capsule_aabb_feature = 0xC002U;
    constexpr std::uint32_t capsule_capsule_feature = 0xC003U;

    if (first.shape == PhysicsShape3DKind::aabb && second.shape == PhysicsShape3DKind::aabb) {
        if (std::fabs(normal.y) > std::fabs(normal.x) && std::fabs(normal.y) >= std::fabs(normal.z)) {
            return aabb_aabb_y_feature;
        }
        if (std::fabs(normal.z) > std::fabs(normal.x) && std::fabs(normal.z) > std::fabs(normal.y)) {
            return aabb_aabb_z_feature;
        }
        return aabb_aabb_x_feature;
    }

    if (first.shape == PhysicsShape3DKind::sphere && second.shape == PhysicsShape3DKind::sphere) {
        return sphere_sphere_feature;
    }

    if ((first.shape == PhysicsShape3DKind::sphere && second.shape == PhysicsShape3DKind::aabb) ||
        (first.shape == PhysicsShape3DKind::aabb && second.shape == PhysicsShape3DKind::sphere)) {
        return sphere_aabb_feature;
    }

    if ((first.shape == PhysicsShape3DKind::capsule && second.shape == PhysicsShape3DKind::sphere) ||
        (first.shape == PhysicsShape3DKind::sphere && second.shape == PhysicsShape3DKind::capsule)) {
        return capsule_sphere_feature;
    }

    if ((first.shape == PhysicsShape3DKind::capsule && second.shape == PhysicsShape3DKind::aabb) ||
        (first.shape == PhysicsShape3DKind::aabb && second.shape == PhysicsShape3DKind::capsule)) {
        return capsule_aabb_feature;
    }

    if (first.shape == PhysicsShape3DKind::capsule && second.shape == PhysicsShape3DKind::capsule) {
        return capsule_capsule_feature;
    }

    return 0U;
}

[[nodiscard]] PhysicsContactManifold3D
make_single_point_manifold(const PhysicsBody3D& first, const PhysicsBody3D& second, const PhysicsContact3D& contact) {
    const auto feature_id = feature_id_for_contact(first, second, contact.normal);
    return PhysicsContactManifold3D{
        .first = contact.first,
        .second = contact.second,
        .normal = contact.normal,
        .points = std::vector<PhysicsContactPoint3D>{PhysicsContactPoint3D{
            .position = contact_point_for(first, second, contact),
            .penetration_depth = contact.penetration_depth,
            .feature_id = feature_id,
            .warm_start_eligible = feature_id != 0U,
        }},
    };
}

[[nodiscard]] std::optional<PhysicsContact3D> aabb_contact(const PhysicsBody3D& first, const PhysicsBody3D& second) {
    const auto delta = second.position - first.position;
    const auto overlap_x = (first.half_extents.x + second.half_extents.x) - std::fabs(delta.x);
    const auto overlap_y = (first.half_extents.y + second.half_extents.y) - std::fabs(delta.y);
    const auto overlap_z = (first.half_extents.z + second.half_extents.z) - std::fabs(delta.z);
    if (overlap_x < 0.0F || overlap_y < 0.0F || overlap_z < 0.0F) {
        return std::nullopt;
    }

    Vec3 normal{.x = delta.x >= 0.0F ? 1.0F : -1.0F, .y = 0.0F, .z = 0.0F};
    auto penetration_depth = overlap_x;
    if (overlap_y < penetration_depth) {
        penetration_depth = overlap_y;
        normal = Vec3{.x = 0.0F, .y = delta.y >= 0.0F ? 1.0F : -1.0F, .z = 0.0F};
    }
    if (overlap_z < penetration_depth) {
        penetration_depth = overlap_z;
        normal = Vec3{.x = 0.0F, .y = 0.0F, .z = delta.z >= 0.0F ? 1.0F : -1.0F};
    }

    return PhysicsContact3D{
        .first = first.id, .second = second.id, .normal = normal, .penetration_depth = penetration_depth};
}

[[nodiscard]] std::optional<PhysicsContact3D> sphere_contact(const PhysicsBody3D& first, const PhysicsBody3D& second) {
    const auto delta = second.position - first.position;
    const auto radius_sum = first.radius + second.radius;
    const auto distance_squared = dot(delta, delta);
    if (distance_squared > radius_sum * radius_sum) {
        return std::nullopt;
    }

    const auto distance = std::sqrt(distance_squared);
    const auto normal = distance > 0.000001F ? delta * (1.0F / distance) : Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    return PhysicsContact3D{
        .first = first.id, .second = second.id, .normal = normal, .penetration_depth = radius_sum - distance};
}

[[nodiscard]] std::optional<PhysicsContact3D> sphere_aabb_contact(const PhysicsBody3D& sphere,
                                                                  const PhysicsBody3D& aabb, bool sphere_is_first) {
    const auto bounds = bounds_for(aabb);
    const auto closest = Vec3{
        .x = clamp_value(sphere.position.x, bounds.minimum.x, bounds.maximum.x),
        .y = clamp_value(sphere.position.y, bounds.minimum.y, bounds.maximum.y),
        .z = clamp_value(sphere.position.z, bounds.minimum.z, bounds.maximum.z),
    };
    auto delta = closest - sphere.position;
    auto distance_squared = dot(delta, delta);

    if (distance_squared > sphere.radius * sphere.radius) {
        return std::nullopt;
    }

    const auto penetration = [&]() -> ContactPenetration3D {
        if (distance_squared > 0.000001F) {
            return contact_penetration_from_distance(delta, distance_squared, sphere.radius,
                                                     Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}, sphere.radius);
        }
        const auto local = sphere.position - aabb.position;
        const auto distance_to_x = aabb.half_extents.x - std::fabs(local.x);
        const auto distance_to_y = aabb.half_extents.y - std::fabs(local.y);
        const auto distance_to_z = aabb.half_extents.z - std::fabs(local.z);
        return contact_penetration_from_distance(delta, distance_squared, sphere.radius, fallback_normal(local),
                                                 sphere.radius +
                                                     std::min({distance_to_x, distance_to_y, distance_to_z}));
    }();

    if (sphere_is_first) {
        return PhysicsContact3D{.first = sphere.id,
                                .second = aabb.id,
                                .normal = penetration.normal,
                                .penetration_depth = penetration.depth};
    }
    return PhysicsContact3D{.first = aabb.id,
                            .second = sphere.id,
                            .normal = penetration.normal * -1.0F,
                            .penetration_depth = penetration.depth};
}

[[nodiscard]] Vec3 closest_point_on_capsule_axis(const PhysicsBody3D& capsule, Vec3 point) noexcept {
    return Vec3{
        .x = capsule.position.x,
        .y = clamp_value(point.y, capsule.position.y - capsule.half_height, capsule.position.y + capsule.half_height),
        .z = capsule.position.z,
    };
}

[[nodiscard]] std::optional<PhysicsContact3D>
capsule_sphere_contact(const PhysicsBody3D& capsule, const PhysicsBody3D& sphere, bool capsule_is_first) {
    const auto closest = closest_point_on_capsule_axis(capsule, sphere.position);
    const auto delta = sphere.position - closest;
    const auto radius_sum = capsule.radius + sphere.radius;
    const auto distance_squared = dot(delta, delta);
    if (distance_squared > radius_sum * radius_sum) {
        return std::nullopt;
    }

    const auto distance = std::sqrt(distance_squared);
    const auto normal_from_capsule_to_sphere =
        distance > 0.000001F ? delta * (1.0F / distance) : fallback_normal(sphere.position - capsule.position);
    const auto penetration_depth = radius_sum - distance;
    if (capsule_is_first) {
        return PhysicsContact3D{.first = capsule.id,
                                .second = sphere.id,
                                .normal = normal_from_capsule_to_sphere,
                                .penetration_depth = penetration_depth};
    }
    return PhysicsContact3D{.first = sphere.id,
                            .second = capsule.id,
                            .normal = normal_from_capsule_to_sphere * -1.0F,
                            .penetration_depth = penetration_depth};
}

[[nodiscard]] std::optional<PhysicsContact3D> capsule_aabb_contact(const PhysicsBody3D& capsule,
                                                                   const PhysicsBody3D& aabb, bool capsule_is_first) {
    const auto bounds = bounds_for(aabb);
    const auto axis_y = capsule_axis_y_for_bounds(capsule, bounds);
    const auto axis_point = Vec3{.x = capsule.position.x, .y = axis_y, .z = capsule.position.z};
    const auto closest = Vec3{
        .x = clamp_value(axis_point.x, bounds.minimum.x, bounds.maximum.x),
        .y = clamp_value(axis_point.y, bounds.minimum.y, bounds.maximum.y),
        .z = clamp_value(axis_point.z, bounds.minimum.z, bounds.maximum.z),
    };
    const auto delta = closest - axis_point;
    const auto distance_squared = dot(delta, delta);
    if (distance_squared > capsule.radius * capsule.radius) {
        return std::nullopt;
    }

    const auto penetration = [&]() -> ContactPenetration3D {
        if (distance_squared > 0.000001F) {
            return contact_penetration_from_distance(delta, distance_squared, capsule.radius,
                                                     Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}, capsule.radius);
        }
        const auto local = axis_point - aabb.position;
        const auto distance_to_x = aabb.half_extents.x - std::fabs(local.x);
        const auto distance_to_y = aabb.half_extents.y - std::fabs(local.y);
        const auto distance_to_z = aabb.half_extents.z - std::fabs(local.z);
        return contact_penetration_from_distance(delta, distance_squared, capsule.radius, fallback_normal(local),
                                                 capsule.radius +
                                                     std::min({distance_to_x, distance_to_y, distance_to_z}));
    }();

    if (capsule_is_first) {
        return PhysicsContact3D{.first = capsule.id,
                                .second = aabb.id,
                                .normal = penetration.normal,
                                .penetration_depth = penetration.depth};
    }
    return PhysicsContact3D{.first = aabb.id,
                            .second = capsule.id,
                            .normal = penetration.normal * -1.0F,
                            .penetration_depth = penetration.depth};
}

[[nodiscard]] std::optional<PhysicsContact3D> capsule_contact(const PhysicsBody3D& first, const PhysicsBody3D& second) {
    const auto axes = capsule_axis_pair(first, second);
    const auto first_point = Vec3{.x = first.position.x, .y = axes.first_y, .z = first.position.z};
    const auto second_point = Vec3{.x = second.position.x, .y = axes.second_y, .z = second.position.z};
    const auto delta = second_point - first_point;
    const auto radius_sum = first.radius + second.radius;
    const auto distance_squared = dot(delta, delta);
    if (distance_squared > radius_sum * radius_sum) {
        return std::nullopt;
    }

    const auto distance = std::sqrt(distance_squared);
    const auto normal = distance > 0.000001F ? delta * (1.0F / distance) : Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    return PhysicsContact3D{
        .first = first.id, .second = second.id, .normal = normal, .penetration_depth = radius_sum - distance};
}

struct ExactSphereCastCandidate3D {
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 normal{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float distance{0.0F};
    bool initial_overlap{false};
};

[[nodiscard]] Vec3 closest_point_on_bounds(const PhysicsBodyBounds3D& bounds, Vec3 point) noexcept {
    return Vec3{
        .x = clamp_value(point.x, bounds.minimum.x, bounds.maximum.x),
        .y = clamp_value(point.y, bounds.minimum.y, bounds.maximum.y),
        .z = clamp_value(point.z, bounds.minimum.z, bounds.maximum.z),
    };
}

[[nodiscard]] bool point_in_axis_bounds(float value, float minimum, float maximum) noexcept {
    constexpr auto bounds_epsilon = 0.00001F;
    return value >= minimum - bounds_epsilon && value <= maximum + bounds_epsilon;
}

[[nodiscard]] bool point_in_face_bounds(Vec3 point, const PhysicsBodyBounds3D& bounds, int axis) noexcept {
    if (axis == 0) {
        return point_in_axis_bounds(point.y, bounds.minimum.y, bounds.maximum.y) &&
               point_in_axis_bounds(point.z, bounds.minimum.z, bounds.maximum.z);
    }
    if (axis == 1) {
        return point_in_axis_bounds(point.x, bounds.minimum.x, bounds.maximum.x) &&
               point_in_axis_bounds(point.z, bounds.minimum.z, bounds.maximum.z);
    }
    return point_in_axis_bounds(point.x, bounds.minimum.x, bounds.maximum.x) &&
           point_in_axis_bounds(point.y, bounds.minimum.y, bounds.maximum.y);
}

[[nodiscard]] float axis_value(Vec3 value, int axis) noexcept {
    if (axis == 0) {
        return value.x;
    }
    if (axis == 1) {
        return value.y;
    }
    return value.z;
}

[[nodiscard]] Vec3 axis_normal(int axis, float sign) noexcept {
    if (axis == 0) {
        return Vec3{.x = sign, .y = 0.0F, .z = 0.0F};
    }
    if (axis == 1) {
        return Vec3{.x = 0.0F, .y = sign, .z = 0.0F};
    }
    return Vec3{.x = 0.0F, .y = 0.0F, .z = sign};
}

void replace_with_nearer(std::optional<ExactSphereCastCandidate3D>& closest,
                         ExactSphereCastCandidate3D candidate) noexcept {
    if (!closest.has_value() || candidate.distance < closest->distance) {
        closest = candidate;
    }
}

void keep_nearest_candidate(std::optional<ExactSphereCastCandidate3D>& closest,
                            std::optional<ExactSphereCastCandidate3D> candidate) noexcept {
    if (candidate.has_value()) {
        replace_with_nearer(closest, *candidate);
    }
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D>
raycast_sphere_surface(Vec3 origin, Vec3 direction, float max_distance, Vec3 center, float radius) noexcept {
    const auto origin_to_center = origin - center;
    const auto c = dot(origin_to_center, origin_to_center) - (radius * radius);
    if (c <= 0.0F) {
        return std::nullopt;
    }

    const auto b = dot(origin_to_center, direction);
    if (b > 0.0F) {
        return std::nullopt;
    }

    const auto discriminant = (b * b) - c;
    if (discriminant < 0.0F) {
        return std::nullopt;
    }

    auto distance = -b - std::sqrt(discriminant);
    distance = std::max(distance, 0.0F);
    if (distance > max_distance) {
        return std::nullopt;
    }

    const auto position = origin + direction * distance;
    const auto normal = normalize_or_fallback(position - center, direction * -1.0F);
    return ExactSphereCastCandidate3D{
        .position = position, .normal = normal, .distance = distance, .initial_overlap = false};
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D>
raycast_axis_cylinder(Vec3 origin, Vec3 direction, float max_distance, int axis, float axis_minimum, float axis_maximum,
                      float fixed_first, float fixed_second, float radius) noexcept {
    float origin_first = 0.0F;
    float origin_second = 0.0F;
    float direction_first = 0.0F;
    float direction_second = 0.0F;

    if (axis == 0) {
        origin_first = origin.y - fixed_first;
        origin_second = origin.z - fixed_second;
        direction_first = direction.y;
        direction_second = direction.z;
    } else if (axis == 1) {
        origin_first = origin.x - fixed_first;
        origin_second = origin.z - fixed_second;
        direction_first = direction.x;
        direction_second = direction.z;
    } else {
        origin_first = origin.x - fixed_first;
        origin_second = origin.y - fixed_second;
        direction_first = direction.x;
        direction_second = direction.y;
    }

    const auto a = (direction_first * direction_first) + (direction_second * direction_second);
    if (a <= 0.000001F) {
        return std::nullopt;
    }

    const auto b = 2.0F * (origin_first * direction_first + origin_second * direction_second);
    const auto c = (origin_first * origin_first) + (origin_second * origin_second) - (radius * radius);
    const auto discriminant = (b * b) - (4.0F * a * c);
    if (discriminant < 0.0F) {
        return std::nullopt;
    }

    const auto sqrt_discriminant = std::sqrt(discriminant);
    const auto inverse_denominator = 1.0F / (2.0F * a);
    const auto first_distance = (-b - sqrt_discriminant) * inverse_denominator;
    const auto second_distance = (-b + sqrt_discriminant) * inverse_denominator;

    const auto make_candidate = [&](float distance) -> std::optional<ExactSphereCastCandidate3D> {
        if (distance < 0.0F || distance > max_distance) {
            return std::nullopt;
        }

        const auto axis_position = axis_value(origin + direction * distance, axis);
        if (!point_in_axis_bounds(axis_position, axis_minimum, axis_maximum)) {
            return std::nullopt;
        }

        const auto hit_position = origin + direction * distance;
        Vec3 normal_delta{};
        if (axis == 0) {
            normal_delta = Vec3{.x = 0.0F, .y = hit_position.y - fixed_first, .z = hit_position.z - fixed_second};
        } else if (axis == 1) {
            normal_delta = Vec3{.x = hit_position.x - fixed_first, .y = 0.0F, .z = hit_position.z - fixed_second};
        } else {
            normal_delta = Vec3{.x = hit_position.x - fixed_first, .y = hit_position.y - fixed_second, .z = 0.0F};
        }

        return ExactSphereCastCandidate3D{
            .position = hit_position,
            .normal = normalize_or_fallback(normal_delta, direction * -1.0F),
            .distance = distance,
            .initial_overlap = false,
        };
    };

    if (const auto first = make_candidate(first_distance); first.has_value()) {
        return first;
    }
    return make_candidate(second_distance);
}

void consider_aabb_face_candidate(std::optional<ExactSphereCastCandidate3D>& closest, Vec3 origin, Vec3 direction,
                                  float max_distance, float radius, const PhysicsBodyBounds3D& bounds, int axis,
                                  float normal_sign) noexcept {
    const auto direction_axis = axis_value(direction, axis);
    if (std::fabs(direction_axis) <= 0.000001F) {
        return;
    }

    const auto surface =
        normal_sign < 0.0F ? axis_value(bounds.minimum, axis) - radius : axis_value(bounds.maximum, axis) + radius;
    const auto distance = (surface - axis_value(origin, axis)) / direction_axis;
    if (distance < 0.0F || distance > max_distance) {
        return;
    }

    const auto position = origin + direction * distance;
    if (!point_in_face_bounds(position, bounds, axis)) {
        return;
    }

    replace_with_nearer(closest, ExactSphereCastCandidate3D{.position = position,
                                                            .normal = axis_normal(axis, normal_sign),
                                                            .distance = distance,
                                                            .initial_overlap = false});
}

void consider_aabb_edge_candidates(std::optional<ExactSphereCastCandidate3D>& closest, Vec3 origin, Vec3 direction,
                                   float max_distance, float radius, const PhysicsBodyBounds3D& bounds) noexcept {
    for (const auto y : {bounds.minimum.y, bounds.maximum.y}) {
        for (const auto z : {bounds.minimum.z, bounds.maximum.z}) {
            keep_nearest_candidate(closest, raycast_axis_cylinder(origin, direction, max_distance, 0, bounds.minimum.x,
                                                                  bounds.maximum.x, y, z, radius));
        }
    }
    for (const auto x : {bounds.minimum.x, bounds.maximum.x}) {
        for (const auto z : {bounds.minimum.z, bounds.maximum.z}) {
            keep_nearest_candidate(closest, raycast_axis_cylinder(origin, direction, max_distance, 1, bounds.minimum.y,
                                                                  bounds.maximum.y, x, z, radius));
        }
    }
    for (const auto x : {bounds.minimum.x, bounds.maximum.x}) {
        for (const auto y : {bounds.minimum.y, bounds.maximum.y}) {
            keep_nearest_candidate(closest, raycast_axis_cylinder(origin, direction, max_distance, 2, bounds.minimum.z,
                                                                  bounds.maximum.z, x, y, radius));
        }
    }
}

void consider_aabb_corner_candidates(std::optional<ExactSphereCastCandidate3D>& closest, Vec3 origin, Vec3 direction,
                                     float max_distance, float radius, const PhysicsBodyBounds3D& bounds) noexcept {
    for (const auto x : {bounds.minimum.x, bounds.maximum.x}) {
        for (const auto y : {bounds.minimum.y, bounds.maximum.y}) {
            for (const auto z : {bounds.minimum.z, bounds.maximum.z}) {
                keep_nearest_candidate(closest, raycast_sphere_surface(origin, direction, max_distance,
                                                                       Vec3{.x = x, .y = y, .z = z}, radius));
            }
        }
    }
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_sphere_cast_aabb(const PhysicsBody3D& body, Vec3 origin,
                                                                               Vec3 direction, float max_distance,
                                                                               float radius) noexcept {
    const auto bounds = bounds_for(body);
    const auto closest_point = closest_point_on_bounds(bounds, origin);
    const auto initial_delta = origin - closest_point;
    if (dot(initial_delta, initial_delta) <= radius * radius) {
        return ExactSphereCastCandidate3D{
            .position = origin,
            .normal = normalize_or_fallback(initial_delta, fallback_normal(origin - body.position)),
            .distance = 0.0F,
            .initial_overlap = true,
        };
    }

    std::optional<ExactSphereCastCandidate3D> closest;
    if (direction.x > 0.0F) {
        consider_aabb_face_candidate(closest, origin, direction, max_distance, radius, bounds, 0, -1.0F);
    } else if (direction.x < 0.0F) {
        consider_aabb_face_candidate(closest, origin, direction, max_distance, radius, bounds, 0, 1.0F);
    }
    if (direction.y > 0.0F) {
        consider_aabb_face_candidate(closest, origin, direction, max_distance, radius, bounds, 1, -1.0F);
    } else if (direction.y < 0.0F) {
        consider_aabb_face_candidate(closest, origin, direction, max_distance, radius, bounds, 1, 1.0F);
    }
    if (direction.z > 0.0F) {
        consider_aabb_face_candidate(closest, origin, direction, max_distance, radius, bounds, 2, -1.0F);
    } else if (direction.z < 0.0F) {
        consider_aabb_face_candidate(closest, origin, direction, max_distance, radius, bounds, 2, 1.0F);
    }

    consider_aabb_edge_candidates(closest, origin, direction, max_distance, radius, bounds);
    consider_aabb_corner_candidates(closest, origin, direction, max_distance, radius, bounds);
    return closest;
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_sphere_cast_sphere(const PhysicsBody3D& body, Vec3 origin,
                                                                                 Vec3 direction, float max_distance,
                                                                                 float radius) noexcept {
    const auto radius_sum = body.radius + radius;
    const auto initial_delta = origin - body.position;
    if (dot(initial_delta, initial_delta) <= radius_sum * radius_sum) {
        return ExactSphereCastCandidate3D{
            .position = origin,
            .normal = normalize_or_fallback(initial_delta, fallback_normal(origin - body.position)),
            .distance = 0.0F,
            .initial_overlap = true,
        };
    }
    return raycast_sphere_surface(origin, direction, max_distance, body.position, radius_sum);
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_sphere_cast_capsule(const PhysicsBody3D& body,
                                                                                  Vec3 origin, Vec3 direction,
                                                                                  float max_distance,
                                                                                  float radius) noexcept {
    const auto radius_sum = body.radius + radius;
    const auto segment_minimum_y = body.position.y - body.half_height;
    const auto segment_maximum_y = body.position.y + body.half_height;
    const auto closest_axis_point = closest_point_on_capsule_axis(body, origin);
    const auto initial_delta = origin - closest_axis_point;
    if (dot(initial_delta, initial_delta) <= radius_sum * radius_sum) {
        return ExactSphereCastCandidate3D{
            .position = origin,
            .normal = normalize_or_fallback(initial_delta, fallback_normal(origin - body.position)),
            .distance = 0.0F,
            .initial_overlap = true,
        };
    }

    std::optional<ExactSphereCastCandidate3D> closest;
    keep_nearest_candidate(closest,
                           raycast_axis_cylinder(origin, direction, max_distance, 1, segment_minimum_y,
                                                 segment_maximum_y, body.position.x, body.position.z, radius_sum));
    keep_nearest_candidate(
        closest,
        raycast_sphere_surface(origin, direction, max_distance,
                               Vec3{.x = body.position.x, .y = segment_minimum_y, .z = body.position.z}, radius_sum));
    keep_nearest_candidate(
        closest,
        raycast_sphere_surface(origin, direction, max_distance,
                               Vec3{.x = body.position.x, .y = segment_maximum_y, .z = body.position.z}, radius_sum));
    return closest;
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_sphere_cast_body(const PhysicsBody3D& body, Vec3 origin,
                                                                               Vec3 direction, float max_distance,
                                                                               float radius) noexcept {
    if (body.shape == PhysicsShape3DKind::aabb) {
        return exact_sphere_cast_aabb(body, origin, direction, max_distance, radius);
    }
    if (body.shape == PhysicsShape3DKind::sphere) {
        return exact_sphere_cast_sphere(body, origin, direction, max_distance, radius);
    }
    if (body.shape == PhysicsShape3DKind::capsule) {
        return exact_sphere_cast_capsule(body, origin, direction, max_distance, radius);
    }
    return std::nullopt;
}

[[nodiscard]] PhysicsBody3D make_query_space_body(PhysicsShape3DKind shape, Vec3 position, Vec3 half_extents,
                                                  float radius, float half_height) noexcept {
    return PhysicsBody3D{
        .id = null_physics_body_3d,
        .position = position,
        .velocity = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .accumulated_force = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .inverse_mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = half_extents,
        .collision_enabled = true,
        .shape = shape,
        .radius = radius,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = half_height,
        .trigger = false,
    };
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_aabb_sweep_aabb(const PhysicsBody3D& body, Vec3 origin,
                                                                              Vec3 direction, float max_distance,
                                                                              Vec3 half_extents) noexcept {
    const auto target_bounds = bounds_for(body);
    const auto query_bounds = bounds_for(origin, half_extents);
    if (bounds_overlap(query_bounds, target_bounds)) {
        return ExactSphereCastCandidate3D{
            .position = origin,
            .normal = fallback_normal(origin - body.position),
            .distance = 0.0F,
            .initial_overlap = true,
        };
    }

    const auto hit = raycast_bounds(origin, direction, max_distance, expand_bounds(target_bounds, half_extents));
    if (!hit.has_value()) {
        return std::nullopt;
    }
    return ExactSphereCastCandidate3D{
        .position = hit->point, .normal = hit->normal, .distance = hit->distance, .initial_overlap = false};
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_aabb_sweep_sphere(const PhysicsBody3D& body, Vec3 origin,
                                                                                Vec3 direction, float max_distance,
                                                                                Vec3 half_extents) noexcept {
    const auto query_box =
        make_query_space_body(PhysicsShape3DKind::aabb, body.position, half_extents, body.radius, half_extents.y);
    return exact_sphere_cast_aabb(query_box, origin, direction, max_distance, body.radius);
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_aabb_sweep_capsule(const PhysicsBody3D& body, Vec3 origin,
                                                                                 Vec3 direction, float max_distance,
                                                                                 Vec3 half_extents) noexcept {
    const auto query_box =
        make_query_space_body(PhysicsShape3DKind::aabb, body.position,
                              Vec3{.x = half_extents.x, .y = body.half_height + half_extents.y, .z = half_extents.z},
                              body.radius, body.half_height + half_extents.y);
    return exact_sphere_cast_aabb(query_box, origin, direction, max_distance, body.radius);
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_aabb_sweep_body(const PhysicsBody3D& body, Vec3 origin,
                                                                              Vec3 direction, float max_distance,
                                                                              Vec3 half_extents) noexcept {
    if (body.shape == PhysicsShape3DKind::aabb) {
        return exact_aabb_sweep_aabb(body, origin, direction, max_distance, half_extents);
    }
    if (body.shape == PhysicsShape3DKind::sphere) {
        return exact_aabb_sweep_sphere(body, origin, direction, max_distance, half_extents);
    }
    if (body.shape == PhysicsShape3DKind::capsule) {
        return exact_aabb_sweep_capsule(body, origin, direction, max_distance, half_extents);
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_capsule_sweep_aabb(const PhysicsBody3D& body, Vec3 origin,
                                                                                 Vec3 direction, float max_distance,
                                                                                 float radius,
                                                                                 float half_height) noexcept {
    const auto query_box = make_query_space_body(
        PhysicsShape3DKind::aabb, body.position,
        Vec3{.x = body.half_extents.x, .y = body.half_extents.y + half_height, .z = body.half_extents.z}, radius,
        body.half_extents.y + half_height);
    return exact_sphere_cast_aabb(query_box, origin, direction, max_distance, radius);
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_capsule_sweep_sphere(const PhysicsBody3D& body,
                                                                                   Vec3 origin, Vec3 direction,
                                                                                   float max_distance, float radius,
                                                                                   float half_height) noexcept {
    const auto query_capsule = make_query_space_body(
        PhysicsShape3DKind::capsule, body.position,
        Vec3{.x = body.radius, .y = body.radius + half_height, .z = body.radius}, body.radius, half_height);
    return exact_sphere_cast_capsule(query_capsule, origin, direction, max_distance, radius);
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_capsule_sweep_capsule(const PhysicsBody3D& body,
                                                                                    Vec3 origin, Vec3 direction,
                                                                                    float max_distance, float radius,
                                                                                    float half_height) noexcept {
    const auto query_capsule = make_query_space_body(
        PhysicsShape3DKind::capsule, body.position,
        Vec3{.x = body.radius, .y = body.radius + body.half_height + half_height, .z = body.radius}, body.radius,
        body.half_height + half_height);
    return exact_sphere_cast_capsule(query_capsule, origin, direction, max_distance, radius);
}

[[nodiscard]] std::optional<ExactSphereCastCandidate3D> exact_capsule_sweep_body(const PhysicsBody3D& body, Vec3 origin,
                                                                                 Vec3 direction, float max_distance,
                                                                                 float radius,
                                                                                 float half_height) noexcept {
    if (body.shape == PhysicsShape3DKind::aabb) {
        return exact_capsule_sweep_aabb(body, origin, direction, max_distance, radius, half_height);
    }
    if (body.shape == PhysicsShape3DKind::sphere) {
        return exact_capsule_sweep_sphere(body, origin, direction, max_distance, radius, half_height);
    }
    if (body.shape == PhysicsShape3DKind::capsule) {
        return exact_capsule_sweep_capsule(body, origin, direction, max_distance, radius, half_height);
    }
    return std::nullopt;
}

[[nodiscard]] bool should_replace_exact_shape_hit(const PhysicsExactShapeSweep3DHit& candidate,
                                                  const PhysicsExactShapeSweep3DHit& closest) noexcept {
    constexpr auto distance_tie_epsilon = 0.00001F;
    if (candidate.distance < closest.distance - distance_tie_epsilon) {
        return true;
    }
    return std::fabs(candidate.distance - closest.distance) <= distance_tie_epsilon &&
           candidate.body.value < closest.body.value;
}

[[nodiscard]] bool is_valid_physics_shape_desc(const PhysicsShape3DDesc& desc) noexcept;

template <typename IncludeBody>
[[nodiscard]] PhysicsExactShapeSweep3DResult exact_shape_sweep_impl(const std::vector<PhysicsBody3D>& bodies,
                                                                    PhysicsExactShapeSweep3DDesc desc,
                                                                    IncludeBody include_body) {
    PhysicsExactShapeSweep3DResult result;

    if (!finite_vec(desc.origin) || !finite_vec(desc.direction) || !finite(desc.max_distance) ||
        desc.max_distance < 0.0F || !is_valid_physics_shape_desc(desc.shape)) {
        result.status = PhysicsExactShapeSweep3DStatus::invalid_request;
        result.diagnostic = PhysicsExactShapeSweep3DDiagnostic::invalid_request;
        return result;
    }

    const auto direction_length = length(desc.direction);
    if (direction_length <= 0.000001F) {
        result.status = PhysicsExactShapeSweep3DStatus::invalid_request;
        result.diagnostic = PhysicsExactShapeSweep3DDiagnostic::invalid_request;
        return result;
    }

    const auto direction = desc.direction * (1.0F / direction_length);
    std::optional<PhysicsExactShapeSweep3DHit> closest;
    for (const auto& body : bodies) {
        if (!body.collision_enabled || body.id == desc.filter.ignored_body ||
            (desc.filter.collision_mask & body.collision_layer) == 0U ||
            (!desc.filter.include_triggers && body.trigger) || !include_body(body)) {
            continue;
        }

        std::optional<ExactSphereCastCandidate3D> candidate;
        if (desc.shape.kind() == PhysicsShape3DKind::sphere) {
            candidate = exact_sphere_cast_body(body, desc.origin, direction, desc.max_distance, desc.shape.radius());
        } else if (desc.shape.kind() == PhysicsShape3DKind::aabb) {
            candidate =
                exact_aabb_sweep_body(body, desc.origin, direction, desc.max_distance, desc.shape.half_extents());
        } else if (desc.shape.kind() == PhysicsShape3DKind::capsule) {
            candidate = exact_capsule_sweep_body(body, desc.origin, direction, desc.max_distance, desc.shape.radius(),
                                                 desc.shape.half_height());
        }
        if (!candidate.has_value()) {
            continue;
        }

        const auto hit = PhysicsExactShapeSweep3DHit{
            .body = body.id,
            .position = candidate->position,
            .normal = candidate->normal,
            .distance = candidate->distance,
            .initial_overlap = candidate->initial_overlap,
        };
        if (!closest.has_value() || should_replace_exact_shape_hit(hit, *closest)) {
            closest = hit;
        }
    }

    result.diagnostic = PhysicsExactShapeSweep3DDiagnostic::none;
    if (closest.has_value()) {
        result.status = PhysicsExactShapeSweep3DStatus::hit;
        result.hit = closest;
        return result;
    }

    result.status = PhysicsExactShapeSweep3DStatus::no_hit;
    return result;
}

[[nodiscard]] std::optional<PhysicsContact3D> contact_for(const PhysicsBody3D& first, const PhysicsBody3D& second) {
    if (first.shape == PhysicsShape3DKind::aabb && second.shape == PhysicsShape3DKind::aabb) {
        return aabb_contact(first, second);
    }
    if (first.shape == PhysicsShape3DKind::sphere && second.shape == PhysicsShape3DKind::sphere) {
        return sphere_contact(first, second);
    }
    if (first.shape == PhysicsShape3DKind::sphere && second.shape == PhysicsShape3DKind::aabb) {
        return sphere_aabb_contact(first, second, true);
    }
    if (first.shape == PhysicsShape3DKind::aabb && second.shape == PhysicsShape3DKind::sphere) {
        return sphere_aabb_contact(second, first, false);
    }
    if (first.shape == PhysicsShape3DKind::capsule && second.shape == PhysicsShape3DKind::sphere) {
        return capsule_sphere_contact(first, second, true);
    }
    if (first.shape == PhysicsShape3DKind::sphere && second.shape == PhysicsShape3DKind::capsule) {
        return capsule_sphere_contact(second, first, false);
    }
    if (first.shape == PhysicsShape3DKind::capsule && second.shape == PhysicsShape3DKind::aabb) {
        return capsule_aabb_contact(first, second, true);
    }
    if (first.shape == PhysicsShape3DKind::aabb && second.shape == PhysicsShape3DKind::capsule) {
        return capsule_aabb_contact(second, first, false);
    }
    if (first.shape == PhysicsShape3DKind::capsule && second.shape == PhysicsShape3DKind::capsule) {
        return capsule_contact(first, second);
    }
    return std::nullopt;
}

[[nodiscard]] bool is_valid_shape_sweep_desc(const PhysicsShapeSweep3DDesc& desc) noexcept {
    if (!finite_vec(desc.origin) || !finite_vec(desc.direction) || !finite(desc.max_distance) ||
        desc.max_distance < 0.0F || !valid_shape(desc.shape)) {
        return false;
    }
    if (desc.shape == PhysicsShape3DKind::sphere) {
        return finite(desc.radius) && desc.radius > 0.0F;
    }
    if (desc.shape == PhysicsShape3DKind::capsule) {
        return finite(desc.radius) && desc.radius > 0.0F && finite(desc.half_height) && desc.half_height > 0.0F;
    }
    return finite_vec(desc.half_extents) && desc.half_extents.x > 0.0F && desc.half_extents.y > 0.0F &&
           desc.half_extents.z > 0.0F;
}

[[nodiscard]] bool is_valid_physics_shape_desc(const PhysicsShape3DDesc& desc) noexcept {
    if (!valid_shape(desc.kind())) {
        return false;
    }
    if (desc.kind() == PhysicsShape3DKind::sphere) {
        return finite(desc.radius()) && desc.radius() > 0.0F;
    }
    if (desc.kind() == PhysicsShape3DKind::capsule) {
        return finite(desc.radius()) && desc.radius() > 0.0F && finite(desc.half_height()) && desc.half_height() > 0.0F;
    }
    const auto half_extents = desc.half_extents();
    return finite_vec(half_extents) && half_extents.x > 0.0F && half_extents.y > 0.0F && half_extents.z > 0.0F;
}

[[nodiscard]] bool is_valid_continuous_step_config(const PhysicsContinuousStep3DConfig& config) noexcept {
    return finite(config.skin_width) && config.skin_width >= 0.0F;
}

[[nodiscard]] PhysicsShape3DDesc shape_desc_for_body(const PhysicsBody3D& body) noexcept {
    if (body.shape == PhysicsShape3DKind::sphere) {
        return PhysicsShape3DDesc::sphere(body.radius);
    }
    if (body.shape == PhysicsShape3DKind::capsule) {
        return PhysicsShape3DDesc::capsule(body.radius, body.half_height);
    }
    return PhysicsShape3DDesc::aabb(body.half_extents);
}

[[nodiscard]] bool is_valid_character_controller_desc(const PhysicsCharacterController3DDesc& desc) noexcept {
    return finite_vec(desc.position) && finite_vec(desc.displacement) && finite(desc.radius) && desc.radius > 0.0F &&
           finite(desc.half_height) && desc.half_height > 0.0F && finite(desc.skin_width) && desc.skin_width >= 0.0F &&
           desc.max_iterations > 0U && desc.max_iterations <= 16U && finite(desc.grounded_normal_y) &&
           desc.grounded_normal_y >= 0.0F && desc.grounded_normal_y <= 1.0F;
}

[[nodiscard]] bool is_valid_character_dynamic_policy_desc(const PhysicsCharacterDynamicPolicy3DDesc& desc) noexcept {
    return finite_vec(desc.position) && finite_vec(desc.displacement) && finite(desc.radius) && desc.radius > 0.0F &&
           finite(desc.half_height) && desc.half_height > 0.0F && desc.character_layer != 0U &&
           finite(desc.skin_width) && desc.skin_width >= 0.0F && finite(desc.step_height) && desc.step_height >= 0.0F &&
           finite(desc.ground_probe_distance) && desc.ground_probe_distance >= 0.0F && finite(desc.grounded_normal_y) &&
           desc.grounded_normal_y >= 0.0F && desc.grounded_normal_y <= 1.0F && finite(desc.max_slope_normal_y) &&
           desc.max_slope_normal_y >= 0.0F && desc.max_slope_normal_y <= 1.0F && finite(desc.dynamic_push_distance) &&
           desc.dynamic_push_distance >= 0.0F;
}

[[nodiscard]] PhysicsCharacterController3DContact make_character_controller_contact(const PhysicsShapeSweepHit3D& hit,
                                                                                    float grounded_normal_y) noexcept {
    return PhysicsCharacterController3DContact{
        .body = hit.body,
        .position = hit.position,
        .normal = hit.normal,
        .distance = hit.distance,
        .initial_overlap = hit.initial_overlap,
        .grounded = hit.normal.y >= grounded_normal_y,
    };
}

[[nodiscard]] PhysicsCharacterController3DStatus
character_controller_status_for(const PhysicsCharacterController3DResult& result) noexcept {
    if (result.contacts.empty()) {
        return PhysicsCharacterController3DStatus::moved;
    }
    if (length(result.applied_displacement) <= 0.000001F) {
        return PhysicsCharacterController3DStatus::blocked;
    }
    return PhysicsCharacterController3DStatus::constrained;
}

[[nodiscard]] PhysicsShape3DDesc
character_dynamic_policy_shape(const PhysicsCharacterDynamicPolicy3DDesc& desc) noexcept {
    return PhysicsShape3DDesc::capsule(desc.radius, desc.half_height);
}

[[nodiscard]] bool target_matches_character_dynamic_policy(const PhysicsCharacterDynamicPolicy3DDesc& desc,
                                                           const PhysicsBody3D& body) noexcept {
    return body.collision_enabled && (desc.collision_mask & body.collision_layer) != 0U &&
           (body.collision_mask & desc.character_layer) != 0U;
}

[[nodiscard]] PhysicsCharacterDynamicPolicy3DRowKind
character_dynamic_policy_kind_for(const PhysicsBody3D& body) noexcept {
    if (body.trigger) {
        return PhysicsCharacterDynamicPolicy3DRowKind::trigger_overlap;
    }
    if (body.dynamic) {
        return PhysicsCharacterDynamicPolicy3DRowKind::dynamic_push;
    }
    return PhysicsCharacterDynamicPolicy3DRowKind::solid_contact;
}

[[nodiscard]] Vec3 suggested_character_dynamic_push(const PhysicsCharacterDynamicPolicy3DDesc& desc,
                                                    const PhysicsExactShapeSweep3DHit& hit) noexcept {
    if (desc.dynamic_push_distance <= 0.000001F) {
        return Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    }

    auto horizontal = Vec3{.x = desc.displacement.x, .y = 0.0F, .z = desc.displacement.z};
    auto horizontal_length = length(horizontal);
    if (horizontal_length <= 0.000001F) {
        horizontal = Vec3{.x = -hit.normal.x, .y = 0.0F, .z = -hit.normal.z};
        horizontal_length = length(horizontal);
    }
    if (horizontal_length <= 0.000001F) {
        return Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    }
    return horizontal * (desc.dynamic_push_distance / horizontal_length);
}

[[nodiscard]] PhysicsCharacterDynamicPolicy3DRow
make_character_dynamic_policy_row(const PhysicsCharacterDynamicPolicy3DDesc& desc, const PhysicsBody3D& body,
                                  const PhysicsExactShapeSweep3DHit& hit) noexcept {
    const auto kind = character_dynamic_policy_kind_for(body);
    PhysicsCharacterDynamicPolicy3DRow row;
    row.kind = kind;
    row.body = body.id;
    row.position = hit.position;
    row.normal = hit.normal;
    row.distance = hit.distance;
    row.initial_overlap = hit.initial_overlap;
    row.grounded = hit.normal.y >= desc.grounded_normal_y;
    row.walkable_slope = hit.normal.y >= desc.max_slope_normal_y;
    if (kind == PhysicsCharacterDynamicPolicy3DRowKind::dynamic_push) {
        row.suggested_displacement = suggested_character_dynamic_push(desc, hit);
    }
    return row;
}

[[nodiscard]] PhysicsCharacterDynamicPolicy3DRow
make_character_dynamic_step_row(const PhysicsCharacterDynamicPolicy3DDesc& desc, PhysicsBody3DId body) noexcept {
    PhysicsCharacterDynamicPolicy3DRow row;
    row.kind = PhysicsCharacterDynamicPolicy3DRowKind::step_up;
    row.body = body;
    row.suggested_displacement = Vec3{.x = 0.0F, .y = desc.step_height, .z = 0.0F};
    return row;
}

struct CharacterDynamicPolicyHit3D {
    PhysicsCharacterDynamicPolicy3DRow row;
    bool blocking{false};
};

[[nodiscard]] bool should_replace_character_dynamic_policy_hit(const CharacterDynamicPolicyHit3D& candidate,
                                                               const CharacterDynamicPolicyHit3D& closest) noexcept {
    constexpr auto distance_tie_epsilon = 0.00001F;
    if (candidate.row.distance < closest.row.distance - distance_tie_epsilon) {
        return true;
    }
    return std::fabs(candidate.row.distance - closest.row.distance) <= distance_tie_epsilon &&
           candidate.row.body.value < closest.row.body.value;
}

[[nodiscard]] std::vector<CharacterDynamicPolicyHit3D> collect_character_dynamic_policy_hits(
    const std::vector<PhysicsBody3D>& bodies, const PhysicsCharacterDynamicPolicy3DDesc& desc, Vec3 origin,
    Vec3 direction, float max_distance, bool include_dynamic, bool include_triggers, bool include_static) {
    std::vector<CharacterDynamicPolicyHit3D> hits;
    for (const auto& body : bodies) {
        if (!target_matches_character_dynamic_policy(desc, body)) {
            continue;
        }
        if (body.trigger && !include_triggers) {
            continue;
        }
        if (body.trigger && !desc.include_triggers) {
            continue;
        }
        if (body.dynamic && !include_dynamic) {
            continue;
        }
        if (!body.dynamic && !body.trigger && !include_static) {
            continue;
        }

        const auto sweep =
            exact_shape_sweep_impl(bodies,
                                   PhysicsExactShapeSweep3DDesc{
                                       .origin = origin,
                                       .direction = direction,
                                       .max_distance = max_distance,
                                       .shape = character_dynamic_policy_shape(desc),
                                       .filter = PhysicsQueryFilter3D{.collision_mask = desc.collision_mask,
                                                                      .ignored_body = null_physics_body_3d,
                                                                      .include_triggers = true},
                                   },
                                   [id = body.id](const PhysicsBody3D& target) noexcept { return target.id == id; });
        if (sweep.status != PhysicsExactShapeSweep3DStatus::hit || !sweep.hit.has_value()) {
            continue;
        }

        const auto kind = character_dynamic_policy_kind_for(body);
        hits.push_back(CharacterDynamicPolicyHit3D{
            .row = make_character_dynamic_policy_row(desc, body, *sweep.hit),
            .blocking = kind == PhysicsCharacterDynamicPolicy3DRowKind::solid_contact,
        });
    }

    std::ranges::sort(hits, [](const CharacterDynamicPolicyHit3D& lhs, const CharacterDynamicPolicyHit3D& rhs) {
        if (lhs.row.distance != rhs.row.distance) {
            return lhs.row.distance < rhs.row.distance;
        }
        if (lhs.row.body.value != rhs.row.body.value) {
            return lhs.row.body.value < rhs.row.body.value;
        }
        return static_cast<int>(lhs.row.kind) < static_cast<int>(rhs.row.kind);
    });
    return hits;
}

[[nodiscard]] std::optional<CharacterDynamicPolicyHit3D>
nearest_blocking_character_dynamic_policy_hit(const std::vector<CharacterDynamicPolicyHit3D>& hits) noexcept {
    std::optional<CharacterDynamicPolicyHit3D> closest;
    for (const auto& hit : hits) {
        if (!hit.blocking) {
            continue;
        }
        if (!closest.has_value() || should_replace_character_dynamic_policy_hit(hit, *closest)) {
            closest = hit;
        }
    }
    return closest;
}

[[nodiscard]] bool character_dynamic_policy_step_is_clear(const std::vector<PhysicsBody3D>& bodies,
                                                          const PhysicsCharacterDynamicPolicy3DDesc& desc,
                                                          float displacement_distance) {
    if (desc.step_height <= 0.000001F || displacement_distance <= 0.000001F) {
        return false;
    }
    const auto raised_origin = desc.position + Vec3{.x = 0.0F, .y = desc.step_height, .z = 0.0F};
    const auto direction = desc.displacement * (1.0F / displacement_distance);
    const auto raised_hits = collect_character_dynamic_policy_hits(bodies, desc, raised_origin, direction,
                                                                   displacement_distance, false, false, true);
    if (nearest_blocking_character_dynamic_policy_hit(raised_hits).has_value()) {
        return false;
    }

    const auto final_origin = desc.position + desc.displacement;
    const auto final_hits = collect_character_dynamic_policy_hits(
        bodies, desc, final_origin, Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}, 0.0F, false, false, true);
    return !nearest_blocking_character_dynamic_policy_hit(final_hits).has_value();
}

[[nodiscard]] std::optional<PhysicsCharacterDynamicPolicy3DRow>
character_dynamic_policy_ground_probe(const std::vector<PhysicsBody3D>& bodies,
                                      const PhysicsCharacterDynamicPolicy3DDesc& desc, Vec3 position) {
    if (desc.ground_probe_distance <= 0.000001F) {
        return std::nullopt;
    }

    const auto hits = collect_character_dynamic_policy_hits(
        bodies, desc, position, Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F}, desc.ground_probe_distance, false, false, true);
    const auto ground = nearest_blocking_character_dynamic_policy_hit(hits);
    if (!ground.has_value()) {
        return std::nullopt;
    }

    auto row = ground->row;
    row.kind = PhysicsCharacterDynamicPolicy3DRowKind::ground_probe;
    row.grounded = row.normal.y >= desc.grounded_normal_y;
    row.walkable_slope = row.normal.y >= desc.max_slope_normal_y;
    return row;
}

[[nodiscard]] bool duplicate_authored_body_name(const std::vector<PhysicsAuthoredCollisionBody3DDesc>& bodies,
                                                std::size_t index) noexcept {
    for (std::size_t previous = 0; previous < index; ++previous) {
        if (bodies[previous].name == bodies[index].name) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool valid_joint_config(PhysicsJointSolve3DConfig config) noexcept {
    return config.iterations > 0U && config.iterations <= 64U && finite(config.tolerance) && config.tolerance >= 0.0F;
}

[[nodiscard]] bool valid_distance_joint_desc(const PhysicsDistanceJoint3DDesc& desc) noexcept {
    return !(desc.first == desc.second) && finite_vec(desc.local_anchor_first) &&
           finite_vec(desc.local_anchor_second) && finite(desc.rest_distance) && desc.rest_distance >= 0.0F;
}

[[nodiscard]] Vec3 distance_joint_anchor(const PhysicsBody3D& body, Vec3 local_anchor) noexcept {
    return body.position + local_anchor;
}

[[nodiscard]] float distance_joint_current_distance(const PhysicsBody3D& first, const PhysicsBody3D& second,
                                                    const PhysicsDistanceJoint3DDesc& joint) noexcept {
    return length(distance_joint_anchor(second, joint.local_anchor_second) -
                  distance_joint_anchor(first, joint.local_anchor_first));
}

struct DistanceJointWorkItem3D {
    std::size_t row_index{0};
    const PhysicsDistanceJoint3DDesc* joint{nullptr};
    PhysicsBody3D* first{nullptr};
    PhysicsBody3D* second{nullptr};
};

inline constexpr std::uint64_t physics_replay_hash_offset_3d = 1469598103934665603ULL;
inline constexpr std::uint64_t physics_replay_hash_prime_3d = 1099511628211ULL;

void append_replay_byte_3d(std::uint64_t& hash, std::uint8_t value) noexcept {
    hash ^= value;
    hash *= physics_replay_hash_prime_3d;
}

void append_replay_u64_3d(std::uint64_t& hash, std::uint64_t value) noexcept {
    for (std::uint32_t shift = 0; shift < 64U; shift += 8U) {
        append_replay_byte_3d(hash, static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
}

void append_replay_u32_3d(std::uint64_t& hash, std::uint32_t value) noexcept {
    append_replay_u64_3d(hash, value);
}

void append_replay_float_3d(std::uint64_t& hash, float value) noexcept {
    append_replay_u32_3d(hash, std::bit_cast<std::uint32_t>(value));
}

void append_replay_bool_3d(std::uint64_t& hash, bool value) noexcept {
    append_replay_byte_3d(hash, value ? std::uint8_t{1U} : std::uint8_t{0U});
}

void append_replay_vec3_3d(std::uint64_t& hash, Vec3 value) noexcept {
    append_replay_float_3d(hash, value.x);
    append_replay_float_3d(hash, value.y);
    append_replay_float_3d(hash, value.z);
}

void append_replay_world_config_3d(std::uint64_t& hash, const PhysicsWorld3DConfig& config) noexcept {
    append_replay_vec3_3d(hash, config.gravity);
}

void append_replay_body_3d(std::uint64_t& hash, const PhysicsBody3D& body) noexcept {
    append_replay_u32_3d(hash, body.id.value);
    append_replay_vec3_3d(hash, body.position);
    append_replay_vec3_3d(hash, body.velocity);
    append_replay_vec3_3d(hash, body.accumulated_force);
    append_replay_float_3d(hash, body.inverse_mass);
    append_replay_float_3d(hash, body.linear_damping);
    append_replay_bool_3d(hash, body.dynamic);
    append_replay_vec3_3d(hash, body.half_extents);
    append_replay_bool_3d(hash, body.collision_enabled);
    append_replay_u32_3d(hash, static_cast<std::uint32_t>(body.shape));
    append_replay_float_3d(hash, body.radius);
    append_replay_u32_3d(hash, body.collision_layer);
    append_replay_u32_3d(hash, body.collision_mask);
    append_replay_float_3d(hash, body.half_height);
    append_replay_bool_3d(hash, body.trigger);
}

[[nodiscard]] bool all_zero_determinism_gate_config_3d(PhysicsDeterminismGate3DConfig config) noexcept {
    return config.max_bodies == 0U && config.max_broadphase_pairs == 0U && config.max_contacts == 0U &&
           config.max_contact_manifolds == 0U && config.max_trigger_overlaps == 0U && config.max_contact_points == 0U;
}

[[nodiscard]] PhysicsDeterminismGate3DCounts count_determinism_gate_3d(const PhysicsWorld3D& world) {
    PhysicsDeterminismGate3DCounts counts;
    const auto pairs = world.broadphase_pairs();
    const auto contacts = world.contacts();
    const auto manifolds = world.contact_manifolds();
    const auto overlaps = world.trigger_overlaps();

    counts.bodies = static_cast<std::uint64_t>(world.bodies().size());
    counts.broadphase_pairs = static_cast<std::uint64_t>(pairs.size());
    counts.contacts = static_cast<std::uint64_t>(contacts.size());
    counts.contact_manifolds = static_cast<std::uint64_t>(manifolds.size());
    counts.trigger_overlaps = static_cast<std::uint64_t>(overlaps.size());
    for (const auto& manifold : manifolds) {
        counts.contact_points += static_cast<std::uint64_t>(manifold.points.size());
    }
    return counts;
}

[[nodiscard]] PhysicsDeterminismGate3DDiagnostic
exceeded_determinism_budget_3d(PhysicsDeterminismGate3DCounts counts, PhysicsDeterminismGate3DConfig config) noexcept {
    if (counts.bodies > config.max_bodies) {
        return PhysicsDeterminismGate3DDiagnostic::bodies_exceeded;
    }
    if (counts.broadphase_pairs > config.max_broadphase_pairs) {
        return PhysicsDeterminismGate3DDiagnostic::broadphase_pairs_exceeded;
    }
    if (counts.contacts > config.max_contacts) {
        return PhysicsDeterminismGate3DDiagnostic::contacts_exceeded;
    }
    if (counts.contact_manifolds > config.max_contact_manifolds) {
        return PhysicsDeterminismGate3DDiagnostic::contact_manifolds_exceeded;
    }
    if (counts.trigger_overlaps > config.max_trigger_overlaps) {
        return PhysicsDeterminismGate3DDiagnostic::trigger_overlaps_exceeded;
    }
    if (counts.contact_points > config.max_contact_points) {
        return PhysicsDeterminismGate3DDiagnostic::contact_points_exceeded;
    }
    return PhysicsDeterminismGate3DDiagnostic::none;
}

} // namespace

bool is_valid_physics_body_desc(const PhysicsBody3DDesc& desc) noexcept {
    if (!finite_vec(desc.position) || !finite_vec(desc.velocity) || !finite(desc.mass) ||
        !finite(desc.linear_damping) || desc.linear_damping < 0.0F || !finite_vec(desc.half_extents) ||
        desc.half_extents.x <= 0.0F || desc.half_extents.y <= 0.0F || desc.half_extents.z <= 0.0F ||
        !valid_shape(desc.shape) || !finite(desc.radius) || desc.radius <= 0.0F || !finite(desc.half_height) ||
        desc.half_height <= 0.0F || desc.collision_layer == 0U) {
        return false;
    }
    return desc.dynamic ? desc.mass > 0.0F : desc.mass >= 0.0F;
}

PhysicsWorld3D::PhysicsWorld3D(PhysicsWorld3DConfig config) : config_(config) {
    if (!finite_vec(config_.gravity)) {
        throw std::invalid_argument("3d physics world gravity is invalid");
    }
}

PhysicsBody3DId PhysicsWorld3D::create_body(const PhysicsBody3DDesc& desc) {
    if (!is_valid_physics_body_desc(desc)) {
        throw std::invalid_argument("3d physics body description is invalid");
    }

    const auto id = PhysicsBody3DId{static_cast<std::uint32_t>(bodies_.size() + 1U)};
    bodies_.push_back(PhysicsBody3D{
        .id = id,
        .position = desc.position,
        .velocity = desc.velocity,
        .accumulated_force = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .inverse_mass = inverse_mass_for(desc),
        .linear_damping = desc.linear_damping,
        .dynamic = desc.dynamic,
        .half_extents = desc.half_extents,
        .collision_enabled = desc.collision_enabled,
        .shape = desc.shape,
        .radius = desc.radius,
        .collision_layer = desc.collision_layer,
        .collision_mask = desc.collision_mask,
        .half_height = desc.half_height,
        .trigger = desc.trigger,
    });
    return id;
}

PhysicsBody3D* PhysicsWorld3D::find_body(PhysicsBody3DId id) noexcept {
    if (id == null_physics_body_3d || id.value > bodies_.size()) {
        return nullptr;
    }
    return &bodies_[id.value - 1U];
}

const PhysicsBody3D* PhysicsWorld3D::find_body(PhysicsBody3DId id) const noexcept {
    if (id == null_physics_body_3d || id.value > bodies_.size()) {
        return nullptr;
    }
    return &bodies_[id.value - 1U];
}

PhysicsWorld3DConfig PhysicsWorld3D::config() const noexcept {
    return config_;
}

const std::vector<PhysicsBody3D>& PhysicsWorld3D::bodies() const noexcept {
    return bodies_;
}

std::vector<PhysicsBroadphasePair3D> PhysicsWorld3D::broadphase_pairs() const {
    struct Candidate {
        std::size_t body_index{0};
        PhysicsBodyBounds3D bounds{};
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

    std::vector<PhysicsBroadphasePair3D> result;
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
                result.push_back(PhysicsBroadphasePair3D{.first = first.id, .second = second.id});
            } else {
                result.push_back(PhysicsBroadphasePair3D{.first = second.id, .second = first.id});
            }
        }
    }

    std::ranges::sort(result, [](const PhysicsBroadphasePair3D& lhs, const PhysicsBroadphasePair3D& rhs) {
        if (lhs.first.value != rhs.first.value) {
            return lhs.first.value < rhs.first.value;
        }
        return lhs.second.value < rhs.second.value;
    });
    const auto unique_end =
        std::ranges::unique(result, [](const PhysicsBroadphasePair3D& lhs, const PhysicsBroadphasePair3D& rhs) {
            return lhs.first == rhs.first && lhs.second == rhs.second;
        });
    result.erase(unique_end.begin(), result.end());
    return result;
}

std::vector<PhysicsContact3D> PhysicsWorld3D::contacts() const {
    std::vector<PhysicsContact3D> result;

    for (const auto& manifold : contact_manifolds()) {
        for (const auto& point : manifold.points) {
            result.push_back(PhysicsContact3D{
                .first = manifold.first,
                .second = manifold.second,
                .normal = manifold.normal,
                .penetration_depth = point.penetration_depth,
            });
        }
    }

    return result;
}

std::vector<PhysicsContactManifold3D> PhysicsWorld3D::contact_manifolds() const {
    std::vector<PhysicsContactManifold3D> result;

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
            result.push_back(make_single_point_manifold(*first, *second, *contact));
        }
    }

    std::ranges::sort(result, [](const PhysicsContactManifold3D& lhs, const PhysicsContactManifold3D& rhs) {
        if (lhs.first.value != rhs.first.value) {
            return lhs.first.value < rhs.first.value;
        }
        return lhs.second.value < rhs.second.value;
    });
    for (auto& manifold : result) {
        std::ranges::sort(manifold.points, [](const PhysicsContactPoint3D& lhs, const PhysicsContactPoint3D& rhs) {
            return lhs.feature_id < rhs.feature_id;
        });
    }
    return result;
}

std::vector<PhysicsTriggerOverlap3D> PhysicsWorld3D::trigger_overlaps() const {
    std::vector<PhysicsTriggerOverlap3D> result;

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
            result.push_back(PhysicsTriggerOverlap3D{.first = pair.first, .second = pair.second});
        }
    }

    return result;
}

std::optional<PhysicsRaycastHit3D> PhysicsWorld3D::raycast(PhysicsRaycast3DDesc desc) const {
    if (!finite_vec(desc.origin) || !finite_vec(desc.direction) || !finite(desc.max_distance) ||
        desc.max_distance < 0.0F) {
        return std::nullopt;
    }

    const auto direction_length = length(desc.direction);
    if (direction_length <= 0.000001F) {
        return std::nullopt;
    }

    const auto direction = desc.direction * (1.0F / direction_length);
    std::optional<PhysicsRaycastHit3D> closest;
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
            closest = PhysicsRaycastHit3D{
                .body = body.id, .point = hit->point, .normal = hit->normal, .distance = hit->distance};
        }
    }

    return closest;
}

std::optional<PhysicsShapeSweepHit3D> PhysicsWorld3D::shape_sweep(PhysicsShapeSweep3DDesc desc) const {
    if (!is_valid_shape_sweep_desc(desc)) {
        return std::nullopt;
    }

    const auto direction_length = length(desc.direction);
    const auto query_extents = bounds_extents_for(desc.shape, desc.half_extents, desc.radius, desc.half_height);
    const auto query_bounds = bounds_for(desc.origin, query_extents);
    std::optional<PhysicsShapeSweepHit3D> closest_initial;

    for (const auto& body : bodies_) {
        if (!body.collision_enabled || body.id == desc.ignored_body ||
            (desc.collision_mask & body.collision_layer) == 0U || (!desc.include_triggers && body.trigger)) {
            continue;
        }

        const auto target_bounds = bounds_for(body);
        if (bounds_overlap(query_bounds, target_bounds)) {
            const auto candidate = PhysicsShapeSweepHit3D{.body = body.id,
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
    std::optional<PhysicsShapeSweepHit3D> closest;
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

        const auto candidate = PhysicsShapeSweepHit3D{.body = body.id,
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

PhysicsExactShapeSweep3DResult PhysicsWorld3D::exact_shape_sweep(PhysicsExactShapeSweep3DDesc desc) const {
    return exact_shape_sweep_impl(bodies_, desc, [](const PhysicsBody3D&) noexcept { return true; });
}

PhysicsExactSphereCast3DResult PhysicsWorld3D::exact_sphere_cast(PhysicsExactSphereCast3DDesc desc) const {
    PhysicsExactSphereCast3DResult result;

    const auto sweep = exact_shape_sweep(PhysicsExactShapeSweep3DDesc{
        .origin = desc.origin,
        .direction = desc.direction,
        .max_distance = desc.max_distance,
        .shape = PhysicsShape3DDesc::sphere(desc.radius),
        .filter = PhysicsQueryFilter3D{.collision_mask = desc.collision_mask,
                                       .ignored_body = desc.ignored_body,
                                       .include_triggers = desc.include_triggers},
    });

    if (sweep.status == PhysicsExactShapeSweep3DStatus::invalid_request) {
        result.status = PhysicsExactSphereCast3DStatus::invalid_request;
        result.diagnostic = PhysicsExactSphereCast3DDiagnostic::invalid_request;
        return result;
    }

    result.diagnostic = PhysicsExactSphereCast3DDiagnostic::none;
    if (sweep.status == PhysicsExactShapeSweep3DStatus::hit && sweep.hit.has_value()) {
        result.status = PhysicsExactSphereCast3DStatus::hit;
        result.hit = PhysicsExactSphereCast3DHit{
            .body = sweep.hit->body,
            .position = sweep.hit->position,
            .normal = sweep.hit->normal,
            .distance = sweep.hit->distance,
            .initial_overlap = sweep.hit->initial_overlap,
        };
        return result;
    }

    result.status = PhysicsExactSphereCast3DStatus::no_hit;
    return result;
}

PhysicsContinuousStep3DResult PhysicsWorld3D::step_continuous(float delta_seconds,
                                                              PhysicsContinuousStep3DConfig config) {
    PhysicsContinuousStep3DResult result;
    if (!finite(delta_seconds) || delta_seconds < 0.0F) {
        result.status = PhysicsContinuousStep3DStatus::invalid_request;
        result.diagnostic = PhysicsContinuousStep3DDiagnostic::invalid_delta_seconds;
        return result;
    }
    if (!is_valid_continuous_step_config(config)) {
        result.status = PhysicsContinuousStep3DStatus::invalid_request;
        result.diagnostic = PhysicsContinuousStep3DDiagnostic::invalid_config;
        return result;
    }

    result.status = PhysicsContinuousStep3DStatus::stepped;
    result.diagnostic = PhysicsContinuousStep3DDiagnostic::none;

    for (auto& body : bodies_) {
        if (!body.dynamic) {
            body.accumulated_force = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
            continue;
        }

        const auto previous_position = body.position;
        const auto acceleration = config_.gravity + (body.accumulated_force * body.inverse_mass);
        body.velocity = body.velocity + (acceleration * delta_seconds);
        const auto damping = std::max(0.0F, 1.0F - (body.linear_damping * delta_seconds));
        body.velocity = body.velocity * damping;
        const auto attempted_displacement = body.velocity * delta_seconds;

        if (!body.collision_enabled || body.trigger) {
            body.position = body.position + attempted_displacement;
            body.accumulated_force = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
            continue;
        }

        PhysicsContinuousStep3DRow row;
        row.body = body.id;
        row.previous_position = previous_position;
        row.attempted_displacement = attempted_displacement;

        const auto displacement_length = length(attempted_displacement);
        if (displacement_length <= 0.000001F) {
            row.applied_displacement = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
            row.remaining_displacement = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
            result.rows.push_back(row);
            body.accumulated_force = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
            continue;
        }

        const auto direction = attempted_displacement * (1.0F / displacement_length);
        const auto moving_layer = body.collision_layer;
        const auto sweep =
            exact_shape_sweep_impl(bodies_,
                                   PhysicsExactShapeSweep3DDesc{
                                       .origin = previous_position,
                                       .direction = direction,
                                       .max_distance = displacement_length,
                                       .shape = shape_desc_for_body(body),
                                       .filter = PhysicsQueryFilter3D{.collision_mask = body.collision_mask,
                                                                      .ignored_body = body.id,
                                                                      .include_triggers = config.include_triggers},
                                   },
                                   [moving_layer](const PhysicsBody3D& target) noexcept {
                                       return !target.dynamic && (target.collision_mask & moving_layer) != 0U;
                                   });

        if (sweep.status == PhysicsExactShapeSweep3DStatus::hit && sweep.hit.has_value()) {
            const auto applied_distance = std::max(0.0F, sweep.hit->distance - config.skin_width);
            row.applied_displacement = direction * applied_distance;
            row.remaining_displacement = attempted_displacement - row.applied_displacement;
            row.hit_body = sweep.hit->body;
            row.hit = sweep.hit;
            row.ccd_applied = true;
            body.position = previous_position + row.applied_displacement;

            const auto normal_component = std::min(0.0F, dot(body.velocity, sweep.hit->normal));
            body.velocity = body.velocity - (sweep.hit->normal * normal_component);
        } else {
            row.applied_displacement = attempted_displacement;
            row.remaining_displacement = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
            body.position = previous_position + attempted_displacement;
        }

        result.rows.push_back(row);
        body.accumulated_force = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    }

    return result;
}

PhysicsCharacterController3DResult move_physics_character_controller_3d(const PhysicsWorld3D& world,
                                                                        const PhysicsCharacterController3DDesc& desc) {
    PhysicsCharacterController3DResult result;
    result.position = desc.position;
    result.remaining_displacement = desc.displacement;

    if (!is_valid_character_controller_desc(desc)) {
        result.status = PhysicsCharacterController3DStatus::invalid_request;
        result.diagnostic = PhysicsCharacterController3DDiagnostic::invalid_request;
        return result;
    }

    const auto initial_overlap = world.shape_sweep(PhysicsShapeSweep3DDesc{
        .origin = desc.position,
        .direction = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 0.0F,
        .shape = PhysicsShape3DKind::capsule,
        .half_extents = Vec3{.x = desc.radius, .y = desc.half_height + desc.radius, .z = desc.radius},
        .radius = desc.radius,
        .half_height = desc.half_height,
        .collision_mask = desc.collision_mask,
        .ignored_body = null_physics_body_3d,
        .include_triggers = desc.include_triggers,
    });
    if (initial_overlap.has_value() && initial_overlap->initial_overlap) {
        auto contact = make_character_controller_contact(*initial_overlap, desc.grounded_normal_y);
        result.grounded = contact.grounded;
        result.contacts.push_back(contact);
        result.status = PhysicsCharacterController3DStatus::initial_overlap;
        result.diagnostic = PhysicsCharacterController3DDiagnostic::initial_overlap;
        return result;
    }

    auto position = desc.position;
    auto remaining = desc.displacement;
    Vec3 applied{.x = 0.0F, .y = 0.0F, .z = 0.0F};

    for (std::uint32_t iteration = 0; iteration < desc.max_iterations; ++iteration) {
        const auto remaining_distance = length(remaining);
        if (remaining_distance <= 0.000001F) {
            remaining = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
            break;
        }

        const auto direction = remaining * (1.0F / remaining_distance);
        const auto hit = world.shape_sweep(PhysicsShapeSweep3DDesc{
            .origin = position,
            .direction = direction,
            .max_distance = remaining_distance,
            .shape = PhysicsShape3DKind::capsule,
            .half_extents = Vec3{.x = desc.radius, .y = desc.half_height + desc.radius, .z = desc.radius},
            .radius = desc.radius,
            .half_height = desc.half_height,
            .collision_mask = desc.collision_mask,
            .ignored_body = null_physics_body_3d,
            .include_triggers = desc.include_triggers,
        });

        if (!hit.has_value()) {
            position = position + remaining;
            applied = applied + remaining;
            remaining = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
            break;
        }

        if (hit->initial_overlap) {
            auto contact = make_character_controller_contact(*hit, desc.grounded_normal_y);
            result.grounded = result.grounded || contact.grounded;
            result.contacts.push_back(contact);
            result.position = position;
            result.applied_displacement = applied;
            result.remaining_displacement = remaining;
            result.status = PhysicsCharacterController3DStatus::initial_overlap;
            result.diagnostic = PhysicsCharacterController3DDiagnostic::initial_overlap;
            return result;
        }

        const auto safe_distance = std::max(0.0F, hit->distance - desc.skin_width);
        const auto step = direction * safe_distance;
        position = position + step;
        applied = applied + step;

        auto contact = make_character_controller_contact(*hit, desc.grounded_normal_y);
        result.grounded = result.grounded || contact.grounded;
        result.contacts.push_back(contact);

        auto leftover = remaining - (direction * hit->distance);
        const auto normal_component = dot(leftover, hit->normal);
        if (normal_component < 0.0F) {
            leftover = leftover - (hit->normal * normal_component);
        }
        remaining = leftover;
    }

    result.position = position;
    result.applied_displacement = applied;
    result.remaining_displacement = remaining;

    if (length(remaining) > 0.000001F) {
        result.status = PhysicsCharacterController3DStatus::blocked;
        result.diagnostic = PhysicsCharacterController3DDiagnostic::iteration_limit;
        return result;
    }

    result.status = character_controller_status_for(result);
    result.diagnostic = PhysicsCharacterController3DDiagnostic::none;
    return result;
}

PhysicsCharacterDynamicPolicy3DResult
evaluate_physics_character_dynamic_policy_3d(const PhysicsWorld3D& world,
                                             const PhysicsCharacterDynamicPolicy3DDesc& desc) {
    PhysicsCharacterDynamicPolicy3DResult result;
    result.position = desc.position;
    result.remaining_displacement = desc.displacement;

    if (!is_valid_character_dynamic_policy_desc(desc)) {
        result.status = PhysicsCharacterDynamicPolicy3DStatus::invalid_request;
        result.diagnostic = PhysicsCharacterDynamicPolicy3DDiagnostic::invalid_request;
        return result;
    }

    const auto initial_hits = collect_character_dynamic_policy_hits(world.bodies(), desc, desc.position,
                                                                    Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}, 0.0F, true,
                                                                    desc.include_triggers, true);
    const auto initial_blocking = nearest_blocking_character_dynamic_policy_hit(initial_hits);
    if (initial_blocking.has_value()) {
        result.rows.push_back(initial_blocking->row);
        result.grounded = initial_blocking->row.grounded;
        result.status = PhysicsCharacterDynamicPolicy3DStatus::initial_overlap;
        result.diagnostic = PhysicsCharacterDynamicPolicy3DDiagnostic::initial_overlap;
        return result;
    }

    const auto displacement_distance = length(desc.displacement);
    if (displacement_distance <= 0.000001F) {
        result.applied_displacement = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
        result.remaining_displacement = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
        if (auto ground = character_dynamic_policy_ground_probe(world.bodies(), desc, desc.position);
            ground.has_value()) {
            result.grounded = ground->grounded;
            result.rows.push_back(*ground);
        }
        result.status = PhysicsCharacterDynamicPolicy3DStatus::moved;
        result.diagnostic = PhysicsCharacterDynamicPolicy3DDiagnostic::none;
        return result;
    }

    const auto direction = desc.displacement * (1.0F / displacement_distance);
    const auto hits = collect_character_dynamic_policy_hits(world.bodies(), desc, desc.position, direction,
                                                            displacement_distance, true, desc.include_triggers, true);
    const auto blocking_hit = nearest_blocking_character_dynamic_policy_hit(hits);
    const auto blocking_distance = blocking_hit.has_value() ? blocking_hit->row.distance : displacement_distance;

    for (const auto& hit : hits) {
        if (hit.blocking) {
            continue;
        }
        if (hit.row.distance <= blocking_distance + 0.00001F) {
            result.rows.push_back(hit.row);
        }
    }

    if (!blocking_hit.has_value()) {
        result.position = desc.position + desc.displacement;
        result.applied_displacement = desc.displacement;
        result.remaining_displacement = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
        if (auto ground = character_dynamic_policy_ground_probe(world.bodies(), desc, result.position);
            ground.has_value()) {
            result.grounded = ground->grounded;
            result.rows.push_back(*ground);
        }
        result.status = PhysicsCharacterDynamicPolicy3DStatus::moved;
        result.diagnostic = PhysicsCharacterDynamicPolicy3DDiagnostic::none;
        return result;
    }

    result.rows.push_back(blocking_hit->row);
    result.grounded = blocking_hit->row.grounded;
    if (blocking_hit->row.initial_overlap) {
        result.status = PhysicsCharacterDynamicPolicy3DStatus::initial_overlap;
        result.diagnostic = PhysicsCharacterDynamicPolicy3DDiagnostic::initial_overlap;
        return result;
    }

    const auto step_requested = desc.step_height > 0.000001F;
    if (character_dynamic_policy_step_is_clear(world.bodies(), desc, displacement_distance)) {
        result.position = desc.position + desc.displacement;
        result.applied_displacement = desc.displacement;
        result.remaining_displacement = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
        result.stepped = true;
        result.rows.push_back(make_character_dynamic_step_row(desc, blocking_hit->row.body));
        if (auto ground = character_dynamic_policy_ground_probe(world.bodies(), desc, result.position);
            ground.has_value()) {
            result.grounded = ground->grounded;
            result.rows.push_back(*ground);
        }
        result.status = PhysicsCharacterDynamicPolicy3DStatus::stepped;
        result.diagnostic = PhysicsCharacterDynamicPolicy3DDiagnostic::none;
        return result;
    }

    const auto applied_distance = std::max(0.0F, blocking_hit->row.distance - desc.skin_width);
    result.applied_displacement = direction * applied_distance;
    result.remaining_displacement = desc.displacement - result.applied_displacement;
    result.position = desc.position + result.applied_displacement;
    if (auto ground = character_dynamic_policy_ground_probe(world.bodies(), desc, result.position);
        ground.has_value()) {
        result.grounded = ground->grounded;
        result.rows.push_back(*ground);
    }
    result.status = PhysicsCharacterDynamicPolicy3DStatus::constrained;
    result.diagnostic = step_requested ? PhysicsCharacterDynamicPolicy3DDiagnostic::step_blocked
                                       : PhysicsCharacterDynamicPolicy3DDiagnostic::none;
    return result;
}

PhysicsJointSolve3DResult solve_physics_joints_3d(PhysicsWorld3D& world, const PhysicsJointSolve3DDesc& desc) {
    PhysicsJointSolve3DResult result;

    if (!valid_joint_config(desc.config)) {
        result.status = PhysicsJoint3DStatus::invalid_request;
        result.diagnostic = PhysicsJoint3DDiagnostic::invalid_config;
        return result;
    }

    result.rows.reserve(desc.distance_joints.size());
    std::vector<DistanceJointWorkItem3D> work_items;
    work_items.reserve(desc.distance_joints.size());
    bool invalid_request = false;

    auto reject = [&invalid_request, &result](PhysicsJoint3DDiagnostic diagnostic) {
        if (!invalid_request) {
            invalid_request = true;
            result.diagnostic = diagnostic;
        }
    };

    for (std::size_t index = 0; index < desc.distance_joints.size(); ++index) {
        const auto& joint = desc.distance_joints[index];
        const auto row_index = result.rows.size();
        PhysicsJointSolve3DRow row;
        row.source_index = index;
        row.first = joint.first;
        row.second = joint.second;
        row.target_distance = joint.rest_distance;

        if (!valid_distance_joint_desc(joint)) {
            row.diagnostic = PhysicsJoint3DDiagnostic::invalid_joint;
            result.rows.push_back(row);
            reject(PhysicsJoint3DDiagnostic::invalid_joint);
            continue;
        }

        auto* first = world.find_body(joint.first);
        auto* second = world.find_body(joint.second);
        if (first == nullptr || second == nullptr) {
            row.diagnostic = PhysicsJoint3DDiagnostic::missing_body;
            result.rows.push_back(row);
            reject(PhysicsJoint3DDiagnostic::missing_body);
            continue;
        }

        row.previous_distance = distance_joint_current_distance(*first, *second, joint);
        row.residual_distance = row.previous_distance - row.target_distance;
        if (first->inverse_mass <= 0.0F && second->inverse_mass <= 0.0F) {
            row.diagnostic = PhysicsJoint3DDiagnostic::static_pair;
            result.rows.push_back(row);
            reject(PhysicsJoint3DDiagnostic::static_pair);
            continue;
        }

        if (!joint.enabled) {
            row.diagnostic = PhysicsJoint3DDiagnostic::disabled_joint;
            result.rows.push_back(row);
            continue;
        }

        result.rows.push_back(row);
        work_items.push_back(
            DistanceJointWorkItem3D{.row_index = row_index, .joint = &joint, .first = first, .second = second});
    }

    if (invalid_request) {
        result.status = PhysicsJoint3DStatus::invalid_request;
        return result;
    }

    for (std::uint32_t iteration = 0; iteration < desc.config.iterations; ++iteration) {
        for (const auto& item : work_items) {
            auto& row = result.rows[item.row_index];
            const auto first_anchor = distance_joint_anchor(*item.first, item.joint->local_anchor_first);
            const auto second_anchor = distance_joint_anchor(*item.second, item.joint->local_anchor_second);
            const auto delta = second_anchor - first_anchor;
            const auto current_distance = length(delta);
            const auto error = current_distance - item.joint->rest_distance;
            if (std::fabs(error) <= desc.config.tolerance) {
                continue;
            }

            const auto direction = normalize_or_fallback(delta, Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
            const auto total_inverse_mass = item.first->inverse_mass + item.second->inverse_mass;
            if (total_inverse_mass <= 0.0F) {
                continue;
            }

            const auto correction = direction * error;
            const auto first_correction = correction * (item.first->inverse_mass / total_inverse_mass);
            const auto second_correction = correction * (-item.second->inverse_mass / total_inverse_mass);
            item.first->position = item.first->position + first_correction;
            item.second->position = item.second->position + second_correction;
            row.first_correction = row.first_correction + first_correction;
            row.second_correction = row.second_correction + second_correction;
        }
    }

    for (const auto& item : work_items) {
        auto& row = result.rows[item.row_index];
        row.residual_distance =
            distance_joint_current_distance(*item.first, *item.second, *item.joint) - row.target_distance;
    }

    result.status = PhysicsJoint3DStatus::solved;
    result.diagnostic = PhysicsJoint3DDiagnostic::none;
    return result;
}

PhysicsReplaySignature3D make_physics_replay_signature_3d(const PhysicsWorld3D& world) {
    PhysicsReplaySignature3D signature;
    signature.value = physics_replay_hash_offset_3d;
    signature.body_count = world.bodies().size();

    append_replay_world_config_3d(signature.value, world.config());
    append_replay_u64_3d(signature.value, static_cast<std::uint64_t>(signature.body_count));
    for (const auto& body : world.bodies()) {
        append_replay_body_3d(signature.value, body);
    }

    return signature;
}

PhysicsDeterminismGate3DResult evaluate_physics_determinism_gate_3d(const PhysicsWorld3D& world,
                                                                    const PhysicsDeterminismGate3DConfig& config) {
    PhysicsDeterminismGate3DResult result;
    result.counts = count_determinism_gate_3d(world);
    result.replay_signature = make_physics_replay_signature_3d(world);

    if (all_zero_determinism_gate_config_3d(config)) {
        result.status = PhysicsDeterminismGate3DStatus::invalid_request;
        result.diagnostic = PhysicsDeterminismGate3DDiagnostic::invalid_config;
        return result;
    }

    const auto exceeded = exceeded_determinism_budget_3d(result.counts, config);
    if (exceeded != PhysicsDeterminismGate3DDiagnostic::none) {
        result.status = PhysicsDeterminismGate3DStatus::budget_exceeded;
        result.diagnostic = exceeded;
        return result;
    }

    result.status = PhysicsDeterminismGate3DStatus::passed;
    result.diagnostic = PhysicsDeterminismGate3DDiagnostic::none;
    return result;
}

PhysicsAuthoredCollisionScene3DBuildResult
build_physics_world_3d_from_authored_collision_scene(const PhysicsAuthoredCollisionScene3DDesc& desc) {
    PhysicsAuthoredCollisionScene3DBuildResult result;

    if (desc.require_native_backend) {
        result.status = PhysicsAuthoredCollision3DBuildStatus::unsupported_native_backend;
        result.diagnostic = PhysicsAuthoredCollision3DDiagnostic::native_backend_unsupported;
        return result;
    }

    if (!finite_vec(desc.world_config.gravity)) {
        result.status = PhysicsAuthoredCollision3DBuildStatus::invalid_request;
        result.diagnostic = PhysicsAuthoredCollision3DDiagnostic::invalid_world_gravity;
        return result;
    }

    for (std::size_t index = 0; index < desc.bodies.size(); ++index) {
        const auto& authored = desc.bodies[index];
        if (authored.name.empty()) {
            result.status = PhysicsAuthoredCollision3DBuildStatus::invalid_body;
            result.diagnostic = PhysicsAuthoredCollision3DDiagnostic::invalid_body_name;
            result.body_index = index;
            return result;
        }
        if (duplicate_authored_body_name(desc.bodies, index)) {
            result.status = PhysicsAuthoredCollision3DBuildStatus::duplicate_name;
            result.diagnostic = PhysicsAuthoredCollision3DDiagnostic::duplicate_body_name;
            result.body_index = index;
            return result;
        }
        if (!is_valid_physics_body_desc(authored.body)) {
            result.status = PhysicsAuthoredCollision3DBuildStatus::invalid_body;
            result.diagnostic = PhysicsAuthoredCollision3DDiagnostic::invalid_body_desc;
            result.body_index = index;
            return result;
        }
    }

    PhysicsWorld3D world(desc.world_config);
    result.bodies.reserve(desc.bodies.size());
    for (std::size_t index = 0; index < desc.bodies.size(); ++index) {
        const auto& authored = desc.bodies[index];
        const auto body = world.create_body(authored.body);
        result.bodies.push_back(
            PhysicsAuthoredCollisionBody3DRow{.name = authored.name, .body = body, .source_index = index});
    }

    result.world = std::move(world);
    result.status = PhysicsAuthoredCollision3DBuildStatus::success;
    result.diagnostic = PhysicsAuthoredCollision3DDiagnostic::none;
    result.body_index = 0U;
    return result;
}

void PhysicsWorld3D::apply_force(PhysicsBody3DId id, Vec3 force) {
    if (!finite_vec(force)) {
        throw std::invalid_argument("3d physics force is invalid");
    }
    auto* body = find_body(id);
    if (body == nullptr) {
        throw std::invalid_argument("3d physics body does not exist");
    }
    if (!body->dynamic) {
        return;
    }
    body->accumulated_force = body->accumulated_force + force;
}

void PhysicsWorld3D::step(float delta_seconds) {
    if (!finite(delta_seconds) || delta_seconds < 0.0F) {
        throw std::invalid_argument("3d physics delta seconds is invalid");
    }

    for (auto& body : bodies_) {
        if (!body.dynamic) {
            body.accumulated_force = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
            continue;
        }

        const auto acceleration = config_.gravity + (body.accumulated_force * body.inverse_mass);
        body.velocity = body.velocity + (acceleration * delta_seconds);
        const auto damping = std::max(0.0F, 1.0F - (body.linear_damping * delta_seconds));
        body.velocity = body.velocity * damping;
        body.position = body.position + (body.velocity * delta_seconds);
        body.accumulated_force = Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    }
}

void PhysicsWorld3D::resolve_contacts(float restitution) {
    resolve_contacts(PhysicsContactSolver3DConfig{.restitution = restitution, .iterations = 1U});
}

void PhysicsWorld3D::resolve_contacts(PhysicsContactSolver3DConfig config) {
    if (!finite(config.restitution) || config.restitution < 0.0F || config.restitution > 1.0F ||
        config.iterations == 0U || config.iterations > 64U || !finite(config.position_correction_percent) ||
        config.position_correction_percent < 0.0F || config.position_correction_percent > 1.0F ||
        !finite(config.penetration_slop) || config.penetration_slop < 0.0F) {
        throw std::invalid_argument("3d physics contact solver config is invalid");
    }

    for (std::uint32_t iteration = 0; iteration < config.iterations; ++iteration) {
        const auto pending_manifolds = contact_manifolds();
        if (pending_manifolds.empty()) {
            return;
        }

        for (const auto& manifold : pending_manifolds) {
            auto* first = find_body(manifold.first);
            auto* second = find_body(manifold.second);
            if (first == nullptr || second == nullptr) {
                continue;
            }

            const auto total_inverse_mass = first->inverse_mass + second->inverse_mass;
            if (total_inverse_mass <= 0.0F) {
                continue;
            }

            const auto first_position_share = first->inverse_mass / total_inverse_mass;
            const auto second_position_share = second->inverse_mass / total_inverse_mass;
            for (const auto& point : manifold.points) {
                const auto correction_depth = std::max(0.0F, point.penetration_depth - config.penetration_slop) *
                                              config.position_correction_percent;
                first->position = first->position - (manifold.normal * (correction_depth * first_position_share));
                second->position = second->position + (manifold.normal * (correction_depth * second_position_share));

                const auto relative_velocity = second->velocity - first->velocity;
                const auto contact_velocity = dot(relative_velocity, manifold.normal);
                if (contact_velocity >= 0.0F) {
                    continue;
                }

                const auto impulse_magnitude = -(1.0F + config.restitution) * contact_velocity / total_inverse_mass;
                const auto impulse = manifold.normal * impulse_magnitude;
                first->velocity = first->velocity - (impulse * first->inverse_mass);
                second->velocity = second->velocity + (impulse * second->inverse_mass);
            }
        }
    }
}

} // namespace mirakana
