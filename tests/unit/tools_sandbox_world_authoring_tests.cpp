// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/tools/sandbox_world_authoring.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::AssetId asset_id(std::string value) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::move(value)});
}

[[nodiscard]] mirakana::CookedTilemapAuthoringDesc make_tilemap_desc() {
    mirakana::CookedTilemapAuthoringDesc desc;
    desc.tilemap_asset = asset_id("sample_2d_desktop_runtime_package/tilemaps/seed");
    desc.output_path = "runtime/assets/worlds/seed.tilemap";
    desc.source_revision = 42;
    desc.atlas_page = asset_id("sample_2d_desktop_runtime_package/textures/terrain");
    desc.atlas_page_uri = "runtime/assets/textures/terrain.texture";
    desc.tile_width = 16;
    desc.tile_height = 16;
    desc.tiles.push_back(mirakana::CookedTilemapTileDesc{
        .id = "grass",
        .page = desc.atlas_page,
        .u0 = 0.0F,
        .v0 = 0.0F,
        .u1 = 0.25F,
        .v1 = 0.25F,
        .color = {1.0F, 1.0F, 1.0F, 1.0F},
    });
    desc.tiles.push_back(mirakana::CookedTilemapTileDesc{
        .id = "water",
        .page = desc.atlas_page,
        .u0 = 0.25F,
        .v0 = 0.0F,
        .u1 = 0.5F,
        .v1 = 0.25F,
        .color = {0.7F, 0.8F, 1.0F, 1.0F},
    });
    desc.layers.push_back(mirakana::CookedTilemapLayerDesc{
        .name = "terrain",
        .width = 2,
        .height = 2,
        .visible = true,
        .cells = {"grass", "water", "grass", ""},
    });
    return desc;
}

[[nodiscard]] std::string base_package_index_content(const mirakana::CookedTilemapAuthoringDesc& desc) {
    return mirakana::serialize_asset_cooked_package_index(mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{
                .asset = desc.atlas_page,
                .kind = mirakana::AssetKind::texture,
                .path = desc.atlas_page_uri,
                .content = "format=GameEngine.CookedTexture.v1\nasset.id=terrain\n",
                .source_revision = 7,
                .dependencies = {},
            },
        },
        {}));
}

[[nodiscard]] mirakana::SandboxWorldAuthoringPackageDesc make_authoring_package_desc() {
    const auto tilemap = make_tilemap_desc();
    mirakana::SandboxWorldAuthoringPackageDesc package;
    package.review_document_path = "runtime/assets/worlds/seed.sandbox_authoring";
    package.tilemap = tilemap;
    package.tile_definitions.push_back(mirakana::SandboxWorldTileDefinitionRow{
        .tile_id = "grass",
        .atlas_frame_id = "terrain/grass",
        .atlas_page = tilemap.atlas_page,
        .u0 = 0.0F,
        .v0 = 0.0F,
        .u1 = 0.25F,
        .v1 = 0.25F,
        .collision_kind = mirakana::SandboxWorldTileCollisionKind::solid,
        .material_tags = {"soil", "walkable"},
        .emits_light = false,
        .liquid = false,
        .update_policy = mirakana::SandboxWorldTileUpdatePolicy::none,
        .localization_key = "tile.grass.name",
        .accessibility_label_key = "tile.grass.accessibility",
        .provenance = "first-party",
        .license_id = "LicenseRef-Proprietary",
        .source_index = 0,
    });
    package.tile_definitions.push_back(mirakana::SandboxWorldTileDefinitionRow{
        .tile_id = "water",
        .atlas_frame_id = "terrain/water",
        .atlas_page = tilemap.atlas_page,
        .u0 = 0.25F,
        .v0 = 0.0F,
        .u1 = 0.5F,
        .v1 = 0.25F,
        .collision_kind = mirakana::SandboxWorldTileCollisionKind::liquid,
        .material_tags = {"liquid", "swimmable"},
        .emits_light = false,
        .liquid = true,
        .update_policy = mirakana::SandboxWorldTileUpdatePolicy::scheduled,
        .localization_key = "tile.water.name",
        .accessibility_label_key = "tile.water.accessibility",
        .provenance = "first-party",
        .license_id = "LicenseRef-Proprietary",
        .source_index = 1,
    });
    package.palette_brushes.push_back(mirakana::SandboxWorldPaletteBrushRow{
        .brush_id = "paint/grass",
        .shape = mirakana::SandboxWorldBrushShape::single_cell,
        .layer_mask = 1,
        .tile_id = "grass",
        .replacement_policy = mirakana::SandboxWorldBrushReplacementPolicy::any,
        .symmetry = mirakana::SandboxWorldBrushSymmetry::mirror_x,
        .fill_policy = mirakana::SandboxWorldBrushFillPolicy::paint,
        .preview_path = "runtime/assets/worlds/brushes/grass.brush",
        .source_index = 0,
    });
    package.chunk_templates.push_back(mirakana::SandboxWorldChunkTemplateRow{
        .template_id = "spawn/meadow",
        .width = 16,
        .height = 16,
        .allowed_tile_ids = {"grass", "water"},
        .object_placement_rules = {mirakana::SandboxWorldObjectPlacementRuleRow{
            .object_id = "object/tree",
            .anchor_tile_id = "grass",
            .max_per_chunk = 4,
            .spawn_weight = 3,
        }},
        .source_index = 0,
    });
    package.procedural_seeds.push_back(mirakana::SandboxWorldProceduralSeedRow{
        .generator_id = "generator/meadow",
        .seed = 12345,
        .width = 32,
        .height = 16,
        .allowed_tile_ids = {"grass", "water"},
        .object_placement_rules = {mirakana::SandboxWorldObjectPlacementRuleRow{
            .object_id = "object/ore",
            .anchor_tile_id = "grass",
            .max_per_chunk = 2,
            .spawn_weight = 1,
        }},
        .source_index = 0,
    });
    return package;
}

