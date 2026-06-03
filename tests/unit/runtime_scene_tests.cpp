// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/runtime/procedural_generation.hpp"
#include "mirakana/runtime_scene/runtime_scene.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord make_record(mirakana::runtime::RuntimeAssetHandle handle,
                                                                mirakana::AssetId asset, mirakana::AssetKind kind,
                                                                std::string path, std::string content,
                                                                std::vector<mirakana::AssetId> dependencies = {}) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = kind,
        .path = std::move(path),
        .content_hash = handle.value,
        .source_revision = 1,
        .dependencies = std::move(dependencies),
        .content = std::move(content),
    };
}

[[nodiscard]] mirakana::Scene make_mesh_scene(mirakana::AssetId mesh, mirakana::AssetId material) {
    mirakana::Scene scene("GameplayLevel");
    const auto node = scene.create_node("Player");

    mirakana::SceneNodeComponents components;
    components.mesh_renderer = mirakana::MeshRendererComponent{.mesh = mesh, .material = material, .visible = true};
    scene.set_components(node, components);

    return scene;
}

[[nodiscard]] mirakana::Scene make_mesh_and_sprite_scene(mirakana::AssetId mesh, mirakana::AssetId material,
                                                         mirakana::AssetId sprite) {
    auto scene = make_mesh_scene(mesh, material);
    const auto sprite_node = scene.create_node("Nameplate");

    mirakana::SceneNodeComponents components;
    components.sprite_renderer = mirakana::SpriteRendererComponent{.sprite = sprite,
                                                                   .material = material,
                                                                   .size = mirakana::Vec2{.x = 2.0F, .y = 1.0F},
                                                                   .tint = {1.0F, 1.0F, 1.0F, 1.0F},
                                                                   .visible = true};
    scene.set_components(sprite_node, components);

    return scene;
}

[[nodiscard]] mirakana::Scene make_environment_scene(mirakana::AssetId environment) {
    mirakana::Scene scene("WeatherLevel");
    scene.set_environment(mirakana::SceneEnvironmentReference{.profile = environment, .required = true});
    return scene;
}

[[nodiscard]] mirakana::AssetId asset_id_from_test_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

[[nodiscard]] const mirakana::runtime_scene::RuntimeSceneAssetIdentityReferenceRow*
find_identity_reference(const mirakana::runtime_scene::RuntimeSceneAssetIdentityAudit& audit,
                        std::string_view placement, mirakana::SceneNodeId node) {
    for (const auto& row : audit.references) {
        if (row.placement == placement && row.node == node) {
            return &row;
        }
    }
    return nullptr;
}

[[nodiscard]] const mirakana::runtime_scene::RuntimeSceneGameplayBindingDiagnostic*
find_gameplay_binding_diagnostic(const mirakana::runtime_scene::RuntimeSceneGameplayBindingResolution& resolution,
                                 mirakana::runtime_scene::RuntimeSceneGameplayBindingDiagnosticCode code,
                                 std::string_view binding_id) {
    for (const auto& diagnostic : resolution.diagnostics) {
        if (diagnostic.code == code && diagnostic.binding_id == binding_id) {
            return &diagnostic;
        }
    }
    return nullptr;
}

[[nodiscard]] const mirakana::runtime_scene::RuntimeSceneGameplayInteractionDiagnostic*
find_gameplay_interaction_diagnostic(const mirakana::runtime_scene::RuntimeSceneGameplayInteractionPlan& plan,
                                     mirakana::runtime_scene::RuntimeSceneGameplayInteractionDiagnosticCode code,
                                     std::string_view action_id) {
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code && diagnostic.action_id == action_id) {
            return &diagnostic;
        }
    }
    return nullptr;
}

[[nodiscard]] const mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDiagnostic*
find_construction_placement_intent_diagnostic(
    const mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentPlan& plan,
    mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDiagnosticCode code,
    std::string_view procedural_output_id) {
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code && diagnostic.procedural_output_id == procedural_output_id) {
            return &diagnostic;
        }
    }
    return nullptr;
}

[[nodiscard]] mirakana::runtime::RuntimeItemCatalogDocument construction_item_catalog_document() {
    using namespace mirakana::runtime;

    return RuntimeItemCatalogDocument{
        .items =
            std::vector<RuntimeItemDesc>{
                RuntimeItemDesc{.id = "wood",
                                .localization_key = "item.wood",
                                .category_id = "material",
                                .tag_ids = {"crafting"},
                                .max_stack = 99U,
                                .placement_id = {},
                                .placement_costs = {}},
                RuntimeItemDesc{.id = "workbench",
                                .localization_key = "item.workbench",
                                .category_id = "station",
                                .tag_ids = {"placeable"},
                                .max_stack = 1U,
                                .placement_id = "grid_2d",
                                .placement_costs =
                                    std::vector<RuntimeItemCostDesc>{
                                        RuntimeItemCostDesc{.item_id = "wood", .quantity = 3U},
                                    }},
            },
    };
}

[[nodiscard]] mirakana::runtime::RuntimeConstructionPlacementValidationResult
valid_construction_placement_validation() {
    using namespace mirakana::runtime;

    static const std::vector<std::string> placement_ids{"grid_2d"};
    static const std::vector<RuntimeConstructionPlacementSurfaceDesc> surfaces{
        RuntimeConstructionPlacementSurfaceDesc{.id = "floor", .placement_id = "grid_2d"},
    };
    const std::vector<RuntimeConstructionPlacementCandidateDesc> candidates{
        RuntimeConstructionPlacementCandidateDesc{
            .item_id = "workbench",
            .surface_id = "floor",
            .grid_x = 4.0F,
            .grid_y = 7.0F,
            .grid_z = 0.0F,
            .world_x = 4.5F,
            .world_y = 7.5F,
            .world_z = 0.0F,
            .footprint_width = 2U,
            .footprint_height = 1U,
            .footprint_depth = 1U,
            .occupied_cells =
                std::vector<RuntimeConstructionPlacementCellDesc>{
                    RuntimeConstructionPlacementCellDesc{.x = 4, .y = 7, .z = 0},
                    RuntimeConstructionPlacementCellDesc{.x = 5, .y = 7, .z = 0},
                },
            .provided_costs =
                std::vector<RuntimeItemCostDesc>{
                    RuntimeItemCostDesc{.item_id = "wood", .quantity = 3U},
                },
        },
    };

    return validate_runtime_construction_placement(
        construction_item_catalog_document(), candidates,
        RuntimeConstructionPlacementValidationContext{
            .supported_placement_ids = std::span<const std::string>{placement_ids},
            .supported_surfaces = std::span<const RuntimeConstructionPlacementSurfaceDesc>{surfaces},
        });
}

[[nodiscard]] mirakana::SceneNodeComponents placement_sprite_components() {
    mirakana::SceneNodeComponents components;
    components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId::from_name("sprites/workbench"),
        .material = mirakana::AssetId::from_name("materials/workbench"),
        .size = mirakana::Vec2{.x = 2.0F, .y = 1.0F},
        .tint = {1.0F, 1.0F, 1.0F, 1.0F},
        .visible = true,
    };
    return components;
}

