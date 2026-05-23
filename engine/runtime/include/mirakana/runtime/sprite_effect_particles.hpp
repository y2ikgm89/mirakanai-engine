// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeSpriteEffectParticleTemplateKind : std::uint8_t {
    single_sprite,
    radial_burst,
};

enum class RuntimeSpriteEffectParticlePlanStatus : std::uint8_t {
    ready,
    no_spawns,
    invalid_request,
    budget_exceeded,
};

enum class RuntimeSpriteEffectParticleDiagnosticCode : std::uint8_t {
    none,
    invalid_template_id,
    duplicate_template_id,
    duplicate_spawn_event_id,
    duplicate_particle_id,
    invalid_sprite_asset,
    invalid_lifetime,
    invalid_size,
    invalid_color,
    invalid_velocity,
    invalid_emitter_id,
    invalid_spawn_event_id,
    invalid_spawn_count,
    missing_template,
    invalid_active_particle,
    invalid_delta_seconds,
    spawn_event_budget_exceeded,
    active_particle_budget_exceeded,
    render_row_budget_exceeded,
};

struct RuntimeSpriteEffectParticleTemplateDesc {
    std::string id;
    RuntimeSpriteEffectParticleTemplateKind kind{RuntimeSpriteEffectParticleTemplateKind::single_sprite};
    AssetId sprite;
    float lifetime_seconds{1.0F};
    float size_x{1.0F};
    float size_y{1.0F};
    float color_r{1.0F};
    float color_g{1.0F};
    float color_b{1.0F};
    float color_a{1.0F};
    float velocity_x{0.0F};
    float velocity_y{0.0F};
    float radial_speed{0.0F};
    int layer{0};
    int order{0};
    std::size_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSpriteEffectParticleTemplateDesc&) const = default;
};

struct RuntimeSpriteEffectParticleState {
    std::string id;
    std::string template_id;
    std::string emitter_id;
    RuntimeSpriteEffectParticleTemplateKind kind{RuntimeSpriteEffectParticleTemplateKind::single_sprite};
    AssetId sprite;
    float age_seconds{0.0F};
    float lifetime_seconds{1.0F};
    float x{0.0F};
    float y{0.0F};
    float velocity_x{0.0F};
    float velocity_y{0.0F};
    float size_x{1.0F};
    float size_y{1.0F};
    float color_r{1.0F};
    float color_g{1.0F};
    float color_b{1.0F};
    float color_a{1.0F};
    int layer{0};
    int order{0};
    std::size_t particle_index{0U};
    std::size_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSpriteEffectParticleState&) const = default;
};

struct RuntimeSpriteEffectParticleSpawnEvent {
    std::string id;
    std::string template_id;
    std::string emitter_id;
    float x{0.0F};
    float y{0.0F};
    std::uint32_t spawn_count{1U};
    std::size_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSpriteEffectParticleSpawnEvent&) const = default;
};

struct RuntimeSpriteEffectParticleRequest {
    std::vector<RuntimeSpriteEffectParticleTemplateDesc> templates;
    std::vector<RuntimeSpriteEffectParticleState> active_particles;
    std::vector<RuntimeSpriteEffectParticleSpawnEvent> spawn_events;
    float delta_seconds{0.0F};
    std::size_t max_spawn_events{256U};
    std::size_t max_spawned_particles{4096U};
    std::size_t max_active_particles{4096U};
    std::size_t max_render_rows{4096U};

    [[nodiscard]] bool operator==(const RuntimeSpriteEffectParticleRequest&) const = default;
};

struct RuntimeSpriteEffectParticleRenderRow {
    std::string particle_id;
    std::string template_id;
    std::string emitter_id;
    RuntimeSpriteEffectParticleTemplateKind kind{RuntimeSpriteEffectParticleTemplateKind::single_sprite};
    AssetId sprite;
    float x{0.0F};
    float y{0.0F};
    float size_x{1.0F};
    float size_y{1.0F};
    float color_r{1.0F};
    float color_g{1.0F};
    float color_b{1.0F};
    float color_a{1.0F};
    float normalized_lifetime{0.0F};
    int layer{0};
    int order{0};
    std::size_t particle_index{0U};
    std::size_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSpriteEffectParticleRenderRow&) const = default;
};

struct RuntimeSpriteEffectParticleDiagnostic {
    RuntimeSpriteEffectParticleDiagnosticCode code{RuntimeSpriteEffectParticleDiagnosticCode::none};
    std::string template_id;
    std::string event_id;
    std::string particle_id;
    std::string emitter_id;
    std::size_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSpriteEffectParticleDiagnostic&) const = default;
};

struct RuntimeSpriteEffectParticleCounters {
    std::size_t templates{0U};
    std::size_t spawn_events{0U};
    std::size_t input_active_particles{0U};
    std::size_t spawned_particles{0U};
    std::size_t surviving_particles{0U};
    std::size_t expired_particles{0U};
    std::size_t render_rows{0U};
    std::size_t single_sprite_rows{0U};
    std::size_t radial_burst_rows{0U};
    std::size_t budget_diagnostics{0U};

    [[nodiscard]] bool operator==(const RuntimeSpriteEffectParticleCounters&) const = default;
};

struct RuntimeSpriteEffectParticlePlan {
    RuntimeSpriteEffectParticlePlanStatus status{RuntimeSpriteEffectParticlePlanStatus::invalid_request};
    std::vector<RuntimeSpriteEffectParticleState> next_active_particles;
    std::vector<RuntimeSpriteEffectParticleRenderRow> render_rows;
    std::vector<RuntimeSpriteEffectParticleDiagnostic> diagnostics;
    RuntimeSpriteEffectParticleCounters counters;

    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] bool operator==(const RuntimeSpriteEffectParticlePlan&) const = default;
};

/// Advances caller-owned transient sprite effect state and returns value-only sprite rows.
/// This planner owns no renderer/RHI resources, launches no background workers, and keeps effect state explicit.
[[nodiscard]] RuntimeSpriteEffectParticlePlan
plan_runtime_sprite_effect_particles(const RuntimeSpriteEffectParticleRequest& request);

} // namespace mirakana::runtime
