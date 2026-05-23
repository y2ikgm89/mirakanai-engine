// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/sprite_effect_particles.hpp"

#include <cmath>
#include <limits>
#include <vector>

namespace {

[[nodiscard]] bool near(const float lhs, const float rhs) noexcept {
    return std::fabs(lhs - rhs) <= 0.0001F;
}

[[nodiscard]] mirakana::runtime::RuntimeSpriteEffectParticleRequest sprite_effect_particle_request() {
    using namespace mirakana;
    using namespace mirakana::runtime;

    return RuntimeSpriteEffectParticleRequest{
        .templates =
            std::vector<RuntimeSpriteEffectParticleTemplateDesc>{
                RuntimeSpriteEffectParticleTemplateDesc{
                    .id = "trail.smoke",
                    .kind = RuntimeSpriteEffectParticleTemplateKind::single_sprite,
                    .sprite = AssetId{101U},
                    .lifetime_seconds = 1.0F,
                    .size_x = 0.25F,
                    .size_y = 0.25F,
                    .color_r = 0.6F,
                    .color_g = 0.55F,
                    .color_b = 0.5F,
                    .color_a = 1.0F,
                    .velocity_x = 2.0F,
                    .velocity_y = -1.0F,
                    .radial_speed = 0.0F,
                    .layer = 0,
                    .order = 0,
                    .source_index = 0U,
                },
                RuntimeSpriteEffectParticleTemplateDesc{
                    .id = "dust.burst",
                    .kind = RuntimeSpriteEffectParticleTemplateKind::radial_burst,
                    .sprite = AssetId{101U},
                    .lifetime_seconds = 0.8F,
                    .size_x = 0.2F,
                    .size_y = 0.2F,
                    .color_r = 0.8F,
                    .color_g = 0.7F,
                    .color_b = 0.45F,
                    .color_a = 1.0F,
                    .velocity_x = 0.0F,
                    .velocity_y = 0.0F,
                    .radial_speed = 1.0F,
                    .layer = 1,
                    .order = 5,
                    .source_index = 1U,
                },
            },
        .active_particles =
            std::vector<RuntimeSpriteEffectParticleState>{
                RuntimeSpriteEffectParticleState{
                    .id = "active.smoke.0",
                    .template_id = "trail.smoke",
                    .emitter_id = "player",
                    .kind = RuntimeSpriteEffectParticleTemplateKind::single_sprite,
                    .sprite = AssetId{101U},
                    .age_seconds = 0.20F,
                    .lifetime_seconds = 1.0F,
                    .x = 0.0F,
                    .y = 1.0F,
                    .velocity_x = 2.0F,
                    .velocity_y = -1.0F,
                    .size_x = 0.25F,
                    .size_y = 0.25F,
                    .color_r = 0.6F,
                    .color_g = 0.55F,
                    .color_b = 0.5F,
                    .color_a = 1.0F,
                    .layer = 0,
                    .order = 0,
                    .particle_index = 0U,
                    .source_index = 0U,
                },
            },
        .spawn_events =
            std::vector<RuntimeSpriteEffectParticleSpawnEvent>{
                RuntimeSpriteEffectParticleSpawnEvent{
                    .id = "impact.1",
                    .template_id = "dust.burst",
                    .emitter_id = "enemy",
                    .x = 2.0F,
                    .y = 1.5F,
                    .spawn_count = 3U,
                    .source_index = 1U,
                },
            },
        .delta_seconds = 0.25F,
        .max_spawn_events = std::numeric_limits<std::size_t>::max(),
        .max_spawned_particles = std::numeric_limits<std::size_t>::max(),
        .max_active_particles = std::numeric_limits<std::size_t>::max(),
        .max_render_rows = std::numeric_limits<std::size_t>::max(),
    };
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime::RuntimeSpriteEffectParticlePlan& plan,
                                  const mirakana::runtime::RuntimeSpriteEffectParticleDiagnosticCode code) {
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime sprite effect particles advance caller owned particles and spawn deterministic rows") {
    using namespace mirakana::runtime;

    const auto request = sprite_effect_particle_request();
    const auto first = plan_runtime_sprite_effect_particles(request);
    const auto second = plan_runtime_sprite_effect_particles(request);

    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(first.status == RuntimeSpriteEffectParticlePlanStatus::ready);
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.next_active_particles == second.next_active_particles);
    MK_REQUIRE(first.render_rows == second.render_rows);
    MK_REQUIRE(first.counters.templates == 2U);
    MK_REQUIRE(first.counters.spawn_events == 1U);
    MK_REQUIRE(first.counters.input_active_particles == 1U);
    MK_REQUIRE(first.counters.spawned_particles == 3U);
    MK_REQUIRE(first.counters.surviving_particles == 4U);
    MK_REQUIRE(first.counters.expired_particles == 0U);
    MK_REQUIRE(first.counters.render_rows == 4U);
    MK_REQUIRE(first.counters.single_sprite_rows == 1U);
    MK_REQUIRE(first.counters.radial_burst_rows == 3U);