[[nodiscard]] mirakana::SandboxWorldAuthoringReviewDesc make_review_desc() {
    auto package = make_authoring_package_desc();
    auto tilemap = package.tilemap;
    return mirakana::SandboxWorldAuthoringReviewDesc{
        .package_index_path = "runtime/sample_2d_desktop_runtime_package.geindex",
        .package_index_content = base_package_index_content(tilemap),
        .package = std::move(package),
    };
}

[[nodiscard]] bool has_diagnostic(const mirakana::SandboxWorldAuthoringReviewResult& result,
                                  mirakana::SandboxWorldAuthoringDiagnosticCode code) {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

class ThrowingWriteFileSystem final : public mirakana::IFileSystem {
  public:
    explicit ThrowingWriteFileSystem(std::string package_index_content) {
        files_.write_text("runtime/sample_2d_desktop_runtime_package.geindex", package_index_content);
    }

    [[nodiscard]] bool exists(std::string_view path) const override {
        return files_.exists(path);
    }

    [[nodiscard]] bool is_directory(std::string_view path) const override {
        return files_.is_directory(path);
    }

    [[nodiscard]] std::string read_text(std::string_view path) const override {
        return files_.read_text(path);
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view root) const override {
        return files_.list_files(root);
    }

    void write_text(std::string_view /*path*/, std::string_view /*text*/) override {
        throw std::runtime_error("write denied");
    }

    void remove(std::string_view path) override {
        files_.remove(path);
    }

    void remove_empty_directory(std::string_view path) override {
        files_.remove_empty_directory(path);
    }

  private:
    mirakana::MemoryFileSystem files_;
};

} // namespace

