// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/chain_ik.hpp"

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/math/mat4.hpp"
#include "mirakana/math/quat.hpp"
#include "mirakana/math/vec.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr float epsilon = 1.0e-6F;
constexpr float pose_application_position_tolerance = 0.001F;
constexpr float pi = std::numbers::pi_v<float>;
constexpr float two_pi = 2.0F * pi;
constexpr std::size_t max_segments = 128U;
constexpr std::size_t max_iterations = 128U;

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_vec(Vec2 value) noexcept {
    return finite(value.x) && finite(value.y);
}

[[nodiscard]] bool finite_vec(Vec3 value) noexcept {
    return finite(value.x) && finite(value.y) && finite(value.z);
}

[[nodiscard]] float length_squared(Vec2 value) noexcept {
    return dot(value, value);
}

[[nodiscard]] float length_squared(Vec3 value) noexcept {
    return dot(value, value);
}

[[nodiscard]] Vec3 multiply_components(Vec3 lhs, Vec3 rhs) noexcept {
    return Vec3{.x = lhs.x * rhs.x, .y = lhs.y * rhs.y, .z = lhs.z * rhs.z};
}

[[nodiscard]] float cross_z(Vec2 lhs, Vec2 rhs) noexcept {
    return (lhs.x * rhs.y) - (lhs.y * rhs.x);
}

[[nodiscard]] float distance(Vec2 lhs, Vec2 rhs) noexcept {
    return length(lhs - rhs);
}

[[nodiscard]] float distance(Vec3 lhs, Vec3 rhs) noexcept {
    return length(lhs - rhs);
}

[[nodiscard]] Vec2 unit_or_fallback(Vec2 value, Vec2 fallback) noexcept {
    const auto magnitude = length(value);
    if (finite(magnitude) && magnitude > epsilon) {
        return value * (1.0F / magnitude);
    }
    const auto fallback_magnitude = length(fallback);
    if (finite(fallback_magnitude) && fallback_magnitude > epsilon) {
        return fallback * (1.0F / fallback_magnitude);
    }
    return Vec2{.x = 1.0F, .y = 0.0F};
}

[[nodiscard]] Vec3 unit_or_fallback(Vec3 value, Vec3 fallback) noexcept {
    const auto magnitude = length(value);
    if (finite(magnitude) && magnitude > epsilon) {
        return value * (1.0F / magnitude);
    }
    const auto fallback_magnitude = length(fallback);
    if (finite(fallback_magnitude) && fallback_magnitude > epsilon) {
        return fallback * (1.0F / fallback_magnitude);
    }
    return Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F};
}

[[nodiscard]] Quat rotation_between_unit_vectors(Vec3 from, Vec3 to) noexcept {
    const auto unit_from = unit_or_fallback(from, Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
    const auto unit_to = unit_or_fallback(to, Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
    const auto cosine = std::clamp(dot(unit_from, unit_to), -1.0F, 1.0F);
    if (cosine >= 1.0F - epsilon) {
        return Quat::identity();
    }
    if (cosine <= -1.0F + epsilon) {
        auto axis = cross(unit_from, Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F});
        if (length_squared(axis) <= epsilon * epsilon) {
            axis = cross(unit_from, Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F});
        }
        return Quat::from_axis_angle(axis, pi);
    }

    const auto axis = cross(unit_from, unit_to);
    const auto scale = std::sqrt((1.0F + cosine) * 2.0F);
    const auto inverse_scale = 1.0F / scale;
    return normalize_quat(Quat{
        .x = axis.x * inverse_scale,
        .y = axis.y * inverse_scale,
        .z = axis.z * inverse_scale,
        .w = scale * 0.5F,
    });
}

[[nodiscard]] Vec2 direction_from_angle(float angle_radians) noexcept {
    return Vec2{.x = std::cos(angle_radians), .y = std::sin(angle_radians)};
}

[[nodiscard]] float normalize_angle_signed_pi(float angle_radians) noexcept {
    const auto wrapped = std::remainder(angle_radians, two_pi);
    if (wrapped <= -pi) {
        return wrapped + two_pi;
    }
    if (wrapped > pi) {
        return wrapped - two_pi;
    }
    return wrapped;
}

[[nodiscard]] float bend_side_sign(AnimationFabrikIkXyBendSide bend_side) noexcept {
    return bend_side == AnimationFabrikIkXyBendSide::positive ? 1.0F : -1.0F;
}

[[nodiscard]] bool resolve_bend_side(const AnimationFabrikIkXyDesc& desc,
                                     std::optional<AnimationFabrikIkXyBendSide>& bend_side, std::string& diagnostic) {
    bend_side = std::nullopt;
    if (desc.bend_side.has_value()) {
        switch (*desc.bend_side) {
        case AnimationFabrikIkXyBendSide::positive:
        case AnimationFabrikIkXyBendSide::negative:
            bend_side = desc.bend_side;
            break;
        default:
            diagnostic = "FABRIK IK bend side is invalid";
            return false;
        }
    }

    if (!desc.pole_vector_xy.has_value()) {
        return true;
    }

    const auto axis = desc.target_xy - desc.root_xy;
    if (length_squared(axis) <= epsilon * epsilon) {
        diagnostic = "FABRIK IK pole vector requires a non-zero root-to-target axis";
        return false;
    }

    const auto pole = *desc.pole_vector_xy;
    if (!finite_vec(pole)) {
        diagnostic = "FABRIK IK pole vector must be finite";
        return false;
    }
    if (length_squared(pole) <= epsilon * epsilon) {
        diagnostic = "FABRIK IK pole vector must be non-zero";
        return false;
    }

    const auto side = cross_z(axis, pole);
    if (std::abs(side) <= epsilon) {
        diagnostic = "FABRIK IK pole vector must not be collinear with the root-to-target axis";
        return false;
    }
    bend_side = side > 0.0F ? AnimationFabrikIkXyBendSide::positive : AnimationFabrikIkXyBendSide::negative;
    return true;
}

