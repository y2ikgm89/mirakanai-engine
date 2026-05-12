// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/source_asset_registry.hpp"
#include "mirakana/assets/tilemap_metadata.hpp"
#include "mirakana/assets/ui_atlas_metadata.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::UiAtlasMetadataDocument make_ui_atlas_metadata_document() {
    mirakana::UiAtlasMetadataDocument document;
    document.asset = mirakana::AssetId{100};
    document.pages.push_back(mirakana::UiAtlasMetadataPage{
        .asset = mirakana::AssetId{20},
        .asset_uri = "runtime/assets/ui/page-b.texture.geasset",
    });
    document.pages.push_back(mirakana::UiAtlasMetadataPage{
        .asset = mirakana::AssetId{10},
        .asset_uri = "runtime/assets/ui/page-a.texture.geasset",
    });
    document.images.push_back(mirakana::UiAtlasMetadataImage{
        .resource_id = "hud/icon",
        .asset_uri = "ui://hud/icon",
        .page = mirakana::AssetId{20},
        .uv = mirakana::UiAtlasUvRect{.u0 = 0.25F, .v0 = 0.5F, .u1 = 0.75F, .v1 = 1.0F},
        .color = std::array<float, 4>{1.0F, 1.0F, 1.0F, 1.0F},
    });
    document.images.push_back(mirakana::UiAtlasMetadataImage{
        .resource_id = "hud/background",
        .asset_uri = "ui://hud/background",
        .page = mirakana::AssetId{10},
        .uv = mirakana::UiAtlasUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 0.5F, .v1 = 0.5F},
        .color = std::array<float, 4>{0.5F, 0.25F, 1.0F, 1.0F},
    });
    document.glyphs.push_back(mirakana::UiAtlasMetadataGlyph{
        .font_family = "ui/body",
        .glyph = 66,
        .page = mirakana::AssetId{20},
        .uv = mirakana::UiAtlasUvRect{.u0 = 0.5F, .v0 = 0.0F, .u1 = 0.75F, .v1 = 0.5F},
        .color = std::array<float, 4>{0.9F, 0.9F, 0.9F, 1.0F},
    });
    document.glyphs.push_back(mirakana::UiAtlasMetadataGlyph{
        .font_family = "ui/body",
        .glyph = 65,
        .page = mirakana::AssetId{10},
        .uv = mirakana::UiAtlasUvRect{.u0 = 0.75F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 0.5F},
        .color = std::array<float, 4>{1.0F, 1.0F, 1.0F, 1.0F},
    });
    return document;
}

[[nodiscard]] mirakana::TilemapMetadataDocument make_tilemap_metadata_document() {
    mirakana::TilemapMetadataDocument document;
    document.asset = mirakana::AssetId{300};
    document.atlas_page = mirakana::AssetId{10};
    document.atlas_page_uri = "runtime/assets/2d/player.texture.geasset";
    document.tile_width = 16;
    document.tile_height = 16;
    document.tiles.push_back(mirakana::TilemapAtlasTile{
        .id = "water",
        .page = mirakana::AssetId{10},
        .uv = mirakana::TilemapUvRect{.u0 = 0.5F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 0.5F},
        .color = std::array<float, 4>{0.25F, 0.5F, 1.0F, 1.0F},
    });
    document.tiles.push_back(mirakana::TilemapAtlasTile{
        .id = "grass",
        .page = mirakana::AssetId{10},
        .uv = mirakana::TilemapUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 0.5F, .v1 = 0.5F},
        .color = std::array<float, 4>{0.5F, 1.0F, 0.25F, 1.0F},
    });
    document.layers.push_back(mirakana::TilemapLayer{
        .name = "ground",
        .width = 2,
        .height = 2,
        .visible = true,
        .cells = {"grass", "water", "", "grass"},
    });
    document.layers.push_back(mirakana::TilemapLayer{
        .name = "collision",
        .width = 2,
        .height = 2,
        .visible = false,
        .cells = {"", "water", "", ""},
    });
    return document;
}

} // namespace

MK_TEST("asset identity v2 rejects duplicate keys deterministically") {
    mirakana::AssetIdentityDocumentV2 document;
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "textures/player/albedo"},
        .kind = mirakana::AssetKind::texture,
        .source_path = "source/textures/player_albedo.png",
    });
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "textures/player/albedo"},
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/player.material",
    });

    const auto diagnostics = mirakana::validate_asset_identity_document_v2(document);

    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::AssetIdentityDiagnosticCodeV2::duplicate_key);
    MK_REQUIRE(diagnostics[0].key.value == "textures/player/albedo");
}

MK_TEST("asset identity v2 serializes stable text and derives asset ids from keys") {
    mirakana::AssetIdentityDocumentV2 document;
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "materials/player"},
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/player.material",
    });

    const std::string expected = "format=GameEngine.AssetIdentity.v2\n"
                                 "asset.0.key=materials/player\n"
                                 "asset.0.id=" +
                                 std::to_string(mirakana::AssetId::from_name("materials/player").value) +
                                 "\n"
                                 "asset.0.kind=material\n"
                                 "asset.0.source=source/materials/player.material\n";

    MK_REQUIRE(mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = "materials/player"}) ==
               mirakana::AssetId::from_name("materials/player"));
    MK_REQUIRE(mirakana::serialize_asset_identity_document_v2(document) == expected);

    const auto round_trip = mirakana::deserialize_asset_identity_document_v2(expected);
    MK_REQUIRE(round_trip.assets.size() == 1);
    MK_REQUIRE(round_trip.assets[0].key.value == "materials/player");
    MK_REQUIRE(round_trip.assets[0].kind == mirakana::AssetKind::material);
}

