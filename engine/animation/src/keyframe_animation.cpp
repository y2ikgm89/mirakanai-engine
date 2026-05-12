// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/keyframe_animation.hpp"

#include "mirakana/math/quat.hpp"
#include "mirakana/math/vec.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_vec(Vec3 value) noexcept {
    return finite(value.x) && finite(value.y) && finite(value.z);
}

[[nodiscard]] bool is_safe_target(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] float lerp(float lhs, float rhs, float t) noexcept {
    return lhs + ((rhs - lhs) * t);
}

[[nodiscard]] Vec3 lerp(Vec3 lhs, Vec3 rhs, float t) noexcept {
    return Vec3{.x = lerp(lhs.x, rhs.x, t), .y = lerp(lhs.y, rhs.y, t), .z = lerp(lhs.z, rhs.z, t)};
}

[[nodiscard]] Quat lerp(Quat lhs, Quat rhs, float t) noexcept {
    return Quat{
        .x = lerp(lhs.x, rhs.x, t),
        .y = lerp(lhs.y, rhs.y, t),
        .z = lerp(lhs.z, rhs.z, t),
        .w = lerp(lhs.w, rhs.w, t),
    };
}

[[nodiscard]] float dot_quat(Quat lhs, Quat rhs) noexcept {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
}

[[nodiscard]] Quat negated(Quat value) noexcept {
    return Quat{.x = -value.x, .y = -value.y, .z = -value.z, .w = -value.w};
}

[[nodiscard]] Quat slerp_shortest_path(Quat lhs, Quat rhs, float t) noexcept {
    const auto from = normalize_quat(lhs);
    auto to = normalize_quat(rhs);
    auto cosine = dot_quat(from, to);
    if (cosine < 0.0F) {
        to = negated(to);
        cosine = -cosine;
    }
    cosine = std::clamp(cosine, -1.0F, 1.0F);
    if (cosine > 0.9995F) {
        return normalize_quat(lerp(from, to, t));
    }

    const auto angle = std::acos(cosine);
    const auto sine = std::sin(angle);
    if (!finite(sine) || std::abs(sine) <= 0.000001F) {
        return normalize_quat(lerp(from, to, t));
    }

    const auto lhs_weight = std::sin((1.0F - t) * angle) / sine;
    const auto rhs_weight = std::sin(t * angle) / sine;
    return normalize_quat(Quat{
        .x = (from.x * lhs_weight) + (to.x * rhs_weight),
        .y = (from.y * lhs_weight) + (to.y * rhs_weight),
        .z = (from.z * lhs_weight) + (to.z * rhs_weight),
        .w = (from.w * lhs_weight) + (to.w * rhs_weight),
    });
}

[[nodiscard]] bool is_valid_transform_component(AnimationTransformComponent component) noexcept {
    switch (component) {
    case AnimationTransformComponent::translation_x:
    case AnimationTransformComponent::translation_y:
    case AnimationTransformComponent::translation_z:
    case AnimationTransformComponent::rotation_z:
    case AnimationTransformComponent::scale_x:
    case AnimationTransformComponent::scale_y:
    case AnimationTransformComponent::scale_z:
        return true;
    }
    return false;
}

[[nodiscard]] bool is_scale_component(AnimationTransformComponent component) noexcept {
    return component == AnimationTransformComponent::scale_x || component == AnimationTransformComponent::scale_y ||
           component == AnimationTransformComponent::scale_z;
}

[[nodiscard]] const FloatAnimationCurveSample* find_sample(std::span<const FloatAnimationCurveSample> samples,
                                                           std::string_view target) noexcept {
    for (const auto& sample : samples) {
        if (sample.target == target) {
            return &sample;
        }
    }
    return nullptr;
}

void apply_transform_sample(Transform3D& transform, AnimationTransformComponent component, float value) noexcept {
    switch (component) {
    case AnimationTransformComponent::translation_x:
        transform.position.x = value;
        break;
    case AnimationTransformComponent::translation_y:
        transform.position.y = value;
        break;
    case AnimationTransformComponent::translation_z:
        transform.position.z = value;
        break;
    case AnimationTransformComponent::rotation_z:
        transform.rotation_radians.z = value;
        break;
    case AnimationTransformComponent::scale_x:
        transform.scale.x = value;
        break;
    case AnimationTransformComponent::scale_y:
        transform.scale.y = value;
        break;
    case AnimationTransformComponent::scale_z:
        transform.scale.z = value;
        break;
    }
}

