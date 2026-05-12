// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/physics_collision_runtime.hpp"

#include <utility>

namespace mirakana::runtime {

PhysicsAuthoredCollisionScene3DBuildResult
build_physics_world_3d_from_runtime_collision_scene(const RuntimePhysicsCollisionScene3DPayload& payload) {
    PhysicsAuthoredCollisionScene3DDesc desc;
    desc.world_config = payload.world_config;
    desc.bodies.reserve(payload.bodies.size());
    for (const auto& row : payload.bodies) {
        desc.bodies.push_back(PhysicsAuthoredCollisionBody3DDesc{.name = row.name, .body = row.body});
    }
    return build_physics_world_3d_from_authored_collision_scene(desc);
}

} // namespace mirakana::runtime