[[nodiscard]] mirakana::runtime::RuntimeProceduralGenerationPlan valid_procedural_generation_plan() {
    using namespace mirakana::runtime;

    return RuntimeProceduralGenerationPlan{
        .succeeded = true,
        .diagnostics = {},
        .rows =
            std::vector<RuntimeProceduralGenerationOutputRow>{
                RuntimeProceduralGenerationOutputRow{
                    .id = "prop:0",
                    .content_id = "prop",
                    .kind = RuntimeProceduralGenerationContentKind::object,
                    .index = 0U,
                    .x = 4U,
                    .y = 7U,
                    .stable_value = 0x1111ULL,
                },
                RuntimeProceduralGenerationOutputRow{
                    .id = "prop:1",
                    .content_id = "prop",
                    .kind = RuntimeProceduralGenerationContentKind::object,
                    .index = 1U,
                    .x = 4U,
                    .y = 7U,
                    .stable_value = 0x2222ULL,
                },
                RuntimeProceduralGenerationOutputRow{
                    .id = "prop:2",
                    .content_id = "prop",
                    .kind = RuntimeProceduralGenerationContentKind::object,
                    .index = 2U,
                    .x = 4U,
                    .y = 7U,
                    .stable_value = 0x3333ULL,
                },
            },
        .replay_hash = 0x4444ULL,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeProceduralGenerationPlan invalid_procedural_generation_bridge_plan() {
    using namespace mirakana::runtime;

    auto plan = valid_procedural_generation_plan();
    plan.rows.push_back(RuntimeProceduralGenerationOutputRow{
        .id = "tile:0",
        .content_id = "terrain",
        .kind = RuntimeProceduralGenerationContentKind::map_tile,
        .index = 0U,
        .x = 1U,
        .y = 1U,
        .stable_value = 0x5555ULL,
    });
    plan.rows.push_back(RuntimeProceduralGenerationOutputRow{
        .id = "loot:0",
        .content_id = "loot",
        .kind = RuntimeProceduralGenerationContentKind::loot,
        .index = 0U,
        .x = 2U,
        .y = 3U,
        .stable_value = 0x6666ULL,
    });
    return plan;
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_scene_package(mirakana::AssetId scene_asset, const mirakana::Scene& scene,
                   std::vector<mirakana::runtime::RuntimeAssetRecord> records) {
    records.push_back(make_record(mirakana::runtime::RuntimeAssetHandle{99}, scene_asset, mirakana::AssetKind::scene,
                                  "assets/scenes/gameplay.scene", mirakana::serialize_scene(scene), {}));
    return mirakana::runtime::RuntimeAssetPackage(std::move(records));
}

} // namespace

MK_TEST("runtime scene instantiates cooked scene payloads and records component references") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/gameplay");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto sprite = mirakana::AssetId::from_name("textures/nameplate");
    const auto source_scene = make_mesh_and_sprite_scene(mesh, material, sprite);
    const auto package =
        make_scene_package(scene_asset, source_scene,
                           {
                               make_record(mirakana::runtime::RuntimeAssetHandle{1}, mesh, mirakana::AssetKind::mesh,
                                           "assets/meshes/player.mesh", {}),
                               make_record(mirakana::runtime::RuntimeAssetHandle{2}, material,
                                           mirakana::AssetKind::material, "assets/materials/player.material", {}),
                               make_record(mirakana::runtime::RuntimeAssetHandle{3}, sprite,
                                           mirakana::AssetKind::texture, "assets/textures/nameplate.texture", {}),
                           });

    const auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.instance->scene_asset == scene_asset);
    MK_REQUIRE(result.instance->handle == mirakana::runtime::RuntimeAssetHandle{99});
    MK_REQUIRE(result.instance->scene.name() == "GameplayLevel");
    MK_REQUIRE(result.instance->references.size() == 4);
    MK_REQUIRE(result.instance->references[0].kind == mirakana::runtime_scene::RuntimeSceneReferenceKind::mesh);
    MK_REQUIRE(result.instance->references[0].asset == mesh);
    MK_REQUIRE(result.instance->references[0].expected_kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(result.instance->references[1].kind == mirakana::runtime_scene::RuntimeSceneReferenceKind::material);
    MK_REQUIRE(result.instance->references[1].asset == material);
    MK_REQUIRE(result.instance->references[2].kind == mirakana::runtime_scene::RuntimeSceneReferenceKind::sprite);
    MK_REQUIRE(result.instance->references[2].asset == sprite);
    MK_REQUIRE(result.instance->references[2].expected_kind == mirakana::AssetKind::texture);
    MK_REQUIRE(result.instance->references[3].kind == mirakana::runtime_scene::RuntimeSceneReferenceKind::material);
}

MK_TEST("runtime scene validates environment profile references") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/weather");
    const auto environment = mirakana::AssetId::from_name("environment/default_outdoor");
    const auto package = make_scene_package(
        scene_asset, make_environment_scene(environment),
        {
            make_record(mirakana::runtime::RuntimeAssetHandle{1}, environment, mirakana::AssetKind::environment_profile,
                        "assets/environment/default_outdoor.geenv", "format=GameEngine.CookedEnvironmentProfile.v1\n"),
        });

    const auto result = mirakana::runtime_scene::instantiate_runtime_scene(
        package, scene_asset,
        mirakana::runtime_scene::RuntimeSceneLoadOptions{.validate_asset_references = true,
                                                         .require_unique_node_names = false,
                                                         .require_environment_profile = true});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.instance->scene.environment().has_value());
    MK_REQUIRE(result.instance->references.size() == 1);
    MK_REQUIRE(result.instance->references[0].node == mirakana::null_scene_node);
    MK_REQUIRE(result.instance->references[0].kind ==
               mirakana::runtime_scene::RuntimeSceneReferenceKind::environment_profile);
    MK_REQUIRE(result.instance->references[0].asset == environment);
    MK_REQUIRE(result.instance->references[0].expected_kind == mirakana::AssetKind::environment_profile);
}

MK_TEST("runtime scene reports missing required environment profiles without implicit defaults") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/no_environment");
    const auto package = make_scene_package(scene_asset, mirakana::Scene("NoEnvironment"), {});

    const auto result = mirakana::runtime_scene::instantiate_runtime_scene(
        package, scene_asset,
        mirakana::runtime_scene::RuntimeSceneLoadOptions{.validate_asset_references = true,
                                                         .require_unique_node_names = false,
                                                         .require_environment_profile = true});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.instance->references.empty());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code ==
               mirakana::runtime_scene::RuntimeSceneDiagnosticCode::missing_environment_profile);
    MK_REQUIRE(result.diagnostics[0].reference_kind ==
               mirakana::runtime_scene::RuntimeSceneReferenceKind::environment_profile);
    MK_REQUIRE(result.diagnostics[0].expected_kind == mirakana::AssetKind::environment_profile);
}

MK_TEST("runtime scene reports environment profile kind mismatches") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/wrong_environment");
    const auto environment = mirakana::AssetId::from_name("environment/not_a_profile");
    const auto package = make_scene_package(
        scene_asset, make_environment_scene(environment),
        {
            make_record(mirakana::runtime::RuntimeAssetHandle{1}, environment, mirakana::AssetKind::material,
                        "assets/materials/not_a_profile.material", "material"),
        });

    const auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code ==
               mirakana::runtime_scene::RuntimeSceneDiagnosticCode::referenced_asset_kind_mismatch);
    MK_REQUIRE(result.diagnostics[0].reference_kind ==
               mirakana::runtime_scene::RuntimeSceneReferenceKind::environment_profile);
    MK_REQUIRE(result.diagnostics[0].expected_kind == mirakana::AssetKind::environment_profile);
    MK_REQUIRE(result.diagnostics[0].actual_kind == mirakana::AssetKind::material);
}

