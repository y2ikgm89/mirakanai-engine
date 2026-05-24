// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/physics/native_adapter.hpp"

#include <cstdint>
#include <utility>

namespace {

class FakeNativeAdapter final : public mirakana::IPhysicsNative3DAdapter {
  public:
    explicit FakeNativeAdapter(mirakana::PhysicsNative3DAdapterCapabilities capabilities)
        : capabilities_(std::move(capabilities)) {}

    [[nodiscard]] mirakana::PhysicsNative3DAdapterCapabilities capabilities() const override {
        return capabilities_;
    }

    [[nodiscard]] mirakana::PhysicsNative3DSimulationResult
    simulate(const mirakana::PhysicsNative3DSimulationRequest& request) override {
        ++dispatches_;
        mirakana::PhysicsNative3DSimulationResult result;
        result.status = mirakana::PhysicsNative3DAdapterStatus::completed;
        result.capabilities = capabilities_;
        result.dispatched = true;
        result.steps_executed = request.step.collision_steps;
        result.backend_body_count =
            request.scene == nullptr ? 0U : static_cast<std::uint64_t>(request.scene->bodies.size());
        if (request.scene != nullptr) {
            result.world = mirakana::PhysicsWorld3D{request.scene->world_config};
            for (std::size_t index = 0; index < request.scene->bodies.size(); ++index) {
                const auto body = result.world.create_body(request.scene->bodies[index].body);
                result.bodies.push_back(mirakana::PhysicsNative3DBodyRow{
                    .source_index = index,
                    .body = body,
                });
            }
        }
        return result;
    }

    [[nodiscard]] int dispatches() const noexcept {
        return dispatches_;
    }

  private:
    mirakana::PhysicsNative3DAdapterCapabilities capabilities_;
    int dispatches_{0};
};

[[nodiscard]] mirakana::PhysicsNative3DAdapterCapabilities ready_capabilities() {
    return mirakana::PhysicsNative3DAdapterCapabilities{
        .adapter_id = "fake-native-physics",
        .available = true,
        .supports_authored_collision_scene = true,
        .supports_step_simulation = true,
        .supports_collision_filters = true,
        .supports_triggers = true,
        .cross_platform_determinism = false,
        .exposes_native_handles = false,
        .supports_disabled_collision_bodies = true,
        .supports_default_collision_mask_wildcard = true,
        .supported_collision_layer_bits = 0xFFFF'FFFFU,
        .supported_collision_mask_bits = 0xFFFF'FFFFU,
        .max_collision_steps = mirakana::physics_native_3d_max_collision_steps,
        .max_worker_threads = mirakana::physics_native_3d_max_worker_threads,
        .min_backend_bodies = 0U,
        .max_backend_bodies = 1024U,
        .max_temp_allocator_bytes = mirakana::physics_native_3d_max_temp_allocator_bytes,
    };
}

[[nodiscard]] mirakana::PhysicsAuthoredCollisionScene3DDesc make_scene() {
    mirakana::PhysicsAuthoredCollisionScene3DDesc scene;
    scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
        .name = "floor",
        .body =
            mirakana::PhysicsBody3DDesc{
                .position = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                .mass = 0.0F,
                .dynamic = false,
                .half_extents = mirakana::Vec3{.x = 4.0F, .y = 0.5F, .z = 4.0F},
            },
    });
    scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
        .name = "sphere",
        .body =
            mirakana::PhysicsBody3DDesc{
                .position = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F},
                .velocity = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                .shape = mirakana::PhysicsShape3DKind::sphere,
                .radius = 0.5F,
            },
    });
    return scene;
}

} // namespace

MK_TEST("native physics adapter rejects missing adapter before dispatch") {
    const auto scene = make_scene();
    const auto result = mirakana::simulate_native_physics_3d(nullptr, mirakana::PhysicsNative3DSimulationRequest{
                                                                          .scene = &scene,
                                                                      });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unavailable);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::missing_adapter);
    MK_REQUIRE(!result.dispatched);
}

MK_TEST("native physics adapter rejects native handle exposure") {
    auto capabilities = ready_capabilities();
    capabilities.exposes_native_handles = true;
    FakeNativeAdapter adapter{capabilities};
    const auto scene = make_scene();

    const auto result = mirakana::simulate_native_physics_3d(&adapter, mirakana::PhysicsNative3DSimulationRequest{
                                                                           .scene = &scene,
                                                                       });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::native_handles_exposed);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(adapter.dispatches() == 0);
}

MK_TEST("native physics adapter rejects unavailable determinism") {
    FakeNativeAdapter adapter{ready_capabilities()};
    const auto scene = make_scene();

    const auto result = mirakana::simulate_native_physics_3d(&adapter, mirakana::PhysicsNative3DSimulationRequest{
                                                                           .scene = &scene,
                                                                           .require_cross_platform_determinism = true,
                                                                       });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::cross_platform_determinism_unavailable);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(adapter.dispatches() == 0);
}

MK_TEST("native physics adapter rejects collision filters when unsupported") {
    auto capabilities = ready_capabilities();
    capabilities.supports_collision_filters = false;
    FakeNativeAdapter adapter{capabilities};
    auto scene = make_scene();
    scene.bodies[1].body.collision_layer = 2U;
    scene.bodies[1].body.collision_mask = 2U;

    const auto result = mirakana::simulate_native_physics_3d(&adapter, mirakana::PhysicsNative3DSimulationRequest{
                                                                           .scene = &scene,
                                                                       });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::collision_filters_unsupported);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(adapter.dispatches() == 0);
}

