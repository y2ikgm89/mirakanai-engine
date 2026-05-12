// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"

#include <cmath>

namespace mirakana {

struct Quat {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
    float w{1.0F};

    [[nodiscard]] static constexpr Quat identity() noexcept {
        return Quat{};
    }

    [[nodiscard]] static Quat from_axis_angle(Vec3 axis, float radians) noexcept;
};

[[nodiscard]] inline bool is_finite_quat(Quat value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && std::isfinite(value.w);
}

[[nodiscard]] inline float length_squared(Quat value) noexcept {
    return (value.x * value.x) + (value.y * value.y) + (value.z * value.z) + (value.w * value.w);
}

[[nodiscard]] inline float length(Quat value) noexcept {
    return std::sqrt(length_squared(value));
}

[[nodiscard]] inline bool is_normalized_quat(Quat value) noexcept {
    if (!is_finite_quat(value)) {
        return false;
    }
    return std::abs(length_squared(value) - 1.0F) <= 0.0001F;
}

[[nodiscard]] inline Quat normalize_quat(Quat value) noexcept {
    if (!is_finite_quat(value)) {
        return Quat::identity();
    }
    const auto magnitude = length(value);
    if (!std::isfinite(magnitude) || magnitude <= 0.000001F) {
        return Quat::identity();
    }
    return Quat{
        .x = value.x / magnitude,
        .y = value.y / magnitude,
        .z = value.z / magnitude,
        .w = value.w / magnitude,
    };
}

[[nodiscard]] inline Quat conjugate(Quat value) noexcept {
    return Quat{.x = -value.x, .y = -value.y, .z = -value.z, .w = value.w};
}

[[nodiscard]] inline Quat operator*(Quat lhs, Quat rhs) noexcept {
    return Quat{
        .x = (lhs.w * rhs.x) + (lhs.x * rhs.w) + (lhs.y * rhs.z) - (lhs.z * rhs.y),
        .y = (lhs.w * rhs.y) - (lhs.x * rhs.z) + (lhs.y * rhs.w) + (lhs.z * rhs.x),
        .z = (lhs.w * rhs.z) + (lhs.x * rhs.y) - (lhs.y * rhs.x) + (lhs.z * rhs.w),
        .w = (lhs.w * rhs.w) - (lhs.x * rhs.x) - (lhs.y * rhs.y) - (lhs.z * rhs.z),
    };
}

[[nodiscard]] inline Vec3 rotate(Quat rotation, Vec3 value) noexcept {
    const auto unit = normalize_quat(rotation);
    const auto vector_quat = Quat{.x = value.x, .y = value.y, .z = value.z, .w = 0.0F};
    const auto rotated = unit * vector_quat * conjugate(unit);
    return Vec3{.x = rotated.x, .y = rotated.y, .z = rotated.z};
}

[[nodiscard]] inline Quat Quat::from_axis_angle(Vec3 axis, float radians) noexcept {
    if (!std::isfinite(radians) || !std::isfinite(axis.x) || !std::isfinite(axis.y) || !std::isfinite(axis.z)) {
        return Quat::identity();
    }
    const auto axis_length = mirakana::length(axis);
    if (!std::isfinite(axis_length) || axis_length <= 0.000001F) {
        return Quat::identity();
    }
    const auto unit_axis = axis * (1.0F / axis_length);
    const auto half_angle = radians * 0.5F;
    const auto sine = std::sin(half_angle);
    return normalize_quat(Quat{
        .x = unit_axis.x * sine,
        .y = unit_axis.y * sine,
        .z = unit_axis.z * sine,
        .w = std::cos(half_angle),
    });
}

} // namespace mirakana
