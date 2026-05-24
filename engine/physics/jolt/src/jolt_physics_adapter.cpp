// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/physics/jolt/jolt_physics_adapter.hpp"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceMask.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterMask.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterMask.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/EPhysicsUpdateError.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace mirakana {
namespace {

namespace broad_phase_layers {
inline constexpr JPH::BroadPhaseLayer all_bodies{0U};
inline constexpr JPH::uint count{1U};
} // namespace broad_phase_layers

struct JoltRuntime {
    JoltRuntime() {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
    }

    ~JoltRuntime() {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    JoltRuntime(const JoltRuntime&) = delete;
    JoltRuntime& operator=(const JoltRuntime&) = delete;
};

[[nodiscard]] JoltRuntime& jolt_runtime() {
    static JoltRuntime runtime;
    return runtime;
}

struct JoltBodySlot {
    JPH::BodyID id{};
    bool active{false};
};

[[nodiscard]] JPH::Vec3 to_jolt_vector(Vec3 value) noexcept {
    return JPH::Vec3{value.x, value.y, value.z};
}

[[nodiscard]] JPH::RVec3 to_jolt_position(Vec3 value) noexcept {
    return JPH::RVec3{value.x, value.y, value.z};
}

[[nodiscard]] Vec3 to_first_party_vector(JPH::Vec3 value) noexcept {
    return Vec3{
        .x = value.GetX(),
        .y = value.GetY(),
        .z = value.GetZ(),
    };
}

[[nodiscard]] Vec3 to_first_party_position(JPH::RVec3 value) noexcept {
    return Vec3{
        .x = value.GetX(),
        .y = value.GetY(),
        .z = value.GetZ(),
    };
}

[[nodiscard]] bool fits_jolt_filter_mask(std::uint32_t value) noexcept {
    return (value & ~JPH::ObjectLayerPairFilterMask::cMask) == 0U;
}

[[nodiscard]] std::uint32_t to_jolt_collision_mask(std::uint32_t collision_mask) noexcept {
    return collision_mask == 0xFFFF'FFFFU ? JPH::ObjectLayerPairFilterMask::cMask : collision_mask;
}

[[nodiscard]] bool supports_jolt_filter(const PhysicsBody3DDesc& desc) noexcept {
    return desc.collision_enabled && fits_jolt_filter_mask(desc.collision_layer) &&
           fits_jolt_filter_mask(to_jolt_collision_mask(desc.collision_mask));
}

[[nodiscard]] JPH::ObjectLayer to_jolt_object_layer(const PhysicsBody3DDesc& desc) noexcept {
    return JPH::ObjectLayerPairFilterMask::sGetObjectLayer(desc.collision_layer,
                                                           to_jolt_collision_mask(desc.collision_mask));
}

[[nodiscard]] JPH::ShapeRefC make_shape(const PhysicsBody3DDesc& desc) {
    switch (desc.shape) {
    case PhysicsShape3DKind::aabb:
        return new JPH::BoxShape{to_jolt_vector(desc.half_extents)};
    case PhysicsShape3DKind::sphere:
        return new JPH::SphereShape{desc.radius};
    case PhysicsShape3DKind::capsule:
        return new JPH::CapsuleShape{desc.half_height, desc.radius};
    }
    return new JPH::BoxShape{to_jolt_vector(desc.half_extents)};
}

[[nodiscard]] JPH::BodyCreationSettings make_body_settings(const PhysicsBody3DDesc& desc, JPH::ObjectLayer layer) {
    auto settings = JPH::BodyCreationSettings{
        make_shape(desc),
        to_jolt_position(desc.position),
        JPH::Quat::sIdentity(),
        desc.dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
        layer,
    };
    settings.mLinearVelocity = to_jolt_vector(desc.velocity);
    settings.mLinearDamping = desc.linear_damping;
    settings.mFriction = 0.0F;
    settings.mRestitution = 0.0F;
    settings.mIsSensor = false;
    if (desc.dynamic) {
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = desc.mass;
    }
    return settings;
}

[[nodiscard]] std::uint64_t dense_body_pair_count(std::size_t body_count) noexcept {
    const auto capped_count =
        static_cast<std::uint64_t>(std::min<std::size_t>(body_count, JPH::PhysicsSystem::cMaxBodiesLimit));
    if (capped_count < 2U) {
        return 0U;
    }
    return (capped_count * (capped_count - 1U)) / 2U;
}

[[nodiscard]] std::uint64_t saturating_multiply(std::uint64_t value, std::uint64_t multiplier) noexcept {
    if (multiplier != 0U && value > std::numeric_limits<std::uint64_t>::max() / multiplier) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return value * multiplier;
}

[[nodiscard]] JPH::uint clamp_jolt_capacity(std::uint64_t value, JPH::uint limit) noexcept {
    return static_cast<JPH::uint>(std::min<std::uint64_t>(value, limit));
}

[[nodiscard]] JPH::uint jolt_body_capacity(std::size_t requested_body_capacity, std::size_t scene_body_count) noexcept {
    return clamp_jolt_capacity(std::max<std::size_t>(requested_body_capacity, scene_body_count),
                               JPH::PhysicsSystem::cMaxBodiesLimit);
}

[[nodiscard]] JPH::uint jolt_body_pair_capacity(std::size_t enabled_body_count) noexcept {
    constexpr std::uint64_t minimum_body_pairs = 1024U;
    const auto dense_pairs = dense_body_pair_count(enabled_body_count);
    const auto capacity = std::max(minimum_body_pairs, saturating_multiply(dense_pairs, 8U));
    return clamp_jolt_capacity(capacity, JPH::PhysicsSystem::cMaxBodyPairsLimit);
}

[[nodiscard]] JPH::uint jolt_contact_constraint_capacity(std::size_t enabled_body_count) noexcept {
    constexpr std::uint64_t minimum_contact_constraints = 1024U;
    const auto dense_pairs = dense_body_pair_count(enabled_body_count);
    const auto capacity = std::max(minimum_contact_constraints, saturating_multiply(dense_pairs, 4U));
    return clamp_jolt_capacity(capacity, JPH::PhysicsSystem::cMaxContactConstraintsLimit);
}

[[nodiscard]] PhysicsNative3DSimulationResult make_jolt_error(PhysicsNative3DAdapterDiagnostic diagnostic) {
    PhysicsNative3DSimulationResult result;
    result.status = PhysicsNative3DAdapterStatus::adapter_error;
    result.diagnostic = diagnostic;
    result.capabilities = jolt_physics_3d_adapter_capabilities();
    result.dispatched = true;
    return result;
}

[[nodiscard]] PhysicsNative3DSimulationResult make_jolt_unsupported(PhysicsNative3DAdapterDiagnostic diagnostic) {
    PhysicsNative3DSimulationResult result;
    result.status = PhysicsNative3DAdapterStatus::unsupported_feature;
    result.diagnostic = diagnostic;
    result.capabilities = jolt_physics_3d_adapter_capabilities();
    result.dispatched = true;
    return result;
}

[[nodiscard]] bool has_update_error(JPH::EPhysicsUpdateError error, JPH::EPhysicsUpdateError flag) noexcept {
    return (error & flag) != JPH::EPhysicsUpdateError::None;
}

[[nodiscard]] PhysicsNative3DAdapterDiagnostic map_update_error(JPH::EPhysicsUpdateError error) noexcept {
    if (has_update_error(error, JPH::EPhysicsUpdateError::BodyPairCacheFull)) {
        return PhysicsNative3DAdapterDiagnostic::native_body_pair_capacity_exceeded;
    }
    if (has_update_error(error, JPH::EPhysicsUpdateError::ContactConstraintsFull)) {
        return PhysicsNative3DAdapterDiagnostic::native_contact_constraint_capacity_exceeded;
    }
    if (has_update_error(error, JPH::EPhysicsUpdateError::ManifoldCacheFull)) {
        return PhysicsNative3DAdapterDiagnostic::native_manifold_capacity_exceeded;
    }
    return PhysicsNative3DAdapterDiagnostic::adapter_failure;
}

class JoltPhysics3DAdapter final : public IPhysicsNative3DAdapter {
  public:
    [[nodiscard]] PhysicsNative3DAdapterCapabilities capabilities() const override {
        return jolt_physics_3d_adapter_capabilities();
    }

