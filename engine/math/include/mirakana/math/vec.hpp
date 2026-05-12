// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cmath>

namespace mirakana {

struct Vec2 {
    float x{0.0F};
    float y{0.0F};

    friend constexpr Vec2 operator+(Vec2 lhs, Vec2 rhs) noexcept {
        return Vec2{.x = lhs.x + rhs.x, .y = lhs.y + rhs.y};
    }

    friend constexpr Vec2 operator-(Vec2 lhs, Vec2 rhs) noexcept {
        return Vec2{.x = lhs.x - rhs.x, .y = lhs.y - rhs.y};
    }

    friend constexpr Vec2 operator*(Vec2 value, float scale) noexcept {
        return Vec2{.x = value.x * scale, .y = value.y * scale};
    }

    friend constexpr bool operator==(Vec2 lhs, Vec2 rhs) noexcept {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
};

struct Vec3 {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};

    friend constexpr Vec3 operator+(Vec3 lhs, Vec3 rhs) noexcept {
        return Vec3{.x = lhs.x + rhs.x, .y = lhs.y + rhs.y, .z = lhs.z + rhs.z};
    }

    friend constexpr Vec3 operator-(Vec3 lhs, Vec3 rhs) noexcept {
        return Vec3{.x = lhs.x - rhs.x, .y = lhs.y - rhs.y, .z = lhs.z - rhs.z};
    }

    friend constexpr Vec3 operator*(Vec3 value, float scale) noexcept {
        return Vec3{.x = value.x * scale, .y = value.y * scale, .z = value.z * scale};
    }

    friend constexpr bool operator==(Vec3 lhs, Vec3 rhs) noexcept {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }
};

[[nodiscard]] inline float dot(Vec2 lhs, Vec2 rhs) noexcept {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

[[nodiscard]] inline float dot(Vec3 lhs, Vec3 rhs) noexcept {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

[[nodiscard]] inline Vec3 cross(Vec3 lhs, Vec3 rhs) noexcept {
    return Vec3{.x = (lhs.y * rhs.z) - (lhs.z * rhs.y),
                .y = (lhs.z * rhs.x) - (lhs.x * rhs.z),
                .z = (lhs.x * rhs.y) - (lhs.y * rhs.x)};
}

[[nodiscard]] inline float length(Vec2 value) noexcept {
    return std::sqrt(dot(value, value));
}

[[nodiscard]] inline float length(Vec3 value) noexcept {
    return std::sqrt(dot(value, value));
}

} // namespace mirakana