[[nodiscard]] bool has_duplicate_segment_rotation_limit(std::span<const AnimationFabrikIkXySegmentRotationLimit> limits,
                                                        std::size_t index) noexcept {
    for (std::size_t other = 0; other < index; ++other) {
        if (limits[other].segment_index == limits[index].segment_index) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool validate_desc(const AnimationFabrikIkXyDesc& desc, std::string& diagnostic) {
    if (desc.segment_lengths.empty() || desc.segment_lengths.size() > max_segments) {
        diagnostic = "FABRIK IK segment lengths must contain 1..128 entries";
        return false;
    }
    if (!finite_vec(desc.root_xy) || !finite_vec(desc.target_xy)) {
        diagnostic = "FABRIK IK root and target must be finite";
        return false;
    }
    if (desc.max_iterations == 0U || desc.max_iterations > max_iterations) {
        diagnostic = "FABRIK IK max_iterations must be in 1..128";
        return false;
    }
    if (!finite(desc.tolerance) || desc.tolerance <= 0.0F) {
        diagnostic = "FABRIK IK tolerance must be finite and positive";
        return false;
    }
    for (const auto segment_length : desc.segment_lengths) {
        if (!finite(segment_length) || segment_length <= 0.0F) {
            diagnostic = "FABRIK IK segment lengths must be finite and strictly positive";
            return false;
        }
    }
    if (!desc.initial_joint_positions.empty() &&
        desc.initial_joint_positions.size() != desc.segment_lengths.size() + 1U) {
        diagnostic = "FABRIK IK initial_joint_positions must be empty or contain segment_count + 1 entries";
        return false;
    }
    for (const auto position : desc.initial_joint_positions) {
        if (!finite_vec(position)) {
            diagnostic = "FABRIK IK initial_joint_positions must be finite";
            return false;
        }
    }
    for (std::size_t index = 0; index < desc.local_rotation_limits.size(); ++index) {
        const auto& limit = desc.local_rotation_limits[index];
        if (limit.segment_index >= desc.segment_lengths.size()) {
            diagnostic = "FABRIK IK local rotation limit segment index is out of range";
            return false;
        }
        if (!finite(limit.min_rotation_z_radians) || !finite(limit.max_rotation_z_radians) ||
            limit.min_rotation_z_radians > limit.max_rotation_z_radians) {
            diagnostic = "FABRIK IK local rotation limit range is invalid";
            return false;
        }
        if (has_duplicate_segment_rotation_limit(desc.local_rotation_limits, index)) {
            diagnostic = "FABRIK IK local rotation limits must not contain duplicate segment indices";
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool validate_desc(const AnimationFabrikIk3dDesc& desc, std::string& diagnostic) {
    if (desc.segment_lengths.empty() || desc.segment_lengths.size() > max_segments) {
        diagnostic = "FABRIK IK 3D segment lengths must contain 1..128 entries";
        return false;
    }
    if (!finite_vec(desc.root_position) || !finite_vec(desc.target_position)) {
        diagnostic = "FABRIK IK 3D root and target must be finite";
        return false;
    }
    if (desc.max_iterations == 0U || desc.max_iterations > max_iterations) {
        diagnostic = "FABRIK IK 3D max_iterations must be in 1..128";
        return false;
    }
    if (!finite(desc.tolerance) || desc.tolerance <= 0.0F) {
        diagnostic = "FABRIK IK 3D tolerance must be finite and positive";
        return false;
    }
    for (const auto segment_length : desc.segment_lengths) {
        if (!finite(segment_length) || segment_length <= 0.0F) {
            diagnostic = "FABRIK IK 3D segment lengths must be finite and strictly positive";
            return false;
        }
    }
    if (!desc.initial_joint_positions.empty() &&
        desc.initial_joint_positions.size() != desc.segment_lengths.size() + 1U) {
        diagnostic = "FABRIK IK 3D initial_joint_positions must be empty or contain segment_count + 1 entries";
        return false;
    }
    for (const auto position : desc.initial_joint_positions) {
        if (!finite_vec(position)) {
            diagnostic = "FABRIK IK 3D initial_joint_positions must be finite";
            return false;
        }
    }
    return true;
}

[[nodiscard]] float first_internal_side(std::span<const Vec2> positions, Vec2 root, Vec2 target) noexcept {
    if (positions.size() < 3U) {
        return 0.0F;
    }

    const auto axis = target - root;
    for (std::size_t joint = 1; joint + 1U < positions.size(); ++joint) {
        const auto side = cross_z(axis, positions[joint] - root);
        if (std::abs(side) > epsilon) {
            return side;
        }
    }
    return 0.0F;
}

[[nodiscard]] Vec2 reflect_across_axis(Vec2 point, Vec2 root, Vec2 axis) noexcept {
    const auto axis_length_squared = length_squared(axis);
    if (axis_length_squared <= epsilon * epsilon) {
        return point;
    }

    const auto offset = point - root;
    const auto projection = axis * (dot(offset, axis) / axis_length_squared);
    const auto perpendicular = offset - projection;
    return root + projection - perpendicular;
}

void enforce_bend_side_by_reflection(std::vector<Vec2>& positions, Vec2 root, Vec2 target,
                                     AnimationFabrikIkXyBendSide bend_side) noexcept {
    if (positions.size() < 3U || length_squared(target - root) <= epsilon * epsilon) {
        return;
    }

    const auto current_side = first_internal_side(positions, root, target);
    if (std::abs(current_side) <= epsilon || current_side * bend_side_sign(bend_side) > 0.0F) {
        return;
    }

    const auto axis = target - root;
    for (std::size_t joint = 1; joint < positions.size(); ++joint) {
        positions[joint] = reflect_across_axis(positions[joint], root, axis);
    }
}

[[nodiscard]] std::vector<Vec2> make_initial_positions(const AnimationFabrikIkXyDesc& desc, std::string& diagnostic) {
    std::vector<Vec2> positions;
    positions.reserve(desc.segment_lengths.size() + 1U);
    positions.push_back(desc.root_xy);

    if (desc.initial_joint_positions.empty()) {
        for (const auto segment_length : desc.segment_lengths) {
            positions.push_back(positions.back() + Vec2{.x = segment_length, .y = 0.0F});
        }
        return positions;
    }

    for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
        const auto raw_delta = desc.initial_joint_positions[segment + 1U] - desc.initial_joint_positions[segment];
        if (length(raw_delta) <= epsilon) {
            diagnostic = "FABRIK IK initial_joint_positions must not contain zero-length segments";
            return {};
        }
        const auto direction = unit_or_fallback(raw_delta, Vec2{.x = 1.0F, .y = 0.0F});
        positions.push_back(positions.back() + (direction * desc.segment_lengths[segment]));
    }
    return positions;
}

[[nodiscard]] std::vector<Vec3> make_initial_positions(const AnimationFabrikIk3dDesc& desc, std::string& diagnostic) {
    std::vector<Vec3> positions;
    positions.reserve(desc.segment_lengths.size() + 1U);
    positions.push_back(desc.root_position);

    if (desc.initial_joint_positions.empty()) {
        for (const auto segment_length : desc.segment_lengths) {
            positions.push_back(positions.back() + Vec3{.x = segment_length, .y = 0.0F, .z = 0.0F});
        }
        return positions;
    }

    for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
        const auto raw_delta = desc.initial_joint_positions[segment + 1U] - desc.initial_joint_positions[segment];
        if (length_squared(raw_delta) <= epsilon * epsilon) {
            diagnostic = "FABRIK IK 3D initial_joint_positions must not contain zero-length segments";
            return {};
        }
        const auto direction = unit_or_fallback(raw_delta, Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
        positions.push_back(positions.back() + (direction * desc.segment_lengths[segment]));
    }
    return positions;
}

void fill_rotations(AnimationFabrikIkXySolution& out) {
    out.segment_rotation_z_radians.clear();
    if (out.joint_positions.size() < 2U) {
        return;
    }
    out.segment_rotation_z_radians.reserve(out.joint_positions.size() - 1U);
    for (std::size_t segment = 0; segment + 1U < out.joint_positions.size(); ++segment) {
        const auto delta = out.joint_positions[segment + 1U] - out.joint_positions[segment];
        out.segment_rotation_z_radians.push_back(std::atan2(delta.y, delta.x));
    }
}

[[nodiscard]] float
clamp_segment_local_rotation(float local_rotation_z_radians, std::size_t segment_index,
                             std::span<const AnimationFabrikIkXySegmentRotationLimit> local_rotation_limits) noexcept {
    for (const auto& limit : local_rotation_limits) {
        if (limit.segment_index == segment_index) {
            return std::clamp(local_rotation_z_radians, limit.min_rotation_z_radians, limit.max_rotation_z_radians);
        }
    }
    return local_rotation_z_radians;
}

void reconstruct_from_root_with_limits(
    std::vector<Vec2>& positions, std::span<const float> segment_lengths, Vec2 fixed_root,
    std::span<const AnimationFabrikIkXySegmentRotationLimit> local_rotation_limits) noexcept {
    positions[0] = fixed_root;
    float parent_world_rotation = 0.0F;
    for (std::size_t segment = 0; segment < segment_lengths.size(); ++segment) {
        const auto fallback_direction = direction_from_angle(parent_world_rotation);
        const auto direction = unit_or_fallback(positions[segment + 1U] - positions[segment], fallback_direction);
        const auto desired_world_rotation = std::atan2(direction.y, direction.x);
        const auto desired_local_rotation = normalize_angle_signed_pi(desired_world_rotation - parent_world_rotation);
        const auto local_rotation =
            clamp_segment_local_rotation(desired_local_rotation, segment, local_rotation_limits);
        const auto world_rotation = parent_world_rotation + local_rotation;
        positions[segment + 1U] =
            positions[segment] + (direction_from_angle(world_rotation) * segment_lengths[segment]);
        parent_world_rotation = world_rotation;
    }
}

[[nodiscard]] bool has_duplicate_joint_index(std::span<const std::size_t> joint_indices, std::size_t index) noexcept {
    for (std::size_t other = 0; other < index; ++other) {
        if (joint_indices[other] == joint_indices[index]) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_duplicate_limit_joint(std::span<const AnimationIkLocalRotationLimit> limits,
                                             std::size_t index) noexcept {
    for (std::size_t other = 0; other < index; ++other) {
        if (limits[other].joint_index == limits[index].joint_index) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_duplicate_limit_joint(std::span<const AnimationIkLocalRotationLimit3d> limits,
                                             std::size_t index) noexcept {
    for (std::size_t other = 0; other < index; ++other) {
        if (limits[other].joint_index == limits[index].joint_index) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::string diagnostic_message(AnimationIkLocalRotationLimit3dDiagnosticCode code) {
    switch (code) {
    case AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_skeleton:
        return "3D local rotation limits require a valid skeleton";
    case AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_joint_index:
        return "3D local rotation limit joint index is out of range";
    case AnimationIkLocalRotationLimit3dDiagnosticCode::duplicate_joint:
        return "3D local rotation limits must not contain duplicate joints";
    case AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_twist_axis:
        return "3D local rotation limit twist axis must be finite and non-zero";
    case AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_swing_limit:
        return "3D local rotation limit swing range must be finite and within 0..pi";
    case AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_twist_range:
        return "3D local rotation limit twist range must be finite, ordered, and within -pi..pi";
    }
    return "3D local rotation limit diagnostic is invalid";
}

[[nodiscard]] AnimationIkLocalRotationLimit3dDiagnostic
make_diagnostic(AnimationIkLocalRotationLimit3dDiagnosticCode code, std::size_t limit_index, std::size_t joint_index) {
    return AnimationIkLocalRotationLimit3dDiagnostic{
        .code = code,
        .limit_index = limit_index,
        .joint_index = joint_index,
        .message = diagnostic_message(code),
    };
}

[[nodiscard]] bool is_segment_controlling_joint(std::span<const std::size_t> joint_indices,
                                                std::size_t joint_index) noexcept {
    for (std::size_t segment = 0; segment + 1U < joint_indices.size(); ++segment) {
        if (joint_indices[segment] == joint_index) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool validate_pose_application(const AnimationSkeletonDesc& skeleton,
                                             const AnimationFabrikIkXySolution& solution,
                                             std::span<const std::size_t> joint_indices, const AnimationPose& pose,
                                             std::span<const AnimationIkLocalRotationLimit> local_rotation_limits,
                                             std::string& diagnostic) {
    if (!is_valid_animation_skeleton_desc(skeleton)) {
        diagnostic = "FABRIK IK pose application skeleton is invalid";
        return false;
    }
    if (!validate_animation_pose(skeleton, pose).empty()) {
        diagnostic = "FABRIK IK pose application pose is invalid";
        return false;
    }
    if (joint_indices.size() < 2U) {
        diagnostic = "FABRIK IK pose application joint chain must include at least two joints";
        return false;
    }
    if (solution.joint_positions.size() != joint_indices.size() ||
        solution.segment_rotation_z_radians.size() + 1U != joint_indices.size()) {
        diagnostic = "FABRIK IK pose application solution shape does not match joint chain";
        return false;
    }
    for (const auto position : solution.joint_positions) {
        if (!finite_vec(position)) {
            diagnostic = "FABRIK IK pose application joint positions must be finite";
            return false;
        }
    }
    for (const auto rotation : solution.segment_rotation_z_radians) {
        if (!finite(rotation)) {
            diagnostic = "FABRIK IK pose application segment rotations must be finite";
            return false;
        }
    }
    for (std::size_t index = 0; index < joint_indices.size(); ++index) {
        const auto joint_index = joint_indices[index];
        if (joint_index >= skeleton.joints.size()) {
            diagnostic = "FABRIK IK pose application joint index is out of range";
            return false;
        }
        if (has_duplicate_joint_index(joint_indices, index)) {
            diagnostic = "FABRIK IK pose application joint chain must not contain duplicates";
            return false;
        }
        if (index > 0U && skeleton.joints[joint_index].parent_index != joint_indices[index - 1U]) {
            diagnostic = "FABRIK IK pose application joint chain must follow parent-child order";
            return false;
        }
    }
    for (std::size_t index = 0; index < local_rotation_limits.size(); ++index) {
        const auto& limit = local_rotation_limits[index];
        if (!finite(limit.min_rotation_z_radians) || !finite(limit.max_rotation_z_radians) ||
            limit.min_rotation_z_radians > limit.max_rotation_z_radians) {
            diagnostic = "FABRIK IK pose application local rotation limit range is invalid";
            return false;
        }
        if (has_duplicate_limit_joint(local_rotation_limits, index)) {
            diagnostic = "FABRIK IK pose application local rotation limits must not contain duplicates";
            return false;
        }
        if (!is_segment_controlling_joint(joint_indices, limit.joint_index)) {
            diagnostic = "FABRIK IK pose application local rotation limit must target a segment joint";
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool validate_pose_application(const AnimationSkeleton3dDesc& skeleton,
                                             const AnimationFabrikIk3dSolution& solution,
                                             std::span<const std::size_t> joint_indices, const AnimationPose3d& pose,
                                             std::string& diagnostic) {
    if (!is_valid_animation_skeleton_3d_desc(skeleton)) {
        diagnostic = "FABRIK IK 3D pose application skeleton is invalid";
        return false;
    }
    if (!validate_animation_pose_3d(skeleton, pose).empty()) {
        diagnostic = "FABRIK IK 3D pose application pose is invalid";
        return false;
    }
    if (joint_indices.size() < 2U) {
        diagnostic = "FABRIK IK 3D pose application joint chain must include at least two joints";
        return false;
    }
    if (solution.joint_positions.size() != joint_indices.size()) {
        diagnostic = "FABRIK IK 3D pose application solution shape does not match joint chain";
        return false;
    }
    for (const auto position : solution.joint_positions) {
        if (!finite_vec(position)) {
            diagnostic = "FABRIK IK 3D pose application joint positions must be finite";
            return false;
        }
    }
    for (std::size_t index = 0; index < joint_indices.size(); ++index) {
        const auto joint_index = joint_indices[index];
        if (joint_index >= skeleton.joints.size()) {
            diagnostic = "FABRIK IK 3D pose application joint index is out of range";
            return false;
        }
        if (has_duplicate_joint_index(joint_indices, index)) {
            diagnostic = "FABRIK IK 3D pose application joint chain must not contain duplicates";
            return false;
        }
        if (index > 0U && skeleton.joints[joint_index].parent_index != joint_indices[index - 1U]) {
            diagnostic = "FABRIK IK 3D pose application joint chain must follow parent-child order";
            return false;
        }
    }

    const auto model_pose = build_animation_model_pose_3d(skeleton, pose);
    const auto root_origin =
        transform_point(model_pose.joint_matrices[joint_indices.front()], Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F});
    if (distance(root_origin, solution.joint_positions.front()) > pose_application_position_tolerance) {
        diagnostic = "FABRIK IK 3D pose application root position does not match solution";
        return false;
    }

    for (std::size_t segment = 0; segment + 1U < joint_indices.size(); ++segment) {
        const auto joint_index = joint_indices[segment];
        const auto child_index = joint_indices[segment + 1U];
        const auto solution_delta = solution.joint_positions[segment + 1U] - solution.joint_positions[segment];
        const auto solution_length = length(solution_delta);
        if (!finite(solution_length) || solution_length <= epsilon) {
            diagnostic = "FABRIK IK 3D pose application solution segment length is invalid";
            return false;
        }

        const auto pose_bone_vector =
            multiply_components(pose.joints[joint_index].scale, pose.joints[child_index].translation);
        const auto pose_bone_length = length(pose_bone_vector);
        if (!finite(pose_bone_length) || pose_bone_length <= epsilon) {
            diagnostic = "FABRIK IK 3D pose application pose bone length is zero-length or invalid";
            return false;
        }
        if (std::abs(pose_bone_length - solution_length) > pose_application_position_tolerance) {
            diagnostic = "FABRIK IK 3D pose application pose bone length does not match solution segment length";
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool validate_pose_application(const AnimationSkeleton3dDesc& skeleton,
                                             const AnimationFabrikIk3dSolution& solution,
                                             std::span<const std::size_t> joint_indices,
                                             std::span<const AnimationIkLocalRotationLimit3d> local_rotation_limits,
                                             const AnimationPose3d& pose, std::string& diagnostic) {
    if (!validate_pose_application(skeleton, solution, joint_indices, pose, diagnostic)) {
        return false;
    }
    const auto limit_diagnostics = validate_animation_ik_local_rotation_limits_3d(skeleton, local_rotation_limits);
    if (!limit_diagnostics.empty()) {
        diagnostic =
            "FABRIK IK 3D pose application local rotation limits are invalid: " + limit_diagnostics.front().message;
        return false;
    }
    for (const auto& limit : local_rotation_limits) {
        if (!is_segment_controlling_joint(joint_indices, limit.joint_index)) {
            diagnostic = "FABRIK IK 3D pose application local rotation limit must target a segment joint";
            return false;
        }
    }
    return true;
}

[[nodiscard]] float
clamp_local_rotation(float rotation_z_radians, std::size_t joint_index,
                     std::span<const AnimationIkLocalRotationLimit> local_rotation_limits) noexcept {
    for (const auto& limit : local_rotation_limits) {
        if (limit.joint_index == joint_index) {
            return std::clamp(rotation_z_radians, limit.min_rotation_z_radians, limit.max_rotation_z_radians);
        }
    }
    return rotation_z_radians;
}

[[nodiscard]] Quat canonical_quat(Quat value) noexcept {
    const auto unit = normalize_quat(value);
    if (unit.w < 0.0F) {
        return Quat{.x = -unit.x, .y = -unit.y, .z = -unit.z, .w = -unit.w};
    }
    return unit;
}

[[nodiscard]] float equivalent_quat_delta_squared(Quat lhs, Quat rhs) noexcept {
    lhs = normalize_quat(lhs);
    rhs = normalize_quat(rhs);
    const auto direct_x = lhs.x - rhs.x;
    const auto direct_y = lhs.y - rhs.y;
    const auto direct_z = lhs.z - rhs.z;
    const auto direct_w = lhs.w - rhs.w;
    const auto flipped_x = lhs.x + rhs.x;
    const auto flipped_y = lhs.y + rhs.y;
    const auto flipped_z = lhs.z + rhs.z;
    const auto flipped_w = lhs.w + rhs.w;
    const auto direct = (direct_x * direct_x) + (direct_y * direct_y) + (direct_z * direct_z) + (direct_w * direct_w);
    const auto flipped =
        (flipped_x * flipped_x) + (flipped_y * flipped_y) + (flipped_z * flipped_z) + (flipped_w * flipped_w);
    return std::min(direct, flipped);
}

struct LocalRotationLimit3dClamp {
    Quat rotation{Quat::identity()};
    bool constrained{false};
};

[[nodiscard]] LocalRotationLimit3dClamp clamp_local_rotation(Quat rotation,
                                                             const AnimationIkLocalRotationLimit3d& limit) noexcept {
    const auto original = normalize_quat(rotation);
    const auto source = canonical_quat(original);
    const auto twist_axis = unit_or_fallback(limit.twist_axis, Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
    const auto source_vector = Vec3{.x = source.x, .y = source.y, .z = source.z};
    const auto projected_vector = twist_axis * dot(source_vector, twist_axis);

    Quat twist = Quat::identity();
    if (length_squared(projected_vector) > epsilon * epsilon || std::abs(source.w) > epsilon) {
        twist = canonical_quat(Quat{
            .x = projected_vector.x,
            .y = projected_vector.y,
            .z = projected_vector.z,
            .w = source.w,
        });
    }

    auto swing = canonical_quat(source * conjugate(twist));
    const auto twist_vector = Vec3{.x = twist.x, .y = twist.y, .z = twist.z};
    const auto twist_vector_length = length(twist_vector);
    auto twist_angle = 2.0F * std::atan2(twist_vector_length, std::max(twist.w, 0.0F));
    if (dot(twist_vector, twist_axis) < 0.0F) {
        twist_angle = -twist_angle;
    }
    twist_angle = normalize_angle_signed_pi(twist_angle);
    const auto clamped_twist_angle = std::clamp(twist_angle, limit.min_twist_radians, limit.max_twist_radians);
    const auto clamped_twist = Quat::from_axis_angle(twist_axis, clamped_twist_angle);

    const auto swing_vector = Vec3{.x = swing.x, .y = swing.y, .z = swing.z};
    const auto swing_vector_length = length(swing_vector);
    const auto swing_angle = 2.0F * std::atan2(swing_vector_length, std::max(swing.w, 0.0F));
    auto clamped_swing = Quat::identity();
    if (swing_vector_length > epsilon && limit.max_swing_radians > 0.0F) {
        const auto swing_axis = swing_vector * (1.0F / swing_vector_length);
        clamped_swing = Quat::from_axis_angle(swing_axis, std::min(swing_angle, limit.max_swing_radians));
    }

    const auto clamped = canonical_quat(clamped_swing * clamped_twist);
    return LocalRotationLimit3dClamp{
        .rotation = clamped,
        .constrained = equivalent_quat_delta_squared(original, clamped) > 0.00000001F,
    };
}

[[nodiscard]] float model_rotation_z(const AnimationSkeletonDesc& skeleton, const AnimationPose& pose,
                                     std::size_t joint_index) noexcept {
    float rotation = 0.0F;
    auto cursor = joint_index;
    while (cursor != animation_no_parent) {
        rotation += pose.joints[cursor].rotation_z_radians;
        cursor = skeleton.joints[cursor].parent_index;
    }
    return rotation;
}

[[nodiscard]] Quat model_rotation(const AnimationSkeleton3dDesc& skeleton, const AnimationPose3d& pose,
                                  std::size_t joint_index) {
    std::vector<std::size_t> lineage;
    auto cursor = joint_index;
    while (cursor != animation_no_parent) {
        lineage.push_back(cursor);
        cursor = skeleton.joints[cursor].parent_index;
    }

    auto rotation = Quat::identity();
    for (const auto joint : std::views::reverse(lineage)) {
        rotation = normalize_quat(rotation * pose.joints[joint].rotation);
    }
    return rotation;
}

} // namespace

bool solve_animation_fabrik_ik_xy_chain(const AnimationFabrikIkXyDesc& desc, AnimationFabrikIkXySolution& out,
                                        std::string& diagnostic) {
    out = AnimationFabrikIkXySolution{};
    diagnostic.clear();
    if (!validate_desc(desc, diagnostic)) {
        return false;
    }
    std::optional<AnimationFabrikIkXyBendSide> bend_side;
    if (!resolve_bend_side(desc, bend_side, diagnostic)) {
        return false;
    }

    auto positions = make_initial_positions(desc, diagnostic);
    if (positions.empty()) {
        return false;
    }
    if (bend_side.has_value()) {
        enforce_bend_side_by_reflection(positions, desc.root_xy, desc.target_xy, *bend_side);
    }

    const auto total_length = std::accumulate(desc.segment_lengths.begin(), desc.segment_lengths.end(), 0.0F);
    const auto longest_segment = *std::ranges::max_element(desc.segment_lengths);
    const auto minimum_reach = std::max(0.0F, longest_segment - (total_length - longest_segment));
    const auto target_distance = distance(desc.root_xy, desc.target_xy);
    if (target_distance > total_length + desc.tolerance) {
        if (!desc.clamp_unreachable_target) {
            diagnostic = "FABRIK IK target is outside maximum reach";
            return false;
        }
        const auto direction = unit_or_fallback(desc.target_xy - desc.root_xy, Vec2{.x = 1.0F, .y = 0.0F});
        positions[0] = desc.root_xy;
        for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
            positions[segment + 1U] = positions[segment] + (direction * desc.segment_lengths[segment]);
        }
        if (bend_side.has_value()) {
            enforce_bend_side_by_reflection(positions, desc.root_xy, desc.target_xy, *bend_side);
        }
        reconstruct_from_root_with_limits(positions, desc.segment_lengths, desc.root_xy, desc.local_rotation_limits);
        out.joint_positions = std::move(positions);
        out.end_effector_error = distance(out.joint_positions.back(), desc.target_xy);
        out.iterations = 0U;
        out.reached = false;
        out.target_was_unreachable = true;
        fill_rotations(out);
        return true;
    }
    if (target_distance < minimum_reach - desc.tolerance) {
        if (!desc.clamp_unreachable_target) {
            diagnostic = "FABRIK IK target is inside minimum reach";
            return false;
        }
        const auto direction = unit_or_fallback(desc.target_xy - desc.root_xy, positions[1] - positions[0]);
        positions[0] = desc.root_xy;
        bool consumed_longest = false;
        for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
            const bool is_longest = !consumed_longest && desc.segment_lengths[segment] == longest_segment;
            consumed_longest = consumed_longest || is_longest;
            const auto sign = is_longest ? 1.0F : -1.0F;
            positions[segment + 1U] = positions[segment] + (direction * (desc.segment_lengths[segment] * sign));
        }
        if (bend_side.has_value()) {
            enforce_bend_side_by_reflection(positions, desc.root_xy, desc.target_xy, *bend_side);
        }
        reconstruct_from_root_with_limits(positions, desc.segment_lengths, desc.root_xy, desc.local_rotation_limits);
        out.joint_positions = std::move(positions);
        out.end_effector_error = distance(out.joint_positions.back(), desc.target_xy);
        out.iterations = 0U;
        out.reached = false;
        out.target_was_unreachable = true;
        fill_rotations(out);
        return true;
    }

    const Vec2 fixed_root = desc.root_xy;
    float error = distance(positions.back(), desc.target_xy);
    std::size_t iterations = 0U;
    for (; iterations < desc.max_iterations && error > desc.tolerance; ++iterations) {
        positions.back() = desc.target_xy;
        for (std::size_t reverse_index = desc.segment_lengths.size(); reverse_index > 0U; --reverse_index) {
            const auto segment = reverse_index - 1U;
            const auto direction =
                unit_or_fallback(positions[segment] - positions[segment + 1U], Vec2{.x = -1.0F, .y = 0.0F});
            positions[segment] = positions[segment + 1U] + (direction * desc.segment_lengths[segment]);
        }

        positions[0] = fixed_root;
        if (bend_side.has_value()) {
            enforce_bend_side_by_reflection(positions, fixed_root, desc.target_xy, *bend_side);
        }
        reconstruct_from_root_with_limits(positions, desc.segment_lengths, fixed_root, desc.local_rotation_limits);
        error = distance(positions.back(), desc.target_xy);
    }

    out.joint_positions = std::move(positions);
    out.end_effector_error = error;
    out.iterations = iterations;
    out.reached = error <= desc.tolerance;
    out.target_was_unreachable = false;
    fill_rotations(out);
    return true;
}

bool solve_animation_fabrik_ik_3d_chain(const AnimationFabrikIk3dDesc& desc, AnimationFabrikIk3dSolution& out,
                                        std::string& diagnostic) {
    out = AnimationFabrikIk3dSolution{};
    diagnostic.clear();
    if (!validate_desc(desc, diagnostic)) {
        return false;
    }

    auto positions = make_initial_positions(desc, diagnostic);
    if (positions.empty()) {
        return false;
    }

    const auto total_length = std::accumulate(desc.segment_lengths.begin(), desc.segment_lengths.end(), 0.0F);
    const auto longest_segment = *std::ranges::max_element(desc.segment_lengths);
    const auto minimum_reach = std::max(0.0F, longest_segment - (total_length - longest_segment));
    const auto target_distance = distance(desc.root_position, desc.target_position);
    if (target_distance > total_length + desc.tolerance) {
        if (!desc.clamp_unreachable_target) {
            diagnostic = "FABRIK IK 3D target is outside maximum reach";
            return false;
        }
        const auto direction =
            unit_or_fallback(desc.target_position - desc.root_position, Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
        positions[0] = desc.root_position;
        for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
            positions[segment + 1U] = positions[segment] + (direction * desc.segment_lengths[segment]);
        }
        out.joint_positions = std::move(positions);
        out.end_effector_error = distance(out.joint_positions.back(), desc.target_position);
        out.iterations = 0U;
        out.reached = false;
        out.target_was_unreachable = true;
        return true;
    }
    if (target_distance < minimum_reach - desc.tolerance) {
        if (!desc.clamp_unreachable_target) {
            diagnostic = "FABRIK IK 3D target is inside minimum reach";
            return false;
        }
        const auto direction = unit_or_fallback(desc.target_position - desc.root_position, positions[1] - positions[0]);
        positions[0] = desc.root_position;
        bool consumed_longest = false;
        for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
            const bool is_longest = !consumed_longest && desc.segment_lengths[segment] == longest_segment;
            consumed_longest = consumed_longest || is_longest;
            const auto sign = is_longest ? 1.0F : -1.0F;
            positions[segment + 1U] = positions[segment] + (direction * (desc.segment_lengths[segment] * sign));
        }
        out.joint_positions = std::move(positions);
        out.end_effector_error = distance(out.joint_positions.back(), desc.target_position);
        out.iterations = 0U;
        out.reached = false;
        out.target_was_unreachable = true;
        return true;
    }

    const Vec3 fixed_root = desc.root_position;
    float error = distance(positions.back(), desc.target_position);
    std::size_t iterations = 0U;
    for (; iterations < desc.max_iterations && error > desc.tolerance; ++iterations) {
        positions.back() = desc.target_position;
        for (std::size_t reverse_index = desc.segment_lengths.size(); reverse_index > 0U; --reverse_index) {
            const auto segment = reverse_index - 1U;
            const auto direction =
                unit_or_fallback(positions[segment] - positions[segment + 1U], Vec3{.x = -1.0F, .y = 0.0F, .z = 0.0F});
            positions[segment] = positions[segment + 1U] + (direction * desc.segment_lengths[segment]);
        }

        positions[0] = fixed_root;
        for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
            const auto direction =
                unit_or_fallback(positions[segment + 1U] - positions[segment], Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
            positions[segment + 1U] = positions[segment] + (direction * desc.segment_lengths[segment]);
        }
        error = distance(positions.back(), desc.target_position);
    }

    out.joint_positions = std::move(positions);
    out.end_effector_error = error;
    out.iterations = iterations;
    out.reached = error <= desc.tolerance;
    out.target_was_unreachable = false;
    return true;
}

std::vector<AnimationIkLocalRotationLimit3dDiagnostic>
validate_animation_ik_local_rotation_limits_3d(const AnimationSkeleton3dDesc& skeleton,
                                               std::span<const AnimationIkLocalRotationLimit3d> local_rotation_limits) {
    std::vector<AnimationIkLocalRotationLimit3dDiagnostic> diagnostics;
    if (!is_valid_animation_skeleton_3d_desc(skeleton)) {
        diagnostics.push_back(make_diagnostic(AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_skeleton, 0U, 0U));
        return diagnostics;
    }

    for (std::size_t index = 0; index < local_rotation_limits.size(); ++index) {
        const auto& limit = local_rotation_limits[index];
        if (limit.joint_index >= skeleton.joints.size()) {
            diagnostics.push_back(make_diagnostic(AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_joint_index,
                                                  index, limit.joint_index));
            continue;
        }
        if (has_duplicate_limit_joint(local_rotation_limits, index)) {
            diagnostics.push_back(make_diagnostic(AnimationIkLocalRotationLimit3dDiagnosticCode::duplicate_joint, index,
                                                  limit.joint_index));
        }
        if (!finite_vec(limit.twist_axis) || length_squared(limit.twist_axis) <= epsilon * epsilon) {
            diagnostics.push_back(make_diagnostic(AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_twist_axis,
                                                  index, limit.joint_index));
        }
        if (!finite(limit.max_swing_radians) || limit.max_swing_radians < 0.0F || limit.max_swing_radians > pi) {
            diagnostics.push_back(make_diagnostic(AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_swing_limit,
                                                  index, limit.joint_index));
        }
        if (!finite(limit.min_twist_radians) || !finite(limit.max_twist_radians) || limit.min_twist_radians < -pi ||
            limit.max_twist_radians > pi || limit.min_twist_radians > limit.max_twist_radians) {
            diagnostics.push_back(make_diagnostic(AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_twist_range,
                                                  index, limit.joint_index));
        }
    }
    return diagnostics;
}

bool is_valid_animation_ik_local_rotation_limits_3d(
    const AnimationSkeleton3dDesc& skeleton, std::span<const AnimationIkLocalRotationLimit3d> local_rotation_limits) {
    return validate_animation_ik_local_rotation_limits_3d(skeleton, local_rotation_limits).empty();
}

AnimationLocalRotationLimit3dApplyResult
apply_animation_local_rotation_limits_3d(const AnimationSkeleton3dDesc& skeleton,
                                         std::span<const AnimationIkLocalRotationLimit3d> local_rotation_limits,
                                         AnimationPose3d& pose) {
    AnimationLocalRotationLimit3dApplyResult result;
    const auto limit_diagnostics = validate_animation_ik_local_rotation_limits_3d(skeleton, local_rotation_limits);
    if (!limit_diagnostics.empty()) {
        result.diagnostic = limit_diagnostics.front().message;
        return result;
    }
    if (!validate_animation_pose_3d(skeleton, pose).empty()) {
        result.diagnostic = "3D local rotation limits require a valid pose";
        return result;
    }

    auto updated_pose = pose;
    for (const auto& limit : local_rotation_limits) {
        auto& joint = updated_pose.joints[limit.joint_index];
        const auto clamped = clamp_local_rotation(joint.rotation, limit);
        joint.rotation = clamped.rotation;
        if (clamped.constrained) {
            result.constrained = true;
            ++result.constrained_joint_count;
        }
    }

    if (!validate_animation_pose_3d(skeleton, updated_pose).empty()) {
        result.diagnostic = "3D local rotation limits produced an invalid pose";
        return result;
    }

    pose = std::move(updated_pose);
    result.succeeded = true;
    return result;
}

bool apply_animation_fabrik_ik_xy_solution_to_pose(const AnimationSkeletonDesc& skeleton,
                                                   const AnimationFabrikIkXySolution& solution,
                                                   std::span<const std::size_t> joint_indices, AnimationPose& pose,
                                                   std::string& diagnostic) {
    return apply_animation_fabrik_ik_xy_solution_to_pose(
        skeleton, solution, joint_indices, std::span<const AnimationIkLocalRotationLimit>{}, pose, diagnostic);
}

bool apply_animation_fabrik_ik_xy_solution_to_pose(const AnimationSkeletonDesc& skeleton,
                                                   const AnimationFabrikIkXySolution& solution,
                                                   std::span<const std::size_t> joint_indices,
                                                   std::span<const AnimationIkLocalRotationLimit> local_rotation_limits,
                                                   AnimationPose& pose, std::string& diagnostic) {
    diagnostic.clear();
    if (!validate_pose_application(skeleton, solution, joint_indices, pose, local_rotation_limits, diagnostic)) {
        return false;
    }

    auto updated_pose = pose;
    for (std::size_t segment = 0; segment < solution.segment_rotation_z_radians.size(); ++segment) {
        const auto joint_index = joint_indices[segment];
        const auto parent_index = skeleton.joints[joint_index].parent_index;
        const auto parent_rotation =
            parent_index == animation_no_parent ? 0.0F : model_rotation_z(skeleton, updated_pose, parent_index);
        const auto local_rotation =
            normalize_angle_signed_pi(solution.segment_rotation_z_radians[segment] - parent_rotation);
        updated_pose.joints[joint_index].rotation_z_radians =
            clamp_local_rotation(local_rotation, joint_index, local_rotation_limits);
    }

    const auto updated_diagnostics = validate_animation_pose(skeleton, updated_pose);
    if (!updated_diagnostics.empty()) {
        diagnostic = "FABRIK IK pose application produced an invalid pose";
        return false;
    }

    pose = std::move(updated_pose);
    return true;
}

bool apply_animation_fabrik_ik_3d_solution_to_pose(const AnimationSkeleton3dDesc& skeleton,
                                                   const AnimationFabrikIk3dSolution& solution,
                                                   std::span<const std::size_t> joint_indices, AnimationPose3d& pose,
                                                   std::string& diagnostic) {
    return apply_animation_fabrik_ik_3d_solution_to_pose(
        skeleton, solution, joint_indices, std::span<const AnimationIkLocalRotationLimit3d>{}, pose, diagnostic);
}

bool apply_animation_fabrik_ik_3d_solution_to_pose(
    const AnimationSkeleton3dDesc& skeleton, const AnimationFabrikIk3dSolution& solution,
    std::span<const std::size_t> joint_indices, std::span<const AnimationIkLocalRotationLimit3d> local_rotation_limits,
    AnimationPose3d& pose, std::string& diagnostic) {
    diagnostic.clear();
    if (!validate_pose_application(skeleton, solution, joint_indices, local_rotation_limits, pose, diagnostic)) {
        return false;
    }

    auto updated_pose = pose;
    for (std::size_t segment = 0; segment + 1U < joint_indices.size(); ++segment) {
        const auto joint_index = joint_indices[segment];
        const auto child_index = joint_indices[segment + 1U];
        const auto parent_index = skeleton.joints[joint_index].parent_index;
        const auto parent_rotation = parent_index == animation_no_parent
                                         ? Quat::identity()
                                         : model_rotation(skeleton, updated_pose, parent_index);
        const auto world_direction =
            unit_or_fallback(solution.joint_positions[segment + 1U] - solution.joint_positions[segment],
                             Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
        const auto desired_parent_direction = rotate(conjugate(parent_rotation), world_direction);
        const auto local_bone_direction = unit_or_fallback(
            multiply_components(updated_pose.joints[joint_index].scale, updated_pose.joints[child_index].translation),
            Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
        updated_pose.joints[joint_index].rotation =
            rotation_between_unit_vectors(local_bone_direction, desired_parent_direction);
    }

    const auto limit_result = apply_animation_local_rotation_limits_3d(skeleton, local_rotation_limits, updated_pose);
    if (!limit_result.succeeded) {
        diagnostic = "FABRIK IK 3D pose application local rotation limits failed: " + limit_result.diagnostic;
        return false;
    }

    const auto updated_diagnostics = validate_animation_pose_3d(skeleton, updated_pose);
    if (!updated_diagnostics.empty()) {
        diagnostic = "FABRIK IK 3D pose application produced an invalid pose";
        return false;
    }

    const auto model_pose = build_animation_model_pose_3d(skeleton, updated_pose);
    for (std::size_t index = 0; index < joint_indices.size(); ++index) {
        const auto joint_origin =
            transform_point(model_pose.joint_matrices[joint_indices[index]], Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        if (distance(joint_origin, solution.joint_positions[index]) > pose_application_position_tolerance * 2.0F) {
            diagnostic = local_rotation_limits.empty()
                             ? "FABRIK IK 3D pose application produced positions that do not match the solution"
                             : "FABRIK IK 3D pose application local rotation limits prevent matching the solution";
            return false;
        }
    }

    pose = std::move(updated_pose);
    return true;
}

} // namespace mirakana