    [[nodiscard]] PhysicsNative3DSimulationResult simulate(const PhysicsNative3DSimulationRequest& request) override {
        static_cast<void>(jolt_runtime());

        const auto& scene = *request.scene;
        for (const auto& body : scene.bodies) {
            if (body.body.trigger) {
                return make_jolt_unsupported(PhysicsNative3DAdapterDiagnostic::triggers_unsupported);
            }
            if (!supports_jolt_filter(body.body)) {
                PhysicsNative3DSimulationResult result;
                result.status = PhysicsNative3DAdapterStatus::unsupported_feature;
                result.diagnostic = PhysicsNative3DAdapterDiagnostic::collision_filters_unsupported;
                result.capabilities = jolt_physics_3d_adapter_capabilities();
                result.dispatched = true;
                return result;
            }
        }
        const auto enabled_body_count = scene.bodies.size();
        if (enabled_body_count < 2U) {
            return make_jolt_unsupported(PhysicsNative3DAdapterDiagnostic::insufficient_backend_bodies);
        }

        JPH::BroadPhaseLayerInterfaceMask broad_phase_layer_interface{broad_phase_layers::count};
        broad_phase_layer_interface.ConfigureLayer(broad_phase_layers::all_bodies,
                                                   JPH::ObjectLayerPairFilterMask::cMask, 0U);
        JPH::ObjectVsBroadPhaseLayerFilterMask object_vs_broad_phase_layer_filter{broad_phase_layer_interface};
        JPH::ObjectLayerPairFilterMask object_layer_pair_filter;

        JPH::PhysicsSystem physics_system;
        physics_system.Init(jolt_body_capacity(request.max_bodies, scene.bodies.size()), 0U,
                            jolt_body_pair_capacity(enabled_body_count),
                            jolt_contact_constraint_capacity(enabled_body_count), broad_phase_layer_interface,
                            object_vs_broad_phase_layer_filter, object_layer_pair_filter);
        physics_system.SetGravity(to_jolt_vector(scene.world_config.gravity));

        auto& body_interface = physics_system.GetBodyInterface();
        std::vector<JoltBodySlot> body_slots(scene.bodies.size());

        for (std::size_t index = 0; index < scene.bodies.size(); ++index) {
            const auto settings =
                make_body_settings(scene.bodies[index].body, to_jolt_object_layer(scene.bodies[index].body));
            const auto activation =
                scene.bodies[index].body.dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;
            const auto body_id = body_interface.CreateAndAddBody(settings, activation);
            if (body_id.IsInvalid()) {
                for (const auto slot : body_slots) {
                    if (slot.active) {
                        body_interface.RemoveBody(slot.id);
                        body_interface.DestroyBody(slot.id);
                    }
                }
                return make_jolt_error(PhysicsNative3DAdapterDiagnostic::adapter_failure);
            }
            body_slots[index] = JoltBodySlot{
                .id = body_id,
                .active = true,
            };
        }

        physics_system.OptimizeBroadPhase();

        JPH::TempAllocatorImpl temp_allocator{request.step.temp_allocator_bytes};
        std::unique_ptr<JPH::JobSystem> job_system;
        if (request.step.worker_threads == 1U) {
            job_system = std::make_unique<JPH::JobSystemSingleThreaded>(JPH::cMaxPhysicsJobs);
        } else {
            job_system = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                                                                    static_cast<int>(request.step.worker_threads));
        }
        const auto update_error = physics_system.Update(
            request.delta_seconds, static_cast<int>(request.step.collision_steps), &temp_allocator, job_system.get());
        if (update_error != JPH::EPhysicsUpdateError::None) {
            for (const auto slot : body_slots) {
                if (slot.active) {
                    body_interface.RemoveBody(slot.id);
                    body_interface.DestroyBody(slot.id);
                }
            }
            return make_jolt_error(map_update_error(update_error));
        }

