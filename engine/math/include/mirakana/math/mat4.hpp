// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/quat.hpp"
#include "mirakana/math/vec.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace mirakana {

class Mat4 {
  public:
    constexpr Mat4() noexcept = default;

    [[nodiscard]] static constexpr Mat4 identity() noexcept {
        Mat4 matrix;
        matrix.at(0, 0) = 1.0F;
        matrix.at(1, 1) = 1.0F;
        matrix.at(2, 2) = 1.0F;
        matrix.at(3, 3) = 1.0F;
        return matrix;
    }

    [[nodiscard]] static constexpr Mat4 translation(Vec3 value) noexcept {
        Mat4 matrix = identity();
        matrix.at(0, 3) = value.x;
        matrix.at(1, 3) = value.y;
        matrix.at(2, 3) = value.z;
        return matrix;
    }

    [[nodiscard]] static constexpr Mat4 scale(Vec3 value) noexcept {
        Mat4 matrix = identity();
        matrix.at(0, 0) = value.x;
        matrix.at(1, 1) = value.y;
        matrix.at(2, 2) = value.z;
        return matrix;
    }

    [[nodiscard]] static Mat4 rotation_x(float radians) noexcept {
        const auto cosine = std::cos(radians);
        const auto sine = std::sin(radians);

        Mat4 matrix = identity();
        matrix.at(1, 1) = cosine;
        matrix.at(1, 2) = -sine;
        matrix.at(2, 1) = sine;
        matrix.at(2, 2) = cosine;
        return matrix;
    }

    [[nodiscard]] static Mat4 rotation_y(float radians) noexcept {
        const auto cosine = std::cos(radians);
        const auto sine = std::sin(radians);

        Mat4 matrix = identity();
        matrix.at(0, 0) = cosine;
        matrix.at(0, 2) = sine;
        matrix.at(2, 0) = -sine;
        matrix.at(2, 2) = cosine;
        return matrix;
    }

    [[nodiscard]] static Mat4 rotation_z(float radians) noexcept {
        const auto cosine = std::cos(radians);
        const auto sine = std::sin(radians);

        Mat4 matrix = identity();
        matrix.at(0, 0) = cosine;
        matrix.at(0, 1) = -sine;
        matrix.at(1, 0) = sine;
        matrix.at(1, 1) = cosine;
        return matrix;
    }

    [[nodiscard]] static Mat4 rotation_quat(Quat rotation) noexcept {
        const auto unit = normalize_quat(rotation);
        const auto xx = unit.x * unit.x;
        const auto yy = unit.y * unit.y;
        const auto zz = unit.z * unit.z;
        const auto xy = unit.x * unit.y;
        const auto xz = unit.x * unit.z;
        const auto yz = unit.y * unit.z;
        const auto xw = unit.x * unit.w;
        const auto yw = unit.y * unit.w;
        const auto zw = unit.z * unit.w;

        Mat4 matrix = identity();
        matrix.at(0, 0) = 1.0F - (2.0F * (yy + zz));
        matrix.at(0, 1) = 2.0F * (xy - zw);
        matrix.at(0, 2) = 2.0F * (xz + yw);
        matrix.at(1, 0) = 2.0F * (xy + zw);
        matrix.at(1, 1) = 1.0F - (2.0F * (xx + zz));
        matrix.at(1, 2) = 2.0F * (yz - xw);
        matrix.at(2, 0) = 2.0F * (xz - yw);
        matrix.at(2, 1) = 2.0F * (yz + xw);
        matrix.at(2, 2) = 1.0F - (2.0F * (xx + yy));
        return matrix;
    }

    [[nodiscard]] constexpr float& at(std::size_t row, std::size_t column) noexcept {
        return values_[(row * 4) + column];
    }

    [[nodiscard]] constexpr float at(std::size_t row, std::size_t column) const noexcept {
        return values_[(row * 4) + column];
    }

    [[nodiscard]] constexpr const std::array<float, 16>& values() const noexcept {
        return values_;
    }

  private:
    std::array<float, 16> values_{};
};

[[nodiscard]] constexpr Mat4 operator*(const Mat4& lhs, const Mat4& rhs) noexcept {
    Mat4 result;
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            float value = 0.0F;
            for (std::size_t i = 0; i < 4; ++i) {
                value += lhs.at(row, i) * rhs.at(i, column);
            }
            result.at(row, column) = value;
        }
    }
    return result;
}

[[nodiscard]] constexpr Vec3 transform_point(const Mat4& matrix, Vec3 point) noexcept {
    return Vec3{
        .x = (matrix.at(0, 0) * point.x) + (matrix.at(0, 1) * point.y) + (matrix.at(0, 2) * point.z) + matrix.at(0, 3),
        .y = (matrix.at(1, 0) * point.x) + (matrix.at(1, 1) * point.y) + (matrix.at(1, 2) * point.z) + matrix.at(1, 3),
        .z = (matrix.at(2, 0) * point.x) + (matrix.at(2, 1) * point.y) + (matrix.at(2, 2) * point.z) + matrix.at(2, 3)};
}

[[nodiscard]] constexpr Vec3 transform_direction(const Mat4& matrix, Vec3 direction) noexcept {
    return Vec3{
        .x = (matrix.at(0, 0) * direction.x) + (matrix.at(0, 1) * direction.y) + (matrix.at(0, 2) * direction.z),
        .y = (matrix.at(1, 0) * direction.x) + (matrix.at(1, 1) * direction.y) + (matrix.at(1, 2) * direction.z),
        .z = (matrix.at(2, 0) * direction.x) + (matrix.at(2, 1) * direction.y) + (matrix.at(2, 2) * direction.z)};
}

} // namespace mirakana
