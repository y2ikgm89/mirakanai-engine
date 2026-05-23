// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/sprite_effect_particles.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana::runtime {
namespace {

constexpr float tau = 6.2831853071795864769F;

[[nodiscard]] bool finite(const float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool valid_id(std::string_view id) noexcept {
    return !id.empty() && std::ranges::none_of(id, [](const char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool valid_asset(const AssetId asset) noexcept {
    return asset.value != 0U;
}

[[nodiscard]] bool valid_color(const float value) noexcept {
    return finite(value) && value >= 0.0F && value <= 1.0F;
}

[[nodiscard]] bool valid_kind(RuntimeSpriteEffectParticleTemplateKind kind) noexcept {
    switch (kind) {
    case RuntimeSpriteEffectParticleTemplateKind::single_sprite:
    case RuntimeSpriteEffectParticleTemplateKind::radial_burst:
        return true;
    }
    return false;
}

void append_diagnostic(std::vector<RuntimeSpriteEffectParticleDiagnostic>& diagnostics,
                       RuntimeSpriteEffectParticleDiagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] const RuntimeSpriteEffectParticleTemplateDesc*
find_template(const std::vector<RuntimeSpriteEffectParticleTemplateDesc>& templates, std::string_view id) noexcept {
    for (const auto& desc : templates) {
        if (desc.id == id) {
            return &desc;
        }
    }
    return nullptr;
}

[[nodiscard]] std::string make_particle_id(const RuntimeSpriteEffectParticleSpawnEvent& event,
                                           const std::size_t particle_index) {
    return std::to_string(event.id.size()) + "_" + event.id + "#" + std::to_string(particle_index);
}

void validate_duplicate_template_ids(const std::vector<RuntimeSpriteEffectParticleTemplateDesc>& templates,
                                     std::vector<RuntimeSpriteEffectParticleDiagnostic>& diagnostics) {
    std::set<std::string> seen;
    for (const auto& desc : templates) {
        if (desc.id.empty()) {
            continue;
        }
        if (!seen.insert(desc.id).second) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::duplicate_template_id,
                                               .template_id = desc.id,
                                               .event_id = {},
                                               .particle_id = {},
                                               .emitter_id = {},
                                               .source_index = desc.source_index,
                                           });
        }
    }
}

void validate_duplicate_spawn_event_ids(const std::vector<RuntimeSpriteEffectParticleSpawnEvent>& events,
                                        std::vector<RuntimeSpriteEffectParticleDiagnostic>& diagnostics) {
    std::set<std::string_view> seen;
    for (const auto& event : events) {
        if (valid_id(event.id) && !seen.insert(event.id).second) {
            append_diagnostic(diagnostics,
                              RuntimeSpriteEffectParticleDiagnostic{
                                  .code = RuntimeSpriteEffectParticleDiagnosticCode::duplicate_spawn_event_id,
                                  .template_id = event.template_id,
                                  .event_id = event.id,
                                  .particle_id = {},
                                  .emitter_id = event.emitter_id,
                                  .source_index = event.source_index,
                              });
        }
    }
}

void validate_particle_ids(const RuntimeSpriteEffectParticleRequest& request,
                           std::vector<RuntimeSpriteEffectParticleDiagnostic>& diagnostics) {
    std::set<std::string> seen;
    for (const auto& particle : request.active_particles) {
        if (valid_id(particle.id) && !seen.insert(particle.id).second) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::duplicate_particle_id,
                                               .template_id = particle.template_id,
                                               .event_id = {},
                                               .particle_id = particle.id,
                                               .emitter_id = particle.emitter_id,
                                               .source_index = particle.source_index,
                                           });
        }
    }
    for (const auto& event : request.spawn_events) {
        for (std::size_t index = 0; index < static_cast<std::size_t>(event.spawn_count); ++index) {
            const auto particle_id = make_particle_id(event, index);
            if (!seen.insert(particle_id).second) {
                append_diagnostic(diagnostics,
                                  RuntimeSpriteEffectParticleDiagnostic{
                                      .code = RuntimeSpriteEffectParticleDiagnosticCode::duplicate_particle_id,
                                      .template_id = event.template_id,
                                      .event_id = event.id,
                                      .particle_id = particle_id,
                                      .emitter_id = event.emitter_id,
                                      .source_index = event.source_index,
                                  });
                return;
            }
        }
    }
}

