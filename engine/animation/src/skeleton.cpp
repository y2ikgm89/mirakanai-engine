// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/skeleton.hpp"

#include "mirakana/animation/keyframe_animation.hpp"

#include "mirakana/math/mat4.hpp"
#include "mirakana/math/quat.hpp"
#include "mirakana/math/vec.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_vec(Vec3 value) noexcept {
    return finite(value.x) && finite(value.y) && finite(value.z);
}

[[nodiscard]] bool positive_vec(Vec3 value) noexcept {
    return finite_vec(value) && value.x > 0.0F && value.y > 0.0F && value.z > 0.0F;
}

[[nodiscard]] bool is_safe_name(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('=') == std::string_view::npos;
}

[[nodiscard]] bool is_safe_target(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] float read_f32_le(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    float value = 0.0F;
    std::memcpy(&value, &bytes[offset], sizeof(float));
    return value;
}

[[nodiscard]] bool is_valid_transform(JointLocalTransform transform) noexcept {
    return finite_vec(transform.translation) && finite(transform.rotation_z_radians) && positive_vec(transform.scale);
}

[[nodiscard]] bool is_valid_transform(AnimationJointLocalTransform3d transform) noexcept {
    return finite_vec(transform.translation) && is_normalized_quat(transform.rotation) && positive_vec(transform.scale);
}

[[nodiscard]] Mat4 local_transform_matrix(AnimationJointLocalTransform3d transform) noexcept {
    return Mat4::translation(transform.translation) * Mat4::rotation_quat(transform.rotation) *
           Mat4::scale(transform.scale);
}

void push_diagnostic(std::vector<AnimationSkeletonDiagnostic>& diagnostics, std::size_t joint_index,
                     AnimationSkeletonDiagnosticCode code, std::string message) {
    diagnostics.push_back(
        AnimationSkeletonDiagnostic{.joint_index = joint_index, .code = code, .message = std::move(message)});
}

void push_root_motion_diagnostic(std::vector<AnimationRootMotionDiagnostic>& diagnostics, std::size_t root_joint_index,
                                 AnimationRootMotionDiagnosticCode code, std::string message) {
    diagnostics.push_back(AnimationRootMotionDiagnostic{
        .root_joint_index = root_joint_index,
        .code = code,
        .message = std::move(message),
    });
}