    MK_REQUIRE(first.render_rows[0].particle_id == "active.smoke.0");
    MK_REQUIRE(first.render_rows[0].template_id == "trail.smoke");
    MK_REQUIRE(near(first.render_rows[0].x, 0.5F));
    MK_REQUIRE(near(first.render_rows[0].y, 0.75F));
    MK_REQUIRE(near(first.render_rows[0].normalized_lifetime, 0.45F));
    MK_REQUIRE(near(first.render_rows[0].color_a, 0.55F));
    MK_REQUIRE(first.render_rows[1].template_id == "dust.burst");
    MK_REQUIRE(first.render_rows[1].emitter_id == "enemy");
    MK_REQUIRE(first.render_rows[1].particle_index == 0U);
    MK_REQUIRE(near(first.render_rows[1].x, 2.0F));
    MK_REQUIRE(near(first.render_rows[1].y, 1.5F));
    MK_REQUIRE(first.render_rows[3].particle_index == 2U);
}

MK_TEST("runtime sprite effect particles fail closed on invalid authoring rows") {
    using Code = mirakana::runtime::RuntimeSpriteEffectParticleDiagnosticCode;

    auto request = sprite_effect_particle_request();
    request.templates[0].id = request.templates[1].id;
    request.templates[1].sprite = mirakana::AssetId{};
    request.templates[1].lifetime_seconds = 0.0F;
    request.spawn_events[0].emitter_id = {};

    const auto plan = mirakana::runtime::plan_runtime_sprite_effect_particles(request);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::invalid_request);
    MK_REQUIRE(plan.next_active_particles.empty());
    MK_REQUIRE(plan.render_rows.empty());
    MK_REQUIRE(has_diagnostic(plan, Code::duplicate_template_id));
    MK_REQUIRE(has_diagnostic(plan, Code::invalid_sprite_asset));
    MK_REQUIRE(has_diagnostic(plan, Code::invalid_lifetime));
    MK_REQUIRE(has_diagnostic(plan, Code::invalid_emitter_id));
}

MK_TEST("runtime sprite effect particles reject reviewed budgets before partial rows escape") {
    using Code = mirakana::runtime::RuntimeSpriteEffectParticleDiagnosticCode;

    auto spawn_budget_request = sprite_effect_particle_request();
    spawn_budget_request.max_spawn_events = 0U;

    const auto spawn_budget_plan = mirakana::runtime::plan_runtime_sprite_effect_particles(spawn_budget_request);

    MK_REQUIRE(!spawn_budget_plan.succeeded());
    MK_REQUIRE(spawn_budget_plan.status == mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::budget_exceeded);
    MK_REQUIRE(spawn_budget_plan.next_active_particles.empty());
    MK_REQUIRE(spawn_budget_plan.render_rows.empty());
    MK_REQUIRE(spawn_budget_plan.diagnostics.size() == 1U);
    MK_REQUIRE(spawn_budget_plan.diagnostics[0].code == Code::spawn_event_budget_exceeded);

    auto active_budget_request = sprite_effect_particle_request();
    active_budget_request.max_active_particles = 3U;

    const auto active_budget_plan = mirakana::runtime::plan_runtime_sprite_effect_particles(active_budget_request);

    MK_REQUIRE(!active_budget_plan.succeeded());
    MK_REQUIRE(active_budget_plan.status == mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::budget_exceeded);
    MK_REQUIRE(active_budget_plan.next_active_particles.empty());
    MK_REQUIRE(active_budget_plan.render_rows.empty());
    MK_REQUIRE(active_budget_plan.diagnostics.size() == 1U);
    MK_REQUIRE(active_budget_plan.diagnostics[0].code == Code::active_particle_budget_exceeded);

    auto render_budget_request = sprite_effect_particle_request();
    render_budget_request.max_render_rows = 3U;

    const auto render_budget_plan = mirakana::runtime::plan_runtime_sprite_effect_particles(render_budget_request);

    MK_REQUIRE(!render_budget_plan.succeeded());
    MK_REQUIRE(render_budget_plan.status == mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::budget_exceeded);
    MK_REQUIRE(render_budget_plan.next_active_particles.empty());
    MK_REQUIRE(render_budget_plan.render_rows.empty());
    MK_REQUIRE(render_budget_plan.diagnostics.size() == 1U);
    MK_REQUIRE(render_budget_plan.diagnostics[0].code == Code::render_row_budget_exceeded);

    auto huge_spawn_budget_request = sprite_effect_particle_request();
    huge_spawn_budget_request.active_particles.clear();
    huge_spawn_budget_request.spawn_events[0].spawn_count = std::numeric_limits<std::uint32_t>::max();
    huge_spawn_budget_request.max_active_particles = 16U;
    huge_spawn_budget_request.max_render_rows = 16U;

    const auto huge_spawn_budget_plan =
        mirakana::runtime::plan_runtime_sprite_effect_particles(huge_spawn_budget_request);

    MK_REQUIRE(!huge_spawn_budget_plan.succeeded());
    MK_REQUIRE(huge_spawn_budget_plan.status ==
               mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::budget_exceeded);
    MK_REQUIRE(huge_spawn_budget_plan.next_active_particles.empty());
    MK_REQUIRE(huge_spawn_budget_plan.render_rows.empty());
    MK_REQUIRE(huge_spawn_budget_plan.diagnostics.size() == 1U);
    MK_REQUIRE(huge_spawn_budget_plan.diagnostics[0].code == Code::active_particle_budget_exceeded);
}