void validate_template_rows(const std::vector<RuntimeSpriteEffectParticleTemplateDesc>& templates,
                            std::vector<RuntimeSpriteEffectParticleDiagnostic>& diagnostics) {
    validate_duplicate_template_ids(templates, diagnostics);
    for (const auto& desc : templates) {
        if (!valid_id(desc.id)) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_template_id,
                                               .template_id = desc.id,
                                               .event_id = {},
                                               .particle_id = {},
                                               .emitter_id = {},
                                               .source_index = desc.source_index,
                                           });
        }
        if (!valid_kind(desc.kind) || !finite(desc.velocity_x) || !finite(desc.velocity_y) ||
            !finite(desc.radial_speed) || desc.radial_speed < 0.0F) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_velocity,
                                               .template_id = desc.id,
                                               .event_id = {},
                                               .particle_id = {},
                                               .emitter_id = {},
                                               .source_index = desc.source_index,
                                           });
        }
        if (!valid_asset(desc.sprite)) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_sprite_asset,
                                               .template_id = desc.id,
                                               .event_id = {},
                                               .particle_id = {},
                                               .emitter_id = {},
                                               .source_index = desc.source_index,
                                           });
        }
        if (!finite(desc.lifetime_seconds) || desc.lifetime_seconds <= 0.0F) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_lifetime,
                                               .template_id = desc.id,
                                               .event_id = {},
                                               .particle_id = {},
                                               .emitter_id = {},
                                               .source_index = desc.source_index,
                                           });
        }
        if (!finite(desc.size_x) || !finite(desc.size_y) || desc.size_x <= 0.0F || desc.size_y <= 0.0F) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_size,
                                               .template_id = desc.id,
                                               .event_id = {},
                                               .particle_id = {},
                                               .emitter_id = {},
                                               .source_index = desc.source_index,
                                           });
        }
        if (!valid_color(desc.color_r) || !valid_color(desc.color_g) || !valid_color(desc.color_b) ||
            !valid_color(desc.color_a)) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_color,
                                               .template_id = desc.id,
                                               .event_id = {},
                                               .particle_id = {},
                                               .emitter_id = {},
                                               .source_index = desc.source_index,
                                           });
        }
    }
}

[[nodiscard]] bool valid_active_particle_values(const RuntimeSpriteEffectParticleState& particle) noexcept {
    return valid_kind(particle.kind) && valid_asset(particle.sprite) && finite(particle.age_seconds) &&
           particle.age_seconds >= 0.0F && finite(particle.lifetime_seconds) && particle.lifetime_seconds > 0.0F &&
           finite(particle.x) && finite(particle.y) && finite(particle.velocity_x) && finite(particle.velocity_y) &&
           finite(particle.size_x) && finite(particle.size_y) && particle.size_x > 0.0F && particle.size_y > 0.0F &&
           valid_color(particle.color_r) && valid_color(particle.color_g) && valid_color(particle.color_b) &&
           valid_color(particle.color_a);
}

void validate_active_particle_rows(const RuntimeSpriteEffectParticleRequest& request,
                                   std::vector<RuntimeSpriteEffectParticleDiagnostic>& diagnostics) {
    for (const auto& particle : request.active_particles) {
        if (!valid_id(particle.id) || !valid_id(particle.template_id) || !valid_id(particle.emitter_id) ||
            !valid_active_particle_values(particle)) {
            append_diagnostic(diagnostics,
                              RuntimeSpriteEffectParticleDiagnostic{
                                  .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_active_particle,
                                  .template_id = particle.template_id,
                                  .event_id = {},
                                  .particle_id = particle.id,
                                  .emitter_id = particle.emitter_id,
                                  .source_index = particle.source_index,
                              });
        }
        if (valid_id(particle.template_id) && find_template(request.templates, particle.template_id) == nullptr) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::missing_template,
                                               .template_id = particle.template_id,
                                               .event_id = {},
                                               .particle_id = particle.id,
                                               .emitter_id = particle.emitter_id,
                                               .source_index = particle.source_index,
                                           });
        }
    }
}

