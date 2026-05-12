// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/math/vec.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class AnimationFabrikIkXyBendSide {
    positive,
    negative,
};

struct AnimationFabrikIkXySegmentRotationLimit {
    std::size_t segment_index{0};
    float min_rotation_z_radians{0.0F};
    float max_rotation_z_radians{0.0F};
};

struct AnimationFabrikIkXyDesc {
    std::vector<float> segment_lengths;
    std::vector<AnimationFabrikIkXySegmentRotationLimit> local_rotation_limits;
    Vec2 root_xy{.x = 0.0F, .y = 0.0F};
    Vec2 target_xy{.x = 0.0F, .y = 0.0F};
    /// Optional mirrored branch side for internal joints when no pole vector is set.
    std::optional<AnimationFabrikIkXyBendSide> bend_side;
    /// Optional local XY guide vector from root. When set, its side relative to root-to-target overrides `bend_side`.
    std::optional<Vec2> pole_vector_xy;
    std::vector<Vec2> initial_joint_positions;
    std::size_t max_iterations{16};
    float tolerance{0.001F};
    bool clamp_unreachable_target{true};
};

struct AnimationFabrikIkXySolution {
    std::vector<Vec2> joint_positions;
    std::vector<float> segment_rotation_z_radians;
    float end_effector_error{0.0F};
    std::size_t iterations{0};
    bool reached{false};
    bool target_was_unreachable{false};
};

struct AnimationFabrikIk3dDesc {
    std::vector<float> segment_lengths;
    Vec3 root_position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 target_position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    std::vector<Vec3> initial_joint_positions;
    std::size_t max_iterations{16};
    float tolerance{0.001F};
    bool clamp_unreachable_target{true};
};

struct AnimationFabrikIk3dSolution {
    std::vector<Vec3> joint_positions;
    float end_effector_error{0.0F};
    std::size_t iterations{0};
    bool reached{false};
    bool target_was_unreachable{false};
};

struct AnimationIkLocalRotationLimit {
    std::size_t joint_index{0};
    float min_rotation_z_radians{0.0F};
    float max_rotation_z_radians{0.0F};
};

struct AnimationIkLocalRotationLimit3d {
    std::size_t joint_index{0};
    Vec3 twist_axis{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float max_swing_radians{0.0F};
    float min_twist_radians{0.0F};
    float max_twist_radians{0.0F};
};

enum class AnimationIkLocalRotationLimit3dDiagnosticCode {
    invalid_skeleton,
    invalid_joint_index,
    duplicate_joint,
    invalid_twist_axis,
    invalid_swing_limit,
    invalid_twist_range,
};

struct AnimationIkLocalRotationLimit3dDiagnostic {
    AnimationIkLocalRotationLimit3dDiagnosticCode code{AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_skeleton};
    std::size_t limit_index{0};
    std::size_t joint_index{0};
    std::string message;
};

struct AnimationLocalRotationLimit3dApplyResult {
    bool succeeded{false};
    bool constrained{false};
    std::size_t constrained_joint_count{0};
    std::string diagnostic;
};

/// Deterministic FABRIK chain IK in the XY plane. Invalid inputs return false with `diagnostic`.
[[nodiscard]] bool solve_animation_fabrik_ik_xy_chain(const AnimationFabrikIkXyDesc& desc,
                                                      AnimationFabrikIkXySolution& out, std::string& diagnostic);

/// Deterministic FABRIK chain IK in 3D position space. Invalid inputs return false with `diagnostic`.
[[nodiscard]] bool solve_animation_fabrik_ik_3d_chain(const AnimationFabrikIk3dDesc& desc,
                                                      AnimationFabrikIk3dSolution& out, std::string& diagnostic);

/// Validates host-independent 3D local swing/twist rotation limits for quaternion pose workflows.
[[nodiscard]] std::vector<AnimationIkLocalRotationLimit3dDiagnostic>
validate_animation_ik_local_rotation_limits_3d(const AnimationSkeleton3dDesc& skeleton,
                                               std::span<const AnimationIkLocalRotationLimit3d> local_rotation_limits);
[[nodiscard]] bool
is_valid_animation_ik_local_rotation_limits_3d(const AnimationSkeleton3dDesc& skeleton,
                                               std::span<const AnimationIkLocalRotationLimit3d> local_rotation_limits);

/// Clamps quaternion local rotations to deterministic swing/twist limits in-place. Invalid inputs leave `pose`
/// unchanged.
[[nodiscard]] AnimationLocalRotationLimit3dApplyResult
apply_animation_local_rotation_limits_3d(const AnimationSkeleton3dDesc& skeleton,
                                         std::span<const AnimationIkLocalRotationLimit3d> local_rotation_limits,
                                         AnimationPose3d& pose);

/// Applies a FABRIK XY solution to local pose Z rotations for an ordered joint chain including the end effector.
[[nodiscard]] bool apply_animation_fabrik_ik_xy_solution_to_pose(const AnimationSkeletonDesc& skeleton,
                                                                 const AnimationFabrikIkXySolution& solution,
                                                                 std::span<const std::size_t> joint_indices,
                                                                 AnimationPose& pose, std::string& diagnostic);
[[nodiscard]] bool apply_animation_fabrik_ik_xy_solution_to_pose(
    const AnimationSkeletonDesc& skeleton, const AnimationFabrikIkXySolution& solution,
    std::span<const std::size_t> joint_indices, std::span<const AnimationIkLocalRotationLimit> local_rotation_limits,
    AnimationPose& pose, std::string& diagnostic);

/// Applies a FABRIK 3D solution to local pose quaternion rotations for an ordered joint chain including the end
/// effector.
[[nodiscard]] bool apply_animation_fabrik_ik_3d_solution_to_pose(const AnimationSkeleton3dDesc& skeleton,
                                                                 const AnimationFabrikIk3dSolution& solution,
                                                                 std::span<const std::size_t> joint_indices,
                                                                 AnimationPose3d& pose, std::string& diagnostic);
[[nodiscard]] bool apply_animation_fabrik_ik_3d_solution_to_pose(
    const AnimationSkeleton3dDesc& skeleton, const AnimationFabrikIk3dSolution& solution,
    std::span<const std::size_t> joint_indices, std::span<const AnimationIkLocalRotationLimit3d> local_rotation_limits,
    AnimationPose3d& pose, std::string& diagnostic);

} // namespace mirakana
