// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/sprite_atlas_packing.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <numeric>
#include <span>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] SpriteAtlasPackingDiagnostic make_diagnostic(SpriteAtlasPackingDiagnosticCode code, std::string message) {
    return SpriteAtlasPackingDiagnostic{.code = code, .message = std::move(message)};
}

[[nodiscard]] bool multiply_u32(std::uint32_t a, std::uint32_t b, std::uint32_t& out) {
    const auto prod = static_cast<std::uint64_t>(a) * static_cast<std::uint64_t>(b);
    if (prod > static_cast<std::uint64_t>((std::numeric_limits<std::uint32_t>::max)())) {
        return false;
    }
    out = static_cast<std::uint32_t>(prod);
    return true;
}

[[nodiscard]] bool multiply_u32_u32_u8(std::uint32_t w, std::uint32_t h, std::size_t& out_bytes) {
    std::uint32_t wh = 0;
    if (!multiply_u32(w, h, wh)) {
        return false;
    }
    const auto bytes64 = static_cast<std::uint64_t>(wh) * 4ULL;
    if (bytes64 > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
        return false;
    }
    out_bytes = static_cast<std::size_t>(bytes64);
    return true;
}

void blit_rgba8_rect(std::vector<std::uint8_t>& atlas, std::uint32_t atlas_width,
                     const SpriteAtlasPackedPlacement& where, std::uint32_t src_width,
                     std::span<const std::uint8_t> rgba) {
    for (std::uint32_t row = 0; row < where.height; ++row) {
        const auto src_offset = static_cast<std::size_t>(row) * static_cast<std::size_t>(src_width) * 4U;
        const auto dst_offset = (static_cast<std::size_t>(where.y + row) * static_cast<std::size_t>(atlas_width) +
                                 static_cast<std::size_t>(where.x)) *
                                4U;
        const auto src = std::span<const std::uint8_t>{rgba}.subspan(src_offset);
        auto dst = std::span<std::uint8_t>{atlas}.subspan(dst_offset);
        std::memcpy(dst.data(), src.data(), static_cast<std::size_t>(where.width) * 4U);
    }
}

[[nodiscard]] ProductionSpriteAtlasPackingDiagnostic
make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode code, std::string message) {
    return ProductionSpriteAtlasPackingDiagnostic{.code = code, .message = std::move(message)};
}

[[nodiscard]] bool is_power_of_two_policy_valid(ProductionSpriteAtlasPowerOfTwoPolicy policy) noexcept {
    return policy == ProductionSpriteAtlasPowerOfTwoPolicy::keep_tight_bounds ||
           policy == ProductionSpriteAtlasPowerOfTwoPolicy::require_power_of_two_pages;
}

[[nodiscard]] bool is_rotation_policy_valid(ProductionSpriteAtlasRotationPolicy policy) noexcept {
    return policy == ProductionSpriteAtlasRotationPolicy::disabled;
}

[[nodiscard]] bool is_texture_format_valid(ProductionSpriteAtlasTextureFormat format) noexcept {
    return format == ProductionSpriteAtlasTextureFormat::rgba8_unorm;
}

[[nodiscard]] bool is_mip_policy_valid(ProductionSpriteAtlasMipPolicy policy) noexcept {
    return policy == ProductionSpriteAtlasMipPolicy::base_level_only;
}

[[nodiscard]] bool add_u32(std::uint32_t a, std::uint32_t b, std::uint32_t& out) noexcept {
    const auto sum = static_cast<std::uint64_t>(a) + static_cast<std::uint64_t>(b);
    if (sum > static_cast<std::uint64_t>((std::numeric_limits<std::uint32_t>::max)())) {
        return false;
    }
    out = static_cast<std::uint32_t>(sum);
    return true;
}

[[nodiscard]] bool padded_extent(std::uint32_t extent, std::uint32_t padding, std::uint32_t& out) noexcept {
    std::uint32_t doubled_padding = 0;
    if (!multiply_u32(padding, 2U, doubled_padding)) {
        return false;
    }
    return add_u32(extent, doubled_padding, out);
}