MK_TEST("physics collision scene kind round trips through asset identity and source registry text") {
    mirakana::AssetIdentityDocumentV2 identity;
    identity.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "physics/collision/main"},
        .kind = mirakana::AssetKind::physics_collision_scene,
        .source_path = "source/physics/main.collision3d",
    });

    const std::string expected_identity = "format=GameEngine.AssetIdentity.v2\n"
                                          "asset.0.key=physics/collision/main\n"
                                          "asset.0.id=" +
                                          std::to_string(mirakana::AssetId::from_name("physics/collision/main").value) +
                                          "\n"
                                          "asset.0.kind=physics_collision_scene\n"
                                          "asset.0.source=source/physics/main.collision3d\n";

    MK_REQUIRE(mirakana::serialize_asset_identity_document_v2(identity) == expected_identity);
    const auto identity_round_trip = mirakana::deserialize_asset_identity_document_v2(expected_identity);
    MK_REQUIRE(identity_round_trip.assets.size() == 1);
    MK_REQUIRE(identity_round_trip.assets[0].kind == mirakana::AssetKind::physics_collision_scene);

    const std::string source_registry = "format=GameEngine.SourceAssetRegistry.v1\n"
                                        "asset.0.key=physics/collision/main\n"
                                        "asset.0.id=" +
                                        std::to_string(mirakana::AssetId::from_name("physics/collision/main").value) +
                                        "\n"
                                        "asset.0.kind=physics_collision_scene\n"
                                        "asset.0.source=source/physics/main.collision3d\n"
                                        "asset.0.source_format=\n"
                                        "asset.0.imported=runtime/assets/physics/main.collision3d\n";
    const auto source_round_trip = mirakana::parse_source_asset_registry_document_unvalidated_v1(source_registry);
    MK_REQUIRE(source_round_trip.assets.size() == 1);
    MK_REQUIRE(source_round_trip.assets[0].kind == mirakana::AssetKind::physics_collision_scene);
    MK_REQUIRE(!mirakana::is_supported_source_asset_kind_v1(mirakana::AssetKind::physics_collision_scene));
    MK_REQUIRE(mirakana::expected_source_asset_format_v1(mirakana::AssetKind::physics_collision_scene).empty());
}

MK_TEST("asset identity v2 rejects duplicate source paths") {
    mirakana::AssetIdentityDocumentV2 document;
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "textures/player/albedo"},
        .kind = mirakana::AssetKind::texture,
        .source_path = "source/shared.asset",
    });
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "materials/player"},
        .kind = mirakana::AssetKind::material,
        .source_path = "source/shared.asset",
    });

    const auto diagnostics = mirakana::validate_asset_identity_document_v2(document);

    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::AssetIdentityDiagnosticCodeV2::duplicate_source_path);
    MK_REQUIRE(diagnostics[0].source_path == "source/shared.asset");
}

MK_TEST("asset identity v2 plans production placement references from stable keys") {
    mirakana::AssetIdentityDocumentV2 document;
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
        .kind = mirakana::AssetKind::mesh,
        .source_path = "source/meshes/hero.mesh",
    });
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "materials/hero"},
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/hero.material",
    });

    const std::array requests{
        mirakana::AssetIdentityPlacementRequestV2{
            .placement = "scene.hero.mesh",
            .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
            .expected_kind = mirakana::AssetKind::mesh,
        },
        mirakana::AssetIdentityPlacementRequestV2{
            .placement = "scene.hero.material",
            .key = mirakana::AssetKeyV2{.value = "materials/hero"},
            .expected_kind = mirakana::AssetKind::material,
        },
    };

    const auto plan = mirakana::plan_asset_identity_placements_v2(document, requests);

    MK_REQUIRE(plan.can_place);
    MK_REQUIRE(plan.identity_diagnostics.empty());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.rows.size() == 2);
    MK_REQUIRE(plan.rows[0].placement == "scene.hero.mesh");
    MK_REQUIRE(plan.rows[0].id == mirakana::AssetId::from_name("meshes/hero"));
    MK_REQUIRE(plan.rows[0].kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(plan.rows[0].source_path == "source/meshes/hero.mesh");
    MK_REQUIRE(plan.rows[1].placement == "scene.hero.material");
    MK_REQUIRE(plan.rows[1].id == mirakana::AssetId::from_name("materials/hero"));
}

MK_TEST("asset identity v2 placement rejects missing keys and kind mismatches") {
    mirakana::AssetIdentityDocumentV2 document;
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "textures/hero"},
        .kind = mirakana::AssetKind::texture,
        .source_path = "source/textures/hero.png",
    });

    const std::array requests{
        mirakana::AssetIdentityPlacementRequestV2{
            .placement = "scene.hero.material",
            .key = mirakana::AssetKeyV2{.value = "textures/hero"},
            .expected_kind = mirakana::AssetKind::material,
        },
        mirakana::AssetIdentityPlacementRequestV2{
            .placement = "scene.hero.mesh",
            .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
            .expected_kind = mirakana::AssetKind::mesh,
        },
    };

    const auto plan = mirakana::plan_asset_identity_placements_v2(document, requests);

    MK_REQUIRE(!plan.can_place);
    MK_REQUIRE(plan.identity_diagnostics.empty());
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(plan.diagnostics.size() == 2);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::AssetIdentityPlacementDiagnosticCodeV2::kind_mismatch);
    MK_REQUIRE(plan.diagnostics[0].actual_kind == mirakana::AssetKind::texture);
    MK_REQUIRE(plan.diagnostics[1].code == mirakana::AssetIdentityPlacementDiagnosticCodeV2::missing_key);
    MK_REQUIRE(plan.diagnostics[1].key.value == "meshes/hero");
}

MK_TEST("asset identity v2 placement carries invalid identity details") {
    mirakana::AssetIdentityDocumentV2 document;
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
        .kind = mirakana::AssetKind::mesh,
        .source_path = "source/meshes/hero.mesh",
    });
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/hero.material",
    });
    const std::array requests{
        mirakana::AssetIdentityPlacementRequestV2{
            .placement = "scene.hero.mesh",
            .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
            .expected_kind = mirakana::AssetKind::mesh,
        },
    };

    const auto plan = mirakana::plan_asset_identity_placements_v2(document, requests);

    MK_REQUIRE(!plan.can_place);
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(plan.diagnostics.size() == 1);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::AssetIdentityPlacementDiagnosticCodeV2::invalid_identity_document);
    MK_REQUIRE(plan.identity_diagnostics.size() == 1);
    MK_REQUIRE(plan.identity_diagnostics[0].code == mirakana::AssetIdentityDiagnosticCodeV2::duplicate_key);
    MK_REQUIRE(plan.identity_diagnostics[0].key.value == "meshes/hero");
}

