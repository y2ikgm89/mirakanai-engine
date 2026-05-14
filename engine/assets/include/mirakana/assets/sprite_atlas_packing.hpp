// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_source_format.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace mirakana {

/// Maximum width or height of the packed atlas (inclusive). Matches the PNG importer
/// `max_texture_extent` policy so tooling can compose atlases from audited decodes.
constexpr std::uint32_t sprite_atlas_packing_max_side = 16384;

/// Hard cap on sprite count for deterministic host-side packing.
constexpr std::size_t sprite_atlas_packing_max_items = 4096;

enum class SpriteAtlasPackingDiagnosticCode : std::uint8_t {
    ok,
    empty_items,
    too_many_items,
    zero_dimension,
    dimension_exceeds_limit,
    atlas_exceeds_max_side,
    rgba_byte_size_mismatch,
};

struct SpriteAtlasPackingDiagnostic {
    SpriteAtlasPackingDiagnosticCode code{SpriteAtlasPackingDiagnosticCode::ok};
    std::string message;
};

/// Pixel placement for one input sprite in the packed atlas (top-left origin).
struct SpriteAtlasPackedPlacement {
    std::uint32_t x{0};
    std::uint32_t y{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
};

/// One decoded RGBA8 sprite tile passed into the packer. `rgba8_pixels` must hold
/// exactly `width * height * 4` bytes in row-major RGBA8 order.
struct SpriteAtlasPackingItemView {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::span<const std::uint8_t> rgba8_pixels;
};

struct SpriteAtlasRgba8PackingOutput {
    TextureSourceDocument atlas;
    /// Parallel to the input `items` span (same length, same order).
    std::vector<SpriteAtlasPackedPlacement> placements;
};

/// Deterministic shelf-style row packing: sprites are ordered by descending height, then
/// descending width, then ascending original index. Rows advance when the next sprite does
/// not fit in `max_side` horizontal space. The atlas size is the tight axis-aligned bounds
/// of all placements (each axis at most `max_side`).
[[nodiscard]] std::variant<SpriteAtlasRgba8PackingOutput, SpriteAtlasPackingDiagnostic>
pack_sprite_atlas_rgba8_max_side(std::span<const SpriteAtlasPackingItemView> items,
                                 std::uint32_t max_side = sprite_atlas_packing_max_side);

} // namespace mirakana
