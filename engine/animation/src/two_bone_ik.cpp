// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/two_bone_ik.hpp"
#include "mirakana/math/vec.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace mirakana {
namespace {

constexpr float epsilon = 1.0e-6F;
constexpr float reach_slack = 1.0e-4F;

[[nodiscard]] bool finite_vec(Vec2 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

[[nodiscard]] bool finite_vec(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] float cross_z(Vec2 lhs, Vec2 rhs) noexcept {
    return (lhs.x * rhs.y) - (lhs.y * rhs.x);
}

[[nodiscard]] Vec3 normalized_or_zero(Vec3 value) noexcept {
    const auto value_length = length(value);
    if (!std::isfinite(value_length) || value_length <= epsilon) {
        return Vec3{};
    }
    return value * (1.0F / value_length);
}

[[nodiscard]] AnimationTwoBoneIk3dJointFrame make_joint_frame(Vec3 x_axis, Vec3 plane_normal) noexcept {
    AnimationTwoBoneIk3dJointFrame frame;
    frame.x_axis = normalized_or_zero(x_axis);
    frame.y_axis = normalized_or_zero(cross(plane_normal, frame.x_axis));
    frame.z_axis = normalized_or_zero(cross(frame.x_axis, frame.y_axis));
    return frame;
}

[[nodiscard]] bool resolve_bend_side(const AnimationTwoBoneIkXyDesc& desc, AnimationTwoBoneIkXyBendSide& bend_side,
                                     std::string& diagnostic) {
    switch (desc.bend_side) {
    case AnimationTwoBoneIkXyBendSide::positive:
    case AnimationTwoBoneIkXyBendSide::negative:
        bend_side = desc.bend_side;
        break;
    default:
        diagnostic = "two-bone IK bend side is invalid";
        return false;
    }

    if (!desc.pole_vector_xy.has_value()) {
        return true;
    }

    const auto pole = *desc.pole_vector_xy;
    if (!finite_vec(pole)) {
        diagnostic = "two-bone IK pole vector must be finite";
        return false;
    }
    const auto pole_length_squared = (pole.x * pole.x) + (pole.y * pole.y);
    if (pole_length_squared <= epsilon * epsilon) {
        diagnostic = "two-bone IK pole vector must be non-zero";
        return false;
    }

    const auto side = cross_z(desc.target_xy, pole);
    if (!std::isfinite(side) || std::abs(side) <= epsilon) {
        diagnostic = "two-bone IK pole vector must not be collinear with the target";
        return false;
    }
    bend_side = side > 0.0F ? AnimationTwoBoneIkXyBendSide::positive : AnimationTwoBoneIkXyBendSide::negative;
    return true;
}

[[nodiscard]] float elbow_sine_sign(AnimationTwoBoneIkXyBendSide bend_side) noexcept {
    return bend_side == AnimationTwoBoneIkXyBendSide::positive ? -1.0F : 1.0F;
}

} // namespace

bool solve_animation_two_bone_ik_xy_plane(const AnimationTwoBoneIkXyDesc& desc, AnimationTwoBoneIkXySolution& out,
                                          std::string& diagnostic) {
    out = AnimationTwoBoneIkXySolution{};
    diagnostic.clear();
    if (!std::isfinite(desc.parent_bone_length) || !std::isfinite(desc.child_bone_length) ||
        !finite_vec(desc.target_xy)) {
        diagnostic = "two-bone IK inputs must be finite";
        return false;
    }
    if (desc.parent_bone_length <= 0.0F || desc.child_bone_length <= 0.0F) {
        diagnostic = "two-bone IK bone lengths must be strictly positive";
        return false;
    }

    const auto x = desc.target_xy.x;
    const auto y = desc.target_xy.y;
    const auto target_distance = std::sqrt((x * x) + (y * y));
    if (!std::isfinite(target_distance) || target_distance < epsilon) {
        diagnostic = "two-bone IK target is too close to the origin";
        return false;
    }

    AnimationTwoBoneIkXyBendSide bend_side{};
    if (!resolve_bend_side(desc, bend_side, diagnostic)) {
        return false;
    }

    const auto parent_length = desc.parent_bone_length;
    const auto child_length = desc.child_bone_length;
    const auto max_reach = parent_length + child_length;
    const auto min_reach = std::abs(parent_length - child_length);
    if (target_distance > max_reach + reach_slack || target_distance < min_reach - reach_slack) {
        diagnostic = "two-bone IK target is outside the reachable annulus";
        return false;
    }

    const auto c2 =
        ((target_distance * target_distance) - (parent_length * parent_length) - (child_length * child_length)) /
        (2.0F * parent_length * child_length);
    if (c2 < -1.0F - 1.0e-4F || c2 > 1.0F + 1.0e-4F) {
        diagnostic = "two-bone IK cosine law produced an out-of-range value";
        return false;
    }
    const auto c2c = std::clamp(c2, -1.0F, 1.0F);
    const auto s2_magnitude = std::sqrt(std::max(0.0F, 1.0F - (c2c * c2c)));
    const auto s2 = elbow_sine_sign(bend_side) * s2_magnitude;
    const auto theta2 = std::atan2(s2, c2c);
    const auto k1 = parent_length + (child_length * c2c);
    const auto k2 = child_length * s2;
    const auto theta1 = std::atan2(y, x) - std::atan2(k2, k1);

    out.parent_rotation_z_radians = theta1;
    out.child_rotation_z_radians = theta2;
    return true;
}

bool solve_animation_two_bone_ik_3d_orientation(const AnimationTwoBoneIk3dDesc& desc, AnimationTwoBoneIk3dSolution& out,
                                                std::string& diagnostic) {
    out = AnimationTwoBoneIk3dSolution{};
    diagnostic.clear();
    if (!std::isfinite(desc.parent_bone_length) || !std::isfinite(desc.child_bone_length) || !finite_vec(desc.target) ||
        !finite_vec(desc.pole_vector)) {
        diagnostic = "two-bone IK 3D inputs must be finite";
        return false;
    }
    if (desc.parent_bone_length <= 0.0F || desc.child_bone_length <= 0.0F) {
        diagnostic = "two-bone IK 3D bone lengths must be strictly positive";
        return false;
    }

    const auto target_distance = length(desc.target);
    if (!std::isfinite(target_distance) || target_distance < epsilon) {
        diagnostic = "two-bone IK 3D target is too close to the origin";
        return false;
    }

    const auto parent_length = desc.parent_bone_length;
    const auto child_length = desc.child_bone_length;
    const auto max_reach = parent_length + child_length;
    const auto min_reach = std::abs(parent_length - child_length);
    if (target_distance > max_reach + reach_slack || target_distance < min_reach - reach_slack) {
        diagnostic = "two-bone IK 3D target is outside the reachable annulus";
        return false;
    }

    const auto target_axis = desc.target * (1.0F / target_distance);
    const auto pole_projection = desc.pole_vector - (target_axis * dot(desc.pole_vector, target_axis));
    const auto pole_projection_length = length(pole_projection);
    if (!std::isfinite(pole_projection_length) || pole_projection_length <= epsilon) {
        diagnostic = "two-bone IK 3D pole vector must not be collinear with the target";
        return false;
    }

    const auto pole_axis = pole_projection * (1.0F / pole_projection_length);
    const auto plane_normal = normalized_or_zero(cross(target_axis, pole_axis));
    if (length(plane_normal) <= epsilon) {
        diagnostic = "two-bone IK 3D pole vector must define a valid bend plane";
        return false;
    }

    const auto c0 =
        ((parent_length * parent_length) + (target_distance * target_distance) - (child_length * child_length)) /
        (2.0F * parent_length * target_distance);
    if (c0 < -1.0F - 1.0e-4F || c0 > 1.0F + 1.0e-4F) {
        diagnostic = "two-bone IK 3D cosine law produced an out-of-range value";
        return false;
    }
    const auto c0c = std::clamp(c0, -1.0F, 1.0F);
    const auto elbow_along_target = parent_length * c0c;
    const auto elbow_offset =
        std::sqrt(std::max(0.0F, (parent_length * parent_length) - (elbow_along_target * elbow_along_target)));

    const auto elbow = (target_axis * elbow_along_target) + (pole_axis * elbow_offset);
    const auto child_vector = desc.target - elbow;
    const auto child_vector_length = length(child_vector);
    if (!std::isfinite(child_vector_length) || child_vector_length <= epsilon) {
        diagnostic = "two-bone IK 3D child bone direction is invalid";
        return false;
    }

    out.root_position = Vec3{};
    out.elbow_position = elbow;
    out.tip_position = desc.target;
    out.parent_frame = make_joint_frame(elbow, plane_normal);
    out.child_frame = make_joint_frame(child_vector, plane_normal);
    return true;
}

} // namespace mirakana