MK_TEST("asset identity v2 placement validates placement grammar and stays all or nothing") {
    mirakana::AssetIdentityDocumentV2 document;
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
        .kind = mirakana::AssetKind::mesh,
        .source_path = "source/meshes/hero.mesh",
    });
    const std::array requests{
        mirakana::AssetIdentityPlacementRequestV2{
            .placement = "scene.hero.mesh",
            .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
            .expected_kind = mirakana::AssetKind::mesh,
        },
        mirakana::AssetIdentityPlacementRequestV2{
            .placement = "../scene",
            .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
            .expected_kind = mirakana::AssetKind::mesh,
        },
        mirakana::AssetIdentityPlacementRequestV2{
            .placement = "scene.hero.mesh",
            .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
            .expected_kind = mirakana::AssetKind::mesh,
        },
        mirakana::AssetIdentityPlacementRequestV2{
            .placement = "scene.hero.bad_key",
            .key = mirakana::AssetKeyV2{.value = "bad key"},
            .expected_kind = mirakana::AssetKind::mesh,
        },
        mirakana::AssetIdentityPlacementRequestV2{
            .placement = "scene.hero.unknown_kind",
            .key = mirakana::AssetKeyV2{.value = "meshes/hero"},
            .expected_kind = mirakana::AssetKind::unknown,
        },
    };

    const auto plan = mirakana::plan_asset_identity_placements_v2(document, requests);

    MK_REQUIRE(!plan.can_place);
    MK_REQUIRE(plan.identity_diagnostics.empty());
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(plan.diagnostics.size() == 4);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::AssetIdentityPlacementDiagnosticCodeV2::invalid_placement);
    MK_REQUIRE(plan.diagnostics[0].placement == "../scene");
    MK_REQUIRE(plan.diagnostics[1].code == mirakana::AssetIdentityPlacementDiagnosticCodeV2::duplicate_placement);
    MK_REQUIRE(plan.diagnostics[2].code == mirakana::AssetIdentityPlacementDiagnosticCodeV2::invalid_key);
    MK_REQUIRE(plan.diagnostics[3].code == mirakana::AssetIdentityPlacementDiagnosticCodeV2::invalid_expected_kind);
}

MK_TEST("ui atlas metadata serializes deterministic cooked document text") {
    const auto document = make_ui_atlas_metadata_document();
    const std::string expected = "format=GameEngine.UiAtlas.v1\n"
                                 "asset.id=100\n"
                                 "asset.kind=ui_atlas\n"
                                 "source.decoding=unsupported\n"
                                 "atlas.packing=unsupported\n"
                                 "page.count=2\n"
                                 "page.0.asset=10\n"
                                 "page.0.asset_uri=runtime/assets/ui/page-a.texture.geasset\n"
                                 "page.1.asset=20\n"
                                 "page.1.asset_uri=runtime/assets/ui/page-b.texture.geasset\n"
                                 "image.count=2\n"
                                 "image.0.resource_id=hud/background\n"
                                 "image.0.asset_uri=ui://hud/background\n"
                                 "image.0.page=10\n"
                                 "image.0.u0=0\n"
                                 "image.0.v0=0\n"
                                 "image.0.u1=0.5\n"
                                 "image.0.v1=0.5\n"
                                 "image.0.color=0.5,0.25,1,1\n"
                                 "image.1.resource_id=hud/icon\n"
                                 "image.1.asset_uri=ui://hud/icon\n"
                                 "image.1.page=20\n"
                                 "image.1.u0=0.25\n"
                                 "image.1.v0=0.5\n"
                                 "image.1.u1=0.75\n"
                                 "image.1.v1=1\n"
                                 "image.1.color=1,1,1,1\n"
                                 "glyph.count=2\n"
                                 "glyph.0.font_family=ui/body\n"
                                 "glyph.0.glyph=65\n"
                                 "glyph.0.page=10\n"
                                 "glyph.0.u0=0.75\n"
                                 "glyph.0.v0=0\n"
                                 "glyph.0.u1=1\n"
                                 "glyph.0.v1=0.5\n"
                                 "glyph.0.color=1,1,1,1\n"
                                 "glyph.1.font_family=ui/body\n"
                                 "glyph.1.glyph=66\n"
                                 "glyph.1.page=20\n"
                                 "glyph.1.u0=0.5\n"
                                 "glyph.1.v0=0\n"
                                 "glyph.1.u1=0.75\n"
                                 "glyph.1.v1=0.5\n"
                                 "glyph.1.color=0.9,0.9,0.9,1\n";

    const auto diagnostics = mirakana::validate_ui_atlas_metadata_document(document);

    MK_REQUIRE(diagnostics.empty());
    MK_REQUIRE(mirakana::serialize_ui_atlas_metadata_document(document) == expected);

    const auto round_trip = mirakana::deserialize_ui_atlas_metadata_document(expected);
    MK_REQUIRE(round_trip.asset == mirakana::AssetId{100});
    MK_REQUIRE(round_trip.pages.size() == 2);
    MK_REQUIRE(round_trip.pages[0].asset == mirakana::AssetId{10});
    MK_REQUIRE(round_trip.images.size() == 2);
    MK_REQUIRE(round_trip.images[0].resource_id == "hud/background");
    MK_REQUIRE(round_trip.glyphs.size() == 2);
    MK_REQUIRE(round_trip.glyphs[0].font_family == "ui/body");
    MK_REQUIRE(round_trip.glyphs[0].glyph == 65);
    MK_REQUIRE(round_trip.glyphs[0].page == mirakana::AssetId{10});
}

MK_TEST("ui atlas metadata rejects duplicate identities malformed uvs and unsupported claims") {
    auto document = make_ui_atlas_metadata_document();
    document.images[1].resource_id = document.images[0].resource_id;
    auto diagnostics = mirakana::validate_ui_atlas_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::UiAtlasMetadataDiagnosticCode::duplicate_resource_id);

    document = make_ui_atlas_metadata_document();
    document.images[1].asset_uri = document.images[0].asset_uri;
    diagnostics = mirakana::validate_ui_atlas_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::UiAtlasMetadataDiagnosticCode::duplicate_asset_uri);

    document = make_ui_atlas_metadata_document();
    document.images[0].uv.u0 = document.images[0].uv.u1;
    diagnostics = mirakana::validate_ui_atlas_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::UiAtlasMetadataDiagnosticCode::invalid_uv_rect);

    document = make_ui_atlas_metadata_document();
    document.source_decoding = "ready";
    diagnostics = mirakana::validate_ui_atlas_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::UiAtlasMetadataDiagnosticCode::unsupported_source_decoding);

    document = make_ui_atlas_metadata_document();
    document.atlas_packing = "ready";
    diagnostics = mirakana::validate_ui_atlas_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::UiAtlasMetadataDiagnosticCode::unsupported_atlas_packing);

    document = make_ui_atlas_metadata_document();
    document.glyphs[1].glyph = document.glyphs[0].glyph;
    diagnostics = mirakana::validate_ui_atlas_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::UiAtlasMetadataDiagnosticCode::duplicate_glyph);

    document = make_ui_atlas_metadata_document();
    document.glyphs[0].font_family.clear();
    diagnostics = mirakana::validate_ui_atlas_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::UiAtlasMetadataDiagnosticCode::missing_glyph_identity);

    document = make_ui_atlas_metadata_document();
    document.glyphs[0].uv.u1 = 1.25F;
    diagnostics = mirakana::validate_ui_atlas_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::UiAtlasMetadataDiagnosticCode::invalid_glyph_uv_rect);
}

