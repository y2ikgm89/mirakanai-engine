// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace mirakana {

enum class AnimationTwoBoneIkXyBendSide : std::uint8_t {
    positive,
    negative,
};

/// Minimal analytic two-bone IK in the parent joint XY plane (Z-axis joint rotations elsewhere in
/// `mirakana_animation`).
struct AnimationTwoBoneIkXyDesc {
    float parent_bone_length{1.0F};
    float child_bone_length{1.0F};
    Vec2 target_xy{.x = 1.0F, .y = 0.0F};
    /// Selects which side of the root-to-target axis contains the intermediate joint when no pole vector is set.
    AnimationTwoBoneIkXyBendSide bend_side{AnimationTwoBoneIkXyBendSide::positive};
    /// Optional local XY guide vector from the root. When set, its side relative to `target_xy` overrides `bend_side`.
    std::optional<Vec2> pole_vector_xy;
};

struct AnimationTwoBoneIkXySolution {
    float parent_rotation_z_radians{0.0F};
    float child_rotation_z_radians{0.0F};
};

/// Returns false with a stable English `diagnostic` when inputs are invalid or the target is unreachable.
[[nodiscard]] bool solve_animation_two_bone_ik_xy_plane(const AnimationTwoBoneIkXyDesc& desc,
                                                        AnimationTwoBoneIkXySolution& out, std::string& diagnostic);

/// Orthonormal 3D joint frame. `x_axis` points along the solved bone direction.
struct AnimationTwoBoneIk3dJointFrame {
    Vec3 x_axis{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    Vec3 y_axis{.x = 0.0F, .y = 1.0F, .z = 0.0F};
    Vec3 z_axis{.x = 0.0F, .y = 0.0F, .z = 1.0F};
};

/// Minimal analytic two-bone IK in 3D target space using a pole guide to select the bend plane.
struct AnimationTwoBoneIk3dDesc {
    float parent_bone_length{1.0F};
    float child_bone_length{1.0F};
    Vec3 target{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    Vec3 pole_vector{.x = 0.0F, .y = 1.0F, .z = 0.0F};
};

struct AnimationTwoBoneIk3dSolution {
    Vec3 root_position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 elbow_position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 tip_position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    AnimationTwoBoneIk3dJointFrame parent_frame;
    AnimationTwoBoneIk3dJointFrame child_frame;
};

/// Returns false with a stable English `diagnostic` when inputs are invalid or the target is unreachable.
[[nodiscard]] bool solve_animation_two_bone_ik_3d_orientation(const AnimationTwoBoneIk3dDesc& desc,
                                                              AnimationTwoBoneIk3dSolution& out,
                                                              std::string& diagnostic);

} // namespace mirakana
