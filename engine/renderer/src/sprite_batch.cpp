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
                                             std::uint64_t sprite_index) {
    if (!sprite.texture.enabled) {
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

} // namespace

SpriteBatchPlan plan_sprite_batches(std::span<const SpriteCommand> sprites) {
    SpriteBatchPlan plan;
    plan.sprite_count = static_cast<std::uint64_t>(sprites.size());

    std::uint64_t sprite_index = 0;
    for (const auto& sprite : sprites) {
        const auto key = classify_sprite(plan, sprite, sprite_index);
        append_or_extend_batch(plan, key, sprite_index);
        ++sprite_index;
    }

    plan.draw_count = static_cast<std::uint64_t>(plan.batches.size());
    for (const auto& batch : plan.batches) {
        if (batch.kind == SpriteBatchKind::textured) {
            ++plan.texture_bind_count;
        }
    }

    return plan;
}

} // namespace mirakana
