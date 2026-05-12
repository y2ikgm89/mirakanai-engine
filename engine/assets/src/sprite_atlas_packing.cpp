// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/sprite_atlas_packing.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <numeric>
#include <span>

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
    std::ranges::iota(order, 0U);
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

} // namespace mirakana
