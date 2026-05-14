// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/quat.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/math/vec.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct FloatKeyframe {
    float time_seconds{0.0F};
    float value{0.0F};
};

struct FloatAnimationTrack {
    std::string target;
    std::vector<FloatKeyframe> keyframes;
};

struct FloatAnimationTrackByteSource {
    std::string_view target;
    std::span<const std::uint8_t> time_seconds_bytes;
    std::span<const std::uint8_t> value_bytes;
};

struct Vec3Keyframe {
    float time_seconds{0.0F};
    Vec3 value{.x = 0.0F, .y = 0.0F, .z = 0.0F};
};

struct Vec3AnimationTrack {
    std::string target;
    std::vector<Vec3Keyframe> keyframes;
};

struct QuatKeyframe {
    float time_seconds{0.0F};
    Quat value{Quat::identity()};
};

struct QuatAnimationTrack {
    std::string target;
    std::vector<QuatKeyframe> keyframes;
};

struct FloatAnimationCurveSample {
    std::string target;
    float value{0.0F};
};

enum class AnimationTransformComponent : std::uint8_t {
    translation_x,
    translation_y,
    translation_z,
    rotation_z,
    scale_x,
    scale_y,
    scale_z,
};

struct AnimationTransformCurveBinding {
    std::string target;
    std::size_t transform_index{0};
    AnimationTransformComponent component{AnimationTransformComponent::translation_x};
};

struct AnimationTransformApplyResult {
    bool succeeded{false};
    std::string diagnostic;
    std::size_t applied_sample_count{0};
};

[[nodiscard]] bool is_valid_float_keyframes(const std::vector<FloatKeyframe>& keyframes) noexcept;
[[nodiscard]] bool is_valid_vec3_keyframes(const std::vector<Vec3Keyframe>& keyframes) noexcept;
[[nodiscard]] bool is_valid_quat_keyframes(const std::vector<QuatKeyframe>& keyframes) noexcept;
[[nodiscard]] bool is_valid_float_animation_track(const FloatAnimationTrack& track) noexcept;
[[nodiscard]] bool is_valid_quat_animation_track(const QuatAnimationTrack& track) noexcept;

[[nodiscard]] FloatAnimationTrack
make_float_animation_track_from_f32_bytes(const FloatAnimationTrackByteSource& source);
[[nodiscard]] std::vector<FloatAnimationTrack>
make_float_animation_tracks_from_f32_bytes(std::span<const FloatAnimationTrackByteSource> sources);

[[nodiscard]] float sample_float_keyframes(const std::vector<FloatKeyframe>& keyframes, float time_seconds);
[[nodiscard]] Vec3 sample_vec3_keyframes(const std::vector<Vec3Keyframe>& keyframes, float time_seconds);
[[nodiscard]] Quat sample_quat_keyframes(const std::vector<QuatKeyframe>& keyframes, float time_seconds);
[[nodiscard]] std::vector<FloatAnimationCurveSample>
sample_float_animation_tracks(const std::vector<FloatAnimationTrack>& tracks, float time_seconds);
[[nodiscard]] AnimationTransformApplyResult
apply_float_animation_samples_to_transform3d(std::span<const FloatAnimationCurveSample> samples,
                                             std::span<const AnimationTransformCurveBinding> bindings,
                                             std::span<Transform3D> transforms);

} // namespace mirakana