MK_TEST("sandbox world authoring reviews tile definitions and package rows") {
    const auto review = mirakana::review_sandbox_world_authoring_package(make_review_desc());

    MK_REQUIRE(review.succeeded());
    MK_REQUIRE(review.status == mirakana::SandboxWorldAuthoringStatus::ready);
    MK_REQUIRE(review.tile_definition_rows == 2);
    MK_REQUIRE(review.palette_brush_rows == 1);
    MK_REQUIRE(review.chunk_template_rows == 1);
    MK_REQUIRE(review.procedural_seed_rows == 1);
    MK_REQUIRE(review.deterministic_preview_hash != 0);
    MK_REQUIRE(review.review_document_content.contains("format=GameEngine.SandboxWorldAuthoringReview.v1\n"));
    MK_REQUIRE(review.review_document_content.contains("tile.0.localization_key=tile.grass.name\n"));
    MK_REQUIRE(review.review_document_content.contains("tile.0.accessibility_label_key=tile.grass.accessibility\n"));
    MK_REQUIRE(review.review_document_content.contains("tile.0.provenance=first-party\n"));
    MK_REQUIRE(review.review_document_content.contains("tile.0.license_id=LicenseRef-Proprietary\n"));
    MK_REQUIRE(review.tilemap_package_update.succeeded());
    MK_REQUIRE(review.changed_files.size() == 3);
    MK_REQUIRE(review.changed_files[0].path == "runtime/assets/worlds/seed.tilemap");
    MK_REQUIRE(review.changed_files[1].path == "runtime/assets/worlds/seed.sandbox_authoring");
    MK_REQUIRE(review.changed_files[2].path == "runtime/sample_2d_desktop_runtime_package.geindex");
    MK_REQUIRE(review.changed_files[1].document_kind == mirakana::sandbox_world_authoring_review_format_v1());
    MK_REQUIRE(!review.external_image_decoding_invoked);
    MK_REQUIRE(!review.external_download_invoked);
    MK_REQUIRE(!review.importer_plugin_invoked);
    MK_REQUIRE(!review.package_apply_invoked);

    const auto index =
        mirakana::deserialize_asset_cooked_package_index(review.tilemap_package_update.package_index_content);
    const auto verification = mirakana::verify_cooked_tilemap_package_metadata(
        index, make_tilemap_desc().tilemap_asset, review.tilemap_package_update.tilemap_content);
    MK_REQUIRE(verification.succeeded());
}

MK_TEST("sandbox world authoring rejects malformed tile documents and unsupported imports") {
    auto desc = make_review_desc();
    desc.package.request_external_image_decoding = true;
    desc.package.request_external_download = true;
    desc.package.request_importer_plugin = true;
    desc.package.tile_definitions[1].tile_id = "grass";
    desc.package.tile_definitions[1].atlas_frame_id.clear();
    desc.package.tile_definitions[1].u0 = desc.package.tile_definitions[1].u1;
    desc.package.tile_definitions[1].material_tags.push_back("bad tag");
    desc.package.tile_definitions[1].localization_key.clear();
    desc.package.tile_definitions[1].accessibility_label_key.clear();
    desc.package.tile_definitions[1].provenance.clear();
    desc.package.tile_definitions[1].license_id.clear();

    const auto review = mirakana::review_sandbox_world_authoring_package(desc);

    MK_REQUIRE(!review.succeeded());
    MK_REQUIRE(review.status == mirakana::SandboxWorldAuthoringStatus::invalid_request);
    MK_REQUIRE(has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::duplicate_tile_id));
    MK_REQUIRE(has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::missing_atlas_frame_id));
    MK_REQUIRE(has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::invalid_uv_rect));
    MK_REQUIRE(has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::invalid_material_tag));
    MK_REQUIRE(has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::invalid_localization_key));
    MK_REQUIRE(has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::invalid_accessibility_label_key));
    MK_REQUIRE(has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::missing_provenance));
    MK_REQUIRE(has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::missing_license_id));
    MK_REQUIRE(
        has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::unsupported_external_image_decoding));
    MK_REQUIRE(has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::unsupported_external_download));
    MK_REQUIRE(has_diagnostic(review, mirakana::SandboxWorldAuthoringDiagnosticCode::unsupported_importer_plugin));
    MK_REQUIRE(review.changed_files.empty());
    MK_REQUIRE(!review.external_image_decoding_invoked);
    MK_REQUIRE(!review.external_download_invoked);
    MK_REQUIRE(!review.importer_plugin_invoked);
}