        PhysicsNative3DSimulationResult result;
        result.status = PhysicsNative3DAdapterStatus::completed;
        result.diagnostic = PhysicsNative3DAdapterDiagnostic::none;
        result.capabilities = jolt_physics_3d_adapter_capabilities();
        result.world = PhysicsWorld3D{scene.world_config};
        result.steps_executed = request.step.collision_steps;
        result.backend_body_count = static_cast<std::uint64_t>(
            std::count_if(body_slots.begin(), body_slots.end(), [](JoltBodySlot slot) { return slot.active; }));
        result.dispatched = true;
        result.bodies.reserve(scene.bodies.size());

        for (std::size_t index = 0; index < scene.bodies.size(); ++index) {
            auto desc = scene.bodies[index].body;
            if (body_slots[index].active) {
                desc.position = to_first_party_position(body_interface.GetPosition(body_slots[index].id));
                desc.velocity = to_first_party_vector(body_interface.GetLinearVelocity(body_slots[index].id));
            }
            const auto body = result.world.create_body(desc);
            result.bodies.push_back(PhysicsNative3DBodyRow{
                .source_index = index,
                .body = body,
            });
        }

        for (const auto slot : body_slots) {
            if (slot.active) {
                body_interface.RemoveBody(slot.id);
                body_interface.DestroyBody(slot.id);
            }
        }

        return result;
    }
};

} // namespace

PhysicsNative3DAdapterCapabilities jolt_physics_3d_adapter_capabilities() {
    return PhysicsNative3DAdapterCapabilities{
        .adapter_id = "joltphysics",
        .available = true,
        .supports_authored_collision_scene = true,
        .supports_step_simulation = true,
        .supports_collision_filters = true,
        .supports_triggers = false,
        .cross_platform_determinism = false,
        .exposes_native_handles = false,
        .supports_disabled_collision_bodies = false,
        .supports_default_collision_mask_wildcard = true,
        .supported_collision_layer_bits = JPH::ObjectLayerPairFilterMask::cMask,
        .supported_collision_mask_bits = JPH::ObjectLayerPairFilterMask::cMask,
        .max_collision_steps = physics_native_3d_max_collision_steps,
        .max_worker_threads = physics_native_3d_max_worker_threads,
        .min_backend_bodies = 2U,
        .max_backend_bodies = JPH::PhysicsSystem::cMaxBodiesLimit,
        .max_temp_allocator_bytes = physics_native_3d_max_temp_allocator_bytes,
    };
}

std::unique_ptr<IPhysicsNative3DAdapter> make_jolt_physics_3d_adapter() {
    return std::make_unique<JoltPhysics3DAdapter>();
}

} // namespace mirakana