[[nodiscard]] std::uint32_t next_power_of_two(std::uint32_t value) noexcept {
    if (value <= 1U) {
        return 1U;
    }
    --value;
    value |= value >> 1U;
    value |= value >> 2U;
    value |= value >> 4U;
    value |= value >> 8U;
    value |= value >> 16U;
    return value + 1U;
}

[[nodiscard]] std::uint32_t production_page_side(std::uint32_t used_side,
                                                 ProductionSpriteAtlasPowerOfTwoPolicy policy) noexcept {
    if (policy == ProductionSpriteAtlasPowerOfTwoPolicy::require_power_of_two_pages) {
        return next_power_of_two(used_side);
    }
    return used_side;
}

struct OrderedProductionSpriteAtlasItem {
    std::size_t source_index{0};
    std::uint32_t padded_width{0};
    std::uint32_t padded_height{0};
};

struct ProductionSpriteAtlasPageBuild {
    std::vector<std::size_t> source_indices;
    std::uint32_t pen_x{0};
    std::uint32_t pen_y{0};
    std::uint32_t line_height{0};
    std::uint32_t used_width{0};
    std::uint32_t used_height{0};
};

[[nodiscard]] ProductionSpriteAtlasPackingDiagnostic
validate_production_request(std::span<const SpriteAtlasPackingItemView> items,
                            const ProductionSpriteAtlasPackingPolicy& policy,
                            std::vector<OrderedProductionSpriteAtlasItem>& ordered_items) {
    if (policy.max_side == 0U || policy.max_pages == 0U) {
        return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::zero_dimension,
                                          "production sprite atlas max_side and max_pages must be non-zero");
    }
    if (policy.max_side > sprite_atlas_packing_max_side) {
        return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::dimension_exceeds_limit,
                                          "production sprite atlas max_side exceeds the supported limit");
    }
    if (!is_power_of_two_policy_valid(policy.power_of_two_policy)) {
        return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::unsupported_power_of_two_policy,
                                          "production sprite atlas power-of-two policy is unsupported");
    }
    if (!is_rotation_policy_valid(policy.rotation_policy)) {
        return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::unsupported_rotation_policy,
                                          "production sprite atlas rotation policy is unsupported");
    }
    if (!is_texture_format_valid(policy.texture_format)) {
        return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::unsupported_texture_format,
                                          "production sprite atlas texture format is unsupported");
    }
    if (!is_mip_policy_valid(policy.mip_policy)) {
        return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::unsupported_mip_policy,
                                          "production sprite atlas mip policy is unsupported");
    }
    if (policy.padding_pixels > policy.max_side / 2U) {
        return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::invalid_padding,
                                          "production sprite atlas padding exceeds half of max_side");
    }
    if (policy.bleed_pixels > policy.padding_pixels) {
        return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::invalid_bleed,
                                          "production sprite atlas bleed must be less than or equal to padding");
    }
    if (items.empty()) {
        return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::empty_items,
                                          "production sprite atlas packing requires at least one sprite");
    }
    if (items.size() > sprite_atlas_packing_max_items) {
        return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::too_many_items,
                                          "production sprite atlas sprite count exceeds the packing limit");
    }

    ordered_items.reserve(items.size());
    for (std::size_t index = 0; index < items.size(); ++index) {
        const auto& item = items[index];
        if (item.width == 0U || item.height == 0U) {
            return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::zero_dimension,
                                              "production sprite atlas dimensions must be non-zero");
        }

        std::uint32_t padded_width = 0;
        std::uint32_t padded_height = 0;
        if (!padded_extent(item.width, policy.padding_pixels, padded_width) ||
            !padded_extent(item.height, policy.padding_pixels, padded_height)) {
            return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::dimension_exceeds_limit,
                                              "production sprite atlas padded dimensions overflow");
        }
        if (padded_width > policy.max_side || padded_height > policy.max_side) {
            return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::dimension_exceeds_limit,
                                              "production sprite atlas padded dimensions exceed max_side");
        }

        std::size_t expected = 0;
        if (!multiply_u32_u32_u8(item.width, item.height, expected) || item.rgba8_pixels.size() != expected) {
            return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::rgba_byte_size_mismatch,
                                              "production sprite atlas rgba8_pixels size must equal width*height*4");
        }
        ordered_items.push_back(OrderedProductionSpriteAtlasItem{
            .source_index = index,
            .padded_width = padded_width,
            .padded_height = padded_height,
        });
    }

    std::ranges::sort(ordered_items, [](const auto& lhs, const auto& rhs) {
        if (lhs.padded_height != rhs.padded_height) {
            return lhs.padded_height > rhs.padded_height;
        }
        if (lhs.padded_width != rhs.padded_width) {
            return lhs.padded_width > rhs.padded_width;
        }
        return lhs.source_index < rhs.source_index;
    });
    return {};
}

