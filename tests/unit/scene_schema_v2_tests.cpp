// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/scene/schema_v2.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>

namespace {

[[nodiscard]] mirakana::PrefabDocumentV2 make_prefab_v2_with_player_mesh() {
    mirakana::PrefabDocumentV2 prefab;
    prefab.name = "Enemy";
    prefab.scene.name = "EnemyScene";

    mirakana::SceneNodeDocumentV2 light;
    light.id = mirakana::AuthoringId{"node/light"};
    light.name = "Light";
    prefab.scene.nodes.push_back(light);

    mirakana::SceneNodeDocumentV2 player;
    player.id = mirakana::AuthoringId{"node/player"};
    player.name = "Player";
    prefab.scene.nodes.push_back(player);

    prefab.scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/player/mesh"},
        .node = mirakana::AuthoringId{"node/player"},
        .type = mirakana::SceneComponentTypeId{"mesh_renderer"},
        .properties = {{.name = "mesh", .value = "assets/meshes/player"},
                       {.name = "material", .value = "assets/materials/base"}},
    });

    return prefab;
}

[[nodiscard]] const mirakana::SceneComponentDocumentV2* find_component(const mirakana::SceneDocumentV2& scene,
                                                                       std::string_view id) {
    for (const auto& component : scene.components) {
        if (component.id.value == id) {
            return &component;
        }
    }
    return nullptr;
}

[[nodiscard]] std::string property_value(const mirakana::SceneComponentDocumentV2& component, std::string_view name) {
    for (const auto& property : component.properties) {
        if (property.name == name) {
            return property.value;
        }
    }
    return {};
}

[[nodiscard]] bool contains_diagnostic(const std::vector<mirakana::SceneSchemaV2Diagnostic>& diagnostics,
                                       mirakana::SceneSchemaV2DiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace

MK_TEST("scene schema v2 rejects duplicate node and component authoring ids") {
    mirakana::SceneDocumentV2 scene;
    scene.name = "Level";
    scene.nodes.push_back(mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/player"}, .name = "Player"});
    scene.nodes.push_back(
        mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/player"}, .name = "Duplicate"});
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/player/transform"},
        .node = mirakana::AuthoringId{"node/player"},
        .type = mirakana::SceneComponentTypeId{"transform3d"},
        .properties = {{.name = "position", .value = "0 0 0"}},
    });
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/player/transform"},
        .node = mirakana::AuthoringId{"node/player"},
        .type = mirakana::SceneComponentTypeId{"mesh_renderer"},
        .properties = {{.name = "mesh", .value = "assets/meshes/player"}},
    });

    const auto diagnostics = mirakana::validate_scene_document_v2(scene);

    MK_REQUIRE(diagnostics.size() == 2);
    MK_REQUIRE(diagnostics[0].code == mirakana::SceneSchemaV2DiagnosticCode::duplicate_node_id);
    MK_REQUIRE(diagnostics[1].code == mirakana::SceneSchemaV2DiagnosticCode::duplicate_component_id);
}

MK_TEST("scene schema v2 serializes deterministic text and round trips authoring ids") {
    mirakana::SceneDocumentV2 scene;
    scene.name = "Level";

    mirakana::SceneNodeDocumentV2 player;
    player.id = mirakana::AuthoringId{"node/player"};
    player.name = "Player";
    player.transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};
    player.transform.rotation_radians = mirakana::Vec3{.x = 0.0F, .y = 0.25F, .z = 0.0F};
    player.transform.scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    scene.nodes.push_back(player);

    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/player/mesh"},
        .node = mirakana::AuthoringId{"node/player"},
        .type = mirakana::SceneComponentTypeId{"mesh_renderer"},
        .properties = {{.name = "mesh", .value = "assets/meshes/player"}},
    });

    const std::string expected = "format=GameEngine.Scene.v2\n"
                                 "scene.name=Level\n"
                                 "node.0.id=node/player\n"
                                 "node.0.name=Player\n"
                                 "node.0.parent=\n"
                                 "node.0.position=1 2 3\n"
                                 "node.0.rotation=0 0.25 0\n"
                                 "node.0.scale=1 1 1\n"
                                 "component.0.id=component/player/mesh\n"
                                 "component.0.node=node/player\n"
                                 "component.0.type=mesh_renderer\n"
                                 "component.0.property.0.name=mesh\n"
                                 "component.0.property.0.value=assets/meshes/player\n";

    MK_REQUIRE(mirakana::serialize_scene_document_v2(scene) == expected);
    const auto round_trip = mirakana::deserialize_scene_document_v2(expected);
    MK_REQUIRE(round_trip.nodes.size() == 1);
    MK_REQUIRE(round_trip.nodes[0].id.value == "node/player");
    MK_REQUIRE(round_trip.components.size() == 1);
    MK_REQUIRE(round_trip.components[0].id.value == "component/player/mesh");
}