MK_TEST("runtime scene audits asset identity keys for component references") {
    const auto mesh = asset_id_from_test_key("meshes/player");
    const auto material = asset_id_from_test_key("materials/player");
    const auto sprite = asset_id_from_test_key("sprites/nameplate");
    const auto source_scene = make_mesh_and_sprite_scene(mesh, material, sprite);
    const mirakana::AssetIdentityDocumentV2 identities{.assets = {
                                                           {.key = {.value = "meshes/player"},
                                                            .kind = mirakana::AssetKind::mesh,
                                                            .source_path = "source/meshes/player.mesh"},
                                                           {.key = {.value = "materials/player"},
                                                            .kind = mirakana::AssetKind::material,
                                                            .source_path = "source/materials/player.material"},
                                                           {.key = {.value = "sprites/nameplate"},
                                                            .kind = mirakana::AssetKind::texture,
                                                            .source_path = "source/textures/nameplate.texture"},
                                                       }};

    const auto audit = mirakana::runtime_scene::audit_runtime_scene_asset_identity(source_scene, identities);

    MK_REQUIRE(audit.diagnostics.empty());
    MK_REQUIRE(audit.references.size() == 4);

    const auto* mesh_row =
        find_identity_reference(audit, "scene.component.mesh_renderer.mesh", mirakana::SceneNodeId{1});
    MK_REQUIRE(mesh_row != nullptr);
    MK_REQUIRE(mesh_row->asset == mesh);
    MK_REQUIRE(mesh_row->key.value == "meshes/player");
    MK_REQUIRE(mesh_row->expected_kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(mesh_row->actual_kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(mesh_row->source_path == "source/meshes/player.mesh");

    const auto* mesh_material_row =
        find_identity_reference(audit, "scene.component.mesh_renderer.material", mirakana::SceneNodeId{1});
    MK_REQUIRE(mesh_material_row != nullptr);
    MK_REQUIRE(mesh_material_row->asset == material);
    MK_REQUIRE(mesh_material_row->key.value == "materials/player");

    const auto* sprite_row =
        find_identity_reference(audit, "scene.component.sprite_renderer.sprite", mirakana::SceneNodeId{2});
    MK_REQUIRE(sprite_row != nullptr);
    MK_REQUIRE(sprite_row->asset == sprite);
    MK_REQUIRE(sprite_row->key.value == "sprites/nameplate");
    MK_REQUIRE(sprite_row->expected_kind == mirakana::AssetKind::texture);

    const auto* sprite_material_row =
        find_identity_reference(audit, "scene.component.sprite_renderer.material", mirakana::SceneNodeId{2});
    MK_REQUIRE(sprite_material_row != nullptr);
    MK_REQUIRE(sprite_material_row->asset == material);
}

MK_TEST("runtime scene asset identity audit records environment profile references") {
    const auto environment = asset_id_from_test_key("environment/default_outdoor");
    const auto source_scene = make_environment_scene(environment);
    const mirakana::AssetIdentityDocumentV2 identities{.assets = {
                                                           {.key = {.value = "environment/default_outdoor"},
                                                            .kind = mirakana::AssetKind::environment_profile,
                                                            .source_path = "source/environment/default_outdoor.geenv"},
                                                       }};

    const auto audit = mirakana::runtime_scene::audit_runtime_scene_asset_identity(source_scene, identities);

    MK_REQUIRE(audit.diagnostics.empty());
    MK_REQUIRE(audit.references.size() == 1);
    const auto* environment_row =
        find_identity_reference(audit, "scene.environment.profile", mirakana::null_scene_node);
    MK_REQUIRE(environment_row != nullptr);
    MK_REQUIRE(environment_row->asset == environment);
    MK_REQUIRE(environment_row->key.value == "environment/default_outdoor");
    MK_REQUIRE(environment_row->expected_kind == mirakana::AssetKind::environment_profile);
    MK_REQUIRE(environment_row->actual_kind == mirakana::AssetKind::environment_profile);
    MK_REQUIRE(environment_row->source_path == "source/environment/default_outdoor.geenv");
}

MK_TEST("runtime scene asset identity audit reports missing and wrong-kind rows") {
    const auto mesh = asset_id_from_test_key("meshes/player");
    const auto material = asset_id_from_test_key("materials/missing");
    const auto sprite = asset_id_from_test_key("sprites/nameplate");
    const auto source_scene = make_mesh_and_sprite_scene(mesh, material, sprite);
    const mirakana::AssetIdentityDocumentV2 identities{.assets = {
                                                           {.key = {.value = "meshes/player"},
                                                            .kind = mirakana::AssetKind::mesh,
                                                            .source_path = "source/meshes/player.mesh"},
                                                           {.key = {.value = "sprites/nameplate"},
                                                            .kind = mirakana::AssetKind::material,
                                                            .source_path = "source/materials/nameplate.material"},
                                                       }};

    const auto audit = mirakana::runtime_scene::audit_runtime_scene_asset_identity(source_scene, identities);

    MK_REQUIRE(audit.references.size() == 1);
    MK_REQUIRE(audit.diagnostics.size() == 3);
    MK_REQUIRE(audit.diagnostics[0].code ==
               mirakana::runtime_scene::RuntimeSceneAssetIdentityDiagnosticCode::missing_identity);
    MK_REQUIRE(audit.diagnostics[0].placement == "scene.component.mesh_renderer.material");
    MK_REQUIRE(audit.diagnostics[0].asset == material);
    MK_REQUIRE(audit.diagnostics[0].node == mirakana::SceneNodeId{1});
    MK_REQUIRE(audit.diagnostics[0].expected_kind == mirakana::AssetKind::material);
    MK_REQUIRE(audit.diagnostics[0].actual_kind == mirakana::AssetKind::unknown);
    MK_REQUIRE(audit.diagnostics[1].code ==
               mirakana::runtime_scene::RuntimeSceneAssetIdentityDiagnosticCode::kind_mismatch);
    MK_REQUIRE(audit.diagnostics[1].placement == "scene.component.sprite_renderer.sprite");
    MK_REQUIRE(audit.diagnostics[1].asset == sprite);
    MK_REQUIRE(audit.diagnostics[1].expected_kind == mirakana::AssetKind::texture);
    MK_REQUIRE(audit.diagnostics[1].actual_kind == mirakana::AssetKind::material);
    MK_REQUIRE(audit.diagnostics[1].key.value == "sprites/nameplate");
    MK_REQUIRE(audit.diagnostics[2].code ==
               mirakana::runtime_scene::RuntimeSceneAssetIdentityDiagnosticCode::missing_identity);
    MK_REQUIRE(audit.diagnostics[2].placement == "scene.component.sprite_renderer.material");
}

MK_TEST("runtime scene asset identity audit rejects invalid identity documents") {
    const auto mesh = asset_id_from_test_key("meshes/player");
    const auto material = asset_id_from_test_key("materials/player");
    const auto source_scene = make_mesh_scene(mesh, material);
    const mirakana::AssetIdentityDocumentV2 identities{.assets = {
                                                           {.key = {.value = "meshes/player"},
                                                            .kind = mirakana::AssetKind::mesh,
                                                            .source_path = "source/meshes/player.mesh"},
                                                           {.key = {.value = "meshes/player"},
                                                            .kind = mirakana::AssetKind::mesh,
                                                            .source_path = "source/meshes/player-copy.mesh"},
                                                           {.key = {.value = "materials/player"},
                                                            .kind = mirakana::AssetKind::material,
                                                            .source_path = "source/materials/player.material"},
                                                       }};

    const auto audit = mirakana::runtime_scene::audit_runtime_scene_asset_identity(source_scene, identities);

    MK_REQUIRE(audit.references.empty());
    MK_REQUIRE(audit.diagnostics.size() == 1);
    MK_REQUIRE(audit.diagnostics[0].code ==
               mirakana::runtime_scene::RuntimeSceneAssetIdentityDiagnosticCode::invalid_identity_document);
    MK_REQUIRE(audit.diagnostics[0].placement == "asset_identity.document");
    MK_REQUIRE(audit.diagnostics[0].key.value == "meshes/player");
    MK_REQUIRE(audit.diagnostics[0].source_path == "source/meshes/player-copy.mesh");
}

MK_TEST("runtime scene asset identity audit accepts skinned meshes for mesh references") {
    const auto mesh = asset_id_from_test_key("meshes/player");
    const auto material = asset_id_from_test_key("materials/player");
    const auto source_scene = make_mesh_scene(mesh, material);
    const mirakana::AssetIdentityDocumentV2 identities{.assets = {
                                                           {.key = {.value = "meshes/player"},
                                                            .kind = mirakana::AssetKind::skinned_mesh,
                                                            .source_path = "source/meshes/player.mesh"},
                                                           {.key = {.value = "materials/player"},
                                                            .kind = mirakana::AssetKind::material,
                                                            .source_path = "source/materials/player.material"},
                                                       }};

    const auto audit = mirakana::runtime_scene::audit_runtime_scene_asset_identity(source_scene, identities);

    MK_REQUIRE(audit.diagnostics.empty());
    MK_REQUIRE(audit.references.size() == 2);
    const auto* mesh_row =
        find_identity_reference(audit, "scene.component.mesh_renderer.mesh", mirakana::SceneNodeId{1});
    MK_REQUIRE(mesh_row != nullptr);
    MK_REQUIRE(mesh_row->expected_kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(mesh_row->actual_kind == mirakana::AssetKind::skinned_mesh);
}

MK_TEST("runtime scene reports missing scene assets") {
    const mirakana::runtime::RuntimeAssetPackage package;

    const auto result =
        mirakana::runtime_scene::instantiate_runtime_scene(package, mirakana::AssetId::from_name("scenes/missing"));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.instance.has_value());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::runtime_scene::RuntimeSceneDiagnosticCode::missing_scene_asset);
}

MK_TEST("runtime scene reports wrong scene asset kinds") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/wrong_kind");
    const mirakana::runtime::RuntimeAssetPackage package(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        make_record(mirakana::runtime::RuntimeAssetHandle{1}, scene_asset, mirakana::AssetKind::texture,
                    "assets/textures/wrong.texture", {}),
    });

    const auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.instance.has_value());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::runtime_scene::RuntimeSceneDiagnosticCode::wrong_asset_kind);
    MK_REQUIRE(result.diagnostics[0].actual_kind == mirakana::AssetKind::texture);
    MK_REQUIRE(result.diagnostics[0].expected_kind == mirakana::AssetKind::scene);
}

MK_TEST("runtime scene reports malformed scene payloads without throwing") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/broken");
    const mirakana::runtime::RuntimeAssetPackage package(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        make_record(mirakana::runtime::RuntimeAssetHandle{1}, scene_asset, mirakana::AssetKind::scene,
                    "assets/scenes/broken.scene", "format=GameEngine.Scene.v1\nscene.name=Broken\nnode.count=1\n", {}),
    });

    const auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.instance.has_value());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code ==
               mirakana::runtime_scene::RuntimeSceneDiagnosticCode::malformed_scene_payload);
}