MK_TEST("tilemap metadata serializes deterministic package data without production claims") {
    const auto document = make_tilemap_metadata_document();
    const std::string expected = "format=GameEngine.Tilemap.v1\n"
                                 "asset.id=300\n"
                                 "asset.kind=tilemap\n"
                                 "source.decoding=unsupported\n"
                                 "atlas.packing=unsupported\n"
                                 "native_gpu_sprite_batching=unsupported\n"
                                 "atlas.page.asset=10\n"
                                 "atlas.page.asset_uri=runtime/assets/2d/player.texture.geasset\n"
                                 "tile.size=16,16\n"
                                 "tile.count=2\n"
                                 "tile.0.id=grass\n"
                                 "tile.0.page=10\n"
                                 "tile.0.u0=0\n"
                                 "tile.0.v0=0\n"
                                 "tile.0.u1=0.5\n"
                                 "tile.0.v1=0.5\n"
                                 "tile.0.color=0.5,1,0.25,1\n"
                                 "tile.1.id=water\n"
                                 "tile.1.page=10\n"
                                 "tile.1.u0=0.5\n"
                                 "tile.1.v0=0\n"
                                 "tile.1.u1=1\n"
                                 "tile.1.v1=0.5\n"
                                 "tile.1.color=0.25,0.5,1,1\n"
                                 "layer.count=2\n"
                                 "layer.0.name=ground\n"
                                 "layer.0.width=2\n"
                                 "layer.0.height=2\n"
                                 "layer.0.visible=true\n"
                                 "layer.0.cells=grass,water,,grass\n"
                                 "layer.1.name=collision\n"
                                 "layer.1.width=2\n"
                                 "layer.1.height=2\n"
                                 "layer.1.visible=false\n"
                                 "layer.1.cells=,water,,\n";

    const auto diagnostics = mirakana::validate_tilemap_metadata_document(document);

    MK_REQUIRE(diagnostics.empty());
    MK_REQUIRE(mirakana::serialize_tilemap_metadata_document(document) == expected);

    const auto round_trip = mirakana::deserialize_tilemap_metadata_document(expected);
    MK_REQUIRE(round_trip.asset == mirakana::AssetId{300});
    MK_REQUIRE(round_trip.atlas_page == mirakana::AssetId{10});
    MK_REQUIRE(round_trip.tiles.size() == 2);
    MK_REQUIRE(round_trip.tiles[0].id == "grass");
    MK_REQUIRE(round_trip.layers.size() == 2);
    MK_REQUIRE(round_trip.layers[0].name == "ground");
    MK_REQUIRE(round_trip.layers[0].cells[1] == "water");
    MK_REQUIRE(round_trip.layers[1].name == "collision");
}

MK_TEST("tilemap metadata rejects unknown cells malformed uvs and unsupported production claims") {
    auto document = make_tilemap_metadata_document();
    document.layers[0].cells[1] = "missing";
    auto diagnostics = mirakana::validate_tilemap_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::TilemapMetadataDiagnosticCode::unknown_cell_tile);

    document = make_tilemap_metadata_document();
    document.tiles[0].uv.u0 = document.tiles[0].uv.u1;
    diagnostics = mirakana::validate_tilemap_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::TilemapMetadataDiagnosticCode::invalid_tile_uv_rect);

    document = make_tilemap_metadata_document();
    document.source_decoding = "ready";
    diagnostics = mirakana::validate_tilemap_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::TilemapMetadataDiagnosticCode::unsupported_source_decoding);

    document = make_tilemap_metadata_document();
    document.atlas_packing = "ready";
    diagnostics = mirakana::validate_tilemap_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::TilemapMetadataDiagnosticCode::unsupported_atlas_packing);

    document = make_tilemap_metadata_document();
    document.native_gpu_sprite_batching = "ready";
    diagnostics = mirakana::validate_tilemap_metadata_document(document);
    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::TilemapMetadataDiagnosticCode::unsupported_native_gpu_sprite_batching);
}

MK_TEST("runtime resource v2 resolves handles and rejects stale generations") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const mirakana::runtime::RuntimeAssetPackage first_package({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 7},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n",
        },
    });

    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    const auto first_build = mirakana::runtime::build_runtime_resource_catalog_v2(catalog, first_package);

    MK_REQUIRE(first_build.succeeded());
    const auto first_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(first_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 first_handle = *first_handle_opt;
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, first_handle));

    const mirakana::runtime::RuntimeAssetPackage second_package({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 9},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 22,
            .source_revision = 2,
            .dependencies = {},
            .content = "format=GameEngine.CookedTexture.v1\ntexture.width=8\n",
        },
    });

    const auto second_build = mirakana::runtime::build_runtime_resource_catalog_v2(catalog, second_package);
    MK_REQUIRE(second_build.succeeded());

    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, first_handle));
    const auto second_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(second_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 second_handle = *second_handle_opt;
    const auto first_generation = first_handle.generation;
    const auto second_generation = second_handle.generation;
    MK_REQUIRE(second_generation != first_generation);
    const auto* record = mirakana::runtime::runtime_resource_record_v2(catalog, second_handle);
    MK_REQUIRE(record != nullptr);
    MK_REQUIRE(record->asset == texture);
    MK_REQUIRE(record->package_handle == mirakana::runtime::RuntimeAssetHandle{.value = 9});
}

MK_TEST("runtime resource v2 duplicate asset leaves existing catalog live") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    const auto first_build = mirakana::runtime::build_runtime_resource_catalog_v2(
        catalog, mirakana::runtime::RuntimeAssetPackage({
                     mirakana::runtime::RuntimeAssetRecord{
                         .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
                         .asset = texture,
                         .kind = mirakana::AssetKind::texture,
                         .path = "assets/textures/player.texture",
                         .content_hash = 11,
                         .source_revision = 1,
                         .dependencies = {},
                         .content = {},
                     },
                 }));
    MK_REQUIRE(first_build.succeeded());
    const auto live_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(live_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 live_handle = *live_handle_opt;

    const auto duplicate_build = mirakana::runtime::build_runtime_resource_catalog_v2(
        catalog, mirakana::runtime::RuntimeAssetPackage({
                     mirakana::runtime::RuntimeAssetRecord{
                         .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
                         .asset = mesh,
                         .kind = mirakana::AssetKind::mesh,
                         .path = "assets/meshes/player.mesh",
                         .content_hash = 22,
                         .source_revision = 1,
                         .dependencies = {},
                         .content = {},
                     },
                     mirakana::runtime::RuntimeAssetRecord{
                         .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
                         .asset = mesh,
                         .kind = mirakana::AssetKind::mesh,
                         .path = "assets/meshes/player-lod.mesh",
                         .content_hash = 23,
                         .source_revision = 1,
                         .dependencies = {},
                         .content = {},
                     },
                 }));

    MK_REQUIRE(!duplicate_build.succeeded());
    MK_REQUIRE(duplicate_build.diagnostics.size() == 1);
    MK_REQUIRE(duplicate_build.diagnostics[0].asset == mesh);
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, live_handle));
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog, mesh).has_value());
}