void validate_spawn_event_rows(const RuntimeSpriteEffectParticleRequest& request,
                               std::vector<RuntimeSpriteEffectParticleDiagnostic>& diagnostics) {
    validate_duplicate_spawn_event_ids(request.spawn_events, diagnostics);
    for (const auto& event : request.spawn_events) {
        if (!valid_id(event.id)) {
            append_diagnostic(diagnostics,
                              RuntimeSpriteEffectParticleDiagnostic{
                                  .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_spawn_event_id,
                                  .template_id = event.template_id,
                                  .event_id = event.id,
                                  .particle_id = {},
                                  .emitter_id = event.emitter_id,
                                  .source_index = event.source_index,
                              });
        }
        if (!valid_id(event.emitter_id)) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_emitter_id,
                                               .template_id = event.template_id,
                                               .event_id = event.id,
                                               .particle_id = {},
                                               .emitter_id = event.emitter_id,
                                               .source_index = event.source_index,
                                           });
        }
        if (!valid_id(event.template_id) || find_template(request.templates, event.template_id) == nullptr) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::missing_template,
                                               .template_id = event.template_id,
                                               .event_id = event.id,
                                               .particle_id = {},
                                               .emitter_id = event.emitter_id,
                                               .source_index = event.source_index,
                                           });
        }
        if (!finite(event.x) || !finite(event.y) || event.spawn_count == 0U) {
            append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                               .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_spawn_count,
                                               .template_id = event.template_id,
                                               .event_id = event.id,
                                               .particle_id = {},
                                               .emitter_id = event.emitter_id,
                                               .source_index = event.source_index,
                                           });
        }
    }
}

void validate_request(const RuntimeSpriteEffectParticleRequest& request,
                      std::vector<RuntimeSpriteEffectParticleDiagnostic>& diagnostics) {
    validate_template_rows(request.templates, diagnostics);
    validate_active_particle_rows(request, diagnostics);
    validate_spawn_event_rows(request, diagnostics);
    if (!finite(request.delta_seconds) || request.delta_seconds < 0.0F) {
        append_diagnostic(diagnostics, RuntimeSpriteEffectParticleDiagnostic{
                                           .code = RuntimeSpriteEffectParticleDiagnosticCode::invalid_delta_seconds,
                                           .template_id = {},
                                           .event_id = {},
                                           .particle_id = {},
                                           .emitter_id = {},
                                           .source_index = 0U,
                                       });
    }
}

[[nodiscard]] RuntimeSpriteEffectParticleState
make_spawned_particle(const RuntimeSpriteEffectParticleTemplateDesc& desc,
                      const RuntimeSpriteEffectParticleSpawnEvent& event, const std::size_t particle_index,
                      const std::size_t particle_count) {
    float radial_x = 0.0F;
    float radial_y = 0.0F;
    if (desc.kind == RuntimeSpriteEffectParticleTemplateKind::radial_burst && particle_count > 0U) {
        const auto angle = tau * (static_cast<float>(particle_index) / static_cast<float>(particle_count));
        radial_x = std::cos(angle) * desc.radial_speed;
        radial_y = std::sin(angle) * desc.radial_speed;
    }

    return RuntimeSpriteEffectParticleState{
        .id = make_particle_id(event, particle_index),
        .template_id = desc.id,
        .emitter_id = event.emitter_id,
        .kind = desc.kind,
        .sprite = desc.sprite,
        .age_seconds = 0.0F,
        .lifetime_seconds = desc.lifetime_seconds,
        .x = event.x,
        .y = event.y,
        .velocity_x = desc.velocity_x + radial_x,
        .velocity_y = desc.velocity_y + radial_y,
        .size_x = desc.size_x,
        .size_y = desc.size_y,
        .color_r = desc.color_r,
        .color_g = desc.color_g,
        .color_b = desc.color_b,
        .color_a = desc.color_a,
        .layer = desc.layer,
        .order = desc.order,
        .particle_index = particle_index,
        .source_index = event.source_index,
    };
}

[[nodiscard]] RuntimeSpriteEffectParticleRenderRow
make_render_row(const RuntimeSpriteEffectParticleState& particle) noexcept {
    const auto normalized_lifetime = particle.lifetime_seconds > 0.0F
                                         ? std::clamp(particle.age_seconds / particle.lifetime_seconds, 0.0F, 1.0F)
                                         : 1.0F;
    const auto fade = 1.0F - normalized_lifetime;
    return RuntimeSpriteEffectParticleRenderRow{
        .particle_id = particle.id,
        .template_id = particle.template_id,
        .emitter_id = particle.emitter_id,
        .kind = particle.kind,
        .sprite = particle.sprite,
        .x = particle.x,
        .y = particle.y,
        .size_x = particle.size_x,
        .size_y = particle.size_y,
        .color_r = particle.color_r,
        .color_g = particle.color_g,
        .color_b = particle.color_b,
        .color_a = particle.color_a * fade,
        .normalized_lifetime = normalized_lifetime,
        .layer = particle.layer,
        .order = particle.order,
        .particle_index = particle.particle_index,
        .source_index = particle.source_index,
    };
}

