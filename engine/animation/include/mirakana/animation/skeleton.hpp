// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/animation/keyframe_animation.hpp"
#include "mirakana/math/mat4.hpp"
#include "mirakana/math/quat.hpp"
#include "mirakana/math/vec.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

inline constexpr std::size_t animation_no_parent = static_cast<std::size_t>(-1);

struct JointLocalTransform {
    Vec3 translation{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    float rotation_z_radians{0.0F};
    Vec3 scale{.x = 1.0F, .y = 1.0F, .z = 1.0F};
};

struct AnimationJointLocalTransform3d {
    Vec3 translation{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Quat rotation{Quat::identity()};
    Vec3 scale{.x = 1.0F, .y = 1.0F, .z = 1.0F};
};

struct AnimationSkeletonJointDesc {
    std::string name;
    std::size_t parent_index{animation_no_parent};
    JointLocalTransform rest;
};

struct AnimationSkeleton3dJointDesc {
    std::string name;
    std::size_t parent_index{animation_no_parent};
    AnimationJointLocalTransform3d rest;
};

struct AnimationSkeletonDesc {
    std::vector<AnimationSkeletonJointDesc> joints;
};

struct AnimationSkeleton3dDesc {
    std::vector<AnimationSkeleton3dJointDesc> joints;
};

struct AnimationPose {
    std::vector<JointLocalTransform> joints;
};

struct AnimationPose3d {
    std::vector<AnimationJointLocalTransform3d> joints;
};

struct AnimationModelPose3d {
    std::vector<Mat4> joint_matrices;
};

struct AnimationJointTrackDesc {
    std::size_t joint_index{0};
    std::vector<Vec3Keyframe> translation_keyframes;
    std::vector<FloatKeyframe> rotation_z_keyframes;
    std::vector<Vec3Keyframe> scale_keyframes;
};

struct AnimationJointTrack3dDesc {
    std::size_t joint_index{0};
    std::vector<Vec3Keyframe> translation_keyframes;
    std::vector<QuatKeyframe> rotation_keyframes;
    std::vector<Vec3Keyframe> scale_keyframes;
};

struct AnimationJointTrack3dByteSource {
    std::size_t joint_index{0};
    std::string_view target;
    std::span<const std::uint8_t> translation_time_seconds_bytes;
    std::span<const std::uint8_t> translation_xyz_bytes;
    std::span<const std::uint8_t> rotation_time_seconds_bytes;
    std::span<const std::uint8_t> rotation_xyzw_bytes;
    std::span<const std::uint8_t> scale_time_seconds_bytes;
    std::span<const std::uint8_t> scale_xyz_bytes;
};

struct AnimationRootMotionSampleDesc {
    std::size_t root_joint_index{0};
    float from_time_seconds{0.0F};
    float to_time_seconds{0.0F};
};

struct AnimationRootMotionSample {
    Vec3 start_translation{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 end_translation{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 delta_translation{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    float start_rotation_z_radians{0.0F};
    float end_rotation_z_radians{0.0F};
    float delta_rotation_z_radians{0.0F};
};

struct AnimationRootMotionAccumulationDesc {
    std::size_t root_joint_index{0};
    float from_time_seconds{0.0F};
    float to_time_seconds{0.0F};
    float clip_duration_seconds{0.0F};
};

struct AnimationRootMotionAccumulation {
    Vec3 delta_translation{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    float delta_rotation_z_radians{0.0F};
};

enum class AnimationSkeletonDiagnosticCode : std::uint8_t {
    empty_skeleton,
    invalid_joint_name,
    duplicate_joint_name,
    invalid_parent_index,
    parent_not_before_child,
    invalid_rest_transform,
    pose_joint_count_mismatch,
    invalid_pose_transform,
    invalid_track_binding,
    duplicate_track_binding,
};

struct AnimationSkeletonDiagnostic {
    std::size_t joint_index{0};
    AnimationSkeletonDiagnosticCode code{AnimationSkeletonDiagnosticCode::empty_skeleton};
    std::string message;
};

enum class AnimationRootMotionDiagnosticCode : std::uint8_t {
    invalid_skeleton,
    invalid_joint_tracks,
    invalid_root_joint_index,
    invalid_time,
    invalid_clip_duration,
};

struct AnimationRootMotionDiagnostic {
    std::size_t root_joint_index{0};
    AnimationRootMotionDiagnosticCode code{AnimationRootMotionDiagnosticCode::invalid_skeleton};
    std::string message;
};

[[nodiscard]] std::vector<AnimationSkeletonDiagnostic>
validate_animation_skeleton_desc(const AnimationSkeletonDesc& desc);
[[nodiscard]] bool is_valid_animation_skeleton_desc(const AnimationSkeletonDesc& desc) noexcept;
[[nodiscard]] std::vector<AnimationSkeletonDiagnostic> validate_animation_pose(const AnimationSkeletonDesc& skeleton,
                                                                               const AnimationPose& pose);
[[nodiscard]] std::vector<AnimationSkeletonDiagnostic>
validate_animation_skeleton_3d_desc(const AnimationSkeleton3dDesc& desc);
[[nodiscard]] bool is_valid_animation_skeleton_3d_desc(const AnimationSkeleton3dDesc& desc) noexcept;
[[nodiscard]] std::vector<AnimationSkeletonDiagnostic>
validate_animation_pose_3d(const AnimationSkeleton3dDesc& skeleton, const AnimationPose3d& pose);
[[nodiscard]] std::vector<AnimationSkeletonDiagnostic>
validate_animation_joint_tracks(const AnimationSkeletonDesc& skeleton,
                                const std::vector<AnimationJointTrackDesc>& tracks);
[[nodiscard]] bool is_valid_animation_joint_tracks(const AnimationSkeletonDesc& skeleton,
                                                   const std::vector<AnimationJointTrackDesc>& tracks) noexcept;
[[nodiscard]] std::vector<AnimationSkeletonDiagnostic>
validate_animation_joint_tracks_3d(const AnimationSkeleton3dDesc& skeleton,
                                   const std::vector<AnimationJointTrack3dDesc>& tracks);
[[nodiscard]] bool is_valid_animation_joint_tracks_3d(const AnimationSkeleton3dDesc& skeleton,
                                                      const std::vector<AnimationJointTrack3dDesc>& tracks) noexcept;
[[nodiscard]] AnimationJointTrack3dDesc
make_animation_joint_track_3d_from_f32_bytes(const AnimationJointTrack3dByteSource& source);
[[nodiscard]] std::vector<AnimationJointTrack3dDesc>
make_animation_joint_tracks_3d_from_f32_bytes(std::span<const AnimationJointTrack3dByteSource> sources);

[[nodiscard]] AnimationPose make_animation_rest_pose(const AnimationSkeletonDesc& skeleton);
[[nodiscard]] AnimationPose3d make_animation_rest_pose_3d(const AnimationSkeleton3dDesc& skeleton);
[[nodiscard]] AnimationModelPose3d build_animation_model_pose_3d(const AnimationSkeleton3dDesc& skeleton,
                                                                 const AnimationPose3d& pose);
[[nodiscard]] AnimationPose sample_animation_local_pose(const AnimationSkeletonDesc& skeleton,
                                                        const std::vector<AnimationJointTrackDesc>& tracks,
                                                        float time_seconds);
[[nodiscard]] AnimationPose3d sample_animation_local_pose_3d(const AnimationSkeleton3dDesc& skeleton,
                                                             const std::vector<AnimationJointTrack3dDesc>& tracks,
                                                             float time_seconds);
[[nodiscard]] std::vector<AnimationRootMotionDiagnostic>
validate_animation_root_motion_sample(const AnimationSkeletonDesc& skeleton,
                                      const std::vector<AnimationJointTrackDesc>& tracks,
                                      const AnimationRootMotionSampleDesc& desc);
[[nodiscard]] bool is_valid_animation_root_motion_sample(const AnimationSkeletonDesc& skeleton,
                                                         const std::vector<AnimationJointTrackDesc>& tracks,
                                                         const AnimationRootMotionSampleDesc& desc) noexcept;
[[nodiscard]] AnimationRootMotionSample sample_animation_root_motion(const AnimationSkeletonDesc& skeleton,
                                                                     const std::vector<AnimationJointTrackDesc>& tracks,
                                                                     const AnimationRootMotionSampleDesc& desc);
[[nodiscard]] std::vector<AnimationRootMotionDiagnostic>
validate_animation_root_motion_accumulation(const AnimationSkeletonDesc& skeleton,
                                            const std::vector<AnimationJointTrackDesc>& tracks,
                                            const AnimationRootMotionAccumulationDesc& desc);
[[nodiscard]] bool
is_valid_animation_root_motion_accumulation(const AnimationSkeletonDesc& skeleton,
                                            const std::vector<AnimationJointTrackDesc>& tracks,
                                            const AnimationRootMotionAccumulationDesc& desc) noexcept;
[[nodiscard]] AnimationRootMotionAccumulation
accumulate_animation_root_motion(const AnimationSkeletonDesc& skeleton,
                                 const std::vector<AnimationJointTrackDesc>& tracks,
                                 const AnimationRootMotionAccumulationDesc& desc);

} // namespace mirakana