MK_TEST("runtime scene reports missing referenced assets while preserving the scene instance") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/missing_refs");
    const auto mesh = mirakana::AssetId::from_name("meshes/missing");
    const auto material = mirakana::AssetId::from_name("materials/missing");
    const auto package = make_scene_package(scene_asset, make_mesh_scene(mesh, material), {});

    const auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.instance->references.size() == 2);
    MK_REQUIRE(result.diagnostics.size() == 2);
    MK_REQUIRE(result.diagnostics[0].code ==
               mirakana::runtime_scene::RuntimeSceneDiagnosticCode::missing_referenced_asset);
    MK_REQUIRE(result.diagnostics[0].asset == mesh);
    MK_REQUIRE(result.diagnostics[1].code ==
               mirakana::runtime_scene::RuntimeSceneDiagnosticCode::missing_referenced_asset);
    MK_REQUIRE(result.diagnostics[1].asset == material);
}

MK_TEST("runtime scene reports repeated missing references per scene node") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/repeated_missing_refs");
    const auto mesh = mirakana::AssetId::from_name("meshes/shared_missing");
    const auto material = mirakana::AssetId::from_name("materials/shared_missing");
    mirakana::Scene scene("RepeatedMissing");
    for (int index = 0; index < 2; ++index) {
        const auto node = scene.create_node("Actor");
        mirakana::SceneNodeComponents components;
        components.mesh_renderer = mirakana::MeshRendererComponent{.mesh = mesh, .material = material, .visible = true};
        scene.set_components(node, components);
    }
    const auto package = make_scene_package(scene_asset, scene, {});

    const auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.diagnostics.size() == 4);
    MK_REQUIRE(result.diagnostics[0].node == mirakana::SceneNodeId{1});
    MK_REQUIRE(result.diagnostics[1].node == mirakana::SceneNodeId{1});
    MK_REQUIRE(result.diagnostics[2].node == mirakana::SceneNodeId{2});
    MK_REQUIRE(result.diagnostics[3].node == mirakana::SceneNodeId{2});
}

MK_TEST("runtime scene reports referenced asset kind mismatches") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/wrong_refs");
    const auto mesh = mirakana::AssetId::from_name("meshes/not_a_mesh");
    const auto material = mirakana::AssetId::from_name("materials/not_a_material");
    const auto package =
        make_scene_package(scene_asset, make_mesh_scene(mesh, material),
                           {
                               make_record(mirakana::runtime::RuntimeAssetHandle{1}, mesh,
                                           mirakana::AssetKind::material, "assets/materials/not_a_mesh.material", {}),
                               make_record(mirakana::runtime::RuntimeAssetHandle{2}, material,
                                           mirakana::AssetKind::texture, "assets/textures/not_a_material.texture", {}),
                           });

    const auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.diagnostics.size() == 2);
    MK_REQUIRE(result.diagnostics[0].code ==
               mirakana::runtime_scene::RuntimeSceneDiagnosticCode::referenced_asset_kind_mismatch);
    MK_REQUIRE(result.diagnostics[0].expected_kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(result.diagnostics[0].actual_kind == mirakana::AssetKind::material);
    MK_REQUIRE(result.diagnostics[1].expected_kind == mirakana::AssetKind::material);
    MK_REQUIRE(result.diagnostics[1].actual_kind == mirakana::AssetKind::texture);
}

MK_TEST("runtime scene reports sprite reference diagnostics and can skip reference validation") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/sprite_refs");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto sprite = mirakana::AssetId::from_name("textures/wrong_sprite");
    const auto package =
        make_scene_package(scene_asset, make_mesh_and_sprite_scene(mesh, material, sprite),
                           {
                               make_record(mirakana::runtime::RuntimeAssetHandle{1}, mesh, mirakana::AssetKind::mesh,
                                           "assets/meshes/player.mesh", {}),
                               make_record(mirakana::runtime::RuntimeAssetHandle{2}, material,
                                           mirakana::AssetKind::material, "assets/materials/player.material", {}),
                               make_record(mirakana::runtime::RuntimeAssetHandle{3}, sprite,
                                           mirakana::AssetKind::material, "assets/materials/wrong_sprite.material", {}),
                           });

    auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code ==
               mirakana::runtime_scene::RuntimeSceneDiagnosticCode::referenced_asset_kind_mismatch);
    MK_REQUIRE(result.diagnostics[0].reference_kind == mirakana::runtime_scene::RuntimeSceneReferenceKind::sprite);
    MK_REQUIRE(result.diagnostics[0].expected_kind == mirakana::AssetKind::texture);
    MK_REQUIRE(result.diagnostics[0].actual_kind == mirakana::AssetKind::material);

    result = mirakana::runtime_scene::instantiate_runtime_scene(
        package, scene_asset,
        mirakana::runtime_scene::RuntimeSceneLoadOptions{.validate_asset_references = false,
                                                         .require_unique_node_names = false});
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.instance->references.size() == 4);
}

MK_TEST("runtime scene node-name lookup returns duplicates in scene order") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/duplicate_names");
    mirakana::Scene scene("DuplicateNames");
    const auto first = scene.create_node("Spawn");
    const auto second = scene.create_node("Spawn");
    const auto package = make_scene_package(scene_asset, scene, {});

    auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.diagnostics.empty());

    const auto matches = mirakana::runtime_scene::find_runtime_scene_nodes_by_name(*result.instance, "Spawn");
    MK_REQUIRE(matches.size() == 2);
    MK_REQUIRE(matches[0] == first);
    MK_REQUIRE(matches[1] == second);

    result = mirakana::runtime_scene::instantiate_runtime_scene(
        package, scene_asset,
        mirakana::runtime_scene::RuntimeSceneLoadOptions{.validate_asset_references = true,
                                                         .require_unique_node_names = true});
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.instance.has_value());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::runtime_scene::RuntimeSceneDiagnosticCode::duplicate_node_name);
    MK_REQUIRE(result.diagnostics[0].asset == scene_asset);
    MK_REQUIRE(result.diagnostics[0].node == second);
}

MK_TEST("runtime scene resolves authored gameplay bindings to component-backed nodes") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/gameplay_bindings");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto sprite = mirakana::AssetId::from_name("textures/nameplate");
    auto source_scene = make_mesh_and_sprite_scene(mesh, material, sprite);
    const auto camera = source_scene.create_node("MainCamera");
    mirakana::SceneNodeComponents camera_components;
    camera_components.camera = mirakana::CameraComponent{.primary = true};
    source_scene.set_components(camera, camera_components);
    const auto package =
        make_scene_package(scene_asset, source_scene,
                           {
                               make_record(mirakana::runtime::RuntimeAssetHandle{1}, mesh, mirakana::AssetKind::mesh,
                                           "assets/meshes/player.mesh", {}),
                               make_record(mirakana::runtime::RuntimeAssetHandle{2}, material,
                                           mirakana::AssetKind::material, "assets/materials/player.material", {}),
                               make_record(mirakana::runtime::RuntimeAssetHandle{3}, sprite,
                                           mirakana::AssetKind::texture, "assets/textures/nameplate.texture", {}),
                           });

    auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.instance.has_value());

    const std::vector<mirakana::runtime_scene::RuntimeSceneGameplayBindingSourceRow> rows{
        {
            .binding_id = "player.actor",
            .gameplay_system_id = "player_controller",
            .slot_id = "actor",
            .node_name = "Player",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::mesh_renderer,
        },
        {
            .binding_id = "nameplate.ui",
            .gameplay_system_id = "nameplate_follow",
            .slot_id = "target",
            .node_name = "Nameplate",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::sprite_renderer,
        },
        {
            .binding_id = "camera.primary",
            .gameplay_system_id = "camera_follow",
            .slot_id = "camera",
            .node_name = "MainCamera",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::camera,
        },
        {
            .binding_id = "player.renderable",
            .gameplay_system_id = "visibility_gate",
            .slot_id = "renderable",
            .node_name = "Player",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::any_renderable,
        },
    };

    const auto resolution = mirakana::runtime_scene::resolve_runtime_scene_gameplay_bindings(*result.instance, rows);

    MK_REQUIRE(resolution.succeeded());
    MK_REQUIRE(resolution.diagnostics.empty());
    MK_REQUIRE(resolution.bindings.size() == 4);
    MK_REQUIRE(resolution.bindings[0].binding_id == "player.actor");
    MK_REQUIRE(resolution.bindings[0].gameplay_system_id == "player_controller");
    MK_REQUIRE(resolution.bindings[0].slot_id == "actor");
    MK_REQUIRE(resolution.bindings[0].node_name == "Player");
    MK_REQUIRE(resolution.bindings[0].node == mirakana::SceneNodeId{1});
    MK_REQUIRE(resolution.bindings[0].required_component ==
               mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::mesh_renderer);
    MK_REQUIRE(resolution.bindings[1].binding_id == "nameplate.ui");
    MK_REQUIRE(resolution.bindings[1].node == mirakana::SceneNodeId{2});
    MK_REQUIRE(resolution.bindings[2].binding_id == "camera.primary");
    MK_REQUIRE(resolution.bindings[2].node == camera);
    MK_REQUIRE(resolution.bindings[3].binding_id == "player.renderable");
    MK_REQUIRE(resolution.bindings[3].required_component ==
               mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::any_renderable);
}

