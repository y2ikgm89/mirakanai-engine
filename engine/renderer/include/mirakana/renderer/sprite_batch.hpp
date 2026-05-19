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

[[nodiscard]] SpriteBatchPlan plan_sprite_batches(std::span<const SpriteCommand> sprites);
[[nodiscard]] SpriteBatchPlan plan_sprite_batches(const SpriteBatchPlanDesc& desc);

} // namespace mirakana
