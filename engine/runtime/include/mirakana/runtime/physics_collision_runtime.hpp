// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/asset_runtime.hpp"

namespace mirakana::runtime {

[[nodiscard]] PhysicsAuthoredCollisionScene3DBuildResult
build_physics_world_3d_from_runtime_collision_scene(const RuntimePhysicsCollisionScene3DPayload& payload);

} // namespace mirakana::runtime
