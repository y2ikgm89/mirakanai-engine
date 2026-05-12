// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/mat4.hpp"
#include "mirakana/math/vec.hpp"

namespace mirakana {

struct Transform2D {
    Vec2 position{.x = 0.0F, .y = 0.0F};
    Vec2 scale{.x = 1.0F, .y = 1.0F};
    float rotation_radians{0.0F};

    [[nodiscard]] Mat4 matrix() const noexcept {
        return Mat4::translation(Vec3{.x = position.x, .y = position.y, .z = 0.0F}) *
               Mat4::rotation_z(rotation_radians) * Mat4::scale(Vec3{.x = scale.x, .y = scale.y, .z = 1.0F});
    }
};

struct Transform3D {
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 scale{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    Vec3 rotation_radians{.x = 0.0F, .y = 0.0F, .z = 0.0F};

    [[nodiscard]] Mat4 matrix() const noexcept {
        return Mat4::translation(position) * Mat4::rotation_z(rotation_radians.z) *
               Mat4::rotation_y(rotation_radians.y) * Mat4::rotation_x(rotation_radians.x) * Mat4::scale(scale);
    }
};

} // namespace mirakana
