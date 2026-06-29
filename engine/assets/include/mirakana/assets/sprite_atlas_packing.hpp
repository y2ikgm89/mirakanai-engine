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

enum class ProductionSpriteAtlasPowerOfTwoPolicy : std::uint8_t {
    keep_tight_bounds,
    require_power_of_two_pages,
};

enum class ProductionSpriteAtlasRotationPolicy : std::uint8_t {
    disabled,
};

enum class ProductionSpriteAtlasTextureFormat : std::uint8_t {
    rgba8_unorm,
};

enum class ProductionSpriteAtlasMipPolicy : std::uint8_t {
    base_level_only,
};

enum class ProductionSpriteAtlasPackingDiagnosticCode : std::uint8_t {
    ok,
    empty_items,
    too_many_items,
    zero_dimension,
    dimension_exceeds_limit,
    atlas_exceeds_max_side,
    rgba_byte_size_mismatch,
    invalid_padding,
    invalid_bleed,
    page_count_exceeds_limit,
    unsupported_power_of_two_policy,
    unsupported_rotation_policy,
    unsupported_texture_format,
    unsupported_mip_policy,
};

struct ProductionSpriteAtlasPackingDiagnostic {
    ProductionSpriteAtlasPackingDiagnosticCode code{ProductionSpriteAtlasPackingDiagnosticCode::ok};
    std::string message;
};

struct ProductionSpriteAtlasPackingPolicy {
    std::uint32_t max_side{sprite_atlas_packing_max_side};
    std::uint32_t padding_pixels{2};
    std::uint32_t bleed_pixels{1};
    std::uint32_t max_pages{1};
    ProductionSpriteAtlasPowerOfTwoPolicy power_of_two_policy{ProductionSpriteAtlasPowerOfTwoPolicy::keep_tight_bounds};
    ProductionSpriteAtlasRotationPolicy rotation_policy{ProductionSpriteAtlasRotationPolicy::disabled};
    ProductionSpriteAtlasTextureFormat texture_format{ProductionSpriteAtlasTextureFormat::rgba8_unorm};
    ProductionSpriteAtlasMipPolicy mip_policy{ProductionSpriteAtlasMipPolicy::base_level_only};
};

struct ProductionSpriteAtlasPage {
    std::uint32_t page_index{0};
    TextureSourceDocument atlas;
    ProductionSpriteAtlasPowerOfTwoPolicy power_of_two_policy{ProductionSpriteAtlasPowerOfTwoPolicy::keep_tight_bounds};
    ProductionSpriteAtlasRotationPolicy rotation_policy{ProductionSpriteAtlasRotationPolicy::disabled};
    ProductionSpriteAtlasTextureFormat texture_format{ProductionSpriteAtlasTextureFormat::rgba8_unorm};
    ProductionSpriteAtlasMipPolicy mip_policy{ProductionSpriteAtlasMipPolicy::base_level_only};
    std::uint32_t padding_pixels{0};
    std::uint32_t bleed_pixels{0};
};

struct ProductionSpriteAtlasPlacement {
    std::uint32_t page_index{0};
    std::uint32_t x{0};
    std::uint32_t y{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t padded_x{0};
    std::uint32_t padded_y{0};
    std::uint32_t padded_width{0};
    std::uint32_t padded_height{0};
    bool rotated{false};
};

struct ProductionSpriteAtlasPackingOutput {
    std::vector<ProductionSpriteAtlasPage> pages;
    /// Parallel to the input `items` span (same length, same order).
    std::vector<ProductionSpriteAtlasPlacement> placements;
};

/// Deterministic production sprite atlas packing for already-decoded RGBA8 frames.
/// The first production policy supports explicit padding/bleed, multi-page output,
/// optional power-of-two page expansion, disabled rotation, RGBA8 targets, and
/// base-level-only mips. It does not decode source image files or create runtime
/// renderer/RHI residency.
[[nodiscard]] std::variant<ProductionSpriteAtlasPackingOutput, ProductionSpriteAtlasPackingDiagnostic>
pack_production_sprite_atlas_rgba8(std::span<const SpriteAtlasPackingItemView> items,
                                   const ProductionSpriteAtlasPackingPolicy& policy = {});

} // namespace mirakana