MK_TEST("runtime package safe point unload clears active package and invalidates handles") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "active texture",
        },
    }));
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, *store.active()).succeeded());
    const auto stale_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(stale_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 stale_handle = *stale_handle_opt;

    store.stage(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/player.mesh",
            .content_hash = 22,
            .source_revision = 1,
            .dependencies = {},
            .content = "pending mesh",
        },
    }));

    const auto unload = mirakana::runtime::commit_runtime_package_safe_point_unload(store, catalog);

    MK_REQUIRE(unload.succeeded());
    MK_REQUIRE(unload.status == mirakana::runtime::RuntimePackageSafePointUnloadStatus::unloaded);
    MK_REQUIRE(unload.previous_record_count == 1);
    MK_REQUIRE(unload.committed_record_count == 0);
    MK_REQUIRE(unload.discarded_pending_package);
    MK_REQUIRE(unload.stale_handle_count == 1);
    MK_REQUIRE(unload.previous_generation != unload.committed_generation);
    MK_REQUIRE(store.active() == nullptr);
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(catalog.records().empty());
    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, stale_handle));
}

MK_TEST("runtime package safe point unload without active package preserves pending package") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.stage(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "pending texture",
        },
    }));

    const auto unload = mirakana::runtime::commit_runtime_package_safe_point_unload(store, catalog);

    MK_REQUIRE(!unload.succeeded());
    MK_REQUIRE(unload.status == mirakana::runtime::RuntimePackageSafePointUnloadStatus::no_active_package);
    MK_REQUIRE(unload.previous_record_count == 0);
    MK_REQUIRE(unload.committed_record_count == 0);
    MK_REQUIRE(unload.previous_generation == 0);
    MK_REQUIRE(unload.committed_generation == 0);
    MK_REQUIRE(!unload.discarded_pending_package);
    MK_REQUIRE(store.active() == nullptr);
    MK_REQUIRE(store.pending() != nullptr);
    MK_REQUIRE(store.pending()->find(texture) != nullptr);
    MK_REQUIRE(catalog.records().empty());
}

MK_TEST("runtime package safe point replacement commits package and catalog together") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n",
        },
    }));
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, *store.active()).succeeded());
    const auto stale_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(stale_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 stale_handle = *stale_handle_opt;

    store.stage(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 9},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 22,
            .source_revision = 2,
            .dependencies = {},
            .content = "format=GameEngine.CookedTexture.v1\ntexture.width=8\n",
        },
    }));

    const auto replacement = mirakana::runtime::commit_runtime_package_safe_point_replacement(store, catalog);
    MK_REQUIRE(replacement.succeeded());
    MK_REQUIRE(replacement.status == mirakana::runtime::RuntimePackageSafePointReplacementStatus::committed);
    MK_REQUIRE(replacement.previous_record_count == 1);
    MK_REQUIRE(replacement.committed_record_count == 1);
    MK_REQUIRE(replacement.stale_handle_count == 1);
    MK_REQUIRE(replacement.previous_generation != replacement.committed_generation);
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content.find("texture.width=8") != std::string::npos);
    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, stale_handle));

    const auto live_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(live_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 live_handle = *live_handle_opt;
    const auto* record = mirakana::runtime::runtime_resource_record_v2(catalog, live_handle);
    MK_REQUIRE(record != nullptr);
    MK_REQUIRE(record->package_handle == mirakana::runtime::RuntimeAssetHandle{.value = 9});
}

MK_TEST("runtime package safe point replacement rejects invalid pending package without replacing active") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "active",
        },
    }));
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, *store.active()).succeeded());
    const auto active_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(active_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 active_handle = *active_handle_opt;

    store.stage(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/player.mesh",
            .content_hash = 22,
            .source_revision = 1,
            .dependencies = {},
            .content = "mesh-a",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 3},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/player-lod.mesh",
            .content_hash = 23,
            .source_revision = 1,
            .dependencies = {},
            .content = "mesh-b",
        },
    }));

    const auto replacement = mirakana::runtime::commit_runtime_package_safe_point_replacement(store, catalog);
    MK_REQUIRE(!replacement.succeeded());
    MK_REQUIRE(replacement.status == mirakana::runtime::RuntimePackageSafePointReplacementStatus::catalog_build_failed);
    MK_REQUIRE(replacement.diagnostics.size() == 1);
    MK_REQUIRE(replacement.diagnostics[0].asset == mesh);
    MK_REQUIRE(replacement.committed_record_count == 0);
    MK_REQUIRE(replacement.stale_handle_count == 0);
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "active");
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, active_handle));
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog, mesh).has_value());
}

MK_TEST("runtime package streaming execution requires validation preflight before safe point commit") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "active",
        },
    }));
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, *store.active()).succeeded());
    const auto active_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(active_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 active_handle = *active_handle_opt;

    const mirakana::runtime::RuntimePackageStreamingExecutionDesc desc{
        .target_id = "packaged-scene-streaming",
        .package_index_path = "runtime/game.geindex",
        .runtime_scene_validation_target_id = "packaged-scene",
        .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point,
        .resident_budget_bytes = 1024,
        .safe_point_required = true,
        .runtime_scene_validation_succeeded = false,
    };

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded;
    loaded.package = mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 22,
            .source_revision = 2,
            .dependencies = {},
            .content = "replacement",
        },
    });

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(store, catalog, desc,
                                                                                                 std::move(loaded));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::validation_preflight_required);
    MK_REQUIRE(result.replacement.status ==
               mirakana::runtime::RuntimePackageSafePointReplacementStatus::no_pending_package);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "runtime-scene-validation-required");
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "active");
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, active_handle));
}