[[nodiscard]] bool has_duplicate_track_binding(const std::vector<AnimationJointTrackDesc>& tracks,
                                               std::size_t index) noexcept {
    for (std::size_t other = 0; other < index; ++other) {
        if (tracks[other].joint_index == tracks[index].joint_index) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_duplicate_track_binding(const std::vector<AnimationJointTrack3dDesc>& tracks,
                                               std::size_t index) noexcept {
    for (std::size_t other = 0; other < index; ++other) {
        if (tracks[other].joint_index == tracks[index].joint_index) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool is_valid_scale_keyframes(const std::vector<Vec3Keyframe>& keyframes) noexcept {
    if (!is_valid_vec3_keyframes(keyframes)) {
        return false;
    }
    return std::ranges::all_of(keyframes, [](const Vec3Keyframe& keyframe) { return positive_vec(keyframe.value); });
}

[[nodiscard]] bool is_valid_skeleton_desc_noalloc(const AnimationSkeletonDesc& desc) noexcept {
    if (desc.joints.empty()) {
        return false;
    }

    for (std::size_t index = 0; index < desc.joints.size(); ++index) {
        const auto& joint = desc.joints[index];
        if (!is_safe_name(joint.name) || !is_valid_transform(joint.rest)) {
            return false;
        }
        for (std::size_t other = 0; other < index; ++other) {
            if (desc.joints[other].name == joint.name) {
                return false;
            }
        }
        if (joint.parent_index != animation_no_parent &&
            (joint.parent_index >= desc.joints.size() || joint.parent_index >= index)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::vector<Vec3Keyframe>
make_vec3_keyframes_from_f32_bytes(std::span<const std::uint8_t> time_seconds_bytes,
                                   std::span<const std::uint8_t> xyz_bytes, std::string_view diagnostic_name) {
    if (time_seconds_bytes.empty() && xyz_bytes.empty()) {
        return {};
    }
    if (time_seconds_bytes.empty() || (time_seconds_bytes.size() % 4U) != 0U) {
        throw std::invalid_argument(std::string(diagnostic_name) + " time byte length is invalid");
    }
    const auto keyframe_count = time_seconds_bytes.size() / 4U;
    if (xyz_bytes.size() != keyframe_count * 12U) {
        throw std::invalid_argument(std::string(diagnostic_name) + " value byte length is invalid");
    }

    std::vector<Vec3Keyframe> keyframes;
    keyframes.reserve(keyframe_count);
    for (std::size_t index = 0; index < keyframe_count; ++index) {
        const auto time_offset = index * 4U;
        const auto value_offset = index * 12U;
        keyframes.push_back(Vec3Keyframe{
            .time_seconds = read_f32_le(time_seconds_bytes, time_offset),
            .value =
                Vec3{
                    .x = read_f32_le(xyz_bytes, value_offset),
                    .y = read_f32_le(xyz_bytes, value_offset + 4U),
                    .z = read_f32_le(xyz_bytes, value_offset + 8U),
                },
        });
    }
    if (!is_valid_vec3_keyframes(keyframes)) {
        throw std::invalid_argument(std::string(diagnostic_name) + " keyframes are invalid");
    }
    return keyframes;
}

[[nodiscard]] std::vector<QuatKeyframe>
make_quat_keyframes_from_f32_bytes(std::span<const std::uint8_t> time_seconds_bytes,
                                   std::span<const std::uint8_t> xyzw_bytes, std::string_view diagnostic_name) {
    if (time_seconds_bytes.empty() && xyzw_bytes.empty()) {
        return {};
    }
    if (time_seconds_bytes.empty() || (time_seconds_bytes.size() % 4U) != 0U) {
        throw std::invalid_argument(std::string(diagnostic_name) + " time byte length is invalid");
    }
    const auto keyframe_count = time_seconds_bytes.size() / 4U;
    if (xyzw_bytes.size() != keyframe_count * 16U) {
        throw std::invalid_argument(std::string(diagnostic_name) + " value byte length is invalid");
    }

    std::vector<QuatKeyframe> keyframes;
    keyframes.reserve(keyframe_count);
    for (std::size_t index = 0; index < keyframe_count; ++index) {
        const auto time_offset = index * 4U;
        const auto value_offset = index * 16U;
        keyframes.push_back(QuatKeyframe{
            .time_seconds = read_f32_le(time_seconds_bytes, time_offset),
            .value =
                Quat{
                    .x = read_f32_le(xyzw_bytes, value_offset),
                    .y = read_f32_le(xyzw_bytes, value_offset + 4U),
                    .z = read_f32_le(xyzw_bytes, value_offset + 8U),
                    .w = read_f32_le(xyzw_bytes, value_offset + 12U),
                },
        });
    }
    if (!is_valid_quat_keyframes(keyframes)) {
        throw std::invalid_argument(std::string(diagnostic_name) + " keyframes are invalid");
    }
    return keyframes;
}

[[nodiscard]] bool is_valid_skeleton_desc_noalloc(const AnimationSkeleton3dDesc& desc) noexcept {
    if (desc.joints.empty()) {
        return false;
    }

    for (std::size_t index = 0; index < desc.joints.size(); ++index) {
        const auto& joint = desc.joints[index];
        if (!is_safe_name(joint.name) || !is_valid_transform(joint.rest)) {
            return false;
        }
        for (std::size_t other = 0; other < index; ++other) {
            if (desc.joints[other].name == joint.name) {
                return false;
            }
        }
        if (joint.parent_index != animation_no_parent &&
            (joint.parent_index >= desc.joints.size() || joint.parent_index >= index)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool is_valid_joint_tracks_noalloc(const AnimationSkeletonDesc& skeleton,
                                                 const std::vector<AnimationJointTrackDesc>& tracks) noexcept {
    if (!is_valid_skeleton_desc_noalloc(skeleton)) {
        return false;
    }

    for (std::size_t index = 0; index < tracks.size(); ++index) {
        const auto& track = tracks[index];
        if (track.joint_index >= skeleton.joints.size() || has_duplicate_track_binding(tracks, index)) {
            return false;
        }
        if ((!track.translation_keyframes.empty() && !is_valid_vec3_keyframes(track.translation_keyframes)) ||
            (!track.rotation_z_keyframes.empty() && !is_valid_float_keyframes(track.rotation_z_keyframes)) ||
            (!track.scale_keyframes.empty() && !is_valid_scale_keyframes(track.scale_keyframes))) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool is_valid_joint_tracks_noalloc(const AnimationSkeleton3dDesc& skeleton,
                                                 const std::vector<AnimationJointTrack3dDesc>& tracks) noexcept {
    if (!is_valid_skeleton_desc_noalloc(skeleton)) {
        return false;
    }

    for (std::size_t index = 0; index < tracks.size(); ++index) {
        const auto& track = tracks[index];
        if (track.joint_index >= skeleton.joints.size() || has_duplicate_track_binding(tracks, index)) {
            return false;
        }
        if ((!track.translation_keyframes.empty() && !is_valid_vec3_keyframes(track.translation_keyframes)) ||
            (!track.rotation_keyframes.empty() && !is_valid_quat_keyframes(track.rotation_keyframes)) ||
            (!track.scale_keyframes.empty() && !is_valid_scale_keyframes(track.scale_keyframes))) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool is_valid_root_motion_sample_noalloc(const AnimationSkeletonDesc& skeleton,
                                                       const std::vector<AnimationJointTrackDesc>& tracks,
                                                       const AnimationRootMotionSampleDesc& desc) noexcept {
    return is_valid_joint_tracks_noalloc(skeleton, tracks) && desc.root_joint_index < skeleton.joints.size() &&
           finite(desc.from_time_seconds) && finite(desc.to_time_seconds);
}

[[nodiscard]] bool has_representable_accumulation_span(const AnimationRootMotionAccumulationDesc& desc) noexcept {
    if (!finite(desc.from_time_seconds) || !finite(desc.to_time_seconds) || !finite(desc.clip_duration_seconds) ||
        desc.from_time_seconds < 0.0F || desc.to_time_seconds < desc.from_time_seconds ||
        desc.clip_duration_seconds <= 0.0F) {
        return false;
    }

    const auto normalized_span =
        (static_cast<double>(desc.to_time_seconds) - static_cast<double>(desc.from_time_seconds)) /
        static_cast<double>(desc.clip_duration_seconds);
    return std::isfinite(normalized_span) && normalized_span <= static_cast<double>(std::numeric_limits<float>::max());
}

[[nodiscard]] bool is_valid_root_motion_accumulation_noalloc(const AnimationSkeletonDesc& skeleton,
                                                             const std::vector<AnimationJointTrackDesc>& tracks,
                                                             const AnimationRootMotionAccumulationDesc& desc) noexcept {
    return is_valid_joint_tracks_noalloc(skeleton, tracks) && desc.root_joint_index < skeleton.joints.size() &&
           has_representable_accumulation_span(desc);
}

[[nodiscard]] float wrap_clip_time(float time_seconds, float clip_duration_seconds) noexcept {
    auto wrapped = std::fmod(time_seconds, clip_duration_seconds);
    if (wrapped < 0.0F) {
        wrapped += clip_duration_seconds;
    }
    return wrapped;
}

void add_root_motion_sample(AnimationRootMotionAccumulation& accumulation,
                            const AnimationRootMotionSample& sample) noexcept {
    accumulation.delta_translation = accumulation.delta_translation + sample.delta_translation;
    accumulation.delta_rotation_z_radians += sample.delta_rotation_z_radians;
}

} // namespace

std::vector<AnimationSkeletonDiagnostic> validate_animation_skeleton_desc(const AnimationSkeletonDesc& desc) {
    std::vector<AnimationSkeletonDiagnostic> diagnostics;
    if (desc.joints.empty()) {
        push_diagnostic(diagnostics, 0, AnimationSkeletonDiagnosticCode::empty_skeleton,
                        "animation skeleton must contain at least one joint");
        return diagnostics;
    }

    for (std::size_t index = 0; index < desc.joints.size(); ++index) {
        const auto& joint = desc.joints[index];
        if (!is_safe_name(joint.name)) {
            push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::invalid_joint_name,
                            "animation skeleton joint name is invalid");
        }
        for (std::size_t other = 0; other < index; ++other) {
            if (desc.joints[other].name == joint.name) {
                push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::duplicate_joint_name,
                                "animation skeleton joint name is duplicated");
                break;
            }
        }

        if (joint.parent_index != animation_no_parent) {
            if (joint.parent_index >= desc.joints.size()) {
                push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::invalid_parent_index,
                                "animation skeleton joint parent index is out of range");
            } else if (joint.parent_index >= index) {
                push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::parent_not_before_child,
                                "animation skeleton parent joint must appear before child joint");
            }
        }

        if (!is_valid_transform(joint.rest)) {
            push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::invalid_rest_transform,
                            "animation skeleton rest transform is invalid");
        }
    }

    return diagnostics;
}

bool is_valid_animation_skeleton_desc(const AnimationSkeletonDesc& desc) noexcept {
    return is_valid_skeleton_desc_noalloc(desc);
}

std::vector<AnimationSkeletonDiagnostic> validate_animation_skeleton_3d_desc(const AnimationSkeleton3dDesc& desc) {
    std::vector<AnimationSkeletonDiagnostic> diagnostics;
    if (desc.joints.empty()) {
        push_diagnostic(diagnostics, 0, AnimationSkeletonDiagnosticCode::empty_skeleton,
                        "animation 3D skeleton must contain at least one joint");
        return diagnostics;
    }

    for (std::size_t index = 0; index < desc.joints.size(); ++index) {
        const auto& joint = desc.joints[index];
        if (!is_safe_name(joint.name)) {
            push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::invalid_joint_name,
                            "animation 3D skeleton joint name is invalid");
        }
        for (std::size_t other = 0; other < index; ++other) {
            if (desc.joints[other].name == joint.name) {
                push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::duplicate_joint_name,
                                "animation 3D skeleton joint name is duplicated");
                break;
            }
        }

        if (joint.parent_index != animation_no_parent) {
            if (joint.parent_index >= desc.joints.size()) {
                push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::invalid_parent_index,
                                "animation 3D skeleton joint parent index is out of range");
            } else if (joint.parent_index >= index) {
                push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::parent_not_before_child,
                                "animation 3D skeleton parent joint must appear before child joint");
            }
        }

        if (!is_valid_transform(joint.rest)) {
            push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::invalid_rest_transform,
                            "animation 3D skeleton rest transform rotation, translation, or scale is invalid");
        }
    }

    return diagnostics;
}

bool is_valid_animation_skeleton_3d_desc(const AnimationSkeleton3dDesc& desc) noexcept {
    return is_valid_skeleton_desc_noalloc(desc);
}

std::vector<AnimationSkeletonDiagnostic> validate_animation_pose(const AnimationSkeletonDesc& skeleton,
                                                                 const AnimationPose& pose) {
    auto diagnostics = validate_animation_skeleton_desc(skeleton);
    if (!diagnostics.empty()) {
        return diagnostics;
    }

    if (pose.joints.size() != skeleton.joints.size()) {
        push_diagnostic(diagnostics, pose.joints.size(), AnimationSkeletonDiagnosticCode::pose_joint_count_mismatch,
                        "animation pose joint count does not match skeleton");
        return diagnostics;
    }

    for (std::size_t index = 0; index < pose.joints.size(); ++index) {
        if (!is_valid_transform(pose.joints[index])) {
            push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::invalid_pose_transform,
                            "animation pose joint transform is invalid");
        }
    }
    return diagnostics;
}

std::vector<AnimationSkeletonDiagnostic> validate_animation_pose_3d(const AnimationSkeleton3dDesc& skeleton,
                                                                    const AnimationPose3d& pose) {
    auto diagnostics = validate_animation_skeleton_3d_desc(skeleton);
    if (!diagnostics.empty()) {
        return diagnostics;
    }

    if (pose.joints.size() != skeleton.joints.size()) {
        push_diagnostic(diagnostics, pose.joints.size(), AnimationSkeletonDiagnosticCode::pose_joint_count_mismatch,
                        "animation 3D pose joint count does not match skeleton");
        return diagnostics;
    }

    for (std::size_t index = 0; index < pose.joints.size(); ++index) {
        if (!is_valid_transform(pose.joints[index])) {
            push_diagnostic(diagnostics, index, AnimationSkeletonDiagnosticCode::invalid_pose_transform,
                            "animation 3D pose joint transform rotation, translation, or scale is invalid");
        }
    }
    return diagnostics;
}

std::vector<AnimationSkeletonDiagnostic>
validate_animation_joint_tracks(const AnimationSkeletonDesc& skeleton,
                                const std::vector<AnimationJointTrackDesc>& tracks) {
    auto diagnostics = validate_animation_skeleton_desc(skeleton);
    if (!diagnostics.empty()) {
        return diagnostics;
    }

    for (std::size_t index = 0; index < tracks.size(); ++index) {
        const auto& track = tracks[index];
        if (track.joint_index >= skeleton.joints.size()) {
            push_diagnostic(diagnostics, track.joint_index, AnimationSkeletonDiagnosticCode::invalid_track_binding,
                            "animation joint track targets an out-of-range joint");
            continue;
        }
        if (has_duplicate_track_binding(tracks, index)) {
            push_diagnostic(diagnostics, track.joint_index, AnimationSkeletonDiagnosticCode::duplicate_track_binding,
                            "animation joint track targets a joint more than once");
        }
        if ((!track.translation_keyframes.empty() && !is_valid_vec3_keyframes(track.translation_keyframes)) ||
            (!track.rotation_z_keyframes.empty() && !is_valid_float_keyframes(track.rotation_z_keyframes)) ||
            (!track.scale_keyframes.empty() && !is_valid_scale_keyframes(track.scale_keyframes))) {
            push_diagnostic(diagnostics, track.joint_index, AnimationSkeletonDiagnosticCode::invalid_track_binding,
                            "animation joint track keyframes are invalid");
        }
    }

    return diagnostics;
}

bool is_valid_animation_joint_tracks(const AnimationSkeletonDesc& skeleton,
                                     const std::vector<AnimationJointTrackDesc>& tracks) noexcept {
    return is_valid_joint_tracks_noalloc(skeleton, tracks);
}

std::vector<AnimationSkeletonDiagnostic>
validate_animation_joint_tracks_3d(const AnimationSkeleton3dDesc& skeleton,
                                   const std::vector<AnimationJointTrack3dDesc>& tracks) {
    auto diagnostics = validate_animation_skeleton_3d_desc(skeleton);
    if (!diagnostics.empty()) {
        return diagnostics;
    }

    for (std::size_t index = 0; index < tracks.size(); ++index) {
        const auto& track = tracks[index];
        if (track.joint_index >= skeleton.joints.size()) {
            push_diagnostic(diagnostics, track.joint_index, AnimationSkeletonDiagnosticCode::invalid_track_binding,
                            "animation 3D joint track targets an out-of-range joint");
            continue;
        }
        if (has_duplicate_track_binding(tracks, index)) {
            push_diagnostic(diagnostics, track.joint_index, AnimationSkeletonDiagnosticCode::duplicate_track_binding,
                            "animation 3D joint track targets a joint more than once");
        }
        if ((!track.translation_keyframes.empty() && !is_valid_vec3_keyframes(track.translation_keyframes)) ||
            (!track.rotation_keyframes.empty() && !is_valid_quat_keyframes(track.rotation_keyframes)) ||
            (!track.scale_keyframes.empty() && !is_valid_scale_keyframes(track.scale_keyframes))) {
            push_diagnostic(diagnostics, track.joint_index, AnimationSkeletonDiagnosticCode::invalid_track_binding,
                            "animation 3D joint track keyframes are invalid");
        }
    }

    return diagnostics;
}

bool is_valid_animation_joint_tracks_3d(const AnimationSkeleton3dDesc& skeleton,
                                        const std::vector<AnimationJointTrack3dDesc>& tracks) noexcept {
    return is_valid_joint_tracks_noalloc(skeleton, tracks);
}

AnimationJointTrack3dDesc make_animation_joint_track_3d_from_f32_bytes(const AnimationJointTrack3dByteSource& source) {
    if (!is_safe_target(source.target)) {
        throw std::invalid_argument("animation 3D joint track byte source target is invalid");
    }

    AnimationJointTrack3dDesc track{
        .joint_index = source.joint_index,
        .translation_keyframes =
            make_vec3_keyframes_from_f32_bytes(source.translation_time_seconds_bytes, source.translation_xyz_bytes,
                                               "animation 3D translation byte source"),
        .rotation_keyframes = make_quat_keyframes_from_f32_bytes(
            source.rotation_time_seconds_bytes, source.rotation_xyzw_bytes, "animation 3D rotation byte source"),
        .scale_keyframes = make_vec3_keyframes_from_f32_bytes(source.scale_time_seconds_bytes, source.scale_xyz_bytes,
                                                              "animation 3D scale byte source"),
    };

    if (track.translation_keyframes.empty() && track.rotation_keyframes.empty() && track.scale_keyframes.empty()) {
        throw std::invalid_argument("animation 3D joint track byte source has no component tracks");
    }
    if (!track.scale_keyframes.empty() && !is_valid_scale_keyframes(track.scale_keyframes)) {
        throw std::invalid_argument("animation 3D scale byte source keyframes are invalid");
    }
    return track;
}

std::vector<AnimationJointTrack3dDesc>
make_animation_joint_tracks_3d_from_f32_bytes(std::span<const AnimationJointTrack3dByteSource> sources) {
    if (sources.empty()) {
        throw std::invalid_argument("animation 3D joint track byte sources are empty");
    }
    std::vector<AnimationJointTrack3dDesc> tracks;
    tracks.reserve(sources.size());
    for (const auto& source : sources) {
        tracks.push_back(make_animation_joint_track_3d_from_f32_bytes(source));
    }
    return tracks;
}

AnimationPose make_animation_rest_pose(const AnimationSkeletonDesc& skeleton) {
    if (!is_valid_animation_skeleton_desc(skeleton)) {
        throw std::invalid_argument("animation skeleton description is invalid");
    }

    AnimationPose pose;
    pose.joints.reserve(skeleton.joints.size());
    for (const auto& joint : skeleton.joints) {
        pose.joints.push_back(joint.rest);
    }
    return pose;
}

AnimationPose3d make_animation_rest_pose_3d(const AnimationSkeleton3dDesc& skeleton) {
    if (!is_valid_animation_skeleton_3d_desc(skeleton)) {
        throw std::invalid_argument("animation 3D skeleton description is invalid");
    }

    AnimationPose3d pose;
    pose.joints.reserve(skeleton.joints.size());
    for (const auto& joint : skeleton.joints) {
        pose.joints.push_back(joint.rest);
    }
    return pose;
}

AnimationModelPose3d build_animation_model_pose_3d(const AnimationSkeleton3dDesc& skeleton,
                                                   const AnimationPose3d& pose) {
    if (!validate_animation_pose_3d(skeleton, pose).empty()) {
        throw std::invalid_argument("animation 3D pose is invalid");
    }

    AnimationModelPose3d model_pose;
    model_pose.joint_matrices.reserve(pose.joints.size());
    for (std::size_t index = 0; index < pose.joints.size(); ++index) {
        auto matrix = local_transform_matrix(pose.joints[index]);
        const auto parent_index = skeleton.joints[index].parent_index;
        if (parent_index != animation_no_parent) {
            matrix = model_pose.joint_matrices[parent_index] * matrix;
        }
        model_pose.joint_matrices.push_back(matrix);
    }
    return model_pose;
}

AnimationPose sample_animation_local_pose(const AnimationSkeletonDesc& skeleton,
                                          const std::vector<AnimationJointTrackDesc>& tracks, float time_seconds) {
    if (!finite(time_seconds) || !is_valid_animation_joint_tracks(skeleton, tracks)) {
        throw std::invalid_argument("animation joint tracks are invalid");
    }

    auto pose = make_animation_rest_pose(skeleton);
    for (const auto& track : tracks) {
        auto& joint = pose.joints[track.joint_index];
        if (!track.translation_keyframes.empty()) {
            joint.translation = sample_vec3_keyframes(track.translation_keyframes, time_seconds);
        }
        if (!track.rotation_z_keyframes.empty()) {
            joint.rotation_z_radians = sample_float_keyframes(track.rotation_z_keyframes, time_seconds);
        }
        if (!track.scale_keyframes.empty()) {
            joint.scale = sample_vec3_keyframes(track.scale_keyframes, time_seconds);
        }
    }

    const auto pose_diagnostics = validate_animation_pose(skeleton, pose);
    if (!pose_diagnostics.empty()) {
        throw std::invalid_argument("sampled animation pose is invalid");
    }
    return pose;
}

AnimationPose3d sample_animation_local_pose_3d(const AnimationSkeleton3dDesc& skeleton,
                                               const std::vector<AnimationJointTrack3dDesc>& tracks,
                                               float time_seconds) {
    if (!finite(time_seconds) || !is_valid_animation_joint_tracks_3d(skeleton, tracks)) {
        throw std::invalid_argument("animation 3D joint tracks are invalid");
    }

    auto pose = make_animation_rest_pose_3d(skeleton);
    for (const auto& track : tracks) {
        auto& joint = pose.joints[track.joint_index];
        if (!track.translation_keyframes.empty()) {
            joint.translation = sample_vec3_keyframes(track.translation_keyframes, time_seconds);
        }
        if (!track.rotation_keyframes.empty()) {
            joint.rotation = sample_quat_keyframes(track.rotation_keyframes, time_seconds);
        }
        if (!track.scale_keyframes.empty()) {
            joint.scale = sample_vec3_keyframes(track.scale_keyframes, time_seconds);
        }
    }

    const auto pose_diagnostics = validate_animation_pose_3d(skeleton, pose);
    if (!pose_diagnostics.empty()) {
        throw std::invalid_argument("sampled animation 3D pose is invalid");
    }
    return pose;
}

std::vector<AnimationRootMotionDiagnostic>
validate_animation_root_motion_sample(const AnimationSkeletonDesc& skeleton,
                                      const std::vector<AnimationJointTrackDesc>& tracks,
                                      const AnimationRootMotionSampleDesc& desc) {
    std::vector<AnimationRootMotionDiagnostic> diagnostics;
    if (!is_valid_animation_skeleton_desc(skeleton)) {
        push_root_motion_diagnostic(diagnostics, desc.root_joint_index,
                                    AnimationRootMotionDiagnosticCode::invalid_skeleton,
                                    "animation root motion skeleton is invalid");
        return diagnostics;
    }
    if (!is_valid_animation_joint_tracks(skeleton, tracks)) {
        push_root_motion_diagnostic(diagnostics, desc.root_joint_index,
                                    AnimationRootMotionDiagnosticCode::invalid_joint_tracks,
                                    "animation root motion joint tracks are invalid");
    }
    if (desc.root_joint_index >= skeleton.joints.size()) {
        push_root_motion_diagnostic(diagnostics, desc.root_joint_index,
                                    AnimationRootMotionDiagnosticCode::invalid_root_joint_index,
                                    "animation root motion root joint index is out of range");
    }
    if (!finite(desc.from_time_seconds) || !finite(desc.to_time_seconds)) {
        push_root_motion_diagnostic(diagnostics, desc.root_joint_index, AnimationRootMotionDiagnosticCode::invalid_time,
                                    "animation root motion sample times must be finite");
    }
    return diagnostics;
}

bool is_valid_animation_root_motion_sample(const AnimationSkeletonDesc& skeleton,
                                           const std::vector<AnimationJointTrackDesc>& tracks,
                                           const AnimationRootMotionSampleDesc& desc) noexcept {
    return is_valid_root_motion_sample_noalloc(skeleton, tracks, desc);
}

AnimationRootMotionSample sample_animation_root_motion(const AnimationSkeletonDesc& skeleton,
                                                       const std::vector<AnimationJointTrackDesc>& tracks,
                                                       const AnimationRootMotionSampleDesc& desc) {
    if (!is_valid_animation_root_motion_sample(skeleton, tracks, desc)) {
        throw std::invalid_argument("animation root motion sample is invalid");
    }

    const auto start_pose = sample_animation_local_pose(skeleton, tracks, desc.from_time_seconds);
    const auto end_pose = sample_animation_local_pose(skeleton, tracks, desc.to_time_seconds);
    const auto start = start_pose.joints[desc.root_joint_index].translation;
    const auto end = end_pose.joints[desc.root_joint_index].translation;
    const auto start_rotation_z = start_pose.joints[desc.root_joint_index].rotation_z_radians;
    const auto end_rotation_z = end_pose.joints[desc.root_joint_index].rotation_z_radians;
    return AnimationRootMotionSample{
        .start_translation = start,
        .end_translation = end,
        .delta_translation = end - start,
        .start_rotation_z_radians = start_rotation_z,
        .end_rotation_z_radians = end_rotation_z,
        .delta_rotation_z_radians = end_rotation_z - start_rotation_z,
    };
}

std::vector<AnimationRootMotionDiagnostic>
validate_animation_root_motion_accumulation(const AnimationSkeletonDesc& skeleton,
                                            const std::vector<AnimationJointTrackDesc>& tracks,
                                            const AnimationRootMotionAccumulationDesc& desc) {
    std::vector<AnimationRootMotionDiagnostic> diagnostics;
    if (!is_valid_animation_skeleton_desc(skeleton)) {
        push_root_motion_diagnostic(diagnostics, desc.root_joint_index,
                                    AnimationRootMotionDiagnosticCode::invalid_skeleton,
                                    "animation root motion accumulation skeleton is invalid");
        return diagnostics;
    }
    if (!is_valid_animation_joint_tracks(skeleton, tracks)) {
        push_root_motion_diagnostic(diagnostics, desc.root_joint_index,
                                    AnimationRootMotionDiagnosticCode::invalid_joint_tracks,
                                    "animation root motion accumulation joint tracks are invalid");
    }
    if (desc.root_joint_index >= skeleton.joints.size()) {
        push_root_motion_diagnostic(diagnostics, desc.root_joint_index,
                                    AnimationRootMotionDiagnosticCode::invalid_root_joint_index,
                                    "animation root motion accumulation root joint index is out of range");
    }
    const auto invalid_time = !finite(desc.from_time_seconds) || !finite(desc.to_time_seconds) ||
                              desc.from_time_seconds < 0.0F || desc.to_time_seconds < desc.from_time_seconds;
    const auto invalid_clip_duration = !finite(desc.clip_duration_seconds) || desc.clip_duration_seconds <= 0.0F;
    if (invalid_time) {
        push_root_motion_diagnostic(
            diagnostics, desc.root_joint_index, AnimationRootMotionDiagnosticCode::invalid_time,
            "animation root motion accumulation times must be finite, non-negative, and forward");
    } else if (!invalid_clip_duration && !has_representable_accumulation_span(desc)) {
        push_root_motion_diagnostic(diagnostics, desc.root_joint_index, AnimationRootMotionDiagnosticCode::invalid_time,
                                    "animation root motion accumulation interval is not representable");
    }
    if (invalid_clip_duration) {
        push_root_motion_diagnostic(diagnostics, desc.root_joint_index,
                                    AnimationRootMotionDiagnosticCode::invalid_clip_duration,
                                    "animation root motion accumulation clip duration must be finite and positive");
    }
    return diagnostics;
}

bool is_valid_animation_root_motion_accumulation(const AnimationSkeletonDesc& skeleton,
                                                 const std::vector<AnimationJointTrackDesc>& tracks,
                                                 const AnimationRootMotionAccumulationDesc& desc) noexcept {
    return is_valid_root_motion_accumulation_noalloc(skeleton, tracks, desc);
}

AnimationRootMotionAccumulation accumulate_animation_root_motion(const AnimationSkeletonDesc& skeleton,
                                                                 const std::vector<AnimationJointTrackDesc>& tracks,
                                                                 const AnimationRootMotionAccumulationDesc& desc) {
    if (!is_valid_animation_root_motion_accumulation(skeleton, tracks, desc)) {
        throw std::invalid_argument("animation root motion accumulation is invalid");
    }

    AnimationRootMotionAccumulation accumulation;
    if (desc.from_time_seconds == desc.to_time_seconds) {
        return accumulation;
    }

    const auto duration = desc.clip_duration_seconds;
    const auto sample_segment = [&](float from_time_seconds, float to_time_seconds) {
        if (from_time_seconds == to_time_seconds) {
            return;
        }
        add_root_motion_sample(accumulation, sample_animation_root_motion(skeleton, tracks,
                                                                          AnimationRootMotionSampleDesc{
                                                                              .root_joint_index = desc.root_joint_index,
                                                                              .from_time_seconds = from_time_seconds,
                                                                              .to_time_seconds = to_time_seconds,
                                                                          }));
    };

    const auto from_cycle = std::floor(static_cast<double>(desc.from_time_seconds) / static_cast<double>(duration));
    const auto to_cycle = std::floor(static_cast<double>(desc.to_time_seconds) / static_cast<double>(duration));
    const auto from_local = wrap_clip_time(desc.from_time_seconds, duration);
    const auto to_local = wrap_clip_time(desc.to_time_seconds, duration);

    if (from_cycle == to_cycle) {
        sample_segment(from_local, to_local);
        return accumulation;
    }

    if (from_local != 0.0F) {
        sample_segment(from_local, duration);
    }

    const auto full_start_cycle = from_cycle + (from_local == 0.0F ? 0.0 : 1.0);
    const auto full_end_cycle = to_cycle;
    const auto full_loop_count = std::max(0.0, full_end_cycle - full_start_cycle);
    if (full_loop_count > 0.0) {
        const auto loop = sample_animation_root_motion(skeleton, tracks,
                                                       AnimationRootMotionSampleDesc{
                                                           .root_joint_index = desc.root_joint_index,
                                                           .from_time_seconds = 0.0F,
                                                           .to_time_seconds = duration,
                                                       });
        const auto loop_count = static_cast<float>(full_loop_count);
        accumulation.delta_translation = accumulation.delta_translation + (loop.delta_translation * loop_count);
        accumulation.delta_rotation_z_radians += loop.delta_rotation_z_radians * loop_count;
    }

    if (to_local != 0.0F) {
        sample_segment(0.0F, to_local);
    }

    return accumulation;
}

} // namespace mirakana
