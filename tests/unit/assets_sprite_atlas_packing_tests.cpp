// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/source_asset_registry.hpp"
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

MK_TEST("source asset registry accepts environment profile package rows") {
    const auto key = mirakana::AssetKeyV2{"environment/default_outdoor"};
    mirakana::SourceAssetRegistryDocumentV1 document;
    document.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = key,
        .kind = mirakana::AssetKind::environment_profile,
        .source_path = "source/environment/default_outdoor.environment",
        .source_format = "GameEngine.EnvironmentProfile.v1",
        .imported_path = "runtime/environment/default_outdoor.environment",
        .dependencies = {},
    });

    MK_REQUIRE(mirakana::is_supported_source_asset_kind_v1(mirakana::AssetKind::environment_profile));
    MK_REQUIRE(mirakana::expected_source_asset_format_v1(mirakana::AssetKind::environment_profile) ==
               "GameEngine.EnvironmentProfile.v1");

    const auto serialized = mirakana::serialize_source_asset_registry_document(document);
    MK_REQUIRE(serialized.find("asset.0.kind=environment_profile\n") != std::string::npos);
    const auto parsed = mirakana::deserialize_source_asset_registry_document(serialized);
    MK_REQUIRE(parsed.assets.size() == 1U);
    MK_REQUIRE(parsed.assets[0].kind == mirakana::AssetKind::environment_profile);

    const auto import_metadata = mirakana::build_source_asset_import_metadata_registry(parsed);
    MK_REQUIRE(import_metadata.environment_profile_count() == 1U);
    const auto plan = mirakana::build_asset_import_plan(import_metadata);
    MK_REQUIRE(plan.actions.size() == 1U);
    MK_REQUIRE(plan.actions[0].kind == mirakana::AssetImportActionKind::environment_profile);
    MK_REQUIRE(plan.actions[0].output_path == "runtime/environment/default_outdoor.environment");
}

MK_TEST("cooked package index accepts environment profile assets") {
    const auto asset = mirakana::AssetId::from_name("environment/default_outdoor");
    std::vector<mirakana::AssetCookedArtifact> artifacts;
    artifacts.push_back(mirakana::AssetCookedArtifact{
        .asset = asset,
        .kind = mirakana::AssetKind::environment_profile,
        .path = "runtime/environment/default_outdoor.environment",
        .content = "format=GameEngine.CookedEnvironmentProfile.v1\nasset.kind=environment_profile\n",
        .source_revision = 1U,
        .dependencies = {},
    });

    const auto index = mirakana::build_asset_cooked_package_index(std::move(artifacts), {});
    MK_REQUIRE(index.entries.size() == 1U);
    MK_REQUIRE(index.entries[0].kind == mirakana::AssetKind::environment_profile);

    const auto serialized = mirakana::serialize_asset_cooked_package_index(index);
    MK_REQUIRE(serialized.find("entry.0.kind=environment_profile\n") != std::string::npos);
    const auto parsed = mirakana::deserialize_asset_cooked_package_index(serialized);
    MK_REQUIRE(parsed.entries.size() == 1U);
    MK_REQUIRE(parsed.entries[0].kind == mirakana::AssetKind::environment_profile);
}

int main() {
    return mirakana::test::run_all();
}
