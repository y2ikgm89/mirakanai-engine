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

namespace {

[[nodiscard]] std::vector<std::uint8_t> solid_rgba8(std::uint32_t width, std::uint32_t height, std::uint8_t red,
                                                    std::uint8_t green, std::uint8_t blue) {
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0xFF);
    for (std::size_t offset = 0; offset < pixels.size(); offset += 4U) {
        pixels[offset + 0U] = red;
        pixels[offset + 1U] = green;
        pixels[offset + 2U] = blue;
        pixels[offset + 3U] = 0xFF;
    }
    return pixels;
}

[[nodiscard]] std::array<std::uint8_t, 4> pixel_at(const mirakana::TextureSourceDocument& texture, std::uint32_t x,
                                                   std::uint32_t y) {
    const auto offset =
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(texture.width) + static_cast<std::size_t>(x)) * 4U;
    return {texture.bytes[offset + 0U], texture.bytes[offset + 1U], texture.bytes[offset + 2U],
            texture.bytes[offset + 3U]};
}

} // namespace

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

MK_TEST("production sprite atlas packing creates deterministic padded power-of-two pages") {
    const auto red = solid_rgba8(3, 3, 0xCC, 0x00, 0x00);
    const auto green = solid_rgba8(3, 3, 0x00, 0xCC, 0x00);
    const auto items = std::array<mirakana::SpriteAtlasPackingItemView, 2>{{
        {.width = 3, .height = 3, .rgba8_pixels = red},
        {.width = 3, .height = 3, .rgba8_pixels = green},
    }};

    const auto policy = mirakana::ProductionSpriteAtlasPackingPolicy{
        .max_side = 8,
        .padding_pixels = 1,
        .bleed_pixels = 1,
        .max_pages = 2,
        .power_of_two_policy = mirakana::ProductionSpriteAtlasPowerOfTwoPolicy::require_power_of_two_pages,
        .rotation_policy = mirakana::ProductionSpriteAtlasRotationPolicy::disabled,
        .texture_format = mirakana::ProductionSpriteAtlasTextureFormat::rgba8_unorm,
        .mip_policy = mirakana::ProductionSpriteAtlasMipPolicy::base_level_only,
    };

    const auto result = mirakana::pack_production_sprite_atlas_rgba8(items, policy);

    MK_REQUIRE(std::holds_alternative<mirakana::ProductionSpriteAtlasPackingOutput>(result));
    const auto& out = std::get<mirakana::ProductionSpriteAtlasPackingOutput>(result);
    MK_REQUIRE(out.pages.size() == 2U);
    MK_REQUIRE(out.placements.size() == 2U);
    MK_REQUIRE(out.pages[0].atlas.width == 8U);
    MK_REQUIRE(out.pages[0].atlas.height == 8U);
    MK_REQUIRE(out.pages[0].texture_format == mirakana::ProductionSpriteAtlasTextureFormat::rgba8_unorm);
    MK_REQUIRE(out.pages[0].mip_policy == mirakana::ProductionSpriteAtlasMipPolicy::base_level_only);
    MK_REQUIRE(out.placements[0].page_index == 0U);
    MK_REQUIRE(out.placements[0].x == 1U);
    MK_REQUIRE(out.placements[0].y == 1U);
    MK_REQUIRE(out.placements[0].width == 3U);
    MK_REQUIRE(out.placements[0].height == 3U);
    MK_REQUIRE(!out.placements[0].rotated);
    MK_REQUIRE(out.placements[1].page_index == 1U);
    const auto expected_red = std::array<std::uint8_t, 4>{0xCC, 0x00, 0x00, 0xFF};
    const auto expected_green = std::array<std::uint8_t, 4>{0x00, 0xCC, 0x00, 0xFF};
    MK_REQUIRE(pixel_at(out.pages[0].atlas, 0, 0) == expected_red);
    MK_REQUIRE(pixel_at(out.pages[1].atlas, 0, 0) == expected_green);
}

MK_TEST("production sprite atlas packing fails closed for invalid bleed and page count") {
    const auto red = solid_rgba8(3, 3, 0xCC, 0x00, 0x00);
    const auto green = solid_rgba8(3, 3, 0x00, 0xCC, 0x00);
    const auto items = std::array<mirakana::SpriteAtlasPackingItemView, 2>{{
        {.width = 3, .height = 3, .rgba8_pixels = red},
        {.width = 3, .height = 3, .rgba8_pixels = green},
    }};

    auto invalid_bleed = mirakana::ProductionSpriteAtlasPackingPolicy{
        .max_side = 8,
        .padding_pixels = 0,
        .bleed_pixels = 1,
        .max_pages = 2,
    };
    const auto invalid_bleed_result = mirakana::pack_production_sprite_atlas_rgba8(items, invalid_bleed);
    MK_REQUIRE(std::holds_alternative<mirakana::ProductionSpriteAtlasPackingDiagnostic>(invalid_bleed_result));
    MK_REQUIRE(std::get<mirakana::ProductionSpriteAtlasPackingDiagnostic>(invalid_bleed_result).code ==
               mirakana::ProductionSpriteAtlasPackingDiagnosticCode::invalid_bleed);

    auto too_few_pages = mirakana::ProductionSpriteAtlasPackingPolicy{
        .max_side = 8,
        .padding_pixels = 1,
        .bleed_pixels = 1,
        .max_pages = 1,
        .power_of_two_policy = mirakana::ProductionSpriteAtlasPowerOfTwoPolicy::require_power_of_two_pages,
    };
    const auto too_few_pages_result = mirakana::pack_production_sprite_atlas_rgba8(items, too_few_pages);
    MK_REQUIRE(std::holds_alternative<mirakana::ProductionSpriteAtlasPackingDiagnostic>(too_few_pages_result));
    MK_REQUIRE(std::get<mirakana::ProductionSpriteAtlasPackingDiagnostic>(too_few_pages_result).code ==
               mirakana::ProductionSpriteAtlasPackingDiagnosticCode::page_count_exceeds_limit);
}