MK_TEST("runtime package streaming execution commits selected loaded package at safe point") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "active",
        },
    }));
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, *store.active()).succeeded());
    const auto stale_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(stale_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 stale_handle = *stale_handle_opt;

    const mirakana::runtime::RuntimePackageStreamingExecutionDesc desc{
        .target_id = "packaged-scene-streaming",
        .package_index_path = "runtime/game.geindex",
        .runtime_scene_validation_target_id = "packaged-scene",
        .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point,
        .resident_budget_bytes = 4096,
        .safe_point_required = true,
        .runtime_scene_validation_succeeded = true,
    };

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded;
    loaded.package = mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 22,
            .source_revision = 2,
            .dependencies = {},
            .content = "replacement texture",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 3},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/player.material",
            .content_hash = 33,
            .source_revision = 1,
            .dependencies = {texture},
            .content = "replacement material",
        },
    });

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(store, catalog, desc,
                                                                                                 std::move(loaded));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.target_id == "packaged-scene-streaming");
    MK_REQUIRE(result.package_index_path == "runtime/game.geindex");
    MK_REQUIRE(result.runtime_scene_validation_target_id == "packaged-scene");
    MK_REQUIRE(result.estimated_resident_bytes > 0);
    MK_REQUIRE(result.estimated_resident_bytes <= desc.resident_budget_bytes);
    MK_REQUIRE(result.replacement.status == mirakana::runtime::RuntimePackageSafePointReplacementStatus::committed);
    MK_REQUIRE(result.replacement.stale_handle_count == 1);
    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, stale_handle));
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog, material).has_value());
}

MK_TEST("runtime package streaming execution rejects planning-only or unsafe descriptors") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "active",
        },
    }));
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, *store.active()).succeeded());

    mirakana::runtime::RuntimePackageStreamingExecutionDesc desc{
        .target_id = "packaged-scene-streaming",
        .package_index_path = "runtime/game.geindex",
        .runtime_scene_validation_target_id = "packaged-scene",
        .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::planning_only,
        .resident_budget_bytes = 1024,
        .safe_point_required = true,
        .runtime_scene_validation_succeeded = true,
    };

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded;
    loaded.package = mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 22,
            .source_revision = 2,
            .dependencies = {},
            .content = "replacement",
        },
    });

    const auto planning_only = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        store, catalog, desc, std::move(loaded));

    MK_REQUIRE(!planning_only.succeeded());
    MK_REQUIRE(planning_only.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::invalid_descriptor);
    MK_REQUIRE(planning_only.diagnostics.size() == 1);
    MK_REQUIRE(planning_only.diagnostics[0].code == "host-gated-safe-point-mode-required");
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "active");

    desc.mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point;
    desc.safe_point_required = false;
    mirakana::runtime::RuntimeAssetPackageLoadResult unsafe_loaded;
    unsafe_loaded.package = mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 3},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 33,
            .source_revision = 3,
            .dependencies = {},
            .content = "unsafe replacement",
        },
    });

    const auto unsafe = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        store, catalog, desc, std::move(unsafe_loaded));

    MK_REQUIRE(!unsafe.succeeded());
    MK_REQUIRE(unsafe.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::invalid_descriptor);
    MK_REQUIRE(unsafe.diagnostics.size() == 1);
    MK_REQUIRE(unsafe.diagnostics[0].code == "safe-point-required");
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "active");
}

MK_TEST("runtime package streaming execution reports over budget intent without allocator enforcement claim") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "active",
        },
    }));
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, *store.active()).succeeded());

    const mirakana::runtime::RuntimePackageStreamingExecutionDesc desc{
        .target_id = "packaged-scene-streaming",
        .package_index_path = "runtime/game.geindex",
        .runtime_scene_validation_target_id = "packaged-scene",
        .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point,
        .resident_budget_bytes = 4,
        .safe_point_required = true,
        .runtime_scene_validation_succeeded = true,
    };

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded;
    loaded.package = mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 22,
            .source_revision = 2,
            .dependencies = {},
            .content = "replacement texture that exceeds intent",
        },
    });

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(store, catalog, desc,
                                                                                                 std::move(loaded));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent);
    MK_REQUIRE(result.estimated_resident_bytes > desc.resident_budget_bytes);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "resident-budget-intent-exceeded");
    MK_REQUIRE(result.diagnostics[0].message.find("allocator") == std::string::npos);
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "active");
}

MK_TEST("runtime package streaming execution rejects missing required preload asset hints before staging") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "active",
        },
    }));
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, *store.active()).succeeded());
    const auto active_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(active_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 active_handle = *active_handle_opt;

    const mirakana::runtime::RuntimePackageStreamingExecutionDesc desc{
        .target_id = "packaged-scene-streaming",
        .package_index_path = "runtime/game.geindex",
        .runtime_scene_validation_target_id = "packaged-scene",
        .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point,
        .resident_budget_bytes = 4096,
        .safe_point_required = true,
        .runtime_scene_validation_succeeded = true,
        .required_preload_assets = {material},
    };

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded;
    loaded.package = mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 22,
            .source_revision = 2,
            .dependencies = {},
            .content = "replacement texture",
        },
    });

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(store, catalog, desc,
                                                                                                 std::move(loaded));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::residency_hint_failed);
    MK_REQUIRE(result.required_preload_asset_count == 1);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "preload-asset-missing");
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "active");
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, active_handle));
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog, material).has_value());
}

MK_TEST("runtime package streaming execution rejects disallowed resident resource kinds before staging") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "active",
        },
    }));
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, *store.active()).succeeded());

    const mirakana::runtime::RuntimePackageStreamingExecutionDesc desc{
        .target_id = "packaged-scene-streaming",
        .package_index_path = "runtime/game.geindex",
        .runtime_scene_validation_target_id = "packaged-scene",
        .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point,
        .resident_budget_bytes = 4096,
        .safe_point_required = true,
        .runtime_scene_validation_succeeded = true,
        .resident_resource_kinds = {mirakana::AssetKind::texture},
    };

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded;
    loaded.package = mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 22,
            .source_revision = 2,
            .dependencies = {},
            .content = "replacement texture",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 3},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/player.material",
            .content_hash = 33,
            .source_revision = 1,
            .dependencies = {texture},
            .content = "replacement material",
        },
    });

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(store, catalog, desc,
                                                                                                 std::move(loaded));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::residency_hint_failed);
    MK_REQUIRE(result.resident_resource_kind_count == 1);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "resident-resource-kind-disallowed");
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "active");
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog, material).has_value());
}