MK_TEST("scene schema v2 rejects oversized sparse indexes") {
    bool threw = false;
    try {
        (void)mirakana::deserialize_scene_document_v2("format=GameEngine.Scene.v2\n"
                                                      "scene.name=Level\n"
                                                      "node.18446744073709551615.id=node/root\n");
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    MK_REQUIRE(threw);
}

MK_TEST("scene schema v2 rejects line-control text values and invalid transforms") {
    auto scene = mirakana::SceneDocumentV2{};
    scene.name = "Level\ncomponent.0.id=component/injected";
    scene.nodes.push_back(mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/root"}, .name = "Root"});

    auto diagnostics = mirakana::validate_scene_document_v2(scene);
    MK_REQUIRE(contains_diagnostic(diagnostics, mirakana::SceneSchemaV2DiagnosticCode::invalid_text_value));

    scene.name = "Level";
    scene.nodes[0].name = "Root\nnode.1.id=node/injected";
    diagnostics = mirakana::validate_scene_document_v2(scene);
    MK_REQUIRE(contains_diagnostic(diagnostics, mirakana::SceneSchemaV2DiagnosticCode::invalid_text_value));

    scene.nodes[0].name = "Root";
    scene.nodes[0].transform.position.x = std::numeric_limits<float>::infinity();
    diagnostics = mirakana::validate_scene_document_v2(scene);
    MK_REQUIRE(contains_diagnostic(diagnostics, mirakana::SceneSchemaV2DiagnosticCode::invalid_transform));

    scene.nodes[0].transform.position.x = 0.0F;
    scene.nodes[0].transform.scale.y = 0.0F;
    diagnostics = mirakana::validate_scene_document_v2(scene);
    MK_REQUIRE(contains_diagnostic(diagnostics, mirakana::SceneSchemaV2DiagnosticCode::invalid_transform));
}

MK_TEST("prefab schema v2 composes property overrides through stable authoring ids") {
    mirakana::PrefabVariantDocumentV2 variant;
    variant.name = "EliteEnemy";
    variant.base_prefab = make_prefab_v2_with_player_mesh();
    variant.overrides.push_back(mirakana::PrefabOverrideV2{
        .path =
            mirakana::PrefabOverridePathV2{"nodes/node/player/components/component/player/mesh/properties/material"},
        .value = "assets/materials/elite",
    });

    const auto result = mirakana::compose_prefab_variant_v2(variant);

    MK_REQUIRE(result.success);
    MK_REQUIRE(result.prefab.name == "EliteEnemy");
    const auto* component = find_component(result.prefab.scene, "component/player/mesh");
    MK_REQUIRE(component != nullptr);
    MK_REQUIRE(property_value(*component, "mesh") == "assets/meshes/player");
    MK_REQUIRE(property_value(*component, "material") == "assets/materials/elite");

    mirakana::PrefabVariantDocumentV2 reordered = variant;
    auto first = reordered.base_prefab.scene.nodes[0];
    reordered.base_prefab.scene.nodes[0] = reordered.base_prefab.scene.nodes[1];
    reordered.base_prefab.scene.nodes[1] = first;

    const auto reordered_result = mirakana::compose_prefab_variant_v2(reordered);

    MK_REQUIRE(reordered_result.success);
    const auto* reordered_component = find_component(reordered_result.prefab.scene, "component/player/mesh");
    MK_REQUIRE(reordered_component != nullptr);
    MK_REQUIRE(property_value(*reordered_component, "material") == "assets/materials/elite");
}

MK_TEST("prefab schema v2 reports missing override targets and duplicate paths") {
    mirakana::PrefabVariantDocumentV2 missing_target;
    missing_target.name = "BrokenEnemy";
    missing_target.base_prefab = make_prefab_v2_with_player_mesh();
    missing_target.overrides.push_back(mirakana::PrefabOverrideV2{
        .path =
            mirakana::PrefabOverridePathV2{"nodes/node/player/components/component/player/missing/properties/material"},
        .value = "assets/materials/elite",
    });

    const auto missing_result = mirakana::compose_prefab_variant_v2(missing_target);

    MK_REQUIRE(!missing_result.success);
    MK_REQUIRE(missing_result.diagnostics.size() == 1);
    MK_REQUIRE(missing_result.diagnostics[0].code == mirakana::SceneSchemaV2DiagnosticCode::missing_override_target);

    mirakana::PrefabVariantDocumentV2 duplicate_path;
    duplicate_path.name = "DuplicateEnemy";
    duplicate_path.base_prefab = make_prefab_v2_with_player_mesh();
    duplicate_path.overrides.push_back(mirakana::PrefabOverrideV2{
        .path =
            mirakana::PrefabOverridePathV2{"nodes/node/player/components/component/player/mesh/properties/material"},
        .value = "assets/materials/elite",
    });
    duplicate_path.overrides.push_back(mirakana::PrefabOverrideV2{
        .path =
            mirakana::PrefabOverridePathV2{"nodes/node/player/components/component/player/mesh/properties/material"},
        .value = "assets/materials/other",
    });

    const auto duplicate_result = mirakana::compose_prefab_variant_v2(duplicate_path);

    MK_REQUIRE(!duplicate_result.success);
    MK_REQUIRE(duplicate_result.diagnostics.size() == 1);
    MK_REQUIRE(duplicate_result.diagnostics[0].code == mirakana::SceneSchemaV2DiagnosticCode::duplicate_override_path);
}

MK_TEST("scene schema v2 plans prefab instance refresh through stable source ids") {
    constexpr std::string_view prefab_path = "source/prefabs/enemy.prefab";

    mirakana::SceneDocumentV2 scene;
    scene.name = "Level";
    scene.nodes.push_back(
        mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/instance/root"}, .name = "EnemyRoot"});
    scene.nodes.push_back(mirakana::SceneNodeDocumentV2{
        .id = mirakana::AuthoringId{"node/instance/stale"},
        .name = "Stale",
        .parent = mirakana::AuthoringId{"node/instance/root"},
    });
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/instance/mesh"},
        .node = mirakana::AuthoringId{"node/instance/root"},
        .type = mirakana::SceneComponentTypeId{"mesh_renderer"},
        .properties = {{.name = "mesh", .value = "assets/meshes/enemy"}},
    });
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/instance/stale"},
        .node = mirakana::AuthoringId{"node/instance/stale"},
        .type = mirakana::SceneComponentTypeId{"light"},
    });
    scene.node_prefab_sources.push_back(mirakana::SceneNodePrefabSourceV2{
        .node = mirakana::AuthoringId{"node/instance/root"},
        .prefab_path = std::string(prefab_path),
        .source_node_id = mirakana::AuthoringId{"node/source/root"},
    });
    scene.node_prefab_sources.push_back(mirakana::SceneNodePrefabSourceV2{
        .node = mirakana::AuthoringId{"node/instance/stale"},
        .prefab_path = std::string(prefab_path),
        .source_node_id = mirakana::AuthoringId{"node/source/stale"},
    });
    scene.component_prefab_sources.push_back(mirakana::SceneComponentPrefabSourceV2{
        .component = mirakana::AuthoringId{"component/instance/mesh"},
        .prefab_path = std::string(prefab_path),
        .source_component_id = mirakana::AuthoringId{"component/source/mesh"},
    });
    scene.component_prefab_sources.push_back(mirakana::SceneComponentPrefabSourceV2{
        .component = mirakana::AuthoringId{"component/instance/stale"},
        .prefab_path = std::string(prefab_path),
        .source_component_id = mirakana::AuthoringId{"component/source/stale"},
    });

    mirakana::PrefabDocumentV2 refreshed_prefab;
    refreshed_prefab.name = "Enemy";
    refreshed_prefab.scene.name = "EnemyScene";
    refreshed_prefab.scene.nodes.push_back(
        mirakana::SceneNodeDocumentV2{.id = mirakana::AuthoringId{"node/source/root"}, .name = "EnemyRoot"});
    refreshed_prefab.scene.nodes.push_back(mirakana::SceneNodeDocumentV2{
        .id = mirakana::AuthoringId{"node/source/weapon"},
        .name = "Weapon",
        .parent = mirakana::AuthoringId{"node/source/root"},
    });
    refreshed_prefab.scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/source/mesh"},
        .node = mirakana::AuthoringId{"node/source/root"},
        .type = mirakana::SceneComponentTypeId{"mesh_renderer"},
        .properties = {{.name = "mesh", .value = "assets/meshes/enemy"}},
    });
    refreshed_prefab.scene.components.push_back(mirakana::SceneComponentDocumentV2{
        .id = mirakana::AuthoringId{"component/source/light"},
        .node = mirakana::AuthoringId{"node/source/weapon"},
        .type = mirakana::SceneComponentTypeId{"light"},
    });

    const auto plan = mirakana::plan_scene_prefab_instance_refresh_v2(
        scene, mirakana::AuthoringId{"node/instance/root"}, refreshed_prefab);

    MK_REQUIRE(plan.valid);
    MK_REQUIRE(!plan.mutates);
    MK_REQUIRE(!plan.executes);
    MK_REQUIRE(plan.instance_root_node.value == "node/instance/root");
    MK_REQUIRE(plan.prefab_path == prefab_path);
    MK_REQUIRE(plan.preserve_node_count == 1U);
    MK_REQUIRE(plan.remove_node_count == 1U);
    MK_REQUIRE(plan.add_node_count == 1U);
    MK_REQUIRE(plan.preserve_component_count == 1U);
    MK_REQUIRE(plan.remove_component_count == 1U);
    MK_REQUIRE(plan.add_component_count == 1U);
    MK_REQUIRE(plan.rows.size() == 6U);

    MK_REQUIRE(plan.rows[0].kind == mirakana::ScenePrefabInstanceRefreshRowKindV2::preserve_node);
    MK_REQUIRE(plan.rows[0].current_node.value == "node/instance/root");
    MK_REQUIRE(plan.rows[0].source_node_id.value == "node/source/root");
    MK_REQUIRE(plan.rows[1].kind == mirakana::ScenePrefabInstanceRefreshRowKindV2::remove_stale_node);
    MK_REQUIRE(plan.rows[1].current_node.value == "node/instance/stale");
    MK_REQUIRE(plan.rows[1].source_node_id.value == "node/source/stale");
    MK_REQUIRE(plan.rows[2].kind == mirakana::ScenePrefabInstanceRefreshRowKindV2::add_source_node);
    MK_REQUIRE(plan.rows[2].source_node_id.value == "node/source/weapon");
    MK_REQUIRE(plan.rows[3].kind == mirakana::ScenePrefabInstanceRefreshRowKindV2::preserve_component);
    MK_REQUIRE(plan.rows[3].current_component.value == "component/instance/mesh");
    MK_REQUIRE(plan.rows[3].source_component_id.value == "component/source/mesh");
    MK_REQUIRE(plan.rows[4].kind == mirakana::ScenePrefabInstanceRefreshRowKindV2::remove_stale_component);
    MK_REQUIRE(plan.rows[4].current_component.value == "component/instance/stale");
    MK_REQUIRE(plan.rows[4].source_component_id.value == "component/source/stale");
    MK_REQUIRE(plan.rows[5].kind == mirakana::ScenePrefabInstanceRefreshRowKindV2::add_source_component);
    MK_REQUIRE(plan.rows[5].source_component_id.value == "component/source/light");
    MK_REQUIRE(plan.rows[5].source_node_id.value == "node/source/weapon");
}

int main() {
    return mirakana::test::run_all();
}