MK_TEST("runtime scene gameplay bindings fail closed for invalid ambiguous and missing component rows") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/gameplay_binding_diagnostics");
    mirakana::Scene scene("GameplayBindingDiagnostics");
    const auto door = scene.create_node("Door");
    const auto first_pickup = scene.create_node("Pickup");
    mirakana::SceneNodeComponents pickup_components;
    pickup_components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mirakana::AssetId::from_name("meshes/pickup"),
                                        .material = mirakana::AssetId::from_name("materials/pickup"),
                                        .visible = true};
    scene.set_components(first_pickup, pickup_components);
    (void)scene.create_node("Pickup");
    const auto package = make_scene_package(scene_asset, scene, {});

    auto result = mirakana::runtime_scene::instantiate_runtime_scene(
        package, scene_asset,
        mirakana::runtime_scene::RuntimeSceneLoadOptions{.validate_asset_references = false,
                                                         .require_unique_node_names = false});
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.instance.has_value());

    const std::vector<mirakana::runtime_scene::RuntimeSceneGameplayBindingSourceRow> rows{
        {
            .binding_id = "",
            .gameplay_system_id = "interaction",
            .slot_id = "target",
            .node_name = "Door",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::none,
        },
        {
            .binding_id = "invalid.system",
            .gameplay_system_id = "",
            .slot_id = "target",
            .node_name = "Door",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::none,
        },
        {
            .binding_id = "invalid.slot",
            .gameplay_system_id = "interaction",
            .slot_id = "",
            .node_name = "Door",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::none,
        },
        {
            .binding_id = "duplicate.pickup",
            .gameplay_system_id = "interaction",
            .slot_id = "candidate",
            .node_name = "Pickup",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::mesh_renderer,
        },
        {
            .binding_id = "duplicate.pickup",
            .gameplay_system_id = "interaction",
            .slot_id = "candidate-copy",
            .node_name = "Door",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::none,
        },
        {
            .binding_id = "missing.node",
            .gameplay_system_id = "interaction",
            .slot_id = "target",
            .node_name = "Missing",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::none,
        },
        {
            .binding_id = "door.mesh",
            .gameplay_system_id = "interaction",
            .slot_id = "door",
            .node_name = "Door",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::mesh_renderer,
        },
    };

    const auto resolution = mirakana::runtime_scene::resolve_runtime_scene_gameplay_bindings(*result.instance, rows);

    MK_REQUIRE(!resolution.succeeded());
    MK_REQUIRE(resolution.bindings.empty());
    MK_REQUIRE(find_gameplay_binding_diagnostic(
                   resolution, mirakana::runtime_scene::RuntimeSceneGameplayBindingDiagnosticCode::invalid_binding_id,
                   "") != nullptr);
    MK_REQUIRE(find_gameplay_binding_diagnostic(
                   resolution,
                   mirakana::runtime_scene::RuntimeSceneGameplayBindingDiagnosticCode::invalid_gameplay_system_id,
                   "invalid.system") != nullptr);
    MK_REQUIRE(find_gameplay_binding_diagnostic(
                   resolution, mirakana::runtime_scene::RuntimeSceneGameplayBindingDiagnosticCode::invalid_slot_id,
                   "invalid.slot") != nullptr);
    MK_REQUIRE(find_gameplay_binding_diagnostic(
                   resolution, mirakana::runtime_scene::RuntimeSceneGameplayBindingDiagnosticCode::duplicate_binding_id,
                   "duplicate.pickup") != nullptr);
    MK_REQUIRE(find_gameplay_binding_diagnostic(
                   resolution, mirakana::runtime_scene::RuntimeSceneGameplayBindingDiagnosticCode::duplicate_node_name,
                   "duplicate.pickup") != nullptr);
    MK_REQUIRE(find_gameplay_binding_diagnostic(
                   resolution, mirakana::runtime_scene::RuntimeSceneGameplayBindingDiagnosticCode::missing_node,
                   "missing.node") != nullptr);
    const auto* missing_component = find_gameplay_binding_diagnostic(
        resolution, mirakana::runtime_scene::RuntimeSceneGameplayBindingDiagnosticCode::missing_required_component,
        "door.mesh");
    MK_REQUIRE(missing_component != nullptr);
    MK_REQUIRE(missing_component->node == door);
}

MK_TEST("runtime scene gameplay interaction plan composes binding rows in authored order") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/gameplay_interactions");
    const auto mesh = mirakana::AssetId::from_name("meshes/player");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto sprite = mirakana::AssetId::from_name("textures/pickup");
    auto source_scene = make_mesh_and_sprite_scene(mesh, material, sprite);
    const auto exit = source_scene.create_node("Exit");
    mirakana::SceneNodeComponents exit_components;
    exit_components.light = mirakana::LightComponent{};
    source_scene.set_components(exit, exit_components);
    const auto package =
        make_scene_package(scene_asset, source_scene,
                           {
                               make_record(mirakana::runtime::RuntimeAssetHandle{1}, mesh, mirakana::AssetKind::mesh,
                                           "assets/meshes/player.mesh", {}),
                               make_record(mirakana::runtime::RuntimeAssetHandle{2}, material,
                                           mirakana::AssetKind::material, "assets/materials/player.material", {}),
                               make_record(mirakana::runtime::RuntimeAssetHandle{3}, sprite,
                                           mirakana::AssetKind::texture, "assets/textures/pickup.texture", {}),
                           });

    auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.instance.has_value());

    const std::vector<mirakana::runtime_scene::RuntimeSceneGameplayBindingSourceRow> binding_rows{
        {
            .binding_id = "player.actor",
            .gameplay_system_id = "interaction",
            .slot_id = "actor",
            .node_name = "Player",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::mesh_renderer,
        },
        {
            .binding_id = "pickup.coin",
            .gameplay_system_id = "interaction",
            .slot_id = "pickup",
            .node_name = "Nameplate",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::sprite_renderer,
        },
        {
            .binding_id = "level.exit",
            .gameplay_system_id = "interaction",
            .slot_id = "trigger",
            .node_name = "Exit",
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::light,
        },
    };
    const auto binding_resolution =
        mirakana::runtime_scene::resolve_runtime_scene_gameplay_bindings(*result.instance, binding_rows);
    MK_REQUIRE(binding_resolution.succeeded());

    const std::vector<mirakana::runtime_scene::RuntimeSceneGameplayInteractionSourceRow> interactions{
        {
            .action_id = "pickup.coin",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::pickup,
            .source_binding_id = "player.actor",
            .target_binding_id = "pickup.coin",
            .amount = 1,
        },
        {
            .action_id = "objective.coin",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::objective_progress,
            .source_binding_id = "pickup.coin",
            .objective_id = "collect_coin",
            .amount = 1,
        },
        {
            .action_id = "exit.win",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::win,
            .source_binding_id = "level.exit",
        },
    };

    const auto plan = mirakana::runtime_scene::plan_runtime_scene_gameplay_interactions(
        binding_resolution.bindings, interactions,
        mirakana::runtime_scene::RuntimeSceneGameplayInteractionPlanRequest{
            .session_state = mirakana::runtime_scene::RuntimeSceneGameplaySessionState::running,
        });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.rows.size() == 3);
    MK_REQUIRE(plan.rows[0].action_id == "pickup.coin");
    MK_REQUIRE(plan.rows[0].kind == mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::pickup);
    MK_REQUIRE(plan.rows[0].source_node == mirakana::SceneNodeId{1});
    MK_REQUIRE(plan.rows[0].target_node == mirakana::SceneNodeId{2});
    MK_REQUIRE(plan.rows[0].amount == 1);
    MK_REQUIRE(plan.rows[0].resulting_session_state ==
               mirakana::runtime_scene::RuntimeSceneGameplaySessionState::running);
    MK_REQUIRE(plan.rows[1].action_id == "objective.coin");
    MK_REQUIRE(plan.rows[1].objective_id == "collect_coin");
    MK_REQUIRE(plan.rows[1].target_node == mirakana::null_scene_node);
    MK_REQUIRE(plan.rows[2].action_id == "exit.win");
    MK_REQUIRE(plan.rows[2].source_node == exit);
    MK_REQUIRE(plan.rows[2].resulting_session_state == mirakana::runtime_scene::RuntimeSceneGameplaySessionState::won);
    MK_REQUIRE(plan.final_session_state == mirakana::runtime_scene::RuntimeSceneGameplaySessionState::won);
}