MK_TEST("runtime package streaming execution commits when residency hints match loaded package") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 11,
            .source_revision = 1,
            .dependencies = {},
            .content = "active",
        },
    }));
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, *store.active()).succeeded());

    const mirakana::runtime::RuntimePackageStreamingExecutionDesc desc{
        .target_id = "packaged-scene-streaming",
        .package_index_path = "runtime/game.geindex",
        .runtime_scene_validation_target_id = "packaged-scene",
        .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point,
        .resident_budget_bytes = 4096,
        .safe_point_required = true,
        .runtime_scene_validation_succeeded = true,
        .required_preload_assets = {texture, material},
        .resident_resource_kinds = {mirakana::AssetKind::texture, mirakana::AssetKind::material},
        .max_resident_packages = 1,
    };

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded;
    loaded.package = mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 22,
            .source_revision = 2,
            .dependencies = {},
            .content = "replacement texture",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 3},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/player.material",
            .content_hash = 33,
            .source_revision = 1,
            .dependencies = {texture},
            .content = "replacement material",
        },
    });

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(store, catalog, desc,
                                                                                                 std::move(loaded));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.required_preload_asset_count == 2);
    MK_REQUIRE(result.resident_resource_kind_count == 2);
    MK_REQUIRE(result.resident_package_count == 1);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(material)->content == "replacement material");
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog, material).has_value());
}

MK_TEST("runtime resident mount overlay merges packages deterministically") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");

    const mirakana::runtime::RuntimeAssetPackage base({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 100,
            .source_revision = 1,
            .dependencies = {},
            .content = "first",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/player.mesh",
            .content_hash = 200,
            .source_revision = 1,
            .dependencies = {texture},
            .content = "mesh-first",
        },
    });

    const mirakana::runtime::RuntimeAssetPackage overlay(
        {
            mirakana::runtime::RuntimeAssetRecord{
                .handle = mirakana::runtime::RuntimeAssetHandle{.value = 99},
                .asset = texture,
                .kind = mirakana::AssetKind::texture,
                .path = "assets/textures/player.texture",
                .content_hash = 101,
                .source_revision = 2,
                .dependencies = {},
                .content = "second",
            },
        },
        {
            mirakana::AssetDependencyEdge{
                .asset = mesh,
                .dependency = texture,
                .kind = mirakana::AssetDependencyKind::material_texture,
                .path = "assets/textures/player.texture",
            },
        });

    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    {
        std::vector<mirakana::runtime::RuntimeAssetPackage> mounts{base, overlay};
        MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2_from_resident_mounts(
                       catalog, mounts, mirakana::runtime::RuntimePackageMountOverlay::first_mount_wins)
                       .succeeded());
        const auto handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
        MK_REQUIRE(handle_opt.has_value());
        const auto* tex_record = mirakana::runtime::runtime_resource_record_v2(catalog, *handle_opt);
        MK_REQUIRE(tex_record != nullptr);
        MK_REQUIRE(tex_record->content_hash == 100);
    }
    {
        std::vector<mirakana::runtime::RuntimeAssetPackage> mounts{base, overlay};
        MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2_from_resident_mounts(
                       catalog, mounts, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins)
                       .succeeded());
        const auto handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
        MK_REQUIRE(handle_opt.has_value());
        const auto* tex_record = mirakana::runtime::runtime_resource_record_v2(catalog, *handle_opt);
        MK_REQUIRE(tex_record != nullptr);
        MK_REQUIRE(tex_record->content_hash == 101);
    }

    const auto merged = mirakana::runtime::merge_runtime_asset_packages_overlay(
        {base, overlay}, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins);
    MK_REQUIRE(merged.records().size() == 2);
    MK_REQUIRE(merged.find(texture)->content == "second");
    MK_REQUIRE(merged.find(mesh)->dependencies.size() == 1);
    MK_REQUIRE(merged.find(mesh)->dependencies[0] == texture);
    MK_REQUIRE(merged.dependency_edges().size() == 1);
    MK_REQUIRE(merged.dependency_edges()[0].asset == mesh);
    MK_REQUIRE(merged.dependency_edges()[0].dependency == texture);
}

MK_TEST("runtime resident package mount set rebuilds catalog from explicit mount order") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");

    const mirakana::runtime::RuntimeAssetPackage base({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 100,
            .source_revision = 1,
            .dependencies = {},
            .content = "base-texture",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/player.mesh",
            .content_hash = 200,
            .source_revision = 1,
            .dependencies = {texture},
            .content = "base-mesh",
        },
    });
    const mirakana::runtime::RuntimeAssetPackage overlay({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 9},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 101,
            .source_revision = 2,
            .dependencies = {},
            .content = "overlay-texture",
        },
    });

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = base,
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "overlay",
                       .package = overlay,
                   })
                   .succeeded());
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(mount_set.generation() == 2);

    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    const auto build = mirakana::runtime::build_runtime_resource_catalog_v2_from_resident_mount_set(
        catalog, mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins);

    MK_REQUIRE(build.succeeded());
    MK_REQUIRE(build.catalog_build.succeeded());
    MK_REQUIRE(build.mounted_package_count == 2);
    MK_REQUIRE(build.mount_generation == mount_set.generation());
    const auto handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(handle_opt.has_value());
    const auto* tex_record = mirakana::runtime::runtime_resource_record_v2(catalog, *handle_opt);
    MK_REQUIRE(tex_record != nullptr);
    MK_REQUIRE(tex_record->content_hash == 101);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog, mesh).has_value());
}

MK_TEST("runtime resident package mount set rejects duplicate invalid and missing ids") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const mirakana::runtime::RuntimeAssetPackage package({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = "texture",
        },
    });

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    const auto invalid = mount_set.mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
        .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{},
        .label = "invalid",
        .package = package,
    });
    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(invalid.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::invalid_mount_id);
    MK_REQUIRE(invalid.diagnostic.code == "invalid-mount-id");
    MK_REQUIRE(mount_set.mounts().empty());
    MK_REQUIRE(mount_set.generation() == 0);

    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
                       .label = "base",
                       .package = package,
                   })
                   .succeeded());
    const std::uint32_t generation_after_mount = mount_set.generation();
    const auto duplicate = mount_set.mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
        .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
        .label = "duplicate",
        .package = package,
    });
    MK_REQUIRE(!duplicate.succeeded());
    MK_REQUIRE(duplicate.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::duplicate_mount_id);
    MK_REQUIRE(duplicate.diagnostic.mount == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7});
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].label == "base");
    MK_REQUIRE(mount_set.generation() == generation_after_mount);

    const auto missing = mount_set.unmount(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 8});
    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::missing_mount_id);
    MK_REQUIRE(missing.diagnostic.code == "missing-mount-id");
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.generation() == generation_after_mount);
}