[[nodiscard]] float read_f32_le(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    const std::uint32_t bits = static_cast<std::uint32_t>(bytes[offset]) |
                               (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
                               (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                               (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
    float value = 0.0F;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}

} // namespace

bool is_valid_float_keyframes(const std::vector<FloatKeyframe>& keyframes) noexcept {
    if (keyframes.empty()) {
        return false;
    }
    float previous_time = -1.0F;
    for (const auto& keyframe : keyframes) {
        if (!finite(keyframe.time_seconds) || !finite(keyframe.value) || keyframe.time_seconds < 0.0F ||
            keyframe.time_seconds <= previous_time) {
            return false;
        }
        previous_time = keyframe.time_seconds;
    }
    return true;
}

bool is_valid_vec3_keyframes(const std::vector<Vec3Keyframe>& keyframes) noexcept {
    if (keyframes.empty()) {
        return false;
    }
    float previous_time = -1.0F;
    for (const auto& keyframe : keyframes) {
        if (!finite(keyframe.time_seconds) || !finite_vec(keyframe.value) || keyframe.time_seconds < 0.0F ||
            keyframe.time_seconds <= previous_time) {
            return false;
        }
        previous_time = keyframe.time_seconds;
    }
    return true;
}

bool is_valid_quat_keyframes(const std::vector<QuatKeyframe>& keyframes) noexcept {
    if (keyframes.empty()) {
        return false;
    }
    float previous_time = -1.0F;
    for (const auto& keyframe : keyframes) {
        if (!finite(keyframe.time_seconds) || keyframe.time_seconds < 0.0F || keyframe.time_seconds <= previous_time ||
            !is_normalized_quat(keyframe.value)) {
            return false;
        }
        previous_time = keyframe.time_seconds;
    }
    return true;
}

bool is_valid_float_animation_track(const FloatAnimationTrack& track) noexcept {
    return is_safe_target(track.target) && is_valid_float_keyframes(track.keyframes);
}

bool is_valid_quat_animation_track(const QuatAnimationTrack& track) noexcept {
    return is_safe_target(track.target) && is_valid_quat_keyframes(track.keyframes);
}

FloatAnimationTrack make_float_animation_track_from_f32_bytes(const FloatAnimationTrackByteSource& source) {
    if (source.time_seconds_bytes.empty() || source.time_seconds_bytes.size() != source.value_bytes.size() ||
        (source.time_seconds_bytes.size() % 4U) != 0U) {
        throw std::invalid_argument("float animation byte source has invalid byte lengths");
    }

    FloatAnimationTrack track;
    track.target = std::string(source.target);
    const auto keyframe_count = source.time_seconds_bytes.size() / 4U;
    track.keyframes.reserve(keyframe_count);
    for (std::size_t index = 0; index < keyframe_count; ++index) {
        const auto byte_offset = index * 4U;
        track.keyframes.push_back(FloatKeyframe{
            .time_seconds = read_f32_le(source.time_seconds_bytes, byte_offset),
            .value = read_f32_le(source.value_bytes, byte_offset),
        });
    }

    if (!is_valid_float_animation_track(track)) {
        throw std::invalid_argument("float animation byte source decodes to an invalid track");
    }
    return track;
}

std::vector<FloatAnimationTrack>
make_float_animation_tracks_from_f32_bytes(std::span<const FloatAnimationTrackByteSource> sources) {
    if (sources.empty()) {
        throw std::invalid_argument("float animation byte sources are empty");
    }
    std::vector<FloatAnimationTrack> tracks;
    tracks.reserve(sources.size());
    for (const auto& source : sources) {
        tracks.push_back(make_float_animation_track_from_f32_bytes(source));
    }
    return tracks;
}

float sample_float_keyframes(const std::vector<FloatKeyframe>& keyframes, float time_seconds) {
    if (!is_valid_float_keyframes(keyframes) || !finite(time_seconds)) {
        throw std::invalid_argument("float animation keyframes are invalid");
    }
    if (time_seconds <= keyframes.front().time_seconds) {
        return keyframes.front().value;
    }
    if (time_seconds >= keyframes.back().time_seconds) {
        return keyframes.back().value;
    }

    for (std::size_t index = 1; index < keyframes.size(); ++index) {
        if (time_seconds <= keyframes[index].time_seconds) {
            const auto& previous = keyframes[index - 1U];
            const auto& next = keyframes[index];
            const auto duration = next.time_seconds - previous.time_seconds;
            return lerp(previous.value, next.value, (time_seconds - previous.time_seconds) / duration);
        }
    }
    return keyframes.back().value;
}

std::vector<FloatAnimationCurveSample> sample_float_animation_tracks(const std::vector<FloatAnimationTrack>& tracks,
                                                                     float time_seconds) {
    if (tracks.empty() || !finite(time_seconds)) {
        throw std::invalid_argument("float animation tracks are invalid");
    }

    std::vector<FloatAnimationCurveSample> samples;
    samples.reserve(tracks.size());
    for (const auto& track : tracks) {
        if (!is_valid_float_animation_track(track)) {
            throw std::invalid_argument("float animation track is invalid");
        }
        samples.push_back(FloatAnimationCurveSample{
            .target = track.target,
            .value = sample_float_keyframes(track.keyframes, time_seconds),
        });
    }

    std::ranges::sort(samples, std::ranges::less{}, &FloatAnimationCurveSample::target);
    return samples;
}

AnimationTransformApplyResult
apply_float_animation_samples_to_transform3d(std::span<const FloatAnimationCurveSample> samples,
                                             std::span<const AnimationTransformCurveBinding> bindings,
                                             std::span<Transform3D> transforms) {
    AnimationTransformApplyResult out;
    if (samples.empty()) {
        out.diagnostic = "animation transform samples are empty";
        return out;
    }
    if (bindings.empty()) {
        out.diagnostic = "animation transform bindings are empty";
        return out;
    }
    if (transforms.empty()) {
        out.diagnostic = "animation transform rows are empty";
        return out;
    }

    for (std::size_t index = 0; index < samples.size(); ++index) {
        const auto& sample = samples[index];
        if (!is_safe_target(sample.target)) {
            out.diagnostic = "animation transform sample target is invalid";
            return out;
        }
        if (!finite(sample.value)) {
            out.diagnostic = "animation transform sample value must be finite";
            return out;
        }
        for (std::size_t other = index + 1U; other < samples.size(); ++other) {
            if (samples[other].target == sample.target) {
                out.diagnostic = "animation transform duplicate sample target: " + sample.target;
                return out;
            }
        }
    }

    for (std::size_t index = 0; index < bindings.size(); ++index) {
        const auto& binding = bindings[index];
        if (!is_safe_target(binding.target)) {
            out.diagnostic = "animation transform binding target is invalid";
            return out;
        }
        if (!is_valid_transform_component(binding.component)) {
            out.diagnostic = "animation transform binding component is invalid";
            return out;
        }
        if (binding.transform_index >= transforms.size()) {
            out.diagnostic = "animation transform binding transform index is out of range";
            return out;
        }
        for (std::size_t other = index + 1U; other < bindings.size(); ++other) {
            if (bindings[other].target == binding.target) {
                out.diagnostic = "animation transform duplicate binding target: " + binding.target;
                return out;
            }
        }
        const auto* sample = find_sample(samples, binding.target);
        if (sample == nullptr) {
            out.diagnostic = "animation transform missing sample for bound target: " + binding.target;
            return out;
        }
        if (is_scale_component(binding.component) && sample->value <= 0.0F) {
            out.diagnostic = "animation transform scale samples must be positive";
            return out;
        }
    }

    for (const auto& binding : bindings) {
        const auto* sample = find_sample(samples, binding.target);
        apply_transform_sample(transforms[binding.transform_index], binding.component, sample->value);
        ++out.applied_sample_count;
    }
    out.succeeded = true;
    return out;
}

Vec3 sample_vec3_keyframes(const std::vector<Vec3Keyframe>& keyframes, float time_seconds) {
    if (!is_valid_vec3_keyframes(keyframes) || !finite(time_seconds)) {
        throw std::invalid_argument("vec3 animation keyframes are invalid");
    }
    if (time_seconds <= keyframes.front().time_seconds) {
        return keyframes.front().value;
    }
    if (time_seconds >= keyframes.back().time_seconds) {
        return keyframes.back().value;
    }

    for (std::size_t index = 1; index < keyframes.size(); ++index) {
        if (time_seconds <= keyframes[index].time_seconds) {
            const auto& previous = keyframes[index - 1U];
            const auto& next = keyframes[index];
            const auto duration = next.time_seconds - previous.time_seconds;
            return lerp(previous.value, next.value, (time_seconds - previous.time_seconds) / duration);
        }
    }
    return keyframes.back().value;
}

Quat sample_quat_keyframes(const std::vector<QuatKeyframe>& keyframes, float time_seconds) {
    if (!is_valid_quat_keyframes(keyframes) || !finite(time_seconds)) {
        throw std::invalid_argument("quat animation keyframes are invalid");
    }
    if (time_seconds <= keyframes.front().time_seconds) {
        return keyframes.front().value;
    }
    if (time_seconds >= keyframes.back().time_seconds) {
        return keyframes.back().value;
    }

    for (std::size_t index = 1; index < keyframes.size(); ++index) {
        if (time_seconds <= keyframes[index].time_seconds) {
            const auto& previous = keyframes[index - 1U];
            const auto& next = keyframes[index];
            const auto duration = next.time_seconds - previous.time_seconds;
            return slerp_shortest_path(previous.value, next.value, (time_seconds - previous.time_seconds) / duration);
        }
    }
    return keyframes.back().value;
}

} // namespace mirakana