[[nodiscard]] bool particle_less(const RuntimeSpriteEffectParticleState& lhs,
                                 const RuntimeSpriteEffectParticleState& rhs) {
    if (lhs.layer != rhs.layer) {
        return lhs.layer < rhs.layer;
    }
    if (lhs.order != rhs.order) {
        return lhs.order < rhs.order;
    }
    if (lhs.source_index != rhs.source_index) {
        return lhs.source_index < rhs.source_index;
    }
    if (lhs.particle_index != rhs.particle_index) {
        return lhs.particle_index < rhs.particle_index;
    }
    return lhs.id < rhs.id;
}

[[nodiscard]] bool row_less(const RuntimeSpriteEffectParticleRenderRow& lhs,
                            const RuntimeSpriteEffectParticleRenderRow& rhs) {
    if (lhs.layer != rhs.layer) {
        return lhs.layer < rhs.layer;
    }
    if (lhs.order != rhs.order) {
        return lhs.order < rhs.order;
    }
    if (lhs.source_index != rhs.source_index) {
        return lhs.source_index < rhs.source_index;
    }
    if (lhs.particle_index != rhs.particle_index) {
        return lhs.particle_index < rhs.particle_index;
    }
    return lhs.particle_id < rhs.particle_id;
}

void fail_budget(RuntimeSpriteEffectParticlePlan& plan, RuntimeSpriteEffectParticleDiagnostic diagnostic) {
    plan.status = RuntimeSpriteEffectParticlePlanStatus::budget_exceeded;
    plan.next_active_particles.clear();
    plan.render_rows.clear();
    append_diagnostic(plan.diagnostics, std::move(diagnostic));
    ++plan.counters.budget_diagnostics;
}

[[nodiscard]] bool add_checked(std::size_t& value, const std::size_t increment) noexcept {
    if (increment > (std::numeric_limits<std::size_t>::max)() - value) {
        return false;
    }
    value += increment;
    return true;
}

[[nodiscard]] std::size_t count_surviving_active_particles(const RuntimeSpriteEffectParticleRequest& request) noexcept {
    std::size_t surviving = 0U;
    for (const auto& particle : request.active_particles) {
        if ((particle.age_seconds + request.delta_seconds) < particle.lifetime_seconds) {
            ++surviving;
        }
    }
    return surviving;
}

[[nodiscard]] bool preflight_particle_budgets(const RuntimeSpriteEffectParticleRequest& request,
                                              RuntimeSpriteEffectParticlePlan& plan,
                                              std::size_t& projected_active_particles) {
    std::size_t spawned_particles = 0U;
    for (const auto& event : request.spawn_events) {
        if (!add_checked(spawned_particles, static_cast<std::size_t>(event.spawn_count)) ||
            spawned_particles > request.max_spawned_particles) {
            fail_budget(plan, RuntimeSpriteEffectParticleDiagnostic{
                                  .code = RuntimeSpriteEffectParticleDiagnosticCode::active_particle_budget_exceeded,
                                  .template_id = event.template_id,
                                  .event_id = event.id,
                                  .particle_id = {},
                                  .emitter_id = event.emitter_id,
                                  .source_index = event.source_index,
                              });
            return false;
        }
    }

    projected_active_particles = count_surviving_active_particles(request);
    if (!add_checked(projected_active_particles, spawned_particles) ||
        projected_active_particles > request.max_active_particles) {
        fail_budget(plan, RuntimeSpriteEffectParticleDiagnostic{
                              .code = RuntimeSpriteEffectParticleDiagnosticCode::active_particle_budget_exceeded,
                              .template_id = {},
                              .event_id = {},
                              .particle_id = {},
                              .emitter_id = {},
                              .source_index = 0U,
                          });
        return false;
    }
    if (projected_active_particles > request.max_render_rows) {
        fail_budget(plan, RuntimeSpriteEffectParticleDiagnostic{
                              .code = RuntimeSpriteEffectParticleDiagnosticCode::render_row_budget_exceeded,
                              .template_id = {},
                              .event_id = {},
                              .particle_id = {},
                              .emitter_id = {},
                              .source_index = 0U,
                          });
        return false;
    }

    return true;
}

} // namespace