MK_TEST("runtime resident package mount set unmounts package and invalidates catalog handles on rebuild") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const mirakana::runtime::RuntimeAssetPackage base({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = "texture",
        },
    });
    const mirakana::runtime::RuntimeAssetPackage overlay({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/player.material",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {texture},
            .content = "material",
        },
    });

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = base,
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "overlay",
                       .package = overlay,
                   })
                   .succeeded());

    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2_from_resident_mount_set(
                   catalog, mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins)
                   .succeeded());
    const auto stale_handle_opt = mirakana::runtime::find_runtime_resource_v2(catalog, material);
    MK_REQUIRE(stale_handle_opt.has_value());
    const mirakana::runtime::RuntimeResourceHandleV2 stale_handle = *stale_handle_opt;

    const auto unmount = mount_set.unmount(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    MK_REQUIRE(unmount.succeeded());
    MK_REQUIRE(unmount.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::unmounted);
    const auto rebuild = mirakana::runtime::build_runtime_resource_catalog_v2_from_resident_mount_set(
        catalog, mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins);

    MK_REQUIRE(rebuild.succeeded());
    MK_REQUIRE(rebuild.mounted_package_count == 1);
    MK_REQUIRE(rebuild.mount_generation == mount_set.generation());
    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, stale_handle));
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog, material).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog, texture).has_value());
}

MK_TEST("estimate_runtime_asset_package_resident_bytes sums cooked record payload sizes") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    mirakana::runtime::RuntimeAssetPackage package({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = "abcd",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/player.mesh",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = "ef",
        },
    });
    MK_REQUIRE(mirakana::runtime::estimate_runtime_asset_package_resident_bytes(package) == 6);
}

MK_TEST("runtime resource residency budget execution passes when caps are unset or generous") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeAssetPackage package({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = "hello",
        },
    });
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 no_limits{};
    const auto pass0 = mirakana::runtime::evaluate_runtime_resource_residency_budget(package, no_limits);
    MK_REQUIRE(pass0.within_budget);
    MK_REQUIRE(pass0.diagnostics.empty());
    MK_REQUIRE(pass0.estimated_resident_content_bytes == 5);
    MK_REQUIRE(pass0.resident_asset_record_count == 1);

    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 generous{
        .max_resident_content_bytes = 100,
        .max_resident_asset_records = 10,
    };
    const auto pass1 = mirakana::runtime::evaluate_runtime_resource_residency_budget(package, generous);
    MK_REQUIRE(pass1.within_budget);
    MK_REQUIRE(pass1.diagnostics.empty());
}

MK_TEST("runtime resource residency budget execution fails deterministically on byte or record caps") {
    const auto a = mirakana::AssetId::from_name("a");
    const auto b = mirakana::AssetId::from_name("b");
    mirakana::runtime::RuntimeAssetPackage package({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = a,
            .kind = mirakana::AssetKind::texture,
            .path = "a.t",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = "12",
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = b,
            .kind = mirakana::AssetKind::mesh,
            .path = "b.m",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = "3",
        },
    });

    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 byte_tight{
        .max_resident_content_bytes = 2,
    };
    const auto byte_fail = mirakana::runtime::evaluate_runtime_resource_residency_budget(package, byte_tight);
    MK_REQUIRE(!byte_fail.within_budget);
    MK_REQUIRE(byte_fail.diagnostics.size() == 1);
    MK_REQUIRE(byte_fail.diagnostics[0].code == "resident-content-bytes-exceed-budget");

    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 record_tight{
        .max_resident_asset_records = 1,
    };
    const auto record_fail = mirakana::runtime::evaluate_runtime_resource_residency_budget(package, record_tight);
    MK_REQUIRE(!record_fail.within_budget);
    MK_REQUIRE(record_fail.diagnostics.size() == 1);
    MK_REQUIRE(record_fail.diagnostics[0].code == "resident-asset-record-count-exceeds-budget");

    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 both_fail{
        .max_resident_content_bytes = 1,
        .max_resident_asset_records = 1,
    };
    const auto both = mirakana::runtime::evaluate_runtime_resource_residency_budget(package, both_fail);
    MK_REQUIRE(!both.within_budget);
    MK_REQUIRE(both.diagnostics.size() == 2);
}

MK_TEST("runtime resident mount catalog budget bundle skips catalog rebuild when budget fails") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");

    mirakana::runtime::RuntimeAssetPackage seed_pkg({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = "seed-catalog",
        },
    });

    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    MK_REQUIRE(mirakana::runtime::build_runtime_resource_catalog_v2(catalog, seed_pkg).succeeded());
    const std::uint32_t generation_before = catalog.generation();
    const auto seed_handle = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(seed_handle.has_value());

    mirakana::runtime::RuntimeAssetPackage overlay({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 9},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/player.mesh",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = "too-large-for-budget",
        },
    });

    std::vector<mirakana::runtime::RuntimeAssetPackage> mounts{seed_pkg, overlay};
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = 5,
    };
    const auto bundle = mirakana::runtime::build_runtime_resource_catalog_v2_from_resident_mounts_with_budget(
        catalog, mounts, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget);

    MK_REQUIRE(!bundle.ok());
    MK_REQUIRE(!bundle.budget_execution.within_budget);
    MK_REQUIRE(!bundle.invoked_catalog_build);
    MK_REQUIRE(catalog.generation() == generation_before);
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, *seed_handle));
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog, mesh).has_value());
}

MK_TEST("runtime resident mount catalog budget bundle rebuilds catalog when budget passes") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");

    const mirakana::runtime::RuntimeAssetPackage base({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content_hash = 10,
            .source_revision = 1,
            .dependencies = {},
            .content = "a",
        },
    });
    const mirakana::runtime::RuntimeAssetPackage overlay({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{.value = 2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/player.mesh",
            .content_hash = 20,
            .source_revision = 1,
            .dependencies = {texture},
            .content = "bb",
        },
    });

    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    std::vector<mirakana::runtime::RuntimeAssetPackage> mounts{base, overlay};
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = 100,
        .max_resident_asset_records = 10,
    };
    const auto bundle = mirakana::runtime::build_runtime_resource_catalog_v2_from_resident_mounts_with_budget(
        catalog, mounts, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget);

    MK_REQUIRE(bundle.ok());
    MK_REQUIRE(bundle.budget_execution.within_budget);
    MK_REQUIRE(bundle.invoked_catalog_build);
    MK_REQUIRE(bundle.catalog_build.succeeded());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog, texture).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog, mesh).has_value());
}

int main() {
    return mirakana::test::run_all();
}