MK_TEST("runtime sprite effect particles reject duplicate event and generated particle ids") {
    using Code = mirakana::runtime::RuntimeSpriteEffectParticleDiagnosticCode;

    auto duplicate_event_request = sprite_effect_particle_request();
    duplicate_event_request.spawn_events.push_back(duplicate_event_request.spawn_events[0]);
    duplicate_event_request.spawn_events.back().source_index = 2U;

    const auto duplicate_event_plan = mirakana::runtime::plan_runtime_sprite_effect_particles(duplicate_event_request);

    MK_REQUIRE(!duplicate_event_plan.succeeded());
    MK_REQUIRE(duplicate_event_plan.status ==
               mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::invalid_request);
    MK_REQUIRE(has_diagnostic(duplicate_event_plan, Code::duplicate_spawn_event_id));
    MK_REQUIRE(duplicate_event_plan.next_active_particles.empty());
    MK_REQUIRE(duplicate_event_plan.render_rows.empty());

    auto duplicate_particle_request = sprite_effect_particle_request();
    duplicate_particle_request.active_particles[0].id = "8_impact.1#0";

    const auto duplicate_particle_plan =
        mirakana::runtime::plan_runtime_sprite_effect_particles(duplicate_particle_request);

    MK_REQUIRE(!duplicate_particle_plan.succeeded());
    MK_REQUIRE(duplicate_particle_plan.status ==
               mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::invalid_request);
    MK_REQUIRE(has_diagnostic(duplicate_particle_plan, Code::duplicate_particle_id));
    MK_REQUIRE(duplicate_particle_plan.next_active_particles.empty());
    MK_REQUIRE(duplicate_particle_plan.render_rows.empty());
}

MK_TEST("runtime sprite effect particles accepts generated next active particles on the next frame") {
    auto first_request = sprite_effect_particle_request();
    first_request.active_particles.clear();

    const auto first = mirakana::runtime::plan_runtime_sprite_effect_particles(first_request);
    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.next_active_particles.size() == 3U);

    auto second_request = sprite_effect_particle_request();
    second_request.active_particles = first.next_active_particles;
    second_request.spawn_events.clear();

    const auto second = mirakana::runtime::plan_runtime_sprite_effect_particles(second_request);

    MK_REQUIRE(second.succeeded());
    MK_REQUIRE(second.diagnostics.empty());
    MK_REQUIRE(second.render_rows.size() == 3U);
}

MK_TEST("runtime sprite effect particles expire old particles without diagnostics") {
    auto request = sprite_effect_particle_request();
    request.active_particles[0].age_seconds = 0.90F;
    request.spawn_events.clear();

    const auto plan = mirakana::runtime::plan_runtime_sprite_effect_particles(request);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::no_spawns);
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.next_active_particles.empty());
    MK_REQUIRE(plan.render_rows.empty());
    MK_REQUIRE(plan.counters.input_active_particles == 1U);
    MK_REQUIRE(plan.counters.expired_particles == 1U);
    MK_REQUIRE(plan.counters.surviving_particles == 0U);
}

int main() {
    return mirakana::test::run_all();
}
