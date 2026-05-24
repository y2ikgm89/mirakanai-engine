// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/physics/native_adapter.hpp"

#include <cmath>
#include <string>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool finite_value(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_vector(Vec3 value) noexcept {
    return finite_value(value.x) && finite_value(value.y) && finite_value(value.z);
}

[[nodiscard]] bool uses_collision_filters(const PhysicsAuthoredCollisionScene3DDesc& scene) noexcept {
    for (const auto& body : scene.bodies) {
        if (!body.body.collision_enabled || body.body.collision_layer != 1U ||
            body.body.collision_mask != 0xFFFF'FFFFU) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool uses_triggers(const PhysicsAuthoredCollisionScene3DDesc& scene) noexcept {
    for (const auto& body : scene.bodies) {
        if (body.body.trigger) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool uses_disabled_collision_bodies(const PhysicsAuthoredCollisionScene3DDesc& scene) noexcept {
    for (const auto& body : scene.bodies) {
        if (!body.body.collision_enabled) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool supported_collision_bits(std::uint32_t value, std::uint32_t supported_bits) noexcept {
    return (value & ~supported_bits) == 0U;
}

[[nodiscard]] bool supported_collision_mask(std::uint32_t value,
                                            const PhysicsNative3DAdapterCapabilities& capabilities) noexcept {
    return (value == 0xFFFF'FFFFU && capabilities.supports_default_collision_mask_wildcard) ||
           supported_collision_bits(value, capabilities.supported_collision_mask_bits);
}

[[nodiscard]] bool uses_unsupported_collision_bits(const PhysicsAuthoredCollisionScene3DDesc& scene,
                                                   const PhysicsNative3DAdapterCapabilities& capabilities) noexcept {
    for (const auto& body : scene.bodies) {
        if (!supported_collision_bits(body.body.collision_layer, capabilities.supported_collision_layer_bits) ||
            !supported_collision_mask(body.body.collision_mask, capabilities)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] PhysicsNative3DSimulationResult make_result(PhysicsNative3DAdapterStatus status,
                                                          PhysicsNative3DAdapterDiagnostic diagnostic,
                                                          PhysicsNative3DAdapterCapabilities capabilities = {}) {
    PhysicsNative3DSimulationResult result;
    result.status = status;
    result.diagnostic = diagnostic;
    result.capabilities = std::move(capabilities);
    return result;
}

[[nodiscard]] PhysicsNative3DAdapterDiagnostic validate_request(const PhysicsNative3DSimulationRequest& request) {
    if (request.scene == nullptr) {
        return PhysicsNative3DAdapterDiagnostic::missing_scene;
    }
    if (!finite_vector(request.scene->world_config.gravity)) {
        return PhysicsNative3DAdapterDiagnostic::invalid_world_gravity;
    }
    if (!finite_value(request.delta_seconds) || request.delta_seconds <= 0.0F) {
        return PhysicsNative3DAdapterDiagnostic::invalid_delta_seconds;
    }
    if (request.step.collision_steps == 0U || request.step.max_collision_steps == 0U ||
        request.step.collision_steps > request.step.max_collision_steps || request.step.worker_threads == 0U ||
        request.step.temp_allocator_bytes == 0U ||
        request.step.collision_steps > physics_native_3d_max_collision_steps ||
        request.step.max_collision_steps > physics_native_3d_max_collision_steps ||
        request.step.worker_threads > physics_native_3d_max_worker_threads ||
        request.step.temp_allocator_bytes > physics_native_3d_max_temp_allocator_bytes) {
        return PhysicsNative3DAdapterDiagnostic::invalid_step_config;
    }
    if (request.max_bodies == 0U || request.scene->bodies.size() > request.max_bodies) {
        return PhysicsNative3DAdapterDiagnostic::body_budget_exceeded;
    }

    std::unordered_set<std::string> body_names;
    body_names.reserve(request.scene->bodies.size());
    for (const auto& body : request.scene->bodies) {
        if (body.name.empty()) {
            return PhysicsNative3DAdapterDiagnostic::invalid_body_name;
        }
        if (!body_names.insert(body.name).second) {
            return PhysicsNative3DAdapterDiagnostic::duplicate_body_name;
        }
        if (!is_valid_physics_body_desc(body.body)) {
            return PhysicsNative3DAdapterDiagnostic::invalid_body_desc;
        }
    }

    return PhysicsNative3DAdapterDiagnostic::none;
}

[[nodiscard]] bool exceeds_adapter_step_limits(const PhysicsNative3DSimulationRequest& request,
                                               const PhysicsNative3DAdapterCapabilities& capabilities) noexcept {
    return capabilities.max_collision_steps == 0U || capabilities.max_worker_threads == 0U ||
           capabilities.max_temp_allocator_bytes == 0U ||
           request.step.collision_steps > capabilities.max_collision_steps ||
           request.step.worker_threads > capabilities.max_worker_threads ||
           request.step.temp_allocator_bytes > capabilities.max_temp_allocator_bytes;
}

} // namespace

PhysicsNative3DSimulationResult simulate_native_physics_3d(IPhysicsNative3DAdapter* adapter,
                                                           const PhysicsNative3DSimulationRequest& request) {
    if (adapter == nullptr) {
        return make_result(PhysicsNative3DAdapterStatus::unavailable,
                           PhysicsNative3DAdapterDiagnostic::missing_adapter);
    }

    auto capabilities = adapter->capabilities();
    if (!capabilities.available) {
        return make_result(PhysicsNative3DAdapterStatus::unavailable,
                           PhysicsNative3DAdapterDiagnostic::adapter_unavailable, std::move(capabilities));
    }
    if (capabilities.exposes_native_handles) {
        return make_result(PhysicsNative3DAdapterStatus::unsupported_feature,
                           PhysicsNative3DAdapterDiagnostic::native_handles_exposed, std::move(capabilities));
    }
    if (!capabilities.supports_authored_collision_scene) {
        return make_result(PhysicsNative3DAdapterStatus::unsupported_feature,
                           PhysicsNative3DAdapterDiagnostic::authored_collision_scene_unsupported,
                           std::move(capabilities));
    }
    if (!capabilities.supports_step_simulation) {
        return make_result(PhysicsNative3DAdapterStatus::unsupported_feature,
                           PhysicsNative3DAdapterDiagnostic::step_simulation_unsupported, std::move(capabilities));
    }
    if (request.require_cross_platform_determinism && !capabilities.cross_platform_determinism) {
        return make_result(PhysicsNative3DAdapterStatus::unsupported_feature,
                           PhysicsNative3DAdapterDiagnostic::cross_platform_determinism_unavailable,
                           std::move(capabilities));
    }

    const auto request_diagnostic = validate_request(request);
    if (request_diagnostic != PhysicsNative3DAdapterDiagnostic::none) {
        return make_result(PhysicsNative3DAdapterStatus::invalid_request, request_diagnostic, std::move(capabilities));
    }
    if (exceeds_adapter_step_limits(request, capabilities)) {
        return make_result(PhysicsNative3DAdapterStatus::invalid_request,
                           PhysicsNative3DAdapterDiagnostic::invalid_step_config, std::move(capabilities));
    }
    if (request.scene->bodies.size() < capabilities.min_backend_bodies ||
        request.scene->bodies.size() > capabilities.max_backend_bodies) {
        return make_result(PhysicsNative3DAdapterStatus::unsupported_feature,
                           PhysicsNative3DAdapterDiagnostic::insufficient_backend_bodies, std::move(capabilities));
    }
    if (!capabilities.supports_disabled_collision_bodies && uses_disabled_collision_bodies(*request.scene)) {
        return make_result(PhysicsNative3DAdapterStatus::unsupported_feature,
                           PhysicsNative3DAdapterDiagnostic::collision_filters_unsupported, std::move(capabilities));
    }
    if (!capabilities.supports_collision_filters && uses_collision_filters(*request.scene)) {
        return make_result(PhysicsNative3DAdapterStatus::unsupported_feature,
                           PhysicsNative3DAdapterDiagnostic::collision_filters_unsupported, std::move(capabilities));
    }
    if (capabilities.supports_collision_filters && uses_unsupported_collision_bits(*request.scene, capabilities)) {
        return make_result(PhysicsNative3DAdapterStatus::unsupported_feature,
                           PhysicsNative3DAdapterDiagnostic::collision_filters_unsupported, std::move(capabilities));
    }
    if (!capabilities.supports_triggers && uses_triggers(*request.scene)) {
        return make_result(PhysicsNative3DAdapterStatus::unsupported_feature,
                           PhysicsNative3DAdapterDiagnostic::triggers_unsupported, std::move(capabilities));
    }

    try {
        auto result = adapter->simulate(request);
        result.capabilities = std::move(capabilities);
        result.dispatched = true;
        if (result.status != PhysicsNative3DAdapterStatus::completed &&
            result.diagnostic == PhysicsNative3DAdapterDiagnostic::none) {
            result.diagnostic = PhysicsNative3DAdapterDiagnostic::adapter_failure;
        }
        return result;
    } catch (...) {
        return make_result(PhysicsNative3DAdapterStatus::adapter_error,
                           PhysicsNative3DAdapterDiagnostic::adapter_failure, std::move(capabilities));
    }
}

} // namespace mirakana