MK_TEST("native physics adapter rejects trigger bodies when unsupported") {
    auto capabilities = ready_capabilities();
    capabilities.supports_triggers = false;
    FakeNativeAdapter adapter{capabilities};
    auto scene = make_scene();
    scene.bodies[0].body.trigger = true;

    const auto result = mirakana::simulate_native_physics_3d(&adapter, mirakana::PhysicsNative3DSimulationRequest{
                                                                           .scene = &scene,
                                                                       });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::triggers_unsupported);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(adapter.dispatches() == 0);
}

MK_TEST("native physics adapter rejects step config above deterministic limits") {
    FakeNativeAdapter adapter{ready_capabilities()};
    const auto scene = make_scene();

    auto worker_request = mirakana::PhysicsNative3DSimulationRequest{
        .scene = &scene,
        .step =
            mirakana::PhysicsNative3DStepConfig{
                .collision_steps = 1U,
                .max_collision_steps = 1U,
                .worker_threads = mirakana::physics_native_3d_max_worker_threads + 1U,
                .temp_allocator_bytes = 1024U * 1024U,
            },
    };
    auto result = mirakana::simulate_native_physics_3d(&adapter, worker_request);
    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::invalid_request);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::invalid_step_config);
    MK_REQUIRE(!result.dispatched);

    auto steps_request = mirakana::PhysicsNative3DSimulationRequest{
        .scene = &scene,
        .step =
            mirakana::PhysicsNative3DStepConfig{
                .collision_steps = mirakana::physics_native_3d_max_collision_steps + 1U,
                .max_collision_steps = mirakana::physics_native_3d_max_collision_steps + 1U,
                .worker_threads = 1U,
                .temp_allocator_bytes = 1024U * 1024U,
            },
    };
    result = mirakana::simulate_native_physics_3d(&adapter, steps_request);
    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::invalid_request);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::invalid_step_config);
    MK_REQUIRE(!result.dispatched);

    auto allocator_request = mirakana::PhysicsNative3DSimulationRequest{
        .scene = &scene,
        .step =
            mirakana::PhysicsNative3DStepConfig{
                .collision_steps = 1U,
                .max_collision_steps = 1U,
                .worker_threads = 1U,
                .temp_allocator_bytes = mirakana::physics_native_3d_max_temp_allocator_bytes + 1U,
            },
    };
    result = mirakana::simulate_native_physics_3d(&adapter, allocator_request);
    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::invalid_request);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::invalid_step_config);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(adapter.dispatches() == 0);
}

MK_TEST("native physics adapter preflights advertised backend scene limits") {
    auto capabilities = ready_capabilities();
    capabilities.min_backend_bodies = 3U;
    FakeNativeAdapter adapter{capabilities};
    const auto scene = make_scene();

    auto result = mirakana::simulate_native_physics_3d(&adapter, mirakana::PhysicsNative3DSimulationRequest{
                                                                     .scene = &scene,
                                                                 });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::insufficient_backend_bodies);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(adapter.dispatches() == 0);
}

MK_TEST("native physics adapter preflights advertised collision bit limits") {
    auto capabilities = ready_capabilities();
    capabilities.supported_collision_layer_bits = 0xFFU;
    capabilities.supported_collision_mask_bits = 0xFFU;
    FakeNativeAdapter adapter{capabilities};
    auto scene = make_scene();
    scene.bodies[1].body.collision_layer = 0x100U;

    auto result = mirakana::simulate_native_physics_3d(&adapter, mirakana::PhysicsNative3DSimulationRequest{
                                                                     .scene = &scene,
                                                                 });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::collision_filters_unsupported);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(adapter.dispatches() == 0);

    capabilities = ready_capabilities();
    capabilities.supports_disabled_collision_bodies = false;
    FakeNativeAdapter no_disabled_adapter{capabilities};
    scene = make_scene();
    scene.bodies[0].body.collision_enabled = false;

    result = mirakana::simulate_native_physics_3d(&no_disabled_adapter, mirakana::PhysicsNative3DSimulationRequest{
                                                                            .scene = &scene,
                                                                        });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::unsupported_feature);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::collision_filters_unsupported);
    MK_REQUIRE(!result.dispatched);
    MK_REQUIRE(no_disabled_adapter.dispatches() == 0);
}

MK_TEST("native physics adapter dispatches first party scene rows") {
    FakeNativeAdapter adapter{ready_capabilities()};
    const auto scene = make_scene();

    const auto result = mirakana::simulate_native_physics_3d(&adapter, mirakana::PhysicsNative3DSimulationRequest{
                                                                           .scene = &scene,
                                                                       });

    MK_REQUIRE(result.status == mirakana::PhysicsNative3DAdapterStatus::completed);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsNative3DAdapterDiagnostic::none);
    MK_REQUIRE(result.dispatched);
    MK_REQUIRE(result.steps_executed == 1U);
    MK_REQUIRE(result.backend_body_count == 2U);
    MK_REQUIRE(result.bodies.size() == 2U);
    MK_REQUIRE(result.world.bodies().size() == 2U);
    MK_REQUIRE(adapter.dispatches() == 1);
}

int main() {
    return mirakana::test::run_all();
}
