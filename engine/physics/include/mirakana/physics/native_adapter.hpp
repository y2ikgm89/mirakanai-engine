// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/physics/physics3d.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace mirakana {

inline constexpr std::uint32_t physics_native_3d_max_collision_steps = 1024U;
inline constexpr std::uint32_t physics_native_3d_max_worker_threads = 64U;
inline constexpr std::size_t physics_native_3d_max_temp_allocator_bytes = 256U * 1024U * 1024U;

enum class PhysicsNative3DAdapterStatus : std::uint8_t {
    completed,
    unavailable,
    invalid_request,
    unsupported_feature,
    adapter_error,
};

enum class PhysicsNative3DAdapterDiagnostic : std::uint8_t {
    none,
    missing_adapter,
    adapter_unavailable,
    missing_scene,
    invalid_world_gravity,
    invalid_body_name,
    invalid_body_desc,
    duplicate_body_name,
    invalid_delta_seconds,
    invalid_step_config,
    body_budget_exceeded,
    authored_collision_scene_unsupported,
    step_simulation_unsupported,
    collision_filters_unsupported,
    triggers_unsupported,
    native_handles_exposed,
    cross_platform_determinism_unavailable,
    insufficient_backend_bodies,
    native_manifold_capacity_exceeded,
    native_body_pair_capacity_exceeded,
    native_contact_constraint_capacity_exceeded,
    adapter_failure,
};

struct PhysicsNative3DAdapterCapabilities {
    std::string adapter_id;
    bool available{false};
    bool supports_authored_collision_scene{false};
    bool supports_step_simulation{false};
    bool supports_collision_filters{false};
    bool supports_triggers{false};
    bool cross_platform_determinism{false};
    bool exposes_native_handles{false};
    bool supports_disabled_collision_bodies{true};
    bool supports_default_collision_mask_wildcard{true};
    std::uint32_t supported_collision_layer_bits{0xFFFF'FFFFU};
    std::uint32_t supported_collision_mask_bits{0xFFFF'FFFFU};
    std::uint32_t max_collision_steps{physics_native_3d_max_collision_steps};
    std::uint32_t max_worker_threads{physics_native_3d_max_worker_threads};
    std::size_t min_backend_bodies{0U};
    std::size_t max_backend_bodies{std::numeric_limits<std::size_t>::max()};
    std::size_t max_temp_allocator_bytes{physics_native_3d_max_temp_allocator_bytes};
};

struct PhysicsNative3DStepConfig {
    std::uint32_t collision_steps{1U};
    std::uint32_t max_collision_steps{8U};
    std::uint32_t worker_threads{1U};
    std::size_t temp_allocator_bytes{4U * 1024U * 1024U};
};

struct PhysicsNative3DSimulationRequest {
    const PhysicsAuthoredCollisionScene3DDesc* scene{nullptr};
    float delta_seconds{1.0F / 60.0F};
    PhysicsNative3DStepConfig step{};
    std::size_t max_bodies{1024U};
    bool require_cross_platform_determinism{false};
};

struct PhysicsNative3DBodyRow {
    std::size_t source_index{0U};
    PhysicsBody3DId body{};
};

struct PhysicsNative3DSimulationResult {
    PhysicsNative3DAdapterStatus status{PhysicsNative3DAdapterStatus::invalid_request};
    PhysicsNative3DAdapterDiagnostic diagnostic{PhysicsNative3DAdapterDiagnostic::none};
    PhysicsNative3DAdapterCapabilities capabilities{};
    PhysicsWorld3D world{};
    std::vector<PhysicsNative3DBodyRow> bodies;
    std::uint32_t steps_executed{0U};
    std::uint64_t backend_body_count{0U};
    bool dispatched{false};
};

class IPhysicsNative3DAdapter {
  public:
    IPhysicsNative3DAdapter() = default;
    IPhysicsNative3DAdapter(const IPhysicsNative3DAdapter&) = delete;
    IPhysicsNative3DAdapter& operator=(const IPhysicsNative3DAdapter&) = delete;
    IPhysicsNative3DAdapter(IPhysicsNative3DAdapter&&) = delete;
    IPhysicsNative3DAdapter& operator=(IPhysicsNative3DAdapter&&) = delete;
    virtual ~IPhysicsNative3DAdapter() = default;

    [[nodiscard]] virtual PhysicsNative3DAdapterCapabilities capabilities() const = 0;
    [[nodiscard]] virtual PhysicsNative3DSimulationResult simulate(const PhysicsNative3DSimulationRequest& request) = 0;
};

[[nodiscard]] PhysicsNative3DSimulationResult
simulate_native_physics_3d(IPhysicsNative3DAdapter* adapter, const PhysicsNative3DSimulationRequest& request);

} // namespace mirakana
