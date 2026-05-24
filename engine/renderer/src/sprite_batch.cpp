// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/sprite_batch.hpp"

#include <cmath>

namespace mirakana {
namespace {

struct SpriteBatchKey {
    SpriteBatchKind kind{SpriteBatchKind::solid_color};
    AssetId atlas_page;
};

[[nodiscard]] bool operator==(SpriteBatchKey lhs, SpriteBatchKey rhs) noexcept {
    return lhs.kind == rhs.kind && lhs.atlas_page == rhs.atlas_page;
}

[[nodiscard]] bool valid_uv_rect(SpriteUvRect rect) noexcept {
    return std::isfinite(rect.u0) && std::isfinite(rect.v0) && std::isfinite(rect.u1) && std::isfinite(rect.v1) &&
           rect.u0 >= 0.0F && rect.v0 >= 0.0F && rect.u1 <= 1.0F && rect.v1 <= 1.0F && rect.u0 < rect.u1 &&
           rect.v0 < rect.v1;
}

[[nodiscard]] SpriteBatchKey classify_sprite(SpriteBatchPlan& plan, const SpriteCommand& sprite,
                                             std::uint64_t sprite_index, SpriteBatchPlanOptions options) {
    if (!sprite.texture.enabled) {
        if (options.require_atlas_backed_sprites) {
            plan.diagnostics.push_back(SpriteBatchDiagnostic{
                .code = SpriteBatchDiagnosticCode::untextured_sprite_disallowed,
                .sprite_index = sprite_index,
            });
        }
        return {};
    }

    if (sprite.texture.atlas_page.value == 0) {
        plan.diagnostics.push_back(SpriteBatchDiagnostic{
            .code = SpriteBatchDiagnosticCode::missing_texture_atlas,
            .sprite_index = sprite_index,
        });
        return {};
    }

    if (!valid_uv_rect(sprite.texture.uv_rect)) {
        plan.diagnostics.push_back(SpriteBatchDiagnostic{
            .code = SpriteBatchDiagnosticCode::invalid_uv_rect,
            .sprite_index = sprite_index,
        });
        return {};
    }

    ++plan.textured_sprite_count;
    return SpriteBatchKey{.kind = SpriteBatchKind::textured, .atlas_page = sprite.texture.atlas_page};
}

void append_or_extend_batch(SpriteBatchPlan& plan, SpriteBatchKey key, std::uint64_t sprite_index) {
    if (!plan.batches.empty()) {
        const auto& last = plan.batches.back();
        const auto last_key = SpriteBatchKey{.kind = last.kind, .atlas_page = last.atlas_page};
        if (last_key == key) {
            ++plan.batches.back().sprite_count;
            return;
        }
    }

    plan.batches.push_back(SpriteBatchRange{
        .kind = key.kind,
        .first_sprite = sprite_index,
        .sprite_count = 1,
        .atlas_page = key.atlas_page,
    });
}

[[nodiscard]] bool budget_desc_valid(SpriteBatchBudgetDesc budget) noexcept {
    return budget.max_sprites != 0 && budget.max_draws != 0;
}

void add_budget_diagnostic(SpriteBatchBudgetProfile& profile, SpriteBatchBudgetLane lane,
                           SpriteBatchBudgetDiagnosticCode code) {
    profile.diagnostics.push_back(SpriteBatchBudgetDiagnostic{
        .code = code,
        .lane = lane,
    });
}

} // namespace

SpriteBatchPlan plan_sprite_batches(std::span<const SpriteCommand> sprites) {
    return plan_sprite_batches(SpriteBatchPlanDesc{.sprites = sprites});
}

SpriteBatchPlan plan_sprite_batches(const SpriteBatchPlanDesc& desc) {
    SpriteBatchPlan plan;
    plan.sprite_count = static_cast<std::uint64_t>(desc.sprites.size());

    if (desc.options.allow_sprite_reordering) {
        plan.diagnostics.push_back(SpriteBatchDiagnostic{
            .code = SpriteBatchDiagnosticCode::unsupported_reordering_policy,
            .sprite_index = 0,
        });
        return plan;
    }

    std::uint64_t sprite_index = 0;
    for (const auto& sprite : desc.sprites) {
        const auto key = classify_sprite(plan, sprite, sprite_index, desc.options);
        append_or_extend_batch(plan, key, sprite_index);
        ++sprite_index;
    }

    plan.draw_count = static_cast<std::uint64_t>(plan.batches.size());
    for (const auto& batch : plan.batches) {
        if (batch.kind == SpriteBatchKind::textured) {
            ++plan.texture_bind_count;
            ++plan.atlas_backed_batch_count;
            if (batch.sprite_count > 1) {
                ++plan.repeated_atlas_batch_count;
                plan.repeated_atlas_sprite_count += batch.sprite_count;
            }
        }
    }

    return plan;
}

SpriteBatchBudgetProfile plan_sprite_batch_budget_profile(std::span<const SpriteBatchBudgetLanePlanDesc> lanes) {
    SpriteBatchBudgetProfile profile;
    profile.rows.reserve(lanes.size());
    if (lanes.empty()) {
        profile.status = SpriteBatchBudgetProfileStatus::invalid_request;
        return profile;
    }

    bool has_invalid_request = false;
    bool has_plan_diagnostics = false;
    bool has_budget_exceeded = false;
    for (const auto& lane : lanes) {
        if (lane.plan == nullptr) {
            has_invalid_request = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::missing_plan);
            continue;
        }
        if (!budget_desc_valid(lane.budget)) {
            has_invalid_request = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::invalid_budget);
            continue;
        }

        const auto& plan = *lane.plan;
        const bool within_budget = plan.sprite_count <= lane.budget.max_sprites &&
                                   plan.draw_count <= lane.budget.max_draws &&
                                   plan.texture_bind_count <= lane.budget.max_texture_binds && plan.diagnostics.empty();
        profile.rows.push_back(SpriteBatchBudgetRow{
            .lane = lane.lane,
            .sprite_count = plan.sprite_count,
            .draw_count = plan.draw_count,
            .texture_bind_count = plan.texture_bind_count,
            .max_sprites = lane.budget.max_sprites,
            .max_draws = lane.budget.max_draws,
            .max_texture_binds = lane.budget.max_texture_binds,
            .within_budget = within_budget,
        });
        profile.total_sprites += plan.sprite_count;
        profile.total_draws += plan.draw_count;
        profile.total_texture_binds += plan.texture_bind_count;

        if (!plan.diagnostics.empty()) {
            has_plan_diagnostics = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::plan_diagnostics);
        }
        if (plan.sprite_count > lane.budget.max_sprites) {
            has_budget_exceeded = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::sprite_budget_exceeded);
        }
        if (plan.draw_count > lane.budget.max_draws) {
            has_budget_exceeded = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::draw_budget_exceeded);
        }
        if (plan.texture_bind_count > lane.budget.max_texture_binds) {
            has_budget_exceeded = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::texture_bind_budget_exceeded);
        }
    }

    if (has_invalid_request) {
        profile.status = SpriteBatchBudgetProfileStatus::invalid_request;
    } else if (has_plan_diagnostics) {
        profile.status = SpriteBatchBudgetProfileStatus::diagnostics;
    } else if (has_budget_exceeded) {
        profile.status = SpriteBatchBudgetProfileStatus::budget_exceeded;
    } else {
        profile.status = SpriteBatchBudgetProfileStatus::ready;
    }
    return profile;
}

} // namespace mirakana
