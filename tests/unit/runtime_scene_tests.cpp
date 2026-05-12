// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/runtime_scene/runtime_scene.hpp"

#include <cmath>
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

int main() {
    return mirakana::test::run_all();
}
