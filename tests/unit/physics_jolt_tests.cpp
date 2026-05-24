// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/physics/jolt/jolt_physics_adapter.hpp"

#include <string>

namespace {

[[nodiscard]] mirakana::PhysicsAuthoredCollisionScene3DDesc make_jolt_scene() {
    mirakana::PhysicsAuthoredCollisionScene3DDesc scene;
    scene.require_native_backend = true;
    scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
        .name = "floor",
        .body =
            mirakana::PhysicsBody3DDesc{
                .position = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                .mass = 0.0F,
                .dynamic = false,
                .half_extents = mirakana::Vec3{.x = 10.0F, .y = 0.5F, .z = 10.0F},
                .collision_layer = 1U,
                .collision_mask = 0xFFFF'FFFFU,
            },
    });
    scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
        .name = "falling_sphere",
        .body =
            mirakana::PhysicsBody3DDesc{
                .position = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F},
                .velocity = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                .shape = mirakana::PhysicsShape3DKind::sphere,
                .radius = 0.5F,
                .collision_layer = 2U,
                .collision_mask = 0xFFFF'FFFFU,
            },
    });
    return scene;
}

[[nodiscard]] mirakana::PhysicsAuthoredCollisionScene3DDesc make_pass_through_scene() {
    mirakana::PhysicsAuthoredCollisionScene3DDesc scene;
    scene.require_native_backend = true;
    scene.world_config.gravity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
        .name = "floor",
        .body =
            mirakana::PhysicsBody3DDesc{
                .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                .mass = 0.0F,
                .dynamic = false,
                .half_extents = mirakana::Vec3{.x = 10.0F, .y = 0.25F, .z = 10.0F},
                .collision_layer = 1U,
                .collision_mask = 0xFFFF'FFFFU,
            },
    });
    scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
        .name = "falling_sphere",
        .body =
            mirakana::PhysicsBody3DDesc{
                .position = mirakana::Vec3{.x = 0.0F, .y = 0.51F, .z = 0.0F},
                .velocity = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                .shape = mirakana::PhysicsShape3DKind::sphere,
                .radius = 0.25F,
                .collision_layer = 2U,
                .collision_mask = 0xFFFF'FFFFU,
            },
    });
    return scene;
}

[[nodiscard]] mirakana::PhysicsAuthoredCollisionScene3DDesc make_single_damped_scene() {
    mirakana::PhysicsAuthoredCollisionScene3DDesc scene;
    scene.require_native_backend = true;
    scene.world_config.gravity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
        .name = "damped_sphere",
        .body =
            mirakana::PhysicsBody3DDesc{
                .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                .velocity = mirakana::Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F},
                .linear_damping = 0.5F,
                .shape = mirakana::PhysicsShape3DKind::sphere,
                .radius = 0.5F,
                .collision_layer = 1U,
                .collision_mask = 0xFFFF'FFFFU,
            },
    });
    return scene;
}

[[nodiscard]] mirakana::PhysicsAuthoredCollisionScene3DDesc make_dense_contact_scene() {
    mirakana::PhysicsAuthoredCollisionScene3DDesc scene;
    scene.require_native_backend = true;
    scene.world_config.gravity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    for (std::uint32_t index = 0U; index < 48U; ++index) {
        scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
            .name = "dense_sphere_" + std::to_string(index),
            .body =
                mirakana::PhysicsBody3DDesc{
                    .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                    .shape = mirakana::PhysicsShape3DKind::sphere,
                    .radius = 1.0F,
                    .collision_layer = 1U,
                    .collision_mask = 0xFFFF'FFFFU,
                },
        });
    }
    return scene;
}

} // namespace

MK_TEST("jolt physics adapter reports first party capabilities without native handles") {
    const auto capabilities = mirakana::jolt_physics_3d_adapter_capabilities();

    MK_REQUIRE(capabilities.available);
    MK_REQUIRE(capabilities.adapter_id == "joltphysics");
    MK_REQUIRE(capabilities.supports_authored_collision_scene);
    MK_REQUIRE(capabilities.supports_step_simulation);
    MK_REQUIRE(capabilities.supports_collision_filters);
    MK_REQUIRE(!capabilities.supports_triggers);
    MK_REQUIRE(!capabilities.cross_platform_determinism);
    MK_REQUIRE(!capabilities.exposes_native_handles);
    MK_REQUIRE(!capabilities.supports_disabled_collision_bodies);
    MK_REQUIRE(capabilities.supports_default_collision_mask_wildcard);
    MK_REQUIRE(capabilities.supported_collision_layer_bits != 0U);
    MK_REQUIRE(capabilities.supported_collision_mask_bits != 0U);
    MK_REQUIRE(capabilities.supported_collision_layer_bits < 0xFFFF'FFFFU);
    MK_REQUIRE(capabilities.supported_collision_mask_bits < 0xFFFF'FFFFU);
    MK_REQUIRE(capabilities.max_collision_steps == mirakana::physics_native_3d_max_collision_steps);
    MK_REQUIRE(capabilities.max_worker_threads == mirakana::physics_native_3d_max_worker_threads);
    MK_REQUIRE(capabilities.min_backend_bodies == 2U);
    MK_REQUIRE(capabilities.max_backend_bodies >= capabilities.min_backend_bodies);
    MK_REQUIRE(capabilities.max_temp_allocator_bytes == mirakana::physics_native_3d_max_temp_allocator_bytes);
}

MK_TEST("jolt physics adapter advances an authored first party scene") {
    auto adapter = mirakana::make_jolt_physics_3d_adapter();
    const auto scene = make_jolt_scene();

    const auto result =
        mirakana::simulate_native_physics_3d(adapter.get(), mirakana::PhysicsNative3DSimulationRequest{
                                                                .scene = &scene,
                                                                .delta_seconds = 1.0F / 30.0F,
                                                                .step =
                                                                    mirakana::PhysicsNative3DStepConfig{
                                                                        .collision_steps = 2U,
                                                                        .max_collision_steps = 4U,
                                                                        .worker_threads = 1U,
                                                                        .temp_allocator_bytes = 1024U * 1024U,
                                                                    },
                                                                .max_bodies = 16U,
                                                            });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::completed);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::none);
    MK_REQUIRE(result.dispatched);
    MK_REQUIRE(result.steps_executed == 2U);
    MK_REQUIRE(result.backend_body_count == 2U);
    MK_REQUIRE(result.bodies.size() == 2U);
    MK_REQUIRE(result.world.bodies().size() == 2U);
    MK_REQUIRE(result.world.bodies()[0].position.y == scene.bodies[0].body.position.y);
    MK_REQUIRE(result.world.bodies()[1].position.y < scene.bodies[1].body.position.y);
}