MK_TEST("runtime scene gameplay interaction plan rejects invalid targets duplicates and terminal transitions") {
    const std::vector<mirakana::runtime_scene::RuntimeSceneGameplayBindingRow> bindings{
        {
            .binding_id = "player.actor",
            .gameplay_system_id = "interaction",
            .slot_id = "actor",
            .node_name = "Player",
            .node = mirakana::SceneNodeId{1},
            .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::mesh_renderer,
        },
    };
    const std::vector<mirakana::runtime_scene::RuntimeSceneGameplayInteractionSourceRow> interactions{
        {
            .action_id = "",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::trigger,
            .source_binding_id = "player.actor",
            .target_binding_id = "missing.trigger",
        },
        {
            .action_id = "duplicate.action",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::heal,
            .source_binding_id = "player.actor",
            .target_binding_id = "player.actor",
            .amount = 1,
        },
        {
            .action_id = "duplicate.action",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::damage,
            .source_binding_id = "player.actor",
            .target_binding_id = "player.actor",
            .amount = 1,
        },
        {
            .action_id = "missing.source",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::pickup,
            .source_binding_id = "missing.actor",
            .target_binding_id = "player.actor",
            .amount = 1,
        },
        {
            .action_id = "missing.target",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::pickup,
            .source_binding_id = "player.actor",
            .target_binding_id = "missing.pickup",
            .amount = 1,
        },
        {
            .action_id = "missing.objective",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::objective_complete,
            .source_binding_id = "player.actor",
        },
        {
            .action_id = "invalid.amount",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::damage,
            .source_binding_id = "player.actor",
            .target_binding_id = "player.actor",
            .amount = 0,
        },
        {
            .action_id = "terminal.pickup",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::pickup,
            .source_binding_id = "player.actor",
            .target_binding_id = "player.actor",
            .amount = 1,
        },
        {
            .action_id = "terminal.restart",
            .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::restart,
            .source_binding_id = "player.actor",
        },
    };

    const auto plan = mirakana::runtime_scene::plan_runtime_scene_gameplay_interactions(
        bindings, interactions,
        mirakana::runtime_scene::RuntimeSceneGameplayInteractionPlanRequest{
            .session_state = mirakana::runtime_scene::RuntimeSceneGameplaySessionState::won,
        });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(find_gameplay_interaction_diagnostic(
                   plan, mirakana::runtime_scene::RuntimeSceneGameplayInteractionDiagnosticCode::invalid_action_id,
                   "") != nullptr);
    MK_REQUIRE(find_gameplay_interaction_diagnostic(
                   plan, mirakana::runtime_scene::RuntimeSceneGameplayInteractionDiagnosticCode::duplicate_action_id,
                   "duplicate.action") != nullptr);
    MK_REQUIRE(find_gameplay_interaction_diagnostic(
                   plan, mirakana::runtime_scene::RuntimeSceneGameplayInteractionDiagnosticCode::missing_source_binding,
                   "missing.source") != nullptr);
    MK_REQUIRE(find_gameplay_interaction_diagnostic(
                   plan, mirakana::runtime_scene::RuntimeSceneGameplayInteractionDiagnosticCode::missing_target_binding,
                   "missing.target") != nullptr);
    MK_REQUIRE(find_gameplay_interaction_diagnostic(
                   plan, mirakana::runtime_scene::RuntimeSceneGameplayInteractionDiagnosticCode::missing_objective_id,
                   "missing.objective") != nullptr);
    MK_REQUIRE(find_gameplay_interaction_diagnostic(
                   plan, mirakana::runtime_scene::RuntimeSceneGameplayInteractionDiagnosticCode::invalid_amount,
                   "invalid.amount") != nullptr);
    MK_REQUIRE(find_gameplay_interaction_diagnostic(
                   plan, mirakana::runtime_scene::RuntimeSceneGameplayInteractionDiagnosticCode::rejected_transition,
                   "terminal.pickup") != nullptr);
    MK_REQUIRE(plan.final_session_state == mirakana::runtime_scene::RuntimeSceneGameplaySessionState::won);
}

MK_TEST("runtime scene resolves authored animation transform bindings and applies samples") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/animated");
    mirakana::Scene scene("AnimatedScene");
    const auto player = scene.create_node("Player");
    const auto prop = scene.create_node("Prop");
    scene.find_node(prop)->transform.rotation_radians.z = 0.25F;
    const auto package = make_scene_package(scene_asset, scene, {});

    auto result = mirakana::runtime_scene::instantiate_runtime_scene(
        package, scene_asset,
        mirakana::runtime_scene::RuntimeSceneLoadOptions{.validate_asset_references = true,
                                                         .require_unique_node_names = true});
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.instance.has_value());

    mirakana::AnimationTransformBindingSourceDocument binding_source;
    binding_source.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "gltf/node/Player/translation/x",
        .node_name = "Player",
        .component = mirakana::AnimationTransformBindingComponent::translation_x,
    });
    binding_source.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "gltf/node/Prop/rotation_z",
        .node_name = "Prop",
        .component = mirakana::AnimationTransformBindingComponent::rotation_z,
    });

    const auto resolution =
        mirakana::runtime_scene::resolve_runtime_scene_animation_transform_bindings(*result.instance, binding_source);

    MK_REQUIRE(resolution.succeeded());
    MK_REQUIRE(resolution.diagnostics.empty());
    MK_REQUIRE(resolution.bindings.size() == 2);
    MK_REQUIRE(resolution.bindings[0].target == "gltf/node/Player/translation/x");
    MK_REQUIRE(resolution.bindings[0].transform_index == 0);
    MK_REQUIRE(resolution.bindings[0].component == mirakana::AnimationTransformComponent::translation_x);
    MK_REQUIRE(resolution.bindings[1].target == "gltf/node/Prop/rotation_z");
    MK_REQUIRE(resolution.bindings[1].transform_index == 1);
    MK_REQUIRE(resolution.bindings[1].component == mirakana::AnimationTransformComponent::rotation_z);

    const std::vector<mirakana::FloatAnimationCurveSample> samples{
        mirakana::FloatAnimationCurveSample{.target = "gltf/node/Player/translation/x", .value = 4.5F},
        mirakana::FloatAnimationCurveSample{.target = "gltf/node/Prop/rotation_z", .value = 1.25F},
    };

    const auto apply_result = mirakana::runtime_scene::apply_runtime_scene_animation_transform_samples(
        *result.instance, binding_source, samples);

    MK_REQUIRE(apply_result.succeeded);
    MK_REQUIRE(apply_result.diagnostic.empty());
    MK_REQUIRE(apply_result.applied_sample_count == 2);
    MK_REQUIRE(result.instance->scene.find_node(player)->transform.position.x == 4.5F);
    MK_REQUIRE(result.instance->scene.find_node(prop)->transform.rotation_radians.z == 1.25F);
}

MK_TEST("runtime scene animation transform binding rejects missing and duplicate node names without mutation") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/ambiguous_animation");
    mirakana::Scene scene("AmbiguousAnimation");
    const auto first_actor = scene.create_node("Actor");
    (void)scene.create_node("Actor");
    scene.find_node(first_actor)->transform.position.x = 2.0F;
    const auto package = make_scene_package(scene_asset, scene, {});

    auto result = mirakana::runtime_scene::instantiate_runtime_scene(package, scene_asset);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.instance.has_value());

    mirakana::AnimationTransformBindingSourceDocument binding_source;
    binding_source.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "curve/actor/translation_x",
        .node_name = "Actor",
        .component = mirakana::AnimationTransformBindingComponent::translation_x,
    });
    binding_source.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "curve/missing/scale_x",
        .node_name = "Missing",
        .component = mirakana::AnimationTransformBindingComponent::scale_x,
    });

    const auto resolution =
        mirakana::runtime_scene::resolve_runtime_scene_animation_transform_bindings(*result.instance, binding_source);

    MK_REQUIRE(!resolution.succeeded());
    MK_REQUIRE(resolution.bindings.empty());
    MK_REQUIRE(resolution.diagnostics.size() == 2);
    MK_REQUIRE(resolution.diagnostics[0].code ==
               mirakana::runtime_scene::RuntimeSceneAnimationTransformBindingDiagnosticCode::duplicate_node_name);
    MK_REQUIRE(resolution.diagnostics[0].node_name == "Actor");
    MK_REQUIRE(resolution.diagnostics[1].code ==
               mirakana::runtime_scene::RuntimeSceneAnimationTransformBindingDiagnosticCode::missing_node);
    MK_REQUIRE(resolution.diagnostics[1].node_name == "Missing");

    const std::vector<mirakana::FloatAnimationCurveSample> samples{
        mirakana::FloatAnimationCurveSample{.target = "curve/actor/translation_x", .value = 99.0F},
        mirakana::FloatAnimationCurveSample{.target = "curve/missing/scale_x", .value = 2.0F},
    };
    const auto apply_result = mirakana::runtime_scene::apply_runtime_scene_animation_transform_samples(
        *result.instance, binding_source, samples);

    MK_REQUIRE(!apply_result.succeeded);
    MK_REQUIRE(apply_result.diagnostic.contains("animation transform binding"));
    MK_REQUIRE(result.instance->scene.find_node(first_actor)->transform.position.x == 2.0F);
}