[[nodiscard]] bool production_item_fits_current_page(const ProductionSpriteAtlasPageBuild& page,
                                                     std::uint32_t padded_height, std::uint32_t max_side) noexcept {
    return static_cast<std::uint64_t>(page.pen_y) +
               static_cast<std::uint64_t>((std::max)(page.line_height, padded_height)) <=
           static_cast<std::uint64_t>(max_side);
}

[[nodiscard]] bool production_item_fits_current_row(const ProductionSpriteAtlasPageBuild& page,
                                                    std::uint32_t padded_width, std::uint32_t max_side) noexcept {
    return static_cast<std::uint64_t>(page.pen_x) + static_cast<std::uint64_t>(padded_width) <=
           static_cast<std::uint64_t>(max_side);
}

void start_next_production_row(ProductionSpriteAtlasPageBuild& page) noexcept {
    page.pen_y += page.line_height;
    page.pen_x = 0U;
    page.line_height = 0U;
}

void blit_production_rgba8_rect(TextureSourceDocument& atlas, const SpriteAtlasPackingItemView& item,
                                const ProductionSpriteAtlasPlacement& where) {
    for (std::uint32_t padded_y = 0; padded_y < where.padded_height; ++padded_y) {
        const auto src_y = padded_y < where.y - where.padded_y
                               ? 0U
                               : (std::min)(padded_y - (where.y - where.padded_y), item.height - 1U);
        for (std::uint32_t padded_x = 0; padded_x < where.padded_width; ++padded_x) {
            const auto src_x = padded_x < where.x - where.padded_x
                                   ? 0U
                                   : (std::min)(padded_x - (where.x - where.padded_x), item.width - 1U);
            const auto src_offset = (static_cast<std::size_t>(src_y) * static_cast<std::size_t>(item.width) +
                                     static_cast<std::size_t>(src_x)) *
                                    4U;
            const auto dst_offset =
                (static_cast<std::size_t>(where.padded_y + padded_y) * static_cast<std::size_t>(atlas.width) +
                 static_cast<std::size_t>(where.padded_x + padded_x)) *
                4U;
            atlas.bytes[dst_offset + 0U] = item.rgba8_pixels[src_offset + 0U];
            atlas.bytes[dst_offset + 1U] = item.rgba8_pixels[src_offset + 1U];
            atlas.bytes[dst_offset + 2U] = item.rgba8_pixels[src_offset + 2U];
            atlas.bytes[dst_offset + 3U] = item.rgba8_pixels[src_offset + 3U];
        }
    }
}

} // namespace

