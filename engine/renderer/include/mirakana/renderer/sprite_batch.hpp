// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/renderer/renderer.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace mirakana {

enum class SpriteBatchKind { solid_color, textured };

enum class SpriteBatchDiagnosticCode {
    none,
    missing_texture_atlas,
    invalid_uv_rect,
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

struct SpriteBatchPlan {
    std::vector<SpriteBatchRange> batches;
    std::vector<SpriteBatchDiagnostic> diagnostics;
    std::uint64_t sprite_count{0};
    std::uint64_t textured_sprite_count{0};
    std::uint64_t draw_count{0};
    std::uint64_t texture_bind_count{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] SpriteBatchPlan plan_sprite_batches(std::span<const SpriteCommand> sprites);

} // namespace mirakana