MK_TEST("runtime scene applies sampled quaternion pose rows to named nodes") {
    const auto scene_asset = mirakana::AssetId::from_name("scenes/quaternion_pose");
    mirakana::Scene scene("QuaternionPose");
    const auto mesh = scene.create_node("PackagedMesh");
    const auto package = make_scene_package(scene_asset, scene, {});

    auto result = mirakana::runtime_scene::instantiate_runtime_scene(
        package, scene_asset,
        mirakana::runtime_scene::RuntimeSceneLoadOptions{.validate_asset_references = true,
                                                         .require_unique_node_names = true});
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.instance.has_value());

    const mirakana::AnimationSkeleton3dDesc skeleton{
        std::vector<mirakana::AnimationSkeleton3dJointDesc>{
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "PackagedMesh", .parent_index = mirakana::animation_no_parent, .rest = {}},
        },
    };
    const mirakana::AnimationPose3d pose{
        std::vector<mirakana::AnimationJointLocalTransform3d>{
            mirakana::AnimationJointLocalTransform3d{
                .translation = mirakana::Vec3{.x = 2.0F, .y = 3.0F, .z = 4.0F},
                .rotation =
                    mirakana::Quat::from_axis_angle(mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}, 1.57079637F),
                .scale = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F},
            },
        },
    };

    const auto apply_result =
        mirakana::runtime_scene::apply_runtime_scene_animation_pose_3d(*result.instance, skeleton, pose);

    MK_REQUIRE(apply_result.succeeded);
    MK_REQUIRE(apply_result.diagnostic.empty());
    MK_REQUIRE(apply_result.applied_sample_count == 1);
    MK_REQUIRE(apply_result.binding_diagnostics.empty());
    const auto* node = result.instance->scene.find_node(mesh);
    MK_REQUIRE(node != nullptr);
    MK_REQUIRE(node->transform.position == (mirakana::Vec3{2.0F, 3.0F, 4.0F}));
    MK_REQUIRE(node->transform.scale == (mirakana::Vec3{1.0F, 2.0F, 3.0F}));
    MK_REQUIRE(std::abs(node->transform.rotation_radians.y - 1.57079637F) < 0.0001F);
}

MK_TEST("runtime scene construction placement intent plans reviewed scene creation rows") {
    const auto placement = valid_construction_placement_validation();
    const std::vector<mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "Workbench",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
        },
    };

    const auto first = mirakana::runtime_scene::plan_runtime_scene_construction_placement_intents(
        placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});
    const auto second = mirakana::runtime_scene::plan_runtime_scene_construction_placement_intents(
        placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});

    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.rows.size() == second.rows.size());
    MK_REQUIRE(first.rows.size() == 1U);
    MK_REQUIRE(first.rows[0].status ==
               mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus::accepted);
    MK_REQUIRE(first.rows[0].candidate_index == 0U);
    MK_REQUIRE(first.rows[0].item_id == "workbench");
    MK_REQUIRE(first.rows[0].placement_id == "grid_2d");
    MK_REQUIRE(first.rows[0].surface_id == "floor");
    MK_REQUIRE(first.rows[0].node_name == "Workbench");
    MK_REQUIRE(first.rows[0].transform.position == (mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}));
    MK_REQUIRE(first.rows[0].components.sprite_renderer.has_value());
    MK_REQUIRE(first.rows[0].occupied_cells.size() == 2U);
    MK_REQUIRE(first.rows[0].occupied_cells[0] ==
               (mirakana::runtime::RuntimeConstructionPlacementCellDesc{.x = 4, .y = 7, .z = 0}));
}

MK_TEST("runtime scene construction placement intent classifies blocked invalid and occupied rows") {
    using Code = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDiagnosticCode;
    using Status = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus;

    const auto placement = valid_construction_placement_validation();
    auto invalid_components = placement_sprite_components();
    invalid_components.sprite_renderer->material = mirakana::AssetId{};

    const std::vector<mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "NeedsReview",
            .transform = mirakana::Transform3D{},
            .components = placement_sprite_components(),
            .reviewed = false,
        },
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "OccupiedWorkbench",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
        },
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 9U,
            .node_name = "MissingCandidate",
            .transform = mirakana::Transform3D{},
            .components = placement_sprite_components(),
            .reviewed = true,
        },
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "Bad\nName",
            .transform = mirakana::Transform3D{},
            .components = placement_sprite_components(),
            .reviewed = true,
        },
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "InvalidComponents",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = invalid_components,
            .reviewed = true,
        },
    };
    const std::vector<mirakana::runtime_scene::RuntimeSceneConstructionPlacementOccupiedCell> occupied_cells{
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementOccupiedCell{
            .cell = mirakana::runtime::RuntimeConstructionPlacementCellDesc{.x = 4, .y = 7, .z = 0},
            .node = mirakana::SceneNodeId{12},
            .node_name = "ExistingWorkbench",
        },
    };

    const auto plan = mirakana::runtime_scene::plan_runtime_scene_construction_placement_intents(
        placement, intents,
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{
            .occupied_cells =
                std::span<const mirakana::runtime_scene::RuntimeSceneConstructionPlacementOccupiedCell>{occupied_cells},
            .existing_node_names = {},
        });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.size() == 5U);
    MK_REQUIRE(plan.diagnostics.size() == 5U);
    MK_REQUIRE(plan.rows[0].status == Status::blocked);
    MK_REQUIRE(plan.diagnostics[0].code == Code::placement_not_reviewed);
    MK_REQUIRE(plan.rows[1].status == Status::already_occupied);
    MK_REQUIRE(plan.diagnostics[1].code == Code::already_occupied);
    MK_REQUIRE(plan.diagnostics[1].existing_node == mirakana::SceneNodeId{12});
    MK_REQUIRE(plan.diagnostics[1].cell_x == 4);
    MK_REQUIRE(plan.rows[2].status == Status::invalid);
    MK_REQUIRE(plan.diagnostics[2].code == Code::missing_candidate);
    MK_REQUIRE(plan.rows[3].status == Status::invalid);
    MK_REQUIRE(plan.diagnostics[3].code == Code::invalid_node_name);
    MK_REQUIRE(plan.rows[4].status == Status::invalid);
    MK_REQUIRE(plan.diagnostics[4].code == Code::invalid_components);
}

MK_TEST("runtime scene construction placement intent rejects same batch occupied cells") {
    using Code = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDiagnosticCode;
    using Status = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus;

    const auto placement = valid_construction_placement_validation();
    const std::vector<mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "WorkbenchA",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
        },
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "WorkbenchB",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
        },
    };

    const auto plan = mirakana::runtime_scene::plan_runtime_scene_construction_placement_intents(
        placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.size() == 2U);
    MK_REQUIRE(plan.diagnostics.size() == 1U);
    MK_REQUIRE(plan.rows[0].status == Status::accepted);
    MK_REQUIRE(plan.rows[1].status == Status::already_occupied);
    MK_REQUIRE(plan.diagnostics[0].code == Code::already_occupied);
    MK_REQUIRE(plan.diagnostics[0].existing_node == mirakana::null_scene_node);
    MK_REQUIRE(plan.diagnostics[0].existing_node_name == "WorkbenchA");
    MK_REQUIRE(plan.diagnostics[0].cell_x == 4);
    MK_REQUIRE(plan.diagnostics[0].cell_y == 7);
    MK_REQUIRE(plan.diagnostics[0].cell_z == 0);
}

MK_TEST("runtime scene construction placement intent rejects invalid and mismatched transforms") {
    using Code = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDiagnosticCode;
    using Status = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus;

    const auto placement = valid_construction_placement_validation();
    auto zero_scale = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}};
    zero_scale.scale.x = 0.0F;
    auto non_finite_position = mirakana::Transform3D{
        .position = mirakana::Vec3{.x = std::numeric_limits<float>::quiet_NaN(), .y = 7.5F, .z = 0.0F},
    };
    const std::vector<mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "ZeroScaleWorkbench",
            .transform = zero_scale,
            .components = placement_sprite_components(),
            .reviewed = true,
        },
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "NaNWorkbench",
            .transform = non_finite_position,
            .components = placement_sprite_components(),
            .reviewed = true,
        },
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "WrongPositionWorkbench",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 99.0F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
        },
    };

    const auto plan = mirakana::runtime_scene::plan_runtime_scene_construction_placement_intents(
        placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.size() == 3U);
    MK_REQUIRE(plan.diagnostics.size() == 3U);
    MK_REQUIRE(plan.rows[0].status == Status::invalid);
    MK_REQUIRE(plan.diagnostics[0].code == Code::invalid_transform);
    MK_REQUIRE(plan.rows[1].status == Status::invalid);
    MK_REQUIRE(plan.diagnostics[1].code == Code::invalid_transform);
    MK_REQUIRE(plan.rows[2].status == Status::invalid);
    MK_REQUIRE(plan.diagnostics[2].code == Code::mismatched_transform_position);
}