MK_TEST("source asset registry accepts environment profile package rows") {
    const auto key = mirakana::AssetKeyV2{"environment/default_outdoor"};
    mirakana::SourceAssetRegistryDocumentV1 document;
    document.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = key,
        .kind = mirakana::AssetKind::environment_profile,
        .source_path = "source/environment/default_outdoor.environment",
        .source_format = "GameEngine.EnvironmentProfile.v2",
        .imported_path = "runtime/environment/default_outdoor.environment",
        .dependencies = {},
    });

    MK_REQUIRE(mirakana::is_supported_source_asset_kind_v1(mirakana::AssetKind::environment_profile));
    MK_REQUIRE(mirakana::expected_source_asset_format_v1(mirakana::AssetKind::environment_profile) ==
               "GameEngine.EnvironmentProfile.v2");

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
        .content = "format=GameEngine.CookedEnvironmentProfile.v2\nasset.kind=environment_profile\n",
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

MK_TEST("cooked package index records environment texture dependency edges") {
    const auto environment = mirakana::AssetId::from_name("environment/default_outdoor");
    const auto radiance = mirakana::AssetId::from_name("environment/textures/studio_radiance_exr");
    const auto skybox = mirakana::AssetId::from_name("environment/textures/studio_skybox_basis");
    std::vector<mirakana::AssetCookedArtifact> artifacts;
    artifacts.push_back(mirakana::AssetCookedArtifact{
        .asset = environment,
        .kind = mirakana::AssetKind::environment_profile,
        .path = "runtime/environment/default_outdoor.geenv",
        .content = "format=GameEngine.CookedEnvironmentProfile.v2\nasset.kind=environment_profile\n",
        .source_revision = 1U,
        .dependencies = {radiance, skybox},
    });
    artifacts.push_back(mirakana::AssetCookedArtifact{
        .asset = radiance,
        .kind = mirakana::AssetKind::texture,
        .path = "runtime/environment/studio_radiance.texture.geasset",
        .content = "format=GameEngine.EnvironmentTextureGeassetMetadata.v1\nasset.kind=environment_texture\n",
        .source_revision = 1U,
        .dependencies = {},
    });
    artifacts.push_back(mirakana::AssetCookedArtifact{
        .asset = skybox,
        .kind = mirakana::AssetKind::texture,
        .path = "runtime/environment/studio_skybox.texture.geasset",
        .content = "format=GameEngine.EnvironmentTextureGeassetMetadata.v1\nasset.kind=environment_texture\n",
        .source_revision = 1U,
        .dependencies = {},
    });

    const auto index = mirakana::build_asset_cooked_package_index(
        std::move(artifacts), {mirakana::AssetDependencyEdge{
                                   .asset = environment,
                                   .dependency = radiance,
                                   .kind = mirakana::AssetDependencyKind::environment_texture,
                                   .path = "runtime/environment/default_outdoor.geenv",
                               },
                               mirakana::AssetDependencyEdge{
                                   .asset = environment,
                                   .dependency = skybox,
                                   .kind = mirakana::AssetDependencyKind::environment_texture,
                                   .path = "runtime/environment/default_outdoor.geenv",
                               }});
    const auto serialized = mirakana::serialize_asset_cooked_package_index(index);
    MK_REQUIRE(serialized.find("dependency.0.kind=environment_texture\n") != std::string::npos);
    MK_REQUIRE(serialized.find("dependency.1.kind=environment_texture\n") != std::string::npos);

    const auto parsed = mirakana::deserialize_asset_cooked_package_index(serialized);
    MK_REQUIRE(parsed.dependencies.size() == 2U);
    MK_REQUIRE(parsed.dependencies[0].kind == mirakana::AssetDependencyKind::environment_texture);
    MK_REQUIRE(parsed.dependencies[1].kind == mirakana::AssetDependencyKind::environment_texture);
}

MK_TEST("cooked package index accepts environment preset pack assets") {
    const auto preset_pack = mirakana::AssetId::from_name("environment/presets/sample_commercial_pack");
    std::vector<mirakana::AssetCookedArtifact> artifacts;
    artifacts.push_back(mirakana::AssetCookedArtifact{
        .asset = preset_pack,
        .kind = mirakana::AssetKind::environment_preset_pack,
        .path = "runtime/assets/desktop_runtime/environment_presets.gepresetpack",
        .content = "format=GameEngine.EnvironmentPresetPack.v1\n"
                   "pack.id=sample_environment_commercial_presets\n"
                   "pack.provenance_id=provenance.environment.sample_commercial_presets\n"
                   "pack.license_id=LicenseRef-Proprietary\n",
        .source_revision = 1U,
        .dependencies = {},
    });

    const auto index = mirakana::build_asset_cooked_package_index(std::move(artifacts), {});
    MK_REQUIRE(index.entries.size() == 1U);
    MK_REQUIRE(index.entries[0].kind == mirakana::AssetKind::environment_preset_pack);

    const auto serialized = mirakana::serialize_asset_cooked_package_index(index);
    MK_REQUIRE(serialized.find("entry.0.kind=environment_preset_pack\n") != std::string::npos);
    const auto parsed = mirakana::deserialize_asset_cooked_package_index(serialized);
    MK_REQUIRE(parsed.entries.size() == 1U);
    MK_REQUIRE(parsed.entries[0].kind == mirakana::AssetKind::environment_preset_pack);
}

int main() {
    return mirakana::test::run_all();
}