MK_TEST("jolt physics adapter rejects filters outside jolt object layer bits") {
    auto adapter = mirakana::make_jolt_physics_3d_adapter();
    auto scene = make_jolt_scene();
    scene.bodies[1].body.collision_layer = 0xFFFF'0000U;

    const auto result = mirakana::simulate_native_physics_3d(adapter.get(), mirakana::PhysicsNative3DSimulationRequest{
                                                                                .scene = &scene,
                                                                                .max_bodies = 16U,
                                                                            });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::collision_filters_unsupported);
    MK_REQUIRE(!result.dispatched);
}

MK_TEST("jolt physics adapter fails closed for one native dynamic body") {
    auto adapter = mirakana::make_jolt_physics_3d_adapter();
    const auto scene = make_single_damped_scene();

    const auto result =
        mirakana::simulate_native_physics_3d(adapter.get(), mirakana::PhysicsNative3DSimulationRequest{
                                                                .scene = &scene,
                                                                .delta_seconds = 0.1F,
                                                                .step =
                                                                    mirakana::PhysicsNative3DStepConfig{
                                                                        .collision_steps = 4U,
                                                                        .max_collision_steps = 4U,
                                                                        .worker_threads = 1U,
                                                                        .temp_allocator_bytes = 1024U * 1024U,
                                                                    },
                                                                .max_bodies = 4U,
                                                            });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::insufficient_backend_bodies);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(result.backend_body_count == 0U);
    MK_REQUIRE(result.world.bodies().empty());
}

MK_TEST("jolt physics adapter sizes dense scene capacity from enabled bodies") {
    auto adapter = mirakana::make_jolt_physics_3d_adapter();
    const auto scene = make_dense_contact_scene();

    const auto result =
        mirakana::simulate_native_physics_3d(adapter.get(), mirakana::PhysicsNative3DSimulationRequest{
                                                                .scene = &scene,
                                                                .delta_seconds = 1.0F / 120.0F,
                                                                .step =
                                                                    mirakana::PhysicsNative3DStepConfig{
                                                                        .collision_steps = 1U,
                                                                        .max_collision_steps = 1U,
                                                                        .worker_threads = 1U,
                                                                        .temp_allocator_bytes = 4U * 1024U * 1024U,
                                                                    },
                                                                .max_bodies = 64U,
                                                            });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::completed);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::none);
    MK_REQUIRE(result.backend_body_count == scene.bodies.size());
}

MK_TEST("jolt physics adapter fails closed for collision disabled bodies") {
    auto adapter = mirakana::make_jolt_physics_3d_adapter();
    auto scene = make_pass_through_scene();
    scene.bodies[0].body.collision_enabled = false;

    const auto result =
        mirakana::simulate_native_physics_3d(adapter.get(), mirakana::PhysicsNative3DSimulationRequest{
                                                                .scene = &scene,
                                                                .delta_seconds = 0.1F,
                                                                .step =
                                                                    mirakana::PhysicsNative3DStepConfig{
                                                                        .collision_steps = 4U,
                                                                        .max_collision_steps = 4U,
                                                                        .worker_threads = 1U,
                                                                        .temp_allocator_bytes = 1024U * 1024U,
                                                                    },
                                                                .max_bodies = 16U,
                                                            });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::collision_filters_unsupported);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(result.backend_body_count == 0U);
    MK_REQUIRE(result.world.bodies().empty());
}

MK_TEST("jolt physics adapter rejects trigger bodies before dispatch") {
    auto adapter = mirakana::make_jolt_physics_3d_adapter();
    auto scene = make_pass_through_scene();
    scene.bodies[0].body.trigger = true;

    const auto result =
        mirakana::simulate_native_physics_3d(adapter.get(), mirakana::PhysicsNative3DSimulationRequest{
                                                                .scene = &scene,
                                                                .delta_seconds = 0.1F,
                                                                .step =
                                                                    mirakana::PhysicsNative3DStepConfig{
                                                                        .collision_steps = 4U,
                                                                        .max_collision_steps = 4U,
                                                                        .worker_threads = 1U,
                                                                        .temp_allocator_bytes = 1024U * 1024U,
                                                                    },
                                                                .max_bodies = 16U,
                                                            });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::triggers_unsupported);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(result.world.bodies().empty());
}

int main() {
    return mirakana::test::run_all();
}