MK_TEST("runtime scene procedural construction placement bridges reviewed output rows") {
    const auto generation = valid_procedural_generation_plan();
    const auto placement = valid_construction_placement_validation();
    const std::vector<mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "prop:0",
            .anchor_id = "grid:4:7",
            .candidate_index = 0U,
            .node_name = "GeneratedWorkbench",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
    };

    const auto first = mirakana::runtime_scene::plan_runtime_scene_procedural_construction_placement_intents(
        generation, placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});
    const auto second = mirakana::runtime_scene::plan_runtime_scene_procedural_construction_placement_intents(
        generation, placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});

    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.rows.size() == second.rows.size());
    MK_REQUIRE(first.rows.size() == 1U);
    MK_REQUIRE(first.rows[0].status ==
               mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus::accepted);
    MK_REQUIRE(first.rows[0].node_name == "GeneratedWorkbench");
    MK_REQUIRE(first.rows[0].procedural_output_id == "prop:0");
    MK_REQUIRE(first.rows[0].anchor_id == "grid:4:7");
    MK_REQUIRE(first.rows[0].procedural_kind == mirakana::runtime::RuntimeProceduralGenerationContentKind::object);
    MK_REQUIRE(first.rows[0].package_visible);
    MK_REQUIRE(first.rows[0].item_id == "workbench");
    MK_REQUIRE(first.rows[0].occupied_cells.size() == 2U);
}

MK_TEST("runtime scene procedural construction placement rejects unsafe output evidence before placement") {
    using Code = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDiagnosticCode;
    using Status = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus;

    const auto generation = invalid_procedural_generation_bridge_plan();
    const auto placement = valid_construction_placement_validation();
    const std::vector<mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "missing:0",
            .anchor_id = "grid:missing",
            .candidate_index = 0U,
            .node_name = "MissingOutput",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "tile:0",
            .anchor_id = "grid:tile",
            .candidate_index = 0U,
            .node_name = "TileOutput",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "prop:0",
            .anchor_id = {},
            .candidate_index = 0U,
            .node_name = "MissingAnchor",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "prop:0",
            .anchor_id = "grid:duplicate",
            .candidate_index = 0U,
            .node_name = "DuplicateOutput",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "loot:0",
            .anchor_id = "grid:loot",
            .candidate_index = 0U,
            .node_name = "PackageInvisibleLoot",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = false,
        },
    };

    const auto plan = mirakana::runtime_scene::plan_runtime_scene_procedural_construction_placement_intents(
        generation, placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.size() == 5U);
    MK_REQUIRE(plan.diagnostics.size() == 5U);
    MK_REQUIRE(plan.rows[0].status == Status::invalid);
    MK_REQUIRE(plan.diagnostics[0].code == Code::missing_procedural_output);
    MK_REQUIRE(plan.diagnostics[0].procedural_output_id == "missing:0");
    MK_REQUIRE(plan.diagnostics[1].code == Code::unsupported_procedural_output_kind);
    MK_REQUIRE(plan.diagnostics[1].procedural_output_id == "tile:0");
    MK_REQUIRE(plan.diagnostics[2].code == Code::missing_procedural_anchor);
    MK_REQUIRE(plan.diagnostics[2].anchor_id.empty());
    MK_REQUIRE(plan.diagnostics[3].code == Code::duplicate_procedural_output);
    MK_REQUIRE(plan.diagnostics[3].procedural_output_id == "prop:0");
    MK_REQUIRE(plan.diagnostics[4].code == Code::package_invisible_procedural_output);
    MK_REQUIRE(plan.diagnostics[4].procedural_output_id == "loot:0");
}

MK_TEST("runtime scene procedural construction placement preserves placement diagnostics") {
    using Code = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDiagnosticCode;
    using Status = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus;

    const auto generation = valid_procedural_generation_plan();
    const auto placement = valid_construction_placement_validation();
    auto invalid_transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}};
    invalid_transform.scale.x = 0.0F;
    const std::vector<mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "prop:0",
            .anchor_id = "grid:first",
            .candidate_index = 0U,
            .node_name = "GeneratedWorkbenchA",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "prop:1",
            .anchor_id = "grid:occupied",
            .candidate_index = 0U,
            .node_name = "GeneratedWorkbenchB",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "prop:2",
            .anchor_id = "grid:invalid-transform",
            .candidate_index = 0U,
            .node_name = "GeneratedWorkbenchC",
            .transform = invalid_transform,
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
    };

    const auto plan = mirakana::runtime_scene::plan_runtime_scene_procedural_construction_placement_intents(
        generation, placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.size() == 3U);
    MK_REQUIRE(plan.diagnostics.size() == 2U);
    MK_REQUIRE(plan.rows[0].status == Status::accepted);
    MK_REQUIRE(plan.rows[1].status == Status::already_occupied);
    MK_REQUIRE(plan.rows[1].procedural_output_id == "prop:1");
    MK_REQUIRE(plan.rows[2].status == Status::invalid);
    MK_REQUIRE(plan.rows[2].procedural_output_id == "prop:2");
    MK_REQUIRE(plan.diagnostics[0].code == Code::already_occupied);
    MK_REQUIRE(plan.diagnostics[0].procedural_output_id == "prop:1");
    MK_REQUIRE(plan.diagnostics[1].code == Code::invalid_transform);
    MK_REQUIRE(plan.diagnostics[1].procedural_output_id == "prop:2");
}

MK_TEST("runtime scene procedural construction placement preserves valid rows in mixed batches") {
    using Code = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDiagnosticCode;
    using Status = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus;

    const auto generation = valid_procedural_generation_plan();
    const auto placement = valid_construction_placement_validation();
    const std::vector<mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "prop:0",
            .anchor_id = "grid:first",
            .candidate_index = 0U,
            .node_name = "MixedWorkbenchA",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "missing:0",
            .anchor_id = "grid:missing",
            .candidate_index = 0U,
            .node_name = "MixedMissing",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "prop:1",
            .anchor_id = "grid:occupied",
            .candidate_index = 0U,
            .node_name = "MixedWorkbenchB",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
    };

    const auto plan = mirakana::runtime_scene::plan_runtime_scene_procedural_construction_placement_intents(
        generation, placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.size() == 3U);
    MK_REQUIRE(plan.rows[0].status == Status::accepted);
    MK_REQUIRE(plan.rows[0].procedural_output_id == "prop:0");
    MK_REQUIRE(plan.rows[1].status == Status::invalid);
    MK_REQUIRE(plan.rows[1].procedural_output_id == "missing:0");
    MK_REQUIRE(plan.rows[2].status == Status::already_occupied);
    MK_REQUIRE(plan.rows[2].procedural_output_id == "prop:1");
    const auto* missing =
        find_construction_placement_intent_diagnostic(plan, Code::missing_procedural_output, "missing:0");
    const auto* occupied = find_construction_placement_intent_diagnostic(plan, Code::already_occupied, "prop:1");
    MK_REQUIRE(missing != nullptr);
    MK_REQUIRE(occupied != nullptr);
    MK_REQUIRE(occupied->anchor_id == "grid:occupied");
}

MK_TEST("runtime scene procedural construction placement keeps diagnostics on duplicate node rows") {
    using Code = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDiagnosticCode;
    using Status = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus;

    const auto generation = valid_procedural_generation_plan();
    const auto placement = valid_construction_placement_validation();
    const std::vector<mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "prop:0",
            .anchor_id = "grid:duplicate-name-a",
            .candidate_index = 0U,
            .node_name = "DuplicateGeneratedWorkbench",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = "prop:1",
            .anchor_id = "grid:duplicate-name-b",
            .candidate_index = 0U,
            .node_name = "DuplicateGeneratedWorkbench",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = placement_sprite_components(),
            .reviewed = true,
            .package_visible = true,
        },
    };

    const auto plan = mirakana::runtime_scene::plan_runtime_scene_procedural_construction_placement_intents(
        generation, placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.size() == 2U);
    MK_REQUIRE(plan.rows[0].status == Status::accepted);
    MK_REQUIRE(plan.rows[1].status == Status::invalid);
    MK_REQUIRE(plan.rows[1].procedural_output_id == "prop:1");
    const auto* duplicate =
        find_construction_placement_intent_diagnostic(plan, Code::duplicate_intent_node_name, "prop:1");
    MK_REQUIRE(duplicate != nullptr);
    MK_REQUIRE(duplicate->anchor_id == "grid:duplicate-name-b");
}

int main() {
    return mirakana::test::run_all();
}
