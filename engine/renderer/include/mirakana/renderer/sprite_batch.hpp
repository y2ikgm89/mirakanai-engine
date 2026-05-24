// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/renderer/renderer.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace mirakana {

enum class SpriteBatchKind : std::uint8_t { solid_color, textured };

enum class SpriteBatchDiagnosticCode : std::uint8_t {
    none,
    missing_texture_atlas,
    invalid_uv_rect,
    unsupported_reordering_policy,
    untextured_sprite_disallowed,
};

struct SpriteBatchDiagnostic {
    SpriteBatchDiagnosticCode code{SpriteBatchDiagnosticCode::none};
    std::uint64_t sprite_index{0};
};

struct SpriteBatchRange {
    SpriteBatchKind kind{SpriteBatchKind::solid_color};
    std::uint64_t first_sprite{0};
    std::uint64_t sprite_count{0};
    AssetId atlas_page;
};

struct SpriteBatchPlanOptions {
    bool allow_sprite_reordering{false};
    bool require_atlas_backed_sprites{false};
};

struct SpriteBatchPlanDesc {
    std::span<const SpriteCommand> sprites;
    SpriteBatchPlanOptions options;
};

struct SpriteBatchPlan {
    std::vector<SpriteBatchRange> batches;
    std::vector<SpriteBatchDiagnostic> diagnostics;
    std::uint64_t sprite_count{0};
    std::uint64_t textured_sprite_count{0};
    std::uint64_t draw_count{0};
    std::uint64_t texture_bind_count{0};
    std::uint64_t atlas_backed_batch_count{0};
    std::uint64_t repeated_atlas_batch_count{0};
    std::uint64_t repeated_atlas_sprite_count{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

enum class SpriteBatchBudgetLane : std::uint8_t { world, ui, effects };

enum class SpriteBatchBudgetProfileStatus : std::uint8_t {
    ready,
    invalid_request,
    diagnostics,
    budget_exceeded,
};

enum class SpriteBatchBudgetDiagnosticCode : std::uint8_t {
    none,
    missing_plan,
    invalid_budget,
    plan_diagnostics,
    sprite_budget_exceeded,
    draw_budget_exceeded,
    texture_bind_budget_exceeded,
};

struct SpriteBatchBudgetDesc {
    std::uint64_t max_sprites{0};
    std::uint64_t max_draws{0};
    std::uint64_t max_texture_binds{0};
};

struct SpriteBatchBudgetLanePlanDesc {
    SpriteBatchBudgetLane lane{SpriteBatchBudgetLane::world};
    const SpriteBatchPlan* plan{nullptr};
    SpriteBatchBudgetDesc budget;
};

struct SpriteBatchBudgetRow {
    SpriteBatchBudgetLane lane{SpriteBatchBudgetLane::world};
    std::uint64_t sprite_count{0};
    std::uint64_t draw_count{0};
    std::uint64_t texture_bind_count{0};
    std::uint64_t max_sprites{0};
    std::uint64_t max_draws{0};
    std::uint64_t max_texture_binds{0};
    bool within_budget{false};
};

struct SpriteBatchBudgetDiagnostic {
    SpriteBatchBudgetDiagnosticCode code{SpriteBatchBudgetDiagnosticCode::none};
    SpriteBatchBudgetLane lane{SpriteBatchBudgetLane::world};
};

struct SpriteBatchBudgetProfile {
    SpriteBatchBudgetProfileStatus status{SpriteBatchBudgetProfileStatus::invalid_request};
    std::vector<SpriteBatchBudgetRow> rows;
    std::vector<SpriteBatchBudgetDiagnostic> diagnostics;
    std::uint64_t total_sprites{0};
    std::uint64_t total_draws{0};
    std::uint64_t total_texture_binds{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == SpriteBatchBudgetProfileStatus::ready && diagnostics.empty();
    }
};

[[nodiscard]] SpriteBatchPlan plan_sprite_batches(std::span<const SpriteCommand> sprites);
[[nodiscard]] SpriteBatchPlan plan_sprite_batches(const SpriteBatchPlanDesc& desc);
[[nodiscard]] SpriteBatchBudgetProfile
plan_sprite_batch_budget_profile(std::span<const SpriteBatchBudgetLanePlanDesc> lanes);

} // namespace mirakana