bool RuntimeSpriteEffectParticlePlan::succeeded() const noexcept {
    return status == RuntimeSpriteEffectParticlePlanStatus::ready ||
           status == RuntimeSpriteEffectParticlePlanStatus::no_spawns;
}

RuntimeSpriteEffectParticlePlan
plan_runtime_sprite_effect_particles(const RuntimeSpriteEffectParticleRequest& request) {
    RuntimeSpriteEffectParticlePlan plan;
    plan.counters.templates = request.templates.size();
    plan.counters.spawn_events = request.spawn_events.size();
    plan.counters.input_active_particles = request.active_particles.size();

    validate_request(request, plan.diagnostics);
    if (!plan.diagnostics.empty()) {
        plan.status = RuntimeSpriteEffectParticlePlanStatus::invalid_request;
        return plan;
    }
    if (request.spawn_events.size() > request.max_spawn_events) {
        fail_budget(plan, RuntimeSpriteEffectParticleDiagnostic{
                              .code = RuntimeSpriteEffectParticleDiagnosticCode::spawn_event_budget_exceeded,
                              .template_id = {},
                              .event_id = {},
                              .particle_id = {},
                              .emitter_id = {},
                              .source_index = 0U,
                          });
        return plan;
    }

    std::size_t projected_active_particles = 0U;
    if (!preflight_particle_budgets(request, plan, projected_active_particles)) {
        return plan;
    }

    validate_particle_ids(request, plan.diagnostics);
    if (!plan.diagnostics.empty()) {
        plan.status = RuntimeSpriteEffectParticlePlanStatus::invalid_request;
        return plan;
    }

    plan.next_active_particles.reserve(projected_active_particles);
    plan.render_rows.reserve(projected_active_particles);

    for (const auto& particle : request.active_particles) {
        auto next = particle;
        next.age_seconds += request.delta_seconds;
        next.x += next.velocity_x * request.delta_seconds;
        next.y += next.velocity_y * request.delta_seconds;
        if (next.age_seconds >= next.lifetime_seconds) {
            ++plan.counters.expired_particles;
            continue;
        }
        plan.next_active_particles.push_back(std::move(next));
    }

    for (const auto& event : request.spawn_events) {
        const auto* desc = find_template(request.templates, event.template_id);
        if (desc == nullptr) {
            continue;
        }
        for (std::size_t index = 0; index < static_cast<std::size_t>(event.spawn_count); ++index) {
            plan.next_active_particles.push_back(
                make_spawned_particle(*desc, event, index, static_cast<std::size_t>(event.spawn_count)));
            ++plan.counters.spawned_particles;
        }
    }

    if (plan.next_active_particles.size() > request.max_active_particles) {
        fail_budget(plan, RuntimeSpriteEffectParticleDiagnostic{
                              .code = RuntimeSpriteEffectParticleDiagnosticCode::active_particle_budget_exceeded,
                              .template_id = {},
                              .event_id = {},
                              .particle_id = {},
                              .emitter_id = {},
                              .source_index = 0U,
                          });
        return plan;
    }

    std::ranges::stable_sort(plan.next_active_particles, particle_less);
    for (const auto& particle : plan.next_active_particles) {
        plan.render_rows.push_back(make_render_row(particle));
    }
    std::ranges::stable_sort(plan.render_rows, row_less);

    if (plan.render_rows.size() > request.max_render_rows) {
        fail_budget(plan, RuntimeSpriteEffectParticleDiagnostic{
                              .code = RuntimeSpriteEffectParticleDiagnosticCode::render_row_budget_exceeded,
                              .template_id = {},
                              .event_id = {},
                              .particle_id = {},
                              .emitter_id = {},
                              .source_index = 0U,
                          });
        return plan;
    }

    plan.counters.surviving_particles = plan.next_active_particles.size();
    plan.counters.render_rows = plan.render_rows.size();
    for (const auto& row : plan.render_rows) {
        if (row.kind == RuntimeSpriteEffectParticleTemplateKind::single_sprite) {
            ++plan.counters.single_sprite_rows;
        } else if (row.kind == RuntimeSpriteEffectParticleTemplateKind::radial_burst) {
            ++plan.counters.radial_burst_rows;
        }
    }
    plan.status = plan.render_rows.empty() ? RuntimeSpriteEffectParticlePlanStatus::no_spawns
                                           : RuntimeSpriteEffectParticlePlanStatus::ready;
    return plan;
}

} // namespace mirakana::runtime