MK_TEST("sandbox world authoring validates palette brushes") {
    auto desc = make_review_desc();
    desc.package.palette_brushes.push_back(mirakana::SandboxWorldPaletteBrushRow{
        .brush_id = "paint/water",
        .shape = mirakana::SandboxWorldBrushShape::rectangle,
        .layer_mask = 2,
        .tile_id = "water",
        .replacement_policy = mirakana::SandboxWorldBrushReplacementPolicy::matching_tile,
        .symmetry = mirakana::SandboxWorldBrushSymmetry::four_way,
        .fill_policy = mirakana::SandboxWorldBrushFillPolicy::replace,
        .preview_path = "runtime/assets/worlds/brushes/water.brush",
        .source_index = 1,
    });
    const auto valid = mirakana::review_sandbox_world_authoring_package(desc);
    MK_REQUIRE(valid.succeeded());
    MK_REQUIRE(valid.palette_brush_rows == 2);

    desc.package.palette_brushes[1].preview_path = "../outside.brush";
    desc.package.palette_brushes[1].tile_id = "missing";
    desc.package.palette_brushes[1].layer_mask = 0;
    const auto invalid = mirakana::review_sandbox_world_authoring_package(desc);
    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(has_diagnostic(invalid, mirakana::SandboxWorldAuthoringDiagnosticCode::invalid_brush_path));
    MK_REQUIRE(has_diagnostic(invalid, mirakana::SandboxWorldAuthoringDiagnosticCode::unknown_brush_tile));
    MK_REQUIRE(has_diagnostic(invalid, mirakana::SandboxWorldAuthoringDiagnosticCode::invalid_layer_mask));
}

MK_TEST("sandbox world authoring validates chunk templates and procedural seeds") {
    const auto first = mirakana::review_sandbox_world_authoring_package(make_review_desc());
    const auto second = mirakana::review_sandbox_world_authoring_package(make_review_desc());
    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(second.succeeded());
    MK_REQUIRE(first.deterministic_preview_hash == second.deterministic_preview_hash);

    auto desc = make_review_desc();
    desc.package.chunk_templates[0].width = 0;
    desc.package.chunk_templates[0].allowed_tile_ids.push_back("missing");
    desc.package.chunk_templates[0].object_placement_rules[0].max_per_chunk = 0;
    desc.package.procedural_seeds.push_back(desc.package.procedural_seeds[0]);
    desc.package.procedural_seeds[1].source_index = 1;
    desc.package.procedural_seeds[1].width = 0;

    const auto invalid = mirakana::review_sandbox_world_authoring_package(desc);
    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(has_diagnostic(invalid, mirakana::SandboxWorldAuthoringDiagnosticCode::invalid_chunk_template_extent));
    MK_REQUIRE(has_diagnostic(invalid, mirakana::SandboxWorldAuthoringDiagnosticCode::unknown_allowed_tile));
    MK_REQUIRE(has_diagnostic(invalid, mirakana::SandboxWorldAuthoringDiagnosticCode::invalid_object_placement_rule));
    MK_REQUIRE(has_diagnostic(invalid, mirakana::SandboxWorldAuthoringDiagnosticCode::duplicate_generator_id));
    MK_REQUIRE(has_diagnostic(invalid, mirakana::SandboxWorldAuthoringDiagnosticCode::invalid_procedural_seed_extent));
}

MK_TEST("sandbox world authoring applies deterministic package updates through filesystem") {
    auto desc = make_review_desc();
    mirakana::MemoryFileSystem filesystem;
    filesystem.write_text(desc.package_index_path, desc.package_index_content);

    const auto applied =
        mirakana::apply_sandbox_world_authoring_package(filesystem, mirakana::SandboxWorldAuthoringApplyDesc{
                                                                        .package_index_path = desc.package_index_path,
                                                                        .package = desc.package,
                                                                    });

    MK_REQUIRE(applied.succeeded());
    MK_REQUIRE(applied.package_apply_invoked);
    MK_REQUIRE(applied.changed_files.size() == 3);
    MK_REQUIRE(filesystem.read_text(applied.changed_files[0].path) == applied.changed_files[0].content);
    MK_REQUIRE(filesystem.read_text(applied.changed_files[1].path) == applied.changed_files[1].content);
    MK_REQUIRE(filesystem.read_text(applied.changed_files[2].path) == applied.changed_files[2].content);

    ThrowingWriteFileSystem throwing{desc.package_index_content};
    const auto failed =
        mirakana::apply_sandbox_world_authoring_package(throwing, mirakana::SandboxWorldAuthoringApplyDesc{
                                                                      .package_index_path = desc.package_index_path,
                                                                      .package = desc.package,
                                                                  });
    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(has_diagnostic(failed, mirakana::SandboxWorldAuthoringDiagnosticCode::apply_write_failed));
}

int main() {
    return mirakana::test::run_all();
}