std::variant<SpriteAtlasRgba8PackingOutput, SpriteAtlasPackingDiagnostic>
pack_sprite_atlas_rgba8_max_side(std::span<const SpriteAtlasPackingItemView> items, std::uint32_t max_side) {
    if (max_side == 0) {
        return make_diagnostic(SpriteAtlasPackingDiagnosticCode::zero_dimension, "max_side must be non-zero");
    }
    if (items.empty()) {
        return make_diagnostic(SpriteAtlasPackingDiagnosticCode::empty_items,
                               "sprite atlas packing requires at least one sprite");
    }
    if (items.size() > sprite_atlas_packing_max_items) {
        return make_diagnostic(SpriteAtlasPackingDiagnosticCode::too_many_items, "sprite count exceeds packing limit");
    }

    for (const auto& it : items) {
        if (it.width == 0 || it.height == 0) {
            return make_diagnostic(SpriteAtlasPackingDiagnosticCode::zero_dimension,
                                   "sprite dimensions must be non-zero");
        }
        if (it.width > max_side || it.height > max_side) {
            return make_diagnostic(SpriteAtlasPackingDiagnosticCode::dimension_exceeds_limit,
                                   "sprite dimensions exceed max_side");
        }
        std::size_t expected = 0;
        if (!multiply_u32_u32_u8(it.width, it.height, expected)) {
            return make_diagnostic(SpriteAtlasPackingDiagnosticCode::rgba_byte_size_mismatch,
                                   "sprite rgba byte size overflows");
        }
        if (it.rgba8_pixels.size() != expected) {
            return make_diagnostic(SpriteAtlasPackingDiagnosticCode::rgba_byte_size_mismatch,
                                   "sprite rgba8_pixels size must equal width*height*4");
        }
    }

    std::vector<std::size_t> order(items.size());
    // NOLINTNEXTLINE(modernize-use-ranges): hosted Clang/AppleClang CI lacks std::ranges::iota.
    std::iota(order.begin(), order.end(), 0U);
    std::ranges::sort(order, [&](std::size_t a, std::size_t b) {
        const auto& pa = items[a];
        const auto& pb = items[b];
        if (pa.height != pb.height) {
            return pa.height > pb.height;
        }
        if (pa.width != pb.width) {
            return pa.width > pb.width;
        }
        return a < b;
    });

    std::vector<SpriteAtlasPackedPlacement> placements(items.size());
    std::uint32_t pen_x = 0;
    std::uint32_t pen_y = 0;
    std::uint32_t line_h = 0;
    std::uint32_t atlas_w = 0;
    std::uint32_t atlas_h = 0;

    for (const std::size_t src_index : order) {
        const auto& it = items[src_index];
        const auto w = it.width;
        const auto h = it.height;

        if (pen_x > 0 &&
            static_cast<std::uint64_t>(pen_x) + static_cast<std::uint64_t>(w) > static_cast<std::uint64_t>(max_side)) {
            pen_y += line_h;
            pen_x = 0;
            line_h = 0;
        }
        if (static_cast<std::uint64_t>(pen_x) + static_cast<std::uint64_t>(w) > static_cast<std::uint64_t>(max_side)) {
            return make_diagnostic(SpriteAtlasPackingDiagnosticCode::atlas_exceeds_max_side,
                                   "packed atlas width would exceed max_side");
        }
        if (static_cast<std::uint64_t>(pen_y) + static_cast<std::uint64_t>((std::max)(line_h, h)) >
            static_cast<std::uint64_t>(max_side)) {
            return make_diagnostic(SpriteAtlasPackingDiagnosticCode::atlas_exceeds_max_side,
                                   "packed atlas height would exceed max_side");
        }

        placements[src_index] = SpriteAtlasPackedPlacement{.x = pen_x, .y = pen_y, .width = w, .height = h};
        pen_x += w;
        line_h = (std::max)(line_h, h);
        atlas_w = (std::max)(atlas_w, pen_x);
        atlas_h = (std::max)(atlas_h, pen_y + line_h);
    }

    std::size_t atlas_bytes = 0;
    if (!multiply_u32_u32_u8(atlas_w, atlas_h, atlas_bytes)) {
        return make_diagnostic(SpriteAtlasPackingDiagnosticCode::atlas_exceeds_max_side,
                               "atlas rgba byte size overflows");
    }

    std::vector<std::uint8_t> atlas_pixels(atlas_bytes, 0U);
    for (std::size_t i = 0; i < items.size(); ++i) {
        blit_rgba8_rect(atlas_pixels, atlas_w, placements[i], items[i].width, items[i].rgba8_pixels);
    }

    SpriteAtlasRgba8PackingOutput out;
    out.atlas.width = atlas_w;
    out.atlas.height = atlas_h;
    out.atlas.pixel_format = TextureSourcePixelFormat::rgba8_unorm;
    out.atlas.bytes = std::move(atlas_pixels);
    out.placements = std::move(placements);
    return out;
}

