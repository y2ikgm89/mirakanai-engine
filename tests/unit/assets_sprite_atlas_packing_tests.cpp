// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/sprite_atlas_packing.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <variant>
#include <vector>

MK_TEST("sprite atlas packing rejects empty input") {
    const std::vector<mirakana::SpriteAtlasPackingItemView> items;
    const auto result = mirakana::pack_sprite_atlas_rgba8_max_side(items);
    MK_REQUIRE(std::holds_alternative<mirakana::SpriteAtlasPackingDiagnostic>(result));
    const auto& diag = std::get<mirakana::SpriteAtlasPackingDiagnostic>(result);
    MK_REQUIRE(diag.code == mirakana::SpriteAtlasPackingDiagnosticCode::empty_items);
}

MK_TEST("sprite atlas packing rejects rgba size mismatch") {
    std::vector<std::uint8_t> bad(3, 0xFF);
    const std::vector<mirakana::SpriteAtlasPackingItemView> items{{.width = 1, .height = 1, .rgba8_pixels = bad}};
    const auto result = mirakana::pack_sprite_atlas_rgba8_max_side(items);
    MK_REQUIRE(std::holds_alternative<mirakana::SpriteAtlasPackingDiagnostic>(result));
    MK_REQUIRE(std::get<mirakana::SpriteAtlasPackingDiagnostic>(result).code ==
               mirakana::SpriteAtlasPackingDiagnosticCode::rgba_byte_size_mismatch);
}

MK_TEST("sprite atlas packing places two 1x1 sprites side by side") {
    constexpr auto px_a = std::to_array<std::uint8_t>({0xFF, 0x00, 0x00, 0xFF});
    constexpr auto px_b = std::to_array<std::uint8_t>({0x00, 0xFF, 0x00, 0xFF});
    const auto items = std::array<mirakana::SpriteAtlasPackingItemView, 2>{{
        {.width = 1, .height = 1, .rgba8_pixels = px_a},
        {.width = 1, .height = 1, .rgba8_pixels = px_b},
    }};
    const auto result = mirakana::pack_sprite_atlas_rgba8_max_side(items);
    MK_REQUIRE(std::holds_alternative<mirakana::SpriteAtlasRgba8PackingOutput>(result));
    const auto& out = std::get<mirakana::SpriteAtlasRgba8PackingOutput>(result);
    MK_REQUIRE(out.atlas.width == 2);
    MK_REQUIRE(out.atlas.height == 1);
    MK_REQUIRE(out.atlas.pixel_format == mirakana::TextureSourcePixelFormat::rgba8_unorm);
    MK_REQUIRE(out.atlas.bytes.size() == 8);
    MK_REQUIRE(out.placements.size() == 2);
    MK_REQUIRE(out.placements[0].x == 0 && out.placements[0].y == 0);
    MK_REQUIRE(out.placements[1].x == 1 && out.placements[1].y == 0);
    MK_REQUIRE(out.atlas.bytes[0] == 0xFF && out.atlas.bytes[1] == 0x00);
    MK_REQUIRE(out.atlas.bytes[4] == 0x00 && out.atlas.bytes[5] == 0xFF);
}

int main() {
    return mirakana::test::run_all();
}