std::variant<ProductionSpriteAtlasPackingOutput, ProductionSpriteAtlasPackingDiagnostic>
pack_production_sprite_atlas_rgba8(std::span<const SpriteAtlasPackingItemView> items,
                                   const ProductionSpriteAtlasPackingPolicy& policy) {
    std::vector<OrderedProductionSpriteAtlasItem> ordered_items;
    auto diagnostic = validate_production_request(items, policy, ordered_items);
    if (diagnostic.code != ProductionSpriteAtlasPackingDiagnosticCode::ok) {
        return diagnostic;
    }

    ProductionSpriteAtlasPackingOutput output;
    output.placements.resize(items.size());
    std::vector<ProductionSpriteAtlasPageBuild> page_builds;
    page_builds.emplace_back();

    for (const auto& ordered : ordered_items) {
        auto* page = &page_builds.back();
        if (page->pen_x > 0U && !production_item_fits_current_row(*page, ordered.padded_width, policy.max_side)) {
            start_next_production_row(*page);
        }
        if (!production_item_fits_current_page(*page, ordered.padded_height, policy.max_side)) {
            if (page_builds.size() >= policy.max_pages) {
                return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::page_count_exceeds_limit,
                                                  "production sprite atlas needs more pages than max_pages");
            }
            page_builds.emplace_back();
            page = &page_builds.back();
        }

        const auto page_index = static_cast<std::uint32_t>(page_builds.size() - 1U);
        const auto x = page->pen_x + policy.padding_pixels;
        const auto y = page->pen_y + policy.padding_pixels;
        const auto& item = items[ordered.source_index];
        output.placements[ordered.source_index] = ProductionSpriteAtlasPlacement{
            .page_index = page_index,
            .x = x,
            .y = y,
            .width = item.width,
            .height = item.height,
            .padded_x = page->pen_x,
            .padded_y = page->pen_y,
            .padded_width = ordered.padded_width,
            .padded_height = ordered.padded_height,
            .rotated = false,
        };
        page->source_indices.push_back(ordered.source_index);
        page->pen_x += ordered.padded_width;
        page->line_height = (std::max)(page->line_height, ordered.padded_height);
        page->used_width = (std::max)(page->used_width, page->pen_x);
        page->used_height = (std::max)(page->used_height, page->pen_y + page->line_height);
    }

    output.pages.reserve(page_builds.size());
    for (std::size_t page_index = 0; page_index < page_builds.size(); ++page_index) {
        const auto& build = page_builds[page_index];
        const auto atlas_width = production_page_side(build.used_width, policy.power_of_two_policy);
        const auto atlas_height = production_page_side(build.used_height, policy.power_of_two_policy);
        if (atlas_width > policy.max_side || atlas_height > policy.max_side) {
            return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::atlas_exceeds_max_side,
                                              "production sprite atlas power-of-two page exceeds max_side");
        }

        std::size_t atlas_bytes = 0;
        if (!multiply_u32_u32_u8(atlas_width, atlas_height, atlas_bytes)) {
            return make_production_diagnostic(ProductionSpriteAtlasPackingDiagnosticCode::atlas_exceeds_max_side,
                                              "production sprite atlas rgba byte size overflows");
        }

        ProductionSpriteAtlasPage page;
        page.page_index = static_cast<std::uint32_t>(page_index);
        page.atlas.width = atlas_width;
        page.atlas.height = atlas_height;
        page.atlas.pixel_format = TextureSourcePixelFormat::rgba8_unorm;
        page.atlas.bytes.resize(atlas_bytes, 0U);
        page.power_of_two_policy = policy.power_of_two_policy;
        page.rotation_policy = policy.rotation_policy;
        page.texture_format = policy.texture_format;
        page.mip_policy = policy.mip_policy;
        page.padding_pixels = policy.padding_pixels;
        page.bleed_pixels = policy.bleed_pixels;

        for (const auto source_index : build.source_indices) {
            blit_production_rgba8_rect(page.atlas, items[source_index], output.placements[source_index]);
        }
        output.pages.push_back(std::move(page));
    }

    return output;
}

} // namespace mirakana
