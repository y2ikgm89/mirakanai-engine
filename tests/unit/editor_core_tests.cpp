// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/material_graph.hpp"

#include "mirakana/editor/ai_command_panel.hpp"
#include "mirakana/editor/asset_pipeline.hpp"
#include "mirakana/editor/command.hpp"
#include "mirakana/editor/content_browser.hpp"
#include "mirakana/editor/content_browser_import_panel.hpp"
#include "mirakana/editor/game_module_driver.hpp"
#include "mirakana/editor/gltf_mesh_catalog.hpp"
#include "mirakana/editor/history.hpp"
#include "mirakana/editor/input_rebinding.hpp"
#include "mirakana/editor/io.hpp"
#include "mirakana/editor/material_asset_preview_panel.hpp"
#include "mirakana/editor/material_authoring.hpp"
#include "mirakana/editor/material_graph_authoring.hpp"
#include "mirakana/editor/palette.hpp"
#include "mirakana/editor/play_in_editor.hpp"
#include "mirakana/editor/playtest_package_review.hpp"
#include "mirakana/editor/prefab_variant_authoring.hpp"
#include "mirakana/editor/profiler.hpp"
#include "mirakana/editor/project.hpp"
#include "mirakana/editor/project_wizard.hpp"
#include "mirakana/editor/render_backend.hpp"
#include "mirakana/editor/resource_panel.hpp"
#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/editor/scene_edit.hpp"
#include "mirakana/editor/shader_artifact_io.hpp"
#include "mirakana/editor/shader_compile.hpp"
#include "mirakana/editor/shader_tool_discovery.hpp"
#include "mirakana/editor/source_registry_browser.hpp"
#include "mirakana/editor/ui_model.hpp"
#include "mirakana/editor/viewport.hpp"
#include "mirakana/editor/viewport_shader_artifacts.hpp"
#include "mirakana/editor/workspace.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/platform/process.hpp"
#include "mirakana/tools/asset_import_adapters.hpp"
#include "mirakana/tools/asset_import_tool.hpp"
#include "mirakana/tools/gltf_mesh_inspect.hpp"
#include "mirakana/tools/shader_compile_action.hpp"

#include "mirakana/assets/asset_hot_reload.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_import_metadata.hpp"
#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/shader_metadata.hpp"
#include "mirakana/assets/shader_pipeline.hpp"
#include "mirakana/assets/source_asset_registry.hpp"
#include "mirakana/assets/tilemap_metadata.hpp"
#include "mirakana/core/diagnostics.hpp"
#include "mirakana/runtime/asset_runtime.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] std::string editor_test_cooked_texture_payload(mirakana::AssetId asset) {
    return "format=GameEngine.CookedTexture.v1\n"
           "asset.id=" +
           std::to_string(asset.value) +
           "\n"
           "asset.kind=texture\n"
           "source.path=source/textures/tilemap-atlas.texture_source\n"
           "texture.width=16\n"
           "texture.height=16\n"
           "texture.pixel_format=rgba8_unorm\n"
           "texture.source_bytes=1024\n";
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_editor_tilemap_test_package() {
    const auto atlas = mirakana::AssetId::from_name("textures/tilemap-atlas");
    const auto tilemap = mirakana::AssetId::from_name("tilemaps/level");

    mirakana::TilemapMetadataDocument document;
    document.asset = tilemap;
    document.atlas_page = atlas;
    document.atlas_page_uri = "runtime/assets/2d/tilemap-atlas.texture";
    document.tile_width = 16;
    document.tile_height = 16;
    document.tiles.push_back(mirakana::TilemapAtlasTile{
        .id = "grass",
        .page = atlas,
        .uv = mirakana::TilemapUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 0.5F, .v1 = 0.5F},
        .color = {0.3F, 0.8F, 0.2F, 1.0F},
    });
    document.tiles.push_back(mirakana::TilemapAtlasTile{
        .id = "water",
        .page = atlas,
        .uv = mirakana::TilemapUvRect{.u0 = 0.5F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 0.5F},
        .color = {0.1F, 0.3F, 1.0F, 1.0F},
    });
    document.layers.push_back(mirakana::TilemapLayer{
        .name = "ground",
        .width = 2,
        .height = 2,
        .visible = true,
        .cells = {"grass", "water", "", "grass"},
    });

    const auto texture_payload = editor_test_cooked_texture_payload(atlas);
    const auto tilemap_payload = mirakana::serialize_tilemap_metadata_document(document);
    return mirakana::runtime::RuntimeAssetPackage(
        {
            mirakana::runtime::RuntimeAssetRecord{
                .handle = mirakana::runtime::RuntimeAssetHandle{1},
                .asset = atlas,
                .kind = mirakana::AssetKind::texture,
                .path = "runtime/assets/2d/tilemap-atlas.texture",
                .content_hash = mirakana::hash_asset_cooked_content(texture_payload),
                .source_revision = 1,
                .dependencies = {},
                .content = texture_payload,
            },
            mirakana::runtime::RuntimeAssetRecord{
                .handle = mirakana::runtime::RuntimeAssetHandle{2},
                .asset = tilemap,
                .kind = mirakana::AssetKind::tilemap,
                .path = "runtime/assets/2d/level.tilemap",
                .content_hash = mirakana::hash_asset_cooked_content(tilemap_payload),
                .source_revision = 1,
                .dependencies = {atlas},
                .content = tilemap_payload,
            },
        },
        {
            mirakana::AssetDependencyEdge{
                .asset = tilemap,
                .dependency = atlas,
                .kind = mirakana::AssetDependencyKind::tilemap_texture,
                .path = "runtime/assets/2d/level.tilemap",
            },
        });
}

[[nodiscard]] mirakana::runtime::RuntimeInputActionTrigger editor_key_trigger(mirakana::Key key) noexcept {
    return mirakana::runtime::RuntimeInputActionTrigger{.kind = mirakana::runtime::RuntimeInputActionTriggerKind::key,
                                                        .key = key};
}

[[nodiscard]] mirakana::runtime::RuntimeInputActionTrigger
editor_gamepad_button_trigger(mirakana::GamepadId gamepad_id, mirakana::GamepadButton button) noexcept {
    return mirakana::runtime::RuntimeInputActionTrigger{
        .kind = mirakana::runtime::RuntimeInputActionTriggerKind::gamepad_button,
        .key = mirakana::Key::unknown,
        .pointer_id = 0,
        .gamepad_id = gamepad_id,
        .gamepad_button = button};
}

[[nodiscard]] mirakana::runtime::RuntimeInputAxisSource editor_gamepad_axis_source(mirakana::GamepadId gamepad_id,
                                                                                   mirakana::GamepadAxis axis,
                                                                                   float scale = 1.0F,
                                                                                   float deadzone = 0.0F) noexcept {
    mirakana::runtime::RuntimeInputAxisSource source;
    source.kind = mirakana::runtime::RuntimeInputAxisSourceKind::gamepad_axis;
    source.gamepad_id = gamepad_id;
    source.gamepad_axis = axis;
    source.scale = scale;
    source.deadzone = deadzone;
    return source;
}

[[nodiscard]] bool
editor_input_rebinding_review_has_code(const mirakana::editor::EditorInputRebindingProfileReviewModel& model,
                                       mirakana::runtime::RuntimeInputRebindingDiagnosticCode code) noexcept {
    return std::ranges::any_of(model.rows, [code](const auto& row) {
        return row.diagnostic_code.has_value() && *row.diagnostic_code == code;
    });
}

[[nodiscard]] const mirakana::editor::EditorInputRebindingProfileBindingRow*
find_input_rebinding_binding_row(const std::vector<mirakana::editor::EditorInputRebindingProfileBindingRow>& rows,
                                 std::string_view id) noexcept {
    const auto it = std::ranges::find_if(
        rows, [id](const mirakana::editor::EditorInputRebindingProfileBindingRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::PrefabVariantConflictRow*
find_prefab_variant_conflict_row(const std::vector<mirakana::editor::PrefabVariantConflictRow>& rows,
                                 std::string_view id) noexcept {
    const auto it = std::ranges::find_if(
        rows, [id](const mirakana::editor::PrefabVariantConflictRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::PrefabVariantBaseRefreshRow*
find_prefab_variant_base_refresh_row(const std::vector<mirakana::editor::PrefabVariantBaseRefreshRow>& rows,
                                     std::string_view id) noexcept {
    const auto it = std::ranges::find_if(
        rows, [id](const mirakana::editor::PrefabVariantBaseRefreshRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::ScenePrefabInstanceRefreshRow*
find_scene_prefab_instance_refresh_row(const std::vector<mirakana::editor::ScenePrefabInstanceRefreshRow>& rows,
                                       std::string_view id) noexcept {
    const auto it = std::ranges::find_if(
        rows, [id](const mirakana::editor::ScenePrefabInstanceRefreshRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::EditorAiPlaytestEvidenceImportReviewRow*
find_ai_evidence_import_row(const std::vector<mirakana::editor::EditorAiPlaytestEvidenceImportReviewRow>& rows,
                            std::string_view recipe_id) noexcept {
    const auto it = std::ranges::find_if(rows, [recipe_id](const auto& row) { return row.recipe_id == recipe_id; });
    return it == rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::EditorResourceRow*
find_editor_resource_row(const std::vector<mirakana::editor::EditorResourceRow>& rows, std::string_view id) noexcept {
    const auto it =
        std::ranges::find_if(rows, [id](const mirakana::editor::EditorResourceRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::EditorResourceCaptureRequestRow*
find_editor_resource_capture_request_row(const std::vector<mirakana::editor::EditorResourceCaptureRequestRow>& rows,
                                         std::string_view id) noexcept {
    const auto it = std::ranges::find_if(
        rows, [id](const mirakana::editor::EditorResourceCaptureRequestRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::EditorResourceCaptureExecutionRow*
find_editor_resource_capture_execution_row(const std::vector<mirakana::editor::EditorResourceCaptureExecutionRow>& rows,
                                           std::string_view id) noexcept {
    const auto it = std::ranges::find_if(
        rows, [id](const mirakana::editor::EditorResourceCaptureExecutionRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &(*it);
}

MK_TEST("editor workspace creates required default panels") {
    const auto workspace = mirakana::editor::Workspace::create_default(
        mirakana::editor::ProjectInfo{.name = "sample", .root_path = "games/sample"});

    MK_REQUIRE(workspace.project().name == "sample");
    MK_REQUIRE(workspace.project().root_path == "games/sample");
    MK_REQUIRE(workspace.panel_count() == 11);
    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::scene));
    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::inspector));
    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::assets));
    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::console));
    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::viewport));
    MK_REQUIRE(!workspace.is_panel_visible(mirakana::editor::PanelId::resources));
    MK_REQUIRE(!workspace.is_panel_visible(mirakana::editor::PanelId::ai_commands));
    MK_REQUIRE(!workspace.is_panel_visible(mirakana::editor::PanelId::input_rebinding));
    MK_REQUIRE(!workspace.is_panel_visible(mirakana::editor::PanelId::profiler));
}

MK_TEST("editor workspace toggles optional panels") {
    auto workspace = mirakana::editor::Workspace::create_default(
        mirakana::editor::ProjectInfo{.name = "sample", .root_path = "games/sample"});

    MK_REQUIRE(!workspace.is_panel_visible(mirakana::editor::PanelId::resources));
    MK_REQUIRE(!workspace.is_panel_visible(mirakana::editor::PanelId::ai_commands));
    MK_REQUIRE(!workspace.is_panel_visible(mirakana::editor::PanelId::input_rebinding));
    MK_REQUIRE(!workspace.is_panel_visible(mirakana::editor::PanelId::profiler));
    MK_REQUIRE(!workspace.is_panel_visible(mirakana::editor::PanelId::project_settings));
    MK_REQUIRE(!workspace.is_panel_visible(mirakana::editor::PanelId::timeline));

    workspace.set_panel_visible(mirakana::editor::PanelId::resources, true);
    workspace.set_panel_visible(mirakana::editor::PanelId::ai_commands, true);
    workspace.set_panel_visible(mirakana::editor::PanelId::input_rebinding, true);
    workspace.set_panel_visible(mirakana::editor::PanelId::profiler, true);
    workspace.set_panel_visible(mirakana::editor::PanelId::project_settings, true);
    workspace.set_panel_visible(mirakana::editor::PanelId::timeline, true);

    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::resources));
    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::ai_commands));
    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::input_rebinding));
    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::profiler));
    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::project_settings));
    MK_REQUIRE(workspace.is_panel_visible(mirakana::editor::PanelId::timeline));
}

MK_TEST("editor workspace serializes and restores panel state") {
    auto workspace = mirakana::editor::Workspace::create_default(
        mirakana::editor::ProjectInfo{.name = "sample", .root_path = "games/sample"});
    workspace.set_panel_visible(mirakana::editor::PanelId::assets, false);
    workspace.set_panel_visible(mirakana::editor::PanelId::profiler, true);

    const auto serialized = mirakana::editor::serialize_workspace(workspace);
    MK_REQUIRE(serialized.contains("format=GameEngine.Workspace.v1"));
    MK_REQUIRE(serialized.contains("project.name=sample"));
    MK_REQUIRE(serialized.contains("panel.assets=hidden"));
    MK_REQUIRE(!serialized.contains("panel.resources=visible"));

    workspace.set_panel_visible(mirakana::editor::PanelId::resources, true);
    workspace.set_panel_visible(mirakana::editor::PanelId::ai_commands, true);
    workspace.set_panel_visible(mirakana::editor::PanelId::input_rebinding, true);
    const auto serialized_with_resources = mirakana::editor::serialize_workspace(workspace);
    MK_REQUIRE(serialized_with_resources.contains("panel.resources=visible"));
    MK_REQUIRE(serialized_with_resources.contains("panel.ai_commands=visible"));
    MK_REQUIRE(serialized_with_resources.contains("panel.input_rebinding=visible"));
    MK_REQUIRE(serialized.contains("panel.profiler=visible"));

    const auto restored = mirakana::editor::deserialize_workspace(serialized_with_resources);
    MK_REQUIRE(restored.project().name == "sample");
    MK_REQUIRE(restored.project().root_path == "games/sample");
    MK_REQUIRE(!restored.is_panel_visible(mirakana::editor::PanelId::assets));
    MK_REQUIRE(restored.is_panel_visible(mirakana::editor::PanelId::resources));
    MK_REQUIRE(restored.is_panel_visible(mirakana::editor::PanelId::ai_commands));
    MK_REQUIRE(restored.is_panel_visible(mirakana::editor::PanelId::input_rebinding));
    MK_REQUIRE(restored.is_panel_visible(mirakana::editor::PanelId::profiler));
    MK_REQUIRE(restored.is_panel_visible(mirakana::editor::PanelId::scene));
}

MK_TEST("editor workspace migrates v0 panel state to current defaults") {
    const auto migrated = mirakana::editor::migrate_workspace("format=GameEngine.Workspace.v0\n"
                                                              "project.name=legacy\n"
                                                              "project.root=games/legacy\n"
                                                              "panel.assets=hidden\n");

    MK_REQUIRE(migrated.source_version == 0);
    MK_REQUIRE(migrated.target_version == 1);
    MK_REQUIRE(migrated.migrated);
    MK_REQUIRE(migrated.workspace.project().name == "legacy");
    MK_REQUIRE(!migrated.workspace.is_panel_visible(mirakana::editor::PanelId::assets));
    MK_REQUIRE(migrated.workspace.is_panel_visible(mirakana::editor::PanelId::viewport));
    MK_REQUIRE(!migrated.workspace.is_panel_visible(mirakana::editor::PanelId::resources));
    MK_REQUIRE(!migrated.workspace.is_panel_visible(mirakana::editor::PanelId::ai_commands));
    MK_REQUIRE(!migrated.workspace.is_panel_visible(mirakana::editor::PanelId::input_rebinding));
    MK_REQUIRE(!migrated.workspace.is_panel_visible(mirakana::editor::PanelId::profiler));
}

MK_TEST("editor workspace rejects duplicate panel state") {
    bool rejected_duplicate_panel = false;
    try {
        (void)mirakana::editor::deserialize_workspace("format=GameEngine.Workspace.v1\n"
                                                      "project.name=sample\n"
                                                      "project.root=games/sample\n"
                                                      "panel.assets=visible\n"
                                                      "panel.assets=hidden\n");
    } catch (const std::invalid_argument&) {
        rejected_duplicate_panel = true;
    }
    MK_REQUIRE(rejected_duplicate_panel);
}

MK_TEST("editor command registry executes registered commands") {
    mirakana::editor::CommandRegistry registry;
    int executed = 0;

    const auto added = registry.try_add(mirakana::editor::Command{
        .id = "editor.save_workspace",
        .label = "Save Workspace",
        .action = [&executed]() { ++executed; },
    });

    MK_REQUIRE(added);
    MK_REQUIRE(registry.contains("editor.save_workspace"));
    MK_REQUIRE(!registry.try_add(mirakana::editor::Command{"editor.save_workspace", "Duplicate", []() {}}));
    MK_REQUIRE(registry.execute("editor.save_workspace"));
    MK_REQUIRE(executed == 1);
    MK_REQUIRE(!registry.execute("editor.missing"));
}

MK_TEST("editor project document serializes and restores project paths") {
    const mirakana::editor::ProjectDocument document{
        .name = "sample",
        .root_path = "games/sample",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
        .shader_tool =
            mirakana::editor::ProjectShaderToolSettings{
                .executable = "toolchains/dxc/bin/dxc.exe",
                .working_directory = ".",
                .artifact_output_root = "out/editor/shaders",
                .cache_index_path = "out/editor/shaders/shader-cache.gecache",
            },
        .render_backend = mirakana::editor::EditorRenderBackend::automatic,
    };

    const auto serialized = mirakana::editor::serialize_project_document(document);
    MK_REQUIRE(serialized.contains("format=GameEngine.Project.v4"));
    MK_REQUIRE(serialized.contains("project.name=sample"));
    MK_REQUIRE(serialized.contains("project.asset_root=assets"));
    MK_REQUIRE(serialized.contains("project.source_registry=source/assets/package.geassets"));
    MK_REQUIRE(serialized.contains("project.startup_scene=scenes/start.scene"));
    MK_REQUIRE(serialized.contains("project.render_backend=auto"));
    MK_REQUIRE(serialized.contains("project.shader_tool.executable=toolchains/dxc/bin/dxc.exe"));
    MK_REQUIRE(serialized.contains("project.shader_tool.artifact_output_root=out/editor/shaders"));
    MK_REQUIRE(serialized.contains("project.shader_tool.cache_index=out/editor/shaders/shader-cache.gecache"));

    const auto restored = mirakana::editor::deserialize_project_document(serialized);
    MK_REQUIRE(restored.name == "sample");
    MK_REQUIRE(restored.root_path == "games/sample");
    MK_REQUIRE(restored.asset_root == "assets");
    MK_REQUIRE(restored.source_registry_path == "source/assets/package.geassets");
    MK_REQUIRE(restored.game_manifest_path == "game.agent.json");
    MK_REQUIRE(restored.startup_scene_path == "scenes/start.scene");
    MK_REQUIRE(restored.render_backend == mirakana::editor::EditorRenderBackend::automatic);
    MK_REQUIRE(restored.shader_tool.executable == "toolchains/dxc/bin/dxc.exe");
    MK_REQUIRE(restored.shader_tool.working_directory == ".");
    MK_REQUIRE(restored.shader_tool.artifact_output_root == "out/editor/shaders");
    MK_REQUIRE(restored.shader_tool.cache_index_path == "out/editor/shaders/shader-cache.gecache");
}

MK_TEST("editor project document migrates v0 defaults") {
    const auto migrated = mirakana::editor::migrate_project_document("format=GameEngine.Project.v0\n"
                                                                     "project.name=legacy\n"
                                                                     "project.root=games/legacy\n");

    MK_REQUIRE(migrated.source_version == 0);
    MK_REQUIRE(migrated.target_version == 4);
    MK_REQUIRE(migrated.migrated);
    MK_REQUIRE(migrated.document.name == "legacy");
    MK_REQUIRE(migrated.document.asset_root == "assets");
    MK_REQUIRE(migrated.document.source_registry_path == "source/assets/package.geassets");
    MK_REQUIRE(migrated.document.game_manifest_path == "game.agent.json");
    MK_REQUIRE(migrated.document.startup_scene_path == "scenes/start.scene");
    MK_REQUIRE(migrated.document.render_backend == mirakana::editor::EditorRenderBackend::automatic);
    MK_REQUIRE(migrated.document.shader_tool.executable == "dxc");
    MK_REQUIRE(migrated.document.shader_tool.artifact_output_root == "out/editor/shaders");
    MK_REQUIRE(migrated.document.shader_tool.cache_index_path == "out/editor/shaders/shader-cache.gecache");
}

MK_TEST("editor project document migrates v1 shader tool defaults") {
    const auto migrated = mirakana::editor::migrate_project_document("format=GameEngine.Project.v1\n"
                                                                     "project.name=legacy\n"
                                                                     "project.root=games/legacy\n"
                                                                     "project.asset_root=assets\n"
                                                                     "project.game_manifest=game.agent.json\n"
                                                                     "project.startup_scene=scenes/start.scene\n");

    MK_REQUIRE(migrated.source_version == 1);
    MK_REQUIRE(migrated.target_version == 4);
    MK_REQUIRE(migrated.migrated);
    MK_REQUIRE(migrated.document.source_registry_path == "source/assets/package.geassets");
    MK_REQUIRE(migrated.document.render_backend == mirakana::editor::EditorRenderBackend::automatic);
    MK_REQUIRE(migrated.document.shader_tool.executable == "dxc");
    MK_REQUIRE(migrated.document.shader_tool.working_directory == ".");
    MK_REQUIRE(migrated.document.shader_tool.artifact_output_root == "out/editor/shaders");
    MK_REQUIRE(migrated.document.shader_tool.cache_index_path == "out/editor/shaders/shader-cache.gecache");
}

MK_TEST("editor project document migrates v3 source registry path default") {
    const auto migrated =
        mirakana::editor::migrate_project_document("format=GameEngine.Project.v3\n"
                                                   "project.name=legacy\n"
                                                   "project.root=games/legacy\n"
                                                   "project.asset_root=assets\n"
                                                   "project.game_manifest=game.agent.json\n"
                                                   "project.startup_scene=scenes/start.scene\n"
                                                   "project.render_backend=vulkan\n"
                                                   "project.shader_tool.executable=dxc\n"
                                                   "project.shader_tool.working_directory=.\n"
                                                   "project.shader_tool.artifact_output_root=out/editor/shaders\n"
                                                   "project.shader_tool.cache_index=out/editor/shaders/"
                                                   "shader-cache.gecache\n");

    MK_REQUIRE(migrated.source_version == 3);
    MK_REQUIRE(migrated.target_version == 4);
    MK_REQUIRE(migrated.migrated);
    MK_REQUIRE(migrated.document.source_registry_path == "source/assets/package.geassets");
    MK_REQUIRE(migrated.document.render_backend == mirakana::editor::EditorRenderBackend::vulkan);
}

MK_TEST("editor project document persists render backend preference") {
    mirakana::editor::ProjectDocument document{
        .name = "sample",
        .root_path = "games/sample",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
        .shader_tool = {},
        .render_backend = mirakana::editor::EditorRenderBackend::automatic,
    };
    document.render_backend = mirakana::editor::EditorRenderBackend::d3d12;

    const auto serialized = mirakana::editor::serialize_project_document(document);
    MK_REQUIRE(serialized.contains("project.render_backend=d3d12"));

    const auto restored = mirakana::editor::deserialize_project_document(serialized);
    MK_REQUIRE(restored.render_backend == mirakana::editor::EditorRenderBackend::d3d12);
}

MK_TEST("editor project document rejects unknown render backend preference") {
    bool rejected_unknown_backend = false;
    try {
        (void)mirakana::editor::deserialize_project_document(
            "format=GameEngine.Project.v3\n"
            "project.name=sample\n"
            "project.root=games/sample\n"
            "project.asset_root=assets\n"
            "project.game_manifest=game.agent.json\n"
            "project.startup_scene=scenes/start.scene\n"
            "project.render_backend=opengl\n"
            "project.shader_tool.executable=dxc\n"
            "project.shader_tool.working_directory=.\n"
            "project.shader_tool.artifact_output_root=out/editor/shaders\n"
            "project.shader_tool.cache_index=out/editor/shaders/"
            "shader-cache.gecache\n");
    } catch (const std::invalid_argument&) {
        rejected_unknown_backend = true;
    }

    MK_REQUIRE(rejected_unknown_backend);
}

MK_TEST("editor project document rejects unsafe relative paths") {
    bool rejected_parent_asset_root = false;
    try {
        mirakana::editor::validate_project_document(mirakana::editor::ProjectDocument{
            .name = "bad",
            .root_path = "games/bad",
            .asset_root = "../assets",
            .source_registry_path = "source/assets/package.geassets",
            .game_manifest_path = "game.agent.json",
            .startup_scene_path = "scenes/start.scene",
            .shader_tool = {},
            .render_backend = mirakana::editor::EditorRenderBackend::automatic,
        });
    } catch (const std::invalid_argument&) {
        rejected_parent_asset_root = true;
    }

    bool rejected_absolute_scene = false;
    try {
        mirakana::editor::validate_project_document(mirakana::editor::ProjectDocument{
            .name = "bad",
            .root_path = "games/bad",
            .asset_root = "assets",
            .source_registry_path = "source/assets/package.geassets",
            .game_manifest_path = "game.agent.json",
            .startup_scene_path = "C:/outside.scene",
            .shader_tool = {},
            .render_backend = mirakana::editor::EditorRenderBackend::automatic,
        });
    } catch (const std::invalid_argument&) {
        rejected_absolute_scene = true;
    }

    MK_REQUIRE(rejected_parent_asset_root);
    MK_REQUIRE(rejected_absolute_scene);
}

MK_TEST("editor project document rejects unsafe source registry paths") {
    bool rejected_parent_source_registry = false;
    try {
        mirakana::editor::validate_project_document(mirakana::editor::ProjectDocument{
            .name = "bad",
            .root_path = "games/bad",
            .asset_root = "assets",
            .source_registry_path = "../package.geassets",
            .game_manifest_path = "game.agent.json",
            .startup_scene_path = "scenes/start.scene",
            .shader_tool = {},
            .render_backend = mirakana::editor::EditorRenderBackend::automatic,
        });
    } catch (const std::invalid_argument&) {
        rejected_parent_source_registry = true;
    }

    bool rejected_absolute_source_registry = false;
    try {
        mirakana::editor::validate_project_document(mirakana::editor::ProjectDocument{
            .name = "bad",
            .root_path = "games/bad",
            .asset_root = "assets",
            .source_registry_path = "C:/outside/package.geassets",
            .game_manifest_path = "game.agent.json",
            .startup_scene_path = "scenes/start.scene",
            .shader_tool = {},
            .render_backend = mirakana::editor::EditorRenderBackend::automatic,
        });
    } catch (const std::invalid_argument&) {
        rejected_absolute_source_registry = true;
    }

    bool rejected_non_geassets_source_registry = false;
    try {
        mirakana::editor::validate_project_document(mirakana::editor::ProjectDocument{
            .name = "bad",
            .root_path = "games/bad",
            .asset_root = "assets",
            .source_registry_path = "source/assets/package.txt",
            .game_manifest_path = "game.agent.json",
            .startup_scene_path = "scenes/start.scene",
            .shader_tool = {},
            .render_backend = mirakana::editor::EditorRenderBackend::automatic,
        });
    } catch (const std::invalid_argument&) {
        rejected_non_geassets_source_registry = true;
    }

    MK_REQUIRE(rejected_parent_source_registry);
    MK_REQUIRE(rejected_absolute_source_registry);
    MK_REQUIRE(rejected_non_geassets_source_registry);
}

MK_TEST("editor project document rejects unsafe shader tool settings") {
    bool rejected_shell_executable = false;
    try {
        mirakana::editor::validate_project_document(mirakana::editor::ProjectDocument{
            .name = "bad",
            .root_path = "games/bad",
            .asset_root = "assets",
            .source_registry_path = "source/assets/package.geassets",
            .game_manifest_path = "game.agent.json",
            .startup_scene_path = "scenes/start.scene",
            .shader_tool =
                mirakana::editor::ProjectShaderToolSettings{
                    .executable = "cmd.exe",
                    .working_directory = ".",
                    .artifact_output_root = "out/editor/shaders",
                    .cache_index_path = "out/editor/shaders/shader-cache.gecache",
                },
            .render_backend = mirakana::editor::EditorRenderBackend::automatic,
        });
    } catch (const std::invalid_argument&) {
        rejected_shell_executable = true;
    }

    bool rejected_parent_cache = false;
    try {
        mirakana::editor::validate_project_document(mirakana::editor::ProjectDocument{
            .name = "bad",
            .root_path = "games/bad",
            .asset_root = "assets",
            .source_registry_path = "source/assets/package.geassets",
            .game_manifest_path = "game.agent.json",
            .startup_scene_path = "scenes/start.scene",
            .shader_tool =
                mirakana::editor::ProjectShaderToolSettings{
                    .executable = "dxc",
                    .working_directory = ".",
                    .artifact_output_root = "out/editor/shaders",
                    .cache_index_path = "../shader-cache.gecache",
                },
            .render_backend = mirakana::editor::EditorRenderBackend::automatic,
        });
    } catch (const std::invalid_argument&) {
        rejected_parent_cache = true;
    }

    MK_REQUIRE(rejected_shell_executable);
    MK_REQUIRE(rejected_parent_cache);
}

MK_TEST("editor project document allows reviewed absolute shader tool executables") {
    const mirakana::editor::ProjectDocument document{
        .name = "sample",
        .root_path = "games/sample",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
        .shader_tool =
            mirakana::editor::ProjectShaderToolSettings{
                .executable = "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/dxc.exe",
                .working_directory = ".",
                .artifact_output_root = "out/editor/shaders",
                .cache_index_path = "out/editor/shaders/shader-cache.gecache",
            },
        .render_backend = mirakana::editor::EditorRenderBackend::automatic,
    };

    const auto serialized = mirakana::editor::serialize_project_document(document);
    MK_REQUIRE(serialized.contains("project.shader_tool.executable=C:/Program Files (x86)/Windows Kits/10/bin/"
                                   "10.0.26100.0/x64/dxc.exe"));

    const auto restored = mirakana::editor::deserialize_project_document(serialized);
    MK_REQUIRE(restored.shader_tool.executable ==
               "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/dxc.exe");
}

MK_TEST("editor project document rejects unsafe absolute shader tool executables") {
    bool rejected_absolute_shell = false;
    try {
        mirakana::editor::validate_project_document(mirakana::editor::ProjectDocument{
            .name = "bad",
            .root_path = "games/bad",
            .asset_root = "assets",
            .source_registry_path = "source/assets/package.geassets",
            .game_manifest_path = "game.agent.json",
            .startup_scene_path = "scenes/start.scene",
            .shader_tool =
                mirakana::editor::ProjectShaderToolSettings{
                    .executable = "C:/Windows/System32/powershell.exe",
                    .working_directory = ".",
                    .artifact_output_root = "out/editor/shaders",
                    .cache_index_path = "out/editor/shaders/shader-cache.gecache",
                },
            .render_backend = mirakana::editor::EditorRenderBackend::automatic,
        });
    } catch (const std::invalid_argument&) {
        rejected_absolute_shell = true;
    }

    bool rejected_parent_segment = false;
    try {
        mirakana::editor::validate_project_document(mirakana::editor::ProjectDocument{
            .name = "bad",
            .root_path = "games/bad",
            .asset_root = "assets",
            .source_registry_path = "source/assets/package.geassets",
            .game_manifest_path = "game.agent.json",
            .startup_scene_path = "scenes/start.scene",
            .shader_tool =
                mirakana::editor::ProjectShaderToolSettings{
                    .executable = "C:/toolchains/../dxc.exe",
                    .working_directory = ".",
                    .artifact_output_root = "out/editor/shaders",
                    .cache_index_path = "out/editor/shaders/shader-cache.gecache",
                },
            .render_backend = mirakana::editor::EditorRenderBackend::automatic,
        });
    } catch (const std::invalid_argument&) {
        rejected_parent_segment = true;
    }

    MK_REQUIRE(rejected_absolute_shell);
    MK_REQUIRE(rejected_parent_segment);
}

MK_TEST("editor project settings draft applies shader tool settings without mutating source") {
    const mirakana::editor::ProjectDocument source{
        .name = "sample",
        .root_path = "games/sample",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
        .shader_tool = {},
        .render_backend = mirakana::editor::EditorRenderBackend::automatic,
    };

    auto draft = mirakana::editor::ProjectSettingsDraft::from_project(source);
    draft.set_shader_tool_executable("toolchains/dxc/bin/dxc.exe");
    draft.set_shader_tool_working_directory(".");
    draft.set_shader_artifact_output_root("out/custom-shaders");
    draft.set_shader_cache_index_path("out/custom-shaders/cache.gecache");
    draft.set_render_backend(mirakana::editor::EditorRenderBackend::vulkan);

    MK_REQUIRE(draft.can_apply());
    MK_REQUIRE(draft.validation_errors().empty());

    const auto updated = draft.apply();
    MK_REQUIRE(source.shader_tool.executable == "dxc");
    MK_REQUIRE(updated.shader_tool.executable == "toolchains/dxc/bin/dxc.exe");
    MK_REQUIRE(updated.shader_tool.artifact_output_root == "out/custom-shaders");
    MK_REQUIRE(updated.shader_tool.cache_index_path == "out/custom-shaders/cache.gecache");
    MK_REQUIRE(updated.render_backend == mirakana::editor::EditorRenderBackend::vulkan);
}

MK_TEST("editor project settings draft applies discovered shader tool descriptor") {
    const mirakana::editor::ProjectDocument source{
        .name = "sample",
        .root_path = "games/sample",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
        .shader_tool = {},
        .render_backend = mirakana::editor::EditorRenderBackend::automatic,
    };

    auto draft = mirakana::editor::ProjectSettingsDraft::from_project(source);
    draft.set_shader_tool_descriptor(mirakana::ShaderToolDescriptor{.kind = mirakana::ShaderToolKind::dxc,
                                                                    .executable_path = "toolchains/dxc/bin/dxc.exe",
                                                                    .version = "dxcompiler 1.8"});

    const auto updated = draft.apply();
    MK_REQUIRE(updated.shader_tool.executable == "toolchains/dxc/bin/dxc.exe");
    MK_REQUIRE(updated.shader_tool.working_directory == ".");
}

MK_TEST("editor project settings draft reports invalid shader tool settings") {
    const mirakana::editor::ProjectDocument source{
        .name = "sample",
        .root_path = "games/sample",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
        .shader_tool = {},
        .render_backend = mirakana::editor::EditorRenderBackend::automatic,
    };

    auto draft = mirakana::editor::ProjectSettingsDraft::from_project(source);
    draft.set_shader_tool_executable("powershell.exe");
    draft.set_shader_cache_index_path("../cache.gecache");

    const auto errors = draft.validation_errors();
    MK_REQUIRE(!draft.can_apply());
    MK_REQUIRE(errors.size() == 2);
    MK_REQUIRE(errors[0].field == "shader_tool.executable");
    MK_REQUIRE(errors[1].field == "shader_tool.cache_index");

    bool rejected_apply = false;
    try {
        (void)draft.apply();
    } catch (const std::invalid_argument&) {
        rejected_apply = true;
    }
    MK_REQUIRE(rejected_apply);
}

MK_TEST("editor project bundle saves and loads project workspace and scene text") {
    mirakana::editor::MemoryTextStore store;
    const mirakana::editor::ProjectBundlePaths paths{
        .project_path = "game.geproject",
        .workspace_path = "workspace.geworkspace",
        .scene_path = "scenes/start.scene",
    };
    const mirakana::editor::ProjectDocument project{
        .name = "sample",
        .root_path = "games/sample",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
        .shader_tool = {},
        .render_backend = mirakana::editor::EditorRenderBackend::automatic,
    };
    auto workspace = mirakana::editor::Workspace::create_default(
        mirakana::editor::ProjectInfo{.name = "sample", .root_path = "games/sample"});
    workspace.set_panel_visible(mirakana::editor::PanelId::profiler, true);

    mirakana::editor::save_project_bundle(store, paths, project, workspace,
                                          "format=GameEngine.Scene.v1\nscene.name=start\nnode.count=0\n");

    MK_REQUIRE(store.exists("game.geproject"));
    MK_REQUIRE(store.exists("workspace.geworkspace"));
    MK_REQUIRE(store.exists("scenes/start.scene"));

    const auto loaded = mirakana::editor::load_project_bundle(store, paths);
    MK_REQUIRE(loaded.project.name == "sample");
    MK_REQUIRE(loaded.workspace.project().root_path == "games/sample");
    MK_REQUIRE(loaded.workspace.is_panel_visible(mirakana::editor::PanelId::profiler));
    MK_REQUIRE(loaded.scene_text.contains("scene.name=start"));
    MK_REQUIRE(!loaded.dirty);
}

MK_TEST("editor shader artifact manifest save and load round trips text store") {
    mirakana::editor::MemoryTextStore store;
    std::vector<mirakana::ShaderSourceMetadata> shaders{
        mirakana::ShaderSourceMetadata{
            .id = mirakana::AssetId::from_name("shaders/editor-grid.hlsl"),
            .source_path = "assets/shaders/editor-grid.hlsl",
            .language = mirakana::ShaderSourceLanguage::hlsl,
            .stage = mirakana::ShaderSourceStage::fragment,
            .entry_point = "ps_main",
            .defines = {"EDITOR_GRID=1"},
            .artifacts = {mirakana::ShaderGeneratedArtifact{.path = "out/shaders/editor-grid.ps.dxil",
                                                            .format = mirakana::ShaderArtifactFormat::dxil,
                                                            .profile = "ps_6_7",
                                                            .entry_point = "ps_main"}},
            .reflection = {mirakana::ShaderDescriptorReflection{
                .set = 0,
                .binding = 0,
                .resource_kind = mirakana::ShaderDescriptorResourceKind::uniform_buffer,
                .stage = mirakana::ShaderSourceStage::fragment,
                .count = 1,
                .semantic = "material_factors",
            }},
        },
    };

    mirakana::editor::save_shader_artifact_manifest(store, "artifacts/shaders.geartifacts", shaders);

    MK_REQUIRE(store.exists("artifacts/shaders.geartifacts"));
    const auto loaded = mirakana::editor::load_shader_artifact_manifest(store, "artifacts/shaders.geartifacts");
    MK_REQUIRE(loaded.size() == 1);
    MK_REQUIRE(loaded[0].source_path == "assets/shaders/editor-grid.hlsl");
    MK_REQUIRE(loaded[0].artifacts.size() == 1);
    MK_REQUIRE(loaded[0].artifacts[0].format == mirakana::ShaderArtifactFormat::dxil);
    MK_REQUIRE(loaded[0].reflection.size() == 1);
    MK_REQUIRE(loaded[0].reflection[0].resource_kind == mirakana::ShaderDescriptorResourceKind::uniform_buffer);
    MK_REQUIRE(loaded[0].reflection[0].semantic == "material_factors");
}

MK_TEST("editor shader artifact manifest IO rejects unsafe paths") {
    mirakana::editor::MemoryTextStore store;
    bool rejected_parent_path = false;
    try {
        mirakana::editor::save_shader_artifact_manifest(store, "../shaders.geartifacts", {});
    } catch (const std::invalid_argument&) {
        rejected_parent_path = true;
    }

    bool rejected_absolute_path = false;
    try {
        (void)mirakana::editor::load_shader_artifact_manifest(store, "C:/tmp/shaders.geartifacts");
    } catch (const std::invalid_argument&) {
        rejected_absolute_path = true;
    }

    MK_REQUIRE(rejected_parent_path);
    MK_REQUIRE(rejected_absolute_path);
}

MK_TEST("editor project bundle rejects missing paths and missing files") {
    mirakana::editor::MemoryTextStore store;

    bool rejected_paths = false;
    try {
        mirakana::editor::save_project_bundle(
            store,
            mirakana::editor::ProjectBundlePaths{
                .project_path = "", .workspace_path = "workspace.geworkspace", .scene_path = "scenes/start.scene"},
            mirakana::editor::ProjectDocument{.name = "sample",
                                              .root_path = "games/sample",
                                              .asset_root = "assets",
                                              .source_registry_path = "source/assets/package.geassets",
                                              .game_manifest_path = "game.agent.json",
                                              .startup_scene_path = "scenes/start.scene",
                                              .shader_tool = {},
                                              .render_backend = mirakana::editor::EditorRenderBackend::automatic},
            mirakana::editor::Workspace::create_default(
                mirakana::editor::ProjectInfo{.name = "sample", .root_path = "games/sample"}),
            "scene");
    } catch (const std::invalid_argument&) {
        rejected_paths = true;
    }

    bool rejected_load = false;
    try {
        (void)mirakana::editor::load_project_bundle(
            store, mirakana::editor::ProjectBundlePaths{.project_path = "game.geproject",
                                                        .workspace_path = "workspace.geworkspace",
                                                        .scene_path = "scenes/start.scene"});
    } catch (const std::out_of_range&) {
        rejected_load = true;
    }

    MK_REQUIRE(rejected_paths);
    MK_REQUIRE(rejected_load);
}

MK_TEST("editor content browser filters sorts and selects assets") {
    mirakana::AssetRegistry registry;
    registry.add(mirakana::AssetRecord{
        .id = mirakana::AssetId::from_name("textures/player.png"),
        .kind = mirakana::AssetKind::texture,
        .path = "textures/player.png",
    });
    registry.add(mirakana::AssetRecord{
        .id = mirakana::AssetId::from_name("materials/player.mat"),
        .kind = mirakana::AssetKind::material,
        .path = "materials/player.mat",
    });
    registry.add(mirakana::AssetRecord{
        .id = mirakana::AssetId::from_name("scenes/start.scene"),
        .kind = mirakana::AssetKind::scene,
        .path = "scenes/start.scene",
    });
    registry.add(mirakana::AssetRecord{
        .id = mirakana::AssetId::from_name("shaders/player.hlsl"),
        .kind = mirakana::AssetKind::shader,
        .path = "shaders/player.hlsl",
    });

    mirakana::editor::ContentBrowserState browser;
    browser.refresh_from(registry);

    const auto all_items = browser.visible_items();
    MK_REQUIRE(all_items.size() == 4);
    MK_REQUIRE(all_items[0].path == "materials/player.mat");
    MK_REQUIRE(all_items[0].display_name == "player.mat");
    MK_REQUIRE(all_items[0].directory == "materials");
    MK_REQUIRE(mirakana::editor::asset_kind_label(mirakana::AssetKind::shader) == "Shader");
    MK_REQUIRE(mirakana::editor::asset_kind_label(mirakana::AssetKind::animation_quaternion_clip) ==
               "Animation Quaternion Clip");

    browser.set_text_filter("player");
    MK_REQUIRE(browser.visible_items().size() == 3);

    browser.set_kind_filter(mirakana::AssetKind::texture);
    const auto texture_items = browser.visible_items();
    MK_REQUIRE(texture_items.size() == 1);
    MK_REQUIRE(texture_items[0].kind == mirakana::AssetKind::texture);
    MK_REQUIRE(browser.select(texture_items[0].id));
    MK_REQUIRE(browser.selected_asset() != nullptr);
    MK_REQUIRE(browser.selected_asset()->path == "textures/player.png");
}

MK_TEST("editor content browser annotates asset identity rows") {
    const mirakana::AssetKeyV2 material_key{"assets/materials/player"};
    const mirakana::AssetKeyV2 pose_key{"assets/animations/player_pose"};
    const mirakana::AssetKeyV2 texture_key{"assets/textures/player"};
    const auto material_id = mirakana::asset_id_from_key_v2(material_key);
    const auto pose_id = mirakana::asset_id_from_key_v2(pose_key);
    const auto texture_id = mirakana::asset_id_from_key_v2(texture_key);

    mirakana::AssetRegistry registry;
    registry.add(mirakana::AssetRecord{
        .id = texture_id, .kind = mirakana::AssetKind::texture, .path = "assets/textures/player.texture"});
    registry.add(mirakana::AssetRecord{
        .id = material_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/player.material"});

    mirakana::AssetIdentityDocumentV2 identity;
    identity.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = material_key, .kind = mirakana::AssetKind::material, .source_path = "source/materials/player.material"});
    identity.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = texture_key, .kind = mirakana::AssetKind::texture, .source_path = "source/textures/player.texture"});

    mirakana::editor::ContentBrowserState browser;
    browser.refresh_from(registry, identity);

    const auto all_items = browser.visible_items();
    MK_REQUIRE(all_items.size() == 2);
    MK_REQUIRE(all_items[0].id == material_id);
    MK_REQUIRE(all_items[0].asset_key.value == material_key.value);
    MK_REQUIRE(all_items[0].asset_key_label == material_key.value);
    MK_REQUIRE(all_items[0].identity_source_path == "source/materials/player.material");
    MK_REQUIRE(all_items[0].identity_backed);

    browser.set_text_filter("source/textures");
    const auto source_filtered = browser.visible_items();
    MK_REQUIRE(source_filtered.size() == 1);
    MK_REQUIRE(source_filtered[0].id == texture_id);

    browser.set_text_filter("assets/materials/player");
    const auto key_filtered = browser.visible_items();
    MK_REQUIRE(key_filtered.size() == 1);
    MK_REQUIRE(key_filtered[0].id == material_id);
    MK_REQUIRE(browser.select(material_id));
    MK_REQUIRE(browser.selected_asset() != nullptr);
    MK_REQUIRE(browser.selected_asset()->identity_source_path == "source/materials/player.material");
}

MK_TEST("editor content browser populates source registry rows") {
    const mirakana::AssetKeyV2 material_key{"assets/materials/player"};
    const mirakana::AssetKeyV2 texture_key{"assets/textures/player"};
    const auto material_id = mirakana::asset_id_from_key_v2(material_key);
    const auto texture_id = mirakana::asset_id_from_key_v2(texture_key);

    mirakana::SourceAssetRegistryDocumentV1 source_registry;
    source_registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = texture_key,
        .kind = mirakana::AssetKind::texture,
        .source_path = "source/textures/player.png",
        .source_format = std::string{mirakana::expected_source_asset_format_v1(mirakana::AssetKind::texture)},
        .imported_path = "assets/textures/player.texture",
    });
    source_registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = material_key,
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/player.material",
        .source_format = std::string{mirakana::expected_source_asset_format_v1(mirakana::AssetKind::material)},
        .imported_path = "assets/materials/player.material",
        .dependencies = {mirakana::SourceAssetDependencyRowV1{.kind = mirakana::AssetDependencyKind::material_texture,
                                                              .key = texture_key}},
    });
    const auto source_registry_diagnostics = mirakana::validate_source_asset_registry_document(source_registry);
    MK_REQUIRE(source_registry_diagnostics.empty());

    mirakana::editor::ContentBrowserState browser;
    browser.refresh_from(source_registry);

    const auto all_items = browser.visible_items();
    MK_REQUIRE(all_items.size() == 2);
    MK_REQUIRE(all_items[0].id == material_id);
    MK_REQUIRE(all_items[0].kind == mirakana::AssetKind::material);
    MK_REQUIRE(all_items[0].path == "assets/materials/player.material");
    MK_REQUIRE(all_items[0].display_name == "player.material");
    MK_REQUIRE(all_items[0].directory == "assets/materials");
    MK_REQUIRE(all_items[0].asset_key.value == material_key.value);
    MK_REQUIRE(all_items[0].asset_key_label == material_key.value);
    MK_REQUIRE(all_items[0].identity_source_path == "source/materials/player.material");
    MK_REQUIRE(all_items[0].identity_backed);

    browser.set_text_filter("source/textures");
    const auto source_filtered = browser.visible_items();
    MK_REQUIRE(source_filtered.size() == 1);
    MK_REQUIRE(source_filtered[0].id == texture_id);

    browser.set_text_filter("assets/materials/player");
    const auto key_filtered = browser.visible_items();
    MK_REQUIRE(key_filtered.size() == 1);
    MK_REQUIRE(key_filtered[0].id == material_id);
    MK_REQUIRE(browser.select(material_key));
    MK_REQUIRE(browser.selected_asset() != nullptr);
    MK_REQUIRE(browser.selected_asset()->identity_source_path == "source/materials/player.material");

    mirakana::AssetImportMetadataRegistry imports;
    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player.png",
        .imported_path = "assets/textures/player.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/player.material",
        .imported_path = "assets/materials/player.material",
        .texture_dependencies = {texture_id},
    });

    const auto plan = mirakana::build_asset_import_plan(imports);
    mirakana::editor::AssetPipelineState pipeline;
    pipeline.set_import_plan(plan);

    const auto model = mirakana::editor::make_editor_content_browser_import_panel_model(browser, pipeline, plan);
    MK_REQUIRE(model.visible_asset_count == 1);
    MK_REQUIRE(model.assets.size() == 1);
    MK_REQUIRE(model.assets[0].asset == material_id);
    MK_REQUIRE(model.assets[0].asset_key_label == material_key.value);
    MK_REQUIRE(model.assets[0].identity_source_path == "source/materials/player.material");
    MK_REQUIRE(model.assets[0].identity_backed);
    MK_REQUIRE(model.has_selected_asset);
    MK_REQUIRE(model.selected_asset.asset_key_label == material_key.value);
    MK_REQUIRE(model.selected_asset.identity_source_path == "source/materials/player.material");
}

MK_TEST("editor source registry browser refresh loads project registry into content browser") {
    const mirakana::AssetKeyV2 material_key{"assets/materials/player"};
    const mirakana::AssetKeyV2 pose_key{"assets/animations/player_pose"};
    const mirakana::AssetKeyV2 texture_key{"assets/textures/player"};
    const auto material_id = mirakana::asset_id_from_key_v2(material_key);
    const auto pose_id = mirakana::asset_id_from_key_v2(pose_key);
    const auto texture_id = mirakana::asset_id_from_key_v2(texture_key);

    mirakana::SourceAssetRegistryDocumentV1 source_registry;
    source_registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = texture_key,
        .kind = mirakana::AssetKind::texture,
        .source_path = "source/textures/player.texture",
        .source_format = std::string{mirakana::expected_source_asset_format_v1(mirakana::AssetKind::texture)},
        .imported_path = "assets/textures/player.texture",
    });
    source_registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = material_key,
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/player.material",
        .source_format = std::string{mirakana::expected_source_asset_format_v1(mirakana::AssetKind::material)},
        .imported_path = "assets/materials/player.material",
        .dependencies = {mirakana::SourceAssetDependencyRowV1{.kind = mirakana::AssetDependencyKind::material_texture,
                                                              .key = texture_key}},
    });
    source_registry.assets.push_back(mirakana::SourceAssetRegistryRowV1{
        .key = pose_key,
        .kind = mirakana::AssetKind::animation_quaternion_clip,
        .source_path = "source/animations/player_pose.animation_quaternion_clip_source",
        .source_format =
            std::string{mirakana::expected_source_asset_format_v1(mirakana::AssetKind::animation_quaternion_clip)},
        .imported_path = "assets/animations/player_pose.animation_quaternion_clip",
    });

    mirakana::editor::MemoryTextStore store;
    store.write_text("games/sample/source/assets/package.geassets",
                     mirakana::serialize_source_asset_registry_document(source_registry));
    const mirakana::editor::ProjectDocument project{
        .name = "sample",
        .root_path = "games/sample",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
        .shader_tool = {},
        .render_backend = mirakana::editor::EditorRenderBackend::automatic,
    };

    mirakana::editor::ContentBrowserState browser;
    const auto result = mirakana::editor::refresh_content_browser_from_project_source_registry(store, project, browser);

    MK_REQUIRE(result.status == mirakana::editor::EditorSourceRegistryBrowserRefreshStatus::loaded);
    MK_REQUIRE(result.loaded);
    MK_REQUIRE(result.source_registry_exists);
    MK_REQUIRE(result.source_registry_path == "games/sample/source/assets/package.geassets");
    MK_REQUIRE(result.asset_count == 3);
    MK_REQUIRE(result.import_plan.actions.size() == 3);
    MK_REQUIRE(std::ranges::any_of(result.import_plan.actions, [texture_id](const mirakana::AssetImportAction& action) {
        return action.id == texture_id && action.source_path == "source/textures/player.texture";
    }));
    MK_REQUIRE(
        std::ranges::any_of(result.import_plan.actions, [material_id](const mirakana::AssetImportAction& action) {
            return action.id == material_id && action.output_path == "assets/materials/player.material";
        }));
    MK_REQUIRE(std::ranges::any_of(result.import_plan.actions, [pose_id](const mirakana::AssetImportAction& action) {
        return action.id == pose_id && action.kind == mirakana::AssetImportActionKind::animation_quaternion_clip;
    }));
    MK_REQUIRE(browser.item_count() == 3);
    MK_REQUIRE(browser.select(material_key));
    MK_REQUIRE(browser.selected_asset() != nullptr);
    MK_REQUIRE(browser.selected_asset()->asset_key_label == material_key.value);
    MK_REQUIRE(browser.selected_asset()->identity_source_path == "source/materials/player.material");

    mirakana::editor::AssetPipelineState pipeline;
    pipeline.set_import_plan(result.import_plan);
    const auto model =
        mirakana::editor::make_editor_content_browser_import_panel_model(browser, pipeline, result.import_plan);
    const auto pose_asset =
        std::ranges::find_if(model.assets, [pose_id](const auto& item) { return item.asset == pose_id; });
    MK_REQUIRE(pose_asset != model.assets.end());
    MK_REQUIRE(pose_asset->kind_label == "Animation Quaternion Clip");
    const auto pose_import =
        std::ranges::find_if(model.import_queue, [pose_id](const auto& item) { return item.asset == pose_id; });
    MK_REQUIRE(pose_import != model.import_queue.end());
    MK_REQUIRE(pose_import->kind_label == "Animation Quaternion Clip");
}

MK_TEST("editor source registry browser refresh leaves browser unchanged when registry is missing") {
    mirakana::AssetRegistry fallback;
    fallback.add(mirakana::AssetRecord{
        .id = mirakana::AssetId::from_name("editor.default.texture"),
        .kind = mirakana::AssetKind::texture,
        .path = "builtin/default.texture",
    });

    mirakana::editor::ContentBrowserState browser;
    browser.refresh_from(fallback);
    MK_REQUIRE(browser.select(mirakana::AssetId::from_name("editor.default.texture")));

    mirakana::editor::MemoryTextStore store;
    const mirakana::editor::ProjectDocument project{
        .name = "sample",
        .root_path = "games/sample",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
        .shader_tool = {},
        .render_backend = mirakana::editor::EditorRenderBackend::automatic,
    };

    const auto result = mirakana::editor::refresh_content_browser_from_project_source_registry(store, project, browser);

    MK_REQUIRE(result.status == mirakana::editor::EditorSourceRegistryBrowserRefreshStatus::missing);
    MK_REQUIRE(!result.loaded);
    MK_REQUIRE(!result.source_registry_exists);
    MK_REQUIRE(!result.diagnostics.empty());
    MK_REQUIRE(browser.item_count() == 1);
    MK_REQUIRE(browser.selected_asset() != nullptr);
    MK_REQUIRE(browser.selected_asset()->path == "builtin/default.texture");
}

MK_TEST("editor source registry browser refresh leaves browser unchanged when registry is invalid") {
    mirakana::AssetRegistry fallback;
    fallback.add(mirakana::AssetRecord{
        .id = mirakana::AssetId::from_name("editor.default.material"),
        .kind = mirakana::AssetKind::material,
        .path = "builtin/default.material",
    });

    mirakana::editor::ContentBrowserState browser;
    browser.refresh_from(fallback);
    MK_REQUIRE(browser.select(mirakana::AssetId::from_name("editor.default.material")));

    mirakana::editor::MemoryTextStore store;
    store.write_text("games/sample/source/assets/package.geassets",
                     "format=GameEngine.SourceAssetRegistry.v1\nasset.0.key=bad key\n");
    const mirakana::editor::ProjectDocument project{
        .name = "sample",
        .root_path = "games/sample",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
        .shader_tool = {},
        .render_backend = mirakana::editor::EditorRenderBackend::automatic,
    };

    const auto result = mirakana::editor::refresh_content_browser_from_project_source_registry(store, project, browser);

    MK_REQUIRE(result.status == mirakana::editor::EditorSourceRegistryBrowserRefreshStatus::blocked);
    MK_REQUIRE(!result.loaded);
    MK_REQUIRE(result.source_registry_exists);
    MK_REQUIRE(!result.diagnostics.empty());
    MK_REQUIRE(browser.item_count() == 1);
    MK_REQUIRE(browser.selected_asset() != nullptr);
    MK_REQUIRE(browser.selected_asset()->path == "builtin/default.material");
}

MK_TEST("editor asset pipeline tracks import statuses and hot reload events") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto texture_id = mirakana::AssetId::from_name("textures/player");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");

    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player.txt",
        .imported_path = "assets/textures/player.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_mesh(mirakana::MeshImportMetadata{
        .id = mesh_id,
        .source_path = "source/meshes/player.txt",
        .imported_path = "assets/meshes/player.mesh",
        .scale = 1.0F,
        .generate_lods = false,
        .generate_collision = true,
    });

    mirakana::editor::AssetPipelineState pipeline;
    pipeline.set_import_plan(mirakana::build_asset_import_plan(imports));

    MK_REQUIRE(pipeline.item_count() == 2);
    MK_REQUIRE(pipeline.pending_count() == 2);
    MK_REQUIRE(pipeline.items()[0].output_path == "assets/meshes/player.mesh");
    MK_REQUIRE(pipeline.items()[0].status == mirakana::editor::EditorAssetImportStatus::pending);

    pipeline.apply_import_updates({
        mirakana::editor::EditorAssetImportUpdate{.asset = mesh_id, .imported = true, .diagnostic = {}},
        mirakana::editor::EditorAssetImportUpdate{
            .asset = texture_id, .imported = false, .diagnostic = "missing source"},
    });

    MK_REQUIRE(pipeline.imported_count() == 1);
    MK_REQUIRE(pipeline.failed_count() == 1);
    MK_REQUIRE(pipeline.items()[1].status == mirakana::editor::EditorAssetImportStatus::failed);
    MK_REQUIRE(pipeline.items()[1].diagnostic == "missing source");

    pipeline.apply_hot_reload_events({
        mirakana::AssetHotReloadEvent{
            .kind = mirakana::AssetHotReloadEventKind::modified,
            .asset = texture_id,
            .path = "assets/textures/player.texture",
            .previous_revision = 1,
            .current_revision = 2,
            .previous_size_bytes = 100,
            .current_size_bytes = 120,
        },
    });

    MK_REQUIRE(pipeline.hot_reload_events().size() == 1);
    MK_REQUIRE(pipeline.hot_reload_events()[0].asset == texture_id);

    pipeline.apply_recook_requests({
        mirakana::AssetHotReloadRecookRequest{
            .asset = texture_id,
            .source_asset = texture_id,
            .trigger_path = "assets/textures/player.texture",
            .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
            .reason = mirakana::AssetHotReloadRecookReason::source_modified,
            .previous_revision = 1,
            .current_revision = 2,
            .ready_tick = 8,
        },
        mirakana::AssetHotReloadRecookRequest{
            .asset = mesh_id,
            .source_asset = texture_id,
            .trigger_path = "assets/textures/player.texture",
            .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
            .reason = mirakana::AssetHotReloadRecookReason::dependency_invalidated,
            .previous_revision = 1,
            .current_revision = 2,
            .ready_tick = 8,
        },
    });
    MK_REQUIRE(pipeline.recook_requests().size() == 2);
    MK_REQUIRE(pipeline.recook_requests()[0].asset == mesh_id);
    MK_REQUIRE(pipeline.recook_requests()[0].reason == mirakana::AssetHotReloadRecookReason::dependency_invalidated);
    MK_REQUIRE(pipeline.recook_requests()[1].asset == texture_id);

    pipeline.apply_hot_reload_results({
        mirakana::AssetHotReloadApplyResult{
            .kind = mirakana::AssetHotReloadApplyResultKind::failed_rolled_back,
            .asset = texture_id,
            .path = "assets/textures/player.texture",
            .requested_revision = 2,
            .active_revision = 1,
            .diagnostic = "decode failed",
        },
        mirakana::AssetHotReloadApplyResult{
            .kind = mirakana::AssetHotReloadApplyResultKind::applied,
            .asset = mesh_id,
            .path = "assets/meshes/player.mesh",
            .requested_revision = 4,
            .active_revision = 4,
            .diagnostic = {},
        },
    });
    MK_REQUIRE(pipeline.hot_reload_results().size() == 2);
    MK_REQUIRE(pipeline.applied_hot_reload_count() == 1);
    MK_REQUIRE(pipeline.failed_hot_reload_count() == 1);
    MK_REQUIRE(pipeline.hot_reload_results()[0].asset == mesh_id);
    MK_REQUIRE(pipeline.hot_reload_results()[1].diagnostic == "decode failed");
    MK_REQUIRE(mirakana::editor::editor_asset_hot_reload_event_kind_label(
                   mirakana::AssetHotReloadEventKind::modified) == "Modified");
    MK_REQUIRE(mirakana::editor::editor_asset_hot_reload_recook_reason_label(
                   mirakana::AssetHotReloadRecookReason::source_modified) == "Source Modified");
    MK_REQUIRE(mirakana::editor::editor_asset_hot_reload_apply_result_label(
                   mirakana::AssetHotReloadApplyResultKind::staged) == "Staged");
    MK_REQUIRE(mirakana::editor::editor_asset_hot_reload_apply_result_label(
                   mirakana::AssetHotReloadApplyResultKind::failed_rolled_back) == "Failed Rolled Back");
}

MK_TEST("editor asset pipeline applies import execution results") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto texture_id = mirakana::AssetId::from_name("textures/player");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");

    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player.texture-source",
        .imported_path = "assets/textures/player.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_mesh(mirakana::MeshImportMetadata{
        .id = mesh_id,
        .source_path = "source/meshes/player.mesh-source",
        .imported_path = "assets/meshes/player.mesh",
        .scale = 1.0F,
        .generate_lods = false,
        .generate_collision = true,
    });

    mirakana::editor::AssetPipelineState pipeline;
    pipeline.set_import_plan(mirakana::build_asset_import_plan(imports));

    mirakana::AssetImportExecutionResult result;
    result.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = mesh_id, .kind = mirakana::AssetImportActionKind::mesh, .output_path = "assets/meshes/player.mesh"});
    result.failures.push_back(mirakana::AssetImportFailure{.asset = texture_id,
                                                           .kind = mirakana::AssetImportActionKind::texture,
                                                           .source_path = "source/textures/player.texture-source",
                                                           .output_path = "assets/textures/player.texture",
                                                           .diagnostic = "missing source"});

    pipeline.apply_import_execution_result(result);

    MK_REQUIRE(pipeline.imported_count() == 1);
    MK_REQUIRE(pipeline.failed_count() == 1);
    MK_REQUIRE(pipeline.items()[0].status == mirakana::editor::EditorAssetImportStatus::imported);
    MK_REQUIRE(pipeline.items()[1].status == mirakana::editor::EditorAssetImportStatus::failed);
    MK_REQUIRE(pipeline.items()[1].diagnostic == "missing source");
}

MK_TEST("editor asset pipeline builds progress dependency and thumbnail models") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto texture_id = mirakana::AssetId::from_name("textures/player");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");
    const auto scene_id = mirakana::AssetId::from_name("scenes/level");

    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player.texture-source",
        .imported_path = "assets/textures/player.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/player.material",
        .imported_path = "assets/materials/player.material",
        .texture_dependencies = {texture_id},
    });
    imports.add_mesh(mirakana::MeshImportMetadata{
        .id = mesh_id,
        .source_path = "source/meshes/player.mesh-source",
        .imported_path = "assets/meshes/player.mesh",
        .scale = 1.0F,
        .generate_lods = false,
        .generate_collision = true,
    });
    imports.add_scene(mirakana::SceneImportMetadata{
        .id = scene_id,
        .source_path = "source/scenes/level.scene",
        .imported_path = "assets/scenes/level.scene",
        .mesh_dependencies = {mesh_id},
        .material_dependencies = {material_id},
        .sprite_dependencies = {},
    });

    const auto plan = mirakana::build_asset_import_plan(imports);
    mirakana::editor::AssetPipelineState pipeline;
    pipeline.set_import_plan(plan);
    pipeline.apply_import_updates({
        mirakana::editor::EditorAssetImportUpdate{.asset = material_id, .imported = true, .diagnostic = {}},
        mirakana::editor::EditorAssetImportUpdate{
            .asset = texture_id, .imported = false, .diagnostic = "decode failed"},
    });

    const auto progress = mirakana::editor::make_editor_asset_import_progress(pipeline);
    MK_REQUIRE(progress.total_count == 4);
    MK_REQUIRE(progress.pending_count == 2);
    MK_REQUIRE(progress.imported_count == 1);
    MK_REQUIRE(progress.failed_count == 1);
    MK_REQUIRE(progress.completed_count == 2);
    MK_REQUIRE(progress.completion_ratio == 0.5F);

    const auto dependencies = mirakana::editor::make_editor_asset_dependency_items(plan);
    MK_REQUIRE(dependencies.size() == 3);
    MK_REQUIRE(dependencies[0].asset == scene_id);
    MK_REQUIRE(dependencies[0].dependency == material_id);
    MK_REQUIRE(dependencies[0].path == "assets/materials/player.material");
    MK_REQUIRE(dependencies[1].asset == scene_id);
    MK_REQUIRE(dependencies[1].dependency == mesh_id);
    MK_REQUIRE(dependencies[1].path == "assets/meshes/player.mesh");
    MK_REQUIRE(dependencies[2].asset == material_id);
    MK_REQUIRE(dependencies[2].dependency == texture_id);
    MK_REQUIRE(dependencies[2].path == "assets/textures/player.texture");

    const auto thumbnails = mirakana::editor::make_editor_asset_thumbnail_requests(plan);
    MK_REQUIRE(thumbnails.size() == 4);
    MK_REQUIRE(thumbnails[0].asset == material_id);
    MK_REQUIRE(thumbnails[0].kind == mirakana::editor::EditorAssetThumbnailKind::material);
    MK_REQUIRE(thumbnails[0].label == "Material");
    MK_REQUIRE(thumbnails[1].kind == mirakana::editor::EditorAssetThumbnailKind::mesh);
    MK_REQUIRE(thumbnails[2].asset == scene_id);
    MK_REQUIRE(thumbnails[2].kind == mirakana::editor::EditorAssetThumbnailKind::scene);
    MK_REQUIRE(thumbnails[2].label == "Scene");
    MK_REQUIRE(thumbnails[3].kind == mirakana::editor::EditorAssetThumbnailKind::texture);
}

MK_TEST("editor material preview exposes factor and binding metadata") {
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::transparent,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.25F, 0.5F, 0.75F, 1.0F},
                .emissive = {0.1F, 0.2F, 0.3F},
                .metallic = 0.1F,
                .roughness = 0.8F,
            },
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_id}},
        .double_sided = true,
    };

    const auto preview = mirakana::editor::make_editor_material_preview(material);

    MK_REQUIRE(preview.material == material_id);
    MK_REQUIRE(preview.name == "Player");
    MK_REQUIRE(preview.base_color[0] == 0.25F);
    MK_REQUIRE(preview.emissive[0] == 0.1F);
    MK_REQUIRE(preview.emissive[1] == 0.2F);
    MK_REQUIRE(preview.emissive[2] == 0.3F);
    MK_REQUIRE(preview.roughness == 0.8F);
    MK_REQUIRE(preview.double_sided);
    MK_REQUIRE(preview.requires_alpha_blending);
    MK_REQUIRE(preview.material_uniform_bytes == 64);
    MK_REQUIRE(preview.bindings.size() == 4);
    MK_REQUIRE(preview.bindings[2].semantic == "texture.base_color");
    MK_REQUIRE(preview.bindings[3].resource_kind == mirakana::MaterialBindingResourceKind::sampler);
    MK_REQUIRE(preview.bindings[3].semantic == "sampler.base_color");
}

MK_TEST("editor material authoring model exposes editable rows validation dirty and staging") {
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    auto document = mirakana::editor::MaterialAuthoringDocument::from_material(
        mirakana::MaterialDefinition{
            .id = material_id,
            .name = "Player",
            .shading_model = mirakana::MaterialShadingModel::lit,
            .surface_mode = mirakana::MaterialSurfaceMode::opaque,
            .factors =
                mirakana::MaterialFactors{
                    .base_color = {0.25F, 0.5F, 0.75F, 1.0F},
                    .emissive = {0.0F, 0.1F, 0.2F},
                    .metallic = 0.2F,
                    .roughness = 0.7F,
                },
            .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                                  .texture = texture_id}},
            .double_sided = false,
        },
        "assets/materials/player.material");
    mirakana::AssetRegistry registry;
    registry.add(mirakana::AssetRecord{
        .id = texture_id, .kind = mirakana::AssetKind::texture, .path = "assets/textures/player.texture"});

    auto model = document.model(registry);
    MK_REQUIRE(!model.dirty);
    MK_REQUIRE(!model.staged);
    MK_REQUIRE(model.valid());
    MK_REQUIRE(model.material == material_id);
    MK_REQUIRE(model.artifact_path == "assets/materials/player.material");
    MK_REQUIRE(model.factor_rows.size() == 4);
    MK_REQUIRE(model.factor_rows[0].id == "base_color");
    MK_REQUIRE(model.factor_rows[0].component_count == 4);
    MK_REQUIRE(model.factor_rows[0].values[2] == 0.75F);
    MK_REQUIRE(model.factor_rows[2].factor == mirakana::editor::MaterialAuthoringFactor::metallic);
    MK_REQUIRE(model.texture_rows.size() == 5);
    MK_REQUIRE(model.texture_rows[0].slot == mirakana::MaterialTextureSlot::base_color);
    MK_REQUIRE(model.texture_rows[0].texture == texture_id);
    MK_REQUIRE(model.texture_rows[0].status == mirakana::editor::MaterialAuthoringTextureStatus::resolved);
    MK_REQUIRE(model.texture_rows[0].bindings.size() == 2);
    MK_REQUIRE(model.texture_rows[0].bindings[0].semantic == "texture.base_color");
    MK_REQUIRE(model.texture_rows[0].bindings[1].semantic == "sampler.base_color");
    MK_REQUIRE(model.texture_rows[1].slot == mirakana::MaterialTextureSlot::normal);
    MK_REQUIRE(model.texture_rows[1].texture.value == 0);
    MK_REQUIRE(model.texture_rows[1].status == mirakana::editor::MaterialAuthoringTextureStatus::empty);
    MK_REQUIRE(mirakana::editor::material_authoring_texture_dependencies(document).size() == 1);

    MK_REQUIRE(!document.set_metallic(1.25F));
    model = document.model(registry);
    MK_REQUIRE(model.valid());
    MK_REQUIRE(!model.dirty);
    MK_REQUIRE(model.factor_rows[2].values[0] == 0.2F);

    auto invalid_replacement = document.material();
    invalid_replacement.id = mirakana::AssetId{};
    MK_REQUIRE(!document.replace_material(invalid_replacement));
    MK_REQUIRE(document.material().id == material_id);
    MK_REQUIRE(document.model(registry).valid());

    MK_REQUIRE(document.set_metallic(0.55F));
    const auto missing_texture = mirakana::AssetId::from_name("textures/missing");
    MK_REQUIRE(document.set_texture(mirakana::MaterialTextureSlot::base_color, missing_texture));
    model = document.model(registry);
    MK_REQUIRE(!model.valid());
    MK_REQUIRE(model.diagnostics.size() == 1);
    MK_REQUIRE(model.diagnostics[0].field == "texture.base_color");
    MK_REQUIRE(model.texture_rows[0].status == mirakana::editor::MaterialAuthoringTextureStatus::missing);
    MK_REQUIRE(!document.stage_changes(registry));
    mirakana::editor::MemoryTextStore rejected_store;
    bool rejected_missing_texture_save = false;
    try {
        mirakana::editor::save_material_authoring_document(rejected_store, "assets/materials/player.material", document,
                                                           registry);
    } catch (const std::invalid_argument&) {
        rejected_missing_texture_save = true;
    }
    MK_REQUIRE(rejected_missing_texture_save);
    MK_REQUIRE(!rejected_store.exists("assets/materials/player.material"));

    const auto wrong_kind_texture = mirakana::AssetId::from_name("materials/not-a-texture");
    registry.add(mirakana::AssetRecord{.id = wrong_kind_texture,
                                       .kind = mirakana::AssetKind::material,
                                       .path = "assets/materials/not-a-texture.material"});
    MK_REQUIRE(document.set_texture(mirakana::MaterialTextureSlot::base_color, wrong_kind_texture));
    model = document.model(registry);
    MK_REQUIRE(!model.valid());
    MK_REQUIRE(model.texture_rows[0].status == mirakana::editor::MaterialAuthoringTextureStatus::wrong_kind);
    MK_REQUIRE(!document.stage_changes(registry));

    MK_REQUIRE(document.set_texture(mirakana::MaterialTextureSlot::base_color, texture_id));
    MK_REQUIRE(document.stage_changes(registry));
    model = document.model(registry);
    MK_REQUIRE(model.valid());
    MK_REQUIRE(model.dirty);
    MK_REQUIRE(model.staged);
}

MK_TEST("editor material authoring save load and undo redo edits") {
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto base_texture = mirakana::AssetId::from_name("textures/player.albedo");
    const auto normal_texture = mirakana::AssetId::from_name("textures/player.normal");
    auto document = mirakana::editor::MaterialAuthoringDocument::from_material(
        mirakana::MaterialDefinition{
            .id = material_id,
            .name = "Player",
            .shading_model = mirakana::MaterialShadingModel::lit,
            .surface_mode = mirakana::MaterialSurfaceMode::opaque,
            .factors = mirakana::MaterialFactors{},
            .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                                  .texture = base_texture}},
            .double_sided = false,
        },
        "assets/materials/player.material");

    mirakana::editor::UndoStack history;
    MK_REQUIRE(!history.execute(mirakana::editor::make_material_authoring_factor_edit_action(
        document, mirakana::editor::MaterialAuthoringFactor::metallic, std::array<float, 4>{1.25F, 0.0F, 0.0F, 0.0F})));
    MK_REQUIRE(document.material().factors.metallic == 0.0F);
    MK_REQUIRE(!document.dirty());

    MK_REQUIRE(history.execute(mirakana::editor::make_material_authoring_factor_edit_action(
        document, mirakana::editor::MaterialAuthoringFactor::roughness,
        std::array<float, 4>{0.42F, 0.0F, 0.0F, 0.0F})));
    MK_REQUIRE(document.material().factors.roughness == 0.42F);
    MK_REQUIRE(document.dirty());

    MK_REQUIRE(history.execute(mirakana::editor::make_material_authoring_texture_edit_action(
        document, mirakana::MaterialTextureSlot::normal, normal_texture)));
    MK_REQUIRE(document.model().texture_rows.size() == 5);
    MK_REQUIRE(mirakana::editor::material_authoring_texture_dependencies(document).size() == 2);
    MK_REQUIRE(mirakana::editor::material_authoring_texture_dependencies(document)[0] == base_texture);
    MK_REQUIRE(mirakana::editor::material_authoring_texture_dependencies(document)[1] == normal_texture);
    MK_REQUIRE(history.undo());
    MK_REQUIRE(mirakana::editor::material_authoring_texture_dependencies(document).size() == 1);
    MK_REQUIRE(history.redo());
    MK_REQUIRE(mirakana::editor::material_authoring_texture_dependencies(document).size() == 2);
    MK_REQUIRE(history.undo());
    MK_REQUIRE(history.undo());
    MK_REQUIRE(!document.dirty());
    MK_REQUIRE(document.material().factors.roughness == 1.0F);

    MK_REQUIRE(history.redo());
    MK_REQUIRE(history.redo());
    MK_REQUIRE(document.stage_changes());

    mirakana::editor::MemoryTextStore store;
    mirakana::editor::save_material_authoring_document(store, "assets/materials/player.material", document);
    MK_REQUIRE(store.exists("assets/materials/player.material"));
    MK_REQUIRE(!document.dirty());
    MK_REQUIRE(!document.staged());

    const auto loaded = mirakana::editor::load_material_authoring_document(store, "assets/materials/player.material");
    MK_REQUIRE(!loaded.dirty());
    MK_REQUIRE(loaded.material().id == material_id);
    MK_REQUIRE(loaded.material().factors.roughness == 0.42F);
    MK_REQUIRE(loaded.model().texture_rows.size() == 5);
    MK_REQUIRE(store.read_text("assets/materials/player.material") ==
               mirakana::serialize_material_definition(loaded.material()));
}

MK_TEST("editor material graph authoring round trip lowering validation undo and registry diagnostics") {
    const auto material_id = mirakana::AssetId::from_name("materials/graph_mat");
    const auto texture_id = mirakana::AssetId::from_name("textures/albedo");

    mirakana::MaterialGraphDesc graph{};
    graph.material_id = material_id;
    graph.material_name = "GraphMat";
    graph.shading_model = mirakana::MaterialShadingModel::lit;
    graph.surface_mode = mirakana::MaterialSurfaceMode::opaque;
    graph.double_sided = true;
    graph.output_node_id = "out";

    mirakana::MaterialGraphNode out_node;
    out_node.id = "out";
    out_node.kind = mirakana::MaterialGraphNodeKind::graph_output;

    mirakana::MaterialGraphNode bc;
    bc.id = "bc_const";
    bc.kind = mirakana::MaterialGraphNodeKind::constant_vec4;
    bc.vec4 = {0.1F, 0.2F, 0.3F, 0.4F};

    mirakana::MaterialGraphNode em;
    em.id = "em_const";
    em.kind = mirakana::MaterialGraphNodeKind::constant_vec3;
    em.vec3 = {1.0F, 2.0F, 3.0F};

    mirakana::MaterialGraphNode met;
    met.id = "metallic_node";
    met.kind = mirakana::MaterialGraphNodeKind::constant_scalar;
    met.scalar_key = "metallic";
    met.scalar_value = 0.35F;

    mirakana::MaterialGraphNode rou;
    rou.id = "roughness_node";
    rou.kind = mirakana::MaterialGraphNodeKind::constant_scalar;
    rou.scalar_key = "roughness";
    rou.scalar_value = 0.55F;

    mirakana::MaterialGraphNode tex;
    tex.id = "tex0";
    tex.kind = mirakana::MaterialGraphNodeKind::texture;
    tex.texture_slot = mirakana::MaterialTextureSlot::base_color;
    tex.texture_id = texture_id;

    graph.nodes = {out_node, bc, em, met, rou, tex};
    graph.edges = {
        mirakana::MaterialGraphEdge{
            .from_node = "bc_const", .from_socket = "out", .to_node = "out", .to_socket = "factor.base_color"},
        mirakana::MaterialGraphEdge{
            .from_node = "em_const", .from_socket = "out", .to_node = "out", .to_socket = "factor.emissive"},
        mirakana::MaterialGraphEdge{
            .from_node = "metallic_node", .from_socket = "out", .to_node = "out", .to_socket = "factor.metallic"},
        mirakana::MaterialGraphEdge{
            .from_node = "roughness_node", .from_socket = "out", .to_node = "out", .to_socket = "factor.roughness"},
        mirakana::MaterialGraphEdge{
            .from_node = "tex0", .from_socket = "out", .to_node = "out", .to_socket = "texture.base_color"},
    };

    const auto text = mirakana::serialize_material_graph(graph);
    const auto parsed = mirakana::deserialize_material_graph(text);
    MK_REQUIRE(parsed == graph);

    const auto lowered = mirakana::lower_material_graph_to_definition(parsed);
    MK_REQUIRE(lowered.id == material_id);
    MK_REQUIRE(lowered.factors.base_color == bc.vec4);
    MK_REQUIRE(lowered.factors.emissive == em.vec3);
    MK_REQUIRE(lowered.factors.metallic == 0.35F);
    MK_REQUIRE(lowered.factors.roughness == 0.55F);
    MK_REQUIRE(lowered.double_sided);
    MK_REQUIRE(lowered.texture_bindings.size() == 1);
    MK_REQUIRE(lowered.texture_bindings[0].slot == mirakana::MaterialTextureSlot::base_color);
    MK_REQUIRE(lowered.texture_bindings[0].texture == texture_id);

    auto bad_duplicate_socket = graph;
    bad_duplicate_socket.edges.push_back(mirakana::MaterialGraphEdge{
        .from_node = "bc_const", .from_socket = "out", .to_node = "out", .to_socket = "factor.base_color"});
    MK_REQUIRE(!mirakana::validate_material_graph(bad_duplicate_socket).empty());

    auto document =
        mirakana::editor::MaterialGraphAuthoringDocument::from_graph(graph, "assets/materials/graph.materialgraph");
    MK_REQUIRE(!document.dirty());
    MK_REQUIRE(mirakana::editor::material_graph_texture_dependencies(document).size() == 1);
    MK_REQUIRE(mirakana::editor::material_graph_texture_dependencies(document)[0] == texture_id);

    mirakana::editor::UndoStack history;
    auto renamed = document.graph();
    renamed.material_name = "Renamed";
    MK_REQUIRE(history.execute(mirakana::editor::make_material_graph_authoring_replace_action(document, renamed)));
    MK_REQUIRE(document.graph().material_name == "Renamed");
    MK_REQUIRE(document.dirty());
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.graph().material_name == "GraphMat");
    MK_REQUIRE(!document.dirty());

    mirakana::AssetRegistry registry;
    const auto missing_texture_id = mirakana::AssetId::from_name("textures/missing");
    auto missing_tex_doc = mirakana::editor::MaterialGraphAuthoringDocument::from_graph(graph);
    auto missing_tex_graph = missing_tex_doc.graph();
    missing_tex_graph.nodes.back().texture_id = missing_texture_id;
    MK_REQUIRE(missing_tex_doc.replace_graph(missing_tex_graph));
    const auto missing_diagnostics =
        mirakana::editor::validate_material_graph_authoring_document(missing_tex_doc, registry);
    MK_REQUIRE(!missing_diagnostics.empty());

    registry.add(mirakana::AssetRecord{
        .id = missing_texture_id, .kind = mirakana::AssetKind::texture, .path = "assets/textures/missing.texture"});
    MK_REQUIRE(mirakana::editor::validate_material_graph_authoring_document(missing_tex_doc, registry).empty());

    mirakana::editor::MemoryTextStore store;
    mirakana::editor::save_material_graph_authoring_document(store, "assets/materials/graph.materialgraph", document);
    const auto loaded =
        mirakana::editor::load_material_graph_authoring_document(store, "assets/materials/graph.materialgraph");
    MK_REQUIRE(!loaded.dirty());
    MK_REQUIRE(loaded.graph() == document.graph());
    MK_REQUIRE(store.read_text("assets/materials/graph.materialgraph") ==
               mirakana::serialize_material_graph(loaded.graph()));

    mirakana::MaterialGraphDesc invalid{};
    MK_REQUIRE(!document.replace_graph(invalid));
}

MK_TEST("editor selected material preview loads cooked artifact and resolves texture rows") {
    mirakana::MemoryFileSystem fs;
    mirakana::AssetRegistry registry;
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    registry.add(mirakana::AssetRecord{
        .id = material_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/player.material"});
    registry.add(mirakana::AssetRecord{
        .id = texture_id, .kind = mirakana::AssetKind::texture, .path = "assets/textures/player.texture"});

    fs.write_text("assets/materials/player.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "Player",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors =
                          mirakana::MaterialFactors{
                              .base_color = {0.3F, 0.4F, 0.5F, 1.0F},
                              .emissive = {0.7F, 0.8F, 0.9F},
                              .metallic = 0.2F,
                              .roughness = 0.6F,
                          },
                      .texture_bindings = {mirakana::MaterialTextureBinding{
                          .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_id}},
                      .double_sided = false,
                  }));

    const auto preview = mirakana::editor::make_editor_selected_material_preview(fs, registry, material_id);

    MK_REQUIRE(preview.status == mirakana::editor::EditorMaterialPreviewStatus::ready);
    MK_REQUIRE(preview.material == material_id);
    MK_REQUIRE(preview.artifact_path == "assets/materials/player.material");
    MK_REQUIRE(preview.name == "Player");
    MK_REQUIRE(preview.base_color[0] == 0.3F);
    MK_REQUIRE(preview.emissive[0] == 0.7F);
    MK_REQUIRE(preview.emissive[1] == 0.8F);
    MK_REQUIRE(preview.emissive[2] == 0.9F);
    MK_REQUIRE(preview.bindings.size() == 4);
    MK_REQUIRE(preview.texture_rows.size() == 1);
    MK_REQUIRE(preview.texture_rows[0].slot == mirakana::MaterialTextureSlot::base_color);
    MK_REQUIRE(preview.texture_rows[0].texture == texture_id);
    MK_REQUIRE(preview.texture_rows[0].artifact_path == "assets/textures/player.texture");
    MK_REQUIRE(preview.texture_rows[0].status == mirakana::editor::EditorMaterialPreviewTextureStatus::resolved);
    MK_REQUIRE(preview.diagnostic.empty());
}

MK_TEST("editor material gpu preview plan loads cooked texture payloads") {
    mirakana::MemoryFileSystem fs;
    mirakana::AssetRegistry registry;
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");

    fs.write_text("source/textures/player.texture",
                  mirakana::serialize_texture_source_document(mirakana::TextureSourceDocument{
                      .width = 2,
                      .height = 1,
                      .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
                      .bytes = std::vector<std::uint8_t>{16, 32, 48, 255, 64, 80, 96, 255},
                  }));
    fs.write_text("source/materials/player.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "Player",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors =
                          mirakana::MaterialFactors{
                              .base_color = {0.3F, 0.4F, 0.5F, 1.0F},
                              .emissive = {0.0F, 0.1F, 0.2F},
                              .metallic = 0.25F,
                              .roughness = 0.75F,
                          },
                      .texture_bindings = {mirakana::MaterialTextureBinding{
                          .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_id}},
                      .double_sided = false,
                  }));

    mirakana::AssetImportMetadataRegistry imports;
    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player.texture",
        .imported_path = "assets/textures/player.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/player.material",
        .imported_path = "assets/materials/player.material",
        .texture_dependencies = {texture_id},
    });

    const auto result = mirakana::execute_asset_import_plan(fs, mirakana::build_asset_import_plan(imports));
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(mirakana::editor::add_imported_asset_records(registry, result) == 2);

    const auto plan = mirakana::editor::make_editor_material_gpu_preview_plan(fs, registry, material_id);

    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.status == mirakana::editor::EditorMaterialGpuPreviewStatus::ready);
    MK_REQUIRE(plan.preview.material == material_id);
    MK_REQUIRE(plan.material.material.id == material_id);
    MK_REQUIRE(plan.material.binding_metadata.bindings.size() == 4);
    MK_REQUIRE(plan.textures.size() == 1);
    MK_REQUIRE(plan.textures[0].slot == mirakana::MaterialTextureSlot::base_color);
    MK_REQUIRE(plan.textures[0].payload.asset == texture_id);
    MK_REQUIRE(plan.textures[0].payload.width == 2);
    MK_REQUIRE(plan.textures[0].payload.height == 1);
    MK_REQUIRE(plan.textures[0].payload.bytes.size() == 8);
    MK_REQUIRE(plan.diagnostic.empty());
}

MK_TEST("editor material gpu preview plan reports invalid cooked texture payloads") {
    mirakana::MemoryFileSystem fs;
    mirakana::AssetRegistry registry;
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    registry.add(mirakana::AssetRecord{
        .id = material_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/player.material"});
    registry.add(mirakana::AssetRecord{
        .id = texture_id, .kind = mirakana::AssetKind::texture, .path = "assets/textures/player.texture"});

    fs.write_text("assets/materials/player.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "Player",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors = mirakana::MaterialFactors{},
                      .texture_bindings = {mirakana::MaterialTextureBinding{
                          .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_id}},
                      .double_sided = false,
                  }));
    fs.write_text("assets/textures/player.texture", "format=GameEngine.CookedTexture.v1\n");

    const auto plan = mirakana::editor::make_editor_material_gpu_preview_plan(fs, registry, material_id);

    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.status == mirakana::editor::EditorMaterialGpuPreviewStatus::invalid_texture_payload);
    MK_REQUIRE(plan.preview.status == mirakana::editor::EditorMaterialPreviewStatus::ready);
    MK_REQUIRE(plan.textures.size() == 1);
    MK_REQUIRE(plan.textures[0].texture == texture_id);
    MK_REQUIRE(plan.textures[0].diagnostic.contains("texture"));
    MK_REQUIRE(plan.diagnostic.contains("texture"));
}

MK_TEST("editor material asset preview panel summarizes payloads shaders and diagnostics") {
    mirakana::MemoryFileSystem fs;
    mirakana::AssetRegistry registry;
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");

    fs.write_text("source/textures/player.texture",
                  mirakana::serialize_texture_source_document(mirakana::TextureSourceDocument{
                      .width = 2,
                      .height = 1,
                      .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
                      .bytes = std::vector<std::uint8_t>{16, 32, 48, 255, 64, 80, 96, 255},
                  }));
    fs.write_text("source/materials/player.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "Player",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors =
                          mirakana::MaterialFactors{
                              .base_color = {0.3F, 0.4F, 0.5F, 1.0F},
                              .emissive = {0.0F, 0.1F, 0.2F},
                              .metallic = 0.25F,
                              .roughness = 0.75F,
                          },
                      .texture_bindings = {mirakana::MaterialTextureBinding{
                          .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_id}},
                      .double_sided = false,
                  }));

    mirakana::AssetImportMetadataRegistry imports;
    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player.texture",
        .imported_path = "assets/textures/player.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/player.material",
        .imported_path = "assets/materials/player.material",
        .texture_dependencies = {texture_id},
    });

    const auto result = mirakana::execute_asset_import_plan(fs, mirakana::build_asset_import_plan(imports));
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(mirakana::editor::add_imported_asset_records(registry, result) == 2);

    const auto shader_requests = mirakana::editor::make_material_preview_shader_compile_requests("out/editor/shaders");
    fs.write_text("out/editor/shaders/editor-material-preview-textured.vs.dxil", "textured vertex bytecode");
    fs.write_text("out/editor/shaders/editor-material-preview-textured.ps.dxil", "textured fragment bytecode");
    fs.write_text("out/editor/shaders/editor-material-preview-textured.vs.spv", "textured vertex spirv");
    fs.write_text("out/editor/shaders/editor-material-preview-textured.ps.spv", "textured fragment spirv");

    mirakana::editor::ViewportShaderArtifactState shader_artifacts;
    shader_artifacts.refresh_from(fs, shader_requests);

    const mirakana::editor::EditorMaterialAssetPreviewPanelModel model =
        mirakana::editor::make_editor_material_asset_preview_panel_model(fs, registry, material_id, shader_artifacts);

    MK_REQUIRE(model.status == mirakana::editor::EditorMaterialAssetPreviewPanelStatus::ready);
    MK_REQUIRE(model.status_label == "ready");
    MK_REQUIRE(model.material == material_id);
    MK_REQUIRE(model.material_path == "assets/materials/player.material");
    MK_REQUIRE(model.material_name == "Player");
    MK_REQUIRE(model.preview.status == mirakana::editor::EditorMaterialPreviewStatus::ready);
    MK_REQUIRE(model.preview.base_color[2] == 0.5F);
    MK_REQUIRE(model.preview.texture_rows.size() == 1);
    MK_REQUIRE(model.preview.texture_rows[0].status == mirakana::editor::EditorMaterialPreviewTextureStatus::resolved);
    MK_REQUIRE(model.gpu_status_label == "Ready");
    MK_REQUIRE(model.gpu_payload_ready);
    MK_REQUIRE(model.texture_payload_rows.size() == 1);
    MK_REQUIRE(model.texture_payload_rows[0].id == "base_color");
    MK_REQUIRE(model.texture_payload_rows[0].ready);
    MK_REQUIRE(model.texture_payload_rows[0].width == 2);
    MK_REQUIRE(model.texture_payload_rows[0].height == 1);
    MK_REQUIRE(model.texture_payload_rows[0].byte_count == 8);
    MK_REQUIRE(model.required_shader_row_id == "textured.d3d12");
    MK_REQUIRE(model.required_shader_ready);
    MK_REQUIRE(model.shader_rows.size() == 4);
    MK_REQUIRE(model.shader_rows[2].id == "textured.d3d12");
    MK_REQUIRE(model.shader_rows[2].ready);
    MK_REQUIRE(model.shader_rows[2].vertex_status_label == "Ready");
    MK_REQUIRE(model.shader_rows[3].id == "textured.vulkan");
    MK_REQUIRE(model.shader_rows[3].ready);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
}

MK_TEST("editor material asset preview panel retained ui exposes diagnostic sections") {
    mirakana::MemoryFileSystem fs;
    mirakana::AssetRegistry registry;
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    registry.add(mirakana::AssetRecord{
        .id = material_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/player.material"});
    fs.write_text("assets/materials/player.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "Player",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors =
                          mirakana::MaterialFactors{
                              .base_color = {0.2F, 0.3F, 0.4F, 1.0F},
                              .emissive = {0.0F, 0.0F, 0.0F},
                              .metallic = 0.1F,
                              .roughness = 0.9F,
                          },
                      .texture_bindings = {},
                      .double_sided = false,
                  }));

    const auto shader_requests = mirakana::editor::make_material_preview_shader_compile_requests("out/editor/shaders");
    fs.write_text("out/editor/shaders/editor-material-preview-factor.vs.dxil", "factor vertex bytecode");
    fs.write_text("out/editor/shaders/editor-material-preview-factor.ps.dxil", "factor fragment bytecode");
    mirakana::editor::ViewportShaderArtifactState shader_artifacts;
    shader_artifacts.refresh_from(fs, shader_requests);

    const mirakana::editor::EditorMaterialAssetPreviewPanelModel model =
        mirakana::editor::make_editor_material_asset_preview_panel_model(fs, registry, material_id, shader_artifacts);
    const auto ui = mirakana::editor::make_material_asset_preview_panel_ui_model(model);

    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"material_asset_preview"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"material_asset_preview.material.path"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"material_asset_preview.factors.base_color"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"material_asset_preview.factors.uniform_bytes"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"material_asset_preview.shaders.factor.d3d12.status"}) != nullptr);
}

MK_TEST("editor material asset preview panel reports host gpu preview execution snapshot") {
    mirakana::MemoryFileSystem fs;
    mirakana::AssetRegistry registry;
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    registry.add(mirakana::AssetRecord{
        .id = material_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/player.material"});
    fs.write_text("assets/materials/player.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "Player",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors =
                          mirakana::MaterialFactors{
                              .base_color = {0.2F, 0.3F, 0.4F, 1.0F},
                              .emissive = {0.0F, 0.0F, 0.0F},
                              .metallic = 0.1F,
                              .roughness = 0.9F,
                          },
                      .texture_bindings = {},
                      .double_sided = false,
                  }));

    const auto shader_requests = mirakana::editor::make_material_preview_shader_compile_requests("out/editor/shaders");
    fs.write_text("out/editor/shaders/editor-material-preview-factor.vs.dxil", "factor vertex bytecode");
    fs.write_text("out/editor/shaders/editor-material-preview-factor.ps.dxil", "factor fragment bytecode");
    mirakana::editor::ViewportShaderArtifactState shader_artifacts;
    shader_artifacts.refresh_from(fs, shader_requests);

    mirakana::editor::EditorMaterialAssetPreviewPanelModel model =
        mirakana::editor::make_editor_material_asset_preview_panel_model(fs, registry, material_id, shader_artifacts);
    MK_REQUIRE(model.gpu_execution_host_owned);
    MK_REQUIRE(!model.gpu_execution_ready);
    MK_REQUIRE(!model.gpu_execution_rendered);
    MK_REQUIRE(model.gpu_execution_status_label == "Host Required");
    MK_REQUIRE(!model.executes);

    const auto default_ui = mirakana::editor::make_material_asset_preview_panel_ui_model(model);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.status"}) != nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.backend"}) != nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.frames"}) != nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.display_path"}) !=
               nullptr);
    MK_REQUIRE(default_ui.find(
                   mirakana::ui::ElementId{"material_asset_preview.gpu.execution.vulkan_visible_refresh"}) != nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.metal_visible_refresh"}) !=
               nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.parity_checklist"}) !=
               nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{
                   "material_asset_preview.gpu.execution.parity_checklist.contract"}) != nullptr);
    const auto* default_backend_scope = default_ui.find(
        mirakana::ui::ElementId{"material_asset_preview.gpu.execution.parity_checklist.rows.backend_scope.status"});
    MK_REQUIRE(default_backend_scope != nullptr);
    MK_REQUIRE(default_backend_scope->text.label == "pending");
    const auto* default_vulkan_gate = default_ui.find(mirakana::ui::ElementId{
        "material_asset_preview.gpu.execution.parity_checklist.rows.vulkan_visible_refresh_gate.status"});
    MK_REQUIRE(default_vulkan_gate != nullptr);
    MK_REQUIRE(default_vulkan_gate->text.label == "not-applicable");
    MK_REQUIRE(model.gpu_display_parity_checklist_rows.size() == 6);
    const auto* default_contract =
        default_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.contract"});
    MK_REQUIRE(default_contract != nullptr);
    MK_REQUIRE(default_contract->text.label == "ge.editor.material_gpu_preview_execution.v1");

    mirakana::editor::EditorMaterialGpuPreviewExecutionSnapshot ready_snapshot;
    ready_snapshot.status = mirakana::editor::EditorMaterialGpuPreviewStatus::ready;
    ready_snapshot.backend_label = "D3D12";
    ready_snapshot.display_path_label = "cpu-readback";
    ready_snapshot.frames_rendered = 1;
    mirakana::editor::apply_editor_material_gpu_preview_execution_snapshot(model, ready_snapshot);

    MK_REQUIRE(model.gpu_execution_vulkan_visible_refresh_evidence == "not-applicable");
    MK_REQUIRE(model.gpu_execution_metal_visible_refresh_evidence == "not-applicable");
    MK_REQUIRE(model.gpu_execution_status_label == "Ready");
    MK_REQUIRE(model.gpu_execution_backend_label == "D3D12");
    MK_REQUIRE(model.gpu_execution_display_path_label == "cpu-readback");
    MK_REQUIRE(model.gpu_execution_frames_rendered == 1);
    MK_REQUIRE(model.gpu_execution_ready);
    MK_REQUIRE(model.gpu_execution_rendered);
    MK_REQUIRE(!model.executes);

    const auto ready_ui = mirakana::editor::make_material_asset_preview_panel_ui_model(model);
    MK_REQUIRE(ready_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.diagnostic"}) != nullptr);
    const auto* ready_vulkan_refresh =
        ready_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.vulkan_visible_refresh"});
    MK_REQUIRE(ready_vulkan_refresh != nullptr);
    MK_REQUIRE(ready_vulkan_refresh->text.label == "not-applicable");
    const auto* ready_metal_refresh =
        ready_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.metal_visible_refresh"});
    MK_REQUIRE(ready_metal_refresh != nullptr);
    MK_REQUIRE(ready_metal_refresh->text.label == "not-applicable");
    const auto* ready_contract =
        ready_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.contract"});
    MK_REQUIRE(ready_contract != nullptr);
    MK_REQUIRE(ready_contract->text.label == "ge.editor.material_gpu_preview_execution.v1");
    const auto* ready_parity_contract =
        ready_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.parity_checklist.contract"});
    MK_REQUIRE(ready_parity_contract != nullptr);
    MK_REQUIRE(ready_parity_contract->text.label == "ge.editor.material_gpu_preview_display_parity_checklist.v1");
    const auto* ready_backend_scope = ready_ui.find(
        mirakana::ui::ElementId{"material_asset_preview.gpu.execution.parity_checklist.rows.backend_scope.status"});
    MK_REQUIRE(ready_backend_scope != nullptr);
    MK_REQUIRE(ready_backend_scope->text.label == "complete");
    const auto* ready_display_scope = ready_ui.find(mirakana::ui::ElementId{
        "material_asset_preview.gpu.execution.parity_checklist.rows.display_path_scope.status"});
    MK_REQUIRE(ready_display_scope != nullptr);
    MK_REQUIRE(ready_display_scope->text.label == "complete");

    mirakana::editor::EditorMaterialGpuPreviewExecutionSnapshot vulkan_snapshot;
    vulkan_snapshot.status = mirakana::editor::EditorMaterialGpuPreviewStatus::ready;
    vulkan_snapshot.backend_label = "Vulkan";
    vulkan_snapshot.display_path_label = "cpu-readback";
    vulkan_snapshot.frames_rendered = 3;
    mirakana::editor::apply_editor_material_gpu_preview_execution_snapshot(model, vulkan_snapshot);

    MK_REQUIRE(model.gpu_execution_vulkan_visible_refresh_evidence == "complete");
    MK_REQUIRE(model.gpu_execution_metal_visible_refresh_evidence == "not-applicable");
    MK_REQUIRE(model.gpu_execution_backend_label == "Vulkan");
    MK_REQUIRE(model.gpu_execution_display_path_label == "cpu-readback");
    MK_REQUIRE(model.gpu_execution_frames_rendered == 3);
    MK_REQUIRE(model.gpu_execution_ready);
    const auto vulkan_ui = mirakana::editor::make_material_asset_preview_panel_ui_model(model);
    const auto* vulkan_contract =
        vulkan_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.contract"});
    MK_REQUIRE(vulkan_contract != nullptr);
    MK_REQUIRE(vulkan_contract->text.label == "ge.editor.material_gpu_preview_execution.v1");
    const auto* vulkan_refresh =
        vulkan_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.vulkan_visible_refresh"});
    MK_REQUIRE(vulkan_refresh != nullptr);
    MK_REQUIRE(vulkan_refresh->text.label == "complete");
    const auto* vulkan_metal_row =
        vulkan_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.metal_visible_refresh"});
    MK_REQUIRE(vulkan_metal_row != nullptr);
    MK_REQUIRE(vulkan_metal_row->text.label == "not-applicable");

    mirakana::editor::EditorMaterialGpuPreviewExecutionSnapshot vulkan_pending_frames;
    vulkan_pending_frames.status = mirakana::editor::EditorMaterialGpuPreviewStatus::ready;
    vulkan_pending_frames.backend_label = "vulkan";
    vulkan_pending_frames.display_path_label = "cpu-readback";
    vulkan_pending_frames.frames_rendered = 0;
    mirakana::editor::apply_editor_material_gpu_preview_execution_snapshot(model, vulkan_pending_frames);
    MK_REQUIRE(model.gpu_execution_vulkan_visible_refresh_evidence == "pending");

    mirakana::editor::EditorMaterialGpuPreviewExecutionSnapshot metal_snapshot;
    metal_snapshot.status = mirakana::editor::EditorMaterialGpuPreviewStatus::ready;
    metal_snapshot.backend_label = "Metal";
    metal_snapshot.display_path_label = "cpu-readback";
    metal_snapshot.frames_rendered = 2;
    mirakana::editor::apply_editor_material_gpu_preview_execution_snapshot(model, metal_snapshot);
    MK_REQUIRE(model.gpu_execution_vulkan_visible_refresh_evidence == "not-applicable");
    MK_REQUIRE(model.gpu_execution_metal_visible_refresh_evidence == "complete");
    const auto metal_ui = mirakana::editor::make_material_asset_preview_panel_ui_model(model);
    const auto* metal_refresh =
        metal_ui.find(mirakana::ui::ElementId{"material_asset_preview.gpu.execution.metal_visible_refresh"});
    MK_REQUIRE(metal_refresh != nullptr);
    MK_REQUIRE(metal_refresh->text.label == "complete");

    mirakana::editor::EditorMaterialGpuPreviewExecutionSnapshot metal_pending_frames;
    metal_pending_frames.status = mirakana::editor::EditorMaterialGpuPreviewStatus::ready;
    metal_pending_frames.backend_label = "metal";
    metal_pending_frames.display_path_label = "cpu-readback";
    metal_pending_frames.frames_rendered = 0;
    mirakana::editor::apply_editor_material_gpu_preview_execution_snapshot(model, metal_pending_frames);
    MK_REQUIRE(model.gpu_execution_metal_visible_refresh_evidence == "pending");

    mirakana::editor::EditorMaterialGpuPreviewExecutionSnapshot failed_snapshot;
    failed_snapshot.status = mirakana::editor::EditorMaterialGpuPreviewStatus::render_failed;
    failed_snapshot.backend_label = "D3D12";
    failed_snapshot.display_path_label = "cpu-readback";
    failed_snapshot.frames_rendered = 0;
    failed_snapshot.diagnostic = "surface readback failed";
    mirakana::editor::apply_editor_material_gpu_preview_execution_snapshot(model, failed_snapshot);

    MK_REQUIRE(model.gpu_execution_status_label == "Render Failed");
    MK_REQUIRE(model.gpu_execution_diagnostic == "surface readback failed");
    MK_REQUIRE(!model.gpu_execution_ready);
    MK_REQUIRE(!model.gpu_execution_rendered);
}

MK_TEST("editor selected material preview reports missing artifact without throwing") {
    mirakana::MemoryFileSystem fs;
    mirakana::AssetRegistry registry;
    const auto material_id = mirakana::AssetId::from_name("materials/missing");
    registry.add(mirakana::AssetRecord{
        .id = material_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/missing.material"});

    const auto preview = mirakana::editor::make_editor_selected_material_preview(fs, registry, material_id);

    MK_REQUIRE(preview.status == mirakana::editor::EditorMaterialPreviewStatus::missing_artifact);
    MK_REQUIRE(preview.material == material_id);
    MK_REQUIRE(preview.artifact_path == "assets/materials/missing.material");
    MK_REQUIRE(preview.diagnostic.contains("missing"));
}

MK_TEST("editor selected material preview reports malformed material diagnostic without disappearing") {
    mirakana::MemoryFileSystem fs;
    mirakana::AssetRegistry registry;
    const auto material_id = mirakana::AssetId::from_name("materials/broken");
    registry.add(mirakana::AssetRecord{
        .id = material_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/broken.material"});
    fs.write_text("assets/materials/broken.material", "format=GameEngine.Material.v1\nmaterial.name=Broken\n");

    const auto preview = mirakana::editor::make_editor_selected_material_preview(fs, registry, material_id);

    MK_REQUIRE(preview.status == mirakana::editor::EditorMaterialPreviewStatus::invalid_material);
    MK_REQUIRE(preview.material == material_id);
    MK_REQUIRE(preview.artifact_path == "assets/materials/broken.material");
    MK_REQUIRE(preview.diagnostic.contains("material"));
}

MK_TEST("editor material preview keeps binding metadata while flagging missing texture dependency") {
    mirakana::MemoryFileSystem fs;
    mirakana::AssetRegistry registry;
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto texture_id = mirakana::AssetId::from_name("textures/missing");
    registry.add(mirakana::AssetRecord{
        .id = material_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/player.material"});
    fs.write_text("assets/materials/player.material",
                  mirakana::serialize_material_definition(mirakana::MaterialDefinition{
                      .id = material_id,
                      .name = "Player",
                      .shading_model = mirakana::MaterialShadingModel::lit,
                      .surface_mode = mirakana::MaterialSurfaceMode::opaque,
                      .factors = mirakana::MaterialFactors{},
                      .texture_bindings = {mirakana::MaterialTextureBinding{
                          .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture_id}},
                      .double_sided = false,
                  }));

    const auto preview = mirakana::editor::make_editor_selected_material_preview(fs, registry, material_id);

    MK_REQUIRE(preview.status == mirakana::editor::EditorMaterialPreviewStatus::warning);
    MK_REQUIRE(preview.bindings.size() == 4);
    MK_REQUIRE(preview.texture_rows.size() == 1);
    MK_REQUIRE(preview.texture_rows[0].status == mirakana::editor::EditorMaterialPreviewTextureStatus::missing);
    MK_REQUIRE(preview.texture_rows[0].texture == texture_id);
    MK_REQUIRE(preview.diagnostic.contains("texture"));
}

MK_TEST("editor asset pipeline panel model aggregates progress diagnostics thumbnails dependencies and materials") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto texture_id = mirakana::AssetId::from_name("textures/player");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto audio_id = mirakana::AssetId::from_name("audio/hit");

    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player.texture-source",
        .imported_path = "assets/textures/player.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/player.material",
        .imported_path = "assets/materials/player.material",
        .texture_dependencies = {texture_id},
    });
    imports.add_audio(mirakana::AudioImportMetadata{
        .id = audio_id,
        .source_path = "source/audio/hit.audio-source",
        .imported_path = "assets/audio/hit.audio",
        .streaming = false,
    });

    const auto plan = mirakana::build_asset_import_plan(imports);
    mirakana::editor::AssetPipelineState pipeline;
    pipeline.set_import_plan(plan);
    pipeline.apply_import_updates({
        mirakana::editor::EditorAssetImportUpdate{.asset = material_id, .imported = true, .diagnostic = {}},
        mirakana::editor::EditorAssetImportUpdate{
            .asset = texture_id, .imported = false, .diagnostic = "decode failed"},
    });

    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::masked,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_id}},
        .double_sided = false,
    };

    const auto model = mirakana::editor::make_editor_asset_pipeline_panel_model(pipeline, plan, {material});

    MK_REQUIRE(model.progress.total_count == 3);
    MK_REQUIRE(model.progress.completed_count == 2);
    MK_REQUIRE(model.dependencies.size() == 1);
    MK_REQUIRE(model.thumbnail_requests.size() == 3);
    MK_REQUIRE(model.material_previews.size() == 1);
    MK_REQUIRE(model.material_previews[0].requires_alpha_test);
    MK_REQUIRE(model.diagnostics.size() == 1);
    MK_REQUIRE(model.diagnostics[0].asset == texture_id);
    MK_REQUIRE(model.diagnostics[0].output_path == "assets/textures/player.texture");
    MK_REQUIRE(model.diagnostics[0].diagnostic == "decode failed");
}

MK_TEST("editor content browser import panel model summarizes assets imports and diagnostics") {
    mirakana::AssetRegistry registry;
    const auto texture_id = mirakana::AssetId::from_name("textures/player");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto audio_id = mirakana::AssetId::from_name("audio/hit");

    registry.add(mirakana::AssetRecord{
        .id = texture_id, .kind = mirakana::AssetKind::texture, .path = "assets/textures/player.texture"});
    registry.add(mirakana::AssetRecord{
        .id = material_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/player.material"});
    registry.add(
        mirakana::AssetRecord{.id = audio_id, .kind = mirakana::AssetKind::audio, .path = "assets/audio/hit.audio"});

    mirakana::AssetIdentityDocumentV2 identity;
    identity.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{"textures/player"},
        .kind = mirakana::AssetKind::texture,
        .source_path = "source/textures/player.texture-source",
    });
    identity.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{"materials/player"},
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/player.material",
    });

    mirakana::editor::ContentBrowserState browser;
    browser.refresh_from(registry, identity);
    browser.set_text_filter("player");
    MK_REQUIRE(browser.select(material_id));

    mirakana::AssetImportMetadataRegistry imports;
    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player.texture-source",
        .imported_path = "assets/textures/player.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/player.material",
        .imported_path = "assets/materials/player.material",
        .texture_dependencies = {texture_id},
    });
    imports.add_audio(mirakana::AudioImportMetadata{
        .id = audio_id,
        .source_path = "source/audio/hit.audio-source",
        .imported_path = "assets/audio/hit.audio",
        .streaming = false,
    });

    const auto plan = mirakana::build_asset_import_plan(imports);
    mirakana::editor::AssetPipelineState pipeline;
    pipeline.set_import_plan(plan);
    pipeline.apply_import_updates({
        mirakana::editor::EditorAssetImportUpdate{.asset = material_id, .imported = true, .diagnostic = {}},
        mirakana::editor::EditorAssetImportUpdate{
            .asset = texture_id, .imported = false, .diagnostic = "decode failed"},
    });
    pipeline.apply_hot_reload_events({
        mirakana::AssetHotReloadEvent{
            .kind = mirakana::AssetHotReloadEventKind::modified,
            .asset = texture_id,
            .path = "assets/textures/player.texture",
            .previous_revision = 1,
            .current_revision = 2,
            .previous_size_bytes = 100,
            .current_size_bytes = 120,
        },
    });
    pipeline.apply_recook_requests({
        mirakana::AssetHotReloadRecookRequest{
            .asset = texture_id,
            .source_asset = texture_id,
            .trigger_path = "assets/textures/player.texture",
            .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
            .reason = mirakana::AssetHotReloadRecookReason::source_modified,
            .previous_revision = 1,
            .current_revision = 2,
            .ready_tick = 8,
        },
    });
    pipeline.apply_hot_reload_results({
        mirakana::AssetHotReloadApplyResult{
            .kind = mirakana::AssetHotReloadApplyResultKind::failed_rolled_back,
            .asset = texture_id,
            .path = "assets/textures/player.texture",
            .requested_revision = 2,
            .active_revision = 1,
            .diagnostic = "recook failed",
        },
    });

    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::masked,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_id}},
        .double_sided = false,
    };

    const mirakana::editor::EditorContentBrowserImportPanelModel model =
        mirakana::editor::make_editor_content_browser_import_panel_model(browser, pipeline, plan, {material});

    MK_REQUIRE(model.status == mirakana::editor::EditorContentBrowserImportPanelStatus::attention);
    MK_REQUIRE(model.status_label == "attention");
    MK_REQUIRE(model.total_asset_count == 3);
    MK_REQUIRE(model.visible_asset_count == 2);
    MK_REQUIRE(model.assets.size() == 2);
    MK_REQUIRE(model.assets[0].asset == material_id);
    MK_REQUIRE(model.assets[0].kind_label == "Material");
    MK_REQUIRE(model.assets[0].asset_key_label == "materials/player");
    MK_REQUIRE(model.assets[0].identity_source_path == "source/materials/player.material");
    MK_REQUIRE(model.assets[0].identity_backed);
    MK_REQUIRE(model.assets[0].selected);
    MK_REQUIRE(model.has_selected_asset);
    MK_REQUIRE(model.selected_asset.asset == material_id);
    MK_REQUIRE(model.selected_asset.path == "assets/materials/player.material");
    MK_REQUIRE(model.selected_asset.asset_key_label == "materials/player");
    MK_REQUIRE(model.selected_asset.identity_source_path == "source/materials/player.material");
    MK_REQUIRE(model.import_queue.size() == 3);
    MK_REQUIRE(model.import_queue[2].asset == texture_id);
    MK_REQUIRE(model.import_queue[2].status_label == "Failed");
    MK_REQUIRE(model.import_queue[2].failed);
    MK_REQUIRE(model.pipeline.progress.failed_count == 1);
    MK_REQUIRE(model.pipeline.diagnostics.size() == 1);
    MK_REQUIRE(model.pipeline.dependencies.size() == 1);
    MK_REQUIRE(model.pipeline.thumbnail_requests.size() == 3);
    MK_REQUIRE(model.pipeline.material_previews.size() == 1);
    MK_REQUIRE(model.pipeline.material_previews[0].requires_alpha_test);
    MK_REQUIRE(model.hot_reload_summary_rows.size() == 4);
    MK_REQUIRE(model.hot_reload_summary_rows[0].id == "events");
    MK_REQUIRE(model.hot_reload_summary_rows[0].count == 1);
    MK_REQUIRE(model.hot_reload_summary_rows[3].id == "failed");
    MK_REQUIRE(model.hot_reload_summary_rows[3].attention);
    MK_REQUIRE(model.has_import_failures);
    MK_REQUIRE(model.has_hot_reload_failures);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
}

MK_TEST("editor content browser import panel retained ui exposes all diagnostic sections") {
    mirakana::AssetRegistry registry;
    const auto texture_id = mirakana::AssetId::from_name("textures/player");
    const auto material_id = mirakana::AssetId::from_name("materials/player");

    registry.add(mirakana::AssetRecord{
        .id = texture_id, .kind = mirakana::AssetKind::texture, .path = "assets/textures/player.texture"});
    registry.add(mirakana::AssetRecord{
        .id = material_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/player.material"});

    mirakana::AssetIdentityDocumentV2 identity;
    identity.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{"textures/player"},
        .kind = mirakana::AssetKind::texture,
        .source_path = "source/textures/player.texture-source",
    });
    identity.assets.push_back(mirakana::AssetIdentityRowV2{
        .key = mirakana::AssetKeyV2{"materials/player"},
        .kind = mirakana::AssetKind::material,
        .source_path = "source/materials/player.material",
    });

    mirakana::editor::ContentBrowserState browser;
    browser.refresh_from(registry, identity);
    MK_REQUIRE(browser.select(material_id));

    mirakana::AssetImportMetadataRegistry imports;
    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player.texture-source",
        .imported_path = "assets/textures/player.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::none,
    });
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/player.material",
        .imported_path = "assets/materials/player.material",
        .texture_dependencies = {texture_id},
    });

    const auto plan = mirakana::build_asset_import_plan(imports);
    mirakana::editor::AssetPipelineState pipeline;
    pipeline.set_import_plan(plan);
    pipeline.apply_import_updates({
        mirakana::editor::EditorAssetImportUpdate{.asset = material_id, .imported = true, .diagnostic = {}},
        mirakana::editor::EditorAssetImportUpdate{
            .asset = texture_id, .imported = false, .diagnostic = "decode failed"},
    });

    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_id}},
        .double_sided = false,
    };

    const auto model =
        mirakana::editor::make_editor_content_browser_import_panel_model(browser, pipeline, plan, {material});
    const auto ui = mirakana::editor::make_content_browser_import_panel_ui_model(model);

    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.assets." + std::to_string(material_id.value) +
                                               ".path"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.assets." + std::to_string(material_id.value) +
                                               ".asset_key"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.assets." + std::to_string(material_id.value) +
                                               ".identity_source_path"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.selection.path"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.selection.asset_key"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.selection.identity_source_path"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.imports." + std::to_string(texture_id.value) +
                                               ".status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.diagnostics." +
                                               std::to_string(texture_id.value) + ".message"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.dependencies.1.path"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.thumbnails.1.path"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.materials." + std::to_string(material_id.value) +
                                               ".status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"content_browser_import.hot_reload.events.count"}) != nullptr);
}

MK_TEST("editor content browser import native file dialog reviews source selections") {
    const auto request = mirakana::editor::make_content_browser_import_open_dialog_request("assets/source");
    MK_REQUIRE(request.kind == mirakana::FileDialogKind::open_file);
    MK_REQUIRE(request.title == "Import Assets");
    MK_REQUIRE(request.default_location == "assets/source");
    MK_REQUIRE(request.allow_many);
    MK_REQUIRE(request.accept_label == "Import");
    MK_REQUIRE(request.cancel_label == "Cancel");
    MK_REQUIRE(request.filters.size() >= 5U);
    MK_REQUIRE(request.filters[0].name == "Texture Source");
    MK_REQUIRE(request.filters[0].pattern == "texture");
    MK_REQUIRE(request.filters[1].pattern == "mesh");
    MK_REQUIRE(request.filters[2].pattern == "material");
    MK_REQUIRE(request.filters[3].pattern == "scene");
    MK_REQUIRE(request.filters[4].pattern == "audio_source");

    const auto accepted = mirakana::editor::make_content_browser_import_open_dialog_model(mirakana::FileDialogResult{
        .id = 7,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"assets/source/hero.texture", "assets/source/level.scene"},
        .selected_filter = 3,
        .error = {},
    });

    auto find_row = [](const mirakana::editor::EditorContentBrowserImportOpenDialogModel& model,
                       std::string_view id) -> const mirakana::editor::EditorContentBrowserImportOpenDialogRow* {
        const auto it = std::ranges::find_if(
            model.rows,
            [id](const mirakana::editor::EditorContentBrowserImportOpenDialogRow& row) { return row.id == id; });
        return it == model.rows.end() ? nullptr : std::addressof(*it);
    };

    MK_REQUIRE(accepted.accepted);
    MK_REQUIRE(accepted.status_label == "Asset import open dialog accepted");
    MK_REQUIRE(accepted.selected_paths.size() == 2U);
    MK_REQUIRE(accepted.selected_paths[0] == "assets/source/hero.texture");
    MK_REQUIRE(accepted.selected_paths[1] == "assets/source/level.scene");
    MK_REQUIRE(find_row(accepted, "status")->value == "Asset import open dialog accepted");
    MK_REQUIRE(find_row(accepted, "selected_count")->value == "2");
    MK_REQUIRE(find_row(accepted, "selected_filter")->value == "3");
    MK_REQUIRE(find_row(accepted, "path.1")->value == "assets/source/hero.texture");
    MK_REQUIRE(find_row(accepted, "path.2")->value == "assets/source/level.scene");

    const auto document = mirakana::editor::make_content_browser_import_open_dialog_ui_model(accepted);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"content_browser_import.open_dialog.status"})->text.label ==
               "Asset import open dialog accepted");
    MK_REQUIRE(
        document.find(mirakana::ui::ElementId{"content_browser_import.open_dialog.selected_count"})->text.label == "2");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"content_browser_import.open_dialog.paths.1"})->text.label ==
               "assets/source/hero.texture");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"content_browser_import.open_dialog.paths.2"})->text.label ==
               "assets/source/level.scene");

    const auto canceled = mirakana::editor::make_content_browser_import_open_dialog_model(mirakana::FileDialogResult{
        .id = 8,
        .status = mirakana::FileDialogStatus::canceled,
        .paths = {},
        .selected_filter = -1,
        .error = {},
    });
    MK_REQUIRE(!canceled.accepted);
    MK_REQUIRE(canceled.status_label == "Asset import open dialog canceled");
    MK_REQUIRE(canceled.diagnostics.empty());

    const auto failed = mirakana::editor::make_content_browser_import_open_dialog_model(mirakana::FileDialogResult{
        .id = 9,
        .status = mirakana::FileDialogStatus::failed,
        .paths = {},
        .selected_filter = -1,
        .error = "native import dialog failed",
    });
    MK_REQUIRE(!failed.accepted);
    MK_REQUIRE(failed.status_label == "Asset import open dialog failed");
    MK_REQUIRE(failed.diagnostics.size() == 1U);
    MK_REQUIRE(failed.diagnostics[0] == "native import dialog failed");

    const auto empty_accepted =
        mirakana::editor::make_content_browser_import_open_dialog_model(mirakana::FileDialogResult{
            .id = 10,
            .status = mirakana::FileDialogStatus::accepted,
            .paths = {},
            .selected_filter = 0,
            .error = {},
        });
    MK_REQUIRE(!empty_accepted.accepted);
    MK_REQUIRE(empty_accepted.status_label == "Asset import open dialog blocked");
    MK_REQUIRE(!empty_accepted.diagnostics.empty());
    MK_REQUIRE(empty_accepted.diagnostics[0].contains("accepted file dialog results"));

    const auto wrong_extension =
        mirakana::editor::make_content_browser_import_open_dialog_model(mirakana::FileDialogResult{
            .id = 11,
            .status = mirakana::FileDialogStatus::accepted,
            .paths = {"assets/source/hero.obj"},
            .selected_filter = 0,
            .error = {},
        });
    MK_REQUIRE(!wrong_extension.accepted);
    MK_REQUIRE(wrong_extension.status_label == "Asset import open dialog blocked");
    MK_REQUIRE(!wrong_extension.diagnostics.empty());
    MK_REQUIRE(wrong_extension.diagnostics[0].contains("first-party source document"));
}

MK_TEST("editor content browser import codec adapter review accepts supported source formats") {
    const auto request = mirakana::editor::make_content_browser_import_open_dialog_request("assets/source");
    MK_REQUIRE(request.filters.size() == 8U);
    MK_REQUIRE(request.filters[5].name == "PNG Texture Source");
    MK_REQUIRE(request.filters[5].pattern == "png");
    MK_REQUIRE(request.filters[6].name == "glTF Mesh Source");
    MK_REQUIRE(request.filters[6].pattern == "gltf;glb");
    MK_REQUIRE(request.filters[7].name == "Common Audio Source");
    MK_REQUIRE(request.filters[7].pattern == "wav;mp3;flac");

    const auto accepted = mirakana::editor::make_content_browser_import_open_dialog_model(mirakana::FileDialogResult{
        .id = 12,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"assets/source/hero.png", "assets/source/ship.gltf", "assets/source/hit.wav"},
        .selected_filter = 5,
        .error = {},
    });
    MK_REQUIRE(accepted.accepted);
    MK_REQUIRE(accepted.status_label == "Asset import open dialog accepted");
    MK_REQUIRE(accepted.selected_paths.size() == 3U);
    MK_REQUIRE(accepted.selected_paths[0] == "assets/source/hero.png");
    MK_REQUIRE(accepted.selected_paths[1] == "assets/source/ship.gltf");
    MK_REQUIRE(accepted.selected_paths[2] == "assets/source/hit.wav");

    const auto document = mirakana::editor::make_content_browser_import_open_dialog_ui_model(accepted);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"content_browser_import.open_dialog.paths.1"})->text.label ==
               "assets/source/hero.png");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"content_browser_import.open_dialog.paths.2"})->text.label ==
               "assets/source/ship.gltf");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"content_browser_import.open_dialog.paths.3"})->text.label ==
               "assets/source/hit.wav");

    const auto blocked_ogg = mirakana::editor::make_content_browser_import_open_dialog_model(mirakana::FileDialogResult{
        .id = 13,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"assets/source/hit.ogg"},
        .selected_filter = 7,
        .error = {},
    });
    MK_REQUIRE(!blocked_ogg.accepted);
    MK_REQUIRE(blocked_ogg.status_label == "Asset import open dialog blocked");
    MK_REQUIRE(!blocked_ogg.diagnostics.empty());

    mirakana::editor::EditorContentBrowserImportExternalSourceCopyInput png_input;
    png_input.source_path = "C:/drop/hero.png";
    png_input.target_project_path = "assets/imported_sources/hero.png";
    png_input.source_exists = true;

    const auto png_ready = mirakana::editor::make_content_browser_import_external_source_copy_model({png_input});
    MK_REQUIRE(png_ready.status == mirakana::editor::EditorContentBrowserImportExternalSourceCopyStatus::ready);
    MK_REQUIRE(png_ready.can_copy);
    MK_REQUIRE(png_ready.target_project_paths.size() == 1U);
    MK_REQUIRE(png_ready.target_project_paths[0] == "assets/imported_sources/hero.png");
}

MK_TEST("editor content browser import external source copy review keeps copying explicit") {
    mirakana::editor::EditorContentBrowserImportExternalSourceCopyInput ready_input;
    ready_input.source_path = "C:/drop/hero.texture";
    ready_input.target_project_path = "assets/imported_sources/hero.texture";
    ready_input.source_exists = true;

    const auto ready = mirakana::editor::make_content_browser_import_external_source_copy_model({ready_input});
    MK_REQUIRE(ready.status == mirakana::editor::EditorContentBrowserImportExternalSourceCopyStatus::ready);
    MK_REQUIRE(ready.status_label == "External import source copy ready");
    MK_REQUIRE(ready.can_copy);
    MK_REQUIRE(!ready.copied);
    MK_REQUIRE(!ready.blocked);
    MK_REQUIRE(ready.copy_count == 1U);
    MK_REQUIRE(ready.target_project_paths.size() == 1U);
    MK_REQUIRE(ready.target_project_paths[0] == "assets/imported_sources/hero.texture");
    MK_REQUIRE(ready.rows.size() == 1U);
    MK_REQUIRE(ready.rows[0].id == "1");
    MK_REQUIRE(ready.rows[0].status_label == "Ready to copy");
    MK_REQUIRE(ready.rows[0].source_path == "C:/drop/hero.texture");
    MK_REQUIRE(ready.rows[0].target_project_path == "assets/imported_sources/hero.texture");

    const auto ready_document = mirakana::editor::make_content_browser_import_external_source_copy_ui_model(ready);
    MK_REQUIRE(
        ready_document.find(mirakana::ui::ElementId{"content_browser_import.external_copy.status"})->text.label ==
        "External import source copy ready");
    MK_REQUIRE(
        ready_document.find(mirakana::ui::ElementId{"content_browser_import.external_copy.copy_count"})->text.label ==
        "1");
    MK_REQUIRE(ready_document.find(mirakana::ui::ElementId{"content_browser_import.external_copy.rows.1.source"})
                   ->text.label == "C:/drop/hero.texture");
    MK_REQUIRE(ready_document.find(mirakana::ui::ElementId{"content_browser_import.external_copy.rows.1.target"})
                   ->text.label == "assets/imported_sources/hero.texture");

    auto blocked_extension = ready_input;
    blocked_extension.source_path = "C:/drop/hero.ogg";
    blocked_extension.target_project_path = "assets/imported_sources/hero.ogg";
    const auto blocked_by_extension =
        mirakana::editor::make_content_browser_import_external_source_copy_model({blocked_extension});
    MK_REQUIRE(blocked_by_extension.status ==
               mirakana::editor::EditorContentBrowserImportExternalSourceCopyStatus::blocked);
    MK_REQUIRE(!blocked_by_extension.can_copy);
    MK_REQUIRE(!blocked_by_extension.diagnostics.empty());
    MK_REQUIRE(blocked_by_extension.diagnostics[0].contains("first-party source document"));

    auto unsafe_target = ready_input;
    unsafe_target.target_project_path = "../hero.texture";
    const auto blocked_by_target =
        mirakana::editor::make_content_browser_import_external_source_copy_model({unsafe_target});
    MK_REQUIRE(blocked_by_target.status ==
               mirakana::editor::EditorContentBrowserImportExternalSourceCopyStatus::blocked);
    MK_REQUIRE(!blocked_by_target.can_copy);
    MK_REQUIRE(!blocked_by_target.diagnostics.empty());
    MK_REQUIRE(blocked_by_target.diagnostics[0].contains("safe project-relative path"));

    auto existing_target = ready_input;
    existing_target.target_exists = true;
    const auto blocked_by_existing =
        mirakana::editor::make_content_browser_import_external_source_copy_model({existing_target});
    MK_REQUIRE(blocked_by_existing.status ==
               mirakana::editor::EditorContentBrowserImportExternalSourceCopyStatus::blocked);
    MK_REQUIRE(!blocked_by_existing.can_copy);
    MK_REQUIRE(!blocked_by_existing.diagnostics.empty());
    MK_REQUIRE(blocked_by_existing.diagnostics[0].contains("already exists"));

    auto empty_source = ready_input;
    empty_source.source_path.clear();
    const auto blocked_by_empty =
        mirakana::editor::make_content_browser_import_external_source_copy_model({empty_source});
    MK_REQUIRE(blocked_by_empty.status ==
               mirakana::editor::EditorContentBrowserImportExternalSourceCopyStatus::blocked);
    MK_REQUIRE(!blocked_by_empty.can_copy);
    MK_REQUIRE(!blocked_by_empty.diagnostics.empty());
    MK_REQUIRE(blocked_by_empty.diagnostics[0].contains("source path"));

    auto failed_copy = ready_input;
    failed_copy.copy_failed = true;
    failed_copy.diagnostic = "copy failed: access denied";
    const auto failed = mirakana::editor::make_content_browser_import_external_source_copy_model({failed_copy});
    MK_REQUIRE(failed.status == mirakana::editor::EditorContentBrowserImportExternalSourceCopyStatus::failed);
    MK_REQUIRE(!failed.can_copy);
    MK_REQUIRE(!failed.diagnostics.empty());
    MK_REQUIRE(failed.diagnostics[0] == "copy failed: access denied");
}

MK_TEST("editor asset pipeline converts imported artifacts to asset records") {
    const auto texture_id = mirakana::AssetId::from_name("textures/player");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto audio_id = mirakana::AssetId::from_name("audio/hit");
    const auto pose_id = mirakana::AssetId::from_name("animations/player_pose");
    const auto scene_id = mirakana::AssetId::from_name("scenes/level");

    mirakana::AssetImportExecutionResult result;
    result.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = pose_id,
        .kind = mirakana::AssetImportActionKind::animation_quaternion_clip,
        .output_path = "assets/animations/player_pose.animation_quaternion_clip",
    });
    result.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = audio_id, .kind = mirakana::AssetImportActionKind::audio, .output_path = "assets/audio/hit.audio"});
    result.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = scene_id, .kind = mirakana::AssetImportActionKind::scene, .output_path = "assets/scenes/level.scene"});
    result.imported.push_back(mirakana::AssetImportedArtifact{.asset = texture_id,
                                                              .kind = mirakana::AssetImportActionKind::texture,
                                                              .output_path = "assets/textures/player.texture"});
    result.imported.push_back(mirakana::AssetImportedArtifact{.asset = material_id,
                                                              .kind = mirakana::AssetImportActionKind::material,
                                                              .output_path = "assets/materials/player.material"});
    result.imported.push_back(mirakana::AssetImportedArtifact{
        .asset = mesh_id, .kind = mirakana::AssetImportActionKind::mesh, .output_path = "assets/meshes/player.mesh"});
    result.imported.push_back(mirakana::AssetImportedArtifact{.asset = mirakana::AssetId::from_name("unsupported"),
                                                              .kind = mirakana::AssetImportActionKind::unknown,
                                                              .output_path = "assets/unsupported.asset"});
    result.failures.push_back(mirakana::AssetImportFailure{.asset = mirakana::AssetId::from_name("textures/missing"),
                                                           .kind = mirakana::AssetImportActionKind::texture,
                                                           .source_path = "source/textures/missing.texture",
                                                           .output_path = "assets/textures/missing.texture",
                                                           .diagnostic = "missing source"});

    const auto records = mirakana::editor::make_imported_asset_records(result);

    MK_REQUIRE(records.size() == 6);
    MK_REQUIRE(records[0].id == pose_id);
    MK_REQUIRE(records[0].kind == mirakana::AssetKind::animation_quaternion_clip);
    MK_REQUIRE(records[0].path == "assets/animations/player_pose.animation_quaternion_clip");
    MK_REQUIRE(records[1].id == audio_id);
    MK_REQUIRE(records[1].kind == mirakana::AssetKind::audio);
    MK_REQUIRE(records[1].path == "assets/audio/hit.audio");
    MK_REQUIRE(records[2].id == material_id);
    MK_REQUIRE(records[2].kind == mirakana::AssetKind::material);
    MK_REQUIRE(records[2].path == "assets/materials/player.material");
    MK_REQUIRE(records[3].id == mesh_id);
    MK_REQUIRE(records[3].kind == mirakana::AssetKind::mesh);
    MK_REQUIRE(records[3].path == "assets/meshes/player.mesh");
    MK_REQUIRE(records[4].id == scene_id);
    MK_REQUIRE(records[4].kind == mirakana::AssetKind::scene);
    MK_REQUIRE(records[4].path == "assets/scenes/level.scene");
    MK_REQUIRE(records[5].id == texture_id);
    MK_REQUIRE(records[5].kind == mirakana::AssetKind::texture);
    MK_REQUIRE(records[5].path == "assets/textures/player.texture");

    mirakana::AssetRegistry registry;
    registry.add(mirakana::AssetRecord{
        .id = texture_id, .kind = mirakana::AssetKind::texture, .path = "assets/textures/player.texture"});

    const auto added = mirakana::editor::add_imported_asset_records(registry, result);

    MK_REQUIRE(added == 5);
    MK_REQUIRE(registry.count() == 6);
    MK_REQUIRE(registry.find(pose_id) != nullptr);
    MK_REQUIRE(registry.find(audio_id) != nullptr);
    MK_REQUIRE(registry.find(material_id) != nullptr);
    MK_REQUIRE(registry.find(mesh_id) != nullptr);
    MK_REQUIRE(registry.find(scene_id) != nullptr);
}

MK_TEST("editor shader compile state tracks queued cached compiled and failed actions") {
    const auto shader_id = mirakana::AssetId::from_name("shaders/fullscreen.hlsl");
    mirakana::editor::ShaderCompileState state;
    state.set_requests({
        mirakana::ShaderCompileRequest{
            .source =
                mirakana::ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = "assets/shaders/fullscreen.hlsl",
                    .language = mirakana::ShaderSourceLanguage::hlsl,
                    .stage = mirakana::ShaderSourceStage::vertex,
                    .entry_point = "vs_main",
                    .defines = {},
                    .artifacts = {},
                    .reflection = {},
                },
            .target = mirakana::ShaderCompileTarget::d3d12_dxil,
            .output_path = "out/shaders/fullscreen.vs.dxil",
            .profile = "vs_6_7",
            .include_paths = {},
        },
    });

    MK_REQUIRE(state.item_count() == 1);
    MK_REQUIRE(state.pending_count() == 1);
    MK_REQUIRE(state.items()[0].status == mirakana::editor::EditorShaderCompileStatus::pending);

    state.apply_updates({
        mirakana::editor::EditorShaderCompileUpdate{
            .shader = shader_id,
            .output_path = "out/shaders/fullscreen.vs.dxil",
            .status = mirakana::editor::EditorShaderCompileStatus::compiled,
            .diagnostic = "compiled",
            .cache_hit = false,
        },
    });

    MK_REQUIRE(state.compiled_count() == 1);
    MK_REQUIRE(state.items()[0].diagnostic == "compiled");

    state.apply_updates({
        mirakana::editor::EditorShaderCompileUpdate{
            .shader = shader_id,
            .output_path = "out/shaders/fullscreen.vs.dxil",
            .status = mirakana::editor::EditorShaderCompileStatus::cached,
            .diagnostic = "cache hit",
            .cache_hit = true,
        },
    });

    MK_REQUIRE(state.cached_count() == 1);
    MK_REQUIRE(state.items()[0].cache_hit);
    MK_REQUIRE(mirakana::editor::editor_shader_compile_status_label(state.items()[0].status) == "Cached");

    state.apply_updates({
        mirakana::editor::EditorShaderCompileUpdate{
            .shader = shader_id,
            .output_path = "out/shaders/fullscreen.vs.dxil",
            .status = mirakana::editor::EditorShaderCompileStatus::failed,
            .diagnostic = "compiler error",
            .cache_hit = false,
        },
    });

    MK_REQUIRE(state.failed_count() == 1);
    MK_REQUIRE(state.items()[0].diagnostic == "compiler error");
}

MK_TEST("editor shader compile state applies execution results") {
    const auto shader_id = mirakana::AssetId::from_name("shaders/fullscreen.hlsl");
    const mirakana::ShaderCompileRequest request{
        .source =
            mirakana::ShaderSourceMetadata{
                .id = shader_id,
                .source_path = "assets/shaders/fullscreen.hlsl",
                .language = mirakana::ShaderSourceLanguage::hlsl,
                .stage = mirakana::ShaderSourceStage::vertex,
                .entry_point = "vs_main",
                .defines = {},
                .artifacts = {},
                .reflection = {},
            },
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.vs.dxil",
        .profile = "vs_6_7",
        .include_paths = {},
    };

    mirakana::editor::ShaderCompileState state;
    state.set_requests({request});

    mirakana::ShaderCompileExecutionResult compiled;
    compiled.tool_result =
        mirakana::ShaderToolRunResult{.exit_code = 0,
                                      .diagnostic = "compiled",
                                      .stdout_text = {},
                                      .stderr_text = {},
                                      .artifact = mirakana::make_shader_compile_command(request).artifact};

    state.apply_execution_results({
        mirakana::editor::EditorShaderCompileExecution{.request = request, .result = compiled},
    });

    MK_REQUIRE(state.compiled_count() == 1);
    MK_REQUIRE(state.items()[0].diagnostic == "compiled");

    mirakana::ShaderCompileExecutionResult cached;
    cached.cache_hit = true;
    cached.tool_result =
        mirakana::ShaderToolRunResult{.exit_code = 0,
                                      .diagnostic = "cache hit",
                                      .stdout_text = {},
                                      .stderr_text = {},
                                      .artifact = mirakana::make_shader_compile_command(request).artifact};

    state.apply_execution_results({
        mirakana::editor::EditorShaderCompileExecution{.request = request, .result = cached},
    });

    MK_REQUIRE(state.cached_count() == 1);
    MK_REQUIRE(state.items()[0].cache_hit);

    mirakana::ShaderCompileExecutionResult failed;
    failed.tool_result =
        mirakana::ShaderToolRunResult{.exit_code = 3,
                                      .diagnostic = "CreateProcessW failed",
                                      .stdout_text = {},
                                      .stderr_text = "dxc missing",
                                      .artifact = mirakana::make_shader_compile_command(request).artifact};

    state.apply_execution_results({
        mirakana::editor::EditorShaderCompileExecution{.request = request, .result = failed},
    });

    MK_REQUIRE(state.failed_count() == 1);
    MK_REQUIRE(state.items()[0].diagnostic.contains("CreateProcessW failed"));
}

MK_TEST("editor viewport shader compile requests include d3d12 and vulkan targets") {
    const auto requests = mirakana::editor::make_viewport_shader_compile_requests("out/editor/shaders");

    MK_REQUIRE(requests.size() == 4);
    MK_REQUIRE(requests[0].source.stage == mirakana::ShaderSourceStage::vertex);
    MK_REQUIRE(requests[0].target == mirakana::ShaderCompileTarget::d3d12_dxil);
    MK_REQUIRE(requests[0].output_path == "out/editor/shaders/editor-default.vs.dxil");
    MK_REQUIRE(requests[1].source.stage == mirakana::ShaderSourceStage::fragment);
    MK_REQUIRE(requests[1].target == mirakana::ShaderCompileTarget::d3d12_dxil);
    MK_REQUIRE(requests[1].output_path == "out/editor/shaders/editor-default.ps.dxil");
    MK_REQUIRE(requests[2].source.stage == mirakana::ShaderSourceStage::vertex);
    MK_REQUIRE(requests[2].target == mirakana::ShaderCompileTarget::vulkan_spirv);
    MK_REQUIRE(requests[2].output_path == "out/editor/shaders/editor-default.vs.spv");
    MK_REQUIRE(requests[3].source.stage == mirakana::ShaderSourceStage::fragment);
    MK_REQUIRE(requests[3].target == mirakana::ShaderCompileTarget::vulkan_spirv);
    MK_REQUIRE(requests[3].output_path == "out/editor/shaders/editor-default.ps.spv");

    const auto vulkan_command = mirakana::make_shader_compile_command(requests[2]);
    MK_REQUIRE(vulkan_command.executable == "dxc");
    MK_REQUIRE(vulkan_command.arguments.size() >= 2);
    MK_REQUIRE(vulkan_command.arguments[0] == "-spirv");
    MK_REQUIRE(vulkan_command.arguments[1] == "-fspv-target-env=vulkan1.3");
}

MK_TEST("editor material preview shader compile requests include factor and textured variants") {
    const auto requests = mirakana::editor::make_material_preview_shader_compile_requests("out/editor/shaders");
    const auto factor_shader = mirakana::AssetId::from_name("editor.material_preview.factor.shader");
    const auto textured_shader = mirakana::AssetId::from_name("editor.material_preview.textured.shader");

    MK_REQUIRE(requests.size() == 8);
    MK_REQUIRE(requests[0].source.id == factor_shader);
    MK_REQUIRE(requests[0].source.source_path == "out/editor/shaders/material-preview.hlsl");
    MK_REQUIRE(requests[0].source.stage == mirakana::ShaderSourceStage::vertex);
    MK_REQUIRE(requests[0].target == mirakana::ShaderCompileTarget::d3d12_dxil);
    MK_REQUIRE(requests[0].output_path == "out/editor/shaders/editor-material-preview-factor.vs.dxil");
    MK_REQUIRE(requests[0].source.defines.size() == 1);
    MK_REQUIRE(requests[0].source.defines[0] == "MK_MATERIAL_PREVIEW_FACTOR_ONLY=1");

    MK_REQUIRE(requests[3].source.id == textured_shader);
    MK_REQUIRE(requests[3].source.stage == mirakana::ShaderSourceStage::fragment);
    MK_REQUIRE(requests[3].target == mirakana::ShaderCompileTarget::d3d12_dxil);
    MK_REQUIRE(requests[3].output_path == "out/editor/shaders/editor-material-preview-textured.ps.dxil");
    MK_REQUIRE(requests[3].source.defines.size() == 1);
    MK_REQUIRE(requests[3].source.defines[0] == "MK_MATERIAL_PREVIEW_TEXTURED=1");

    MK_REQUIRE(requests[6].source.id == textured_shader);
    MK_REQUIRE(requests[6].source.stage == mirakana::ShaderSourceStage::vertex);
    MK_REQUIRE(requests[6].target == mirakana::ShaderCompileTarget::vulkan_spirv);
    MK_REQUIRE(requests[6].output_path == "out/editor/shaders/editor-material-preview-textured.vs.spv");

    const auto textured_command = mirakana::make_shader_compile_command(requests[6]);
    MK_REQUIRE(textured_command.arguments.size() >= 4);
    MK_REQUIRE(textured_command.arguments[0] == "-spirv");
    MK_REQUIRE(textured_command.arguments[1] == "-fspv-target-env=vulkan1.3");
    MK_REQUIRE(textured_command.arguments[2] == "-T");
    MK_REQUIRE(textured_command.arguments[3] == "vs_6_7");
    MK_REQUIRE(std::ranges::contains(textured_command.arguments, std::string_view{"MK_MATERIAL_PREVIEW_TEXTURED=1"}));
}

MK_TEST("editor shader tool discovery state exposes deterministic tool options") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("toolchains/dxc/bin/dxc.exe", "binary marker");
    fs.write_text("toolchains/dxc/bin/dxc.version", "dxcompiler 1.8.2505\n");
    fs.write_text("toolchains/dxc/bin/dxc.spirv-codegen", "enabled\n");
    fs.write_text("toolchains/vulkan/bin/spirv-val.exe", "binary marker");
    fs.write_text("toolchains/apple/bin/metal", "binary marker");

    mirakana::editor::ShaderToolDiscoveryState state;
    state.refresh_from(mirakana::discover_shader_tools(
        fs, mirakana::ShaderToolDiscoveryRequest{
                .search_roots = {"toolchains/apple/bin", "toolchains/vulkan/bin", "toolchains/dxc/bin"}}));

    MK_REQUIRE(state.item_count() == 3);
    MK_REQUIRE(state.items()[0].kind == mirakana::ShaderToolKind::dxc);
    MK_REQUIRE(state.items()[0].label == "dxc");
    MK_REQUIRE(state.items()[0].executable_path == "toolchains/dxc/bin/dxc.exe");
    MK_REQUIRE(state.items()[0].version == "dxcompiler 1.8.2505");
    MK_REQUIRE(state.items()[1].kind == mirakana::ShaderToolKind::spirv_val);
    MK_REQUIRE(state.items()[1].label == "spirv-val");
    MK_REQUIRE(state.items()[2].kind == mirakana::ShaderToolKind::metal);
    MK_REQUIRE(state.find_first(mirakana::ShaderToolKind::dxc) == state.items().data());
    MK_REQUIRE(state.find_first(mirakana::ShaderToolKind::metallib) == nullptr);

    const auto readiness = state.readiness();
    MK_REQUIRE(readiness.ready_for_d3d12_dxil());
    MK_REQUIRE(readiness.ready_for_vulkan_spirv());
    MK_REQUIRE(!readiness.ready_for_metal_library());
    MK_REQUIRE(readiness.diagnostics.size() == 1);
    MK_REQUIRE(readiness.diagnostics[0].contains("metallib"));
}

MK_TEST("editor viewport shader artifacts require non-empty d3d12 vertex and fragment bytecode") {
    const auto shader_id = mirakana::AssetId::from_name("shaders/editor-default.hlsl");
    const std::vector<mirakana::ShaderCompileRequest> requests{
        mirakana::ShaderCompileRequest{
            .source =
                mirakana::ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = "out/editor/shaders/default.hlsl",
                    .language = mirakana::ShaderSourceLanguage::hlsl,
                    .stage = mirakana::ShaderSourceStage::vertex,
                    .entry_point = "vs_main",
                    .defines = {},
                    .artifacts = {},
                    .reflection = {},
                },
            .target = mirakana::ShaderCompileTarget::d3d12_dxil,
            .output_path = "out/editor/shaders/editor-default.vs.dxil",
            .profile = "vs_6_7",
            .include_paths = {},
        },
        mirakana::ShaderCompileRequest{
            .source =
                mirakana::ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = "out/editor/shaders/default.hlsl",
                    .language = mirakana::ShaderSourceLanguage::hlsl,
                    .stage = mirakana::ShaderSourceStage::fragment,
                    .entry_point = "ps_main",
                    .defines = {},
                    .artifacts = {},
                    .reflection = {},
                },
            .target = mirakana::ShaderCompileTarget::d3d12_dxil,
            .output_path = "out/editor/shaders/editor-default.ps.dxil",
            .profile = "ps_6_7",
            .include_paths = {},
        },
    };

    mirakana::MemoryFileSystem fs;
    mirakana::editor::ViewportShaderArtifactState state;
    state.refresh_from(fs, requests);

    MK_REQUIRE(state.item_count() == 2);
    MK_REQUIRE(!state.ready_for_d3d12());
    MK_REQUIRE(state.find(mirakana::ShaderSourceStage::vertex)->status ==
               mirakana::editor::ViewportShaderArtifactStatus::missing);
    MK_REQUIRE(state.find(mirakana::ShaderSourceStage::fragment)->status ==
               mirakana::editor::ViewportShaderArtifactStatus::missing);

    fs.write_text("out/editor/shaders/editor-default.vs.dxil", "vertex bytecode");
    state.refresh_from(fs, requests);

    MK_REQUIRE(!state.ready_for_d3d12());
    MK_REQUIRE(state.find(mirakana::ShaderSourceStage::vertex)->status ==
               mirakana::editor::ViewportShaderArtifactStatus::ready);
    MK_REQUIRE(state.find(mirakana::ShaderSourceStage::vertex)->byte_size == 15);

    fs.write_text("out/editor/shaders/editor-default.ps.dxil", "fragment bytecode");
    state.refresh_from(fs, requests);

    MK_REQUIRE(state.ready_for_d3d12());
    MK_REQUIRE(state.ready_count() == 2);
    MK_REQUIRE(state.find(mirakana::ShaderSourceStage::fragment)->byte_size == 17);
}

MK_TEST("editor shader artifacts can query material preview variants by shader id") {
    auto requests = mirakana::editor::make_viewport_shader_compile_requests("out/editor/shaders");
    auto material_requests = mirakana::editor::make_material_preview_shader_compile_requests("out/editor/shaders");
    requests.insert(requests.end(), material_requests.begin(), material_requests.end());

    const auto factor_shader = mirakana::AssetId::from_name("editor.material_preview.factor.shader");
    const auto textured_shader = mirakana::AssetId::from_name("editor.material_preview.textured.shader");

    mirakana::MemoryFileSystem fs;
    fs.write_text("out/editor/shaders/editor-material-preview-textured.vs.dxil", "textured vertex bytecode");
    fs.write_text("out/editor/shaders/editor-material-preview-textured.ps.dxil", "textured fragment bytecode");
    fs.write_text("out/editor/shaders/editor-material-preview-factor.vs.spv", "factor vertex spirv");
    fs.write_text("out/editor/shaders/editor-material-preview-factor.ps.spv", "factor fragment spirv");

    mirakana::editor::ViewportShaderArtifactState state;
    state.refresh_from(fs, requests);

    MK_REQUIRE(!state.ready_for_d3d12());
    MK_REQUIRE(state.ready_for_d3d12(textured_shader));
    MK_REQUIRE(!state.ready_for_d3d12(factor_shader));
    MK_REQUIRE(state.ready_for_vulkan(factor_shader));
    MK_REQUIRE(!state.ready_for_vulkan(textured_shader));
    MK_REQUIRE(state.find(textured_shader, mirakana::ShaderSourceStage::vertex,
                          mirakana::ShaderCompileTarget::d3d12_dxil) != nullptr);
    MK_REQUIRE(
        state.find(textured_shader, mirakana::ShaderSourceStage::vertex, mirakana::ShaderCompileTarget::d3d12_dxil)
            ->byte_size == 24);
    MK_REQUIRE(
        state.find(factor_shader, mirakana::ShaderSourceStage::fragment, mirakana::ShaderCompileTarget::vulkan_spirv)
            ->byte_size == 21);
}

MK_TEST("editor viewport shader artifacts reject empty artifacts and non d3d12 targets") {
    const auto shader_id = mirakana::AssetId::from_name("shaders/editor-default.hlsl");
    const std::vector<mirakana::ShaderCompileRequest> requests{
        mirakana::ShaderCompileRequest{
            .source =
                mirakana::ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = "out/editor/shaders/default.hlsl",
                    .language = mirakana::ShaderSourceLanguage::hlsl,
                    .stage = mirakana::ShaderSourceStage::vertex,
                    .entry_point = "vs_main",
                    .defines = {},
                    .artifacts = {},
                    .reflection = {},
                },
            .target = mirakana::ShaderCompileTarget::d3d12_dxil,
            .output_path = "out/editor/shaders/editor-default.vs.dxil",
            .profile = "vs_6_7",
            .include_paths = {},
        },
        mirakana::ShaderCompileRequest{
            .source =
                mirakana::ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = "out/editor/shaders/default.hlsl",
                    .language = mirakana::ShaderSourceLanguage::hlsl,
                    .stage = mirakana::ShaderSourceStage::fragment,
                    .entry_point = "ps_main",
                    .defines = {},
                    .artifacts = {},
                    .reflection = {},
                },
            .target = mirakana::ShaderCompileTarget::vulkan_spirv,
            .output_path = "out/editor/shaders/editor-default.ps.spv",
            .profile = "ps_6_7",
            .include_paths = {},
        },
        mirakana::ShaderCompileRequest{
            .source =
                mirakana::ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = "out/editor/shaders/default.hlsl",
                    .language = mirakana::ShaderSourceLanguage::hlsl,
                    .stage = mirakana::ShaderSourceStage::fragment,
                    .entry_point = "ps_main",
                    .defines = {},
                    .artifacts = {},
                    .reflection = {},
                },
            .target = mirakana::ShaderCompileTarget::metal_ir,
            .output_path = "out/editor/shaders/editor-default.ps.air",
            .profile = "ps_6_7",
            .include_paths = {},
        },
    };

    mirakana::MemoryFileSystem fs;
    fs.write_text("out/editor/shaders/editor-default.vs.dxil", "");
    fs.write_text("out/editor/shaders/editor-default.ps.spv", "spirv bytecode");
    fs.write_text("out/editor/shaders/editor-default.ps.air", "metal bytecode");

    mirakana::editor::ViewportShaderArtifactState state;
    state.refresh_from(fs, requests);

    MK_REQUIRE(!state.ready_for_d3d12());
    MK_REQUIRE(!state.ready_for_vulkan());
    MK_REQUIRE(state.find(mirakana::ShaderSourceStage::vertex)->status ==
               mirakana::editor::ViewportShaderArtifactStatus::empty);
    MK_REQUIRE(state.find(mirakana::ShaderSourceStage::fragment, mirakana::ShaderCompileTarget::vulkan_spirv)->status ==
               mirakana::editor::ViewportShaderArtifactStatus::ready);
    MK_REQUIRE(state.find(mirakana::ShaderSourceStage::fragment, mirakana::ShaderCompileTarget::metal_ir)->status ==
               mirakana::editor::ViewportShaderArtifactStatus::unsupported_target);
    MK_REQUIRE(mirakana::editor::viewport_shader_artifact_status_label(
                   mirakana::editor::ViewportShaderArtifactStatus::unsupported_target) == "Unsupported Target");
}

MK_TEST("editor viewport shader artifacts require non-empty vulkan vertex and fragment spirv") {
    const auto shader_id = mirakana::AssetId::from_name("shaders/editor-default.hlsl");
    const std::vector<mirakana::ShaderCompileRequest> requests{
        mirakana::ShaderCompileRequest{
            .source =
                mirakana::ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = "out/editor/shaders/default.hlsl",
                    .language = mirakana::ShaderSourceLanguage::hlsl,
                    .stage = mirakana::ShaderSourceStage::vertex,
                    .entry_point = "vs_main",
                    .defines = {},
                    .artifacts = {},
                    .reflection = {},
                },
            .target = mirakana::ShaderCompileTarget::vulkan_spirv,
            .output_path = "out/editor/shaders/editor-default.vs.spv",
            .profile = "vs_6_7",
            .include_paths = {},
        },
        mirakana::ShaderCompileRequest{
            .source =
                mirakana::ShaderSourceMetadata{
                    .id = shader_id,
                    .source_path = "out/editor/shaders/default.hlsl",
                    .language = mirakana::ShaderSourceLanguage::hlsl,
                    .stage = mirakana::ShaderSourceStage::fragment,
                    .entry_point = "ps_main",
                    .defines = {},
                    .artifacts = {},
                    .reflection = {},
                },
            .target = mirakana::ShaderCompileTarget::vulkan_spirv,
            .output_path = "out/editor/shaders/editor-default.ps.spv",
            .profile = "ps_6_7",
            .include_paths = {},
        },
    };

    mirakana::MemoryFileSystem fs;
    mirakana::editor::ViewportShaderArtifactState state;
    state.refresh_from(fs, requests);

    MK_REQUIRE(!state.ready_for_vulkan());
    MK_REQUIRE(!state.ready_for_d3d12());

    fs.write_text("out/editor/shaders/editor-default.vs.spv", "vertex spirv");
    state.refresh_from(fs, requests);

    MK_REQUIRE(!state.ready_for_vulkan());

    fs.write_text("out/editor/shaders/editor-default.ps.spv", "fragment spirv");
    state.refresh_from(fs, requests);

    MK_REQUIRE(state.ready_for_vulkan());
    MK_REQUIRE(!state.ready_for_d3d12());
    MK_REQUIRE(state.ready_count() == 2);
}

MK_TEST("editor viewport tracks renderer target state and frame updates") {
    mirakana::editor::ViewportState viewport;

    MK_REQUIRE(!viewport.ready());
    viewport.set_renderer("d3d12");
    viewport.resize(mirakana::editor::ViewportExtent{.width = 1280, .height = 720});
    viewport.set_focused(true);
    viewport.set_hovered(true);

    MK_REQUIRE(viewport.ready());
    MK_REQUIRE(viewport.renderer_name() == "d3d12");
    MK_REQUIRE(viewport.extent().width == 1280);
    MK_REQUIRE(viewport.focused());
    MK_REQUIRE(viewport.hovered());
    MK_REQUIRE(viewport.rendered_frame_count() == 0);

    viewport.mark_frame_rendered();
    viewport.mark_frame_rendered();
    MK_REQUIRE(viewport.rendered_frame_count() == 2);

    bool rejected_zero_extent = false;
    try {
        viewport.resize(mirakana::editor::ViewportExtent{.width = 0, .height = 720});
    } catch (const std::invalid_argument&) {
        rejected_zero_extent = true;
    }
    MK_REQUIRE(rejected_zero_extent);
}

MK_TEST("editor render backend selection prefers host native backends before null") {
    const mirakana::editor::EditorRenderBackendAvailability availability{
        .d3d12 = true,
        .vulkan = true,
        .metal = true,
    };

    const auto windows_choice =
        mirakana::editor::choose_editor_render_backend(mirakana::editor::EditorRenderBackend::automatic, availability,
                                                       mirakana::editor::EditorRenderBackendHost::windows);
    MK_REQUIRE(windows_choice.active == mirakana::editor::EditorRenderBackend::d3d12);
    MK_REQUIRE(windows_choice.exact_match);

    const auto linux_choice =
        mirakana::editor::choose_editor_render_backend(mirakana::editor::EditorRenderBackend::automatic, availability,
                                                       mirakana::editor::EditorRenderBackendHost::linux);
    MK_REQUIRE(linux_choice.active == mirakana::editor::EditorRenderBackend::vulkan);

    const auto apple_choice =
        mirakana::editor::choose_editor_render_backend(mirakana::editor::EditorRenderBackend::automatic, availability,
                                                       mirakana::editor::EditorRenderBackendHost::apple);
    MK_REQUIRE(apple_choice.active == mirakana::editor::EditorRenderBackend::metal);
}

MK_TEST("editor render backend selection falls back to null when requested backend is unavailable") {
    const mirakana::editor::EditorRenderBackendAvailability availability{};

    const auto choice = mirakana::editor::choose_editor_render_backend(
        mirakana::editor::EditorRenderBackend::d3d12, availability, mirakana::editor::EditorRenderBackendHost::windows);

    MK_REQUIRE(choice.requested == mirakana::editor::EditorRenderBackend::d3d12);
    MK_REQUIRE(choice.active == mirakana::editor::EditorRenderBackend::null);
    MK_REQUIRE(!choice.exact_match);
    MK_REQUIRE(choice.diagnostic == "Requested backend unavailable; using NullRhiDevice");
}

MK_TEST("editor render backend descriptors expose availability and ids") {
    const auto descriptors = mirakana::editor::make_editor_render_backend_descriptors(
        mirakana::editor::EditorRenderBackendAvailability{.d3d12 = true, .vulkan = false, .metal = true});

    MK_REQUIRE(descriptors.size() == 4);
    MK_REQUIRE(descriptors[0].backend == mirakana::editor::EditorRenderBackend::null);
    MK_REQUIRE(descriptors[0].available);
    MK_REQUIRE(descriptors[1].backend == mirakana::editor::EditorRenderBackend::d3d12);
    MK_REQUIRE(descriptors[1].available);
    MK_REQUIRE(descriptors[2].backend == mirakana::editor::EditorRenderBackend::vulkan);
    MK_REQUIRE(!descriptors[2].available);
    MK_REQUIRE(descriptors[3].backend == mirakana::editor::EditorRenderBackend::metal);
    MK_REQUIRE(descriptors[3].id == "metal");
}

MK_TEST("editor viewport records requested and active render backend selection") {
    mirakana::editor::ViewportState viewport;
    const auto choice = mirakana::editor::choose_editor_render_backend(
        mirakana::editor::EditorRenderBackend::metal, mirakana::editor::EditorRenderBackendAvailability{},
        mirakana::editor::EditorRenderBackendHost::apple);

    viewport.set_render_backend_selection(choice);
    viewport.resize(mirakana::editor::ViewportExtent{.width = 640, .height = 360});

    MK_REQUIRE(viewport.ready());
    MK_REQUIRE(viewport.requested_render_backend() == mirakana::editor::EditorRenderBackend::metal);
    MK_REQUIRE(viewport.active_render_backend() == mirakana::editor::EditorRenderBackend::null);
    MK_REQUIRE(viewport.renderer_name() == "null");
    MK_REQUIRE(viewport.render_backend_diagnostic() == "Requested backend unavailable; using NullRhiDevice");
}

MK_TEST("editor viewport controls run mode simulation ticks and active tool") {
    mirakana::editor::ViewportState viewport;

    MK_REQUIRE(viewport.run_mode() == mirakana::editor::ViewportRunMode::edit);
    MK_REQUIRE(viewport.active_tool() == mirakana::editor::ViewportTool::select);
    MK_REQUIRE(viewport.simulation_frame_count() == 0);
    MK_REQUIRE(!viewport.mark_simulation_tick());

    MK_REQUIRE(viewport.play());
    MK_REQUIRE(viewport.run_mode() == mirakana::editor::ViewportRunMode::play);
    MK_REQUIRE(!viewport.play());
    MK_REQUIRE(viewport.mark_simulation_tick());
    MK_REQUIRE(viewport.simulation_frame_count() == 1);

    MK_REQUIRE(viewport.pause());
    MK_REQUIRE(viewport.run_mode() == mirakana::editor::ViewportRunMode::paused);
    MK_REQUIRE(!viewport.pause());
    MK_REQUIRE(!viewport.mark_simulation_tick());
    MK_REQUIRE(viewport.simulation_frame_count() == 1);

    MK_REQUIRE(viewport.resume());
    MK_REQUIRE(viewport.run_mode() == mirakana::editor::ViewportRunMode::play);
    MK_REQUIRE(viewport.mark_simulation_tick());
    MK_REQUIRE(viewport.simulation_frame_count() == 2);

    viewport.set_active_tool(mirakana::editor::ViewportTool::rotate);
    MK_REQUIRE(viewport.active_tool() == mirakana::editor::ViewportTool::rotate);
    MK_REQUIRE(mirakana::editor::viewport_tool_label(viewport.active_tool()) == "Rotate");
    MK_REQUIRE(mirakana::editor::viewport_run_mode_label(viewport.run_mode()) == "Play");

    MK_REQUIRE(viewport.stop());
    MK_REQUIRE(viewport.run_mode() == mirakana::editor::ViewportRunMode::edit);
    MK_REQUIRE(viewport.simulation_frame_count() == 0);
    MK_REQUIRE(!viewport.stop());
}

MK_TEST("editor play in editor session isolates simulation scene from source document") {
    mirakana::Scene scene("Editor Scene");
    const auto player = scene.create_node("Player");
    scene.find_node(player)->transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};

    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");
    MK_REQUIRE(document.select_node(player));

    mirakana::editor::EditorPlaySession session;
    MK_REQUIRE(session.state() == mirakana::editor::EditorPlaySessionState::edit);
    MK_REQUIRE(session.begin(document) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(session.active());
    MK_REQUIRE(session.source_scene_edits_blocked());

    auto* simulation_scene = session.mutable_simulation_scene();
    MK_REQUIRE(simulation_scene != nullptr);
    auto* simulation_player = simulation_scene->find_node(player);
    MK_REQUIRE(simulation_player != nullptr);
    simulation_player->name = "Simulated Player";
    simulation_player->transform.position = mirakana::Vec3{.x = 8.0F, .y = 9.0F, .z = 10.0F};

    const auto* source_player = document.scene().find_node(player);
    MK_REQUIRE(source_player != nullptr);
    MK_REQUIRE(source_player->name == "Player");
    MK_REQUIRE(source_player->transform.position == (mirakana::Vec3{1.0F, 2.0F, 3.0F}));

    const auto report = mirakana::editor::make_editor_play_session_report(session, document);
    MK_REQUIRE(report.state == mirakana::editor::EditorPlaySessionState::play);
    MK_REQUIRE(report.active);
    MK_REQUIRE(report.source_scene_edits_blocked);
    MK_REQUIRE(report.source_node_count == 1);
    MK_REQUIRE(report.simulation_node_count == 1);
    MK_REQUIRE(report.selected_node.value == player.value);
    MK_REQUIRE(report.diagnostic == "play session is using an isolated simulation scene");
}

MK_TEST("editor play in editor session controls ticks pause resume stop and rejects invalid transitions") {
    mirakana::Scene scene("Editor Scene");
    const auto player = scene.create_node("Player");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");

    mirakana::editor::EditorPlaySession session;
    MK_REQUIRE(session.stop() == mirakana::editor::EditorPlaySessionActionStatus::rejected_inactive);
    MK_REQUIRE(session.begin(document) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(session.begin(document) == mirakana::editor::EditorPlaySessionActionStatus::rejected_active);
    MK_REQUIRE(session.tick() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(session.simulation_frame_count() == 1);

    MK_REQUIRE(session.pause() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(session.state() == mirakana::editor::EditorPlaySessionState::paused);
    MK_REQUIRE(session.tick() == mirakana::editor::EditorPlaySessionActionStatus::rejected_wrong_state);
    MK_REQUIRE(session.simulation_frame_count() == 1);
    MK_REQUIRE(session.resume() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(session.tick() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(session.simulation_frame_count() == 2);

    auto* simulation_scene = session.mutable_simulation_scene();
    MK_REQUIRE(simulation_scene != nullptr);
    simulation_scene->find_node(player)->name = "Runtime Only";

    MK_REQUIRE(session.stop() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(!session.active());
    MK_REQUIRE(session.state() == mirakana::editor::EditorPlaySessionState::edit);
    MK_REQUIRE(session.simulation_frame_count() == 0);
    MK_REQUIRE(session.simulation_scene() == nullptr);
    MK_REQUIRE(document.scene().find_node(player)->name == "Player");

    mirakana::Scene empty("Empty");
    auto empty_document = mirakana::editor::SceneAuthoringDocument::from_scene(empty, "scenes/empty.scene");
    MK_REQUIRE(session.begin(empty_document) == mirakana::editor::EditorPlaySessionActionStatus::rejected_empty_scene);
    MK_REQUIRE(mirakana::editor::editor_play_session_state_label(mirakana::editor::EditorPlaySessionState::play) ==
               "Play");
    MK_REQUIRE(mirakana::editor::editor_play_session_action_status_label(
                   mirakana::editor::EditorPlaySessionActionStatus::rejected_wrong_state) == "RejectedWrongState");
}

class RecordingEditorPlaySessionDriver final : public mirakana::editor::IEditorPlaySessionDriver {
  public:
    void on_play_begin(mirakana::Scene& scene) override {
        ++begin_count_;
        (void)scene.create_node("DriverBegin");
        begin_node_count_ = scene.nodes().size();
    }

    void on_play_tick(mirakana::Scene& scene, const mirakana::editor::EditorPlaySessionTickContext& context) override {
        ++tick_count_;
        tick_contexts_.push_back(context);
        (void)scene.create_node("DriverTick" + std::to_string(tick_count_));
    }

    void on_play_end(mirakana::Scene& scene) override {
        ++end_count_;
        end_node_count_ = scene.nodes().size();
    }

    [[nodiscard]] int begin_count() const noexcept {
        return begin_count_;
    }
    [[nodiscard]] int tick_count() const noexcept {
        return tick_count_;
    }
    [[nodiscard]] int end_count() const noexcept {
        return end_count_;
    }
    [[nodiscard]] std::size_t begin_node_count() const noexcept {
        return begin_node_count_;
    }
    [[nodiscard]] std::size_t end_node_count() const noexcept {
        return end_node_count_;
    }
    [[nodiscard]] const std::vector<mirakana::editor::EditorPlaySessionTickContext>& tick_contexts() const noexcept {
        return tick_contexts_;
    }

  private:
    int begin_count_{0};
    int tick_count_{0};
    int end_count_{0};
    std::size_t begin_node_count_{0};
    std::size_t end_node_count_{0};
    std::vector<mirakana::editor::EditorPlaySessionTickContext> tick_contexts_;
};

struct FunctionTableDriverState {
    int begin_count{0};
    int tick_count{0};
    int end_count{0};
    int destroy_count{0};
    std::vector<mirakana::editor::EditorPlaySessionTickContext> tick_contexts;
};

void module_driver_begin(void* user_data, mirakana::Scene* scene) {
    auto* state = static_cast<FunctionTableDriverState*>(user_data);
    MK_REQUIRE(state != nullptr);
    MK_REQUIRE(scene != nullptr);
    ++state->begin_count;
    (void)scene->create_node("ModuleBegin");
}

void module_driver_tick(void* user_data, mirakana::Scene* scene,
                        const mirakana::editor::EditorPlaySessionTickContext* context) {
    auto* state = static_cast<FunctionTableDriverState*>(user_data);
    MK_REQUIRE(state != nullptr);
    MK_REQUIRE(scene != nullptr);
    MK_REQUIRE(context != nullptr);
    ++state->tick_count;
    state->tick_contexts.push_back(*context);
    (void)scene->create_node("ModuleTick" + std::to_string(state->tick_count));
}

void module_driver_end(void* user_data, mirakana::Scene* scene) {
    auto* state = static_cast<FunctionTableDriverState*>(user_data);
    MK_REQUIRE(state != nullptr);
    MK_REQUIRE(scene != nullptr);
    ++state->end_count;
}

void module_driver_destroy(void* user_data) noexcept {
    auto* state = static_cast<FunctionTableDriverState*>(user_data);
    if (state != nullptr) {
        ++state->destroy_count;
    }
}

[[nodiscard]] mirakana::editor::EditorGameModuleDriverApi
make_test_game_module_driver_api(FunctionTableDriverState& state) {
    mirakana::editor::EditorGameModuleDriverApi api;
    api.abi_version = mirakana::editor::editor_game_module_driver_abi_version_v1;
    api.user_data = &state;
    api.begin = module_driver_begin;
    api.tick = module_driver_tick;
    api.end = module_driver_end;
    api.destroy = module_driver_destroy;
    return api;
}

MK_TEST("editor play in editor gameplay driver mutates only isolated simulation scene") {
    mirakana::Scene scene("Editor Scene");
    const auto player = scene.create_node("Player");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");

    RecordingEditorPlaySessionDriver driver;
    mirakana::editor::EditorPlaySession session;
    MK_REQUIRE(session.begin(document, driver) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(session.gameplay_driver_attached());
    MK_REQUIRE(driver.begin_count() == 1);
    MK_REQUIRE(driver.begin_node_count() == 2);
    MK_REQUIRE(document.scene().nodes().size() == 1);

    MK_REQUIRE(session.tick(0.25) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(session.simulation_frame_count() == 1);
    MK_REQUIRE(driver.tick_count() == 1);
    MK_REQUIRE(driver.tick_contexts().size() == 1);
    MK_REQUIRE(driver.tick_contexts()[0].frame_index == 0);
    MK_REQUIRE(driver.tick_contexts()[0].delta_seconds == 0.25);
    MK_REQUIRE(session.simulation_scene()->nodes().size() == 3);
    MK_REQUIRE(document.scene().nodes().size() == 1);
    MK_REQUIRE(document.scene().find_node(player)->name == "Player");

    const auto report = mirakana::editor::make_editor_play_session_report(session, document);
    MK_REQUIRE(report.gameplay_driver_attached);
    MK_REQUIRE(report.last_delta_seconds == 0.25);
    MK_REQUIRE(report.simulation_node_count == 3);

    MK_REQUIRE(session.stop() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(driver.end_count() == 1);
    MK_REQUIRE(driver.end_node_count() == 3);
    MK_REQUIRE(!session.gameplay_driver_attached());
    MK_REQUIRE(session.simulation_scene() == nullptr);
    MK_REQUIRE(document.scene().nodes().size() == 1);
}

MK_TEST("editor game module function table driver mutates only isolated simulation scene") {
    MK_REQUIRE(mirakana::editor::editor_game_module_driver_abi_name_v1 == "GameEngine.EditorGameModuleDriver.v1");
    MK_REQUIRE(mirakana::editor::editor_game_module_driver_factory_symbol_v1 ==
               "mirakana_create_editor_game_module_driver_v1");

    mirakana::Scene scene("Editor Scene");
    const auto player = scene.create_node("Player");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");

    FunctionTableDriverState state;
    auto result = mirakana::editor::make_editor_game_module_driver_from_api(make_test_game_module_driver_api(state));
    MK_REQUIRE(result.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
    MK_REQUIRE(result.driver != nullptr);
    MK_REQUIRE(result.diagnostics.empty());

    auto driver = std::move(result.driver);
    mirakana::editor::EditorPlaySession session;
    MK_REQUIRE(session.begin(document, *driver) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(state.begin_count == 1);
    MK_REQUIRE(document.scene().nodes().size() == 1);

    MK_REQUIRE(session.tick(0.5) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(state.tick_count == 1);
    MK_REQUIRE(state.tick_contexts.size() == 1);
    MK_REQUIRE(state.tick_contexts[0].frame_index == 0);
    MK_REQUIRE(state.tick_contexts[0].delta_seconds == 0.5);
    MK_REQUIRE(session.simulation_scene()->nodes().size() == 3);
    MK_REQUIRE(document.scene().nodes().size() == 1);
    MK_REQUIRE(document.scene().find_node(player)->name == "Player");

    MK_REQUIRE(session.stop() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(state.end_count == 1);
    MK_REQUIRE(state.destroy_count == 0);

    driver.reset();
    MK_REQUIRE(state.destroy_count == 1);
}

MK_TEST("editor game module driver rejects invalid function tables") {
    FunctionTableDriverState state;
    auto api = make_test_game_module_driver_api(state);

    auto invalid_abi = api;
    invalid_abi.abi_version = mirakana::editor::editor_game_module_driver_abi_version_v1 + 1U;
    const auto invalid_abi_result = mirakana::editor::make_editor_game_module_driver_from_api(invalid_abi);
    MK_REQUIRE(invalid_abi_result.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(invalid_abi_result.driver == nullptr);
    MK_REQUIRE(std::ranges::find(invalid_abi_result.blocked_by, "invalid-abi-version") !=
               invalid_abi_result.blocked_by.end());

    auto missing_tick = api;
    missing_tick.tick = nullptr;
    const auto missing_tick_result = mirakana::editor::make_editor_game_module_driver_from_api(missing_tick);
    MK_REQUIRE(missing_tick_result.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(missing_tick_result.driver == nullptr);
    MK_REQUIRE(std::ranges::find(missing_tick_result.blocked_by, "missing-tick-callback") !=
               missing_tick_result.blocked_by.end());

    auto missing_destroy = api;
    missing_destroy.destroy = nullptr;
    const auto missing_destroy_result = mirakana::editor::make_editor_game_module_driver_from_api(missing_destroy);
    MK_REQUIRE(missing_destroy_result.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(missing_destroy_result.driver == nullptr);
    MK_REQUIRE(std::ranges::find(missing_destroy_result.blocked_by, "missing-destroy-callback") !=
               missing_destroy_result.blocked_by.end());
}

mirakana::editor::EditorGameModuleDriverApi make_symbol_test_driver() {
    static FunctionTableDriverState state;
    return make_test_game_module_driver_api(state);
}

MK_TEST("editor game module driver factory symbol creates driver") {
    const auto result = mirakana::editor::make_editor_game_module_driver_from_symbol(make_symbol_test_driver);
    MK_REQUIRE(result.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
    MK_REQUIRE(result.driver != nullptr);

    const auto missing = mirakana::editor::make_editor_game_module_driver_from_symbol(nullptr);
    MK_REQUIRE(missing.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(std::ranges::find(missing.blocked_by, "missing-factory-symbol") != missing.blocked_by.end());
}

MK_TEST("editor game module driver load model blocks unsafe path and symbol") {
    mirakana::editor::EditorGameModuleDriverLoadDesc unsafe_path;
    unsafe_path.id = "reviewed_driver";
    unsafe_path.module_path = "runtime/game_driver.dll";
    const auto unsafe_path_model = mirakana::editor::make_editor_game_module_driver_load_model(unsafe_path);
    MK_REQUIRE(unsafe_path_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!unsafe_path_model.can_load);
    MK_REQUIRE(std::ranges::find(unsafe_path_model.blocked_by, "absolute-module-path-required") !=
               unsafe_path_model.blocked_by.end());

    mirakana::editor::EditorGameModuleDriverLoadDesc unsafe_symbol;
    unsafe_symbol.id = "reviewed_driver";
    unsafe_symbol.module_path = (std::filesystem::current_path() / "game_driver.dll").string();
    unsafe_symbol.factory_symbol = "bad symbol";
    const auto unsafe_symbol_model = mirakana::editor::make_editor_game_module_driver_load_model(unsafe_symbol);
    MK_REQUIRE(unsafe_symbol_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!unsafe_symbol_model.can_load);
    MK_REQUIRE(std::ranges::find(unsafe_symbol_model.blocked_by, "unsafe-factory-symbol") !=
               unsafe_symbol_model.blocked_by.end());
}

MK_TEST("editor game module driver load model defaults empty factory symbol") {
    mirakana::editor::EditorGameModuleDriverLoadDesc desc;
    desc.id = "reviewed_driver";
    desc.module_path = (std::filesystem::current_path() / "game_driver.dll").string();
    desc.factory_symbol.clear();

    const auto model = mirakana::editor::make_editor_game_module_driver_load_model(desc);
    MK_REQUIRE(model.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
    MK_REQUIRE(model.can_load);
    MK_REQUIRE(model.factory_symbol == mirakana::editor::editor_game_module_driver_factory_symbol_v1);
}

MK_TEST("editor game module driver load model gates active play session and already loaded driver") {
    mirakana::editor::EditorGameModuleDriverLoadDesc base;
    base.id = "reviewed_driver";
    base.module_path = (std::filesystem::current_path() / "game_driver.dll").string();

    auto play_active = base;
    play_active.play_session_active = true;
    const auto play_model = mirakana::editor::make_editor_game_module_driver_load_model(play_active);
    MK_REQUIRE(play_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!play_model.can_load);
    MK_REQUIRE(std::ranges::find(play_model.blocked_by, "play-session-active") != play_model.blocked_by.end());

    auto already = base;
    already.driver_already_loaded = true;
    const auto loaded_model = mirakana::editor::make_editor_game_module_driver_load_model(already);
    MK_REQUIRE(loaded_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!loaded_model.can_load);
    MK_REQUIRE(std::ranges::find(loaded_model.blocked_by, "driver-already-loaded") != loaded_model.blocked_by.end());
}

MK_TEST("editor game module driver load model reports unsupported claims deterministically") {
    mirakana::editor::EditorGameModuleDriverLoadDesc desc;
    desc.id = "reviewed_driver";
    desc.module_path = (std::filesystem::current_path() / "game_driver.dll").string();
    desc.request_hot_reload = true;
    desc.request_desktop_game_runner_embedding = true;
    desc.request_package_script_execution = true;
    desc.request_validation_recipe_execution = true;
    desc.request_arbitrary_shell_execution = true;
    desc.request_renderer_rhi_uploads = true;
    desc.request_renderer_rhi_handle_exposure = true;
    desc.request_package_streaming = true;
    desc.request_stable_third_party_abi = true;
    desc.request_broad_editor_productization = true;

    const auto model = mirakana::editor::make_editor_game_module_driver_load_model(desc);
    MK_REQUIRE(model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!model.can_load);
    MK_REQUIRE(model.unsupported_claims == (std::vector<std::string>{
                                               "hot reload",
                                               "DesktopGameRunner embedding",
                                               "package scripts",
                                               "validation recipes",
                                               "arbitrary shell",
                                               "renderer/RHI uploads",
                                               "renderer/RHI handles",
                                               "package streaming",
                                               "stable third-party ABI",
                                               "broad editor productization",
                                           }));
}

MK_TEST("editor game module driver load ui model keeps retained rows") {
    mirakana::editor::EditorGameModuleDriverLoadDesc desc;
    desc.id = "reviewed_driver";
    desc.label = "Reviewed Driver";
    desc.module_path = (std::filesystem::current_path() / "game_driver.dll").string();

    const auto model = mirakana::editor::make_editor_game_module_driver_load_model(desc);
    MK_REQUIRE(model.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
    MK_REQUIRE(model.can_load);
    MK_REQUIRE(model.abi_contract == "GameEngine.EditorGameModuleDriver.v1");
    MK_REQUIRE(model.factory_symbol == mirakana::editor::editor_game_module_driver_factory_symbol_v1);

    const auto ui = mirakana::editor::make_editor_game_module_driver_load_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.reviewed_driver.status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.reviewed_driver.module_path"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.reviewed_driver.factory_symbol"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.reviewed_driver.diagnostic"}) !=
               nullptr);
}

MK_TEST("editor game module driver reload model reviews stopped safe reload") {
    mirakana::editor::EditorGameModuleDriverReloadDesc desc;
    desc.id = "reviewed_driver";
    desc.label = "Reviewed Driver";
    desc.module_path = (std::filesystem::current_path() / "game_driver.dll").string();
    desc.driver_loaded = true;

    const auto model = mirakana::editor::make_editor_game_module_driver_reload_model(desc);
    MK_REQUIRE(model.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
    MK_REQUIRE(model.can_reload);
    MK_REQUIRE(model.blocked_by.empty());
    MK_REQUIRE(model.unsupported_claims.empty());
    MK_REQUIRE(model.factory_symbol == mirakana::editor::editor_game_module_driver_factory_symbol_v1);
    MK_REQUIRE(model.abi_contract == "GameEngine.EditorGameModuleDriver.v1");

    const auto ui = mirakana::editor::make_editor_game_module_driver_reload_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.reload"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.reload.reviewed_driver.status"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.reload.reviewed_driver.module_path"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.reload.reviewed_driver.diagnostic"}) != nullptr);

    auto missing_driver = desc;
    missing_driver.driver_loaded = false;
    const auto missing_driver_model = mirakana::editor::make_editor_game_module_driver_reload_model(missing_driver);
    MK_REQUIRE(missing_driver_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!missing_driver_model.can_reload);
    MK_REQUIRE(std::ranges::find(missing_driver_model.blocked_by, "loaded-driver-required") !=
               missing_driver_model.blocked_by.end());

    auto active_session = desc;
    active_session.play_session_active = true;
    const auto active_session_model = mirakana::editor::make_editor_game_module_driver_reload_model(active_session);
    MK_REQUIRE(active_session_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!active_session_model.can_reload);
    MK_REQUIRE(std::ranges::find(active_session_model.blocked_by, "play-session-active") !=
               active_session_model.blocked_by.end());

    auto unsafe_path = desc;
    unsafe_path.module_path = "runtime/game_driver.dll";
    const auto unsafe_path_model = mirakana::editor::make_editor_game_module_driver_reload_model(unsafe_path);
    MK_REQUIRE(unsafe_path_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(std::ranges::find(unsafe_path_model.blocked_by, "absolute-module-path-required") !=
               unsafe_path_model.blocked_by.end());

    auto unsafe_symbol = desc;
    unsafe_symbol.factory_symbol = "bad symbol";
    const auto unsafe_symbol_model = mirakana::editor::make_editor_game_module_driver_reload_model(unsafe_symbol);
    MK_REQUIRE(unsafe_symbol_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(std::ranges::find(unsafe_symbol_model.blocked_by, "unsafe-factory-symbol") !=
               unsafe_symbol_model.blocked_by.end());

    auto unsupported = desc;
    unsupported.request_hot_reload = true;
    unsupported.request_active_session_reload = true;
    unsupported.request_desktop_game_runner_embedding = true;
    unsupported.request_package_script_execution = true;
    unsupported.request_validation_recipe_execution = true;
    unsupported.request_arbitrary_shell_execution = true;
    unsupported.request_renderer_rhi_uploads = true;
    unsupported.request_renderer_rhi_handle_exposure = true;
    unsupported.request_package_streaming = true;
    unsupported.request_stable_third_party_abi = true;
    unsupported.request_broad_editor_productization = true;
    const auto unsupported_model = mirakana::editor::make_editor_game_module_driver_reload_model(unsupported);
    MK_REQUIRE(unsupported_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!unsupported_model.can_reload);
    MK_REQUIRE(unsupported_model.unsupported_claims == (std::vector<std::string>{
                                                           "hot reload",
                                                           "DesktopGameRunner embedding",
                                                           "package scripts",
                                                           "validation recipes",
                                                           "arbitrary shell",
                                                           "renderer/RHI uploads",
                                                           "renderer/RHI handles",
                                                           "package streaming",
                                                           "stable third-party ABI",
                                                           "broad editor productization",
                                                           "active-session reload",
                                                       }));
}

MK_TEST("editor game module driver unload model gates loaded driver and active play session") {
    mirakana::editor::EditorGameModuleDriverUnloadDesc ready{};
    ready.id = "reviewed_driver";
    ready.label = "Unload";
    ready.driver_loaded = true;
    ready.play_session_active = false;
    const auto ok = mirakana::editor::make_editor_game_module_driver_unload_model(ready);
    MK_REQUIRE(ok.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
    MK_REQUIRE(ok.can_unload);

    auto missing = ready;
    missing.driver_loaded = false;
    const auto no_driver = mirakana::editor::make_editor_game_module_driver_unload_model(missing);
    MK_REQUIRE(no_driver.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!no_driver.can_unload);
    MK_REQUIRE(std::ranges::find(no_driver.blocked_by, "no-loaded-driver") != no_driver.blocked_by.end());

    auto active = ready;
    active.play_session_active = true;
    const auto blocked = mirakana::editor::make_editor_game_module_driver_unload_model(active);
    MK_REQUIRE(blocked.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!blocked.can_unload);
    MK_REQUIRE(std::ranges::find(blocked.blocked_by, "play-session-active") != blocked.blocked_by.end());

    const auto ui = mirakana::editor::make_editor_game_module_driver_unload_ui_model(ok);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.unload"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.unload.reviewed_driver.status"}) !=
               nullptr);
}

MK_TEST("editor game module driver host session snapshot classifies play and residency phases") {
    const auto idle = mirakana::editor::make_editor_game_module_driver_host_session_snapshot(false, false);
    MK_REQUIRE(idle.phase == mirakana::editor::EditorGameModuleDriverHostSessionPhase::idle_no_driver_play_stopped);
    MK_REQUIRE(idle.phase_id == "idle_no_driver_play_stopped");
    MK_REQUIRE(!idle.play_session_active);
    MK_REQUIRE(!idle.driver_loaded);

    MK_REQUIRE(idle.policy_dll_mutation_order_guidance == "phase_idle_no_driver_order_review_load_then_load_library");

    const auto resident = mirakana::editor::make_editor_game_module_driver_host_session_snapshot(false, true);
    MK_REQUIRE(resident.phase ==
               mirakana::editor::EditorGameModuleDriverHostSessionPhase::driver_resident_play_stopped);
    MK_REQUIRE(resident.phase_id == "driver_resident_play_stopped");
    MK_REQUIRE(resident.policy_dll_mutation_order_guidance ==
               "phase_driver_stopped_order_unload_or_stopped_state_reload_only");

    const auto play_only = mirakana::editor::make_editor_game_module_driver_host_session_snapshot(true, false);
    MK_REQUIRE(play_only.phase == mirakana::editor::EditorGameModuleDriverHostSessionPhase::play_active_without_driver);
    MK_REQUIRE(play_only.phase_id == "play_active_without_driver");
    MK_REQUIRE(play_only.policy_dll_mutation_order_guidance ==
               "phase_play_no_driver_order_stop_play_before_any_dll_change");

    const auto play_driver = mirakana::editor::make_editor_game_module_driver_host_session_snapshot(true, true);
    MK_REQUIRE(play_driver.phase == mirakana::editor::EditorGameModuleDriverHostSessionPhase::play_active_with_driver);
    MK_REQUIRE(play_driver.phase_id == "play_active_with_driver");
    MK_REQUIRE(play_driver.policy_dll_mutation_order_guidance ==
               "phase_play_with_driver_order_stop_play_unload_driver_free_module_then_reload");
    MK_REQUIRE(play_driver.barrier_play_dll_surface_mutation_status == "enforced_block_load_unload_reload");
    MK_REQUIRE(play_driver.policy_active_session_hot_reload == "unsupported_no_silent_dll_replacement_mid_play");
    MK_REQUIRE(play_driver.policy_stopped_state_reload_scope ==
               "reload_and_duplicate_load_reviews_require_play_stopped_explicit_paths");

    const auto ui = mirakana::editor::make_editor_game_module_driver_host_session_ui_model(idle);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.session"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.session.contract_label"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.session.phase_id"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.session.summary"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.session.play_session_active"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.session.driver_loaded"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.session.barriers_contract_label"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.session.barrier.play_dll_surface_mutation.status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.session.policy.active_session_hot_reload"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.session.policy.stopped_state_reload_scope"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.session.policy.dll_mutation_order_guidance"}) != nullptr);
    MK_REQUIRE(mirakana::editor::editor_game_module_driver_host_session_contract_v1 ==
               "ge.editor.editor_game_module_driver_host_session.v1");
    MK_REQUIRE(mirakana::editor::editor_game_module_driver_host_session_dll_barriers_contract_v1 ==
               "ge.editor.editor_game_module_driver_host_session_dll_barriers.v1");

    const auto ui_play = mirakana::editor::make_editor_game_module_driver_host_session_ui_model(play_driver);
    MK_REQUIRE(ui_play.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.session.barrier.play_dll_surface_mutation.status"}) != nullptr);
}

MK_TEST("editor game module driver load reload unload models block dll mutation while play session active") {
    const auto abs_path = (std::filesystem::current_path() / "game_driver.dll").string();

    mirakana::editor::EditorGameModuleDriverLoadDesc load;
    load.id = "reviewed_driver";
    load.module_path = abs_path;
    load.play_session_active = true;
    load.driver_already_loaded = true;
    const auto load_model = mirakana::editor::make_editor_game_module_driver_load_model(load);
    MK_REQUIRE(load_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!load_model.can_load);
    MK_REQUIRE(std::ranges::find(load_model.blocked_by, "play-session-active") != load_model.blocked_by.end());
    MK_REQUIRE(std::ranges::find(load_model.blocked_by, "driver-already-loaded") != load_model.blocked_by.end());

    mirakana::editor::EditorGameModuleDriverReloadDesc reload;
    reload.id = "reviewed_reload";
    reload.module_path = abs_path;
    reload.driver_loaded = true;
    reload.play_session_active = true;
    const auto reload_model = mirakana::editor::make_editor_game_module_driver_reload_model(reload);
    MK_REQUIRE(reload_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!reload_model.can_reload);
    MK_REQUIRE(std::ranges::find(reload_model.blocked_by, "play-session-active") != reload_model.blocked_by.end());

    mirakana::editor::EditorGameModuleDriverUnloadDesc unload;
    unload.id = "reviewed_unload";
    unload.driver_loaded = true;
    unload.play_session_active = true;
    const auto unload_model = mirakana::editor::make_editor_game_module_driver_unload_model(unload);
    MK_REQUIRE(unload_model.status == mirakana::editor::EditorGameModuleDriverStatus::blocked);
    MK_REQUIRE(!unload_model.can_unload);
    MK_REQUIRE(std::ranges::find(unload_model.blocked_by, "play-session-active") != unload_model.blocked_by.end());
}

MK_TEST("editor game module driver contract metadata documents same engine ABI boundary") {
    const auto model = mirakana::editor::make_editor_game_module_driver_contract_metadata_model();
    MK_REQUIRE(model.id == "editor_game_module_driver_contract_v1");
    MK_REQUIRE(model.abi_contract == mirakana::editor::editor_game_module_driver_abi_name_v1);
    MK_REQUIRE(model.abi_version == mirakana::editor::editor_game_module_driver_abi_version_v1);
    MK_REQUIRE(model.factory_symbol == mirakana::editor::editor_game_module_driver_factory_symbol_v1);
    MK_REQUIRE(model.same_engine_build_required);
    MK_REQUIRE(!model.stable_third_party_abi_supported);
    MK_REQUIRE(!model.hot_reload_supported);
    MK_REQUIRE(!model.active_session_reload_supported);
    MK_REQUIRE(!model.desktop_game_runner_embedding_supported);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);

    const auto find_row =
        [&](std::string_view id) -> const mirakana::editor::EditorGameModuleDriverContractMetadataRow* {
        const auto it = std::ranges::find_if(model.rows, [id](const auto& row) { return row.id == id; });
        return it == model.rows.end() ? nullptr : &*it;
    };

    const auto* abi = find_row("abi.name");
    MK_REQUIRE(abi != nullptr);
    MK_REQUIRE(abi->required);
    MK_REQUIRE(abi->supported);
    MK_REQUIRE(abi->value == "GameEngine.EditorGameModuleDriver.v1");

    const auto* tick = find_row("callback.tick");
    MK_REQUIRE(tick != nullptr);
    MK_REQUIRE(tick->required);
    MK_REQUIRE(tick->supported);
    MK_REQUIRE(tick->value == "required");

    const auto* destroy = find_row("callback.destroy");
    MK_REQUIRE(destroy != nullptr);
    MK_REQUIRE(destroy->required);
    MK_REQUIRE(destroy->supported);
    MK_REQUIRE(destroy->value == "required");

    const auto* begin = find_row("callback.begin");
    MK_REQUIRE(begin != nullptr);
    MK_REQUIRE(!begin->required);
    MK_REQUIRE(begin->supported);
    MK_REQUIRE(begin->value == "optional");

    const auto* stable_abi = find_row("compatibility.stable_third_party_abi");
    MK_REQUIRE(stable_abi != nullptr);
    MK_REQUIRE(!stable_abi->required);
    MK_REQUIRE(!stable_abi->supported);
    MK_REQUIRE(stable_abi->value == "unsupported");
    MK_REQUIRE(stable_abi->diagnostic.contains("same-engine-build"));

    const auto* active_reload = find_row("reload.active_session");
    MK_REQUIRE(active_reload != nullptr);
    MK_REQUIRE(!active_reload->supported);
    MK_REQUIRE(active_reload->value == "unsupported");

    const auto ui = mirakana::editor::make_editor_game_module_driver_contract_metadata_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.contract"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.contract.abi.name.value"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.contract.callback.tick.value"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.contract.compatibility.stable_third_party_abi.value"}) !=
               nullptr);
}

MK_TEST("editor game module driver ctest probe evidence ui exposes retained ids") {
    const auto model = mirakana::editor::make_editor_game_module_driver_ctest_probe_evidence_model();
    MK_REQUIRE(model.id == "editor_game_module_driver_ctest_probe_evidence_v1");
    MK_REQUIRE(model.probe_shared_library_target == "MK_editor_game_module_driver_probe");
    MK_REQUIRE(model.ctest_executable_target == "MK_editor_game_module_driver_load_tests");
    MK_REQUIRE(model.factory_symbol == mirakana::editor::editor_game_module_driver_factory_symbol_v1);

    const auto ui = mirakana::editor::make_editor_game_module_driver_ctest_probe_evidence_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.game_module_driver.ctest_probe_evidence"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.ctest_probe_evidence.probe_shared_library_target"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.ctest_probe_evidence.ctest_executable_target"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.ctest_probe_evidence.factory_symbol"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.ctest_probe_evidence.host_scope_note"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.ctest_probe_evidence.editor_boundary_note"}) != nullptr);
}

MK_TEST("editor game module driver reload transaction recipe evidence ui exposes retained ids") {
    const auto model = mirakana::editor::make_editor_game_module_driver_reload_transaction_recipe_evidence_model();
    MK_REQUIRE(model.id == "editor_game_module_driver_reload_transaction_recipe_evidence_v1");
    MK_REQUIRE(model.validation_recipe_id == "dev-windows-editor-game-module-driver-load-tests");
    MK_REQUIRE(model.host_gate_acknowledgement_id == "windows-msvc-dev-editor-game-module-driver-ctest");
    MK_REQUIRE(mirakana::editor::editor_game_module_driver_reload_transaction_recipe_evidence_contract_v1() ==
               "ge.editor.editor_game_module_driver_reload_transaction_recipe_evidence.v1");

    const auto ui = mirakana::editor::make_editor_game_module_driver_reload_transaction_recipe_evidence_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.reload_transaction_recipe_evidence"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.contract_label"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.validation_recipe_id"}) !=
               nullptr);
    MK_REQUIRE(
        ui.find(mirakana::ui::ElementId{
            "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.host_gate_acknowledgement_id"}) !=
        nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.reviewed_dry_run_command"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.reviewed_execute_command"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.editor_boundary_note"}) !=
               nullptr);
}

MK_TEST("editor play in editor gameplay driver rejects paused and invalid ticks") {
    mirakana::Scene scene("Editor Scene");
    (void)scene.create_node("Player");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");

    RecordingEditorPlaySessionDriver driver;
    mirakana::editor::EditorPlaySession session;
    MK_REQUIRE(session.begin(document, driver) == mirakana::editor::EditorPlaySessionActionStatus::applied);

    MK_REQUIRE(session.tick(0.0) == mirakana::editor::EditorPlaySessionActionStatus::rejected_invalid_delta);
    MK_REQUIRE(session.tick(-0.25) == mirakana::editor::EditorPlaySessionActionStatus::rejected_invalid_delta);
    MK_REQUIRE(session.tick(std::numeric_limits<double>::quiet_NaN()) ==
               mirakana::editor::EditorPlaySessionActionStatus::rejected_invalid_delta);
    MK_REQUIRE(driver.tick_count() == 0);
    MK_REQUIRE(session.simulation_frame_count() == 0);

    MK_REQUIRE(session.pause() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(session.tick(1.0 / 60.0) == mirakana::editor::EditorPlaySessionActionStatus::rejected_wrong_state);
    MK_REQUIRE(driver.tick_count() == 0);
    MK_REQUIRE(session.resume() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(session.tick(1.0 / 60.0) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(driver.tick_count() == 1);

    MK_REQUIRE(mirakana::editor::editor_play_session_action_status_label(
                   mirakana::editor::EditorPlaySessionActionStatus::rejected_invalid_delta) == "RejectedInvalidDelta");

    MK_REQUIRE(session.stop() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    const auto inactive_report = mirakana::editor::make_editor_play_session_report(session, document);
    MK_REQUIRE(!inactive_report.gameplay_driver_attached);
    MK_REQUIRE(inactive_report.last_delta_seconds == 0.0);
}

[[nodiscard]] const mirakana::editor::EditorPlaySessionControlRow*
find_play_session_control(const mirakana::editor::EditorPlaySessionControlsModel& model,
                          mirakana::editor::EditorPlaySessionControlCommand command) noexcept {
    const auto it = std::ranges::find_if(model.controls, [command](const auto& row) { return row.command == command; });
    return it == model.controls.end() ? nullptr : &*it;
}

MK_TEST("editor play in editor controls model exposes edit and empty source controls") {
    mirakana::Scene scene("Editor Scene");
    (void)scene.create_node("Player");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");

    mirakana::editor::EditorPlaySession session;
    const auto model = mirakana::editor::make_editor_play_session_controls_model(session, document);

    MK_REQUIRE(model.report.state == mirakana::editor::EditorPlaySessionState::edit);
    MK_REQUIRE(!model.viewport_uses_simulation_scene);
    MK_REQUIRE(model.controls.size() == 4);

    const auto* play = find_play_session_control(model, mirakana::editor::EditorPlaySessionControlCommand::play);
    const auto* pause = find_play_session_control(model, mirakana::editor::EditorPlaySessionControlCommand::pause);
    const auto* resume = find_play_session_control(model, mirakana::editor::EditorPlaySessionControlCommand::resume);
    const auto* stop = find_play_session_control(model, mirakana::editor::EditorPlaySessionControlCommand::stop);
    MK_REQUIRE(play != nullptr);
    MK_REQUIRE(pause != nullptr);
    MK_REQUIRE(resume != nullptr);
    MK_REQUIRE(stop != nullptr);
    MK_REQUIRE(play->id == "play");
    MK_REQUIRE(play->label == "Play");
    MK_REQUIRE(play->enabled);
    MK_REQUIRE(!pause->enabled);
    MK_REQUIRE(!resume->enabled);
    MK_REQUIRE(!stop->enabled);

    mirakana::Scene empty_scene("Empty");
    auto empty_document = mirakana::editor::SceneAuthoringDocument::from_scene(empty_scene, "scenes/empty.scene");
    const auto empty_model = mirakana::editor::make_editor_play_session_controls_model(session, empty_document);
    const auto* empty_play =
        find_play_session_control(empty_model, mirakana::editor::EditorPlaySessionControlCommand::play);
    MK_REQUIRE(empty_play != nullptr);
    MK_REQUIRE(!empty_play->enabled);
}

MK_TEST("editor play in editor controls model exposes play paused and stopped controls") {
    mirakana::Scene scene("Editor Scene");
    (void)scene.create_node("Player");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");

    mirakana::editor::EditorPlaySession session;
    MK_REQUIRE(session.begin(document) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    const auto play_model = mirakana::editor::make_editor_play_session_controls_model(session, document);
    MK_REQUIRE(play_model.report.state == mirakana::editor::EditorPlaySessionState::play);
    MK_REQUIRE(play_model.viewport_uses_simulation_scene);
    MK_REQUIRE(
        !find_play_session_control(play_model, mirakana::editor::EditorPlaySessionControlCommand::play)->enabled);
    MK_REQUIRE(
        find_play_session_control(play_model, mirakana::editor::EditorPlaySessionControlCommand::pause)->enabled);
    MK_REQUIRE(
        !find_play_session_control(play_model, mirakana::editor::EditorPlaySessionControlCommand::resume)->enabled);
    MK_REQUIRE(find_play_session_control(play_model, mirakana::editor::EditorPlaySessionControlCommand::stop)->enabled);

    MK_REQUIRE(session.pause() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    const auto paused_model = mirakana::editor::make_editor_play_session_controls_model(session, document);
    MK_REQUIRE(paused_model.report.state == mirakana::editor::EditorPlaySessionState::paused);
    MK_REQUIRE(paused_model.viewport_uses_simulation_scene);
    MK_REQUIRE(
        !find_play_session_control(paused_model, mirakana::editor::EditorPlaySessionControlCommand::play)->enabled);
    MK_REQUIRE(
        !find_play_session_control(paused_model, mirakana::editor::EditorPlaySessionControlCommand::pause)->enabled);
    MK_REQUIRE(
        find_play_session_control(paused_model, mirakana::editor::EditorPlaySessionControlCommand::resume)->enabled);
    MK_REQUIRE(
        find_play_session_control(paused_model, mirakana::editor::EditorPlaySessionControlCommand::stop)->enabled);

    MK_REQUIRE(session.stop() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    const auto stopped_model = mirakana::editor::make_editor_play_session_controls_model(session, document);
    MK_REQUIRE(stopped_model.report.state == mirakana::editor::EditorPlaySessionState::edit);
    MK_REQUIRE(!stopped_model.viewport_uses_simulation_scene);
    MK_REQUIRE(
        find_play_session_control(stopped_model, mirakana::editor::EditorPlaySessionControlCommand::play)->enabled);
    MK_REQUIRE(
        !find_play_session_control(stopped_model, mirakana::editor::EditorPlaySessionControlCommand::pause)->enabled);
    MK_REQUIRE(
        !find_play_session_control(stopped_model, mirakana::editor::EditorPlaySessionControlCommand::resume)->enabled);
    MK_REQUIRE(
        !find_play_session_control(stopped_model, mirakana::editor::EditorPlaySessionControlCommand::stop)->enabled);
}

MK_TEST("editor runtime host playtest launch reviews safe external process commands") {
    mirakana::editor::EditorRuntimeHostPlaytestLaunchDesc desc;
    desc.id = "sample_desktop_runtime_game";
    desc.label = "Sample Desktop Runtime Game";
    desc.working_directory = ".";
    desc.argv = {
        "out/install/desktop-runtime-release/bin/sample_desktop_runtime_game.exe",
        "--smoke",
        "--require-config",
        "runtime/sample_desktop_runtime_game.config",
        "--require-scene-package",
        "runtime/sample_desktop_runtime_game.geindex",
    };

    const auto model = mirakana::editor::make_editor_runtime_host_playtest_launch_model(desc);
    MK_REQUIRE(model.status == mirakana::editor::EditorRuntimeHostPlaytestLaunchStatus::ready);
    MK_REQUIRE(model.status_label == "ready");
    MK_REQUIRE(model.can_execute);
    MK_REQUIRE(!model.host_gate_acknowledgement_required);
    MK_REQUIRE(model.command.executable == "out/install/desktop-runtime-release/bin/sample_desktop_runtime_game.exe");
    MK_REQUIRE(model.command.arguments.size() == 5);
    MK_REQUIRE(model.command.arguments[0] == "--smoke");
    MK_REQUIRE(model.command.arguments[4] == "runtime/sample_desktop_runtime_game.geindex");
    MK_REQUIRE(model.command.working_directory == ".");

    const auto ui = mirakana::editor::make_editor_runtime_host_playtest_launch_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.runtime_host"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.runtime_host.sample_desktop_runtime_game.status"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.runtime_host.sample_desktop_runtime_game.command"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.runtime_host.sample_desktop_runtime_game.diagnostic"}) !=
               nullptr);

    auto host_gated_desc = desc;
    host_gated_desc.host_gates = {"d3d12-windows-primary"};
    const auto host_gated = mirakana::editor::make_editor_runtime_host_playtest_launch_model(host_gated_desc);
    MK_REQUIRE(host_gated.status == mirakana::editor::EditorRuntimeHostPlaytestLaunchStatus::host_gated);
    MK_REQUIRE(!host_gated.can_execute);
    MK_REQUIRE(host_gated.host_gate_acknowledgement_required);
    MK_REQUIRE(!host_gated.host_gates_acknowledged);

    host_gated_desc.acknowledge_host_gates = true;
    host_gated_desc.acknowledged_host_gates = {"d3d12-windows-primary"};
    const auto acknowledged = mirakana::editor::make_editor_runtime_host_playtest_launch_model(host_gated_desc);
    MK_REQUIRE(acknowledged.status == mirakana::editor::EditorRuntimeHostPlaytestLaunchStatus::ready);
    MK_REQUIRE(acknowledged.can_execute);
    MK_REQUIRE(acknowledged.host_gates_acknowledged);

    auto blocked_desc = desc;
    blocked_desc.request_dynamic_game_module_loading = true;
    blocked_desc.request_editor_core_execution = true;
    blocked_desc.request_package_script_execution = true;
    blocked_desc.request_raw_manifest_command_evaluation = true;
    blocked_desc.request_renderer_rhi_handle_exposure = true;
    const auto blocked = mirakana::editor::make_editor_runtime_host_playtest_launch_model(blocked_desc);
    const auto has_diagnostic = [&blocked](std::string_view text) {
        return std::ranges::any_of(blocked.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };
    MK_REQUIRE(blocked.status == mirakana::editor::EditorRuntimeHostPlaytestLaunchStatus::blocked);
    MK_REQUIRE(!blocked.can_execute);
    MK_REQUIRE(has_diagnostic("dynamic game module"));
    MK_REQUIRE(has_diagnostic("editor core"));
    MK_REQUIRE(has_diagnostic("package script"));
    MK_REQUIRE(has_diagnostic("raw manifest"));
    MK_REQUIRE(has_diagnostic("renderer/RHI handle"));

    auto unsafe_desc = desc;
    unsafe_desc.argv[1] = "bad\narg";
    const auto unsafe = mirakana::editor::make_editor_runtime_host_playtest_launch_model(unsafe_desc);
    MK_REQUIRE(unsafe.status == mirakana::editor::EditorRuntimeHostPlaytestLaunchStatus::blocked);
    MK_REQUIRE(!unsafe.can_execute);
    MK_REQUIRE(std::ranges::find(unsafe.blocked_by, "unsafe-argv-token") != unsafe.blocked_by.end());

    auto empty_desc = desc;
    empty_desc.argv.clear();
    const auto empty = mirakana::editor::make_editor_runtime_host_playtest_launch_model(empty_desc);
    MK_REQUIRE(empty.status == mirakana::editor::EditorRuntimeHostPlaytestLaunchStatus::blocked);
    MK_REQUIRE(std::ranges::find(empty.blocked_by, "missing-reviewed-argv") != empty.blocked_by.end());
}

MK_TEST("editor in process runtime host review starts linked gameplay driver sessions") {
    mirakana::Scene scene("Editor Scene");
    (void)scene.create_node("Player");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");

    mirakana::editor::EditorPlaySession session;
    mirakana::editor::EditorInProcessRuntimeHostDesc desc;
    desc.id = "linked_gameplay_driver";
    desc.label = "Linked Gameplay Driver";
    desc.linked_gameplay_driver_available = true;

    const auto model = mirakana::editor::make_editor_in_process_runtime_host_model(session, document, desc);
    MK_REQUIRE(model.status == mirakana::editor::EditorInProcessRuntimeHostStatus::ready);
    MK_REQUIRE(model.status_label == "ready");
    MK_REQUIRE(model.can_begin);
    MK_REQUIRE(!model.can_tick);
    MK_REQUIRE(!model.can_stop);
    MK_REQUIRE(model.source_scene_available);
    MK_REQUIRE(model.linked_gameplay_driver_available);
    MK_REQUIRE(!model.has_blocking_diagnostics);

    const auto ui = mirakana::editor::make_editor_in_process_runtime_host_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"play_in_editor.in_process_runtime_host"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.in_process_runtime_host.linked_gameplay_driver.status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.in_process_runtime_host.linked_gameplay_driver.driver"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "play_in_editor.in_process_runtime_host.linked_gameplay_driver.diagnostic"}) != nullptr);

    RecordingEditorPlaySessionDriver driver;
    const auto begin_result =
        mirakana::editor::begin_editor_in_process_runtime_host_session(session, document, driver, desc);
    MK_REQUIRE(begin_result.status == mirakana::editor::EditorInProcessRuntimeHostStatus::active);
    MK_REQUIRE(begin_result.action_status == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(begin_result.model.can_stop);
    MK_REQUIRE(begin_result.model.viewport_uses_simulation_scene);
    MK_REQUIRE(session.active());
    MK_REQUIRE(session.gameplay_driver_attached());
    MK_REQUIRE(driver.begin_count() == 1);

    MK_REQUIRE(session.tick(0.125) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(driver.tick_count() == 1);
    MK_REQUIRE(driver.tick_contexts()[0].delta_seconds == 0.125);

    const auto active_model = mirakana::editor::make_editor_in_process_runtime_host_model(session, document, desc);
    MK_REQUIRE(active_model.status == mirakana::editor::EditorInProcessRuntimeHostStatus::active);
    MK_REQUIRE(!active_model.can_begin);
    MK_REQUIRE(active_model.can_tick);
    MK_REQUIRE(active_model.can_stop);

    MK_REQUIRE(session.stop() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(driver.end_count() == 1);

    auto no_driver_desc = desc;
    no_driver_desc.linked_gameplay_driver_available = false;
    const auto no_driver =
        mirakana::editor::make_editor_in_process_runtime_host_model(session, document, no_driver_desc);
    MK_REQUIRE(no_driver.status == mirakana::editor::EditorInProcessRuntimeHostStatus::blocked);
    MK_REQUIRE(!no_driver.can_begin);
    MK_REQUIRE(std::ranges::find(no_driver.blocked_by, "missing-linked-gameplay-driver") != no_driver.blocked_by.end());

    auto unsupported_desc = desc;
    unsupported_desc.request_dynamic_game_module_loading = true;
    unsupported_desc.request_renderer_rhi_handle_exposure = true;
    unsupported_desc.request_package_script_execution = true;
    const auto unsupported =
        mirakana::editor::make_editor_in_process_runtime_host_model(session, document, unsupported_desc);
    const auto has_diagnostic = [&unsupported](std::string_view text) {
        return std::ranges::any_of(unsupported.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };
    MK_REQUIRE(unsupported.status == mirakana::editor::EditorInProcessRuntimeHostStatus::blocked);
    MK_REQUIRE(has_diagnostic("dynamic game module"));
    MK_REQUIRE(has_diagnostic("renderer/RHI handle"));
    MK_REQUIRE(has_diagnostic("package script"));
}

MK_TEST("editor scene transform draft applies absolute transform values") {
    mirakana::Scene scene("Editor Scene");
    const auto node = scene.create_node("Player");

    auto draft = mirakana::editor::make_scene_node_transform_draft(scene, node);
    MK_REQUIRE(draft.has_value());
    draft->position = mirakana::Vec3{.x = 2.0F, .y = 3.0F, .z = 4.0F};
    draft->rotation_radians = mirakana::Vec3{.x = 0.1F, .y = 0.2F, .z = 0.3F};
    draft->scale = mirakana::Vec3{.x = 2.0F, .y = 3.0F, .z = 4.0F};

    MK_REQUIRE(mirakana::editor::apply_scene_node_transform_draft(scene, *draft));

    const auto* edited = scene.find_node(node);
    MK_REQUIRE(edited != nullptr);
    MK_REQUIRE(edited->transform.position == (mirakana::Vec3{2.0F, 3.0F, 4.0F}));
    MK_REQUIRE(edited->transform.rotation_radians == (mirakana::Vec3{0.1F, 0.2F, 0.3F}));
    MK_REQUIRE(edited->transform.scale == (mirakana::Vec3{2.0F, 3.0F, 4.0F}));
}

MK_TEST("editor scene transform draft rejects missing nodes and nonpositive scale") {
    mirakana::Scene scene("Editor Scene");
    const auto node = scene.create_node("Player");

    auto draft = mirakana::editor::make_scene_node_transform_draft(scene, node);
    MK_REQUIRE(draft.has_value());
    draft->scale = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 1.0F};
    MK_REQUIRE(!mirakana::editor::apply_scene_node_transform_draft(scene, *draft));
    MK_REQUIRE(scene.find_node(node)->transform.scale == (mirakana::Vec3{1.0F, 1.0F, 1.0F}));

    MK_REQUIRE(!mirakana::editor::make_scene_node_transform_draft(scene, mirakana::SceneNodeId{99}).has_value());
}

MK_TEST("editor viewport transform edits selected nodes by active tool") {
    mirakana::Scene scene("Editor Scene");
    const auto node = scene.create_node("Player");

    MK_REQUIRE(mirakana::editor::apply_viewport_transform_edit(
        scene, mirakana::editor::ViewportTransformEdit{node, mirakana::editor::ViewportTool::translate,
                                                       mirakana::Vec3{2.0F, 3.0F, 4.0F}}));
    MK_REQUIRE(scene.find_node(node)->transform.position == (mirakana::Vec3{2.0F, 3.0F, 4.0F}));

    MK_REQUIRE(mirakana::editor::apply_viewport_transform_edit(
        scene, mirakana::editor::ViewportTransformEdit{node, mirakana::editor::ViewportTool::rotate,
                                                       mirakana::Vec3{0.1F, 0.2F, 0.3F}}));
    MK_REQUIRE(scene.find_node(node)->transform.rotation_radians == (mirakana::Vec3{0.1F, 0.2F, 0.3F}));

    MK_REQUIRE(mirakana::editor::apply_viewport_transform_edit(
        scene, mirakana::editor::ViewportTransformEdit{node, mirakana::editor::ViewportTool::scale,
                                                       mirakana::Vec3{1.0F, 2.0F, 3.0F}}));
    MK_REQUIRE(scene.find_node(node)->transform.scale == (mirakana::Vec3{2.0F, 3.0F, 4.0F}));
}

MK_TEST("editor viewport transform edits ignore select and reject invalid scale deltas") {
    mirakana::Scene scene("Editor Scene");
    const auto node = scene.create_node("Player");

    MK_REQUIRE(!mirakana::editor::apply_viewport_transform_edit(
        scene, mirakana::editor::ViewportTransformEdit{node, mirakana::editor::ViewportTool::select,
                                                       mirakana::Vec3{1.0F, 0.0F, 0.0F}}));
    MK_REQUIRE(scene.find_node(node)->transform.position == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));

    MK_REQUIRE(!mirakana::editor::apply_viewport_transform_edit(
        scene, mirakana::editor::ViewportTransformEdit{node, mirakana::editor::ViewportTool::scale,
                                                       mirakana::Vec3{-1.0F, 0.0F, 0.0F}}));
    MK_REQUIRE(scene.find_node(node)->transform.scale == (mirakana::Vec3{1.0F, 1.0F, 1.0F}));
}

MK_TEST("editor scene component draft applies camera light and mesh renderer components") {
    mirakana::Scene scene("Editor Scene");
    const auto node = scene.create_node("Player");

    auto draft = mirakana::editor::make_scene_node_component_draft(scene, node);
    MK_REQUIRE(draft.has_value());
    draft->components.camera = mirakana::CameraComponent{
        .projection = mirakana::CameraProjectionMode::orthographic,
        .vertical_fov_radians = 1.0F,
        .orthographic_height = 12.0F,
        .near_plane = 0.1F,
        .far_plane = 250.0F,
        .primary = true,
    };
    draft->components.light = mirakana::LightComponent{
        .type = mirakana::LightType::point,
        .color = mirakana::Vec3{.x = 1.0F, .y = 0.9F, .z = 0.8F},
        .intensity = 3.0F,
        .range = 20.0F,
        .inner_cone_radians = 0.0F,
        .outer_cone_radians = 0.0F,
        .casts_shadows = false,
    };
    draft->components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    draft->components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId::from_name("sprites/player"),
        .material = mirakana::AssetId::from_name("materials/sprite"),
        .size = mirakana::Vec2{.x = 2.0F, .y = 3.0F},
        .tint = {0.4F, 0.6F, 0.8F, 1.0F},
        .visible = true,
    };

    MK_REQUIRE(mirakana::editor::apply_scene_node_component_draft(scene, *draft));

    const auto* edited = scene.find_node(node);
    MK_REQUIRE(edited != nullptr);
    MK_REQUIRE(edited->components.camera.has_value());
    MK_REQUIRE(edited->components.camera->projection == mirakana::CameraProjectionMode::orthographic);
    MK_REQUIRE(edited->components.light.has_value());
    MK_REQUIRE(edited->components.light->type == mirakana::LightType::point);
    MK_REQUIRE(edited->components.mesh_renderer.has_value());
    MK_REQUIRE(edited->components.mesh_renderer->visible);
    MK_REQUIRE(edited->components.sprite_renderer.has_value());
    MK_REQUIRE(edited->components.sprite_renderer->size == (mirakana::Vec2{2.0F, 3.0F}));
}

MK_TEST("editor scene component draft removes components and rejects invalid values without mutation") {
    mirakana::Scene scene("Editor Scene");
    const auto node = scene.create_node("Player");
    mirakana::SceneNodeComponents components;
    components.camera = mirakana::CameraComponent{};
    components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    scene.set_components(node, components);

    auto draft = mirakana::editor::make_scene_node_component_draft(scene, node);
    MK_REQUIRE(draft.has_value());
    draft->components.camera.reset();
    MK_REQUIRE(mirakana::editor::apply_scene_node_component_draft(scene, *draft));
    MK_REQUIRE(!scene.find_node(node)->components.camera.has_value());
    MK_REQUIRE(scene.find_node(node)->components.mesh_renderer.has_value());

    auto invalid = mirakana::editor::make_scene_node_component_draft(scene, node);
    MK_REQUIRE(invalid.has_value());
    invalid->components.mesh_renderer = mirakana::MeshRendererComponent{};
    MK_REQUIRE(!mirakana::editor::apply_scene_node_component_draft(scene, *invalid));
    MK_REQUIRE(scene.find_node(node)->components.mesh_renderer.has_value());

    MK_REQUIRE(!mirakana::editor::make_scene_node_component_draft(scene, mirakana::SceneNodeId{99}).has_value());
}

MK_TEST("editor scene authoring document edits hierarchy with undoable actions") {
    mirakana::Scene scene("Authoring Scene");
    const auto root = scene.create_node("Root");
    const auto child = scene.create_node("Child");
    scene.set_parent(child, root);

    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/level.scene");
    MK_REQUIRE(document.scene_path() == "assets/scenes/level.scene");
    MK_REQUIRE(!document.dirty());

    auto rows = document.hierarchy_rows();
    MK_REQUIRE(rows.size() == 2);
    MK_REQUIRE(rows[0].node == root);
    MK_REQUIRE(rows[0].depth == 0);
    MK_REQUIRE(rows[1].node == child);
    MK_REQUIRE(rows[1].depth == 1);

    MK_REQUIRE(document.select_node(child));
    MK_REQUIRE(document.selected_node() == child);
    rows = document.hierarchy_rows();
    MK_REQUIRE(rows[1].selected);

    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_rename_node_action(document, child, "Weapon")));
    MK_REQUIRE(document.scene().find_node(child)->name == "Weapon");
    MK_REQUIRE(document.dirty());
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().find_node(child)->name == "Child");
    MK_REQUIRE(!document.dirty());

    MK_REQUIRE(
        history.execute(mirakana::editor::make_scene_authoring_create_node_action(document, "SpawnPoint", root)));
    MK_REQUIRE(document.scene().nodes().size() == 3);
    MK_REQUIRE(document.scene().nodes().back().name == "SpawnPoint");
    MK_REQUIRE(document.scene().nodes().back().parent == root);
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().nodes().size() == 2);

    MK_REQUIRE(
        history.execute(mirakana::editor::make_scene_authoring_duplicate_subtree_action(document, root, "RootCopy")));
    MK_REQUIRE(document.scene().nodes().size() == 4);
    MK_REQUIRE(document.scene().nodes()[2].name == "RootCopy");
    MK_REQUIRE(document.scene().nodes()[3].parent == document.scene().nodes()[2].id);
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().nodes().size() == 2);

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_delete_node_action(document, child)));
    MK_REQUIRE(document.scene().nodes().size() == 1);
    MK_REQUIRE(document.selected_node() == mirakana::null_scene_node);
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().nodes().size() == 2);

    auto transform = mirakana::editor::make_scene_node_transform_draft(document.scene(), child);
    MK_REQUIRE(transform.has_value());
    transform->position = mirakana::Vec3{.x = 3.0F, .y = 4.0F, .z = 5.0F};
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_transform_edit_action(document, *transform)));
    MK_REQUIRE(document.scene().find_node(child)->transform.position == (mirakana::Vec3{3.0F, 4.0F, 5.0F}));
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().find_node(child)->transform.position == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));

    auto components = mirakana::editor::make_scene_node_component_draft(document.scene(), child);
    MK_REQUIRE(components.has_value());
    components->components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_component_edit_action(document, *components)));
    MK_REQUIRE(document.scene().find_node(child)->components.mesh_renderer.has_value());
    MK_REQUIRE(history.undo());
    MK_REQUIRE(!document.scene().find_node(child)->components.mesh_renderer.has_value());

    MK_REQUIRE(!history.execute(mirakana::editor::make_scene_authoring_rename_node_action(document, child, "")));
    MK_REQUIRE(!history.execute(mirakana::editor::make_scene_authoring_reparent_node_action(document, root, child)));
    MK_REQUIRE(document.scene().find_node(root)->parent == mirakana::null_scene_node);
}

MK_TEST("editor scene authoring reparent parent options exclude invalid targets") {
    MK_REQUIRE(mirakana::editor::make_scene_authoring_reparent_parent_options(
                   mirakana::editor::SceneAuthoringDocument::from_scene(mirakana::Scene("E"), "e.scene"),
                   mirakana::null_scene_node)
                   .empty());

    mirakana::Scene scene("T");
    const auto root = scene.create_node("Root");
    const auto child = scene.create_node("Child");
    scene.set_parent(child, root);
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/reparent.scene");
    MK_REQUIRE(document.select_node(child));
    MK_REQUIRE(mirakana::editor::make_scene_authoring_reparent_parent_options(document, child).empty());

    const auto sibling = scene.create_node("Sib");
    scene.set_parent(sibling, root);
    document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/reparent.scene");
    MK_REQUIRE(document.select_node(child));
    const auto opts = mirakana::editor::make_scene_authoring_reparent_parent_options(document, child);
    MK_REQUIRE(opts.size() == 1);
    MK_REQUIRE(opts.front().parent == sibling);

    mirakana::Scene chain("Chain");
    const auto r = chain.create_node("R");
    const auto a = chain.create_node("A");
    const auto b = chain.create_node("B");
    chain.set_parent(a, r);
    chain.set_parent(b, a);
    auto chain_doc = mirakana::editor::SceneAuthoringDocument::from_scene(chain, "assets/scenes/chain.scene");
    MK_REQUIRE(chain_doc.select_node(a));
    MK_REQUIRE(mirakana::editor::make_scene_authoring_reparent_parent_options(chain_doc, a).empty());
    MK_REQUIRE(chain_doc.select_node(b));
    const auto under_b = mirakana::editor::make_scene_authoring_reparent_parent_options(chain_doc, b);
    MK_REQUIRE(under_b.size() == 1);
    MK_REQUIRE(under_b.front().parent == r);

    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_reparent_node_action(chain_doc, b, r)));
    MK_REQUIRE(chain_doc.scene().find_node(b)->parent == r);
}

MK_TEST("editor scene authoring saves prefabs and instantiates them with undo") {
    mirakana::Scene scene("Prefab Authoring");
    const auto root = scene.create_node("Player");
    const auto child = scene.create_node("Weapon");
    scene.set_parent(child, root);
    scene.find_node(child)->components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/weapon"),
        .material = mirakana::AssetId::from_name("materials/weapon"),
        .visible = true,
    };

    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/level.scene");
    MK_REQUIRE(document.select_node(root));

    const auto prefab = mirakana::editor::build_prefab_from_selected_node(document, "player.prefab");
    MK_REQUIRE(prefab.has_value());
    MK_REQUIRE(prefab->nodes.size() == 2);

    mirakana::editor::MemoryTextStore store;
    mirakana::editor::save_prefab_authoring_document(store, "assets/prefabs/player.prefab", *prefab);
    const auto loaded = mirakana::editor::load_prefab_authoring_document(store, "assets/prefabs/player.prefab");
    MK_REQUIRE(loaded.name == "player.prefab");
    MK_REQUIRE(loaded.nodes.size() == 2);

    mirakana::Scene empty_scene("Empty");
    auto target = mirakana::editor::SceneAuthoringDocument::from_scene(empty_scene, "assets/scenes/empty.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(target, loaded)));
    MK_REQUIRE(target.scene().nodes().size() == 2);
    MK_REQUIRE(target.scene().nodes()[0].name == "Player");
    MK_REQUIRE(target.scene().nodes()[1].parent == target.scene().nodes()[0].id);
    MK_REQUIRE(history.undo());
    MK_REQUIRE(target.scene().nodes().empty());
}

MK_TEST("editor scene prefab source link model reviews instantiated prefab nodes") {
    mirakana::PrefabDefinition prefab;
    prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Player";
    prefab.nodes.push_back(root);
    mirakana::PrefabNodeTemplate child;
    child.name = "Weapon";
    child.parent_index = 1;
    prefab.nodes.push_back(child);

    mirakana::Scene empty_scene("Empty");
    auto target = mirakana::editor::SceneAuthoringDocument::from_scene(empty_scene, "assets/scenes/empty.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        target, prefab, "assets/prefabs/player.prefab")));

    const auto model = mirakana::editor::make_scene_prefab_instance_source_link_model(target);
    MK_REQUIRE(model.rows.size() == 2);
    MK_REQUIRE(model.linked_count == 2);
    MK_REQUIRE(model.stale_count == 0);
    MK_REQUIRE(model.rows[0].id == "1");
    MK_REQUIRE(model.rows[0].node == mirakana::SceneNodeId{1});
    MK_REQUIRE(model.rows[0].node_name == "Player");
    MK_REQUIRE(model.rows[0].status == mirakana::editor::ScenePrefabInstanceSourceLinkStatus::linked);
    MK_REQUIRE(model.rows[0].status_label == "Linked");
    MK_REQUIRE(model.rows[0].prefab_name == "player.prefab");
    MK_REQUIRE(model.rows[0].prefab_path == "assets/prefabs/player.prefab");
    MK_REQUIRE(model.rows[0].source_node_index == 1);
    MK_REQUIRE(model.rows[0].source_node_name == "Player");
    MK_REQUIRE(model.rows[1].source_node_index == 2);
    MK_REQUIRE(model.rows[1].source_node_name == "Weapon");

    const auto ui = mirakana::editor::make_scene_prefab_instance_source_link_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_source_links"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_source_links.rows.1.prefab_path"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_source_links.rows.1.source_node"}) != nullptr);

    MK_REQUIRE(history.undo());
    MK_REQUIRE(mirakana::editor::make_scene_prefab_instance_source_link_model(target).rows.empty());
}

MK_TEST("editor scene prefab source link model reports stale link diagnostics") {
    mirakana::Scene scene("Source Links");
    const auto node = scene.create_node("Broken");
    scene.find_node(node)->prefab_source = mirakana::ScenePrefabSourceLink{
        .prefab_name = "broken.prefab",
        .prefab_path = "assets/prefabs/broken.prefab",
        .source_node_index = 0,
        .source_node_name = "",
    };
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(mirakana::Scene("Valid"),
                                                                         "assets/scenes/source-links.scene");
    MK_REQUIRE(document.replace_scene(scene, node));

    const auto model = mirakana::editor::make_scene_prefab_instance_source_link_model(document);
    MK_REQUIRE(model.rows.size() == 1);
    MK_REQUIRE(model.linked_count == 0);
    MK_REQUIRE(model.stale_count == 1);
    MK_REQUIRE(model.rows[0].status == mirakana::editor::ScenePrefabInstanceSourceLinkStatus::stale);
    MK_REQUIRE(model.rows[0].status_label == "Stale");
    MK_REQUIRE(model.rows[0].diagnostic == "prefab source link is incomplete");
}

MK_TEST("editor scene prefab instance refresh review preserves linked nodes and applies with undo") {
    mirakana::PrefabDefinition prefab;
    prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon.parent_index = 1;
    prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate old_accessory;
    old_accessory.name = "OldAccessory";
    old_accessory.parent_index = 1;
    prefab.nodes.push_back(old_accessory);

    mirakana::Scene scene("Prefab Refresh");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(document.scene().nodes().size() == 3);

    auto edited = document.scene();
    auto* player_node = edited.find_node(mirakana::SceneNodeId{1});
    MK_REQUIRE(player_node != nullptr);
    player_node->name = "HeroPlayer";
    auto* weapon_node = edited.find_node(mirakana::SceneNodeId{2});
    MK_REQUIRE(weapon_node != nullptr);
    weapon_node->transform.position = mirakana::Vec3{.x = 4.0F, .y = 5.0F, .z = 6.0F};
    weapon_node->components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mirakana::AssetId::from_name("meshes/hero-weapon"),
                                        .material = mirakana::AssetId::from_name("materials/hero-weapon"),
                                        .visible = true};
    MK_REQUIRE(document.replace_scene(edited, mirakana::SceneNodeId{2}));

    mirakana::PrefabDefinition refreshed;
    refreshed.name = "player.prefab";
    mirakana::PrefabNodeTemplate refreshed_player;
    refreshed_player.name = "Player";
    refreshed_player.transform.position = mirakana::Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F};
    refreshed.nodes.push_back(refreshed_player);
    mirakana::PrefabNodeTemplate refreshed_weapon;
    refreshed_weapon.name = "Weapon";
    refreshed_weapon.parent_index = 1;
    refreshed_weapon.transform.position = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    refreshed.nodes.push_back(refreshed_weapon);
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    shield.transform.position = mirakana::Vec3{.x = 9.0F, .y = 0.0F, .z = 0.0F};
    shield.components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mirakana::AssetId::from_name("meshes/shield"),
                                        .material = mirakana::AssetId::from_name("materials/shield"),
                                        .visible = false};
    refreshed.nodes.push_back(shield);

    const auto plan = mirakana::editor::plan_scene_prefab_instance_refresh(document, mirakana::SceneNodeId{1},
                                                                           refreshed, "assets/prefabs/player.prefab");
    MK_REQUIRE(plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::warning);
    MK_REQUIRE(plan.status_label == "warning");
    MK_REQUIRE(plan.can_apply);
    MK_REQUIRE(plan.mutates);
    MK_REQUIRE(!plan.executes);
    MK_REQUIRE(plan.instance_root == mirakana::SceneNodeId{1});
    MK_REQUIRE(plan.prefab_name == "player.prefab");
    MK_REQUIRE(plan.prefab_path == "assets/prefabs/player.prefab");
    MK_REQUIRE(plan.row_count == 4);
    MK_REQUIRE(plan.preserve_count == 2);
    MK_REQUIRE(plan.add_count == 1);
    MK_REQUIRE(plan.remove_count == 1);
    MK_REQUIRE(plan.blocking_count == 0);
    MK_REQUIRE(plan.warning_count == 1);

    const auto* player_row = find_scene_prefab_instance_refresh_row(plan.rows, "source.Player");
    MK_REQUIRE(player_row != nullptr);
    MK_REQUIRE(player_row->kind == mirakana::editor::ScenePrefabInstanceRefreshRowKind::preserve_node);
    MK_REQUIRE(player_row->current_node == mirakana::SceneNodeId{1});
    MK_REQUIRE(player_row->current_node_name == "HeroPlayer");
    MK_REQUIRE(player_row->refreshed_node_index == 1);
    MK_REQUIRE(!player_row->blocking);

    const auto* shield_row = find_scene_prefab_instance_refresh_row(plan.rows, "source.Shield");
    MK_REQUIRE(shield_row != nullptr);
    MK_REQUIRE(shield_row->kind == mirakana::editor::ScenePrefabInstanceRefreshRowKind::add_source_node);
    MK_REQUIRE(shield_row->current_node == mirakana::null_scene_node);
    MK_REQUIRE(shield_row->refreshed_node_index == 3);

    const auto* old_row = find_scene_prefab_instance_refresh_row(plan.rows, "source.OldAccessory");
    MK_REQUIRE(old_row != nullptr);
    MK_REQUIRE(old_row->kind == mirakana::editor::ScenePrefabInstanceRefreshRowKind::remove_stale_node);
    MK_REQUIRE(old_row->status == mirakana::editor::ScenePrefabInstanceRefreshStatus::warning);
    MK_REQUIRE(old_row->current_node == mirakana::SceneNodeId{3});
    MK_REQUIRE(old_row->refreshed_node_index == 0);

    const auto ui = mirakana::editor::make_scene_prefab_instance_refresh_ui_model(plan);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.apply"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.source_node_variant_alignment.contract"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.source_node_variant_alignment."
                                               "contract"})
                   ->text.label == "ge.editor.scene_prefab_source_node_variant_alignment.v1");
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.source_node_variant_alignment."
                                               "review_pattern"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.Player.kind"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.Shield.status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.Player."
                                               "source_node_variant_alignment.resolution_kind"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.Shield."
                                               "source_node_variant_alignment.resolution_kind"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.OldAccessory."
                                               "source_node_variant_alignment.resolution_kind"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.Player."
                                               "source_node_variant_alignment.resolution_kind"})
                   ->text.label == "accept_current_node");
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.Shield."
                                               "source_node_variant_alignment.resolution_kind"})
                   ->text.label == "retarget_override");
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.OldAccessory."
                                               "source_node_variant_alignment.resolution_kind"})
                   ->text.label == "remove_override");

    const auto result = mirakana::editor::apply_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab");
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.scene.has_value());
    MK_REQUIRE(result.preserved_count == 2);
    MK_REQUIRE(result.added_count == 1);
    MK_REQUIRE(result.removed_count == 1);
    MK_REQUIRE(result.selected_node == mirakana::SceneNodeId{2});
    MK_REQUIRE(result.diagnostic == "scene prefab instance refresh applied");

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_action(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab")));
    MK_REQUIRE(document.scene().nodes().size() == 3);
    MK_REQUIRE(document.selected_node() == mirakana::SceneNodeId{2});
    const auto* refreshed_player_node = document.scene().find_node(mirakana::SceneNodeId{1});
    MK_REQUIRE(refreshed_player_node != nullptr);
    MK_REQUIRE(refreshed_player_node->name == "HeroPlayer");
    MK_REQUIRE(refreshed_player_node->prefab_source->source_node_name == "Player");
    MK_REQUIRE(refreshed_player_node->prefab_source->prefab_path == "assets/prefabs/player.prefab");
    const auto* refreshed_weapon_node = document.scene().find_node(mirakana::SceneNodeId{2});
    MK_REQUIRE(refreshed_weapon_node != nullptr);
    MK_REQUIRE(refreshed_weapon_node->name == "Weapon");
    MK_REQUIRE(refreshed_weapon_node->transform.position == (mirakana::Vec3{4.0F, 5.0F, 6.0F}));
    MK_REQUIRE(refreshed_weapon_node->components.mesh_renderer.has_value());
    const auto* refreshed_shield_node = document.scene().find_node(mirakana::SceneNodeId{3});
    MK_REQUIRE(refreshed_shield_node != nullptr);
    MK_REQUIRE(refreshed_shield_node->name == "Shield");
    MK_REQUIRE(refreshed_shield_node->parent == mirakana::SceneNodeId{1});
    MK_REQUIRE(refreshed_shield_node->transform.position == (mirakana::Vec3{9.0F, 0.0F, 0.0F}));
    MK_REQUIRE(refreshed_shield_node->components.mesh_renderer.has_value());
    MK_REQUIRE(refreshed_shield_node->prefab_source->source_node_index == 3);
    MK_REQUIRE(refreshed_shield_node->prefab_source->source_node_name == "Shield");

    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().nodes().size() == 3);
    MK_REQUIRE(document.scene().find_node(mirakana::SceneNodeId{3})->name == "OldAccessory");
    MK_REQUIRE(document.scene().find_node(mirakana::SceneNodeId{2})->transform.position ==
               (mirakana::Vec3{4.0F, 5.0F, 6.0F}));
}

MK_TEST("editor scene prefab instance refresh batch plans disjoint roots and blocks hierarchy conflicts") {
    mirakana::PrefabDefinition solo;
    solo.name = "solo.prefab";
    mirakana::PrefabNodeTemplate solo_root;
    solo_root.name = "Root";
    solo.nodes.push_back(solo_root);

    mirakana::Scene scene("Batch Prefab");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/batch.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, solo, "assets/prefabs/solo.prefab")));
    const auto first_root = document.selected_node();
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, solo, "assets/prefabs/solo.prefab")));
    const auto second_root = document.selected_node();
    MK_REQUIRE(first_root != second_root);

    mirakana::editor::ScenePrefabInstanceRefreshPolicy policy{};
    const auto empty_batch = mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, {}, policy);
    MK_REQUIRE(empty_batch.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(!empty_batch.can_apply);
    MK_REQUIRE(!empty_batch.batch_diagnostics.empty());

    MK_REQUIRE(history.execute(
        mirakana::editor::make_scene_authoring_reparent_node_action(document, second_root, first_root)));
    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> nested_roots;
    nested_roots.push_back({first_root, solo, "assets/prefabs/solo.prefab"});
    nested_roots.push_back({second_root, solo, "assets/prefabs/solo.prefab"});
    auto batch_nested =
        mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, std::move(nested_roots), policy);
    MK_REQUIRE(batch_nested.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(!batch_nested.can_apply);
    MK_REQUIRE(!batch_nested.batch_diagnostics.empty());

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy propagation_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = false,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = {},
    };
    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> nested_roots_with_propagation;
    nested_roots_with_propagation.push_back({first_root, solo, "assets/prefabs/solo.prefab"});
    nested_roots_with_propagation.push_back({second_root, solo, "assets/prefabs/solo.prefab"});
    const auto batch_nested_with_propagation = mirakana::editor::plan_scene_prefab_instance_refresh_batch(
        document, std::move(nested_roots_with_propagation), propagation_policy);
    MK_REQUIRE(batch_nested_with_propagation.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(!batch_nested_with_propagation.can_apply);
    MK_REQUIRE(batch_nested_with_propagation.apply_reviewed_nested_prefab_propagation_requested);
    const auto blocked_propagation_ui =
        mirakana::editor::make_scene_prefab_instance_refresh_batch_ui_model(batch_nested_with_propagation);
    const auto* propagation_flag = blocked_propagation_ui.find(
        mirakana::ui::ElementId{"scene_prefab_instance_refresh_batch.apply_reviewed_nested_prefab_propagation"});
    MK_REQUIRE(propagation_flag != nullptr);
    MK_REQUIRE(propagation_flag->text.label == "true");

    MK_REQUIRE(history.undo());

    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> disjoint;
    disjoint.push_back({first_root, solo, "assets/prefabs/solo.prefab"});
    disjoint.push_back({second_root, solo, "assets/prefabs/solo.prefab"});
    auto batch_disjoint =
        mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, std::move(disjoint), policy);
    MK_REQUIRE(batch_disjoint.target_count == 2);
    MK_REQUIRE(batch_disjoint.batch_diagnostics.empty());
    MK_REQUIRE(batch_disjoint.ordered_targets.size() == 2);
    MK_REQUIRE(batch_disjoint.can_apply);
    MK_REQUIRE(batch_disjoint.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::ready);

    const auto batch_ui = mirakana::editor::make_scene_prefab_instance_refresh_batch_ui_model(batch_disjoint);
    MK_REQUIRE(batch_ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh_batch"}) != nullptr);
    MK_REQUIRE(batch_ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh_batch.apply"}) != nullptr);
    MK_REQUIRE(
        batch_ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh_batch.source_node_variant_alignment."
                                              "contract"}) != nullptr);
    MK_REQUIRE(batch_ui
                   .find(mirakana::ui::ElementId{"scene_prefab_instance_refresh_batch.source_node_variant_alignment."
                                                 "contract"})
                   ->text.label == "ge.editor.scene_prefab_source_node_variant_alignment.v1");
    MK_REQUIRE(
        batch_ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh_batch.source_node_variant_alignment."
                                              "review_pattern"}) != nullptr);
    MK_REQUIRE(batch_ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh_batch.targets.0.status"}) !=
               nullptr);
    MK_REQUIRE(batch_ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh_batch.targets.1.status"}) !=
               nullptr);

    auto apply_inputs = batch_disjoint.ordered_targets;
    MK_REQUIRE(history.execute(
        mirakana::editor::make_scene_prefab_instance_refresh_batch_action(document, std::move(apply_inputs), policy)));
}

MK_TEST("editor scene prefab instance refresh review blocks unsafe mappings") {
    mirakana::PrefabDefinition prefab;
    prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon.parent_index = 1;
    prefab.nodes.push_back(weapon);

    mirakana::Scene scene("Prefab Refresh Blockers");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, prefab, "assets/prefabs/player.prefab")));

    const auto child_plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{2}, prefab, "assets/prefabs/player.prefab");
    MK_REQUIRE(child_plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(!child_plan.can_apply);
    MK_REQUIRE(child_plan.blocking_count == 1);
    MK_REQUIRE(child_plan.rows[0].kind == mirakana::editor::ScenePrefabInstanceRefreshRowKind::invalid_instance_root);

    mirakana::PrefabDefinition duplicate = prefab;
    mirakana::PrefabNodeTemplate duplicate_weapon;
    duplicate_weapon.name = "Weapon";
    duplicate_weapon.parent_index = 1;
    duplicate.nodes.push_back(duplicate_weapon);
    const auto duplicate_plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, duplicate, "assets/prefabs/player.prefab");
    MK_REQUIRE(duplicate_plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(!duplicate_plan.can_apply);
    MK_REQUIRE(duplicate_plan.blocking_count == 1);
    const auto* ambiguous_row =
        find_scene_prefab_instance_refresh_row(duplicate_plan.rows, "source.Weapon.refreshed_ambiguous");
    MK_REQUIRE(ambiguous_row != nullptr);
    MK_REQUIRE(ambiguous_row->kind == mirakana::editor::ScenePrefabInstanceRefreshRowKind::ambiguous_source_node);
    MK_REQUIRE(ambiguous_row->blocking);
    const auto duplicate_ui = mirakana::editor::make_scene_prefab_instance_refresh_ui_model(duplicate_plan);
    MK_REQUIRE(duplicate_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.rows.source.Weapon.refreshed_ambiguous.kind"}) != nullptr);

    const auto linked_scene_before_duplicate_instance = document.scene();
    auto duplicate_instance_scene = linked_scene_before_duplicate_instance;
    auto* duplicate_instance_weapon = duplicate_instance_scene.find_node(mirakana::SceneNodeId{2});
    MK_REQUIRE(duplicate_instance_weapon != nullptr);
    duplicate_instance_weapon->prefab_source->source_node_name = "Player";
    MK_REQUIRE(document.replace_scene(duplicate_instance_scene, mirakana::SceneNodeId{1}));
    mirakana::PrefabDefinition duplicate_both = prefab;
    mirakana::PrefabNodeTemplate duplicate_player;
    duplicate_player.name = "Player";
    duplicate_player.parent_index = 1;
    duplicate_both.nodes.push_back(duplicate_player);
    const auto duplicate_both_plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, duplicate_both, "assets/prefabs/player.prefab");
    MK_REQUIRE(duplicate_both_plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(find_scene_prefab_instance_refresh_row(duplicate_both_plan.rows, "source.Player.instance_ambiguous") !=
               nullptr);
    MK_REQUIRE(find_scene_prefab_instance_refresh_row(duplicate_both_plan.rows, "source.Player.refreshed_ambiguous") !=
               nullptr);
    const auto duplicate_both_ui = mirakana::editor::make_scene_prefab_instance_refresh_ui_model(duplicate_both_plan);
    MK_REQUIRE(duplicate_both_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.rows.source.Player.instance_ambiguous.kind"}) != nullptr);
    MK_REQUIRE(duplicate_both_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.rows.source.Player.refreshed_ambiguous.kind"}) != nullptr);

    MK_REQUIRE(document.replace_scene(linked_scene_before_duplicate_instance, mirakana::SceneNodeId{1}));

    mirakana::PrefabDefinition stale_parent_conflict;
    stale_parent_conflict.name = "player.prefab";
    stale_parent_conflict.nodes.push_back(player);
    mirakana::PrefabNodeTemplate old_group;
    old_group.name = "OldGroup";
    old_group.parent_index = 1;
    stale_parent_conflict.nodes.push_back(old_group);
    mirakana::PrefabNodeTemplate weapon_under_old_group = weapon;
    weapon_under_old_group.parent_index = 2;
    stale_parent_conflict.nodes.push_back(weapon_under_old_group);
    auto conflict_document = mirakana::editor::SceneAuthoringDocument::from_scene(
        mirakana::Scene("Prefab Refresh Conflict"), "assets/scenes/conflict.scene");
    mirakana::editor::UndoStack conflict_history;
    MK_REQUIRE(conflict_history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        conflict_document, stale_parent_conflict, "assets/prefabs/player.prefab")));
    const mirakana::editor::ScenePrefabInstanceRefreshPolicy keep_stale{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = true,
        .keep_nested_prefab_instances = false,
        .apply_reviewed_nested_prefab_propagation = false,
        .load_prefab_for_nested_propagation = {},
    };
    const auto stale_conflict_plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        conflict_document, mirakana::SceneNodeId{1}, prefab, "assets/prefabs/player.prefab", keep_stale);
    MK_REQUIRE(stale_conflict_plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(!stale_conflict_plan.can_apply);
    const auto* stale_conflict_row =
        find_scene_prefab_instance_refresh_row(stale_conflict_plan.rows, "source.OldGroup.stale_subtree_conflict");
    MK_REQUIRE(stale_conflict_row != nullptr);
    MK_REQUIRE(stale_conflict_row->kind ==
               mirakana::editor::ScenePrefabInstanceRefreshRowKind::unsupported_stale_source_subtree);
    MK_REQUIRE(stale_conflict_row->blocking);

    const auto stale_conflict_ui = mirakana::editor::make_scene_prefab_instance_refresh_ui_model(stale_conflict_plan);
    MK_REQUIRE(stale_conflict_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.stale_source_variant_alignment.contract"}) != nullptr);
    MK_REQUIRE(stale_conflict_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.stale_source_variant_alignment.review_pattern"}) != nullptr);
    MK_REQUIRE(stale_conflict_ui.find(
                   mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.OldGroup.stale_subtree_conflict."
                                           "stale_source_variant_alignment.resolution_kind"}) != nullptr);

    auto with_local_child = document.scene();
    const auto local_child = with_local_child.create_node("LocalNote");
    with_local_child.set_parent(local_child, mirakana::SceneNodeId{1});
    MK_REQUIRE(document.replace_scene(with_local_child, mirakana::SceneNodeId{1}));
    const auto local_child_plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, prefab, "assets/prefabs/player.prefab");
    MK_REQUIRE(local_child_plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(!local_child_plan.can_apply);
    const auto* local_child_row = find_scene_prefab_instance_refresh_row(local_child_plan.rows, "node.3.local_child");
    MK_REQUIRE(local_child_row != nullptr);
    MK_REQUIRE(local_child_row->kind == mirakana::editor::ScenePrefabInstanceRefreshRowKind::unsupported_local_child);
    MK_REQUIRE(local_child_row->current_node == mirakana::SceneNodeId{3});
    MK_REQUIRE(local_child_row->blocking);

    const auto blocked_local_ui = mirakana::editor::make_scene_prefab_instance_refresh_ui_model(local_child_plan);
    MK_REQUIRE(blocked_local_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.local_child_variant_alignment.contract"}) != nullptr);
    MK_REQUIRE(blocked_local_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.local_child_variant_alignment.review_pattern"}) != nullptr);
    MK_REQUIRE(blocked_local_ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.node.3.local_child."
                                                             "local_child_variant_alignment.resolution_kind"}) !=
               nullptr);

    const auto blocked_result = mirakana::editor::apply_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, prefab, "assets/prefabs/player.prefab");
    MK_REQUIRE(!blocked_result.applied);
    MK_REQUIRE(!blocked_result.scene.has_value());
    MK_REQUIRE(blocked_result.diagnostic == "scene prefab instance refresh is blocked");
}

MK_TEST("editor scene prefab instance refresh review blocks multi root source prefabs") {
    mirakana::PrefabDefinition prefab;
    prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon.parent_index = 1;
    prefab.nodes.push_back(weapon);

    mirakana::Scene scene("Prefab Refresh Multi Root");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, prefab, "assets/prefabs/player.prefab")));

    mirakana::PrefabDefinition multi_root = prefab;
    mirakana::PrefabNodeTemplate companion_root;
    companion_root.name = "Companion";
    companion_root.parent_index = 0;
    multi_root.nodes.push_back(companion_root);

    const auto plan = mirakana::editor::plan_scene_prefab_instance_refresh(document, mirakana::SceneNodeId{1},
                                                                           multi_root, "assets/prefabs/player.prefab");
    MK_REQUIRE(plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(!plan.can_apply);
    const auto* multi_root_row = find_scene_prefab_instance_refresh_row(plan.rows, "root.multi_root");
    MK_REQUIRE(multi_root_row != nullptr);
    MK_REQUIRE(multi_root_row->kind == mirakana::editor::ScenePrefabInstanceRefreshRowKind::invalid_instance_root);
    MK_REQUIRE(multi_root_row->blocking);

    const auto result = mirakana::editor::apply_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, multi_root, "assets/prefabs/player.prefab");
    MK_REQUIRE(!result.applied);
    MK_REQUIRE(!result.scene.has_value());
}

MK_TEST("editor scene prefab instance refresh review can keep local child subtrees") {
    mirakana::PrefabDefinition prefab;
    prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon.parent_index = 1;
    prefab.nodes.push_back(weapon);

    mirakana::Scene scene("Prefab Refresh Local Children");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, prefab, "assets/prefabs/player.prefab")));

    auto edited = document.scene();
    const auto local_note = edited.create_node("LocalNote");
    auto* local_note_node = edited.find_node(local_note);
    MK_REQUIRE(local_note_node != nullptr);
    local_note_node->transform.position = mirakana::Vec3{.x = 2.0F, .y = 3.0F, .z = 4.0F};
    edited.set_parent(local_note, mirakana::SceneNodeId{1});
    const auto local_leaf = edited.create_node("LocalLeaf");
    auto* local_leaf_node = edited.find_node(local_leaf);
    MK_REQUIRE(local_leaf_node != nullptr);
    local_leaf_node->components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/local-leaf"),
        .material = mirakana::AssetId::from_name("materials/local-leaf"),
        .visible = true,
    };
    edited.set_parent(local_leaf, local_note);
    MK_REQUIRE(document.replace_scene(edited, local_leaf));

    mirakana::PrefabDefinition refreshed = prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed.nodes.push_back(shield);

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy keep_local{
        .keep_local_children = true,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = false,
        .apply_reviewed_nested_prefab_propagation = false,
        .load_prefab_for_nested_propagation = {},
    };
    const auto plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab", keep_local);
    MK_REQUIRE(plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::warning);
    MK_REQUIRE(plan.can_apply);
    MK_REQUIRE(plan.row_count == 4);
    MK_REQUIRE(plan.preserve_count == 2);
    MK_REQUIRE(plan.add_count == 1);
    MK_REQUIRE(plan.remove_count == 0);
    MK_REQUIRE(plan.keep_local_child_count == 1);
    MK_REQUIRE(plan.blocking_count == 0);
    MK_REQUIRE(plan.warning_count == 1);
    MK_REQUIRE(plan.keep_local_children);

    const auto* local_row = find_scene_prefab_instance_refresh_row(plan.rows, "node.3.local_child");
    MK_REQUIRE(local_row != nullptr);
    MK_REQUIRE(local_row->kind == mirakana::editor::ScenePrefabInstanceRefreshRowKind::keep_local_child);
    MK_REQUIRE(local_row->status == mirakana::editor::ScenePrefabInstanceRefreshStatus::warning);
    MK_REQUIRE(local_row->current_node == local_note);
    MK_REQUIRE(local_row->current_node_name == "LocalNote");
    MK_REQUIRE(!local_row->blocking);

    const auto ui = mirakana::editor::make_scene_prefab_instance_refresh_ui_model(plan);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.summary.keep_local"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.keep_local_children"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.local_child_variant_alignment.contract"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.local_child_variant_alignment.review_pattern"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.node.3.local_child."
                                               "local_child_variant_alignment.resolution_kind"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.node.3.local_child.kind"}) !=
               nullptr);

    const auto result = mirakana::editor::apply_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab", keep_local);
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.scene.has_value());
    MK_REQUIRE(result.preserved_count == 2);
    MK_REQUIRE(result.added_count == 1);
    MK_REQUIRE(result.removed_count == 0);
    MK_REQUIRE(result.kept_local_child_count == 1);
    MK_REQUIRE(result.selected_node == mirakana::SceneNodeId{5});

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_action(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab", keep_local)));
    MK_REQUIRE(document.scene().nodes().size() == 5);
    MK_REQUIRE(document.selected_node() == mirakana::SceneNodeId{5});
    const auto* refreshed_shield = document.scene().find_node(mirakana::SceneNodeId{3});
    MK_REQUIRE(refreshed_shield != nullptr);
    MK_REQUIRE(refreshed_shield->name == "Shield");
    MK_REQUIRE(refreshed_shield->prefab_source.has_value());
    const auto* kept_local_note = document.scene().find_node(mirakana::SceneNodeId{4});
    MK_REQUIRE(kept_local_note != nullptr);
    MK_REQUIRE(kept_local_note->name == "LocalNote");
    MK_REQUIRE(kept_local_note->parent == mirakana::SceneNodeId{1});
    MK_REQUIRE(kept_local_note->transform.position == (mirakana::Vec3{2.0F, 3.0F, 4.0F}));
    MK_REQUIRE(!kept_local_note->prefab_source.has_value());
    const auto* kept_local_leaf = document.scene().find_node(mirakana::SceneNodeId{5});
    MK_REQUIRE(kept_local_leaf != nullptr);
    MK_REQUIRE(kept_local_leaf->name == "LocalLeaf");
    MK_REQUIRE(kept_local_leaf->parent == mirakana::SceneNodeId{4});
    MK_REQUIRE(kept_local_leaf->components.mesh_renderer.has_value());

    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().nodes().size() == 4);
    MK_REQUIRE(document.selected_node() == local_leaf);
    MK_REQUIRE(document.scene().find_node(local_note)->name == "LocalNote");
}

MK_TEST("editor scene prefab instance refresh review can keep nested prefab instances") {
    // Needles: ge.editor.scene_nested_prefab_propagation_preview.v1 preview_only_no_chained_refresh
    // (check-ai-integration prefabVariantAuthoringChecks).
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::Scene scene("Prefab Refresh Nested Prefab");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));

    auto nested_scene = document.scene();
    nested_scene.set_parent(mirakana::SceneNodeId{3}, mirakana::SceneNodeId{2});
    MK_REQUIRE(document.replace_scene(nested_scene, mirakana::SceneNodeId{4}));

    mirakana::PrefabDefinition refreshed = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed.nodes.push_back(shield);

    const auto default_plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab");
    MK_REQUIRE(default_plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(!default_plan.can_apply);
    MK_REQUIRE(default_plan.blocking_count == 1);
    MK_REQUIRE(default_plan.keep_nested_prefab_instance_count == 0);
    MK_REQUIRE(default_plan.unsupported_nested_prefab_instance_count == 1);
    MK_REQUIRE(default_plan.descendant_linked_prefab_instance_root_count == 1);
    MK_REQUIRE(default_plan.distinct_nested_prefab_asset_count == 1);
    MK_REQUIRE(default_plan.nested_prefab_propagation_preview.size() == 1);
    MK_REQUIRE(default_plan.nested_prefab_propagation_preview[0].preview_order == 0);
    MK_REQUIRE(default_plan.nested_prefab_propagation_preview[0].instance_root == mirakana::SceneNodeId{3});
    MK_REQUIRE(default_plan.nested_prefab_propagation_preview[0].node_name == "Weapon");
    MK_REQUIRE(default_plan.nested_prefab_propagation_preview[0].prefab_name == "weapon.prefab");
    MK_REQUIRE(default_plan.nested_prefab_propagation_preview[0].prefab_path == "assets/prefabs/weapon.prefab");
    const auto* default_nested_row = find_scene_prefab_instance_refresh_row(default_plan.rows, "node.3.nested_prefab");
    MK_REQUIRE(default_nested_row != nullptr);
    MK_REQUIRE(default_nested_row->kind ==
               mirakana::editor::ScenePrefabInstanceRefreshRowKind::unsupported_nested_prefab_instance);
    MK_REQUIRE(default_nested_row->status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
    MK_REQUIRE(default_nested_row->current_node == mirakana::SceneNodeId{3});
    MK_REQUIRE(default_nested_row->current_node_name == "Weapon");

    const auto default_ui = mirakana::editor::make_scene_prefab_instance_refresh_ui_model(default_plan);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.summary.unsupported_nested"}) !=
               nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.summary.descendant_linked_prefab_roots"}) != nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.nested_prefab_variant_alignment.contract"}) != nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.nested_prefab_variant_alignment.review_pattern"}) != nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.rows.node.3.nested_prefab.nested_variant_alignment."
                   "resolution_kind"}) != nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.propagation_preview.contract"}) !=
               nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.propagation_preview.operator_policy"}) != nullptr);
    MK_REQUIRE(default_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.propagation_preview.rows.0.instance_root"}) != nullptr);

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy keep_nested{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = false,
        .load_prefab_for_nested_propagation = {},
    };
    const auto keep_plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab", keep_nested);
    MK_REQUIRE(keep_plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::warning);
    MK_REQUIRE(keep_plan.can_apply);
    MK_REQUIRE(keep_plan.preserve_count == 2);
    MK_REQUIRE(keep_plan.add_count == 1);
    MK_REQUIRE(keep_plan.remove_count == 0);
    MK_REQUIRE(keep_plan.keep_nested_prefab_instance_count == 1);
    MK_REQUIRE(keep_plan.unsupported_nested_prefab_instance_count == 0);
    MK_REQUIRE(keep_plan.descendant_linked_prefab_instance_root_count == 1);
    MK_REQUIRE(keep_plan.distinct_nested_prefab_asset_count == 1);
    MK_REQUIRE(keep_plan.nested_prefab_propagation_preview.size() == 1);
    MK_REQUIRE(keep_plan.nested_prefab_propagation_preview[0].instance_root == mirakana::SceneNodeId{3});
    MK_REQUIRE(keep_plan.blocking_count == 0);
    MK_REQUIRE(keep_plan.warning_count == 1);
    MK_REQUIRE(keep_plan.keep_nested_prefab_instances);

    const auto* keep_nested_row = find_scene_prefab_instance_refresh_row(keep_plan.rows, "node.3.nested_prefab");
    MK_REQUIRE(keep_nested_row != nullptr);
    MK_REQUIRE(keep_nested_row->kind ==
               mirakana::editor::ScenePrefabInstanceRefreshRowKind::keep_nested_prefab_instance);
    MK_REQUIRE(keep_nested_row->status == mirakana::editor::ScenePrefabInstanceRefreshStatus::warning);
    MK_REQUIRE(keep_nested_row->current_node == mirakana::SceneNodeId{3});
    MK_REQUIRE(!keep_nested_row->blocking);

    const auto ui = mirakana::editor::make_scene_prefab_instance_refresh_ui_model(keep_plan);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.summary.keep_nested"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.summary.unsupported_nested"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.summary.descendant_linked_prefab_roots"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.summary.distinct_nested_prefab_assets"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.keep_nested_prefab_instances"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.node.3.nested_prefab.kind"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.nested_prefab_variant_alignment."
                                               "contract"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.nested_prefab_variant_alignment."
                                               "review_pattern"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.node.3.nested_prefab."
                                               "nested_variant_alignment.resolution_kind"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.propagation_preview.contract"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.propagation_preview.rows.0.prefab_path"}) != nullptr);

    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> nested_preview_batch;
    nested_preview_batch.push_back({mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab"});
    const auto nested_propagation_batch = mirakana::editor::plan_scene_prefab_instance_refresh_batch(
        document, std::move(nested_preview_batch), keep_nested);
    MK_REQUIRE(nested_propagation_batch.targets.size() == 1);
    MK_REQUIRE(nested_propagation_batch.targets.front().nested_prefab_propagation_preview.size() == 1);
    const auto nested_propagation_batch_ui =
        mirakana::editor::make_scene_prefab_instance_refresh_batch_ui_model(nested_propagation_batch);
    MK_REQUIRE(nested_propagation_batch_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh_batch.targets.0.propagation_preview.contract"}) != nullptr);
    MK_REQUIRE(nested_propagation_batch_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh_batch.targets.0.propagation_preview.rows.0.instance_root"}) !=
               nullptr);

    MK_REQUIRE(mirakana::editor::prefab_variant_conflict_resolution_kind_label(
                   mirakana::editor::PrefabVariantConflictResolutionKind::accept_current_node) ==
               "accept_current_node");

    const auto result = mirakana::editor::apply_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab", keep_nested);
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.scene.has_value());
    MK_REQUIRE(result.preserved_count == 2);
    MK_REQUIRE(result.added_count == 1);
    MK_REQUIRE(result.removed_count == 0);
    MK_REQUIRE(result.kept_nested_prefab_instance_count == 1);
    MK_REQUIRE(result.selected_node == mirakana::SceneNodeId{5});

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_action(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab", keep_nested)));
    MK_REQUIRE(document.scene().nodes().size() == 5);
    MK_REQUIRE(document.selected_node() == mirakana::SceneNodeId{5});
    const auto* refreshed_shield = document.scene().find_node(mirakana::SceneNodeId{3});
    MK_REQUIRE(refreshed_shield != nullptr);
    MK_REQUIRE(refreshed_shield->name == "Shield");
    MK_REQUIRE(refreshed_shield->prefab_source.has_value());
    const auto* kept_weapon = document.scene().find_node(mirakana::SceneNodeId{4});
    MK_REQUIRE(kept_weapon != nullptr);
    MK_REQUIRE(kept_weapon->name == "Weapon");
    MK_REQUIRE(kept_weapon->parent == mirakana::SceneNodeId{2});
    MK_REQUIRE(kept_weapon->prefab_source.has_value());
    MK_REQUIRE(kept_weapon->prefab_source->prefab_name == "weapon.prefab");
    MK_REQUIRE(kept_weapon->prefab_source->source_node_index == 1);
    MK_REQUIRE(kept_weapon->prefab_source->source_node_name == "Weapon");
    const auto* kept_blade = document.scene().find_node(mirakana::SceneNodeId{5});
    MK_REQUIRE(kept_blade != nullptr);
    MK_REQUIRE(kept_blade->name == "Blade");
    MK_REQUIRE(kept_blade->parent == mirakana::SceneNodeId{4});
    MK_REQUIRE(kept_blade->prefab_source.has_value());
    MK_REQUIRE(kept_blade->prefab_source->prefab_name == "weapon.prefab");
    MK_REQUIRE(kept_blade->prefab_source->source_node_index == 2);
    MK_REQUIRE(kept_blade->prefab_source->source_node_name == "Blade");

    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().nodes().size() == 4);
    MK_REQUIRE(document.selected_node() == mirakana::SceneNodeId{4});
    MK_REQUIRE(document.scene().find_node(mirakana::SceneNodeId{3})->prefab_source->prefab_name == "weapon.prefab");
    MK_REQUIRE(document.scene().find_node(mirakana::SceneNodeId{4})->prefab_source->source_node_name == "Blade");
}

MK_TEST("editor scene prefab instance refresh review can apply nested prefab propagation chain") {
    // ai-contract-needle: reviewed_chained_prefab_refresh_after_root (MK_ui propagation_preview.operator_policy label
    // when apply is on)
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::PrefabDefinition refreshed_weapon = weapon_prefab;
    mirakana::PrefabNodeTemplate grip;
    grip.name = "Grip";
    grip.parent_index = 1;
    refreshed_weapon.nodes.push_back(grip);

    mirakana::Scene scene("Prefab Refresh Nested Propagation");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));

    auto nested_scene = document.scene();
    nested_scene.set_parent(mirakana::SceneNodeId{3}, mirakana::SceneNodeId{2});
    MK_REQUIRE(document.replace_scene(nested_scene, mirakana::SceneNodeId{4}));

    mirakana::PrefabDefinition refreshed_player = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed_player.nodes.push_back(shield);

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy propagation_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path == "assets/prefabs/weapon.prefab") {
                return refreshed_weapon;
            }
            return std::nullopt;
        },
    };

    const auto propagation_plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab", propagation_policy);
    MK_REQUIRE(propagation_plan.can_apply);
    MK_REQUIRE(propagation_plan.apply_reviewed_nested_prefab_propagation_requested);
    const auto propagation_ui = mirakana::editor::make_scene_prefab_instance_refresh_ui_model(propagation_plan);
    MK_REQUIRE(propagation_ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.apply_reviewed_nested_prefab_propagation"}) != nullptr);
    MK_REQUIRE(propagation_ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.propagation_preview."
                                                           "operator_policy"}) != nullptr);

    const auto count_weapon_grip_shield = [](const mirakana::Scene& scene) {
        struct Scan {
            bool weapon_root{};
            bool grip{};
            bool shield{};
        };
        Scan out{};
        for (const auto& node : scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == "weapon.prefab" && src.source_node_name == "Weapon" && src.source_node_index == 1U) {
                out.weapon_root = true;
            }
            if (src.prefab_name == "weapon.prefab" && src.source_node_name == "Grip") {
                out.grip = true;
            }
            if (src.prefab_name == "player.prefab" && src.source_node_name == "Shield") {
                out.shield = true;
            }
        }
        return out;
    };

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_action(
        document, mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab", propagation_policy)));

    {
        const auto scan = count_weapon_grip_shield(document.scene());
        MK_REQUIRE(scan.weapon_root);
        MK_REQUIRE(scan.grip);
        MK_REQUIRE(scan.shield);
    }

    MK_REQUIRE(history.undo());
    {
        const auto scan = count_weapon_grip_shield(document.scene());
        MK_REQUIRE(scan.weapon_root);
        MK_REQUIRE(!scan.grip);
        MK_REQUIRE(!scan.shield);
    }

    MK_REQUIRE(history.redo());
    {
        const auto scan = count_weapon_grip_shield(document.scene());
        MK_REQUIRE(scan.weapon_root);
        MK_REQUIRE(scan.grip);
        MK_REQUIRE(scan.shield);
    }
}

MK_TEST("editor scene prefab instance refresh batch can apply nested prefab propagation chain with undo redo") {
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::PrefabDefinition refreshed_weapon = weapon_prefab;
    mirakana::PrefabNodeTemplate grip;
    grip.name = "Grip";
    grip.parent_index = 1;
    refreshed_weapon.nodes.push_back(grip);

    mirakana::Scene scene("Prefab Refresh Nested Propagation Batch");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));

    auto nested_scene = document.scene();
    nested_scene.set_parent(mirakana::SceneNodeId{3}, mirakana::SceneNodeId{2});
    MK_REQUIRE(document.replace_scene(nested_scene, mirakana::SceneNodeId{4}));

    mirakana::PrefabDefinition refreshed_player = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed_player.nodes.push_back(shield);

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy propagation_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path == "assets/prefabs/weapon.prefab") {
                return refreshed_weapon;
            }
            return std::nullopt;
        },
    };

    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> batch_targets;
    batch_targets.push_back({mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab"});
    const auto batch_plan =
        mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, batch_targets, propagation_policy);
    MK_REQUIRE(batch_plan.can_apply);
    MK_REQUIRE(batch_plan.targets.size() == 1);
    MK_REQUIRE(batch_plan.apply_reviewed_nested_prefab_propagation_requested);
    MK_REQUIRE(batch_plan.targets.front().apply_reviewed_nested_prefab_propagation_requested);

    const auto count_weapon_grip_shield = [](const mirakana::Scene& scene) {
        struct Scan {
            bool weapon_root{};
            bool grip{};
            bool shield{};
        };
        Scan out{};
        for (const auto& node : scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == "weapon.prefab" && src.source_node_name == "Weapon" && src.source_node_index == 1U) {
                out.weapon_root = true;
            }
            if (src.prefab_name == "weapon.prefab" && src.source_node_name == "Grip") {
                out.grip = true;
            }
            if (src.prefab_name == "player.prefab" && src.source_node_name == "Shield") {
                out.shield = true;
            }
        }
        return out;
    };

    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> batch_apply;
    batch_apply.push_back({mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab"});
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_batch_action(
        document, std::move(batch_apply), propagation_policy)));

    {
        const auto scan = count_weapon_grip_shield(document.scene());
        MK_REQUIRE(scan.weapon_root);
        MK_REQUIRE(scan.grip);
        MK_REQUIRE(scan.shield);
    }

    MK_REQUIRE(history.undo());
    {
        const auto scan = count_weapon_grip_shield(document.scene());
        MK_REQUIRE(scan.weapon_root);
        MK_REQUIRE(!scan.grip);
        MK_REQUIRE(!scan.shield);
    }

    MK_REQUIRE(history.redo());
    {
        const auto scan = count_weapon_grip_shield(document.scene());
        MK_REQUIRE(scan.weapon_root);
        MK_REQUIRE(scan.grip);
        MK_REQUIRE(scan.shield);
    }
}

MK_TEST("editor scene prefab instance refresh batch multi-target can apply nested prefab propagation") {
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::PrefabDefinition refreshed_weapon = weapon_prefab;
    mirakana::PrefabNodeTemplate grip;
    grip.name = "Grip";
    grip.parent_index = 1;
    refreshed_weapon.nodes.push_back(grip);

    mirakana::Scene scene("Prefab Refresh Nested Propagation Batch Multi");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    auto nested_scene_a = document.scene();
    nested_scene_a.set_parent(mirakana::SceneNodeId{3}, mirakana::SceneNodeId{2});
    MK_REQUIRE(document.replace_scene(nested_scene_a, mirakana::SceneNodeId{4}));

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    auto nested_scene_b = document.scene();
    nested_scene_b.set_parent(mirakana::SceneNodeId{7}, mirakana::SceneNodeId{6});
    MK_REQUIRE(document.replace_scene(nested_scene_b, mirakana::SceneNodeId{8}));

    mirakana::PrefabDefinition refreshed_player = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed_player.nodes.push_back(shield);

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy propagation_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path == "assets/prefabs/weapon.prefab") {
                return refreshed_weapon;
            }
            return std::nullopt;
        },
    };

    const auto count_grip_nodes = [](const mirakana::Scene& scene) {
        std::size_t n = 0;
        for (const auto& node : scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == "weapon.prefab" && src.source_node_name == "Grip") {
                ++n;
            }
        }
        return n;
    };
    const auto count_shield_nodes = [](const mirakana::Scene& scene) {
        std::size_t n = 0;
        for (const auto& node : scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == "player.prefab" && src.source_node_name == "Shield") {
                ++n;
            }
        }
        return n;
    };

    MK_REQUIRE(count_grip_nodes(document.scene()) == 0);
    MK_REQUIRE(count_shield_nodes(document.scene()) == 0);

    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> batch_targets;
    batch_targets.push_back({mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab"});
    batch_targets.push_back({mirakana::SceneNodeId{5}, refreshed_player, "assets/prefabs/player.prefab"});
    const auto batch_plan =
        mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, batch_targets, propagation_policy);
    MK_REQUIRE(batch_plan.can_apply);
    MK_REQUIRE(batch_plan.ordered_targets.size() == 2);
    MK_REQUIRE(batch_plan.apply_reviewed_nested_prefab_propagation_requested);

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_batch_action(
        document, std::move(batch_targets), propagation_policy)));

    MK_REQUIRE(count_grip_nodes(document.scene()) == 2);
    MK_REQUIRE(count_shield_nodes(document.scene()) == 2);

    MK_REQUIRE(history.undo());
    MK_REQUIRE(count_grip_nodes(document.scene()) == 0);
    MK_REQUIRE(count_shield_nodes(document.scene()) == 0);

    MK_REQUIRE(history.redo());
    MK_REQUIRE(count_grip_nodes(document.scene()) == 2);
    MK_REQUIRE(count_shield_nodes(document.scene()) == 2);
}

MK_TEST("editor scene prefab instance refresh batch triple disjoint can apply nested prefab propagation") {
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::PrefabDefinition refreshed_weapon = weapon_prefab;
    mirakana::PrefabNodeTemplate grip;
    grip.name = "Grip";
    grip.parent_index = 1;
    refreshed_weapon.nodes.push_back(grip);

    mirakana::Scene scene("Prefab Refresh Nested Propagation Batch Triple");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    {
        auto nested_scene = document.scene();
        nested_scene.set_parent(mirakana::SceneNodeId{3}, mirakana::SceneNodeId{2});
        MK_REQUIRE(document.replace_scene(nested_scene, mirakana::SceneNodeId{4}));
    }

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    {
        auto nested_scene = document.scene();
        nested_scene.set_parent(mirakana::SceneNodeId{7}, mirakana::SceneNodeId{6});
        MK_REQUIRE(document.replace_scene(nested_scene, mirakana::SceneNodeId{8}));
    }

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    {
        auto nested_scene = document.scene();
        nested_scene.set_parent(mirakana::SceneNodeId{11}, mirakana::SceneNodeId{10});
        MK_REQUIRE(document.replace_scene(nested_scene, mirakana::SceneNodeId{12}));
    }

    mirakana::PrefabDefinition refreshed_player = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed_player.nodes.push_back(shield);

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy propagation_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path == "assets/prefabs/weapon.prefab") {
                return refreshed_weapon;
            }
            return std::nullopt;
        },
    };

    const auto count_grip_nodes = [](const mirakana::Scene& scene) {
        std::size_t n = 0;
        for (const auto& node : scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == "weapon.prefab" && src.source_node_name == "Grip") {
                ++n;
            }
        }
        return n;
    };
    const auto count_shield_nodes = [](const mirakana::Scene& scene) {
        std::size_t n = 0;
        for (const auto& node : scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == "player.prefab" && src.source_node_name == "Shield") {
                ++n;
            }
        }
        return n;
    };

    MK_REQUIRE(count_grip_nodes(document.scene()) == 0);
    MK_REQUIRE(count_shield_nodes(document.scene()) == 0);

    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> batch_targets;
    batch_targets.push_back({mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab"});
    batch_targets.push_back({mirakana::SceneNodeId{5}, refreshed_player, "assets/prefabs/player.prefab"});
    batch_targets.push_back({mirakana::SceneNodeId{9}, refreshed_player, "assets/prefabs/player.prefab"});
    const auto batch_plan =
        mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, batch_targets, propagation_policy);
    MK_REQUIRE(batch_plan.can_apply);
    MK_REQUIRE(batch_plan.ordered_targets.size() == 3);
    MK_REQUIRE(batch_plan.apply_reviewed_nested_prefab_propagation_requested);

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_batch_action(
        document, std::move(batch_targets), propagation_policy)));

    MK_REQUIRE(count_grip_nodes(document.scene()) == 3);
    MK_REQUIRE(count_shield_nodes(document.scene()) == 3);

    MK_REQUIRE(history.undo());
    MK_REQUIRE(count_grip_nodes(document.scene()) == 0);
    MK_REQUIRE(count_shield_nodes(document.scene()) == 0);

    MK_REQUIRE(history.redo());
    MK_REQUIRE(count_grip_nodes(document.scene()) == 3);
    MK_REQUIRE(count_shield_nodes(document.scene()) == 3);
}

MK_TEST("editor scene prefab instance refresh batch blocks nested propagation when loader unavailable") {
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::PrefabDefinition refreshed_player = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed_player.nodes.push_back(shield);

    mirakana::Scene scene("Prefab Refresh Nested Propagation Missing Loader");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    auto nested_scene = document.scene();
    nested_scene.set_parent(mirakana::SceneNodeId{3}, mirakana::SceneNodeId{2});
    MK_REQUIRE(document.replace_scene(nested_scene, mirakana::SceneNodeId{4}));

    const auto count_source_nodes = [](const mirakana::Scene& scene, std::string_view prefab_name,
                                       std::string_view source_node_name) {
        std::size_t n = 0;
        for (const auto& node : scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == prefab_name && src.source_node_name == source_node_name) {
                ++n;
            }
        }
        return n;
    };
    const auto require_scene_unchanged = [&]() {
        MK_REQUIRE(document.scene().nodes().size() == 4);
        MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 0);
        MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 0);
        const auto* weapon_root = document.scene().find_node(mirakana::SceneNodeId{3});
        MK_REQUIRE(weapon_root != nullptr);
        MK_REQUIRE(weapon_root->parent == mirakana::SceneNodeId{2});
    };

    const auto require_blocked_policy = [&](const mirakana::editor::ScenePrefabInstanceRefreshPolicy& policy) {
        std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> batch_targets;
        batch_targets.push_back({mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab"});
        const auto batch_plan =
            mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, batch_targets, policy);
        MK_REQUIRE(batch_plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);
        MK_REQUIRE(!batch_plan.can_apply);
        MK_REQUIRE(batch_plan.apply_reviewed_nested_prefab_propagation_requested);
        MK_REQUIRE(batch_plan.targets.size() == 1);
        MK_REQUIRE(batch_plan.targets.front().status == mirakana::editor::ScenePrefabInstanceRefreshStatus::blocked);

        auto action = mirakana::editor::make_scene_prefab_instance_refresh_batch_action(
            document, std::move(batch_targets), policy);
        MK_REQUIRE(!history.execute(std::move(action)));
        require_scene_unchanged();
    };

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy missing_loader_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = {},
    };
    require_blocked_policy(missing_loader_policy);

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy nullopt_loader_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [](std::string_view) -> std::optional<mirakana::PrefabDefinition> {
            return std::nullopt;
        },
    };
    require_blocked_policy(nullopt_loader_policy);

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy mismatched_loader_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view) -> std::optional<mirakana::PrefabDefinition> {
            return refreshed_player;
        },
    };
    require_blocked_policy(mismatched_loader_policy);
}

MK_TEST("editor scene prefab instance refresh batch stays atomic when nested loader drifts during apply") {
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::PrefabDefinition refreshed_player = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed_player.nodes.push_back(shield);

    mirakana::PrefabDefinition refreshed_weapon = weapon_prefab;
    mirakana::PrefabNodeTemplate grip;
    grip.name = "Grip";
    grip.parent_index = 1;
    refreshed_weapon.nodes.push_back(grip);

    mirakana::Scene scene("Prefab Refresh Nested Propagation Loader Drift");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    auto nested_scene = document.scene();
    nested_scene.set_parent(mirakana::SceneNodeId{3}, mirakana::SceneNodeId{2});
    MK_REQUIRE(document.replace_scene(nested_scene, mirakana::SceneNodeId{4}));

    const auto count_source_nodes = [](const mirakana::Scene& source_scene, std::string_view prefab_name,
                                       std::string_view source_node_name) {
        std::size_t n = 0;
        for (const auto& node : source_scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == prefab_name && src.source_node_name == source_node_name) {
                ++n;
            }
        }
        return n;
    };
    const auto require_scene_unchanged = [&]() {
        MK_REQUIRE(document.scene().nodes().size() == 4);
        MK_REQUIRE(document.selected_node() == mirakana::SceneNodeId{4});
        MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 0);
        MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 0);
        const auto* player_root = document.scene().find_node(mirakana::SceneNodeId{1});
        MK_REQUIRE(player_root != nullptr);
        MK_REQUIRE(player_root->name == "Player");
        const auto* weapon_root = document.scene().find_node(mirakana::SceneNodeId{3});
        MK_REQUIRE(weapon_root != nullptr);
        MK_REQUIRE(weapon_root->parent == mirakana::SceneNodeId{2});
        const auto* selected_blade = document.scene().find_node(document.selected_node());
        MK_REQUIRE(selected_blade != nullptr);
        MK_REQUIRE(selected_blade->name == "Blade");
        MK_REQUIRE(selected_blade->parent == mirakana::SceneNodeId{3});
    };

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy stable_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path == "assets/prefabs/weapon.prefab") {
                return refreshed_weapon;
            }
            return std::nullopt;
        },
    };
    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> stable_targets;
    stable_targets.push_back({mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab"});
    const auto stable_plan =
        mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, stable_targets, stable_policy);
    MK_REQUIRE(stable_plan.can_apply);
    MK_REQUIRE(stable_plan.targets.size() == 1);
    MK_REQUIRE(stable_plan.targets.front().nested_prefab_propagation_preview.size() == 1);

    std::size_t load_count = 0;
    const mirakana::editor::ScenePrefabInstanceRefreshPolicy drifting_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path != "assets/prefabs/weapon.prefab") {
                return std::nullopt;
            }
            ++load_count;
            if (load_count <= 2) {
                return refreshed_weapon;
            }
            return std::nullopt;
        },
    };

    require_scene_unchanged();
    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> drift_targets;
    drift_targets.push_back({mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab"});
    const auto undo_count_before = history.undo_count();
    const auto redo_count_before = history.redo_count();
    auto action = mirakana::editor::make_scene_prefab_instance_refresh_batch_action(document, std::move(drift_targets),
                                                                                    drifting_policy);
    MK_REQUIRE(!history.execute(std::move(action)));
    MK_REQUIRE(history.undo_count() == undo_count_before);
    MK_REQUIRE(history.redo_count() == redo_count_before);
    MK_REQUIRE(load_count >= 3);
    require_scene_unchanged();
}

MK_TEST("editor scene prefab instance refresh batch stays atomic when later nested loader drifts during apply") {
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::PrefabDefinition refreshed_player = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed_player.nodes.push_back(shield);

    mirakana::PrefabDefinition refreshed_weapon = weapon_prefab;
    mirakana::PrefabNodeTemplate grip;
    grip.name = "Grip";
    grip.parent_index = 1;
    refreshed_weapon.nodes.push_back(grip);

    mirakana::Scene scene("Prefab Refresh Nested Propagation Multi Loader Drift");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    {
        auto nested_scene = document.scene();
        nested_scene.set_parent(mirakana::SceneNodeId{3}, mirakana::SceneNodeId{2});
        MK_REQUIRE(document.replace_scene(nested_scene, mirakana::SceneNodeId{4}));
    }

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    {
        auto nested_scene = document.scene();
        nested_scene.set_parent(mirakana::SceneNodeId{7}, mirakana::SceneNodeId{6});
        MK_REQUIRE(document.replace_scene(nested_scene, mirakana::SceneNodeId{8}));
    }

    const auto count_source_nodes = [](const mirakana::Scene& source_scene, std::string_view prefab_name,
                                       std::string_view source_node_name) {
        std::size_t n = 0;
        for (const auto& node : source_scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == prefab_name && src.source_node_name == source_node_name) {
                ++n;
            }
        }
        return n;
    };
    const auto require_scene_unchanged = [&]() {
        MK_REQUIRE(document.scene().nodes().size() == 8);
        MK_REQUIRE(document.selected_node() == mirakana::SceneNodeId{8});
        MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 0);
        MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 0);
        const auto* first_weapon_root = document.scene().find_node(mirakana::SceneNodeId{3});
        MK_REQUIRE(first_weapon_root != nullptr);
        MK_REQUIRE(first_weapon_root->parent == mirakana::SceneNodeId{2});
        const auto* second_weapon_root = document.scene().find_node(mirakana::SceneNodeId{7});
        MK_REQUIRE(second_weapon_root != nullptr);
        MK_REQUIRE(second_weapon_root->parent == mirakana::SceneNodeId{6});
        const auto* selected_blade = document.scene().find_node(document.selected_node());
        MK_REQUIRE(selected_blade != nullptr);
        MK_REQUIRE(selected_blade->name == "Blade");
        MK_REQUIRE(selected_blade->parent == mirakana::SceneNodeId{7});
    };

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy stable_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path == "assets/prefabs/weapon.prefab") {
                return refreshed_weapon;
            }
            return std::nullopt;
        },
    };
    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> stable_targets;
    stable_targets.push_back({mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab"});
    stable_targets.push_back({mirakana::SceneNodeId{5}, refreshed_player, "assets/prefabs/player.prefab"});
    const auto stable_plan =
        mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, stable_targets, stable_policy);
    MK_REQUIRE(stable_plan.can_apply);
    MK_REQUIRE(stable_plan.ordered_targets.size() == 2);
    MK_REQUIRE(stable_plan.targets.size() == 2);
    MK_REQUIRE(stable_plan.targets.front().nested_prefab_propagation_preview.size() == 1);
    MK_REQUIRE(stable_plan.targets.back().nested_prefab_propagation_preview.size() == 1);

    std::size_t load_count = 0;
    const mirakana::editor::ScenePrefabInstanceRefreshPolicy drifting_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path != "assets/prefabs/weapon.prefab") {
                return std::nullopt;
            }
            ++load_count;
            if (load_count <= 5) {
                return refreshed_weapon;
            }
            return std::nullopt;
        },
    };

    require_scene_unchanged();
    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> drift_targets;
    drift_targets.push_back({mirakana::SceneNodeId{1}, refreshed_player, "assets/prefabs/player.prefab"});
    drift_targets.push_back({mirakana::SceneNodeId{5}, refreshed_player, "assets/prefabs/player.prefab"});
    const auto undo_count_before = history.undo_count();
    const auto redo_count_before = history.redo_count();
    auto action = mirakana::editor::make_scene_prefab_instance_refresh_batch_action(document, std::move(drift_targets),
                                                                                    drifting_policy);
    MK_REQUIRE(!history.execute(std::move(action)));
    MK_REQUIRE(history.undo_count() == undo_count_before);
    MK_REQUIRE(history.redo_count() == redo_count_before);
    MK_REQUIRE(load_count >= 6);
    require_scene_unchanged();
}

MK_TEST("editor scene prefab instance refresh batch applies two-level nested prefab propagation") {
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::PrefabDefinition gem_prefab;
    gem_prefab.name = "gem.prefab";
    mirakana::PrefabNodeTemplate gem;
    gem.name = "Gem";
    gem_prefab.nodes.push_back(gem);

    mirakana::PrefabDefinition refreshed_player = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed_player.nodes.push_back(shield);

    mirakana::PrefabDefinition refreshed_weapon = weapon_prefab;
    mirakana::PrefabNodeTemplate grip;
    grip.name = "Grip";
    grip.parent_index = 1;
    refreshed_weapon.nodes.push_back(grip);

    mirakana::PrefabDefinition refreshed_gem = gem_prefab;
    mirakana::PrefabNodeTemplate sparkle;
    sparkle.name = "Sparkle";
    sparkle.parent_index = 1;
    refreshed_gem.nodes.push_back(sparkle);

    mirakana::Scene scene("Prefab Refresh Two-Level Nested Propagation Batch");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;

    const auto find_linked_child = [](const mirakana::Scene& source_scene, mirakana::SceneNodeId parent,
                                      std::string_view prefab_name, std::string_view source_node_name) {
        for (const auto& node : source_scene.nodes()) {
            if (node.parent != parent || !node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == prefab_name && src.source_node_name == source_node_name) {
                return node.id;
            }
        }
        return mirakana::null_scene_node;
    };

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    const auto first_player_root = document.selected_node();
    const auto first_player_socket = find_linked_child(document.scene(), first_player_root, "player.prefab", "Socket");
    MK_REQUIRE(first_player_socket != mirakana::null_scene_node);
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    const auto first_weapon_root = document.selected_node();
    const auto first_weapon_blade = find_linked_child(document.scene(), first_weapon_root, "weapon.prefab", "Blade");
    MK_REQUIRE(first_weapon_blade != mirakana::null_scene_node);
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, gem_prefab, "assets/prefabs/gem.prefab")));
    const auto first_gem_root = document.selected_node();
    {
        auto nested_scene = document.scene();
        nested_scene.set_parent(first_weapon_root, first_player_socket);
        nested_scene.set_parent(first_gem_root, first_weapon_blade);
        MK_REQUIRE(document.replace_scene(nested_scene, first_gem_root));
    }

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    const auto second_player_root = document.selected_node();
    const auto second_player_socket =
        find_linked_child(document.scene(), second_player_root, "player.prefab", "Socket");
    MK_REQUIRE(second_player_socket != mirakana::null_scene_node);
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    const auto second_weapon_root = document.selected_node();
    const auto second_weapon_blade = find_linked_child(document.scene(), second_weapon_root, "weapon.prefab", "Blade");
    MK_REQUIRE(second_weapon_blade != mirakana::null_scene_node);
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, gem_prefab, "assets/prefabs/gem.prefab")));
    const auto selected_gem_root = document.selected_node();
    {
        auto nested_scene = document.scene();
        nested_scene.set_parent(second_weapon_root, second_player_socket);
        nested_scene.set_parent(selected_gem_root, second_weapon_blade);
        auto* selected_gem = nested_scene.find_node(selected_gem_root);
        MK_REQUIRE(selected_gem != nullptr);
        selected_gem->name = "SelectedGem";
        MK_REQUIRE(document.replace_scene(nested_scene, selected_gem_root));
    }

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy propagation_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path == "assets/prefabs/weapon.prefab") {
                return refreshed_weapon;
            }
            if (path == "assets/prefabs/gem.prefab") {
                return refreshed_gem;
            }
            return std::nullopt;
        },
    };

    const auto count_source_nodes = [](const mirakana::Scene& scene, std::string_view prefab_name,
                                       std::string_view source_node_name) {
        std::size_t n = 0;
        for (const auto& node : scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == prefab_name && src.source_node_name == source_node_name) {
                ++n;
            }
        }
        return n;
    };
    const auto require_selected_gem_root = [&]() {
        const auto* selected = document.scene().find_node(document.selected_node());
        MK_REQUIRE(selected != nullptr);
        MK_REQUIRE(selected->name == "SelectedGem");
        MK_REQUIRE(selected->prefab_source.has_value());
        const auto& selected_source = *selected->prefab_source;
        MK_REQUIRE(selected_source.prefab_name == "gem.prefab");
        MK_REQUIRE(selected_source.source_node_index == 1U);
        MK_REQUIRE(selected_source.source_node_name == "Gem");
        MK_REQUIRE(selected->parent != mirakana::null_scene_node);
        const auto* parent = document.scene().find_node(selected->parent);
        MK_REQUIRE(parent != nullptr);
        MK_REQUIRE(parent->prefab_source.has_value());
        const auto& parent_source = *parent->prefab_source;
        MK_REQUIRE(parent_source.prefab_name == "weapon.prefab");
        MK_REQUIRE(parent_source.source_node_name == "Blade");
    };

    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 0);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 0);
    MK_REQUIRE(count_source_nodes(document.scene(), "gem.prefab", "Sparkle") == 0);
    require_selected_gem_root();

    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> batch_targets;
    batch_targets.push_back({first_player_root, refreshed_player, "assets/prefabs/player.prefab"});
    batch_targets.push_back({second_player_root, refreshed_player, "assets/prefabs/player.prefab"});
    const auto batch_plan =
        mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, batch_targets, propagation_policy);
    MK_REQUIRE(batch_plan.can_apply);
    MK_REQUIRE(batch_plan.ordered_targets.size() == 2);
    MK_REQUIRE(batch_plan.targets.size() == 2);
    MK_REQUIRE(batch_plan.targets.front().nested_prefab_propagation_preview.size() == 2);
    MK_REQUIRE(batch_plan.targets.back().nested_prefab_propagation_preview.size() == 2);
    MK_REQUIRE(batch_plan.apply_reviewed_nested_prefab_propagation_requested);

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_batch_action(
        document, std::move(batch_targets), propagation_policy)));

    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 2);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 2);
    MK_REQUIRE(count_source_nodes(document.scene(), "gem.prefab", "Sparkle") == 2);
    require_selected_gem_root();

    MK_REQUIRE(history.undo());
    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 0);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 0);
    MK_REQUIRE(count_source_nodes(document.scene(), "gem.prefab", "Sparkle") == 0);
    require_selected_gem_root();

    MK_REQUIRE(history.redo());
    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 2);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 2);
    MK_REQUIRE(count_source_nodes(document.scene(), "gem.prefab", "Sparkle") == 2);
    require_selected_gem_root();
}

MK_TEST("editor scene prefab instance refresh batch keeps recreated nested source node selected") {
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::PrefabDefinition gem_prefab;
    gem_prefab.name = "gem.prefab";
    mirakana::PrefabNodeTemplate gem;
    gem.name = "Gem";
    gem_prefab.nodes.push_back(gem);

    mirakana::PrefabDefinition refreshed_player = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed_player.nodes.push_back(shield);

    mirakana::PrefabDefinition refreshed_weapon = weapon_prefab;
    mirakana::PrefabNodeTemplate grip;
    grip.name = "Grip";
    grip.parent_index = 1;
    refreshed_weapon.nodes.push_back(grip);

    mirakana::PrefabDefinition refreshed_gem = gem_prefab;
    mirakana::PrefabNodeTemplate sparkle;
    sparkle.name = "Sparkle";
    sparkle.parent_index = 1;
    refreshed_gem.nodes.push_back(sparkle);

    mirakana::Scene scene("Prefab Refresh Selected Nested Source Remap");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;

    const auto find_linked_child = [](const mirakana::Scene& source_scene, mirakana::SceneNodeId parent,
                                      std::string_view prefab_name, std::string_view source_node_name) {
        for (const auto& node : source_scene.nodes()) {
            if (node.parent != parent || !node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == prefab_name && src.source_node_name == source_node_name) {
                return node.id;
            }
        }
        return mirakana::null_scene_node;
    };

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    const auto player_root = document.selected_node();
    const auto player_socket = find_linked_child(document.scene(), player_root, "player.prefab", "Socket");
    MK_REQUIRE(player_socket != mirakana::null_scene_node);
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    const auto weapon_root = document.selected_node();
    const auto original_blade = find_linked_child(document.scene(), weapon_root, "weapon.prefab", "Blade");
    MK_REQUIRE(original_blade != mirakana::null_scene_node);
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, gem_prefab, "assets/prefabs/gem.prefab")));
    const auto gem_root = document.selected_node();
    {
        auto nested_scene = document.scene();
        nested_scene.set_parent(weapon_root, player_socket);
        nested_scene.set_parent(gem_root, original_blade);
        auto* selected_blade = nested_scene.find_node(original_blade);
        MK_REQUIRE(selected_blade != nullptr);
        selected_blade->name = "SelectedBlade";
        MK_REQUIRE(document.replace_scene(nested_scene, original_blade));
    }

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy propagation_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path == "assets/prefabs/weapon.prefab") {
                return refreshed_weapon;
            }
            if (path == "assets/prefabs/gem.prefab") {
                return refreshed_gem;
            }
            return std::nullopt;
        },
    };

    const auto count_source_nodes = [](const mirakana::Scene& source_scene, std::string_view prefab_name,
                                       std::string_view source_node_name) {
        std::size_t n = 0;
        for (const auto& node : source_scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == prefab_name && src.source_node_name == source_node_name) {
                ++n;
            }
        }
        return n;
    };
    const auto require_selected_blade = [&](std::string_view expected_name) {
        const auto* selected = document.scene().find_node(document.selected_node());
        MK_REQUIRE(selected != nullptr);
        MK_REQUIRE(selected->name == expected_name);
        MK_REQUIRE(selected->prefab_source.has_value());
        const auto& selected_source = *selected->prefab_source;
        MK_REQUIRE(selected_source.prefab_name == "weapon.prefab");
        MK_REQUIRE(selected_source.source_node_index == 2U);
        MK_REQUIRE(selected_source.source_node_name == "Blade");
        MK_REQUIRE(selected->parent != mirakana::null_scene_node);
        const auto* parent = document.scene().find_node(selected->parent);
        MK_REQUIRE(parent != nullptr);
        MK_REQUIRE(parent->prefab_source.has_value());
        const auto& parent_source = *parent->prefab_source;
        MK_REQUIRE(parent_source.prefab_name == "weapon.prefab");
        MK_REQUIRE(parent_source.source_node_name == "Weapon");
    };

    require_selected_blade("SelectedBlade");

    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> batch_targets;
    batch_targets.push_back({player_root, refreshed_player, "assets/prefabs/player.prefab"});
    const auto batch_plan =
        mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, batch_targets, propagation_policy);
    MK_REQUIRE(batch_plan.can_apply);
    MK_REQUIRE(batch_plan.targets.size() == 1);
    MK_REQUIRE(batch_plan.targets.front().nested_prefab_propagation_preview.size() == 2);

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_batch_action(
        document, std::move(batch_targets), propagation_policy)));

    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 1);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 1);
    MK_REQUIRE(count_source_nodes(document.scene(), "gem.prefab", "Sparkle") == 1);
    require_selected_blade("SelectedBlade");

    MK_REQUIRE(history.undo());
    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 0);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 0);
    MK_REQUIRE(count_source_nodes(document.scene(), "gem.prefab", "Sparkle") == 0);
    require_selected_blade("SelectedBlade");

    MK_REQUIRE(history.redo());
    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 1);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 1);
    MK_REQUIRE(count_source_nodes(document.scene(), "gem.prefab", "Sparkle") == 1);
    require_selected_blade("SelectedBlade");
}

MK_TEST("editor scene prefab instance refresh batch keeps local child during nested prefab propagation") {
    mirakana::PrefabDefinition player_prefab;
    player_prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    player_prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate socket;
    socket.name = "Socket";
    socket.parent_index = 1;
    player_prefab.nodes.push_back(socket);

    mirakana::PrefabDefinition weapon_prefab;
    weapon_prefab.name = "weapon.prefab";
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon_prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate blade;
    blade.name = "Blade";
    blade.parent_index = 1;
    weapon_prefab.nodes.push_back(blade);

    mirakana::PrefabDefinition refreshed_player = player_prefab;
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed_player.nodes.push_back(shield);

    mirakana::PrefabDefinition refreshed_weapon = weapon_prefab;
    mirakana::PrefabNodeTemplate grip;
    grip.name = "Grip";
    grip.parent_index = 1;
    refreshed_weapon.nodes.push_back(grip);

    mirakana::Scene scene("Prefab Refresh Nested Propagation Local Children");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;

    const auto find_linked_child = [](const mirakana::Scene& source_scene, mirakana::SceneNodeId parent,
                                      std::string_view prefab_name, std::string_view source_node_name) {
        for (const auto& node : source_scene.nodes()) {
            if (node.parent != parent || !node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == prefab_name && src.source_node_name == source_node_name) {
                return node.id;
            }
        }
        return mirakana::null_scene_node;
    };

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, player_prefab, "assets/prefabs/player.prefab")));
    const auto player_root = document.selected_node();
    const auto player_socket = find_linked_child(document.scene(), player_root, "player.prefab", "Socket");
    MK_REQUIRE(player_socket != mirakana::null_scene_node);
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, weapon_prefab, "assets/prefabs/weapon.prefab")));
    const auto weapon_root = document.selected_node();

    {
        auto nested_scene = document.scene();
        nested_scene.set_parent(weapon_root, player_socket);
        const auto local_note = nested_scene.create_node("LocalWeaponNote");
        auto* local_note_node = nested_scene.find_node(local_note);
        MK_REQUIRE(local_note_node != nullptr);
        local_note_node->transform.position = mirakana::Vec3{.x = 6.0F, .y = 7.0F, .z = 8.0F};
        nested_scene.set_parent(local_note, weapon_root);
        const auto local_leaf = nested_scene.create_node("LocalWeaponLeaf");
        auto* local_leaf_node = nested_scene.find_node(local_leaf);
        MK_REQUIRE(local_leaf_node != nullptr);
        local_leaf_node->components.mesh_renderer = mirakana::MeshRendererComponent{
            .mesh = mirakana::AssetId::from_name("meshes/local-weapon-leaf"),
            .material = mirakana::AssetId::from_name("materials/local-weapon-leaf"),
            .visible = true,
        };
        nested_scene.set_parent(local_leaf, local_note);
        MK_REQUIRE(document.replace_scene(nested_scene, local_leaf));
    }

    const auto count_source_nodes = [](const mirakana::Scene& source_scene, std::string_view prefab_name,
                                       std::string_view source_node_name) {
        std::size_t n = 0;
        for (const auto& node : source_scene.nodes()) {
            if (!node.prefab_source.has_value()) {
                continue;
            }
            const auto& src = *node.prefab_source;
            if (src.prefab_name == prefab_name && src.source_node_name == source_node_name) {
                ++n;
            }
        }
        return n;
    };
    const auto require_local_subtree = [&]() {
        const mirakana::SceneNode* local_note = nullptr;
        const mirakana::SceneNode* local_leaf = nullptr;
        for (const auto& node : document.scene().nodes()) {
            if (node.name == "LocalWeaponNote") {
                local_note = &node;
            } else if (node.name == "LocalWeaponLeaf") {
                local_leaf = &node;
            }
        }
        MK_REQUIRE(local_note != nullptr);
        MK_REQUIRE(local_leaf != nullptr);
        MK_REQUIRE(!local_note->prefab_source.has_value());
        MK_REQUIRE(!local_leaf->prefab_source.has_value());
        MK_REQUIRE(local_note->transform.position == (mirakana::Vec3{6.0F, 7.0F, 8.0F}));
        MK_REQUIRE(local_leaf->parent == local_note->id);
        MK_REQUIRE(local_leaf->components.mesh_renderer.has_value());

        const auto* weapon_parent = document.scene().find_node(local_note->parent);
        MK_REQUIRE(weapon_parent != nullptr);
        MK_REQUIRE(weapon_parent->prefab_source.has_value());
        const auto& weapon_source = *weapon_parent->prefab_source;
        MK_REQUIRE(weapon_source.prefab_name == "weapon.prefab");
        MK_REQUIRE(weapon_source.source_node_name == "Weapon");

        const auto* selected = document.scene().find_node(document.selected_node());
        MK_REQUIRE(selected != nullptr);
        MK_REQUIRE(selected->id == local_leaf->id);
        MK_REQUIRE(selected->name == "LocalWeaponLeaf");
    };

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy blocked_nested_local_policy{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path == "assets/prefabs/weapon.prefab") {
                return refreshed_weapon;
            }
            return std::nullopt;
        },
    };
    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> blocked_targets;
    blocked_targets.push_back({player_root, refreshed_player, "assets/prefabs/player.prefab"});
    const auto blocked_batch_plan = mirakana::editor::plan_scene_prefab_instance_refresh_batch(
        document, blocked_targets, blocked_nested_local_policy);
    MK_REQUIRE(!blocked_batch_plan.can_apply);
    MK_REQUIRE(blocked_batch_plan.blocking_target_count == 1);

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy propagation_policy{
        .keep_local_children = true,
        .keep_stale_source_nodes_as_local = false,
        .keep_nested_prefab_instances = true,
        .apply_reviewed_nested_prefab_propagation = true,
        .load_prefab_for_nested_propagation = [&](std::string_view path) -> std::optional<mirakana::PrefabDefinition> {
            if (path == "assets/prefabs/weapon.prefab") {
                return refreshed_weapon;
            }
            return std::nullopt;
        },
    };

    require_local_subtree();
    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 0);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 0);

    std::vector<mirakana::editor::ScenePrefabInstanceRefreshBatchTargetInput> batch_targets;
    batch_targets.push_back({player_root, refreshed_player, "assets/prefabs/player.prefab"});
    const auto batch_plan =
        mirakana::editor::plan_scene_prefab_instance_refresh_batch(document, batch_targets, propagation_policy);
    MK_REQUIRE(batch_plan.can_apply);
    MK_REQUIRE(batch_plan.targets.size() == 1);
    MK_REQUIRE(batch_plan.targets.front().keep_local_children);
    MK_REQUIRE(batch_plan.targets.front().nested_prefab_propagation_preview.size() == 1);
    MK_REQUIRE(batch_plan.apply_reviewed_nested_prefab_propagation_requested);

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_batch_action(
        document, std::move(batch_targets), propagation_policy)));

    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 1);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 1);
    require_local_subtree();

    MK_REQUIRE(history.undo());
    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 0);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 0);
    require_local_subtree();

    MK_REQUIRE(history.redo());
    MK_REQUIRE(count_source_nodes(document.scene(), "player.prefab", "Shield") == 1);
    MK_REQUIRE(count_source_nodes(document.scene(), "weapon.prefab", "Grip") == 1);
    require_local_subtree();
}

MK_TEST("editor scene prefab instance refresh review can keep stale source node subtrees as local") {
    mirakana::PrefabDefinition prefab;
    prefab.name = "player.prefab";
    mirakana::PrefabNodeTemplate player;
    player.name = "Player";
    prefab.nodes.push_back(player);
    mirakana::PrefabNodeTemplate weapon;
    weapon.name = "Weapon";
    weapon.parent_index = 1;
    prefab.nodes.push_back(weapon);
    mirakana::PrefabNodeTemplate old_accessory;
    old_accessory.name = "OldAccessory";
    old_accessory.parent_index = 1;
    prefab.nodes.push_back(old_accessory);
    mirakana::PrefabNodeTemplate old_leaf;
    old_leaf.name = "OldLeaf";
    old_leaf.parent_index = 3;
    prefab.nodes.push_back(old_leaf);

    mirakana::Scene scene("Prefab Refresh Stale Nodes");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/prefab-refresh.scene");
    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_instantiate_prefab_action(
        document, prefab, "assets/prefabs/player.prefab")));
    MK_REQUIRE(document.scene().nodes().size() == 4);
    MK_REQUIRE(document.replace_scene(document.scene(), mirakana::SceneNodeId{4}));

    mirakana::PrefabDefinition refreshed;
    refreshed.name = "player.prefab";
    mirakana::PrefabNodeTemplate refreshed_player;
    refreshed_player.name = "Player";
    refreshed.nodes.push_back(refreshed_player);
    mirakana::PrefabNodeTemplate refreshed_weapon;
    refreshed_weapon.name = "Weapon";
    refreshed_weapon.parent_index = 1;
    refreshed.nodes.push_back(refreshed_weapon);
    mirakana::PrefabNodeTemplate shield;
    shield.name = "Shield";
    shield.parent_index = 1;
    refreshed.nodes.push_back(shield);

    const auto default_plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab");
    MK_REQUIRE(default_plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::warning);
    MK_REQUIRE(default_plan.can_apply);
    MK_REQUIRE(default_plan.remove_count == 2);
    MK_REQUIRE(default_plan.keep_stale_source_node_count == 0);
    const auto* default_old_accessory_row =
        find_scene_prefab_instance_refresh_row(default_plan.rows, "source.OldAccessory");
    MK_REQUIRE(default_old_accessory_row != nullptr);
    MK_REQUIRE(default_old_accessory_row->kind ==
               mirakana::editor::ScenePrefabInstanceRefreshRowKind::remove_stale_node);
    const auto* default_old_leaf_row = find_scene_prefab_instance_refresh_row(default_plan.rows, "source.OldLeaf");
    MK_REQUIRE(default_old_leaf_row != nullptr);
    MK_REQUIRE(default_old_leaf_row->kind == mirakana::editor::ScenePrefabInstanceRefreshRowKind::remove_stale_node);

    const mirakana::editor::ScenePrefabInstanceRefreshPolicy keep_stale{
        .keep_local_children = false,
        .keep_stale_source_nodes_as_local = true,
        .keep_nested_prefab_instances = false,
        .apply_reviewed_nested_prefab_propagation = false,
        .load_prefab_for_nested_propagation = {},
    };
    const auto keep_plan = mirakana::editor::plan_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab", keep_stale);
    MK_REQUIRE(keep_plan.status == mirakana::editor::ScenePrefabInstanceRefreshStatus::warning);
    MK_REQUIRE(keep_plan.can_apply);
    MK_REQUIRE(keep_plan.row_count == 4);
    MK_REQUIRE(keep_plan.preserve_count == 2);
    MK_REQUIRE(keep_plan.add_count == 1);
    MK_REQUIRE(keep_plan.remove_count == 0);
    MK_REQUIRE(keep_plan.keep_stale_source_node_count == 1);
    MK_REQUIRE(keep_plan.blocking_count == 0);
    MK_REQUIRE(keep_plan.warning_count == 1);
    MK_REQUIRE(keep_plan.keep_stale_source_nodes_as_local);

    const auto* kept_old_accessory_row = find_scene_prefab_instance_refresh_row(keep_plan.rows, "source.OldAccessory");
    MK_REQUIRE(kept_old_accessory_row != nullptr);
    MK_REQUIRE(kept_old_accessory_row->kind ==
               mirakana::editor::ScenePrefabInstanceRefreshRowKind::keep_stale_source_node_as_local);
    MK_REQUIRE(kept_old_accessory_row->status == mirakana::editor::ScenePrefabInstanceRefreshStatus::warning);
    MK_REQUIRE(kept_old_accessory_row->current_node == mirakana::SceneNodeId{3});
    MK_REQUIRE(kept_old_accessory_row->current_node_name == "OldAccessory");
    MK_REQUIRE(!kept_old_accessory_row->blocking);
    MK_REQUIRE(find_scene_prefab_instance_refresh_row(keep_plan.rows, "source.OldLeaf") == nullptr);

    const auto ui = mirakana::editor::make_scene_prefab_instance_refresh_ui_model(keep_plan);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.summary.keep_stale"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.keep_stale_source_nodes_as_local"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.stale_source_variant_alignment.contract"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "scene_prefab_instance_refresh.stale_source_variant_alignment.review_pattern"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.OldAccessory."
                                               "stale_source_variant_alignment.resolution_kind"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"scene_prefab_instance_refresh.rows.source.OldAccessory.kind"}) !=
               nullptr);

    const auto result = mirakana::editor::apply_scene_prefab_instance_refresh(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab", keep_stale);
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.scene.has_value());
    MK_REQUIRE(result.preserved_count == 2);
    MK_REQUIRE(result.added_count == 1);
    MK_REQUIRE(result.removed_count == 0);
    MK_REQUIRE(result.kept_stale_source_node_count == 1);
    MK_REQUIRE(result.selected_node == mirakana::SceneNodeId{5});

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_prefab_instance_refresh_action(
        document, mirakana::SceneNodeId{1}, refreshed, "assets/prefabs/player.prefab", keep_stale)));
    MK_REQUIRE(document.scene().nodes().size() == 5);
    MK_REQUIRE(document.selected_node() == mirakana::SceneNodeId{5});
    const auto* refreshed_shield = document.scene().find_node(mirakana::SceneNodeId{3});
    MK_REQUIRE(refreshed_shield != nullptr);
    MK_REQUIRE(refreshed_shield->name == "Shield");
    MK_REQUIRE(refreshed_shield->prefab_source.has_value());
    const auto* kept_old_accessory = document.scene().find_node(mirakana::SceneNodeId{4});
    MK_REQUIRE(kept_old_accessory != nullptr);
    MK_REQUIRE(kept_old_accessory->name == "OldAccessory");
    MK_REQUIRE(kept_old_accessory->parent == mirakana::SceneNodeId{1});
    MK_REQUIRE(!kept_old_accessory->prefab_source.has_value());
    const auto* kept_old_leaf = document.scene().find_node(mirakana::SceneNodeId{5});
    MK_REQUIRE(kept_old_leaf != nullptr);
    MK_REQUIRE(kept_old_leaf->name == "OldLeaf");
    MK_REQUIRE(kept_old_leaf->parent == mirakana::SceneNodeId{4});
    MK_REQUIRE(!kept_old_leaf->prefab_source.has_value());

    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().nodes().size() == 4);
    MK_REQUIRE(document.selected_node() == mirakana::SceneNodeId{4});
    MK_REQUIRE(document.scene().find_node(mirakana::SceneNodeId{3})->prefab_source.has_value());
    MK_REQUIRE(document.scene().find_node(mirakana::SceneNodeId{4})->prefab_source.has_value());
}

MK_TEST("editor prefab variant authoring saves loads rows and instantiates composed prefab") {
    mirakana::PrefabDefinition base;
    base.name = "player.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Player";
    base.nodes.push_back(root);
    mirakana::PrefabNodeTemplate body;
    body.name = "Body";
    body.parent_index = 1;
    base.nodes.push_back(body);

    auto document = mirakana::editor::PrefabVariantAuthoringDocument::from_base_prefab(
        base, "elite-player.prefabvariant", "assets/prefabs/elite-player.prefabvariant");

    MK_REQUIRE(!document.dirty());
    MK_REQUIRE(document.path() == "assets/prefabs/elite-player.prefabvariant");
    MK_REQUIRE(document.variant().name == "elite-player.prefabvariant");

    MK_REQUIRE(document.set_name_override(1, "ElitePlayer"));
    mirakana::Transform3D transform;
    transform.position = mirakana::Vec3{.x = 4.0F, .y = 5.0F, .z = 6.0F};
    transform.scale = mirakana::Vec3{.x = 2.0F, .y = 2.0F, .z = 2.0F};
    transform.rotation_radians = mirakana::Vec3{.x = 0.0F, .y = 0.25F, .z = 0.5F};
    MK_REQUIRE(document.set_transform_override(2, transform));
    mirakana::Transform3D invalid_transform;
    invalid_transform.position = mirakana::Vec3{.x = std::numeric_limits<float>::infinity(), .y = 0.0F, .z = 0.0F};
    MK_REQUIRE(!document.set_transform_override(2, invalid_transform));
    MK_REQUIRE(document.variant().overrides.size() == 2);

    mirakana::SceneNodeComponents components;
    components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/elite-player"),
        .material = mirakana::AssetId::from_name("materials/elite-player"),
        .visible = true,
    };
    MK_REQUIRE(document.set_component_override(2, components));
    MK_REQUIRE(document.dirty());

    const auto model = document.model();
    MK_REQUIRE(model.valid());
    MK_REQUIRE(model.override_rows.size() == 3);
    MK_REQUIRE(model.override_rows[0].node_index == 1);
    MK_REQUIRE(model.override_rows[0].kind == mirakana::PrefabOverrideKind::name);
    MK_REQUIRE(model.override_rows[0].node_name == "Player");
    MK_REQUIRE(model.override_rows[0].diagnostic_count == 0);
    MK_REQUIRE(model.override_rows[2].has_mesh_renderer);

    const auto composed = document.composed_prefab();
    MK_REQUIRE(composed.name == "elite-player.prefabvariant");
    MK_REQUIRE(composed.nodes[0].name == "ElitePlayer");
    MK_REQUIRE(composed.nodes[1].transform.position == (mirakana::Vec3{4.0F, 5.0F, 6.0F}));
    MK_REQUIRE(composed.nodes[1].components.mesh_renderer.has_value());

    mirakana::editor::MemoryTextStore store;
    mirakana::editor::save_prefab_variant_authoring_document(store, document.path(), document);
    MK_REQUIRE(!document.dirty());
    MK_REQUIRE(store.exists("assets/prefabs/elite-player.prefabvariant"));

    const auto loaded =
        mirakana::editor::load_prefab_variant_authoring_document(store, "assets/prefabs/elite-player.prefabvariant");
    MK_REQUIRE(!loaded.dirty());
    MK_REQUIRE(loaded.variant().overrides.size() == 3);
    MK_REQUIRE(loaded.composed_prefab().nodes[0].name == "ElitePlayer");

    mirakana::Scene runtime_scene("Runtime");
    const auto instance = mirakana::instantiate_prefab(runtime_scene, loaded.composed_prefab());
    MK_REQUIRE(instance.nodes.size() == 2);
    MK_REQUIRE(runtime_scene.nodes()[0].name == "ElitePlayer");
}

MK_TEST("editor prefab variant native file dialogs review results and retained rows") {
    const auto open_request = mirakana::editor::make_prefab_variant_open_dialog_request("assets/prefabs");
    MK_REQUIRE(open_request.kind == mirakana::FileDialogKind::open_file);
    MK_REQUIRE(open_request.title == "Open Prefab Variant");
    MK_REQUIRE(open_request.default_location == "assets/prefabs");
    MK_REQUIRE(!open_request.allow_many);
    MK_REQUIRE(open_request.filters.size() == 1);
    MK_REQUIRE(open_request.filters[0].name == "Prefab Variant");
    MK_REQUIRE(open_request.filters[0].pattern == "prefabvariant");
    MK_REQUIRE(open_request.accept_label == "Open");

    const auto save_request =
        mirakana::editor::make_prefab_variant_save_dialog_request("assets/prefabs/selected.prefabvariant");
    MK_REQUIRE(save_request.kind == mirakana::FileDialogKind::save_file);
    MK_REQUIRE(save_request.title == "Save Prefab Variant");
    MK_REQUIRE(save_request.default_location == "assets/prefabs/selected.prefabvariant");
    MK_REQUIRE(!save_request.allow_many);
    MK_REQUIRE(save_request.filters.size() == 1);
    MK_REQUIRE(save_request.filters[0].pattern == "prefabvariant");
    MK_REQUIRE(save_request.accept_label == "Save");

    const auto open_accepted = mirakana::editor::make_prefab_variant_open_dialog_model(mirakana::FileDialogResult{
        .id = 7,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"games/sample/assets/prefabs/enemy.prefabvariant"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(open_accepted.accepted);
    MK_REQUIRE(open_accepted.status_label == "Prefab variant open dialog accepted");
    MK_REQUIRE(open_accepted.selected_path == "games/sample/assets/prefabs/enemy.prefabvariant");
    MK_REQUIRE(open_accepted.rows.size() == 4);
    MK_REQUIRE(open_accepted.rows[0].id == "status");
    MK_REQUIRE(open_accepted.rows[1].id == "selected_path");
    MK_REQUIRE(open_accepted.rows[1].value == "games/sample/assets/prefabs/enemy.prefabvariant");

    const auto open_ui = mirakana::editor::make_prefab_variant_file_dialog_ui_model(open_accepted);
    MK_REQUIRE(open_ui.find(mirakana::ui::ElementId{"prefab_variant_file_dialog.open.status.value"})->text.label ==
               "Prefab variant open dialog accepted");
    MK_REQUIRE(
        open_ui.find(mirakana::ui::ElementId{"prefab_variant_file_dialog.open.selected_path.value"})->text.label ==
        "games/sample/assets/prefabs/enemy.prefabvariant");

    const auto save_accepted = mirakana::editor::make_prefab_variant_save_dialog_model(mirakana::FileDialogResult{
        .id = 8,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"games/sample/assets/prefabs/enemy.prefabvariant"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(save_accepted.accepted);
    MK_REQUIRE(save_accepted.status_label == "Prefab variant save dialog accepted");
    const auto save_ui = mirakana::editor::make_prefab_variant_file_dialog_ui_model(save_accepted);
    MK_REQUIRE(save_ui.find(mirakana::ui::ElementId{"prefab_variant_file_dialog.save.status.value"})->text.label ==
               "Prefab variant save dialog accepted");

    const auto canceled = mirakana::editor::make_prefab_variant_open_dialog_model(mirakana::FileDialogResult{
        .id = 9,
        .status = mirakana::FileDialogStatus::canceled,
        .paths = {},
        .selected_filter = -1,
        .error = {},
    });
    MK_REQUIRE(!canceled.accepted);
    MK_REQUIRE(canceled.status_label == "Prefab variant open dialog canceled");

    const auto failed = mirakana::editor::make_prefab_variant_save_dialog_model(mirakana::FileDialogResult{
        .id = 10,
        .status = mirakana::FileDialogStatus::failed,
        .paths = {},
        .selected_filter = -1,
        .error = "native dialog failed",
    });
    MK_REQUIRE(!failed.accepted);
    MK_REQUIRE(failed.status_label == "Prefab variant save dialog failed");
    MK_REQUIRE(failed.diagnostics.size() == 1);
    MK_REQUIRE(failed.diagnostics[0] == "native dialog failed");

    const auto empty_accepted = mirakana::editor::make_prefab_variant_open_dialog_model(mirakana::FileDialogResult{
        .id = 11,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!empty_accepted.accepted);
    MK_REQUIRE(empty_accepted.diagnostics.size() == 1);
    MK_REQUIRE(empty_accepted.diagnostics[0].contains("at least one path"));

    const auto multi = mirakana::editor::make_prefab_variant_save_dialog_model(mirakana::FileDialogResult{
        .id = 12,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"a.prefabvariant", "b.prefabvariant"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!multi.accepted);
    MK_REQUIRE(multi.diagnostics.size() == 1);
    MK_REQUIRE(multi.diagnostics[0].contains("exactly one"));

    const auto wrong_extension = mirakana::editor::make_prefab_variant_open_dialog_model(mirakana::FileDialogResult{
        .id = 13,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"assets/prefabs/enemy.prefab"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!wrong_extension.accepted);
    MK_REQUIRE(wrong_extension.diagnostics.size() == 1);
    MK_REQUIRE(wrong_extension.diagnostics[0].contains(".prefabvariant"));
}

MK_TEST("editor scene native file dialogs review results and retained rows") {
    const auto open_request = mirakana::editor::make_scene_open_dialog_request("scenes");
    MK_REQUIRE(open_request.kind == mirakana::FileDialogKind::open_file);
    MK_REQUIRE(open_request.title == "Open Scene");
    MK_REQUIRE(open_request.default_location == "scenes");
    MK_REQUIRE(!open_request.allow_many);
    MK_REQUIRE(open_request.filters.size() == 1);
    MK_REQUIRE(open_request.filters[0].name == "Scene");
    MK_REQUIRE(open_request.filters[0].pattern == "scene");
    MK_REQUIRE(open_request.accept_label == "Open");

    const auto save_request = mirakana::editor::make_scene_save_dialog_request("scenes/start.scene");
    MK_REQUIRE(save_request.kind == mirakana::FileDialogKind::save_file);
    MK_REQUIRE(save_request.title == "Save Scene");
    MK_REQUIRE(save_request.default_location == "scenes/start.scene");
    MK_REQUIRE(!save_request.allow_many);
    MK_REQUIRE(save_request.filters.size() == 1);
    MK_REQUIRE(save_request.filters[0].pattern == "scene");
    MK_REQUIRE(save_request.accept_label == "Save");

    const auto open_accepted = mirakana::editor::make_scene_open_dialog_model(mirakana::FileDialogResult{
        .id = 21,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"games/sample/scenes/start.scene"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(open_accepted.accepted);
    MK_REQUIRE(open_accepted.status_label == "Scene open dialog accepted");
    MK_REQUIRE(open_accepted.selected_path == "games/sample/scenes/start.scene");
    MK_REQUIRE(open_accepted.rows.size() == 4);
    MK_REQUIRE(open_accepted.rows[0].id == "status");
    MK_REQUIRE(open_accepted.rows[1].id == "selected_path");
    MK_REQUIRE(open_accepted.rows[1].value == "games/sample/scenes/start.scene");

    const auto open_ui = mirakana::editor::make_scene_file_dialog_ui_model(open_accepted);
    MK_REQUIRE(open_ui.find(mirakana::ui::ElementId{"scene_file_dialog.open.status.value"})->text.label ==
               "Scene open dialog accepted");
    MK_REQUIRE(open_ui.find(mirakana::ui::ElementId{"scene_file_dialog.open.selected_path.value"})->text.label ==
               "games/sample/scenes/start.scene");

    const auto save_accepted = mirakana::editor::make_scene_save_dialog_model(mirakana::FileDialogResult{
        .id = 22,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"games/sample/scenes/start.scene"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(save_accepted.accepted);
    MK_REQUIRE(save_accepted.status_label == "Scene save dialog accepted");
    const auto save_ui = mirakana::editor::make_scene_file_dialog_ui_model(save_accepted);
    MK_REQUIRE(save_ui.find(mirakana::ui::ElementId{"scene_file_dialog.save.status.value"})->text.label ==
               "Scene save dialog accepted");

    const auto canceled = mirakana::editor::make_scene_open_dialog_model(mirakana::FileDialogResult{
        .id = 23,
        .status = mirakana::FileDialogStatus::canceled,
        .paths = {},
        .selected_filter = -1,
        .error = {},
    });
    MK_REQUIRE(!canceled.accepted);
    MK_REQUIRE(canceled.status_label == "Scene open dialog canceled");

    const auto failed = mirakana::editor::make_scene_save_dialog_model(mirakana::FileDialogResult{
        .id = 24,
        .status = mirakana::FileDialogStatus::failed,
        .paths = {},
        .selected_filter = -1,
        .error = "native dialog failed",
    });
    MK_REQUIRE(!failed.accepted);
    MK_REQUIRE(failed.status_label == "Scene save dialog failed");
    MK_REQUIRE(failed.diagnostics.size() == 1);
    MK_REQUIRE(failed.diagnostics[0] == "native dialog failed");

    const auto empty_accepted = mirakana::editor::make_scene_open_dialog_model(mirakana::FileDialogResult{
        .id = 25,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!empty_accepted.accepted);
    MK_REQUIRE(empty_accepted.diagnostics.size() == 1);
    MK_REQUIRE(empty_accepted.diagnostics[0].contains("at least one path"));

    const auto multi = mirakana::editor::make_scene_save_dialog_model(mirakana::FileDialogResult{
        .id = 26,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"scenes/a.scene", "scenes/b.scene"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!multi.accepted);
    MK_REQUIRE(multi.diagnostics.size() == 1);
    MK_REQUIRE(multi.diagnostics[0].contains("exactly one"));

    const auto wrong_extension = mirakana::editor::make_scene_open_dialog_model(mirakana::FileDialogResult{
        .id = 27,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"scenes/start.prefab"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!wrong_extension.accepted);
    MK_REQUIRE(wrong_extension.diagnostics.size() == 1);
    MK_REQUIRE(wrong_extension.diagnostics[0].contains(".scene"));
}

MK_TEST("editor scene authoring document updates scene path for save as") {
    mirakana::Scene scene("Editor Scene");
    (void)scene.create_node("Player");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");

    document.set_scene_path("scenes/renamed.scene");

    MK_REQUIRE(document.scene_path() == "scenes/renamed.scene");
    const auto candidates = mirakana::editor::make_scene_package_candidate_rows(
        document, "runtime/scenes/renamed.scene", "runtime/start.geindex");
    MK_REQUIRE(!candidates.empty());
    MK_REQUIRE(candidates[0].path == "scenes/renamed.scene");
}

MK_TEST("editor project native file dialogs review results and retained rows") {
    const auto open_request = mirakana::editor::make_project_open_dialog_request(".");
    MK_REQUIRE(open_request.kind == mirakana::FileDialogKind::open_file);
    MK_REQUIRE(open_request.title == "Open Project");
    MK_REQUIRE(open_request.default_location == ".");
    MK_REQUIRE(!open_request.allow_many);
    MK_REQUIRE(open_request.filters.size() == 1);
    MK_REQUIRE(open_request.filters[0].name == "Project");
    MK_REQUIRE(open_request.filters[0].pattern == "geproject");
    MK_REQUIRE(open_request.accept_label == "Open");

    const auto save_request = mirakana::editor::make_project_save_dialog_request("games/sample/GameEngine.geproject");
    MK_REQUIRE(save_request.kind == mirakana::FileDialogKind::save_file);
    MK_REQUIRE(save_request.title == "Save Project");
    MK_REQUIRE(save_request.default_location == "games/sample/GameEngine.geproject");
    MK_REQUIRE(!save_request.allow_many);
    MK_REQUIRE(save_request.filters.size() == 1);
    MK_REQUIRE(save_request.filters[0].pattern == "geproject");
    MK_REQUIRE(save_request.accept_label == "Save");

    const auto open_accepted = mirakana::editor::make_project_open_dialog_model(mirakana::FileDialogResult{
        .id = 31,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"games/sample/GameEngine.geproject"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(open_accepted.accepted);
    MK_REQUIRE(open_accepted.status_label == "Project open dialog accepted");
    MK_REQUIRE(open_accepted.selected_path == "games/sample/GameEngine.geproject");
    MK_REQUIRE(open_accepted.rows.size() == 4);
    MK_REQUIRE(open_accepted.rows[0].id == "status");
    MK_REQUIRE(open_accepted.rows[1].id == "selected_path");
    MK_REQUIRE(open_accepted.rows[1].value == "games/sample/GameEngine.geproject");

    const auto open_ui = mirakana::editor::make_project_file_dialog_ui_model(open_accepted);
    MK_REQUIRE(open_ui.find(mirakana::ui::ElementId{"project_file_dialog.open.status.value"})->text.label ==
               "Project open dialog accepted");
    MK_REQUIRE(open_ui.find(mirakana::ui::ElementId{"project_file_dialog.open.selected_path.value"})->text.label ==
               "games/sample/GameEngine.geproject");

    const auto save_accepted = mirakana::editor::make_project_save_dialog_model(mirakana::FileDialogResult{
        .id = 32,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"games/sample/GameEngine.geproject"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(save_accepted.accepted);
    MK_REQUIRE(save_accepted.status_label == "Project save dialog accepted");
    const auto save_ui = mirakana::editor::make_project_file_dialog_ui_model(save_accepted);
    MK_REQUIRE(save_ui.find(mirakana::ui::ElementId{"project_file_dialog.save.status.value"})->text.label ==
               "Project save dialog accepted");

    const auto canceled = mirakana::editor::make_project_open_dialog_model(mirakana::FileDialogResult{
        .id = 33,
        .status = mirakana::FileDialogStatus::canceled,
        .paths = {},
        .selected_filter = -1,
        .error = {},
    });
    MK_REQUIRE(!canceled.accepted);
    MK_REQUIRE(canceled.status_label == "Project open dialog canceled");

    const auto failed = mirakana::editor::make_project_save_dialog_model(mirakana::FileDialogResult{
        .id = 34,
        .status = mirakana::FileDialogStatus::failed,
        .paths = {},
        .selected_filter = -1,
        .error = "native dialog failed",
    });
    MK_REQUIRE(!failed.accepted);
    MK_REQUIRE(failed.status_label == "Project save dialog failed");
    MK_REQUIRE(failed.diagnostics.size() == 1);
    MK_REQUIRE(failed.diagnostics[0] == "native dialog failed");

    const auto empty_accepted = mirakana::editor::make_project_open_dialog_model(mirakana::FileDialogResult{
        .id = 35,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!empty_accepted.accepted);
    MK_REQUIRE(empty_accepted.diagnostics.size() == 1);
    MK_REQUIRE(empty_accepted.diagnostics[0].contains("at least one path"));

    const auto multi = mirakana::editor::make_project_save_dialog_model(mirakana::FileDialogResult{
        .id = 36,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"a.geproject", "b.geproject"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!multi.accepted);
    MK_REQUIRE(multi.diagnostics.size() == 1);
    MK_REQUIRE(multi.diagnostics[0].contains("exactly one"));

    const auto wrong_extension = mirakana::editor::make_project_open_dialog_model(mirakana::FileDialogResult{
        .id = 37,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"games/sample/GameEngine.geworkspace"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!wrong_extension.accepted);
    MK_REQUIRE(wrong_extension.diagnostics.size() == 1);
    MK_REQUIRE(wrong_extension.diagnostics[0].contains(".geproject"));
}

MK_TEST("editor prefab variant authoring reports registry backed component override diagnostics") {
    mirakana::PrefabDefinition base;
    base.name = "ui.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Widget";
    base.nodes.push_back(root);

    auto document = mirakana::editor::PrefabVariantAuthoringDocument::from_base_prefab(
        base, "broken-widget.prefabvariant", "assets/prefabs/broken-widget.prefabvariant");

    const auto missing_mesh = mirakana::AssetId::from_name("meshes/missing");
    const auto wrong_material = mirakana::AssetId::from_name("textures/not-a-material");
    const auto missing_sprite = mirakana::AssetId::from_name("textures/missing-sprite");
    const auto valid_material = mirakana::AssetId::from_name("materials/widget");

    mirakana::SceneNodeComponents components;
    components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = missing_mesh, .material = wrong_material, .visible = true};
    components.sprite_renderer = mirakana::SpriteRendererComponent{.sprite = missing_sprite,
                                                                   .material = valid_material,
                                                                   .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
                                                                   .tint = {1.0F, 1.0F, 1.0F, 1.0F},
                                                                   .visible = true};
    MK_REQUIRE(document.set_component_override(1, components));

    mirakana::AssetRegistry registry;
    registry.add(mirakana::AssetRecord{
        .id = wrong_material, .kind = mirakana::AssetKind::texture, .path = "assets/textures/not-a-material.png"});
    registry.add(mirakana::AssetRecord{
        .id = valid_material, .kind = mirakana::AssetKind::material, .path = "assets/materials/widget.material"});

    const auto model = document.model(registry);
    MK_REQUIRE(!model.valid());
    MK_REQUIRE(model.override_rows.size() == 1);
    MK_REQUIRE(model.override_rows[0].diagnostic_count == 3);
    MK_REQUIRE(model.diagnostics.size() == 3);

    const auto has_diagnostic = [&model](mirakana::editor::PrefabVariantAuthoringDiagnosticKind kind,
                                         std::string_view field) {
        return std::ranges::any_of(model.diagnostics, [kind, field](const auto& diagnostic) {
            return diagnostic.kind == kind && diagnostic.field == field;
        });
    };
    MK_REQUIRE(
        has_diagnostic(mirakana::editor::PrefabVariantAuthoringDiagnosticKind::missing_asset, "mesh_renderer.mesh"));
    MK_REQUIRE(has_diagnostic(mirakana::editor::PrefabVariantAuthoringDiagnosticKind::wrong_asset_kind,
                              "mesh_renderer.material"));
    MK_REQUIRE(has_diagnostic(mirakana::editor::PrefabVariantAuthoringDiagnosticKind::missing_asset,
                              "sprite_renderer.sprite"));
}

MK_TEST("editor prefab variant authoring undo actions edit name transform and components") {
    mirakana::PrefabDefinition base;
    base.name = "player.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Player";
    base.nodes.push_back(root);
    mirakana::PrefabNodeTemplate body;
    body.name = "Body";
    body.parent_index = 1;
    base.nodes.push_back(body);

    auto document = mirakana::editor::PrefabVariantAuthoringDocument::from_base_prefab(
        base, "undo-player.prefabvariant", "assets/prefabs/undo-player.prefabvariant");
    mirakana::editor::UndoStack history;

    MK_REQUIRE(!history.execute(mirakana::editor::make_prefab_variant_name_override_action(document, 3, "Missing")));
    MK_REQUIRE(history.execute(mirakana::editor::make_prefab_variant_name_override_action(document, 1, "ElitePlayer")));
    MK_REQUIRE(document.variant().overrides.size() == 1);
    MK_REQUIRE(document.composed_prefab().nodes[0].name == "ElitePlayer");
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.variant().overrides.empty());
    MK_REQUIRE(history.redo());
    MK_REQUIRE(document.composed_prefab().nodes[0].name == "ElitePlayer");

    mirakana::Transform3D transform;
    transform.position = mirakana::Vec3{.x = 8.0F, .y = 9.0F, .z = 10.0F};
    MK_REQUIRE(
        history.execute(mirakana::editor::make_prefab_variant_transform_override_action(document, 2, transform)));

    mirakana::SceneNodeComponents components;
    components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mirakana::AssetId::from_name("meshes/player"),
                                        .material = mirakana::AssetId::from_name("materials/player"),
                                        .visible = false};
    MK_REQUIRE(
        history.execute(mirakana::editor::make_prefab_variant_component_override_action(document, 2, components)));
    MK_REQUIRE(document.variant().overrides.size() == 3);
    MK_REQUIRE(document.composed_prefab().nodes[1].components.mesh_renderer.has_value());

    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.variant().overrides.size() == 2);
    MK_REQUIRE(!document.composed_prefab().nodes[1].components.mesh_renderer.has_value());
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.variant().overrides.size() == 1);
    MK_REQUIRE(document.composed_prefab().nodes[1].transform.position == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(history.redo());
    MK_REQUIRE(document.composed_prefab().nodes[1].transform.position == (mirakana::Vec3{8.0F, 9.0F, 10.0F}));
}

MK_TEST("editor prefab variant conflict review reports blocking and warning rows") {
    mirakana::PrefabDefinition base;
    base.name = "player.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Player";
    base.nodes.push_back(root);
    mirakana::PrefabNodeTemplate body;
    body.name = "Body";
    body.parent_index = 1;
    body.components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mirakana::AssetId::from_name("meshes/body"),
                                        .material = mirakana::AssetId::from_name("materials/body"),
                                        .visible = true};
    base.nodes.push_back(body);

    auto document = mirakana::editor::PrefabVariantAuthoringDocument::from_base_prefab(
        base, "elite-player.prefabvariant", "assets/prefabs/elite-player.prefabvariant");
    MK_REQUIRE(document.set_name_override(1, "ElitePlayer"));
    mirakana::SceneNodeComponents clean_components;
    clean_components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mirakana::AssetId::from_name("meshes/elite-body"),
                                        .material = mirakana::AssetId::from_name("materials/elite-body"),
                                        .visible = true};
    MK_REQUIRE(document.set_component_override(2, clean_components));

    const auto clean_model = mirakana::editor::make_prefab_variant_conflict_review_model(document);
    MK_REQUIRE(clean_model.status == mirakana::editor::PrefabVariantConflictStatus::ready);
    MK_REQUIRE(clean_model.status_label == "ready");
    MK_REQUIRE(clean_model.can_compose);
    MK_REQUIRE(!clean_model.has_blocking_conflicts);
    MK_REQUIRE(clean_model.blocking_count == 0U);
    MK_REQUIRE(clean_model.warning_count == 0U);
    MK_REQUIRE(!clean_model.mutates);
    MK_REQUIRE(!clean_model.executes);
    MK_REQUIRE(clean_model.rows.size() == 2U);

    const auto* clean_name = find_prefab_variant_conflict_row(clean_model.rows, "node.1.name");
    MK_REQUIRE(clean_name != nullptr);
    MK_REQUIRE(clean_name->conflict == mirakana::editor::PrefabVariantConflictKind::clean);
    MK_REQUIRE(clean_name->status_label == "ready");
    MK_REQUIRE(clean_name->base_value == "Player");
    MK_REQUIRE(clean_name->override_value == "ElitePlayer");

    const auto clean_ui = mirakana::editor::make_prefab_variant_conflict_review_ui_model(clean_model);
    MK_REQUIRE(clean_ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts"}) != nullptr);
    MK_REQUIRE(clean_ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.rows.node.1.name.status"}) != nullptr);
    MK_REQUIRE(clean_ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.rows.node.2.components.conflict"}) !=
               nullptr);

    mirakana::PrefabVariantDefinition invalid = document.variant();
    invalid.overrides.push_back(mirakana::PrefabNodeOverride{
        .node_index = 3U,
        .kind = mirakana::PrefabOverrideKind::name,
        .name = "MissingNode",
        .transform = {},
        .components = {},
        .source_node_name = {},
    });
    invalid.overrides.push_back(mirakana::PrefabNodeOverride{
        .node_index = 1U,
        .kind = mirakana::PrefabOverrideKind::name,
        .name = "DuplicatePlayer",
        .transform = {},
        .components = {},
        .source_node_name = {},
    });
    const auto invalid_model = mirakana::editor::make_prefab_variant_conflict_review_model(invalid);
    MK_REQUIRE(invalid_model.status == mirakana::editor::PrefabVariantConflictStatus::blocked);
    MK_REQUIRE(!invalid_model.can_compose);
    MK_REQUIRE(invalid_model.has_blocking_conflicts);
    MK_REQUIRE(invalid_model.blocking_count >= 2U);
    MK_REQUIRE(find_prefab_variant_conflict_row(invalid_model.rows, "node.3.name") != nullptr);
    MK_REQUIRE(std::ranges::any_of(invalid_model.rows, [](const auto& row) {
        return row.conflict == mirakana::editor::PrefabVariantConflictKind::duplicate_override && row.blocking;
    }));
    MK_REQUIRE(std::ranges::any_of(invalid_model.rows, [](const auto& row) {
        return row.conflict == mirakana::editor::PrefabVariantConflictKind::missing_node && row.blocking;
    }));

    mirakana::PrefabVariantDefinition warning = document.variant();
    warning.overrides.clear();
    warning.overrides.push_back(mirakana::PrefabNodeOverride{.node_index = 1U,
                                                             .kind = mirakana::PrefabOverrideKind::name,
                                                             .name = "Player",
                                                             .transform = {},
                                                             .components = {},
                                                             .source_node_name = {}});
    mirakana::SceneNodeComponents replacement_components;
    replacement_components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId::from_name("textures/body"),
        .material = mirakana::AssetId::from_name("materials/body"),
        .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .tint = {1.0F, 1.0F, 1.0F, 1.0F},
        .visible = true,
    };
    mirakana::PrefabNodeOverride component_override;
    component_override.node_index = 2U;
    component_override.kind = mirakana::PrefabOverrideKind::components;
    component_override.components = replacement_components;
    warning.overrides.push_back(std::move(component_override));

    const auto warning_model = mirakana::editor::make_prefab_variant_conflict_review_model(warning);
    MK_REQUIRE(warning_model.status == mirakana::editor::PrefabVariantConflictStatus::warning);
    MK_REQUIRE(warning_model.can_compose);
    MK_REQUIRE(!warning_model.has_blocking_conflicts);
    MK_REQUIRE(warning_model.warning_count == 2U);

    const auto* redundant = find_prefab_variant_conflict_row(warning_model.rows, "node.1.name");
    MK_REQUIRE(redundant != nullptr);
    MK_REQUIRE(redundant->conflict == mirakana::editor::PrefabVariantConflictKind::redundant_override);
    MK_REQUIRE(!redundant->blocking);

    const auto* replacement = find_prefab_variant_conflict_row(warning_model.rows, "node.2.components");
    MK_REQUIRE(replacement != nullptr);
    MK_REQUIRE(replacement->conflict == mirakana::editor::PrefabVariantConflictKind::component_family_replacement);
    MK_REQUIRE(!replacement->blocking);
}

MK_TEST("editor prefab variant reviewed resolution removes redundant overrides with undo") {
    mirakana::PrefabDefinition base;
    base.name = "player.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Player";
    base.nodes.push_back(root);

    auto document = mirakana::editor::PrefabVariantAuthoringDocument::from_base_prefab(
        base, "redundant-player.prefabvariant", "assets/prefabs/redundant-player.prefabvariant");
    MK_REQUIRE(document.set_name_override(1, "Player"));

    const auto warning_model = mirakana::editor::make_prefab_variant_conflict_review_model(document);
    MK_REQUIRE(warning_model.status == mirakana::editor::PrefabVariantConflictStatus::warning);
    MK_REQUIRE(warning_model.warning_count == 1U);

    const auto* redundant = find_prefab_variant_conflict_row(warning_model.rows, "node.1.name");
    MK_REQUIRE(redundant != nullptr);
    MK_REQUIRE(redundant->conflict == mirakana::editor::PrefabVariantConflictKind::redundant_override);
    MK_REQUIRE(redundant->resolution_available);
    MK_REQUIRE(!redundant->resolution_id.empty());
    MK_REQUIRE(redundant->resolution_label == "Remove redundant override");

    const auto ui = mirakana::editor::make_prefab_variant_conflict_review_ui_model(warning_model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.rows.node.1.name.resolution"}) != nullptr);

    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(
        mirakana::editor::make_prefab_variant_conflict_resolution_action(document, redundant->resolution_id)));
    MK_REQUIRE(document.variant().overrides.empty());

    const auto resolved_model = mirakana::editor::make_prefab_variant_conflict_review_model(document);
    MK_REQUIRE(resolved_model.status == mirakana::editor::PrefabVariantConflictStatus::ready);
    MK_REQUIRE(resolved_model.warning_count == 0U);

    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.variant().overrides.size() == 1U);
    MK_REQUIRE(document.variant().overrides[0].kind == mirakana::PrefabOverrideKind::name);
    MK_REQUIRE(document.variant().overrides[0].name == "Player");
}

MK_TEST("editor prefab variant reviewed resolution exposes duplicate and missing cleanup while blocked") {
    mirakana::PrefabDefinition base;
    base.name = "player.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Player";
    base.nodes.push_back(root);

    mirakana::PrefabVariantDefinition variant;
    variant.name = "blocked-player.prefabvariant";
    variant.base_prefab = base;
    variant.overrides.push_back(mirakana::PrefabNodeOverride{
        .node_index = 1U,
        .kind = mirakana::PrefabOverrideKind::name,
        .name = "ElitePlayer",
        .transform = {},
        .components = {},
        .source_node_name = {},
    });
    variant.overrides.push_back(mirakana::PrefabNodeOverride{
        .node_index = 1U,
        .kind = mirakana::PrefabOverrideKind::name,
        .name = "DuplicatePlayer",
        .transform = {},
        .components = {},
        .source_node_name = {},
    });
    variant.overrides.push_back(mirakana::PrefabNodeOverride{
        .node_index = 2U,
        .kind = mirakana::PrefabOverrideKind::name,
        .name = "MissingNode",
        .transform = {},
        .components = {},
        .source_node_name = {},
    });

    const auto model = mirakana::editor::make_prefab_variant_conflict_review_model(variant);
    MK_REQUIRE(model.status == mirakana::editor::PrefabVariantConflictStatus::blocked);

    const auto* duplicate = find_prefab_variant_conflict_row(model.rows, "node.1.name.duplicate.2");
    MK_REQUIRE(duplicate != nullptr);
    MK_REQUIRE(duplicate->conflict == mirakana::editor::PrefabVariantConflictKind::duplicate_override);
    MK_REQUIRE(duplicate->resolution_available);
    MK_REQUIRE(duplicate->resolution_id == "remove.node.1.name.duplicate.2");
    MK_REQUIRE(duplicate->resolution_label == "Remove duplicate override");

    const auto* missing = find_prefab_variant_conflict_row(model.rows, "node.2.name");
    MK_REQUIRE(missing != nullptr);
    MK_REQUIRE(missing->conflict == mirakana::editor::PrefabVariantConflictKind::missing_node);
    MK_REQUIRE(missing->blocking);
    MK_REQUIRE(missing->resolution_available);
    MK_REQUIRE(missing->resolution_label == "Remove missing-node override");

    const auto result = mirakana::editor::resolve_prefab_variant_conflict(variant, duplicate->resolution_id);
    MK_REQUIRE(result.applied);
    MK_REQUIRE(!result.valid_after_apply);
    MK_REQUIRE(result.variant.overrides.size() == 2U);
    MK_REQUIRE(result.diagnostic.contains("validation diagnostics"));
    MK_REQUIRE(std::ranges::none_of(result.variant.overrides, [](const auto& override) {
        return override.node_index == 1U && override.kind == mirakana::PrefabOverrideKind::name &&
               override.name == "DuplicatePlayer";
    }));
}

MK_TEST("editor prefab variant missing node cleanup resolves stale raw variants") {
    mirakana::PrefabDefinition base;
    base.name = "player.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Player";
    base.nodes.push_back(root);

    mirakana::PrefabVariantDefinition variant;
    variant.name = "stale-player.prefabvariant";
    variant.base_prefab = base;
    variant.overrides.push_back(mirakana::PrefabNodeOverride{
        .node_index = 2U,
        .kind = mirakana::PrefabOverrideKind::name,
        .name = "MissingNode",
        .transform = {},
        .components = {},
        .source_node_name = {},
    });

    const auto model = mirakana::editor::make_prefab_variant_conflict_review_model(variant);
    MK_REQUIRE(model.status == mirakana::editor::PrefabVariantConflictStatus::blocked);
    MK_REQUIRE(!model.can_compose);

    const auto* missing = find_prefab_variant_conflict_row(model.rows, "node.2.name");
    MK_REQUIRE(missing != nullptr);
    MK_REQUIRE(missing->conflict == mirakana::editor::PrefabVariantConflictKind::missing_node);
    MK_REQUIRE(missing->resolution_available);
    MK_REQUIRE(missing->resolution_id == "remove.node.2.name");
    MK_REQUIRE(missing->resolution_label == "Remove missing-node override");
    MK_REQUIRE(missing->resolution_diagnostic ==
               "remove the stale override because the target node is absent from the base prefab");

    const auto ui = mirakana::editor::make_prefab_variant_conflict_review_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.rows.node.2.name.resolution"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.rows.node.2.name.resolution_diagnostic"}) !=
               nullptr);

    const auto empty_result = mirakana::editor::resolve_prefab_variant_conflict(variant, "");
    MK_REQUIRE(!empty_result.applied);
    MK_REQUIRE(empty_result.diagnostic == "prefab variant conflict resolution id is empty");

    const auto result = mirakana::editor::resolve_prefab_variant_conflict(variant, missing->resolution_id);
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.valid_after_apply);
    MK_REQUIRE(result.variant.overrides.empty());
    MK_REQUIRE(result.diagnostic == "prefab variant conflict resolution applied");
}

MK_TEST("editor prefab variant authoring loads stale variants for reviewed missing node cleanup") {
    mirakana::editor::MemoryTextStore store;
    store.write_text("assets/prefabs/stale-player.prefabvariant", "format=GameEngine.PrefabVariant.v1\n"
                                                                  "variant.name=stale-player.prefabvariant\n"
                                                                  "base.format=GameEngine.Prefab.v1\n"
                                                                  "base.prefab.name=player.prefab\n"
                                                                  "base.node.count=1\n"
                                                                  "base.node.1.name=Player\n"
                                                                  "base.node.1.parent=0\n"
                                                                  "base.node.1.position=0,0,0\n"
                                                                  "base.node.1.scale=1,1,1\n"
                                                                  "base.node.1.rotation=0,0,0\n"
                                                                  "override.count=1\n"
                                                                  "override.1.node=2\n"
                                                                  "override.1.kind=name\n"
                                                                  "override.1.name=MissingNode\n");

    auto document =
        mirakana::editor::load_prefab_variant_authoring_document(store, "assets/prefabs/stale-player.prefabvariant");
    const auto blocked_model = mirakana::editor::make_prefab_variant_conflict_review_model(document);
    const auto* missing = find_prefab_variant_conflict_row(blocked_model.rows, "node.2.name");
    MK_REQUIRE(missing != nullptr);
    MK_REQUIRE(missing->resolution_available);

    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(
        mirakana::editor::make_prefab_variant_conflict_resolution_action(document, missing->resolution_id)));
    MK_REQUIRE(document.variant().overrides.empty());
    MK_REQUIRE(document.model().valid());

    mirakana::editor::save_prefab_variant_authoring_document(store, "assets/prefabs/stale-player.prefabvariant",
                                                             document);
    MK_REQUIRE(store.read_text("assets/prefabs/stale-player.prefabvariant").contains("override.count=0"));

    MK_REQUIRE(history.undo());
    MK_REQUIRE(!document.model().valid());
    MK_REQUIRE(document.variant().overrides.size() == 1U);
    MK_REQUIRE(document.variant().overrides[0].node_index == 2U);
}

MK_TEST("editor prefab variant reviewed retarget resolves unique source node hints") {
    mirakana::editor::MemoryTextStore store;
    store.write_text("assets/prefabs/camera-retarget.prefabvariant", "format=GameEngine.PrefabVariant.v1\n"
                                                                     "variant.name=camera-retarget.prefabvariant\n"
                                                                     "base.format=GameEngine.Prefab.v1\n"
                                                                     "base.prefab.name=camera.prefab\n"
                                                                     "base.node.count=2\n"
                                                                     "base.node.1.name=Root\n"
                                                                     "base.node.1.parent=0\n"
                                                                     "base.node.1.position=0,0,0\n"
                                                                     "base.node.1.scale=1,1,1\n"
                                                                     "base.node.1.rotation=0,0,0\n"
                                                                     "base.node.2.name=Camera\n"
                                                                     "base.node.2.parent=1\n"
                                                                     "base.node.2.position=0,0,0\n"
                                                                     "base.node.2.scale=1,1,1\n"
                                                                     "base.node.2.rotation=0,0,0\n"
                                                                     "override.count=1\n"
                                                                     "override.1.node=3\n"
                                                                     "override.1.kind=transform\n"
                                                                     "override.1.source_node_name=Camera\n"
                                                                     "override.1.position=1,2,3\n"
                                                                     "override.1.scale=1,1,1\n"
                                                                     "override.1.rotation=0,0.5,0\n");

    auto document =
        mirakana::editor::load_prefab_variant_authoring_document(store, "assets/prefabs/camera-retarget.prefabvariant");
    const auto blocked_model = mirakana::editor::make_prefab_variant_conflict_review_model(document);
    MK_REQUIRE(blocked_model.status == mirakana::editor::PrefabVariantConflictStatus::blocked);
    MK_REQUIRE(!blocked_model.can_compose);

    const auto* missing = find_prefab_variant_conflict_row(blocked_model.rows, "node.3.transform");
    MK_REQUIRE(missing != nullptr);
    MK_REQUIRE(missing->conflict == mirakana::editor::PrefabVariantConflictKind::missing_node);
    MK_REQUIRE(missing->resolution_available);
    MK_REQUIRE(missing->resolution_id == "retarget.node.3.transform.to.2");
    MK_REQUIRE(missing->resolution_label == "Retarget override to node 2");
    MK_REQUIRE(missing->resolution_diagnostic == "retarget the stale override to unique source node Camera");

    const auto ui = mirakana::editor::make_prefab_variant_conflict_review_ui_model(blocked_model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.rows.node.3.transform.resolution_kind"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "prefab_variant_conflicts.rows.node.3.transform.resolution_target_node"}) != nullptr);

    const auto result = mirakana::editor::resolve_prefab_variant_conflict(document.variant(), missing->resolution_id);
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.valid_after_apply);
    MK_REQUIRE(result.variant.overrides.size() == 1U);
    MK_REQUIRE(result.variant.overrides[0].node_index == 2U);

    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(
        mirakana::editor::make_prefab_variant_conflict_resolution_action(document, missing->resolution_id)));
    MK_REQUIRE(document.model().valid());
    MK_REQUIRE(document.variant().overrides[0].node_index == 2U);

    mirakana::editor::save_prefab_variant_authoring_document(store, "assets/prefabs/camera-retarget.prefabvariant",
                                                             document);
    const auto saved = store.read_text("assets/prefabs/camera-retarget.prefabvariant");
    MK_REQUIRE(saved.contains("override.1.node=2\n"));
    MK_REQUIRE(saved.contains("override.1.source_node_name=Camera\n"));

    MK_REQUIRE(history.undo());
    MK_REQUIRE(!document.model().valid());
    MK_REQUIRE(document.variant().overrides[0].node_index == 3U);
}

MK_TEST("editor prefab variant reviewed retarget falls back when source node hints are ambiguous") {
    mirakana::editor::MemoryTextStore store;
    store.write_text("assets/prefabs/ambiguous-camera.prefabvariant", "format=GameEngine.PrefabVariant.v1\n"
                                                                      "variant.name=ambiguous-camera.prefabvariant\n"
                                                                      "base.format=GameEngine.Prefab.v1\n"
                                                                      "base.prefab.name=camera.prefab\n"
                                                                      "base.node.count=3\n"
                                                                      "base.node.1.name=Root\n"
                                                                      "base.node.1.parent=0\n"
                                                                      "base.node.1.position=0,0,0\n"
                                                                      "base.node.1.scale=1,1,1\n"
                                                                      "base.node.1.rotation=0,0,0\n"
                                                                      "base.node.2.name=Camera\n"
                                                                      "base.node.2.parent=1\n"
                                                                      "base.node.2.position=0,0,0\n"
                                                                      "base.node.2.scale=1,1,1\n"
                                                                      "base.node.2.rotation=0,0,0\n"
                                                                      "base.node.3.name=Camera\n"
                                                                      "base.node.3.parent=1\n"
                                                                      "base.node.3.position=0,0,0\n"
                                                                      "base.node.3.scale=1,1,1\n"
                                                                      "base.node.3.rotation=0,0,0\n"
                                                                      "override.count=1\n"
                                                                      "override.1.node=4\n"
                                                                      "override.1.kind=name\n"
                                                                      "override.1.source_node_name=Camera\n"
                                                                      "override.1.name=HeroCamera\n");

    const auto document = mirakana::editor::load_prefab_variant_authoring_document(
        store, "assets/prefabs/ambiguous-camera.prefabvariant");
    const auto model = mirakana::editor::make_prefab_variant_conflict_review_model(document);

    const auto* missing = find_prefab_variant_conflict_row(model.rows, "node.4.name");
    MK_REQUIRE(missing != nullptr);
    MK_REQUIRE(missing->conflict == mirakana::editor::PrefabVariantConflictKind::missing_node);
    MK_REQUIRE(missing->resolution_available);
    MK_REQUIRE(missing->resolution_id == "remove.node.4.name");
    MK_REQUIRE(missing->resolution_label == "Remove missing-node override");
    MK_REQUIRE(missing->resolution_diagnostic ==
               "remove the stale override because the target node is absent from the base prefab");
}

MK_TEST("editor prefab variant source mismatch retargets existing stale node hints") {
    mirakana::editor::MemoryTextStore store;
    store.write_text("assets/prefabs/source-mismatch-camera.prefabvariant",
                     "format=GameEngine.PrefabVariant.v1\n"
                     "variant.name=source-mismatch-camera.prefabvariant\n"
                     "base.format=GameEngine.Prefab.v1\n"
                     "base.prefab.name=camera.prefab\n"
                     "base.node.count=2\n"
                     "base.node.1.name=Enemy\n"
                     "base.node.1.parent=0\n"
                     "base.node.1.position=0,0,0\n"
                     "base.node.1.scale=1,1,1\n"
                     "base.node.1.rotation=0,0,0\n"
                     "base.node.2.name=Camera\n"
                     "base.node.2.parent=1\n"
                     "base.node.2.position=0,0,0\n"
                     "base.node.2.scale=1,1,1\n"
                     "base.node.2.rotation=0,0,0\n"
                     "override.count=1\n"
                     "override.1.node=1\n"
                     "override.1.kind=transform\n"
                     "override.1.source_node_name=Camera\n"
                     "override.1.position=1,2,3\n"
                     "override.1.scale=1,1,1\n"
                     "override.1.rotation=0,0.5,0\n");

    auto document = mirakana::editor::load_prefab_variant_authoring_document(
        store, "assets/prefabs/source-mismatch-camera.prefabvariant");
    const auto blocked_model = mirakana::editor::make_prefab_variant_conflict_review_model(document);
    MK_REQUIRE(blocked_model.status == mirakana::editor::PrefabVariantConflictStatus::blocked);
    MK_REQUIRE(!blocked_model.can_compose);
    MK_REQUIRE(blocked_model.batch_resolution.resolution_count == 1U);

    const auto* mismatch = find_prefab_variant_conflict_row(blocked_model.rows, "node.1.transform");
    MK_REQUIRE(mismatch != nullptr);
    MK_REQUIRE(mismatch->conflict == mirakana::editor::PrefabVariantConflictKind::source_node_mismatch);
    MK_REQUIRE(mismatch->blocking);
    MK_REQUIRE(mismatch->diagnostic == "override source node hint does not match the current target node");
    MK_REQUIRE(mismatch->resolution_available);
    MK_REQUIRE(mismatch->resolution_id == "retarget.node.1.transform.to.2");
    MK_REQUIRE(mismatch->resolution_label == "Retarget override to node 2");
    MK_REQUIRE(mismatch->resolution_diagnostic == "retarget the stale override to unique source node Camera");

    const auto ui = mirakana::editor::make_prefab_variant_conflict_review_ui_model(blocked_model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.rows.node.1.transform.resolution_kind"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "prefab_variant_conflicts.rows.node.1.transform.resolution_target_node"}) != nullptr);

    const auto result = mirakana::editor::resolve_prefab_variant_conflict(document.variant(), mismatch->resolution_id);
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.valid_after_apply);
    MK_REQUIRE(result.variant.overrides.size() == 1U);
    MK_REQUIRE(result.variant.overrides[0].node_index == 2U);
    MK_REQUIRE(result.variant.overrides[0].source_node_name == "Camera");

    const auto batch = mirakana::editor::resolve_prefab_variant_conflicts(document.variant());
    MK_REQUIRE(batch.applied);
    MK_REQUIRE(batch.valid_after_apply);
    MK_REQUIRE(batch.applied_count == 1U);
    MK_REQUIRE(batch.applied_resolution_ids.size() == 1U);
    MK_REQUIRE(batch.applied_resolution_ids[0] == mismatch->resolution_id);
    MK_REQUIRE(batch.variant.overrides[0].node_index == 2U);

    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(
        mirakana::editor::make_prefab_variant_conflict_resolution_action(document, mismatch->resolution_id)));
    MK_REQUIRE(document.variant().overrides[0].node_index == 2U);
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.variant().overrides[0].node_index == 1U);
}

MK_TEST("editor prefab variant source mismatch accepts current indexed node hints") {
    mirakana::editor::MemoryTextStore store;
    store.write_text("assets/prefabs/source-mismatch-renamed.prefabvariant",
                     "format=GameEngine.PrefabVariant.v1\n"
                     "variant.name=source-mismatch-renamed.prefabvariant\n"
                     "base.format=GameEngine.Prefab.v1\n"
                     "base.prefab.name=renamed.prefab\n"
                     "base.node.count=2\n"
                     "base.node.1.name=Enemy\n"
                     "base.node.1.parent=0\n"
                     "base.node.1.position=0,0,0\n"
                     "base.node.1.scale=1,1,1\n"
                     "base.node.1.rotation=0,0,0\n"
                     "base.node.2.name=Camera\n"
                     "base.node.2.parent=1\n"
                     "base.node.2.position=0,0,0\n"
                     "base.node.2.scale=1,1,1\n"
                     "base.node.2.rotation=0,0,0\n"
                     "override.count=1\n"
                     "override.1.node=1\n"
                     "override.1.kind=transform\n"
                     "override.1.source_node_name=OldEnemy\n"
                     "override.1.position=1,2,3\n"
                     "override.1.scale=1,1,1\n"
                     "override.1.rotation=0,0.5,0\n");

    auto document = mirakana::editor::load_prefab_variant_authoring_document(
        store, "assets/prefabs/source-mismatch-renamed.prefabvariant");
    const auto blocked_model = mirakana::editor::make_prefab_variant_conflict_review_model(document);
    MK_REQUIRE(blocked_model.status == mirakana::editor::PrefabVariantConflictStatus::blocked);
    MK_REQUIRE(!blocked_model.can_compose);
    MK_REQUIRE(blocked_model.batch_resolution.resolution_count == 1U);

    const auto* mismatch = find_prefab_variant_conflict_row(blocked_model.rows, "node.1.transform");
    MK_REQUIRE(mismatch != nullptr);
    MK_REQUIRE(mismatch->conflict == mirakana::editor::PrefabVariantConflictKind::source_node_mismatch);
    MK_REQUIRE(mismatch->blocking);
    MK_REQUIRE(mismatch->resolution_available);
    MK_REQUIRE(mismatch->resolution_id == "accept_current.node.1.transform");
    MK_REQUIRE(mismatch->resolution_label == "Accept current node 1");
    MK_REQUIRE(mismatch->resolution_kind == mirakana::editor::PrefabVariantConflictResolutionKind::accept_current_node);
    MK_REQUIRE(mismatch->resolution_target_node_index == 1U);
    MK_REQUIRE(mismatch->resolution_target_node_name == "Enemy");
    MK_REQUIRE(mismatch->resolution_diagnostic == "update the source node hint to current node Enemy");

    const auto ui = mirakana::editor::make_prefab_variant_conflict_review_ui_model(blocked_model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.rows.node.1.transform.resolution_kind"}) !=
               nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "prefab_variant_conflicts.rows.node.1.transform.resolution_target_node"}) != nullptr);

    const auto result = mirakana::editor::resolve_prefab_variant_conflict(document.variant(), mismatch->resolution_id);
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.valid_after_apply);
    MK_REQUIRE(result.variant.overrides.size() == 1U);
    MK_REQUIRE(result.variant.overrides[0].node_index == 1U);
    MK_REQUIRE(result.variant.overrides[0].source_node_name == "Enemy");
    const auto resolved_model = mirakana::editor::make_prefab_variant_conflict_review_model(result.variant);
    MK_REQUIRE(resolved_model.status == mirakana::editor::PrefabVariantConflictStatus::ready);
    MK_REQUIRE(resolved_model.can_compose);

    const auto batch = mirakana::editor::resolve_prefab_variant_conflicts(document.variant());
    MK_REQUIRE(batch.applied);
    MK_REQUIRE(batch.valid_after_apply);
    MK_REQUIRE(batch.applied_count == 1U);
    MK_REQUIRE(batch.applied_resolution_ids.size() == 1U);
    MK_REQUIRE(batch.applied_resolution_ids[0] == mismatch->resolution_id);
    MK_REQUIRE(batch.variant.overrides[0].node_index == 1U);
    MK_REQUIRE(batch.variant.overrides[0].source_node_name == "Enemy");

    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(
        mirakana::editor::make_prefab_variant_conflict_resolution_action(document, mismatch->resolution_id)));
    MK_REQUIRE(document.variant().overrides[0].node_index == 1U);
    MK_REQUIRE(document.variant().overrides[0].source_node_name == "Enemy");
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.variant().overrides[0].node_index == 1U);
    MK_REQUIRE(document.variant().overrides[0].source_node_name == "OldEnemy");
}

MK_TEST("editor prefab variant batch resolution applies reviewed rows with one undo") {
    mirakana::editor::MemoryTextStore store;
    store.write_text("assets/prefabs/batch-camera.prefabvariant", "format=GameEngine.PrefabVariant.v1\n"
                                                                  "variant.name=batch-camera.prefabvariant\n"
                                                                  "base.format=GameEngine.Prefab.v1\n"
                                                                  "base.prefab.name=camera.prefab\n"
                                                                  "base.node.count=2\n"
                                                                  "base.node.1.name=Player\n"
                                                                  "base.node.1.parent=0\n"
                                                                  "base.node.1.position=0,0,0\n"
                                                                  "base.node.1.scale=1,1,1\n"
                                                                  "base.node.1.rotation=0,0,0\n"
                                                                  "base.node.2.name=Camera\n"
                                                                  "base.node.2.parent=1\n"
                                                                  "base.node.2.position=0,0,0\n"
                                                                  "base.node.2.scale=1,1,1\n"
                                                                  "base.node.2.rotation=0,0,0\n"
                                                                  "override.count=4\n"
                                                                  "override.1.node=1\n"
                                                                  "override.1.kind=name\n"
                                                                  "override.1.source_node_name=Player\n"
                                                                  "override.1.name=HeroPlayer\n"
                                                                  "override.2.node=1\n"
                                                                  "override.2.kind=name\n"
                                                                  "override.2.source_node_name=Player\n"
                                                                  "override.2.name=DuplicatePlayer\n"
                                                                  "override.3.node=3\n"
                                                                  "override.3.kind=transform\n"
                                                                  "override.3.source_node_name=Camera\n"
                                                                  "override.3.position=1,2,3\n"
                                                                  "override.3.scale=1,1,1\n"
                                                                  "override.3.rotation=0,0.5,0\n"
                                                                  "override.4.node=2\n"
                                                                  "override.4.kind=name\n"
                                                                  "override.4.source_node_name=Camera\n"
                                                                  "override.4.name=Camera\n");

    auto document =
        mirakana::editor::load_prefab_variant_authoring_document(store, "assets/prefabs/batch-camera.prefabvariant");
    const auto blocked_model = mirakana::editor::make_prefab_variant_conflict_review_model(document);
    MK_REQUIRE(blocked_model.status == mirakana::editor::PrefabVariantConflictStatus::blocked);
    MK_REQUIRE(blocked_model.batch_resolution.available);
    MK_REQUIRE(blocked_model.batch_resolution.id == "apply_all_reviewed_resolutions");
    MK_REQUIRE(blocked_model.batch_resolution.label == "Apply All Reviewed Resolutions");
    MK_REQUIRE(blocked_model.batch_resolution.resolution_count == 3U);
    MK_REQUIRE(blocked_model.batch_resolution.blocking_resolution_count == 2U);
    MK_REQUIRE(blocked_model.batch_resolution.warning_resolution_count == 1U);
    MK_REQUIRE(blocked_model.batch_resolution.mutates);
    MK_REQUIRE(!blocked_model.batch_resolution.executes);
    MK_REQUIRE(blocked_model.batch_resolution.resolution_ids.size() == 3U);
    MK_REQUIRE(blocked_model.batch_resolution.resolution_ids[0] == "remove.node.1.name.duplicate.2");
    MK_REQUIRE(blocked_model.batch_resolution.resolution_ids[1] == "retarget.node.3.transform.to.2");
    MK_REQUIRE(blocked_model.batch_resolution.resolution_ids[2] == "remove.node.2.name");

    const auto ui = mirakana::editor::make_prefab_variant_conflict_review_ui_model(blocked_model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.batch_resolution"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.batch_resolution.apply_all"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.batch_resolution.count"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_conflicts.batch_resolution.diagnostic"}) != nullptr);

    const auto result = mirakana::editor::resolve_prefab_variant_conflicts(document.variant());
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.valid_after_apply);
    MK_REQUIRE(result.applied_count == 3U);
    MK_REQUIRE(result.remaining_available_count == 0U);
    MK_REQUIRE(result.remaining_blocking_count == 0U);
    MK_REQUIRE(result.applied_resolution_ids == blocked_model.batch_resolution.resolution_ids);
    MK_REQUIRE(result.variant.overrides.size() == 2U);
    MK_REQUIRE(std::ranges::any_of(result.variant.overrides, [](const auto& override) {
        return override.node_index == 1U && override.kind == mirakana::PrefabOverrideKind::name &&
               override.name == "HeroPlayer";
    }));
    MK_REQUIRE(std::ranges::any_of(result.variant.overrides, [](const auto& override) {
        return override.node_index == 2U && override.kind == mirakana::PrefabOverrideKind::transform &&
               override.source_node_name == "Camera";
    }));

    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_prefab_variant_conflict_batch_resolution_action(document)));
    MK_REQUIRE(document.model().valid());
    MK_REQUIRE(document.variant().overrides.size() == 2U);

    MK_REQUIRE(history.undo());
    MK_REQUIRE(!document.model().valid());
    MK_REQUIRE(document.variant().overrides.size() == 4U);
}

MK_TEST("editor prefab variant base refresh retargets overrides by source node names") {
    mirakana::PrefabDefinition base;
    base.name = "lighting.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Root";
    base.nodes.push_back(root);
    mirakana::PrefabNodeTemplate camera;
    camera.name = "Camera";
    camera.parent_index = 1;
    base.nodes.push_back(camera);
    mirakana::PrefabNodeTemplate light;
    light.name = "Light";
    light.parent_index = 1;
    base.nodes.push_back(light);

    auto document = mirakana::editor::PrefabVariantAuthoringDocument::from_base_prefab(
        base, "lighting-variant.prefabvariant", "assets/prefabs/lighting-variant.prefabvariant");
    mirakana::Transform3D camera_transform;
    camera_transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};
    MK_REQUIRE(document.set_transform_override(2, camera_transform));
    MK_REQUIRE(document.set_name_override(3, "KeyLight"));
    mirakana::SceneNodeComponents camera_components;
    camera_components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mirakana::AssetId::from_name("meshes/camera"),
                                        .material = mirakana::AssetId::from_name("materials/camera"),
                                        .visible = true};
    MK_REQUIRE(document.set_component_override(2, camera_components));
    MK_REQUIRE(document.variant().overrides[0].source_node_name == "Camera");
    MK_REQUIRE(document.variant().overrides[1].source_node_name == "Light");
    MK_REQUIRE(document.variant().overrides[2].source_node_name == "Camera");

    mirakana::PrefabDefinition refreshed = base;
    refreshed.name = "lighting-refreshed.prefab";
    mirakana::PrefabNodeTemplate inserted;
    inserted.name = "Inserted";
    inserted.parent_index = 1;
    refreshed.nodes.insert(refreshed.nodes.begin() + 1, inserted);

    const auto plan = mirakana::editor::plan_prefab_variant_base_refresh(document, refreshed);
    MK_REQUIRE(plan.status == mirakana::editor::PrefabVariantBaseRefreshStatus::warning);
    MK_REQUIRE(plan.status_label == "warning");
    MK_REQUIRE(plan.variant_name == "lighting-variant.prefabvariant");
    MK_REQUIRE(plan.current_base_prefab_name == "lighting.prefab");
    MK_REQUIRE(plan.refreshed_base_prefab_name == "lighting-refreshed.prefab");
    MK_REQUIRE(plan.can_apply);
    MK_REQUIRE(plan.mutates);
    MK_REQUIRE(!plan.executes);
    MK_REQUIRE(plan.row_count == 3U);
    MK_REQUIRE(plan.retarget_count == 3U);
    MK_REQUIRE(plan.blocking_count == 0U);
    MK_REQUIRE(plan.warning_count == 3U);

    const auto* camera_row = find_prefab_variant_base_refresh_row(plan.rows, "override.1.node.2.transform");
    MK_REQUIRE(camera_row != nullptr);
    MK_REQUIRE(camera_row->old_node_index == 2U);
    MK_REQUIRE(camera_row->old_node_name == "Camera");
    MK_REQUIRE(camera_row->source_node_name == "Camera");
    MK_REQUIRE(camera_row->refreshed_node_index == 3U);
    MK_REQUIRE(camera_row->refreshed_node_name == "Camera");
    MK_REQUIRE(camera_row->override_kind == mirakana::PrefabOverrideKind::transform);
    MK_REQUIRE(camera_row->kind == mirakana::editor::PrefabVariantBaseRefreshRowKind::retarget_by_source_name);
    MK_REQUIRE(camera_row->status == mirakana::editor::PrefabVariantBaseRefreshStatus::warning);
    MK_REQUIRE(!camera_row->blocking);

    const auto* light_row = find_prefab_variant_base_refresh_row(plan.rows, "override.2.node.3.name");
    MK_REQUIRE(light_row != nullptr);
    MK_REQUIRE(light_row->refreshed_node_index == 4U);
    MK_REQUIRE(light_row->refreshed_node_name == "Light");

    const auto* component_row = find_prefab_variant_base_refresh_row(plan.rows, "override.3.node.2.components");
    MK_REQUIRE(component_row != nullptr);
    MK_REQUIRE(component_row->refreshed_node_index == 3U);
    MK_REQUIRE(component_row->override_kind == mirakana::PrefabOverrideKind::components);
    MK_REQUIRE(component_row->kind == mirakana::editor::PrefabVariantBaseRefreshRowKind::retarget_by_source_name);
    MK_REQUIRE(!component_row->blocking);

    const auto ui = mirakana::editor::make_prefab_variant_base_refresh_ui_model(plan);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_base_refresh"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"prefab_variant_base_refresh.apply"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "prefab_variant_base_refresh.rows.override.1.node.2.transform.status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "prefab_variant_base_refresh.rows.override.1.node.2.transform.target_node"}) != nullptr);

    const auto result = mirakana::editor::apply_prefab_variant_base_refresh(document.variant(), refreshed);
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.valid_after_apply);
    MK_REQUIRE(result.retargeted_count == 3U);
    MK_REQUIRE(result.diagnostic == "prefab variant base refresh applied");
    MK_REQUIRE(result.variant.base_prefab.name == "lighting-refreshed.prefab");
    MK_REQUIRE(result.variant.overrides[0].node_index == 3U);
    MK_REQUIRE(result.variant.overrides[0].source_node_name == "Camera");
    MK_REQUIRE(result.variant.overrides[1].node_index == 4U);
    MK_REQUIRE(result.variant.overrides[1].source_node_name == "Light");
    MK_REQUIRE(result.variant.overrides[2].node_index == 3U);
    MK_REQUIRE(result.variant.overrides[2].kind == mirakana::PrefabOverrideKind::components);
    MK_REQUIRE(result.variant.overrides[2].source_node_name == "Camera");
    const auto refreshed_conflicts = mirakana::editor::make_prefab_variant_conflict_review_model(result.variant);
    MK_REQUIRE(refreshed_conflicts.status == mirakana::editor::PrefabVariantConflictStatus::ready);
    MK_REQUIRE(refreshed_conflicts.can_compose);

    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_prefab_variant_base_refresh_action(document, refreshed)));
    MK_REQUIRE(document.variant().base_prefab.name == "lighting-refreshed.prefab");
    MK_REQUIRE(document.variant().overrides[0].node_index == 3U);
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.variant().base_prefab.name == "lighting.prefab");
    MK_REQUIRE(document.variant().overrides[0].node_index == 2U);
    MK_REQUIRE(document.variant().overrides[1].node_index == 3U);
    MK_REQUIRE(document.variant().overrides[2].node_index == 2U);
    MK_REQUIRE(history.redo());
    MK_REQUIRE(document.variant().base_prefab.name == "lighting-refreshed.prefab");
    MK_REQUIRE(document.variant().overrides[2].node_index == 3U);
}

MK_TEST("editor prefab variant base refresh blocks unsafe source mappings") {
    mirakana::PrefabDefinition base;
    base.name = "camera.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Root";
    base.nodes.push_back(root);
    mirakana::PrefabNodeTemplate camera;
    camera.name = "Camera";
    camera.parent_index = 1;
    base.nodes.push_back(camera);

    auto document = mirakana::editor::PrefabVariantAuthoringDocument::from_base_prefab(
        base, "camera-variant.prefabvariant", "assets/prefabs/camera-variant.prefabvariant");
    mirakana::Transform3D transform;
    transform.position = mirakana::Vec3{.x = 4.0F, .y = 5.0F, .z = 6.0F};
    MK_REQUIRE(document.set_transform_override(2, transform));

    mirakana::PrefabDefinition ambiguous = base;
    ambiguous.name = "camera-ambiguous.prefab";
    mirakana::PrefabNodeTemplate duplicate_camera;
    duplicate_camera.name = "Camera";
    duplicate_camera.parent_index = 1;
    ambiguous.nodes.push_back(duplicate_camera);

    const auto ambiguous_plan = mirakana::editor::plan_prefab_variant_base_refresh(document, ambiguous);
    MK_REQUIRE(ambiguous_plan.status == mirakana::editor::PrefabVariantBaseRefreshStatus::blocked);
    MK_REQUIRE(!ambiguous_plan.can_apply);
    MK_REQUIRE(ambiguous_plan.blocking_count == 1U);
    const auto* ambiguous_row =
        find_prefab_variant_base_refresh_row(ambiguous_plan.rows, "override.1.node.2.transform");
    MK_REQUIRE(ambiguous_row != nullptr);
    MK_REQUIRE(ambiguous_row->kind == mirakana::editor::PrefabVariantBaseRefreshRowKind::ambiguous_source_node);
    MK_REQUIRE(ambiguous_row->blocking);
    const auto ambiguous_result = mirakana::editor::apply_prefab_variant_base_refresh(document.variant(), ambiguous);
    MK_REQUIRE(!ambiguous_result.applied);
    MK_REQUIRE(!ambiguous_result.valid_after_apply);

    mirakana::PrefabDefinition missing = base;
    missing.name = "camera-missing.prefab";
    missing.nodes[1].name = "LookAt";
    const auto missing_plan = mirakana::editor::plan_prefab_variant_base_refresh(document, missing);
    MK_REQUIRE(missing_plan.status == mirakana::editor::PrefabVariantBaseRefreshStatus::blocked);
    const auto* missing_row = find_prefab_variant_base_refresh_row(missing_plan.rows, "override.1.node.2.transform");
    MK_REQUIRE(missing_row != nullptr);
    MK_REQUIRE(missing_row->kind == mirakana::editor::PrefabVariantBaseRefreshRowKind::missing_source_node);

    auto source_less = document.variant();
    source_less.overrides[0].source_node_name.clear();
    const auto source_less_plan = mirakana::editor::plan_prefab_variant_base_refresh(source_less, base);
    MK_REQUIRE(source_less_plan.status == mirakana::editor::PrefabVariantBaseRefreshStatus::blocked);
    const auto* source_less_row =
        find_prefab_variant_base_refresh_row(source_less_plan.rows, "override.1.node.2.transform");
    MK_REQUIRE(source_less_row != nullptr);
    MK_REQUIRE(source_less_row->kind == mirakana::editor::PrefabVariantBaseRefreshRowKind::missing_source_node_hint);

    mirakana::PrefabDefinition invalid_base;
    invalid_base.name = "";
    const auto invalid_plan = mirakana::editor::plan_prefab_variant_base_refresh(document, invalid_base);
    MK_REQUIRE(invalid_plan.status == mirakana::editor::PrefabVariantBaseRefreshStatus::blocked);
    MK_REQUIRE(!invalid_plan.can_apply);
    MK_REQUIRE(invalid_plan.row_count == 0U);
    MK_REQUIRE(!invalid_plan.diagnostics.empty());
}

MK_TEST("editor prefab variant base refresh blocks duplicate target override keys") {
    mirakana::PrefabDefinition base;
    base.name = "duplicate-source.prefab";
    mirakana::PrefabNodeTemplate first;
    first.name = "Shared";
    base.nodes.push_back(first);
    mirakana::PrefabNodeTemplate second;
    second.name = "Shared";
    second.parent_index = 1;
    base.nodes.push_back(second);

    mirakana::PrefabVariantDefinition variant;
    variant.name = "duplicate-source.prefabvariant";
    variant.base_prefab = base;
    mirakana::PrefabNodeOverride first_override;
    first_override.node_index = 1U;
    first_override.kind = mirakana::PrefabOverrideKind::transform;
    first_override.source_node_name = "Shared";
    first_override.transform.position = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    variant.overrides.push_back(first_override);
    mirakana::PrefabNodeOverride second_override = first_override;
    second_override.node_index = 2U;
    second_override.transform.position = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F};
    variant.overrides.push_back(second_override);

    mirakana::PrefabDefinition refreshed;
    refreshed.name = "deduplicated-source.prefab";
    mirakana::PrefabNodeTemplate shared;
    shared.name = "Shared";
    refreshed.nodes.push_back(shared);

    const auto plan = mirakana::editor::plan_prefab_variant_base_refresh(variant, refreshed);
    MK_REQUIRE(plan.status == mirakana::editor::PrefabVariantBaseRefreshStatus::blocked);
    MK_REQUIRE(!plan.can_apply);
    MK_REQUIRE(plan.row_count == 2U);
    MK_REQUIRE(plan.blocking_count == 1U);

    const auto* duplicate_row = find_prefab_variant_base_refresh_row(plan.rows, "override.2.node.2.transform");
    MK_REQUIRE(duplicate_row != nullptr);
    MK_REQUIRE(duplicate_row->kind == mirakana::editor::PrefabVariantBaseRefreshRowKind::duplicate_target_override);
    MK_REQUIRE(duplicate_row->refreshed_node_index == 1U);
    MK_REQUIRE(duplicate_row->blocking);

    const auto result = mirakana::editor::apply_prefab_variant_base_refresh(variant, refreshed);
    MK_REQUIRE(!result.applied);
    MK_REQUIRE(!result.valid_after_apply);
    MK_REQUIRE(result.variant.base_prefab.name == "duplicate-source.prefab");
}

MK_TEST("editor scene authoring keeps selection on the same node after deleting an earlier node") {
    mirakana::Scene scene("Selection Remap");
    const auto first = scene.create_node("First");
    const auto removed = scene.create_node("Removed");
    const auto selected = scene.create_node("Selected");
    const auto last = scene.create_node("Last");

    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/selection.scene");
    MK_REQUIRE(document.select_node(selected));

    mirakana::editor::UndoStack history;
    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_delete_node_action(document, removed)));
    MK_REQUIRE(document.scene().nodes().size() == 3);
    MK_REQUIRE(document.scene().find_node(document.selected_node())->name == "Selected");
    MK_REQUIRE(document.selected_node().value == 2);
    MK_REQUIRE(document.scene().nodes()[0].id == first);
    MK_REQUIRE(document.scene().nodes()[2].name == "Last");

    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().nodes().size() == 4);
    MK_REQUIRE(document.selected_node() == selected);
    MK_REQUIRE(document.scene().find_node(last)->name == "Last");
}

MK_TEST("editor scene authoring hierarchy rows tolerate corrupted child cycles") {
    mirakana::Scene scene("Corrupted Hierarchy");
    const auto root = scene.create_node("Root");
    const auto child = scene.create_node("Child");
    scene.set_parent(child, root);
    scene.find_node(child)->children.push_back(root);

    const auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/corrupted.scene");
    const auto rows = document.hierarchy_rows();

    MK_REQUIRE(rows.size() == 2);
    MK_REQUIRE(rows[0].node == root);
    MK_REQUIRE(rows[1].node == child);
}

MK_TEST("editor scene authoring undo actions tolerate corrupted child cycles") {
    mirakana::Scene scene("Corrupted Undo Hierarchy");
    const auto root = scene.create_node("Root");
    const auto child = scene.create_node("Child");
    scene.set_parent(child, root);
    scene.find_node(child)->children.push_back(root);

    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/corrupted-undo.scene");
    MK_REQUIRE(document.select_node(root));

    mirakana::editor::UndoStack history;
    MK_REQUIRE(!history.execute(mirakana::editor::make_scene_authoring_reparent_node_action(document, child, root)));
    MK_REQUIRE(document.scene().nodes().size() == 2);
    MK_REQUIRE(document.selected_node() == root);

    MK_REQUIRE(history.execute(mirakana::editor::make_scene_authoring_delete_node_action(document, child)));
    MK_REQUIRE(document.scene().nodes().empty());
    MK_REQUIRE(document.selected_node() == mirakana::null_scene_node);
    MK_REQUIRE(history.undo());
    MK_REQUIRE(document.scene().nodes().size() == 2);
    MK_REQUIRE(document.selected_node() == root);
}

MK_TEST("editor scene authoring validates asset references and package candidate rows") {
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto sprite_id = mirakana::AssetId::from_name("sprites/player");

    mirakana::Scene scene("Validation Scene");
    const auto node = scene.create_node("Renderable");
    mirakana::SceneNodeComponents components;
    components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mesh_id, .material = material_id, .visible = true};
    components.sprite_renderer = mirakana::SpriteRendererComponent{.sprite = sprite_id,
                                                                   .material = material_id,
                                                                   .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
                                                                   .tint = {1.0F, 1.0F, 1.0F, 1.0F},
                                                                   .visible = true};
    scene.set_components(node, components);

    mirakana::AssetRegistry registry;
    registry.add(
        mirakana::AssetRecord{.id = mesh_id, .kind = mirakana::AssetKind::mesh, .path = "assets/meshes/player.mesh"});
    registry.add(mirakana::AssetRecord{
        .id = sprite_id, .kind = mirakana::AssetKind::material, .path = "assets/materials/not-a-texture.material"});

    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "assets/scenes/level.scene");
    const auto diagnostics = mirakana::editor::validate_scene_authoring_references(document.scene(), registry);
    auto has_diagnostic = [&diagnostics](mirakana::AssetId asset, std::string_view field, std::string_view text) {
        return std::ranges::any_of(diagnostics,
                                   [asset, field, text](const mirakana::editor::SceneAuthoringDiagnostic& row) {
                                       return row.asset == asset && row.field == field && row.diagnostic.contains(text);
                                   });
    };

    MK_REQUIRE(diagnostics.size() == 3);
    MK_REQUIRE(has_diagnostic(material_id, "mesh_renderer.material", "missing"));
    MK_REQUIRE(has_diagnostic(material_id, "sprite_renderer.material", "missing"));
    MK_REQUIRE(has_diagnostic(sprite_id, "sprite_renderer.sprite", "texture"));

    const auto rows = mirakana::editor::make_scene_package_candidate_rows(
        document, "assets/cooked/level.scene", "assets/package.geindex", {"assets/prefabs/player.prefab"});
    MK_REQUIRE(rows.size() == 4);
    MK_REQUIRE(rows[0].kind == mirakana::editor::ScenePackageCandidateKind::scene_source);
    MK_REQUIRE(rows[0].path == "assets/scenes/level.scene");
    MK_REQUIRE(!rows[0].runtime_file);
    MK_REQUIRE(rows[1].kind == mirakana::editor::ScenePackageCandidateKind::scene_cooked);
    MK_REQUIRE(rows[1].runtime_file);
    MK_REQUIRE(rows[2].kind == mirakana::editor::ScenePackageCandidateKind::package_index);
    MK_REQUIRE(rows[3].kind == mirakana::editor::ScenePackageCandidateKind::prefab_source);
}

MK_TEST("editor scene package registration draft classifies additions and existing entries") {
    mirakana::Scene scene("Package Draft Scene");
    const auto root = scene.create_node("Root");
    MK_REQUIRE(root != mirakana::null_scene_node);
    const auto document =
        mirakana::editor::SceneAuthoringDocument::from_scene(scene, "games/sample/assets/scenes/start.scene");

    const auto candidates = mirakana::editor::make_scene_package_candidate_rows(
        document, "games/sample/runtime/scenes/start.scene", "games/sample/runtime/start.geindex",
        {"games/sample/assets/prefabs/root.prefab"});
    const auto rows = mirakana::editor::make_scene_package_registration_draft_rows(candidates, "games/sample",
                                                                                   {"runtime/start.geindex"});

    MK_REQUIRE(rows.size() == 4);
    MK_REQUIRE(rows[0].kind == mirakana::editor::ScenePackageCandidateKind::scene_source);
    MK_REQUIRE(rows[0].candidate_path == "games/sample/assets/scenes/start.scene");
    MK_REQUIRE(rows[0].runtime_package_path == "assets/scenes/start.scene");
    MK_REQUIRE(rows[0].status == mirakana::editor::ScenePackageRegistrationDraftStatus::rejected_source_file);
    MK_REQUIRE(rows[0].diagnostic.contains("source"));

    MK_REQUIRE(rows[1].kind == mirakana::editor::ScenePackageCandidateKind::scene_cooked);
    MK_REQUIRE(rows[1].runtime_file);
    MK_REQUIRE(rows[1].runtime_package_path == "runtime/scenes/start.scene");
    MK_REQUIRE(rows[1].status == mirakana::editor::ScenePackageRegistrationDraftStatus::add_runtime_file);
    MK_REQUIRE(rows[1].diagnostic.contains("add"));

    MK_REQUIRE(rows[2].kind == mirakana::editor::ScenePackageCandidateKind::package_index);
    MK_REQUIRE(rows[2].runtime_package_path == "runtime/start.geindex");
    MK_REQUIRE(rows[2].status == mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered);
    MK_REQUIRE(rows[2].diagnostic.contains("registered"));

    MK_REQUIRE(rows[3].kind == mirakana::editor::ScenePackageCandidateKind::prefab_source);
    MK_REQUIRE(rows[3].status == mirakana::editor::ScenePackageRegistrationDraftStatus::rejected_source_file);
    MK_REQUIRE(mirakana::editor::scene_package_registration_draft_status_label(rows[1].status) == "add_runtime_file");
    MK_REQUIRE(mirakana::editor::scene_package_registration_draft_status_label(rows[2].status) == "already_registered");
}

MK_TEST("editor scene package registration draft rejects unsafe and duplicate runtime entries") {
    const std::vector<mirakana::editor::ScenePackageCandidateRow> candidates{
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .path = "games/sample/runtime/scenes/start.scene",
         .runtime_file = true},
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .path = "games/sample/runtime/scenes/start.scene",
         .runtime_file = true},
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .path = "../runtime/escape.scene",
         .runtime_file = true},
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .path = "C:/outside.geindex",
         .runtime_file = true},
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .path = "games/sample/runtime/bad\n.scene",
         .runtime_file = true},
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .path = "games/sample/runtime/nested/ok.geindex",
         .runtime_file = true},
    };

    const auto rows = mirakana::editor::make_scene_package_registration_draft_rows(candidates, "games/sample", {});

    MK_REQUIRE(rows.size() == 6);
    MK_REQUIRE(rows[0].runtime_package_path == "runtime/scenes/start.scene");
    MK_REQUIRE(rows[0].status == mirakana::editor::ScenePackageRegistrationDraftStatus::add_runtime_file);
    MK_REQUIRE(rows[1].status == mirakana::editor::ScenePackageRegistrationDraftStatus::rejected_duplicate);
    MK_REQUIRE(rows[1].diagnostic.contains("duplicate"));
    MK_REQUIRE(rows[2].status == mirakana::editor::ScenePackageRegistrationDraftStatus::rejected_unsafe_path);
    MK_REQUIRE(rows[3].status == mirakana::editor::ScenePackageRegistrationDraftStatus::rejected_unsafe_path);
    MK_REQUIRE(rows[4].status == mirakana::editor::ScenePackageRegistrationDraftStatus::rejected_unsafe_path);
    MK_REQUIRE(rows[5].runtime_package_path == "runtime/nested/ok.geindex");
    MK_REQUIRE(rows[5].status == mirakana::editor::ScenePackageRegistrationDraftStatus::add_runtime_file);
    MK_REQUIRE(mirakana::editor::scene_package_registration_draft_status_label(rows[1].status) == "rejected_duplicate");
    MK_REQUIRE(mirakana::editor::scene_package_candidate_kind_label(rows[5].kind) == "package_index");
}

MK_TEST("editor scene package registration draft strips project root case-insensitively") {
    const std::vector<mirakana::editor::ScenePackageCandidateRow> candidates{
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .path = "Games/Sample/runtime/start.geindex",
         .runtime_file = true},
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .path = "Games/Sample/runtime/scenes/start.scene",
         .runtime_file = true},
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .path = "Games/Other/runtime/scenes/outside.scene",
         .runtime_file = true},
    };
    const std::vector<std::string> existing_package_files{"runtime/start.geindex"};

    const auto rows = mirakana::editor::make_scene_package_registration_draft_rows(candidates, "games/sample",
                                                                                   existing_package_files);

    MK_REQUIRE(rows.size() == 3);
    MK_REQUIRE(rows[0].runtime_package_path == "runtime/start.geindex");
    MK_REQUIRE(rows[0].status == mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered);
    MK_REQUIRE(rows[1].runtime_package_path == "runtime/scenes/start.scene");
    MK_REQUIRE(rows[1].status == mirakana::editor::ScenePackageRegistrationDraftStatus::add_runtime_file);
    MK_REQUIRE(rows[2].status == mirakana::editor::ScenePackageRegistrationDraftStatus::rejected_unsafe_path);
}

MK_TEST("editor scene package registration apply plan filters reviewed additions") {
    const std::vector<mirakana::editor::ScenePackageRegistrationDraftRow> rows{
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .candidate_path = "games/sample/runtime/scenes/start.scene",
         .runtime_package_path = "runtime/scenes/start.scene",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::add_runtime_file,
         .diagnostic = "add runtimePackageFiles entry"},
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .candidate_path = "games/sample/runtime/start.geindex",
         .runtime_package_path = "runtime/start.geindex",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         .diagnostic = "runtime file is already registered"},
        {.kind = mirakana::editor::ScenePackageCandidateKind::prefab_source,
         .candidate_path = "games/sample/assets/prefabs/root.prefab",
         .runtime_package_path = "assets/prefabs/root.prefab",
         .runtime_file = false,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::rejected_source_file,
         .diagnostic = "source asset is not a runtime package file"},
    };

    const auto plan =
        mirakana::editor::make_scene_package_registration_apply_plan(rows, "games/sample", "game.agent.json");

    MK_REQUIRE(plan.can_apply);
    MK_REQUIRE(plan.game_manifest_path == "games/sample/game.agent.json");
    MK_REQUIRE(plan.runtime_package_files.size() == 1);
    MK_REQUIRE(plan.runtime_package_files[0] == "runtime/scenes/start.scene");

    const auto no_changes =
        mirakana::editor::make_scene_package_registration_apply_plan(rows, "games/sample", "../game.agent.json");
    MK_REQUIRE(!no_changes.can_apply);
    MK_REQUIRE(no_changes.diagnostic.contains("unsafe"));
}

MK_TEST("editor scene package registration apply updates manifest runtime files") {
    mirakana::editor::MemoryTextStore store;
    store.write_text("games/sample/game.agent.json", "{\n"
                                                     "  \"schemaVersion\": 1,\n"
                                                     "  \"name\": \"sample\",\n"
                                                     "  \"runtimePackageFiles\": [\n"
                                                     "    \"runtime/start.geindex\"\n"
                                                     "  ],\n"
                                                     "  \"validationRecipes\": []\n"
                                                     "}\n");

    const mirakana::editor::ScenePackageRegistrationApplyPlan plan{
        .game_manifest_path = "games/sample/game.agent.json",
        .runtime_package_files = {"runtime/scenes/start.scene", "runtime/start.geindex"},
        .can_apply = true,
        .diagnostic = {},
    };

    const auto result = mirakana::editor::apply_scene_package_registration_to_manifest(store, plan);

    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.runtime_package_files.size() == 2);
    MK_REQUIRE(result.runtime_package_files[0] == "runtime/start.geindex");
    MK_REQUIRE(result.runtime_package_files[1] == "runtime/scenes/start.scene");
    const auto manifest = store.read_text("games/sample/game.agent.json");
    MK_REQUIRE(manifest.contains("\"runtimePackageFiles\": [\n"
                                 "    \"runtime/start.geindex\",\n"
                                 "    \"runtime/scenes/start.scene\"\n"
                                 "  ]"));
    MK_REQUIRE(manifest.contains("\"validationRecipes\": []"));

    const auto idempotent = mirakana::editor::apply_scene_package_registration_to_manifest(store, plan);
    MK_REQUIRE(idempotent.applied);
    MK_REQUIRE(idempotent.runtime_package_files.size() == 2);
    const auto idempotent_manifest = store.read_text("games/sample/game.agent.json");
    MK_REQUIRE(idempotent_manifest.find("runtime/scenes/start.scene") ==
               idempotent_manifest.rfind("runtime/scenes/start.scene"));
}

MK_TEST("editor scene package registration apply creates property and rejects malformed manifests") {
    mirakana::editor::MemoryTextStore missing_array_store;
    missing_array_store.write_text("games/sample/game.agent.json", "{\n"
                                                                   "  \"schemaVersion\": 1,\n"
                                                                   "  \"name\": \"sample\"\n"
                                                                   "}\n");
    const mirakana::editor::ScenePackageRegistrationApplyPlan plan{
        .game_manifest_path = "games/sample/game.agent.json",
        .runtime_package_files = {"runtime/start.geindex"},
        .can_apply = true,
        .diagnostic = {},
    };

    const auto created = mirakana::editor::apply_scene_package_registration_to_manifest(missing_array_store, plan);
    MK_REQUIRE(created.applied);
    MK_REQUIRE(created.runtime_package_files.size() == 1);
    MK_REQUIRE(missing_array_store.read_text("games/sample/game.agent.json")
                   .contains("\"runtimePackageFiles\": [\n"
                             "    \"runtime/start.geindex\"\n"
                             "  ]"));

    mirakana::editor::MemoryTextStore malformed_store;
    malformed_store.write_text("games/sample/game.agent.json", "{\n"
                                                               "  \"schemaVersion\": 1,\n"
                                                               "  \"runtimePackageFiles\": \"runtime/start.geindex\"\n"
                                                               "}\n");
    const auto rejected = mirakana::editor::apply_scene_package_registration_to_manifest(malformed_store, plan);
    MK_REQUIRE(!rejected.applied);
    MK_REQUIRE(rejected.diagnostic.contains("array"));
    MK_REQUIRE(malformed_store.read_text("games/sample/game.agent.json")
                   .contains("\"runtimePackageFiles\": \"runtime/start.geindex\""));
}

MK_TEST("editor input rebinding review model marks clean profiles ready without mutation") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {editor_gamepad_button_trigger(mirakana::GamepadId{1}, mirakana::GamepadButton::south)}});
    profile.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move_x",
        .sources = {editor_gamepad_axis_source(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F)}});

    const auto model = mirakana::editor::make_editor_input_rebinding_profile_review_model(
        mirakana::editor::EditorInputRebindingProfileReviewDesc{.base_actions = base, .profile = profile});

    MK_REQUIRE(model.ready_for_save);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(!model.has_conflicts);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(model.unsupported_claims.empty());
    MK_REQUIRE(model.rows.size() == 1U);
    MK_REQUIRE(model.rows[0].status == mirakana::editor::EditorInputRebindingProfileReviewStatus::ready);
}

MK_TEST("editor input rebinding review model surfaces runtime conflicts") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_key_in_context("gameplay", "cancel", mirakana::Key::escape);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "confirm", .triggers = {editor_key_trigger(mirakana::Key::space)}});
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay", .action = "cancel", .triggers = {editor_key_trigger(mirakana::Key::space)}});

    const auto model = mirakana::editor::make_editor_input_rebinding_profile_review_model(
        mirakana::editor::EditorInputRebindingProfileReviewDesc{.base_actions = base, .profile = profile});

    MK_REQUIRE(!model.ready_for_save);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(model.has_conflicts);
    MK_REQUIRE(editor_input_rebinding_review_has_code(
        model, mirakana::runtime::RuntimeInputRebindingDiagnosticCode::trigger_conflict));

    const auto panel = mirakana::editor::make_editor_input_rebinding_profile_panel_model(
        mirakana::editor::EditorInputRebindingProfileReviewDesc{.base_actions = base, .profile = profile});
    MK_REQUIRE(panel.status == mirakana::editor::EditorInputRebindingProfilePanelStatus::blocked);
    MK_REQUIRE(panel.status_label == "blocked");
    MK_REQUIRE(!panel.ready_for_save);
    MK_REQUIRE(panel.has_conflicts);
    MK_REQUIRE(panel.review_rows.size() == model.rows.size());
    MK_REQUIRE(!panel.diagnostics.empty());
}

MK_TEST("editor input rebinding review model blocks unsupported execution mutation claims") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {editor_gamepad_button_trigger(mirakana::GamepadId{1}, mirakana::GamepadButton::south)}});

    mirakana::editor::EditorInputRebindingProfileReviewDesc desc{.base_actions = base, .profile = profile};
    desc.request_mutation = true;
    desc.request_execution = true;
    desc.request_native_handle_exposure = true;
    desc.request_sdl3_input_api = true;
    desc.request_dear_imgui_or_editor_private_runtime_dependency = true;
    desc.request_ui_focus_consumption = true;
    desc.request_multiplayer_device_assignment = true;
    desc.request_input_glyph_generation = true;

    const auto model = mirakana::editor::make_editor_input_rebinding_profile_review_model(desc);

    MK_REQUIRE(!model.ready_for_save);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(model.unsupported_claims.size() == 8U);
    MK_REQUIRE(model.diagnostics.size() == 8U);
}

MK_TEST("editor input rebinding panel model exposes reviewed bindings and ui rows") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {editor_gamepad_button_trigger(mirakana::GamepadId{1}, mirakana::GamepadButton::south)}});
    profile.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move_x",
        .sources = {editor_gamepad_axis_source(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F)}});

    const auto model = mirakana::editor::make_editor_input_rebinding_profile_panel_model(
        mirakana::editor::EditorInputRebindingProfileReviewDesc{.base_actions = base, .profile = profile});

    MK_REQUIRE(model.status == mirakana::editor::EditorInputRebindingProfilePanelStatus::ready);
    MK_REQUIRE(model.status_label == "ready");
    MK_REQUIRE(model.profile_id == "player_one");
    MK_REQUIRE(model.ready_for_save);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(!model.has_conflicts);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(model.action_binding_count == 1U);
    MK_REQUIRE(model.axis_binding_count == 1U);
    MK_REQUIRE(model.action_override_count == 1U);
    MK_REQUIRE(model.axis_override_count == 1U);

    const auto* action = find_input_rebinding_binding_row(model.binding_rows, "action.gameplay.confirm");
    MK_REQUIRE(action != nullptr);
    MK_REQUIRE(action->kind_label == "action");
    MK_REQUIRE(action->current_binding.contains("space"));
    MK_REQUIRE(action->profile_binding.contains("south"));
    MK_REQUIRE(action->overridden);
    MK_REQUIRE(action->ready);

    const auto* axis = find_input_rebinding_binding_row(model.binding_rows, "axis.gameplay.move_x");
    MK_REQUIRE(axis != nullptr);
    MK_REQUIRE(axis->kind_label == "axis");
    MK_REQUIRE(axis->current_binding.contains("left"));
    MK_REQUIRE(axis->current_binding.contains("right"));
    MK_REQUIRE(axis->profile_binding.contains("left_x"));
    MK_REQUIRE(axis->overridden);
    MK_REQUIRE(axis->ready);

    const auto ui = mirakana::editor::make_input_rebinding_profile_panel_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.profile.id"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.bindings.action.gameplay.confirm.current"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.bindings.axis.gameplay.move_x.profile"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.review.profile.status"}) != nullptr);
}

MK_TEST("editor input rebinding profile store path validation rejects unsafe paths") {
    MK_REQUIRE(!mirakana::editor::validate_editor_input_rebinding_profile_store_path("").empty());
    MK_REQUIRE(!mirakana::editor::validate_editor_input_rebinding_profile_store_path("x.json").empty());
    MK_REQUIRE(!mirakana::editor::validate_editor_input_rebinding_profile_store_path("a/../b.inputrebinding").empty());
    MK_REQUIRE(!mirakana::editor::validate_editor_input_rebinding_profile_store_path("/x.inputrebinding").empty());
    MK_REQUIRE(!mirakana::editor::validate_editor_input_rebinding_profile_store_path("C:/x.inputrebinding").empty());
    MK_REQUIRE(!mirakana::editor::validate_editor_input_rebinding_profile_store_path("bad\\x.inputrebinding").empty());
    MK_REQUIRE(
        mirakana::editor::validate_editor_input_rebinding_profile_store_path("settings/player.inputrebinding").empty());
}

MK_TEST("editor input rebinding profile save load roundtrips over memory text store") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {editor_gamepad_button_trigger(mirakana::GamepadId{1}, mirakana::GamepadButton::south)}});
    profile.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move_x",
        .sources = {editor_gamepad_axis_source(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F)}});

    mirakana::editor::MemoryTextStore store;
    const std::string path = "settings/p1.inputrebinding";
    const auto saved =
        mirakana::editor::save_editor_input_rebinding_profile_to_project_store(store, path, base, profile);
    MK_REQUIRE(saved.succeeded);
    MK_REQUIRE(saved.diagnostic.empty());

    const auto loaded = mirakana::editor::load_editor_input_rebinding_profile_from_project_store(store, path, base);
    MK_REQUIRE(loaded.succeeded);
    MK_REQUIRE(loaded.diagnostic.empty());
    MK_REQUIRE(loaded.profile.profile_id == "player_one");
    MK_REQUIRE(loaded.profile.action_overrides.size() == 1U);
    MK_REQUIRE(loaded.profile.axis_overrides.size() == 1U);
}

MK_TEST("editor input rebinding profile panel ui exposes persistence rows when requested") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    const auto model = mirakana::editor::make_editor_input_rebinding_profile_panel_model(
        mirakana::editor::EditorInputRebindingProfileReviewDesc{.base_actions = base, .profile = profile});

    mirakana::editor::EditorInputRebindingProfilePersistenceUiModel persistence{
        .path_display = "settings/p1.inputrebinding", .last_status = "saved", .diagnostics = {"note"}};
    const auto ui = mirakana::editor::make_input_rebinding_profile_panel_ui_model(model, &persistence);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.persistence.path"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.persistence.last_status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.persistence.diagnostics.0"}) != nullptr);
}

MK_TEST("editor input rebinding capture model captures pressed key candidate") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::right);

    mirakana::editor::EditorInputRebindingCaptureDesc desc;
    desc.base_actions = base;
    desc.profile = profile;
    desc.context = "gameplay";
    desc.action = "confirm";
    desc.state = mirakana::runtime::RuntimeInputStateView{.keyboard = &keyboard};

    const auto model = mirakana::editor::make_editor_input_rebinding_capture_action_model(desc);

    MK_REQUIRE(model.status == mirakana::editor::EditorInputRebindingCaptureStatus::captured);
    MK_REQUIRE(model.status_label == "captured");
    MK_REQUIRE(model.mutates_profile);
    MK_REQUIRE(!model.mutates_files);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(model.trigger.has_value());
    MK_REQUIRE(model.trigger_label == "key:right");
    MK_REQUIRE(model.diagnostics.empty());
    MK_REQUIRE(model.candidate_profile.profile_id == "player_one");
    MK_REQUIRE(model.candidate_profile.action_overrides.size() == 1U);
    MK_REQUIRE(model.candidate_profile.action_overrides[0].context == "gameplay");
    MK_REQUIRE(model.candidate_profile.action_overrides[0].action == "confirm");
    MK_REQUIRE(model.candidate_profile.action_overrides[0].triggers.size() == 1U);
    MK_REQUIRE(model.candidate_profile.action_overrides[0].triggers[0].kind ==
               mirakana::runtime::RuntimeInputActionTriggerKind::key);
    MK_REQUIRE(model.candidate_profile.action_overrides[0].triggers[0].key == mirakana::Key::right);

    const auto ui = mirakana::editor::make_input_rebinding_capture_action_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.capture"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.capture.status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.capture.trigger"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.capture.diagnostics"}) != nullptr);
}

MK_TEST("editor input rebinding axis capture model captures gamepad axis candidate") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_gamepad_axis_in_context("gameplay", "move_x", mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F,
                                      0.15F);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualGamepadInput gamepad;
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::right_y, 0.9F);

    mirakana::editor::EditorInputRebindingAxisCaptureDesc desc;
    desc.base_actions = base;
    desc.profile = profile;
    desc.context = "gameplay";
    desc.action = "move_x";
    desc.state = mirakana::runtime::RuntimeInputStateView{.gamepad = &gamepad};

    const auto model = mirakana::editor::make_editor_input_rebinding_capture_axis_model(desc);

    MK_REQUIRE(model.status == mirakana::editor::EditorInputRebindingCaptureStatus::captured);
    MK_REQUIRE(model.mutates_profile);
    MK_REQUIRE(model.axis_source.has_value());
    MK_REQUIRE(model.candidate_profile.axis_overrides.size() == 1U);

    const auto ui = mirakana::editor::make_input_rebinding_capture_axis_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.capture.axis"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.capture.axis.source"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"input_rebinding.capture.axis.diagnostics"}) != nullptr);
}

MK_TEST("editor input rebinding capture model waits without mutating profile") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {editor_gamepad_button_trigger(mirakana::GamepadId{1}, mirakana::GamepadButton::south)}});

    mirakana::VirtualInput keyboard;

    mirakana::editor::EditorInputRebindingCaptureDesc desc;
    desc.base_actions = base;
    desc.profile = profile;
    desc.context = "gameplay";
    desc.action = "confirm";
    desc.state = mirakana::runtime::RuntimeInputStateView{.keyboard = &keyboard};

    const auto model = mirakana::editor::make_editor_input_rebinding_capture_action_model(desc);

    MK_REQUIRE(model.status == mirakana::editor::EditorInputRebindingCaptureStatus::waiting);
    MK_REQUIRE(model.status_label == "waiting");
    MK_REQUIRE(!model.mutates_profile);
    MK_REQUIRE(!model.mutates_files);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(!model.trigger.has_value());
    MK_REQUIRE(model.trigger_label == "-");
    MK_REQUIRE(model.diagnostics.empty());
    MK_REQUIRE(model.candidate_profile.action_overrides.size() == 1U);
    MK_REQUIRE(model.candidate_profile.action_overrides[0].triggers[0].gamepad_button ==
               mirakana::GamepadButton::south);
}

MK_TEST("editor input rebinding capture model blocks unsupported file command and native claims") {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::escape);

    mirakana::editor::EditorInputRebindingCaptureDesc desc;
    desc.base_actions = base;
    desc.profile = profile;
    desc.context = "gameplay";
    desc.action = "confirm";
    desc.state = mirakana::runtime::RuntimeInputStateView{.keyboard = &keyboard};
    desc.request_file_mutation = true;
    desc.request_command_execution = true;
    desc.request_native_handle_exposure = true;
    desc.request_sdl3_input_api = true;
    desc.request_ui_focus_consumption = true;
    desc.request_input_glyph_generation = true;
    desc.request_multiplayer_device_assignment = true;

    const auto model = mirakana::editor::make_editor_input_rebinding_capture_action_model(desc);

    MK_REQUIRE(model.status == mirakana::editor::EditorInputRebindingCaptureStatus::blocked);
    MK_REQUIRE(model.status_label == "blocked");
    MK_REQUIRE(!model.mutates_profile);
    MK_REQUIRE(!model.mutates_files);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(!model.trigger.has_value());
    MK_REQUIRE(model.unsupported_claims.size() == 7U);
    MK_REQUIRE(model.diagnostics.size() == 7U);
    MK_REQUIRE(model.candidate_profile.action_overrides.empty());
}

MK_TEST("editor playtest package review model orders validation before host smoke") {
    const std::vector<mirakana::editor::ScenePackageRegistrationDraftRow> draft_rows{
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .candidate_path = "games/sample/runtime/scenes/start.scene",
         .runtime_package_path = "runtime/scenes/start.scene",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         .diagnostic = "runtime file is already registered"},
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .candidate_path = "games/sample/runtime/start.geindex",
         .runtime_package_path = "runtime/start.geindex",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         .diagnostic = "runtime file is already registered"},
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_source,
         .candidate_path = "games/sample/assets/scenes/start.scene",
         .runtime_package_path = "assets/scenes/start.scene",
         .runtime_file = false,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::rejected_source_file,
         .diagnostic = "source asset is not a runtime package file"},
    };
    const std::vector<mirakana::editor::RuntimeSceneValidationTargetRow> validation_targets{
        {.id = "packaged-scene",
         .package_index_path = "runtime/start.geindex",
         .scene_asset_key = "sample/scenes/start",
         .content_root = "",
         .validate_asset_references = true,
         .require_unique_node_names = true},
    };

    const auto model =
        mirakana::editor::make_editor_playtest_package_review_model(mirakana::editor::EditorPlaytestPackageReviewDesc{
            .package_registration_draft_rows = draft_rows,
            .runtime_scene_validation_targets = validation_targets,
            .selected_runtime_scene_validation_target_id = "packaged-scene",
            .host_gated_smoke_recipes = {"desktop-game-runtime", "desktop-runtime-release-target"},
        });

    MK_REQUIRE(model.steps.size() == 5);
    MK_REQUIRE(model.steps[0].id == "review-editor-package-candidates");
    MK_REQUIRE(model.steps[1].id == "apply-reviewed-runtime-package-files");
    MK_REQUIRE(model.steps[2].id == "select-runtime-scene-validation-target");
    MK_REQUIRE(model.steps[3].id == "validate-runtime-scene-package");
    MK_REQUIRE(model.steps[4].id == "run-host-gated-desktop-smoke");
    MK_REQUIRE(model.steps[3].surface == "validate-runtime-scene-package");
    MK_REQUIRE(model.steps[4].status == mirakana::editor::EditorPlaytestReviewStepStatus::host_gated);
    MK_REQUIRE(model.ready_for_runtime_scene_validation);
    MK_REQUIRE(model.host_gated_desktop_smoke_available);
    MK_REQUIRE(model.selected_runtime_scene_validation_target.has_value());
    MK_REQUIRE(model.selected_runtime_scene_validation_target->id == "packaged-scene");
    MK_REQUIRE(model.selected_runtime_scene_validation_target->package_index_path == "runtime/start.geindex");
    MK_REQUIRE(mirakana::editor::editor_playtest_review_step_status_label(model.steps[4].status) == "host_gated");
}

MK_TEST("editor playtest package review model blocks validation until reviewed package files are applied") {
    const std::vector<mirakana::editor::ScenePackageRegistrationDraftRow> draft_rows{
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .candidate_path = "games/sample/runtime/scenes/start.scene",
         .runtime_package_path = "runtime/scenes/start.scene",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::add_runtime_file,
         .diagnostic = "add runtimePackageFiles entry"},
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .candidate_path = "games/sample/runtime/start.geindex",
         .runtime_package_path = "runtime/start.geindex",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::add_runtime_file,
         .diagnostic = "add runtimePackageFiles entry"},
    };
    const std::vector<mirakana::editor::RuntimeSceneValidationTargetRow> validation_targets{
        {.id = "packaged-scene",
         .package_index_path = "runtime/start.geindex",
         .scene_asset_key = "sample/scenes/start",
         .content_root = "",
         .validate_asset_references = true,
         .require_unique_node_names = true},
    };

    const auto model =
        mirakana::editor::make_editor_playtest_package_review_model(mirakana::editor::EditorPlaytestPackageReviewDesc{
            .package_registration_draft_rows = draft_rows,
            .runtime_scene_validation_targets = validation_targets,
            .selected_runtime_scene_validation_target_id = "packaged-scene",
            .host_gated_smoke_recipes = {"desktop-game-runtime"},
        });

    MK_REQUIRE(!model.ready_for_runtime_scene_validation);
    MK_REQUIRE(!model.host_gated_desktop_smoke_available);
    MK_REQUIRE(model.pending_runtime_package_files.size() == 2);
    MK_REQUIRE(model.steps[1].status == mirakana::editor::EditorPlaytestReviewStepStatus::ready);
    MK_REQUIRE(model.steps[2].status == mirakana::editor::EditorPlaytestReviewStepStatus::blocked);
    MK_REQUIRE(model.steps[3].status == mirakana::editor::EditorPlaytestReviewStepStatus::blocked);
    MK_REQUIRE(model.steps[4].status == mirakana::editor::EditorPlaytestReviewStepStatus::blocked);
    MK_REQUIRE(model.steps[2].diagnostic.contains("Apply Package Registration"));
}

MK_TEST("editor playtest package review model rejects missing validation targets before smoke") {
    const std::vector<mirakana::editor::ScenePackageRegistrationDraftRow> draft_rows{
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .candidate_path = "games/sample/runtime/start.geindex",
         .runtime_package_path = "runtime/start.geindex",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         .diagnostic = "runtime file is already registered"},
    };
    const std::vector<mirakana::editor::RuntimeSceneValidationTargetRow> validation_targets{
        {.id = "other-scene",
         .package_index_path = "runtime/start.geindex",
         .scene_asset_key = "sample/scenes/other",
         .content_root = "",
         .validate_asset_references = true,
         .require_unique_node_names = true},
    };

    const auto model =
        mirakana::editor::make_editor_playtest_package_review_model(mirakana::editor::EditorPlaytestPackageReviewDesc{
            .package_registration_draft_rows = draft_rows,
            .runtime_scene_validation_targets = validation_targets,
            .selected_runtime_scene_validation_target_id = "packaged-scene",
            .host_gated_smoke_recipes = {"desktop-game-runtime"},
        });

    MK_REQUIRE(!model.selected_runtime_scene_validation_target.has_value());
    MK_REQUIRE(!model.ready_for_runtime_scene_validation);
    MK_REQUIRE(model.steps[2].status == mirakana::editor::EditorPlaytestReviewStepStatus::blocked);
    MK_REQUIRE(model.steps[3].status == mirakana::editor::EditorPlaytestReviewStepStatus::blocked);
    MK_REQUIRE(model.steps[4].status == mirakana::editor::EditorPlaytestReviewStepStatus::blocked);
    MK_REQUIRE(model.steps[2].diagnostic.contains("runtimeSceneValidationTargets"));
}

MK_TEST("editor runtime scene package validation execution records reviewed evidence") {
    const mirakana::AssetKeyV2 scene_key{"assets/scenes/validation-level"};
    const auto scene_asset = mirakana::asset_id_from_key_v2(scene_key);
    const auto mesh_asset = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/meshes/validation-cube"});
    const auto material_asset =
        mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/materials/validation-hero"});
    const auto sprite_asset = mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"assets/textures/validation-sprite"});

    mirakana::Scene scene{"Validated Runtime Level"};
    const auto root = scene.create_node("Root");
    const auto prop = scene.create_node("Prop");
    scene.set_parent(prop, root);
    mirakana::SceneNodeComponents components;
    components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mesh_asset, .material = material_asset, .visible = true};
    components.sprite_renderer = mirakana::SpriteRendererComponent{.sprite = sprite_asset,
                                                                   .material = material_asset,
                                                                   .size = mirakana::Vec2{.x = 1.0F, .y = 2.0F},
                                                                   .tint = {1.0F, 1.0F, 1.0F, 1.0F},
                                                                   .visible = true};
    scene.set_components(prop, components);

    mirakana::MemoryFileSystem fs;
    const std::vector<mirakana::AssetCookedArtifact> artifacts{
        {.asset = scene_asset,
         .kind = mirakana::AssetKind::scene,
         .path = "runtime/assets/scenes/validation-level.scene",
         .content = mirakana::serialize_scene(scene),
         .source_revision = 7,
         .dependencies = {}},
        {.asset = mesh_asset,
         .kind = mirakana::AssetKind::mesh,
         .path = "runtime/assets/meshes/validation-cube.mesh",
         .content = "mesh",
         .source_revision = 7,
         .dependencies = {}},
        {.asset = material_asset,
         .kind = mirakana::AssetKind::material,
         .path = "runtime/assets/materials/validation-hero.material",
         .content = "material",
         .source_revision = 7,
         .dependencies = {}},
        {.asset = sprite_asset,
         .kind = mirakana::AssetKind::texture,
         .path = "runtime/assets/textures/validation-sprite.texture",
         .content = "sprite",
         .source_revision = 7,
         .dependencies = {}},
    };
    fs.write_text("runtime/validation.geindex", mirakana::serialize_asset_cooked_package_index(
                                                    mirakana::build_asset_cooked_package_index(artifacts, {})));
    for (const auto& artifact : artifacts) {
        fs.write_text(artifact.path, artifact.content);
    }

    const std::vector<mirakana::editor::ScenePackageRegistrationDraftRow> ready_draft_rows{
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .candidate_path = "games/sample/runtime/validation.geindex",
         .runtime_package_path = "runtime/validation.geindex",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         .diagnostic = "runtime file is already registered"},
    };
    const std::vector<mirakana::editor::RuntimeSceneValidationTargetRow> validation_targets{
        {.id = "packaged-scene",
         .package_index_path = "runtime/validation.geindex",
         .scene_asset_key = "assets/scenes/validation-level",
         .content_root = "",
         .validate_asset_references = true,
         .require_unique_node_names = true},
    };

    const auto review =
        mirakana::editor::make_editor_playtest_package_review_model(mirakana::editor::EditorPlaytestPackageReviewDesc{
            .package_registration_draft_rows = ready_draft_rows,
            .runtime_scene_validation_targets = validation_targets,
            .selected_runtime_scene_validation_target_id = "packaged-scene",
            .host_gated_smoke_recipes = {"desktop-game-runtime"},
        });
    MK_REQUIRE(review.ready_for_runtime_scene_validation);

    mirakana::editor::EditorRuntimeScenePackageValidationExecutionDesc desc;
    desc.playtest_review = review;
    const auto model = mirakana::editor::make_editor_runtime_scene_package_validation_execution_model(desc);

    MK_REQUIRE(model.status == mirakana::editor::EditorRuntimeScenePackageValidationExecutionStatus::ready);
    MK_REQUIRE(model.status_label == "ready");
    MK_REQUIRE(model.can_execute);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes_external_process);
    MK_REQUIRE(model.request.package_index_path == "runtime/validation.geindex");
    MK_REQUIRE(model.request.content_root.empty());
    MK_REQUIRE(model.request.scene_asset_key.value == "assets/scenes/validation-level");
    MK_REQUIRE(model.request.validate_asset_references);
    MK_REQUIRE(model.request.require_unique_node_names);

    const auto passed = mirakana::editor::execute_editor_runtime_scene_package_validation(fs, model);
    MK_REQUIRE(passed.status == mirakana::editor::EditorRuntimeScenePackageValidationExecutionStatus::passed);
    MK_REQUIRE(passed.status_label == "passed");
    MK_REQUIRE(passed.evidence_available);
    MK_REQUIRE(passed.validation.succeeded());
    MK_REQUIRE(passed.validation.summary.package_record_count == 4U);
    MK_REQUIRE(passed.validation.summary.scene_node_count == 2U);
    MK_REQUIRE(passed.validation.summary.references.size() == 4U);

    const auto ui = mirakana::editor::make_editor_runtime_scene_package_validation_execution_ui_model(passed);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"playtest_package_review.runtime_scene_validation"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "playtest_package_review.runtime_scene_validation.packaged-scene.status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "playtest_package_review.runtime_scene_validation.packaged-scene.package"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "playtest_package_review.runtime_scene_validation.packaged-scene.scene_asset_key"}) != nullptr);

    const std::vector<mirakana::editor::ScenePackageRegistrationDraftRow> pending_draft_rows{
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .candidate_path = "games/sample/runtime/validation.geindex",
         .runtime_package_path = "runtime/validation.geindex",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::add_runtime_file,
         .diagnostic = "add runtimePackageFiles entry"},
    };
    const auto pending_review =
        mirakana::editor::make_editor_playtest_package_review_model(mirakana::editor::EditorPlaytestPackageReviewDesc{
            .package_registration_draft_rows = pending_draft_rows,
            .runtime_scene_validation_targets = validation_targets,
            .selected_runtime_scene_validation_target_id = "packaged-scene",
            .host_gated_smoke_recipes = {"desktop-game-runtime"},
        });
    mirakana::editor::EditorRuntimeScenePackageValidationExecutionDesc pending_desc;
    pending_desc.playtest_review = pending_review;
    const auto pending_model =
        mirakana::editor::make_editor_runtime_scene_package_validation_execution_model(pending_desc);
    MK_REQUIRE(pending_model.status == mirakana::editor::EditorRuntimeScenePackageValidationExecutionStatus::blocked);
    MK_REQUIRE(!pending_model.can_execute);
    MK_REQUIRE(!pending_model.blocked_by.empty());
    MK_REQUIRE(std::ranges::find(pending_model.blocked_by, "pending-runtime-package-files") !=
               pending_model.blocked_by.end());

    mirakana::MemoryFileSystem missing_package_fs;
    const auto failed = mirakana::editor::execute_editor_runtime_scene_package_validation(missing_package_fs, model);
    MK_REQUIRE(failed.status == mirakana::editor::EditorRuntimeScenePackageValidationExecutionStatus::failed);
    MK_REQUIRE(failed.evidence_available);
    MK_REQUIRE(!failed.validation.succeeded());
    MK_REQUIRE(!failed.validation.diagnostics.empty());

    mirakana::editor::EditorRuntimeScenePackageValidationExecutionDesc unsupported_desc;
    unsupported_desc.playtest_review = review;
    unsupported_desc.request_package_cooking = true;
    unsupported_desc.request_arbitrary_shell_execution = true;
    unsupported_desc.request_renderer_rhi_residency = true;
    const auto unsupported =
        mirakana::editor::make_editor_runtime_scene_package_validation_execution_model(unsupported_desc);
    MK_REQUIRE(unsupported.status == mirakana::editor::EditorRuntimeScenePackageValidationExecutionStatus::blocked);
    MK_REQUIRE(!unsupported.can_execute);
    MK_REQUIRE(unsupported.unsupported_claims.size() == 3U);
    MK_REQUIRE(unsupported.diagnostics.size() >= 3U);
}

MK_TEST("editor ai package authoring diagnostics summarize descriptors payloads and host gates without mutation") {
    const std::vector<mirakana::editor::ScenePackageRegistrationDraftRow> draft_rows{
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .candidate_path = "games/sample/runtime/scenes/start.scene",
         .runtime_package_path = "runtime/scenes/start.scene",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         .diagnostic = "runtime file is already registered"},
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .candidate_path = "games/sample/runtime/start.geindex",
         .runtime_package_path = "runtime/start.geindex",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         .diagnostic = "runtime file is already registered"},
    };
    const std::vector<mirakana::editor::RuntimeSceneValidationTargetRow> validation_targets{
        {.id = "packaged-scene",
         .package_index_path = "runtime/start.geindex",
         .scene_asset_key = "sample/scenes/start",
         .content_root = "",
         .validate_asset_references = true,
         .require_unique_node_names = true},
    };

    const auto model = mirakana::editor::make_editor_ai_package_authoring_diagnostics_model(
        mirakana::editor::EditorAiPackageAuthoringDiagnosticsDesc{
            .playtest_review =
                mirakana::editor::EditorPlaytestPackageReviewDesc{
                    .package_registration_draft_rows = draft_rows,
                    .runtime_scene_validation_targets = validation_targets,
                    .selected_runtime_scene_validation_target_id = "packaged-scene",
                    .host_gated_smoke_recipes = {"desktop-game-runtime"},
                },
            .descriptor_rows =
                {
                    {.id = "runtime-scene-target",
                     .surface = "game.agent.json.runtimeSceneValidationTargets",
                     .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
                     .mutates = false,
                     .executes = false,
                     .diagnostic = "runtime scene validation target selected"},
                    {.id = "prefab-scene-target",
                     .surface = "game.agent.json.prefabScenePackageAuthoringTargets",
                     .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
                     .mutates = false,
                     .executes = false,
                     .diagnostic = "prefab scene authoring descriptor selected"},
                },
            .payload_rows =
                {
                    {.id = "runtime-package-payloads",
                     .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
                     .diagnostic = "scene, mesh, material, and texture payloads are inspectable"},
                },
            .validation_recipe_rows =
                {
                    {.id = "desktop-game-runtime",
                     .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
                     .host_gated = true,
                     .executes = false,
                     .diagnostic = "desktop smoke remains host-gated"},
                },
        });

    MK_REQUIRE(model.playtest_review.ready_for_runtime_scene_validation);
    MK_REQUIRE(model.playtest_review.host_gated_desktop_smoke_available);
    MK_REQUIRE(model.descriptor_rows.size() == 2);
    MK_REQUIRE(model.payload_rows.size() == 1);
    MK_REQUIRE(model.validation_recipe_rows.size() == 1);
    MK_REQUIRE(model.has_host_gated_recipes);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(mirakana::editor::editor_ai_package_authoring_diagnostic_status_label(
                   model.validation_recipe_rows[0].status) == "host_gated");
}

MK_TEST("editor tilemap package diagnostics surface runtime tilemap rows") {
    const auto model =
        mirakana::editor::make_editor_tilemap_package_diagnostics_model(make_editor_tilemap_test_package());

    MK_REQUIRE(model.has_tilemap_payloads);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(model.rows.size() == 1);
    MK_REQUIRE(model.rows[0].status == mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready);
    MK_REQUIRE(model.rows[0].path == "runtime/assets/2d/level.tilemap");
    MK_REQUIRE(model.rows[0].layer_count == 1);
    MK_REQUIRE(model.rows[0].visible_layer_count == 1);
    MK_REQUIRE(model.rows[0].tile_count == 2);
    MK_REQUIRE(model.rows[0].non_empty_cell_count == 3);
    MK_REQUIRE(model.rows[0].sampled_cell_count == 3);
    MK_REQUIRE(model.rows[0].diagnostic_count == 0);
}

MK_TEST("editor ai package authoring diagnostics reject mutation and execution claims") {
    const std::vector<mirakana::editor::ScenePackageRegistrationDraftRow> draft_rows{
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .candidate_path = "games/sample/runtime/start.geindex",
         .runtime_package_path = "runtime/start.geindex",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         .diagnostic = "runtime file is already registered"},
    };
    const std::vector<mirakana::editor::RuntimeSceneValidationTargetRow> validation_targets{
        {.id = "packaged-scene",
         .package_index_path = "runtime/start.geindex",
         .scene_asset_key = "sample/scenes/start",
         .content_root = "",
         .validate_asset_references = true,
         .require_unique_node_names = true},
    };

    const auto model = mirakana::editor::make_editor_ai_package_authoring_diagnostics_model(
        mirakana::editor::EditorAiPackageAuthoringDiagnosticsDesc{
            .playtest_review =
                mirakana::editor::EditorPlaytestPackageReviewDesc{
                    .package_registration_draft_rows = draft_rows,
                    .runtime_scene_validation_targets = validation_targets,
                    .selected_runtime_scene_validation_target_id = "packaged-scene",
                    .host_gated_smoke_recipes = {},
                },
            .descriptor_rows =
                {
                    {.id = "manifest-patch",
                     .surface = "free-form game.agent.json edit",
                     .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
                     .mutates = true,
                     .executes = false,
                     .diagnostic = "free-form manifest patch should not be accepted"},
                },
            .payload_rows =
                {
                    {.id = "runtime-package-payloads",
                     .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::blocked,
                     .diagnostic = "package payload inspection is missing"},
                },
            .validation_recipe_rows =
                {
                    {.id = "desktop-game-runtime",
                     .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
                     .host_gated = false,
                     .executes = true,
                     .diagnostic = "recipe execution should stay outside editor diagnostics"},
                },
        });

    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(model.diagnostics.size() >= 3);
    MK_REQUIRE(model.diagnostics[0].contains("diagnostics-only"));
    MK_REQUIRE(model.diagnostics[1].contains("preflight-only"));
    MK_REQUIRE(model.diagnostics[2].contains("blocked"));
}

MK_TEST("editor ai validation recipe preflight maps manifest recipes to dry run plans") {
    const std::vector<mirakana::editor::EditorAiValidationRecipeDryRunPlanRow> dry_run_rows{
        {.id = "agent-contract",
         .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1",
         .host_gates = {},
         .blocked_by = {},
         .executes = false,
         .diagnostic = "agent contract dry-run ready",
         .argv = {}},
        {.id = "desktop-runtime-sample-game-scene-gpu-package",
         .command_display =
             "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget "
             "sample_desktop_runtime_game",
         .host_gates = {"d3d12-windows-primary"},
         .blocked_by = {},
         .executes = false,
         .diagnostic = "D3D12 package validation dry-run requires host acknowledgement",
         .argv = {}},
    };

    const auto model = mirakana::editor::make_editor_ai_validation_recipe_preflight_model(
        mirakana::editor::EditorAiValidationRecipePreflightDesc{
            .manifest_validation_recipe_ids = {"agent-contract", "desktop-runtime-sample-game-scene-gpu-package"},
            .selected_validation_recipe_ids = {"agent-contract", "desktop-runtime-sample-game-scene-gpu-package"},
            .dry_run_plan_rows = dry_run_rows,
        });

    MK_REQUIRE(model.rows.size() == 2);
    MK_REQUIRE(model.rows[0].id == "agent-contract");
    MK_REQUIRE(model.rows[0].status == mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready);
    MK_REQUIRE(model.rows[0].manifest_declared);
    MK_REQUIRE(model.rows[0].reviewed_dry_run_available);
    MK_REQUIRE(model.rows[1].id == "desktop-runtime-sample-game-scene-gpu-package");
    MK_REQUIRE(model.rows[1].status == mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated);
    MK_REQUIRE(model.rows[1].host_gates.size() == 1);
    MK_REQUIRE(model.rows[1].host_gates[0] == "d3d12-windows-primary");
    MK_REQUIRE(model.has_host_gated_recipes);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(!model.executes);
}

MK_TEST("editor ai validation recipe preflight blocks execution and unsupported claims") {
    const std::vector<mirakana::editor::EditorAiValidationRecipeDryRunPlanRow> dry_run_rows{
        {.id = "agent-contract",
         .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1",
         .host_gates = {},
         .blocked_by = {},
         .executes = true,
         .diagnostic = "execute result must not be accepted as preflight",
         .argv = {}},
    };

    const auto model = mirakana::editor::make_editor_ai_validation_recipe_preflight_model(
        mirakana::editor::EditorAiValidationRecipePreflightDesc{
            .manifest_validation_recipe_ids = {"agent-contract"},
            .selected_validation_recipe_ids = {"agent-contract", "not-in-manifest", "missing-dry-run"},
            .dry_run_plan_rows = dry_run_rows,
            .request_arbitrary_shell_execution = true,
            .request_raw_manifest_command_evaluation = true,
            .request_package_script_execution = true,
            .request_play_in_editor_productization = true,
            .request_renderer_rhi_handle_exposure = true,
            .request_metal_readiness_claim = true,
            .request_renderer_quality_claim = true,
        });

    auto has_diagnostic = [&model](std::string_view text) {
        return std::ranges::any_of(model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(model.rows.size() == 3);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(model.rows[0].status == mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::blocked);
    MK_REQUIRE(model.rows[1].diagnostic.contains("not declared"));
    MK_REQUIRE(model.rows[2].diagnostic.contains("dry-run"));
    MK_REQUIRE(has_diagnostic("preflight-only"));
    MK_REQUIRE(has_diagnostic("arbitrary shell"));
    MK_REQUIRE(has_diagnostic("raw manifest command"));
    MK_REQUIRE(has_diagnostic("package script execution"));
    MK_REQUIRE(has_diagnostic("play-in-editor productization"));
    MK_REQUIRE(has_diagnostic("renderer/RHI handle exposure"));
    MK_REQUIRE(has_diagnostic("Metal readiness"));
    MK_REQUIRE(has_diagnostic("renderer quality"));
}

MK_TEST("editor ai playtest readiness report aggregates package diagnostics and validation preflight") {
    const std::vector<mirakana::editor::ScenePackageRegistrationDraftRow> draft_rows{
        {.kind = mirakana::editor::ScenePackageCandidateKind::scene_cooked,
         .candidate_path = "games/sample/runtime/scenes/start.scene",
         .runtime_package_path = "runtime/scenes/start.scene",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         .diagnostic = "runtime file is already registered"},
        {.kind = mirakana::editor::ScenePackageCandidateKind::package_index,
         .candidate_path = "games/sample/runtime/start.geindex",
         .runtime_package_path = "runtime/start.geindex",
         .runtime_file = true,
         .status = mirakana::editor::ScenePackageRegistrationDraftStatus::already_registered,
         .diagnostic = "runtime file is already registered"},
    };
    const std::vector<mirakana::editor::RuntimeSceneValidationTargetRow> validation_targets{
        {.id = "packaged-scene",
         .package_index_path = "runtime/start.geindex",
         .scene_asset_key = "sample/scenes/start",
         .content_root = "",
         .validate_asset_references = true,
         .require_unique_node_names = true},
    };
    const auto package_diagnostics = mirakana::editor::make_editor_ai_package_authoring_diagnostics_model(
        mirakana::editor::EditorAiPackageAuthoringDiagnosticsDesc{
            .playtest_review =
                mirakana::editor::EditorPlaytestPackageReviewDesc{
                    .package_registration_draft_rows = draft_rows,
                    .runtime_scene_validation_targets = validation_targets,
                    .selected_runtime_scene_validation_target_id = "packaged-scene",
                    .host_gated_smoke_recipes = {"desktop-runtime-sample-game-scene-gpu-package"},
                },
            .descriptor_rows =
                {
                    {.id = "runtime-scene-target",
                     .surface = "game.agent.json.runtimeSceneValidationTargets",
                     .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
                     .mutates = false,
                     .executes = false,
                     .diagnostic = "runtime scene validation target selected"},
                },
            .payload_rows =
                {
                    {.id = "runtime-package-payloads",
                     .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
                     .diagnostic = "runtime package payloads are inspectable"},
                },
            .validation_recipe_rows =
                {
                    {.id = "desktop-runtime-sample-game-scene-gpu-package",
                     .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
                     .host_gated = true,
                     .executes = false,
                     .diagnostic = "desktop package smoke remains host-gated"},
                },
        });

    const std::vector<mirakana::editor::EditorAiValidationRecipeDryRunPlanRow> dry_run_rows{
        {.id = "desktop-runtime-sample-game-scene-gpu-package",
         .command_display =
             "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget "
             "sample_desktop_runtime_game",
         .host_gates = {"d3d12-windows-primary"},
         .blocked_by = {},
         .executes = false,
         .diagnostic = "D3D12 package validation dry-run ready",
         .argv = {}},
    };
    const auto validation_preflight = mirakana::editor::make_editor_ai_validation_recipe_preflight_model(
        mirakana::editor::EditorAiValidationRecipePreflightDesc{
            .manifest_validation_recipe_ids = {"desktop-runtime-sample-game-scene-gpu-package"},
            .selected_validation_recipe_ids = {"desktop-runtime-sample-game-scene-gpu-package"},
            .dry_run_plan_rows = dry_run_rows,
        });

    const mirakana::editor::EditorAiPlaytestReadinessReportModel model =
        mirakana::editor::make_editor_ai_playtest_readiness_report_model(
            mirakana::editor::EditorAiPlaytestReadinessReportDesc{.package_diagnostics = package_diagnostics,
                                                                  .validation_preflight = validation_preflight});

    MK_REQUIRE(model.rows.size() == 2);
    MK_REQUIRE(model.rows[0].id == "package-authoring-diagnostics");
    MK_REQUIRE(model.rows[1].id == "validation-recipe-preflight");
    MK_REQUIRE(model.status == mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated);
    MK_REQUIRE(model.has_host_gates);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(model.ready_for_operator_validation);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
}

MK_TEST("editor ai playtest readiness report blocks mutation execution and unsupported claims") {
    mirakana::editor::EditorAiPackageAuthoringDiagnosticsModel package_diagnostics;
    package_diagnostics.has_blocking_diagnostics = true;
    package_diagnostics.diagnostics.emplace_back("package diagnostics blocked");

    mirakana::editor::EditorAiValidationRecipePreflightModel validation_preflight;
    validation_preflight.has_blocking_diagnostics = true;
    validation_preflight.diagnostics.emplace_back("validation preflight blocked");

    const mirakana::editor::EditorAiPlaytestReadinessReportModel model =
        mirakana::editor::make_editor_ai_playtest_readiness_report_model(
            mirakana::editor::EditorAiPlaytestReadinessReportDesc{
                .package_diagnostics = package_diagnostics,
                .validation_preflight = validation_preflight,
                .request_mutation = true,
                .request_execution = true,
                .request_arbitrary_shell_execution = true,
                .request_free_form_manifest_edit = true,
                .request_play_in_editor_productization = true,
                .request_renderer_rhi_handle_exposure = true,
                .request_metal_readiness_claim = true,
                .request_renderer_quality_claim = true,
            });

    auto has_diagnostic = [&model](std::string_view text) {
        return std::ranges::any_of(model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(model.status == mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::blocked);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.ready_for_operator_validation);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(has_diagnostic("package diagnostics blocked"));
    MK_REQUIRE(has_diagnostic("validation preflight blocked"));
    MK_REQUIRE(has_diagnostic("read-only"));
    MK_REQUIRE(has_diagnostic("must not execute"));
    MK_REQUIRE(has_diagnostic("arbitrary shell"));
    MK_REQUIRE(has_diagnostic("free-form manifest edits"));
    MK_REQUIRE(has_diagnostic("play-in-editor productization"));
    MK_REQUIRE(has_diagnostic("renderer/RHI handle exposure"));
    MK_REQUIRE(has_diagnostic("Metal readiness"));
    MK_REQUIRE(has_diagnostic("renderer quality"));
}

MK_TEST("editor ai playtest operator handoff lists reviewed commands after readiness report") {
    mirakana::editor::EditorAiPlaytestReadinessReportModel readiness_report;
    readiness_report.status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated;
    readiness_report.ready_for_operator_validation = true;
    readiness_report.has_host_gates = true;

    mirakana::editor::EditorAiValidationRecipePreflightModel validation_preflight;
    validation_preflight.has_host_gated_recipes = true;
    validation_preflight.rows.push_back(mirakana::editor::EditorAiValidationRecipePreflightRow{
        .id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .manifest_declared = true,
        .reviewed_dry_run_available = true,
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {},
        .diagnostic = "D3D12 package validation dry-run ready",
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget "
            "sample_desktop_runtime_game",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/package-desktop-runtime.ps1",
                 "-GameTarget", "sample_desktop_runtime_game"},
    });

    const mirakana::editor::EditorAiPlaytestOperatorHandoffModel model =
        mirakana::editor::make_editor_ai_playtest_operator_handoff_model(
            mirakana::editor::EditorAiPlaytestOperatorHandoffDesc{.readiness_report = readiness_report,
                                                                  .validation_preflight = validation_preflight});

    MK_REQUIRE(model.command_rows.size() == 1);
    MK_REQUIRE(model.command_rows[0].recipe_id == "desktop-runtime-sample-game-scene-gpu-package");
    MK_REQUIRE(model.command_rows[0].status == mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated);
    MK_REQUIRE(model.command_rows[0].readiness_dependency ==
               "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation");
    MK_REQUIRE(model.command_rows[0].command_display.contains("package-desktop-runtime.ps1"));
    MK_REQUIRE(model.command_rows[0].argv.size() == 8);
    MK_REQUIRE(model.command_rows[0].host_gates.size() == 1);
    MK_REQUIRE(model.command_rows[0].host_gates[0] == "d3d12-windows-primary");
    MK_REQUIRE(model.status == mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated);
    MK_REQUIRE(model.ready_for_operator_handoff);
    MK_REQUIRE(model.has_host_gates);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
}

MK_TEST("editor ai playtest operator handoff blocks execution mutation and unsupported claims") {
    mirakana::editor::EditorAiPlaytestReadinessReportModel readiness_report;
    readiness_report.status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::blocked;
    readiness_report.has_blocking_diagnostics = true;
    readiness_report.diagnostics.emplace_back("readiness report blocked");

    mirakana::editor::EditorAiValidationRecipePreflightModel validation_preflight;
    validation_preflight.rows.push_back(mirakana::editor::EditorAiValidationRecipePreflightRow{
        .id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .manifest_declared = true,
        .reviewed_dry_run_available = true,
        .host_gates = {},
        .blocked_by = {},
        .diagnostic = "agent contract dry-run ready",
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode "
                           "Execute -Recipe agent-contract",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "Execute", "-Recipe", "agent-contract"},
    });

    const mirakana::editor::EditorAiPlaytestOperatorHandoffModel model =
        mirakana::editor::make_editor_ai_playtest_operator_handoff_model(
            mirakana::editor::EditorAiPlaytestOperatorHandoffDesc{
                .readiness_report = readiness_report,
                .validation_preflight = validation_preflight,
                .request_mutation = true,
                .request_validation_execution = true,
                .request_arbitrary_shell_execution = true,
                .request_raw_manifest_command_evaluation = true,
                .request_package_script_execution = true,
                .request_free_form_manifest_edit = true,
                .request_play_in_editor_productization = true,
                .request_renderer_rhi_handle_exposure = true,
                .request_metal_readiness_claim = true,
                .request_renderer_quality_claim = true,
            });

    auto has_diagnostic = [&model](std::string_view text) {
        return std::ranges::any_of(model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(model.command_rows.size() == 1);
    MK_REQUIRE(model.command_rows[0].status == mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::blocked);
    MK_REQUIRE(model.command_rows[0].blocked_by.size() == 1);
    MK_REQUIRE(model.command_rows[0].blocked_by[0] == "readiness-report-blocked");
    MK_REQUIRE(!model.ready_for_operator_handoff);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(has_diagnostic("readiness report blocked"));
    MK_REQUIRE(has_diagnostic("mutation"));
    MK_REQUIRE(has_diagnostic("validation execution"));
    MK_REQUIRE(has_diagnostic("arbitrary shell"));
    MK_REQUIRE(has_diagnostic("raw manifest command"));
    MK_REQUIRE(has_diagnostic("package script execution"));
    MK_REQUIRE(has_diagnostic("free-form manifest edits"));
    MK_REQUIRE(has_diagnostic("play-in-editor productization"));
    MK_REQUIRE(has_diagnostic("renderer/RHI handle exposure"));
    MK_REQUIRE(has_diagnostic("Metal readiness"));
    MK_REQUIRE(has_diagnostic("renderer quality"));
}

MK_TEST("editor ai playtest evidence summary maps external evidence to operator handoff rows") {
    mirakana::editor::EditorAiPlaytestOperatorHandoffModel operator_handoff;
    operator_handoff.status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated;
    operator_handoff.ready_for_operator_handoff = true;
    operator_handoff.has_host_gates = true;
    operator_handoff.command_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode "
                           "Execute -Recipe agent-contract",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "Execute", "-Recipe", "agent-contract"},
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "agent contract handoff ready",
    });
    operator_handoff.command_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget "
            "sample_desktop_runtime_game",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/package-desktop-runtime.ps1",
                 "-GameTarget", "sample_desktop_runtime_game"},
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "D3D12 package handoff is host-gated",
    });
    operator_handoff.command_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow{
        .recipe_id = "shader-toolchain",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode "
                           "Execute -Recipe shader-toolchain",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "Execute", "-Recipe", "shader-toolchain"},
        .host_gates = {"vulkan-strict", "metal-apple"},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "shader toolchain handoff is host-gated",
    });
    operator_handoff.command_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow{
        .recipe_id = "desktop-game-runtime",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe "
            "desktop-game-runtime",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "Execute", "-Recipe", "desktop-game-runtime"},
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "desktop runtime handoff ready",
    });

    const std::vector<mirakana::editor::EditorAiPlaytestValidationEvidenceRow> evidence_rows{
        {.recipe_id = "agent-contract",
         .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::passed,
         .exit_code = 0,
         .summary = "agent-contract passed",
         .stdout_summary = "agent-check and schema-check passed",
         .stderr_summary = "",
         .host_gates = {},
         .blocked_by = {}},
        {.recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
         .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::failed,
         .exit_code = 1,
         .summary = "package smoke failed",
         .stdout_summary = "",
         .stderr_summary = "missing package artifact",
         .host_gates = {"d3d12-windows-primary"},
         .blocked_by = {"package-artifact-missing"}},
        {.recipe_id = "shader-toolchain",
         .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::host_gated,
         .exit_code = std::nullopt,
         .summary = "Metal tools missing on this host",
         .stdout_summary = "",
         .stderr_summary = "",
         .host_gates = {"vulkan-strict", "metal-apple"},
         .blocked_by = {"metal-tools-missing"}},
    };

    const mirakana::editor::EditorAiPlaytestEvidenceSummaryModel model =
        mirakana::editor::make_editor_ai_playtest_evidence_summary_model(
            mirakana::editor::EditorAiPlaytestEvidenceSummaryDesc{.operator_handoff = operator_handoff,
                                                                  .evidence_rows = evidence_rows});

    MK_REQUIRE(model.rows.size() == 4);
    MK_REQUIRE(model.rows[0].recipe_id == "agent-contract");
    MK_REQUIRE(model.rows[0].status == mirakana::editor::EditorAiPlaytestEvidenceStatus::passed);
    MK_REQUIRE(model.rows[0].exit_code.has_value());
    MK_REQUIRE(model.rows[0].exit_code.value() == 0);
    MK_REQUIRE(model.rows[0].summary.contains("agent-contract passed"));
    MK_REQUIRE(model.rows[1].recipe_id == "desktop-runtime-sample-game-scene-gpu-package");
    MK_REQUIRE(model.rows[1].status == mirakana::editor::EditorAiPlaytestEvidenceStatus::failed);
    MK_REQUIRE(model.rows[1].exit_code.has_value());
    MK_REQUIRE(model.rows[1].exit_code.value() == 1);
    MK_REQUIRE(model.rows[1].blocked_by[0] == "package-artifact-missing");
    MK_REQUIRE(model.rows[2].status == mirakana::editor::EditorAiPlaytestEvidenceStatus::host_gated);
    MK_REQUIRE(model.rows[2].host_gates.size() == 2);
    MK_REQUIRE(model.rows[3].recipe_id == "desktop-game-runtime");
    MK_REQUIRE(model.rows[3].status == mirakana::editor::EditorAiPlaytestEvidenceStatus::missing);
    MK_REQUIRE(model.rows[3].blocked_by[0] == "missing-external-validation-evidence");
    MK_REQUIRE(model.status == mirakana::editor::EditorAiPlaytestEvidenceStatus::failed);
    MK_REQUIRE(model.has_failed_evidence);
    MK_REQUIRE(model.has_missing_evidence);
    MK_REQUIRE(model.has_host_gates);
    MK_REQUIRE(!model.all_required_evidence_passed);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(mirakana::editor::editor_ai_playtest_evidence_status_label(model.rows[3].status) == "missing");
}

MK_TEST("editor ai playtest evidence summary blocks editor core execution and unsupported claims") {
    mirakana::editor::EditorAiPlaytestOperatorHandoffModel operator_handoff;
    operator_handoff.status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready;
    operator_handoff.ready_for_operator_handoff = true;
    operator_handoff.command_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode "
                           "Execute -Recipe agent-contract",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "Execute", "-Recipe", "agent-contract"},
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "agent contract handoff ready",
    });

    const std::vector<mirakana::editor::EditorAiPlaytestValidationEvidenceRow> evidence_rows{
        {.recipe_id = "agent-contract",
         .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::passed,
         .exit_code = 0,
         .summary = "editor core should not be the execution source",
         .stdout_summary = "agent-check passed",
         .stderr_summary = "",
         .host_gates = {},
         .blocked_by = {},
         .externally_supplied = false,
         .claims_editor_core_execution = true},
    };

    const mirakana::editor::EditorAiPlaytestEvidenceSummaryModel model =
        mirakana::editor::make_editor_ai_playtest_evidence_summary_model(
            mirakana::editor::EditorAiPlaytestEvidenceSummaryDesc{
                .operator_handoff = operator_handoff,
                .evidence_rows = evidence_rows,
                .request_mutation = true,
                .request_validation_execution = true,
                .request_arbitrary_shell_execution = true,
                .request_raw_manifest_command_evaluation = true,
                .request_package_script_execution = true,
                .request_free_form_manifest_edit = true,
                .request_play_in_editor_productization = true,
                .request_renderer_rhi_handle_exposure = true,
                .request_metal_readiness_claim = true,
                .request_renderer_quality_claim = true,
            });

    auto has_diagnostic = [&model](std::string_view text) {
        return std::ranges::any_of(model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(model.rows.size() == 1);
    MK_REQUIRE(model.rows[0].status == mirakana::editor::EditorAiPlaytestEvidenceStatus::blocked);
    MK_REQUIRE(model.rows[0].blocked_by.size() == 2);
    MK_REQUIRE(model.rows[0].blocked_by[0] == "evidence-not-externally-supplied");
    MK_REQUIRE(model.rows[0].blocked_by[1] == "editor-core-validation-execution-claim");
    MK_REQUIRE(model.status == mirakana::editor::EditorAiPlaytestEvidenceStatus::blocked);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.all_required_evidence_passed);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(has_diagnostic("externally supplied"));
    MK_REQUIRE(has_diagnostic("mutation"));
    MK_REQUIRE(has_diagnostic("validation execution"));
    MK_REQUIRE(has_diagnostic("arbitrary shell"));
    MK_REQUIRE(has_diagnostic("raw manifest command"));
    MK_REQUIRE(has_diagnostic("package script execution"));
    MK_REQUIRE(has_diagnostic("free-form manifest edits"));
    MK_REQUIRE(has_diagnostic("play-in-editor productization"));
    MK_REQUIRE(has_diagnostic("renderer/RHI handle exposure"));
    MK_REQUIRE(has_diagnostic("Metal readiness"));
    MK_REQUIRE(has_diagnostic("renderer quality"));
}

MK_TEST("editor ai playtest evidence import parses external rows without execution") {
    mirakana::editor::EditorAiPlaytestEvidenceImportDesc desc;
    desc.expected_recipe_ids = {
        "agent-contract",
        "desktop-runtime-sample-game-scene-gpu-package",
        "desktop-game-runtime",
    };
    desc.text = "format=GameEngine.EditorAiPlaytestEvidence.v1\n"
                "evidence.agent-contract.status=passed\n"
                "evidence.agent-contract.exit_code=0\n"
                "evidence.agent-contract.summary=agent-contract passed after external operator validation\n"
                "evidence.agent-contract.stdout=agent-check and schema-check passed\n"
                "evidence.agent-contract.stderr=\n"
                "evidence.desktop-runtime-sample-game-scene-gpu-package.status=host_gated\n"
                "evidence.desktop-runtime-sample-game-scene-gpu-package.summary=D3D12 host gate is not available here\n"
                "evidence.desktop-runtime-sample-game-scene-gpu-package.host_gates=d3d12-windows-primary,metal-apple\n"
                "evidence.desktop-runtime-sample-game-scene-gpu-package.blocked_by=d3d12-driver-missing\n";

    const auto model = mirakana::editor::make_editor_ai_playtest_evidence_import_model(desc);

    MK_REQUIRE(model.status == mirakana::editor::EditorAiPlaytestEvidenceImportStatus::importable);
    MK_REQUIRE(model.status_label == "importable");
    MK_REQUIRE(model.importable);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(model.has_missing_expected_evidence);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(model.evidence_rows.size() == 2U);
    MK_REQUIRE(model.review_rows.size() == 3U);
    MK_REQUIRE(model.imported_count == 2U);
    MK_REQUIRE(model.missing_expected_count == 1U);

    const auto* agent_contract = find_ai_evidence_import_row(model.review_rows, "agent-contract");
    MK_REQUIRE(agent_contract != nullptr);
    MK_REQUIRE(agent_contract->status == mirakana::editor::EditorAiPlaytestEvidenceImportStatus::importable);
    MK_REQUIRE(agent_contract->imported);
    MK_REQUIRE(agent_contract->exit_code.has_value());
    MK_REQUIRE(agent_contract->exit_code.value() == 0);
    MK_REQUIRE(agent_contract->summary.contains("agent-contract passed"));

    const auto* host_gated =
        find_ai_evidence_import_row(model.review_rows, "desktop-runtime-sample-game-scene-gpu-package");
    MK_REQUIRE(host_gated != nullptr);
    MK_REQUIRE(host_gated->evidence_status == mirakana::editor::EditorAiPlaytestEvidenceStatus::host_gated);
    MK_REQUIRE(host_gated->host_gates.size() == 2U);
    MK_REQUIRE(host_gated->blocked_by.size() == 1U);

    const auto* missing = find_ai_evidence_import_row(model.review_rows, "desktop-game-runtime");
    MK_REQUIRE(missing != nullptr);
    MK_REQUIRE(missing->status == mirakana::editor::EditorAiPlaytestEvidenceImportStatus::missing);
    MK_REQUIRE(!missing->imported);
    MK_REQUIRE(missing->diagnostic.contains("missing externally supplied evidence"));

    const auto ui = mirakana::editor::make_editor_ai_playtest_evidence_import_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_evidence_import"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_evidence_import.rows.agent-contract.status"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_evidence_import.rows.desktop-game-runtime.diagnostic"}) != nullptr);
}

MK_TEST("editor ai playtest evidence import blocks malformed rows and unsupported claims") {
    mirakana::editor::EditorAiPlaytestEvidenceImportDesc desc;
    desc.expected_recipe_ids = {"agent-contract"};
    desc.request_mutation = true;
    desc.request_validation_execution = true;
    desc.request_arbitrary_shell_execution = true;
    desc.request_package_script_execution = true;
    desc.text = "format=GameEngine.EditorAiPlaytestEvidence.v1\n"
                "evidence.agent-contract.status=passed\n"
                "evidence.agent-contract.status=wat\n"
                "evidence.agent-contract.exit_code=nope\n"
                "evidence.agent-contract.claims_editor_core_execution=true\n"
                "evidence.agent-contract.externally_supplied=false\n"
                "evidence.unknown-recipe.status=passed\n"
                "evidence.unknown-recipe.exit_code=0\n"
                "evidence.agent-contract.unknown_field=value\n";

    const auto model = mirakana::editor::make_editor_ai_playtest_evidence_import_model(desc);

    auto has_diagnostic = [&model](std::string_view text) {
        return std::ranges::any_of(model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(model.status == mirakana::editor::EditorAiPlaytestEvidenceImportStatus::blocked);
    MK_REQUIRE(!model.importable);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(model.evidence_rows.empty());
    MK_REQUIRE(model.review_rows.size() == 2U);

    const auto* agent_contract = find_ai_evidence_import_row(model.review_rows, "agent-contract");
    MK_REQUIRE(agent_contract != nullptr);
    MK_REQUIRE(agent_contract->status == mirakana::editor::EditorAiPlaytestEvidenceImportStatus::blocked);
    MK_REQUIRE(agent_contract->blocked_by.size() >= 4U);

    const auto* unknown = find_ai_evidence_import_row(model.review_rows, "unknown-recipe");
    MK_REQUIRE(unknown != nullptr);
    MK_REQUIRE(unknown->status == mirakana::editor::EditorAiPlaytestEvidenceImportStatus::blocked);
    MK_REQUIRE(unknown->blocked_by[0] == "unexpected-recipe-id");

    MK_REQUIRE(has_diagnostic("duplicate field"));
    MK_REQUIRE(has_diagnostic("invalid evidence status"));
    MK_REQUIRE(has_diagnostic("invalid exit_code"));
    MK_REQUIRE(has_diagnostic("editor-core validation execution"));
    MK_REQUIRE(has_diagnostic("externally supplied"));
    MK_REQUIRE(has_diagnostic("unexpected recipe"));
    MK_REQUIRE(has_diagnostic("unknown evidence import field"));
    MK_REQUIRE(has_diagnostic("mutation"));
    MK_REQUIRE(has_diagnostic("validation execution"));
    MK_REQUIRE(has_diagnostic("arbitrary shell"));
    MK_REQUIRE(has_diagnostic("package script execution"));
}

MK_TEST("editor ai playtest remediation queue classifies failed blocked missing and host gated evidence") {
    mirakana::editor::EditorAiPlaytestEvidenceSummaryModel evidence_summary;
    evidence_summary.status = mirakana::editor::EditorAiPlaytestEvidenceStatus::failed;
    evidence_summary.has_failed_evidence = true;
    evidence_summary.has_missing_evidence = true;
    evidence_summary.has_host_gates = true;
    evidence_summary.rows.push_back(mirakana::editor::EditorAiPlaytestEvidenceSummaryRow{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::passed,
        .handoff_status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .handoff_row_available = true,
        .exit_code = 0,
        .summary = "agent-contract passed",
        .stdout_summary = "agent-check passed",
        .stderr_summary = "",
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "agent evidence passed",
    });
    evidence_summary.rows.push_back(mirakana::editor::EditorAiPlaytestEvidenceSummaryRow{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::failed,
        .handoff_status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .handoff_row_available = true,
        .exit_code = 1,
        .summary = "package smoke failed",
        .stdout_summary = "",
        .stderr_summary = "missing package artifact",
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {"package-artifact-missing"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "package smoke failed",
    });
    evidence_summary.rows.push_back(mirakana::editor::EditorAiPlaytestEvidenceSummaryRow{
        .recipe_id = "shader-toolchain",
        .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::blocked,
        .handoff_status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::blocked,
        .handoff_row_available = true,
        .exit_code = std::nullopt,
        .summary = "dry-run plan missing",
        .stdout_summary = "",
        .stderr_summary = "",
        .host_gates = {},
        .blocked_by = {"missing-reviewed-dry-run-plan"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "operator handoff row is blocked",
    });
    evidence_summary.rows.push_back(mirakana::editor::EditorAiPlaytestEvidenceSummaryRow{
        .recipe_id = "desktop-game-runtime",
        .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::missing,
        .handoff_status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .handoff_row_available = true,
        .exit_code = std::nullopt,
        .summary = "",
        .stdout_summary = "",
        .stderr_summary = "",
        .host_gates = {},
        .blocked_by = {"missing-external-validation-evidence"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "externally supplied validation evidence is missing",
    });
    evidence_summary.rows.push_back(mirakana::editor::EditorAiPlaytestEvidenceSummaryRow{
        .recipe_id = "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package",
        .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::host_gated,
        .handoff_status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .handoff_row_available = true,
        .exit_code = std::nullopt,
        .summary = "Vulkan tools missing",
        .stdout_summary = "",
        .stderr_summary = "",
        .host_gates = {"vulkan-strict"},
        .blocked_by = {"spirv-val-missing"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "strict Vulkan package validation is host-gated",
    });

    const mirakana::editor::EditorAiPlaytestRemediationQueueModel model =
        mirakana::editor::make_editor_ai_playtest_remediation_queue_model(
            mirakana::editor::EditorAiPlaytestRemediationQueueDesc{.evidence_summary = evidence_summary});

    MK_REQUIRE(model.rows.size() == 4);
    MK_REQUIRE(model.rows[0].recipe_id == "desktop-runtime-sample-game-scene-gpu-package");
    MK_REQUIRE(model.rows[0].category == mirakana::editor::EditorAiPlaytestRemediationCategory::investigate_failure);
    MK_REQUIRE(model.rows[0].evidence_status == mirakana::editor::EditorAiPlaytestEvidenceStatus::failed);
    MK_REQUIRE(model.rows[0].exit_code.has_value());
    MK_REQUIRE(model.rows[0].blocked_by[0] == "package-artifact-missing");
    MK_REQUIRE(model.rows[0].next_action.contains("Inspect external validation output"));
    MK_REQUIRE(model.rows[1].category == mirakana::editor::EditorAiPlaytestRemediationCategory::resolve_blocker);
    MK_REQUIRE(model.rows[1].blocked_by[0] == "missing-reviewed-dry-run-plan");
    MK_REQUIRE(model.rows[2].category ==
               mirakana::editor::EditorAiPlaytestRemediationCategory::collect_missing_evidence);
    MK_REQUIRE(model.rows[2].next_action.contains("externally supplied validation evidence"));
    MK_REQUIRE(model.rows[3].category == mirakana::editor::EditorAiPlaytestRemediationCategory::satisfy_host_gate);
    MK_REQUIRE(model.rows[3].host_gates[0] == "vulkan-strict");
    MK_REQUIRE(model.rows[3].readiness_dependency ==
               "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation");
    MK_REQUIRE(model.remediation_required);
    MK_REQUIRE(model.has_failed_evidence);
    MK_REQUIRE(model.has_blocked_evidence);
    MK_REQUIRE(model.has_missing_evidence);
    MK_REQUIRE(model.has_host_gates);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(mirakana::editor::editor_ai_playtest_remediation_category_label(model.rows[2].category) ==
               "collect_missing_evidence");
}

MK_TEST("editor ai playtest remediation queue rejects execution mutation fixes and unsupported claims") {
    mirakana::editor::EditorAiPlaytestEvidenceSummaryModel evidence_summary;
    evidence_summary.status = mirakana::editor::EditorAiPlaytestEvidenceStatus::missing;
    evidence_summary.mutates = true;
    evidence_summary.executes = true;
    evidence_summary.rows.push_back(mirakana::editor::EditorAiPlaytestEvidenceSummaryRow{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::missing,
        .handoff_status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .handoff_row_available = true,
        .exit_code = std::nullopt,
        .summary = "",
        .stdout_summary = "",
        .stderr_summary = "",
        .host_gates = {},
        .blocked_by = {"missing-external-validation-evidence"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "externally supplied validation evidence is missing",
    });

    const mirakana::editor::EditorAiPlaytestRemediationQueueModel model =
        mirakana::editor::make_editor_ai_playtest_remediation_queue_model(
            mirakana::editor::EditorAiPlaytestRemediationQueueDesc{
                .evidence_summary = evidence_summary,
                .request_mutation = true,
                .request_validation_execution = true,
                .request_arbitrary_shell_execution = true,
                .request_raw_manifest_command_evaluation = true,
                .request_package_script_execution = true,
                .request_free_form_manifest_edit = true,
                .request_evidence_mutation = true,
                .request_fix_execution = true,
                .request_play_in_editor_productization = true,
                .request_renderer_rhi_handle_exposure = true,
                .request_metal_readiness_claim = true,
                .request_renderer_quality_claim = true,
            });

    auto has_diagnostic = [&model](std::string_view text) {
        return std::ranges::any_of(model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(model.rows.size() == 1);
    MK_REQUIRE(model.rows[0].category ==
               mirakana::editor::EditorAiPlaytestRemediationCategory::collect_missing_evidence);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(has_diagnostic("evidence summary input must be read-only"));
    MK_REQUIRE(has_diagnostic("evidence summary input must not execute"));
    MK_REQUIRE(has_diagnostic("mutation"));
    MK_REQUIRE(has_diagnostic("validation execution"));
    MK_REQUIRE(has_diagnostic("arbitrary shell"));
    MK_REQUIRE(has_diagnostic("raw/free-form command evaluation"));
    MK_REQUIRE(has_diagnostic("package script execution"));
    MK_REQUIRE(has_diagnostic("free-form manifest edits"));
    MK_REQUIRE(has_diagnostic("evidence mutation"));
    MK_REQUIRE(has_diagnostic("fix execution"));
    MK_REQUIRE(has_diagnostic("play-in-editor productization"));
    MK_REQUIRE(has_diagnostic("renderer/RHI handle exposure"));
    MK_REQUIRE(has_diagnostic("Metal readiness"));
    MK_REQUIRE(has_diagnostic("renderer quality"));
}

MK_TEST("editor ai playtest remediation handoff maps queue rows to external operator actions") {
    mirakana::editor::EditorAiPlaytestRemediationQueueModel remediation_queue;
    remediation_queue.remediation_required = true;
    remediation_queue.has_failed_evidence = true;
    remediation_queue.has_blocked_evidence = true;
    remediation_queue.has_missing_evidence = true;
    remediation_queue.has_host_gates = true;
    remediation_queue.unsupported_claims.emplace_back("renderer quality");
    remediation_queue.rows.push_back(mirakana::editor::EditorAiPlaytestRemediationQueueRow{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .evidence_status = mirakana::editor::EditorAiPlaytestEvidenceStatus::failed,
        .category = mirakana::editor::EditorAiPlaytestRemediationCategory::investigate_failure,
        .exit_code = 1,
        .evidence_summary = "package smoke failed",
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {"package-artifact-missing"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .next_action = "Inspect external validation output before choosing a reviewed fix surface",
        .diagnostic = "package smoke failed",
    });
    remediation_queue.rows.push_back(mirakana::editor::EditorAiPlaytestRemediationQueueRow{
        .recipe_id = "shader-toolchain",
        .evidence_status = mirakana::editor::EditorAiPlaytestEvidenceStatus::blocked,
        .category = mirakana::editor::EditorAiPlaytestRemediationCategory::resolve_blocker,
        .exit_code = std::nullopt,
        .evidence_summary = "dry-run plan missing",
        .host_gates = {},
        .blocked_by = {"missing-reviewed-dry-run-plan"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .next_action = "Resolve blocker before collecting evidence",
        .diagnostic = "operator handoff row is blocked",
    });
    remediation_queue.rows.push_back(mirakana::editor::EditorAiPlaytestRemediationQueueRow{
        .recipe_id = "desktop-game-runtime",
        .evidence_status = mirakana::editor::EditorAiPlaytestEvidenceStatus::missing,
        .category = mirakana::editor::EditorAiPlaytestRemediationCategory::collect_missing_evidence,
        .exit_code = std::nullopt,
        .evidence_summary = "",
        .host_gates = {},
        .blocked_by = {"missing-external-validation-evidence"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .next_action = "Collect externally supplied validation evidence",
        .diagnostic = "externally supplied validation evidence is missing",
    });
    remediation_queue.rows.push_back(mirakana::editor::EditorAiPlaytestRemediationQueueRow{
        .recipe_id = "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package",
        .evidence_status = mirakana::editor::EditorAiPlaytestEvidenceStatus::host_gated,
        .category = mirakana::editor::EditorAiPlaytestRemediationCategory::satisfy_host_gate,
        .exit_code = std::nullopt,
        .evidence_summary = "Vulkan tools missing",
        .host_gates = {"vulkan-strict"},
        .blocked_by = {"spirv-val-missing"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .next_action = "Satisfy host gates before rerunning external validation",
        .diagnostic = "strict Vulkan package validation is host-gated",
    });

    const mirakana::editor::EditorAiPlaytestRemediationHandoffModel model =
        mirakana::editor::make_editor_ai_playtest_remediation_handoff_model(
            mirakana::editor::EditorAiPlaytestRemediationHandoffDesc{.remediation_queue = remediation_queue});

    MK_REQUIRE(model.rows.size() == 4);
    MK_REQUIRE(model.rows[0].recipe_id == "desktop-runtime-sample-game-scene-gpu-package");
    MK_REQUIRE(model.rows[0].evidence_status == mirakana::editor::EditorAiPlaytestEvidenceStatus::failed);
    MK_REQUIRE(model.rows[0].remediation_category ==
               mirakana::editor::EditorAiPlaytestRemediationCategory::investigate_failure);
    MK_REQUIRE(model.rows[0].action_kind ==
               mirakana::editor::EditorAiPlaytestRemediationHandoffActionKind::investigate_external_failure);
    MK_REQUIRE(model.rows[0].external_owner == "external-validation-operator");
    MK_REQUIRE(model.rows[0].handoff_text.contains("outside editor core"));
    MK_REQUIRE(model.rows[0].unsupported_claims[0] == "renderer quality");
    MK_REQUIRE(model.rows[1].action_kind ==
               mirakana::editor::EditorAiPlaytestRemediationHandoffActionKind::resolve_external_blocker);
    MK_REQUIRE(model.rows[1].external_owner == "external-workflow-owner");
    MK_REQUIRE(model.rows[2].action_kind ==
               mirakana::editor::EditorAiPlaytestRemediationHandoffActionKind::collect_external_evidence);
    MK_REQUIRE(model.rows[2].blocked_by[0] == "missing-external-validation-evidence");
    MK_REQUIRE(model.rows[3].action_kind ==
               mirakana::editor::EditorAiPlaytestRemediationHandoffActionKind::satisfy_host_gate);
    MK_REQUIRE(model.rows[3].external_owner == "host-environment-operator");
    MK_REQUIRE(model.rows[3].host_gates[0] == "vulkan-strict");
    MK_REQUIRE(model.rows[3].readiness_dependency ==
               "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation");
    MK_REQUIRE(model.handoff_required);
    MK_REQUIRE(model.has_failed_evidence);
    MK_REQUIRE(model.has_blocked_evidence);
    MK_REQUIRE(model.has_missing_evidence);
    MK_REQUIRE(model.has_host_gates);
    MK_REQUIRE(!model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(mirakana::editor::editor_ai_playtest_remediation_handoff_action_kind_label(model.rows[2].action_kind) ==
               "collect_external_evidence");
}

MK_TEST("editor ai playtest remediation handoff rejects execution mutation remediation edits and unsupported claims") {
    mirakana::editor::EditorAiPlaytestRemediationQueueModel remediation_queue;
    remediation_queue.remediation_required = true;
    remediation_queue.mutates = true;
    remediation_queue.executes = true;
    remediation_queue.unsupported_claims.emplace_back("free-form manifest edits");
    remediation_queue.rows.push_back(mirakana::editor::EditorAiPlaytestRemediationQueueRow{
        .recipe_id = "agent-contract",
        .evidence_status = mirakana::editor::EditorAiPlaytestEvidenceStatus::missing,
        .category = mirakana::editor::EditorAiPlaytestRemediationCategory::collect_missing_evidence,
        .exit_code = std::nullopt,
        .evidence_summary = "",
        .host_gates = {},
        .blocked_by = {"missing-external-validation-evidence"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .next_action = "Collect externally supplied validation evidence",
        .diagnostic = "externally supplied validation evidence is missing",
    });

    const mirakana::editor::EditorAiPlaytestRemediationHandoffModel model =
        mirakana::editor::make_editor_ai_playtest_remediation_handoff_model(
            mirakana::editor::EditorAiPlaytestRemediationHandoffDesc{
                .remediation_queue = remediation_queue,
                .request_mutation = true,
                .request_validation_execution = true,
                .request_arbitrary_shell_execution = true,
                .request_raw_manifest_command_evaluation = true,
                .request_package_script_execution = true,
                .request_free_form_manifest_edit = true,
                .request_evidence_mutation = true,
                .request_remediation_mutation = true,
                .request_fix_execution = true,
                .request_play_in_editor_productization = true,
                .request_renderer_rhi_handle_exposure = true,
                .request_metal_readiness_claim = true,
                .request_renderer_quality_claim = true,
            });

    auto has_diagnostic = [&model](std::string_view text) {
        return std::ranges::any_of(model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(model.rows.size() == 1);
    MK_REQUIRE(model.rows[0].action_kind ==
               mirakana::editor::EditorAiPlaytestRemediationHandoffActionKind::collect_external_evidence);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(has_diagnostic("remediation queue input must be read-only"));
    MK_REQUIRE(has_diagnostic("remediation queue input must not execute"));
    MK_REQUIRE(has_diagnostic("mutation"));
    MK_REQUIRE(has_diagnostic("validation execution"));
    MK_REQUIRE(has_diagnostic("arbitrary shell"));
    MK_REQUIRE(has_diagnostic("raw/free-form command evaluation"));
    MK_REQUIRE(has_diagnostic("package script execution"));
    MK_REQUIRE(has_diagnostic("free-form manifest edits"));
    MK_REQUIRE(has_diagnostic("evidence mutation"));
    MK_REQUIRE(has_diagnostic("remediation mutation"));
    MK_REQUIRE(has_diagnostic("fix execution"));
    MK_REQUIRE(has_diagnostic("play-in-editor productization"));
    MK_REQUIRE(has_diagnostic("renderer/RHI handle exposure"));
    MK_REQUIRE(has_diagnostic("Metal readiness"));
    MK_REQUIRE(has_diagnostic("renderer quality"));
}

MK_TEST("editor ai playtest operator workflow report surfaces existing model rows") {
    mirakana::editor::EditorAiPackageAuthoringDiagnosticsModel package_diagnostics;
    package_diagnostics.has_host_gated_recipes = true;
    package_diagnostics.descriptor_rows.push_back(mirakana::editor::EditorAiPackageDescriptorDiagnosticRow{
        .id = "runtime-scene-target",
        .surface = "game.agent.json.runtimeSceneValidationTargets",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .mutates = false,
        .executes = false,
        .diagnostic = "runtime scene validation target selected",
    });
    package_diagnostics.payload_rows.push_back(mirakana::editor::EditorAiPackagePayloadDiagnosticRow{
        .id = "runtime-package-payloads",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .diagnostic = "runtime package payloads are inspectable",
    });
    package_diagnostics.validation_recipe_rows.push_back(mirakana::editor::EditorAiPackageValidationRecipeDiagnosticRow{
        .id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .host_gated = true,
        .executes = false,
        .diagnostic = "desktop package smoke remains host-gated",
    });

    mirakana::editor::EditorAiValidationRecipePreflightModel validation_preflight;
    validation_preflight.has_host_gated_recipes = true;
    validation_preflight.rows.push_back(mirakana::editor::EditorAiValidationRecipePreflightRow{
        .id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .manifest_declared = true,
        .reviewed_dry_run_available = true,
        .host_gates = {},
        .blocked_by = {},
        .diagnostic = "agent contract dry-run ready",
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode "
                           "Execute -Recipe agent-contract",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "Execute", "-Recipe", "agent-contract"},
    });
    validation_preflight.rows.push_back(mirakana::editor::EditorAiValidationRecipePreflightRow{
        .id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .manifest_declared = true,
        .reviewed_dry_run_available = true,
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {},
        .diagnostic = "D3D12 package validation dry-run ready",
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget "
            "sample_desktop_runtime_game",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/package-desktop-runtime.ps1",
                 "-GameTarget", "sample_desktop_runtime_game"},
    });

    mirakana::editor::EditorAiPlaytestReadinessReportModel readiness_report;
    readiness_report.status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated;
    readiness_report.ready_for_operator_validation = true;
    readiness_report.has_host_gates = true;
    readiness_report.rows.push_back(mirakana::editor::EditorAiPlaytestReadinessReportRow{
        .id = "package-authoring-diagnostics",
        .surface = "EditorAiPackageAuthoringDiagnosticsModel",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .diagnostic = "package diagnostics are ready",
    });
    readiness_report.rows.push_back(mirakana::editor::EditorAiPlaytestReadinessReportRow{
        .id = "validation-recipe-preflight",
        .surface = "EditorAiValidationRecipePreflightModel",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .diagnostic = "validation preflight is host-gated",
    });

    mirakana::editor::EditorAiPlaytestOperatorHandoffModel operator_handoff;
    operator_handoff.status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated;
    operator_handoff.ready_for_operator_handoff = true;
    operator_handoff.has_host_gates = true;
    operator_handoff.command_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode "
                           "Execute -Recipe agent-contract",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "Execute", "-Recipe", "agent-contract"},
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "agent contract handoff ready",
    });
    operator_handoff.command_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget "
            "sample_desktop_runtime_game",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/package-desktop-runtime.ps1",
                 "-GameTarget", "sample_desktop_runtime_game"},
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "D3D12 package handoff is host-gated",
    });

    mirakana::editor::EditorAiPlaytestEvidenceSummaryModel evidence_summary;
    evidence_summary.status = mirakana::editor::EditorAiPlaytestEvidenceStatus::failed;
    evidence_summary.has_failed_evidence = true;
    evidence_summary.has_host_gates = true;
    evidence_summary.rows.push_back(mirakana::editor::EditorAiPlaytestEvidenceSummaryRow{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::passed,
        .handoff_status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .handoff_row_available = true,
        .exit_code = 0,
        .summary = "agent-contract passed",
        .stdout_summary = "agent-check and schema-check passed",
        .stderr_summary = "",
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "agent evidence passed",
    });
    evidence_summary.rows.push_back(mirakana::editor::EditorAiPlaytestEvidenceSummaryRow{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::failed,
        .handoff_status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .handoff_row_available = true,
        .exit_code = 1,
        .summary = "package smoke failed",
        .stdout_summary = "",
        .stderr_summary = "missing package artifact",
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {"package-artifact-missing"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "package smoke failed",
    });

    mirakana::editor::EditorAiPlaytestRemediationQueueModel remediation_queue;
    remediation_queue.remediation_required = true;
    remediation_queue.has_failed_evidence = true;
    remediation_queue.has_host_gates = true;
    remediation_queue.rows.push_back(mirakana::editor::EditorAiPlaytestRemediationQueueRow{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .evidence_status = mirakana::editor::EditorAiPlaytestEvidenceStatus::failed,
        .category = mirakana::editor::EditorAiPlaytestRemediationCategory::investigate_failure,
        .exit_code = 1,
        .evidence_summary = "package smoke failed",
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {"package-artifact-missing"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .next_action = "Inspect external validation output before choosing a reviewed fix surface",
        .diagnostic = "package smoke failed",
    });

    mirakana::editor::EditorAiPlaytestRemediationHandoffModel remediation_handoff;
    remediation_handoff.handoff_required = true;
    remediation_handoff.has_failed_evidence = true;
    remediation_handoff.has_host_gates = true;
    remediation_handoff.rows.push_back(mirakana::editor::EditorAiPlaytestRemediationHandoffRow{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .evidence_status = mirakana::editor::EditorAiPlaytestEvidenceStatus::failed,
        .remediation_category = mirakana::editor::EditorAiPlaytestRemediationCategory::investigate_failure,
        .action_kind = mirakana::editor::EditorAiPlaytestRemediationHandoffActionKind::investigate_external_failure,
        .external_owner = "external-validation-operator",
        .handoff_text = "Investigate package smoke failure outside editor core",
        .exit_code = 1,
        .evidence_summary = "package smoke failed",
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {"package-artifact-missing"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .unsupported_claims = {},
        .diagnostic = "package smoke failed",
    });

    const mirakana::editor::EditorAiPlaytestOperatorWorkflowReportModel model =
        mirakana::editor::make_editor_ai_playtest_operator_workflow_report_model(
            mirakana::editor::EditorAiPlaytestOperatorWorkflowReportDesc{
                .package_diagnostics = package_diagnostics,
                .validation_preflight = validation_preflight,
                .readiness_report = readiness_report,
                .operator_handoff = operator_handoff,
                .evidence_summary = evidence_summary,
                .remediation_queue = remediation_queue,
                .remediation_handoff = remediation_handoff,
            });

    MK_REQUIRE(model.stage_rows.size() == 8);
    MK_REQUIRE(model.stage_rows[0].id == "package-diagnostics");
    MK_REQUIRE(model.stage_rows[0].source_model == "EditorAiPackageAuthoringDiagnosticsModel");
    MK_REQUIRE(model.stage_rows[0].source_row_count == 3);
    MK_REQUIRE(model.stage_rows[0].source_row_ids[0] == "runtime-scene-target");
    MK_REQUIRE(model.stage_rows[1].id == "validation-preflight");
    MK_REQUIRE(model.stage_rows[1].source_model == "EditorAiValidationRecipePreflightModel");
    MK_REQUIRE(model.stage_rows[1].source_row_count == 2);
    MK_REQUIRE(model.stage_rows[3].id == "operator-handoff");
    MK_REQUIRE(model.stage_rows[3].source_row_ids[1] == "desktop-runtime-sample-game-scene-gpu-package");
    MK_REQUIRE(model.stage_rows[4].status ==
               mirakana::editor::EditorAiPlaytestOperatorWorkflowStageStatus::remediation_required);
    MK_REQUIRE(model.stage_rows[5].source_model == "EditorAiPlaytestRemediationQueueModel");
    MK_REQUIRE(model.stage_rows[5].status ==
               mirakana::editor::EditorAiPlaytestOperatorWorkflowStageStatus::remediation_required);
    MK_REQUIRE(model.stage_rows[6].status ==
               mirakana::editor::EditorAiPlaytestOperatorWorkflowStageStatus::handoff_required);
    MK_REQUIRE(model.stage_rows[7].id == "closeout");
    MK_REQUIRE(model.stage_rows[7].source_model ==
               "EditorAiPlaytestEvidenceSummaryModel+EditorAiPlaytestRemediationQueueModel+"
               "EditorAiPlaytestRemediationHandoffModel");
    MK_REQUIRE(model.stage_rows[7].status ==
               mirakana::editor::EditorAiPlaytestOperatorWorkflowStageStatus::handoff_required);
    MK_REQUIRE(model.remediation_required);
    MK_REQUIRE(model.handoff_required);
    MK_REQUIRE(model.external_action_required);
    MK_REQUIRE(!model.closeout_complete);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(mirakana::editor::editor_ai_playtest_operator_workflow_stage_status_label(model.stage_rows[7].status) ==
               "handoff_required");
}

MK_TEST("editor ai playtest operator workflow report closes through evidence summary rerun") {
    mirakana::editor::EditorAiPackageAuthoringDiagnosticsModel package_diagnostics;

    mirakana::editor::EditorAiValidationRecipePreflightModel validation_preflight;
    validation_preflight.rows.push_back(mirakana::editor::EditorAiValidationRecipePreflightRow{
        .id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .manifest_declared = true,
        .reviewed_dry_run_available = true,
        .host_gates = {},
        .blocked_by = {},
        .diagnostic = "agent contract dry-run ready",
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode "
                           "Execute -Recipe agent-contract",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "Execute", "-Recipe", "agent-contract"},
    });

    mirakana::editor::EditorAiPlaytestReadinessReportModel readiness_report;
    readiness_report.status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready;
    readiness_report.ready_for_operator_validation = true;
    readiness_report.rows.push_back(mirakana::editor::EditorAiPlaytestReadinessReportRow{
        .id = "package-authoring-diagnostics",
        .surface = "EditorAiPackageAuthoringDiagnosticsModel",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .diagnostic = "package diagnostics are ready",
    });

    mirakana::editor::EditorAiPlaytestOperatorHandoffModel operator_handoff;
    operator_handoff.status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready;
    operator_handoff.ready_for_operator_handoff = true;
    operator_handoff.command_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode "
                           "Execute -Recipe agent-contract",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "Execute", "-Recipe", "agent-contract"},
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "agent contract handoff ready",
    });

    mirakana::editor::EditorAiPlaytestEvidenceSummaryModel evidence_summary;
    evidence_summary.status = mirakana::editor::EditorAiPlaytestEvidenceStatus::passed;
    evidence_summary.all_required_evidence_passed = true;
    evidence_summary.rows.push_back(mirakana::editor::EditorAiPlaytestEvidenceSummaryRow{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::passed,
        .handoff_status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .handoff_row_available = true,
        .exit_code = 0,
        .summary = "agent-contract passed after external remediation",
        .stdout_summary = "agent-check and schema-check passed",
        .stderr_summary = "",
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "evidence summary rerun passed",
    });

    const mirakana::editor::EditorAiPlaytestOperatorWorkflowReportModel model =
        mirakana::editor::make_editor_ai_playtest_operator_workflow_report_model(
            mirakana::editor::EditorAiPlaytestOperatorWorkflowReportDesc{
                .package_diagnostics = package_diagnostics,
                .validation_preflight = validation_preflight,
                .readiness_report = readiness_report,
                .operator_handoff = operator_handoff,
                .evidence_summary = evidence_summary,
                .remediation_queue = mirakana::editor::EditorAiPlaytestRemediationQueueModel{},
                .remediation_handoff = mirakana::editor::EditorAiPlaytestRemediationHandoffModel{},
            });

    MK_REQUIRE(model.stage_rows.size() == 8);
    MK_REQUIRE(model.stage_rows[4].status == mirakana::editor::EditorAiPlaytestOperatorWorkflowStageStatus::ready);
    MK_REQUIRE(model.stage_rows[5].status == mirakana::editor::EditorAiPlaytestOperatorWorkflowStageStatus::ready);
    MK_REQUIRE(model.stage_rows[6].status == mirakana::editor::EditorAiPlaytestOperatorWorkflowStageStatus::ready);
    MK_REQUIRE(model.stage_rows[7].status == mirakana::editor::EditorAiPlaytestOperatorWorkflowStageStatus::closed);
    MK_REQUIRE(model.closeout_complete);
    MK_REQUIRE(!model.remediation_required);
    MK_REQUIRE(!model.handoff_required);
    MK_REQUIRE(!model.external_action_required);
}

MK_TEST("editor ai playtest operator workflow report rejects execution mutation and unsupported claims") {
    mirakana::editor::EditorAiPackageAuthoringDiagnosticsModel package_diagnostics;
    package_diagnostics.mutates = true;
    package_diagnostics.executes = true;
    mirakana::editor::EditorAiValidationRecipePreflightModel validation_preflight;
    validation_preflight.executes = true;
    mirakana::editor::EditorAiPlaytestReadinessReportModel readiness_report;
    readiness_report.mutates = true;
    readiness_report.executes = true;
    mirakana::editor::EditorAiPlaytestOperatorHandoffModel operator_handoff;
    operator_handoff.mutates = true;
    operator_handoff.executes = true;
    mirakana::editor::EditorAiPlaytestEvidenceSummaryModel evidence_summary;
    evidence_summary.mutates = true;
    evidence_summary.executes = true;
    mirakana::editor::EditorAiPlaytestRemediationQueueModel remediation_queue;
    remediation_queue.mutates = true;
    remediation_queue.executes = true;
    mirakana::editor::EditorAiPlaytestRemediationHandoffModel remediation_handoff;
    remediation_handoff.mutates = true;
    remediation_handoff.executes = true;

    const mirakana::editor::EditorAiPlaytestOperatorWorkflowReportModel model =
        mirakana::editor::make_editor_ai_playtest_operator_workflow_report_model(
            mirakana::editor::EditorAiPlaytestOperatorWorkflowReportDesc{
                .package_diagnostics = package_diagnostics,
                .validation_preflight = validation_preflight,
                .readiness_report = readiness_report,
                .operator_handoff = operator_handoff,
                .evidence_summary = evidence_summary,
                .remediation_queue = remediation_queue,
                .remediation_handoff = remediation_handoff,
                .request_mutation = true,
                .request_manifest_mutation = true,
                .request_validation_execution = true,
                .request_arbitrary_shell_execution = true,
                .request_raw_manifest_command_evaluation = true,
                .request_free_form_command_evaluation = true,
                .request_package_script_execution = true,
                .request_free_form_manifest_edit = true,
                .request_evidence_mutation = true,
                .request_remediation_mutation = true,
                .request_fix_execution = true,
                .request_play_in_editor_productization = true,
                .request_renderer_rhi_handle_exposure = true,
                .request_metal_readiness_claim = true,
                .request_renderer_quality_claim = true,
            });

    auto has_diagnostic = [&model](std::string_view text) {
        return std::ranges::any_of(model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.closeout_complete);
    MK_REQUIRE(model.external_action_required);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(has_diagnostic("EditorAiPackageAuthoringDiagnosticsModel input must be read-only"));
    MK_REQUIRE(has_diagnostic("EditorAiPlaytestRemediationHandoffModel input must not execute"));
    MK_REQUIRE(has_diagnostic("mutation"));
    MK_REQUIRE(has_diagnostic("manifest mutation"));
    MK_REQUIRE(has_diagnostic("validation execution"));
    MK_REQUIRE(has_diagnostic("arbitrary shell"));
    MK_REQUIRE(has_diagnostic("raw/free-form command evaluation"));
    MK_REQUIRE(has_diagnostic("package script execution"));
    MK_REQUIRE(has_diagnostic("free-form manifest edits"));
    MK_REQUIRE(has_diagnostic("evidence mutation"));
    MK_REQUIRE(has_diagnostic("remediation mutation"));
    MK_REQUIRE(has_diagnostic("fix execution"));
    MK_REQUIRE(has_diagnostic("play-in-editor productization"));
    MK_REQUIRE(has_diagnostic("renderer/RHI handle exposure"));
    MK_REQUIRE(has_diagnostic("Metal readiness"));
    MK_REQUIRE(has_diagnostic("renderer quality"));
}

MK_TEST("editor ai command panel summarizes workflow command and evidence rows") {
    mirakana::editor::EditorAiPlaytestOperatorWorkflowReportModel workflow;
    workflow.stage_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorWorkflowStageRow{
        .id = "package-diagnostics",
        .source_model = "EditorAiPackageAuthoringDiagnosticsModel",
        .status = mirakana::editor::EditorAiPlaytestOperatorWorkflowStageStatus::ready,
        .source_row_count = 3,
        .source_row_ids = {"runtime-scene-target", "runtime-package-payloads", "agent-contract"},
        .host_gates = {},
        .blocked_by = {},
        .diagnostic = "package diagnostics ready",
    });
    workflow.stage_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorWorkflowStageRow{
        .id = "validation-preflight",
        .source_model = "EditorAiValidationRecipePreflightModel",
        .status = mirakana::editor::EditorAiPlaytestOperatorWorkflowStageStatus::host_gated,
        .source_row_count = 2,
        .source_row_ids = {"agent-contract", "desktop-runtime-sample-game-scene-gpu-package"},
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {},
        .diagnostic = "D3D12 package smoke is host-gated",
    });
    workflow.handoff_required = true;
    workflow.external_action_required = true;
    workflow.has_host_gates = true;

    mirakana::editor::EditorAiPlaytestOperatorHandoffModel handoff;
    handoff.status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated;
    handoff.ready_for_operator_handoff = true;
    handoff.has_host_gates = true;
    handoff.command_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode "
                           "Execute -Recipe agent-contract",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "Execute", "-Recipe", "agent-contract"},
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "agent contract handoff ready",
    });
    handoff.command_rows.push_back(mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget "
            "sample_desktop_runtime_game",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/package-desktop-runtime.ps1",
                 "-GameTarget", "sample_desktop_runtime_game"},
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "D3D12 package handoff is host-gated",
    });

    mirakana::editor::EditorAiPlaytestEvidenceSummaryModel evidence;
    evidence.status = mirakana::editor::EditorAiPlaytestEvidenceStatus::missing;
    evidence.has_missing_evidence = true;
    evidence.has_host_gates = true;
    evidence.rows.push_back(mirakana::editor::EditorAiPlaytestEvidenceSummaryRow{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPlaytestEvidenceStatus::missing,
        .handoff_status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .handoff_row_available = true,
        .exit_code = std::nullopt,
        .summary = "external operator evidence is missing",
        .stdout_summary = "",
        .stderr_summary = "",
        .host_gates = {},
        .blocked_by = {"external-evidence-missing"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "collect external validation evidence",
    });

    const mirakana::editor::EditorAiCommandPanelModel model =
        mirakana::editor::make_editor_ai_command_panel_model(mirakana::editor::EditorAiCommandPanelDesc{
            .workflow_report = workflow, .operator_handoff = handoff, .evidence_summary = evidence});

    MK_REQUIRE(model.status == mirakana::editor::EditorAiCommandPanelStatus::external_action_required);
    MK_REQUIRE(model.status_label == "external_action_required");
    MK_REQUIRE(model.stage_rows.size() == 2);
    MK_REQUIRE(model.ready_stage_count == 1);
    MK_REQUIRE(model.host_gated_stage_count == 1);
    MK_REQUIRE(model.command_rows.size() == 2);
    MK_REQUIRE(model.command_rows[1].recipe_id == "desktop-runtime-sample-game-scene-gpu-package");
    MK_REQUIRE(model.command_rows[1].host_gates[0] == "d3d12-windows-primary");
    MK_REQUIRE(model.evidence_rows.size() == 1);
    MK_REQUIRE(model.evidence_rows[0].status_label == "missing");
    MK_REQUIRE(model.external_action_required);
    MK_REQUIRE(model.has_host_gates);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);

    const auto ui = mirakana::editor::make_ai_command_panel_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands.stages.package-diagnostics"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands.commands.agent-contract.command"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands.evidence.agent-contract.summary"}) != nullptr);
}

MK_TEST("editor ai command panel blocks mutating or executing workflow inputs") {
    mirakana::editor::EditorAiPlaytestOperatorWorkflowReportModel workflow;
    workflow.mutates = true;
    workflow.executes = true;
    workflow.has_blocking_diagnostics = true;
    workflow.diagnostics.emplace_back("upstream workflow blocked");

    const mirakana::editor::EditorAiCommandPanelModel model =
        mirakana::editor::make_editor_ai_command_panel_model(mirakana::editor::EditorAiCommandPanelDesc{
            .workflow_report = workflow,
            .operator_handoff = mirakana::editor::EditorAiPlaytestOperatorHandoffModel{},
            .evidence_summary = mirakana::editor::EditorAiPlaytestEvidenceSummaryModel{}});

    auto has_diagnostic = [&model](std::string_view text) {
        return std::ranges::any_of(model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(model.status == mirakana::editor::EditorAiCommandPanelStatus::blocked);
    MK_REQUIRE(model.has_blocking_diagnostics);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(has_diagnostic("read-only"));
    MK_REQUIRE(has_diagnostic("must not execute"));
    MK_REQUIRE(has_diagnostic("upstream workflow blocked"));
}

MK_TEST("editor ai reviewed validation execution plan converts reviewed dry run argv") {
    mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow row{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun "
                           "-Recipe agent-contract",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "agent-contract"},
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "agent contract handoff ready",
    };

    const auto model = mirakana::editor::make_editor_ai_reviewed_validation_execution_plan(
        mirakana::editor::EditorAiReviewedValidationExecutionDesc{
            .command_row = row,
            .working_directory = "G:/workspace/development/GameEngine",
            .acknowledge_host_gates = false,
            .acknowledged_host_gates = {},
        });

    MK_REQUIRE(model.status == mirakana::editor::EditorAiReviewedValidationExecutionStatus::ready);
    MK_REQUIRE(model.status_label == "ready");
    MK_REQUIRE(model.recipe_id == "agent-contract");
    MK_REQUIRE(model.can_execute);
    MK_REQUIRE(!model.mutates);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(model.command.executable == "pwsh");
    MK_REQUIRE(model.command.arguments.size() == 9U);
    MK_REQUIRE(model.command.arguments[0] == "-NoProfile");
    MK_REQUIRE(model.command.arguments[1] == "-ExecutionPolicy");
    MK_REQUIRE(model.command.arguments[2] == "Bypass");
    MK_REQUIRE(model.command.arguments[3] == "-File");
    MK_REQUIRE(model.command.arguments[4] == "tools/run-validation-recipe.ps1");
    MK_REQUIRE(model.command.arguments[5] == "-Mode");
    MK_REQUIRE(model.command.arguments[6] == "Execute");
    MK_REQUIRE(model.command.arguments[7] == "-Recipe");
    MK_REQUIRE(model.command.arguments[8] == "agent-contract");
    MK_REQUIRE(model.command.working_directory == "G:/workspace/development/GameEngine");
    MK_REQUIRE(mirakana::is_safe_reviewed_validation_recipe_invocation(model.command));
    MK_REQUIRE(!mirakana::is_safe_process_command(model.command));

    const auto ui = mirakana::editor::make_ai_reviewed_validation_execution_ui_model(model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands.execution"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands.execution.agent-contract.command"}) != nullptr);
}

MK_TEST("editor ai reviewed validation execution plan blocks unsafe or host gated rows") {
    mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow host_gated{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe "
            "desktop-runtime-sample-game-scene-gpu-package -GameTarget sample_desktop_runtime_game",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "desktop-runtime-sample-game-scene-gpu-package", "-GameTarget",
                 "sample_desktop_runtime_game"},
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "D3D12 package handoff is host-gated",
    };

    const auto host_model = mirakana::editor::make_editor_ai_reviewed_validation_execution_plan(
        mirakana::editor::EditorAiReviewedValidationExecutionDesc{
            .command_row = host_gated,
            .working_directory = "G:/workspace/development/GameEngine",
            .acknowledge_host_gates = false,
            .acknowledged_host_gates = {},
        });

    MK_REQUIRE(host_model.status == mirakana::editor::EditorAiReviewedValidationExecutionStatus::host_gated);
    MK_REQUIRE(!host_model.can_execute);
    MK_REQUIRE(host_model.host_gates[0] == "d3d12-windows-primary");
    MK_REQUIRE(host_model.diagnostics[0].contains("host-gated"));

    mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow unsafe{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .command_display = "pwsh -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe agent-contract",
        .argv = {"pwsh", "-File", "tools/run-validation-recipe.ps1", "-Mode", "DryRun", "-Recipe", "agent-contract"},
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "unsafe shell handoff",
    };

    mirakana::editor::EditorAiReviewedValidationExecutionDesc desc{
        .command_row = unsafe,
        .working_directory = "G:/workspace/development/GameEngine",
        .acknowledge_host_gates = false,
        .acknowledged_host_gates = {},
    };
    desc.request_validation_execution = true;
    desc.request_arbitrary_shell_execution = true;
    desc.request_raw_manifest_command_evaluation = true;
    desc.request_package_script_execution = true;
    desc.request_free_form_manifest_edit = true;

    const auto blocked_model = mirakana::editor::make_editor_ai_reviewed_validation_execution_plan(desc);

    auto has_diagnostic = [&blocked_model](std::string_view text) {
        return std::ranges::any_of(blocked_model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(blocked_model.status == mirakana::editor::EditorAiReviewedValidationExecutionStatus::blocked);
    MK_REQUIRE(!blocked_model.can_execute);
    MK_REQUIRE(!blocked_model.mutates);
    MK_REQUIRE(!blocked_model.executes);
    MK_REQUIRE(has_diagnostic("reviewed run-validation-recipe argv"));
    MK_REQUIRE(has_diagnostic("editor core must not execute validation recipes"));
    MK_REQUIRE(has_diagnostic("arbitrary shell"));
    MK_REQUIRE(has_diagnostic("raw manifest command"));
    MK_REQUIRE(has_diagnostic("package script"));
    MK_REQUIRE(has_diagnostic("free-form manifest"));
}

MK_TEST("editor ai reviewed validation execution requires host gate acknowledgement") {
    mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow row{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe "
            "desktop-runtime-sample-game-scene-gpu-package -GameTarget sample_desktop_runtime_game",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "desktop-runtime-sample-game-scene-gpu-package", "-GameTarget",
                 "sample_desktop_runtime_game"},
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "D3D12 package handoff is host-gated",
    };

    const auto blocked_model = mirakana::editor::make_editor_ai_reviewed_validation_execution_plan(
        mirakana::editor::EditorAiReviewedValidationExecutionDesc{
            .command_row = row,
            .working_directory = "G:/workspace/development/GameEngine",
            .acknowledge_host_gates = false,
            .acknowledged_host_gates = {},
        });

    MK_REQUIRE(blocked_model.status == mirakana::editor::EditorAiReviewedValidationExecutionStatus::host_gated);
    MK_REQUIRE(!blocked_model.can_execute);
    MK_REQUIRE(blocked_model.host_gate_acknowledgement_required);
    MK_REQUIRE(!blocked_model.host_gates_acknowledged);
    MK_REQUIRE(blocked_model.command.executable.empty());
    MK_REQUIRE(std::ranges::any_of(blocked_model.diagnostics, [](const std::string& diagnostic) {
        return diagnostic.contains("requires host-gate acknowledgement");
    }));

    mirakana::editor::EditorAiReviewedValidationExecutionDesc desc{
        .command_row = row,
        .working_directory = "G:/workspace/development/GameEngine",
        .acknowledge_host_gates = true,
        .acknowledged_host_gates = {"d3d12-windows-primary"},
    };

    const auto acknowledged_model = mirakana::editor::make_editor_ai_reviewed_validation_execution_plan(desc);

    MK_REQUIRE(acknowledged_model.status == mirakana::editor::EditorAiReviewedValidationExecutionStatus::ready);
    MK_REQUIRE(acknowledged_model.can_execute);
    MK_REQUIRE(acknowledged_model.host_gate_acknowledgement_required);
    MK_REQUIRE(acknowledged_model.host_gates_acknowledged);
    MK_REQUIRE(acknowledged_model.command.executable == "pwsh");
    MK_REQUIRE(acknowledged_model.command.arguments.size() == 13U);
    MK_REQUIRE(acknowledged_model.command.arguments[5] == "-Mode");
    MK_REQUIRE(acknowledged_model.command.arguments[6] == "Execute");
    MK_REQUIRE(acknowledged_model.command.arguments[7] == "-Recipe");
    MK_REQUIRE(acknowledged_model.command.arguments[8] == "desktop-runtime-sample-game-scene-gpu-package");
    MK_REQUIRE(acknowledged_model.command.arguments[9] == "-GameTarget");
    MK_REQUIRE(acknowledged_model.command.arguments[10] == "sample_desktop_runtime_game");
    MK_REQUIRE(acknowledged_model.command.arguments[11] == "-HostGateAcknowledgements");
    MK_REQUIRE(acknowledged_model.command.arguments[12] == "d3d12-windows-primary");
    MK_REQUIRE(mirakana::is_safe_reviewed_validation_recipe_invocation(acknowledged_model.command));
    MK_REQUIRE(!mirakana::is_safe_process_command(acknowledged_model.command));

    const auto ui = mirakana::editor::make_ai_reviewed_validation_execution_ui_model(acknowledged_model);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands.execution.desktop-runtime-sample-game-scene-gpu-package."
                                               "host_gate_acknowledgement_required"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{
                   "ai_commands.execution.desktop-runtime-sample-game-scene-gpu-package.host_gates_acknowledged"}) !=
               nullptr);
}

MK_TEST("editor ai reviewed validation execution rejects mismatched host gate acknowledgement") {
    mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow row{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe "
            "desktop-runtime-sample-game-scene-gpu-package -GameTarget sample_desktop_runtime_game",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "desktop-runtime-sample-game-scene-gpu-package", "-GameTarget",
                 "sample_desktop_runtime_game"},
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "D3D12 package handoff is host-gated",
    };

    mirakana::editor::EditorAiReviewedValidationExecutionDesc desc{
        .command_row = row,
        .working_directory = "G:/workspace/development/GameEngine",
        .acknowledge_host_gates = true,
        .acknowledged_host_gates = {"metal-apple"},
        .request_package_script_execution = true,
    };

    const auto model = mirakana::editor::make_editor_ai_reviewed_validation_execution_plan(desc);

    auto has_diagnostic = [&model](std::string_view text) {
        return std::ranges::any_of(model.diagnostics,
                                   [text](const std::string& diagnostic) { return diagnostic.contains(text); });
    };

    MK_REQUIRE(model.status == mirakana::editor::EditorAiReviewedValidationExecutionStatus::blocked);
    MK_REQUIRE(!model.can_execute);
    MK_REQUIRE(model.host_gate_acknowledgement_required);
    MK_REQUIRE(!model.host_gates_acknowledged);
    MK_REQUIRE(has_diagnostic("missing host-gate acknowledgement"));
    MK_REQUIRE(has_diagnostic("unknown host-gate acknowledgement"));
    MK_REQUIRE(has_diagnostic("package script"));
}

MK_TEST("editor ai reviewed validation batch execution collects ready acknowledged plans") {
    mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow agent_contract{
        .recipe_id = "agent-contract",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun "
                           "-Recipe agent-contract",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "agent-contract"},
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "agent contract handoff ready",
    };
    mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow d3d12_package{
        .recipe_id = "desktop-runtime-sample-game-scene-gpu-package",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe "
            "desktop-runtime-sample-game-scene-gpu-package -GameTarget sample_desktop_runtime_game",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "desktop-runtime-sample-game-scene-gpu-package", "-GameTarget",
                 "sample_desktop_runtime_game"},
        .host_gates = {"d3d12-windows-primary"},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "D3D12 package handoff is host-gated",
    };
    mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow vulkan_package{
        .recipe_id = "vulkan-package-smoke",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe "
            "vulkan-package-smoke",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "vulkan-package-smoke"},
        .host_gates = {"vulkan-runtime"},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "Vulkan package handoff is host-gated",
    };
    mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow blocked_recipe{
        .recipe_id = "blocked-recipe",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::blocked,
        .command_display = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun "
                           "-Recipe blocked-recipe",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "blocked-recipe"},
        .host_gates = {},
        .blocked_by = {"upstream-blocker"},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "blocked handoff row",
    };

    const auto batch = mirakana::editor::make_editor_ai_reviewed_validation_execution_batch(
        mirakana::editor::EditorAiReviewedValidationExecutionBatchDesc{
            .command_rows = {agent_contract, d3d12_package, vulkan_package, blocked_recipe},
            .working_directory = "G:/workspace/development/GameEngine",
            .acknowledged_host_gate_recipe_ids = {"desktop-runtime-sample-game-scene-gpu-package"},
        });

    MK_REQUIRE(batch.plans.size() == 4U);
    MK_REQUIRE(batch.ready_count == 2U);
    MK_REQUIRE(batch.host_gated_count == 1U);
    MK_REQUIRE(batch.blocked_count == 1U);
    MK_REQUIRE(batch.can_execute_any);
    const std::vector<std::size_t> expected_executable_plan_indexes{0U, 1U};
    MK_REQUIRE(batch.executable_plan_indexes == expected_executable_plan_indexes);
    MK_REQUIRE(batch.commands.size() == 2U);
    MK_REQUIRE(batch.commands[0].arguments[6] == "Execute");
    MK_REQUIRE(batch.commands[1].arguments[11] == "-HostGateAcknowledgements");
    MK_REQUIRE(batch.commands[1].arguments[12] == "d3d12-windows-primary");
    MK_REQUIRE(batch.plans[2].status == mirakana::editor::EditorAiReviewedValidationExecutionStatus::host_gated);
    MK_REQUIRE(batch.plans[3].status == mirakana::editor::EditorAiReviewedValidationExecutionStatus::blocked);

    const auto ui = mirakana::editor::make_ai_reviewed_validation_execution_batch_ui_model(batch);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands.execution.batch.ready_count"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands.execution.batch.host_gated_count"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands.execution.batch.blocked_count"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"ai_commands.execution.batch.executable_count"}) != nullptr);
}

MK_TEST("editor file text store writes under its root and rejects traversal") {
    const auto root = std::filesystem::temp_directory_path() / "MK_editor_core_file_store_test";
    std::filesystem::remove_all(root);
    mirakana::editor::FileTextStore store(root);

    store.write_text("projects/sample/game.geproject", "project");
    MK_REQUIRE(store.exists("projects/sample/game.geproject"));
    MK_REQUIRE(store.read_text("projects/sample/game.geproject") == "project");

    bool rejected_traversal = false;
    try {
        store.write_text("../outside.txt", "bad");
    } catch (const std::invalid_argument&) {
        rejected_traversal = true;
    }

    bool rejected_absolute = false;
    try {
        (void)store.read_text("C:/outside.txt");
    } catch (const std::invalid_argument&) {
        rejected_absolute = true;
    }

    MK_REQUIRE(rejected_traversal);
    MK_REQUIRE(rejected_absolute);

    std::filesystem::remove_all(root);
}

MK_TEST("editor dirty state tracks changes and saved revision") {
    mirakana::editor::DocumentDirtyState dirty;

    MK_REQUIRE(!dirty.dirty());
    MK_REQUIRE(dirty.revision() == 0);
    MK_REQUIRE(dirty.saved_revision() == 0);

    dirty.mark_dirty();
    dirty.mark_dirty();
    MK_REQUIRE(dirty.dirty());
    MK_REQUIRE(dirty.revision() == 2);
    MK_REQUIRE(dirty.saved_revision() == 0);

    dirty.mark_saved();
    MK_REQUIRE(!dirty.dirty());
    MK_REQUIRE(dirty.saved_revision() == 2);

    dirty.mark_dirty();
    MK_REQUIRE(dirty.dirty());
    dirty.reset_clean();
    MK_REQUIRE(!dirty.dirty());
    MK_REQUIRE(dirty.revision() == 0);
    MK_REQUIRE(dirty.saved_revision() == 0);
}

MK_TEST("editor project creation wizard advances through valid draft steps") {
    auto wizard = mirakana::editor::ProjectCreationWizard::begin();

    MK_REQUIRE(wizard.step() == mirakana::editor::ProjectCreationStep::identity);
    MK_REQUIRE(!wizard.can_advance());
    MK_REQUIRE(!wizard.advance());

    wizard.set_name("Arcade Sample");
    wizard.set_root_path("games/arcade-sample");
    MK_REQUIRE(wizard.can_advance());
    MK_REQUIRE(wizard.advance());
    MK_REQUIRE(wizard.step() == mirakana::editor::ProjectCreationStep::paths);

    MK_REQUIRE(wizard.advance());
    MK_REQUIRE(wizard.step() == mirakana::editor::ProjectCreationStep::review);

    const auto project = wizard.create_project_document();
    MK_REQUIRE(project.name == "Arcade Sample");
    MK_REQUIRE(project.root_path == "games/arcade-sample");
    MK_REQUIRE(project.asset_root == "assets");
    MK_REQUIRE(project.source_registry_path == "source/assets/package.geassets");
    MK_REQUIRE(project.game_manifest_path == "game.agent.json");
    MK_REQUIRE(project.startup_scene_path == "scenes/start.scene");
}

MK_TEST("editor project creation wizard rejects unsafe source registry path before review") {
    auto wizard = mirakana::editor::ProjectCreationWizard::begin();
    wizard.set_name("Arcade Sample");
    wizard.set_root_path("games/arcade-sample");
    MK_REQUIRE(wizard.advance());
    MK_REQUIRE(wizard.step() == mirakana::editor::ProjectCreationStep::paths);

    wizard.set_source_registry_path("../package.geassets");
    auto errors = wizard.validation_errors();
    MK_REQUIRE(!wizard.can_advance());
    MK_REQUIRE(std::ranges::any_of(errors, [](const mirakana::editor::ProjectCreationError& error) {
        return error.field == "source_registry_path";
    }));

    wizard.set_source_registry_path("C:/outside/package.geassets");
    errors = wizard.validation_errors();
    MK_REQUIRE(!wizard.can_advance());
    MK_REQUIRE(std::ranges::any_of(errors, [](const mirakana::editor::ProjectCreationError& error) {
        return error.field == "source_registry_path";
    }));

    wizard.set_source_registry_path("source/assets/package.txt");
    errors = wizard.validation_errors();
    MK_REQUIRE(!wizard.can_advance());
    MK_REQUIRE(std::ranges::any_of(errors, [](const mirakana::editor::ProjectCreationError& error) {
        return error.field == "source_registry_path";
    }));

    wizard.set_source_registry_path("source/assets/package.geassets");
    MK_REQUIRE(wizard.can_advance());
}

MK_TEST("editor project creation wizard reports invalid fields and supports back navigation") {
    auto wizard = mirakana::editor::ProjectCreationWizard::begin();

    wizard.set_name("Broken=Project");
    wizard.set_root_path("");
    const auto errors = wizard.validation_errors();
    MK_REQUIRE(errors.size() == 2);
    MK_REQUIRE(errors[0].field == "name");
    MK_REQUIRE(errors[1].field == "root_path");
    MK_REQUIRE(!wizard.advance());

    wizard.set_name("Fixed Project");
    wizard.set_root_path("games/fixed");
    MK_REQUIRE(wizard.advance());
    wizard.set_startup_scene_path("");
    MK_REQUIRE(!wizard.can_advance());
    MK_REQUIRE(!wizard.advance());

    MK_REQUIRE(wizard.back());
    MK_REQUIRE(wizard.step() == mirakana::editor::ProjectCreationStep::identity);
    MK_REQUIRE(!wizard.back());
}

MK_TEST("editor history executes undo and redo actions") {
    mirakana::editor::UndoStack history;
    int value = 0;

    const bool executed = history.execute(mirakana::editor::UndoableAction{
        .label = "Increment",
        .redo = [&value]() { ++value; },
        .undo = [&value]() { --value; },
    });

    MK_REQUIRE(executed);
    MK_REQUIRE(value == 1);
    MK_REQUIRE(history.can_undo());
    MK_REQUIRE(!history.can_redo());
    MK_REQUIRE(history.undo_label() == "Increment");

    MK_REQUIRE(history.undo());
    MK_REQUIRE(value == 0);
    MK_REQUIRE(!history.can_undo());
    MK_REQUIRE(history.can_redo());
    MK_REQUIRE(history.redo_label() == "Increment");

    MK_REQUIRE(history.redo());
    MK_REQUIRE(value == 1);
    MK_REQUIRE(history.can_undo());
    MK_REQUIRE(!history.can_redo());
}

MK_TEST("editor history clears redo stack when a new action executes") {
    mirakana::editor::UndoStack history;
    int value = 0;

    MK_REQUIRE(history.execute(
        mirakana::editor::UndoableAction{"Add one", [&value]() { value += 1; }, [&value]() { value -= 1; }}));
    MK_REQUIRE(history.execute(
        mirakana::editor::UndoableAction{"Add ten", [&value]() { value += 10; }, [&value]() { value -= 10; }}));
    MK_REQUIRE(value == 11);
    MK_REQUIRE(history.undo());
    MK_REQUIRE(value == 1);
    MK_REQUIRE(history.can_redo());

    MK_REQUIRE(history.execute(
        mirakana::editor::UndoableAction{"Add two", [&value]() { value += 2; }, [&value]() { value -= 2; }}));
    MK_REQUIRE(value == 3);
    MK_REQUIRE(!history.can_redo());
    MK_REQUIRE(history.undo_label() == "Add two");
}

MK_TEST("editor history rejects incomplete actions") {
    mirakana::editor::UndoStack history;

    MK_REQUIRE(!history.execute(mirakana::editor::UndoableAction{"", []() {}, []() {}}));
    MK_REQUIRE(!history.execute(mirakana::editor::UndoableAction{"Missing do", {}, []() {}}));
    MK_REQUIRE(!history.execute(mirakana::editor::UndoableAction{"Missing undo", []() {}, {}}));
    MK_REQUIRE(!history.can_undo());
    MK_REQUIRE(!history.can_redo());
}

MK_TEST("editor command palette filters commands by label or id") {
    mirakana::editor::CommandRegistry registry;
    int executed = 0;
    MK_REQUIRE(registry.try_add(
        mirakana::editor::Command{"scene.add_empty", "Add Empty Node", [&executed]() { ++executed; }}));
    MK_REQUIRE(
        registry.try_add(mirakana::editor::Command{"file.save_scene", "Save Scene", [&executed]() { ++executed; }}));
    MK_REQUIRE(registry.try_add(
        mirakana::editor::Command{"view.console", "Toggle Console", [&executed]() { ++executed; }, false}));

    const auto scene_matches = mirakana::editor::query_command_palette(registry, "scene");
    MK_REQUIRE(scene_matches.size() == 2);
    MK_REQUIRE(scene_matches[0].id == "scene.add_empty");
    MK_REQUIRE(scene_matches[1].id == "file.save_scene");

    const auto case_insensitive_matches = mirakana::editor::query_command_palette(registry, "EMPTY");
    MK_REQUIRE(case_insensitive_matches.size() == 1);
    MK_REQUIRE(case_insensitive_matches[0].label == "Add Empty Node");
    MK_REQUIRE(case_insensitive_matches[0].enabled);

    const auto disabled_matches = mirakana::editor::query_command_palette(registry, "console");
    MK_REQUIRE(disabled_matches.size() == 1);
    MK_REQUIRE(!disabled_matches[0].enabled);
}

MK_TEST("editor command palette executes enabled commands only") {
    mirakana::editor::CommandRegistry registry;
    int executed = 0;
    MK_REQUIRE(registry.try_add(
        mirakana::editor::Command{"scene.add_empty", "Add Empty Node", [&executed]() { ++executed; }}));
    MK_REQUIRE(registry.try_add(
        mirakana::editor::Command{"view.console", "Toggle Console", [&executed]() { ++executed; }, false}));

    MK_REQUIRE(mirakana::editor::execute_palette_command(registry, "scene.add_empty"));
    MK_REQUIRE(executed == 1);
    MK_REQUIRE(!mirakana::editor::execute_palette_command(registry, "view.console"));
    MK_REQUIRE(executed == 1);
    MK_REQUIRE(!mirakana::editor::execute_palette_command(registry, "missing.command"));
}

MK_TEST("editor ui models build retained inspector assets commands and diagnostics") {
    const auto inspector = mirakana::editor::make_inspector_ui_model({
        mirakana::editor::EditorPropertyRow{
            .id = "transform.position", .label = "Position", .value = "1,2,3", .editable = true},
        mirakana::editor::EditorPropertyRow{.id = "camera.fov", .label = "FOV", .value = "60", .editable = false},
    });

    const auto* inspector_root = inspector.find(mirakana::ui::ElementId{"inspector"});
    const auto* position_value = inspector.find(mirakana::ui::ElementId{"inspector.transform.position.value"});
    MK_REQUIRE(inspector_root != nullptr);
    MK_REQUIRE(inspector_root->role == mirakana::ui::SemanticRole::panel);
    MK_REQUIRE(position_value != nullptr);
    MK_REQUIRE(position_value->text.label == "1,2,3");
    MK_REQUIRE(position_value->enabled);

    const auto assets = mirakana::editor::make_asset_list_ui_model({
        mirakana::editor::EditorAssetListRow{
            .id = "asset.player", .path = "textures/player.getex", .kind = "Texture", .enabled = true},
        mirakana::editor::EditorAssetListRow{
            .id = "asset.music", .path = "audio/theme.geaudio", .kind = "Audio", .enabled = false},
    });
    MK_REQUIRE(assets.find(mirakana::ui::ElementId{"assets.asset.player"})->role ==
               mirakana::ui::SemanticRole::list_item);
    MK_REQUIRE(assets.find(mirakana::ui::ElementId{"assets.asset.player.path"})->text.label == "textures/player.getex");
    MK_REQUIRE(!assets.find(mirakana::ui::ElementId{"assets.asset.music"})->enabled);

    const auto commands = mirakana::editor::make_command_palette_ui_model({
        mirakana::editor::EditorCommandPaletteEntry{
            .id = "scene.add_empty", .label = "Add Empty Node", .enabled = true},
        mirakana::editor::EditorCommandPaletteEntry{.id = "run.pause", .label = "Pause", .enabled = false},
    });
    MK_REQUIRE(commands.find(mirakana::ui::ElementId{"commands.scene.add_empty"})->role ==
               mirakana::ui::SemanticRole::button);
    MK_REQUIRE(!commands.find(mirakana::ui::ElementId{"commands.run.pause"})->enabled);

    const auto diagnostics = mirakana::editor::make_diagnostics_ui_model({
        mirakana::editor::EditorDiagnosticRow{.id = "shader.dxc",
                                              .severity = mirakana::editor::EditorDiagnosticSeverity::warning,
                                              .message = "DXC missing"},
    });
    MK_REQUIRE(diagnostics.find(mirakana::ui::ElementId{"diagnostics.shader.dxc.message"})->text.label ==
               "DXC missing");
    MK_REQUIRE(mirakana::editor::editor_diagnostic_severity_label(
                   mirakana::editor::EditorDiagnosticSeverity::warning) == "Warning");

    const auto serialized = mirakana::editor::serialize_editor_ui_model(inspector);
    MK_REQUIRE(serialized.contains("format=GameEngine.EditorUiModel.v1"));
    MK_REQUIRE(serialized.contains("element.2.id=inspector.transform.position.label"));
    MK_REQUIRE(serialized.contains("element.3.label=1,2,3"));
}

MK_TEST("editor profiler panel model summarizes captures deterministically") {
    const mirakana::DiagnosticCapture capture{
        .events =
            {
                mirakana::DiagnosticEvent{.severity = mirakana::DiagnosticSeverity::warning,
                                          .category = "runtime",
                                          .message = "slow frame",
                                          .frame_index = 2},
                mirakana::DiagnosticEvent{.severity = mirakana::DiagnosticSeverity::info,
                                          .category = "editor",
                                          .message = "ready",
                                          .frame_index = 1},
            },
        .counters =
            {
                mirakana::CounterSample{.name = "renderer.meshes_submitted", .value = 4.0, .frame_index = 2},
                mirakana::CounterSample{.name = "renderer.frames_started", .value = 2.0, .frame_index = 1},
            },
        .profiles =
            {
                mirakana::ProfileSample{.name = "editor.assets",
                                        .frame_index = 2,
                                        .start_time_ns = 2'000'000,
                                        .duration_ns = 500'000,
                                        .depth = 1},
                mirakana::ProfileSample{.name = "editor.frame",
                                        .frame_index = 1,
                                        .start_time_ns = 1'000'000,
                                        .duration_ns = 250'000,
                                        .depth = 0},
            },
    };
    const mirakana::editor::EditorProfilerStatus status{
        .log_records = 3,
        .undo_stack = 2,
        .redo_stack = 1,
        .asset_imports = 5,
        .hot_reload_events = 7,
        .shader_compiles = 11,
        .dirty = true,
        .revision = 42,
    };

    const auto model = mirakana::editor::make_editor_profiler_panel_model(capture, status);

    MK_REQUIRE(!model.capture_empty);
    MK_REQUIRE(model.status_rows.size() == 8);
    MK_REQUIRE(model.status_rows[0].label == "Log records");
    MK_REQUIRE(model.status_rows[0].value == "3");
    MK_REQUIRE(model.summary_rows.size() >= 8);
    MK_REQUIRE(model.summary_rows[0].label == "Events");
    MK_REQUIRE(model.summary_rows[0].value == "2");
    MK_REQUIRE(model.summary_rows[1].label == "Warnings");
    MK_REQUIRE(model.summary_rows[1].value == "1");
    MK_REQUIRE(model.profile_rows.size() == 2);
    MK_REQUIRE(model.profile_rows[0].name == "editor.frame");
    MK_REQUIRE(model.profile_rows[0].frame_index == 1);
    MK_REQUIRE(model.profile_rows[0].duration == "0.250 ms");
    MK_REQUIRE(model.profile_rows[1].name == "editor.assets");
    MK_REQUIRE(model.counter_rows.size() == 2);
    MK_REQUIRE(model.counter_rows[0].name == "renderer.frames_started");
    MK_REQUIRE(model.counter_rows[0].value == "2");
    MK_REQUIRE(model.event_rows.size() == 2);
    MK_REQUIRE(model.event_rows[0].severity == "info");
    MK_REQUIRE(model.event_rows[0].category == "editor");
    MK_REQUIRE(model.event_rows[1].severity == "warning");
    MK_REQUIRE(model.trace_status == "Trace export available");

    const auto document = mirakana::editor::make_profiler_ui_model(model);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.summary.events.value"})->text.label == "2");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.profiles.1.duration"})->text.label == "0.250 ms");
    MK_REQUIRE(model.trace_export.can_export);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_export.payload_bytes.value"}) != nullptr);
    MK_REQUIRE(model.trace_file_save.output_path == "diagnostics/editor-profiler-trace.json");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_file_save.output_path.value"})->text.label ==
               "diagnostics/editor-profiler-trace.json");
    MK_REQUIRE(model.telemetry.status_label == "unsupported");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.telemetry.status.value"})->text.label == "unsupported");
    MK_REQUIRE(model.trace_import.status_label == "Trace import empty");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_import.status.value"})->text.label ==
               "Trace import empty");
}

MK_TEST("editor profiler trace export model exposes deterministic trace json") {
    const mirakana::DiagnosticCapture capture{
        .events =
            {
                mirakana::DiagnosticEvent{.severity = mirakana::DiagnosticSeverity::warning,
                                          .category = "runtime",
                                          .message = "slow frame",
                                          .frame_index = 2},
            },
        .counters =
            {
                mirakana::CounterSample{.name = "renderer.frames_started", .value = 1.0, .frame_index = 3},
            },
        .profiles =
            {
                mirakana::ProfileSample{
                    .name = "editor.frame", .frame_index = 4, .start_time_ns = 1'000, .duration_ns = 2'000, .depth = 0},
            },
    };

    const auto model = mirakana::editor::make_editor_profiler_trace_export_model(capture);

    auto find_row = [&model](std::string_view id) -> const mirakana::editor::EditorProfilerKeyValueRow* {
        const auto it = std::ranges::find_if(
            model.rows, [id](const mirakana::editor::EditorProfilerKeyValueRow& row) { return row.id == id; });
        return it == model.rows.end() ? nullptr : std::addressof(*it);
    };

    MK_REQUIRE(model.can_export);
    MK_REQUIRE(model.status_label == "Trace export ready");
    MK_REQUIRE(model.format == "Chrome Trace Event JSON");
    MK_REQUIRE(model.producer == "mirakana::export_diagnostics_trace_json");
    MK_REQUIRE(model.payload.contains("traceEvents"));
    MK_REQUIRE(model.payload.contains("slow frame"));
    MK_REQUIRE(model.payload.contains("renderer.frames_started"));
    MK_REQUIRE(model.payload.contains("editor.frame"));
    MK_REQUIRE(model.payload_bytes == model.payload.size());
    MK_REQUIRE(find_row("status") != nullptr);
    MK_REQUIRE(find_row("format") != nullptr);
    MK_REQUIRE(find_row("producer") != nullptr);
    MK_REQUIRE(find_row("payload_bytes") != nullptr);
    MK_REQUIRE(find_row("events") != nullptr);
    MK_REQUIRE(find_row("counters") != nullptr);
    MK_REQUIRE(find_row("profiles") != nullptr);
}

MK_TEST("editor profiler trace file save writes project relative json") {
    const mirakana::DiagnosticCapture capture{
        .events =
            {
                mirakana::DiagnosticEvent{.severity = mirakana::DiagnosticSeverity::warning,
                                          .category = "runtime",
                                          .message = "slow frame",
                                          .frame_index = 2},
            },
        .counters =
            {
                mirakana::CounterSample{.name = "renderer.frames_started", .value = 1.0, .frame_index = 3},
            },
        .profiles =
            {
                mirakana::ProfileSample{
                    .name = "editor.frame", .frame_index = 4, .start_time_ns = 1'000, .duration_ns = 2'000, .depth = 0},
            },
    };

    mirakana::editor::MemoryTextStore store;
    mirakana::editor::EditorProfilerTraceFileSaveRequest request;
    request.output_path = "diagnostics/editor-trace.json";

    const auto result = mirakana::editor::save_editor_profiler_trace_json(store, capture, request);

    MK_REQUIRE(result.saved);
    MK_REQUIRE(result.status_label == "Trace file saved");
    MK_REQUIRE(result.output_path == "diagnostics/editor-trace.json");
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(store.exists("diagnostics/editor-trace.json"));
    const auto saved = store.read_text("diagnostics/editor-trace.json");
    MK_REQUIRE(result.payload_bytes == saved.size());
    MK_REQUIRE(saved.contains("traceEvents"));
    MK_REQUIRE(saved.contains("slow frame"));
    MK_REQUIRE(saved.contains("renderer.frames_started"));
    MK_REQUIRE(saved.contains("editor.frame"));

    mirakana::editor::MemoryTextStore empty_store;
    const auto empty_result =
        mirakana::editor::save_editor_profiler_trace_json(empty_store, mirakana::DiagnosticCapture{}, request);
    MK_REQUIRE(!empty_result.saved);
    MK_REQUIRE(empty_result.diagnostics.size() == 1U);
    MK_REQUIRE(empty_result.diagnostics[0] == "trace file save requires at least one diagnostic sample");
    MK_REQUIRE(!empty_store.exists("diagnostics/editor-trace.json"));

    const std::vector<std::string_view> invalid_paths{
        "../trace.json",
        "/tmp/trace.json",
        "diagnostics/trace.txt",
        "diagnostics/bad=name.json",
    };
    for (const auto invalid_path : invalid_paths) {
        mirakana::editor::MemoryTextStore rejected_store;
        mirakana::editor::EditorProfilerTraceFileSaveRequest invalid_request;
        invalid_request.output_path = std::string(invalid_path);

        const auto invalid_result =
            mirakana::editor::save_editor_profiler_trace_json(rejected_store, capture, invalid_request);

        MK_REQUIRE(!invalid_result.saved);
        MK_REQUIRE(invalid_result.payload_bytes == 0U);
        MK_REQUIRE(!invalid_result.diagnostics.empty());
        MK_REQUIRE(!rejected_store.exists("diagnostics/editor-profiler-trace.json"));
        if (invalid_path == "diagnostics/trace.txt") {
            MK_REQUIRE(!rejected_store.exists("diagnostics/trace.txt"));
        }
    }
}

MK_TEST("editor profiler telemetry handoff reports backend readiness") {
    const mirakana::DiagnosticCapture capture{
        .events =
            {
                mirakana::DiagnosticEvent{.severity = mirakana::DiagnosticSeverity::warning,
                                          .category = "runtime",
                                          .message = "slow frame",
                                          .frame_index = 2},
            },
        .counters =
            {
                mirakana::CounterSample{.name = "renderer.frames_started", .value = 1.0, .frame_index = 3},
            },
        .profiles =
            {
                mirakana::ProfileSample{
                    .name = "editor.frame", .frame_index = 4, .start_time_ns = 1'000, .duration_ns = 2'000, .depth = 0},
            },
    };

    const auto model = mirakana::editor::make_editor_profiler_telemetry_handoff_model(capture);

    auto find_row = [](const mirakana::editor::EditorProfilerTelemetryHandoffModel& handoff,
                       std::string_view id) -> const mirakana::editor::EditorProfilerKeyValueRow* {
        const auto it = std::ranges::find_if(
            handoff.rows, [id](const mirakana::editor::EditorProfilerKeyValueRow& row) { return row.id == id; });
        return it == handoff.rows.end() ? nullptr : std::addressof(*it);
    };

    MK_REQUIRE(!model.ready);
    MK_REQUIRE(model.status_label == "unsupported");
    MK_REQUIRE(model.kind == "telemetry_upload");
    MK_REQUIRE(model.format == "caller-defined telemetry payload");
    MK_REQUIRE(model.blocker.contains("No telemetry backend"));
    MK_REQUIRE(model.diagnostics.size() == 1U);
    MK_REQUIRE(model.diagnostics[0].contains("No telemetry backend"));
    MK_REQUIRE(find_row(model, "kind") != nullptr);
    MK_REQUIRE(find_row(model, "status") != nullptr);
    MK_REQUIRE(find_row(model, "format") != nullptr);
    MK_REQUIRE(find_row(model, "blocker") != nullptr);
    MK_REQUIRE(find_row(model, "events") != nullptr);
    MK_REQUIRE(find_row(model, "counters") != nullptr);
    MK_REQUIRE(find_row(model, "profiles") != nullptr);

    mirakana::DiagnosticsOpsPlanOptions options;
    options.host_status.telemetry_backend_configured = true;
    const auto ready_model = mirakana::editor::make_editor_profiler_telemetry_handoff_model(capture, options);

    MK_REQUIRE(ready_model.ready);
    MK_REQUIRE(ready_model.status_label == "ready");
    MK_REQUIRE(ready_model.producer == "caller-provided telemetry backend");
    MK_REQUIRE(ready_model.diagnostics.empty());
    MK_REQUIRE(find_row(ready_model, "producer") != nullptr);
}

MK_TEST("editor profiler trace import review model reports pasted trace json") {
    mirakana::DiagnosticCapture capture;
    capture.events.push_back(mirakana::DiagnosticEvent{.severity = mirakana::DiagnosticSeverity::warning,
                                                       .category = "runtime",
                                                       .message = "slow frame",
                                                       .frame_index = 2});
    capture.counters.push_back(
        mirakana::CounterSample{.name = "renderer.frames_started", .value = 1.0, .frame_index = 3});
    capture.profiles.push_back(mirakana::ProfileSample{
        .name = "editor.frame", .frame_index = 4, .start_time_ns = 1'000, .duration_ns = 2'000, .depth = 0});

    const auto payload = mirakana::export_diagnostics_trace_json(capture);
    const auto model = mirakana::editor::make_editor_profiler_trace_import_review_model(payload);

    auto find_row = [&model](std::string_view id) -> const mirakana::editor::EditorProfilerKeyValueRow* {
        const auto it = std::ranges::find_if(
            model.rows, [id](const mirakana::editor::EditorProfilerKeyValueRow& row) { return row.id == id; });
        return it == model.rows.end() ? nullptr : std::addressof(*it);
    };

    MK_REQUIRE(model.valid);
    MK_REQUIRE(model.status_label == "Trace import ready");
    MK_REQUIRE(model.payload_bytes == payload.size());
    MK_REQUIRE(model.diagnostics.empty());
    MK_REQUIRE(find_row("status") != nullptr);
    MK_REQUIRE(find_row("format") != nullptr);
    MK_REQUIRE(find_row("payload_bytes") != nullptr);
    MK_REQUIRE(find_row("events")->value == "5");
    MK_REQUIRE(find_row("metadata")->value == "2");
    MK_REQUIRE(find_row("instant_events")->value == "1");
    MK_REQUIRE(find_row("counters")->value == "1");
    MK_REQUIRE(find_row("profiles")->value == "1");
    MK_REQUIRE(find_row("unknown_events")->value == "0");

    auto panel = mirakana::editor::make_editor_profiler_panel_model(capture, mirakana::editor::EditorProfilerStatus{});
    panel.trace_import = model;
    const auto document = mirakana::editor::make_profiler_ui_model(panel);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_import.events.value"})->text.label == "5");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_import.metadata.value"})->text.label == "2");

    const auto empty = mirakana::editor::make_editor_profiler_trace_import_review_model("");
    MK_REQUIRE(!empty.valid);
    MK_REQUIRE(empty.status_label == "Trace import empty");
    MK_REQUIRE(empty.diagnostics.size() == 1U);
    MK_REQUIRE(empty.diagnostics[0] == "trace import review requires non-empty JSON");

    const auto malformed = mirakana::editor::make_editor_profiler_trace_import_review_model(R"({"traceEvents":[)");
    MK_REQUIRE(!malformed.valid);
    MK_REQUIRE(malformed.status_label == "Trace import blocked");
    MK_REQUIRE(!malformed.diagnostics.empty());
    MK_REQUIRE(malformed.diagnostics[0].contains("malformed trace JSON"));
}

MK_TEST("editor profiler trace file import review reads project relative json") {
    mirakana::DiagnosticCapture capture;
    capture.events.push_back(mirakana::DiagnosticEvent{.severity = mirakana::DiagnosticSeverity::warning,
                                                       .category = "runtime",
                                                       .message = "slow frame",
                                                       .frame_index = 2});
    capture.counters.push_back(
        mirakana::CounterSample{.name = "renderer.frames_started", .value = 1.0, .frame_index = 3});
    capture.profiles.push_back(mirakana::ProfileSample{
        .name = "editor.frame", .frame_index = 4, .start_time_ns = 1'000, .duration_ns = 2'000, .depth = 0});

    const auto payload = mirakana::export_diagnostics_trace_json(capture);
    mirakana::editor::MemoryTextStore store;
    store.write_text("diagnostics/editor-trace.json", payload);

    mirakana::editor::EditorProfilerTraceFileImportRequest request;
    request.input_path = "diagnostics/editor-trace.json";

    const auto result = mirakana::editor::import_editor_profiler_trace_json(store, request);

    auto find_row = [&result](std::string_view id) -> const mirakana::editor::EditorProfilerKeyValueRow* {
        const auto it = std::ranges::find_if(
            result.rows, [id](const mirakana::editor::EditorProfilerKeyValueRow& row) { return row.id == id; });
        return it == result.rows.end() ? nullptr : std::addressof(*it);
    };

    MK_REQUIRE(result.imported);
    MK_REQUIRE(result.status_label == "Trace file import ready");
    MK_REQUIRE(result.input_path == "diagnostics/editor-trace.json");
    MK_REQUIRE(result.payload_bytes == payload.size());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.review.valid);
    MK_REQUIRE(find_row("status")->value == "Trace file import ready");
    MK_REQUIRE(find_row("input_path")->value == "diagnostics/editor-trace.json");
    MK_REQUIRE(find_row("payload_bytes")->value == std::to_string(payload.size()));
    MK_REQUIRE(find_row("events")->value == "5");
    MK_REQUIRE(find_row("metadata")->value == "2");
    MK_REQUIRE(find_row("instant_events")->value == "1");
    MK_REQUIRE(find_row("counters")->value == "1");
    MK_REQUIRE(find_row("profiles")->value == "1");
    MK_REQUIRE(find_row("unknown_events")->value == "0");

    auto panel = mirakana::editor::make_editor_profiler_panel_model(capture, mirakana::editor::EditorProfilerStatus{});
    panel.trace_file_import = result;
    const auto document = mirakana::editor::make_profiler_ui_model(panel);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_file_import.events.value"})->text.label == "5");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_file_import.input_path.value"})->text.label ==
               "diagnostics/editor-trace.json");

    mirakana::editor::EditorProfilerTraceFileImportRequest empty_request;
    empty_request.input_path = "diagnostics/empty-trace.json";
    store.write_text(empty_request.input_path, "");
    const auto empty = mirakana::editor::import_editor_profiler_trace_json(store, empty_request);
    MK_REQUIRE(!empty.imported);
    MK_REQUIRE(empty.status_label == "Trace file import empty");
    MK_REQUIRE(empty.diagnostics.size() == 1U);
    MK_REQUIRE(empty.diagnostics[0] == "trace import review requires non-empty JSON");

    mirakana::editor::EditorProfilerTraceFileImportRequest malformed_request;
    malformed_request.input_path = "diagnostics/malformed-trace.json";
    store.write_text(malformed_request.input_path, R"({"traceEvents":[)");
    const auto malformed = mirakana::editor::import_editor_profiler_trace_json(store, malformed_request);
    MK_REQUIRE(!malformed.imported);
    MK_REQUIRE(malformed.status_label == "Trace file import blocked");
    MK_REQUIRE(!malformed.diagnostics.empty());
    MK_REQUIRE(malformed.diagnostics[0].contains("malformed trace JSON"));

    mirakana::editor::EditorProfilerTraceFileImportRequest missing_request;
    missing_request.input_path = "diagnostics/missing-trace.json";
    const auto missing = mirakana::editor::import_editor_profiler_trace_json(store, missing_request);
    MK_REQUIRE(!missing.imported);
    MK_REQUIRE(missing.status_label == "Trace file import failed");
    MK_REQUIRE(!missing.diagnostics.empty());
    MK_REQUIRE(missing.diagnostics[0].contains("trace file import failed"));

    const std::vector<std::string_view> invalid_paths{
        "../trace.json",
        "/tmp/trace.json",
        "diagnostics/trace.txt",
        "diagnostics/bad=name.json",
    };
    for (const auto invalid_path : invalid_paths) {
        mirakana::editor::EditorProfilerTraceFileImportRequest invalid_request;
        invalid_request.input_path = std::string(invalid_path);

        const auto invalid = mirakana::editor::import_editor_profiler_trace_json(store, invalid_request);

        MK_REQUIRE(!invalid.imported);
        MK_REQUIRE(invalid.status_label == "Trace file import blocked");
        MK_REQUIRE(invalid.payload_bytes == 0U);
        MK_REQUIRE(!invalid.diagnostics.empty());
        MK_REQUIRE(invalid.diagnostics[0].contains("trace file import path"));
    }
}

MK_TEST("editor profiler trace import reconstructs capture rows") {
    mirakana::DiagnosticCapture capture;
    capture.events.push_back(mirakana::DiagnosticEvent{.severity = mirakana::DiagnosticSeverity::error,
                                                       .category = "runtime",
                                                       .message = "asset failed",
                                                       .frame_index = 7});
    capture.counters.push_back(
        mirakana::CounterSample{.name = "renderer.frames_started", .value = 8.5, .frame_index = 8});
    capture.profiles.push_back(mirakana::ProfileSample{
        .name = "editor.frame", .frame_index = 9, .start_time_ns = 3'000, .duration_ns = 4'000, .depth = 1});

    const auto payload = mirakana::export_diagnostics_trace_json(capture);
    const auto model = mirakana::editor::make_editor_profiler_trace_import_review_model(payload);

    auto find_model_row = [&model](std::string_view id) -> const mirakana::editor::EditorProfilerKeyValueRow* {
        const auto it = std::ranges::find_if(
            model.rows, [id](const mirakana::editor::EditorProfilerKeyValueRow& row) { return row.id == id; });
        return it == model.rows.end() ? nullptr : std::addressof(*it);
    };

    MK_REQUIRE(model.valid);
    MK_REQUIRE(model.capture_reconstructed);
    MK_REQUIRE(model.capture.events.size() == 1U);
    MK_REQUIRE(model.capture.events[0].message == "asset failed");
    MK_REQUIRE(model.capture.counters.size() == 1U);
    MK_REQUIRE(model.capture.counters[0].value == 8.5);
    MK_REQUIRE(model.capture.profiles.size() == 1U);
    MK_REQUIRE(model.capture.profiles[0].duration_ns == 4'000);
    MK_REQUIRE(model.reconstructed_event_rows.size() == 1U);
    MK_REQUIRE(model.reconstructed_event_rows[0].severity == "error");
    MK_REQUIRE(model.reconstructed_counter_rows.size() == 1U);
    MK_REQUIRE(model.reconstructed_counter_rows[0].value == "8.5");
    MK_REQUIRE(model.reconstructed_profile_rows.size() == 1U);
    MK_REQUIRE(model.reconstructed_profile_rows[0].duration == "0.004 ms");
    MK_REQUIRE(find_model_row("reconstructed_event_count")->value == "1");
    MK_REQUIRE(find_model_row("reconstructed_counter_count")->value == "1");
    MK_REQUIRE(find_model_row("reconstructed_profile_count")->value == "1");

    mirakana::editor::MemoryTextStore store;
    store.write_text("diagnostics/reconstruct-trace.json", payload);
    mirakana::editor::EditorProfilerTraceFileImportRequest request;
    request.input_path = "diagnostics/reconstruct-trace.json";

    const auto result = mirakana::editor::import_editor_profiler_trace_json(store, request);

    auto find_file_row = [&result](std::string_view id) -> const mirakana::editor::EditorProfilerKeyValueRow* {
        const auto it = std::ranges::find_if(
            result.rows, [id](const mirakana::editor::EditorProfilerKeyValueRow& row) { return row.id == id; });
        return it == result.rows.end() ? nullptr : std::addressof(*it);
    };

    MK_REQUIRE(result.imported);
    MK_REQUIRE(result.capture_reconstructed);
    MK_REQUIRE(result.capture.events.size() == 1U);
    MK_REQUIRE(result.capture.counters.size() == 1U);
    MK_REQUIRE(result.capture.profiles.size() == 1U);
    MK_REQUIRE(find_file_row("reconstructed_event_count")->value == "1");
    MK_REQUIRE(find_file_row("reconstructed_counter_count")->value == "1");
    MK_REQUIRE(find_file_row("reconstructed_profile_count")->value == "1");

    auto panel = mirakana::editor::make_editor_profiler_panel_model(capture, mirakana::editor::EditorProfilerStatus{});
    panel.trace_import = model;
    panel.trace_file_import = result;
    const auto document = mirakana::editor::make_profiler_ui_model(panel);
    MK_REQUIRE(
        document.find(mirakana::ui::ElementId{"profiler.trace_import.reconstructed_event_count.value"})->text.label ==
        "1");
    MK_REQUIRE(
        document.find(mirakana::ui::ElementId{"profiler.trace_import.reconstructed_profiles.1.duration"})->text.label ==
        "0.004 ms");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_file_import.reconstructed_profile_count.value"})
                   ->text.label == "1");
}

MK_TEST("editor profiler native trace open dialog reviews selection") {
    const auto request = mirakana::editor::make_editor_profiler_trace_open_dialog_request();
    MK_REQUIRE(request.kind == mirakana::FileDialogKind::open_file);
    MK_REQUIRE(request.title == "Open Trace JSON");
    MK_REQUIRE(request.default_location == "diagnostics");
    MK_REQUIRE(!request.allow_many);
    MK_REQUIRE(request.filters.size() == 1U);
    MK_REQUIRE(request.filters[0].name == "Trace JSON");
    MK_REQUIRE(request.filters[0].pattern == "json");

    const auto accepted = mirakana::editor::make_editor_profiler_trace_open_dialog_model(mirakana::FileDialogResult{
        .id = 7,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"diagnostics/editor-trace.json"},
        .selected_filter = 0,
        .error = {},
    });

    auto find_row = [](const mirakana::editor::EditorProfilerTraceOpenDialogModel& model,
                       std::string_view id) -> const mirakana::editor::EditorProfilerKeyValueRow* {
        const auto it = std::ranges::find_if(
            model.rows, [id](const mirakana::editor::EditorProfilerKeyValueRow& row) { return row.id == id; });
        return it == model.rows.end() ? nullptr : std::addressof(*it);
    };

    MK_REQUIRE(accepted.accepted);
    MK_REQUIRE(accepted.status_label == "Trace open dialog accepted");
    MK_REQUIRE(accepted.selected_path == "diagnostics/editor-trace.json");
    MK_REQUIRE(accepted.diagnostics.empty());
    MK_REQUIRE(find_row(accepted, "status")->value == "Trace open dialog accepted");
    MK_REQUIRE(find_row(accepted, "selected_path")->value == "diagnostics/editor-trace.json");
    MK_REQUIRE(find_row(accepted, "selected_count")->value == "1");
    MK_REQUIRE(find_row(accepted, "selected_filter")->value == "0");

    auto panel = mirakana::editor::make_editor_profiler_panel_model(mirakana::DiagnosticCapture{},
                                                                    mirakana::editor::EditorProfilerStatus{});
    panel.trace_open_dialog = accepted;
    const auto document = mirakana::editor::make_profiler_ui_model(panel);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_open_dialog.status.value"})->text.label ==
               "Trace open dialog accepted");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_open_dialog.selected_path.value"})->text.label ==
               "diagnostics/editor-trace.json");

    const auto canceled = mirakana::editor::make_editor_profiler_trace_open_dialog_model(mirakana::FileDialogResult{
        .id = 8,
        .status = mirakana::FileDialogStatus::canceled,
        .paths = {},
        .selected_filter = -1,
        .error = {},
    });
    MK_REQUIRE(!canceled.accepted);
    MK_REQUIRE(canceled.status_label == "Trace open dialog canceled");
    MK_REQUIRE(canceled.diagnostics.empty());

    const auto failed = mirakana::editor::make_editor_profiler_trace_open_dialog_model(mirakana::FileDialogResult{
        .id = 9,
        .status = mirakana::FileDialogStatus::failed,
        .paths = {},
        .selected_filter = -1,
        .error = "native file dialog failed",
    });
    MK_REQUIRE(!failed.accepted);
    MK_REQUIRE(failed.status_label == "Trace open dialog failed");
    MK_REQUIRE(failed.diagnostics.size() == 1U);
    MK_REQUIRE(failed.diagnostics[0] == "native file dialog failed");

    const auto empty_accepted =
        mirakana::editor::make_editor_profiler_trace_open_dialog_model(mirakana::FileDialogResult{
            .id = 10,
            .status = mirakana::FileDialogStatus::accepted,
            .paths = {},
            .selected_filter = 0,
            .error = {},
        });
    MK_REQUIRE(!empty_accepted.accepted);
    MK_REQUIRE(empty_accepted.status_label == "Trace open dialog blocked");
    MK_REQUIRE(!empty_accepted.diagnostics.empty());
    MK_REQUIRE(empty_accepted.diagnostics[0].contains("accepted file dialog results"));

    const auto multi = mirakana::editor::make_editor_profiler_trace_open_dialog_model(mirakana::FileDialogResult{
        .id = 11,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"diagnostics/a.json", "diagnostics/b.json"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!multi.accepted);
    MK_REQUIRE(multi.status_label == "Trace open dialog blocked");
    MK_REQUIRE(!multi.diagnostics.empty());
    MK_REQUIRE(multi.diagnostics[0] == "trace open dialog requires exactly one selected path");

    const auto wrong_extension =
        mirakana::editor::make_editor_profiler_trace_open_dialog_model(mirakana::FileDialogResult{
            .id = 12,
            .status = mirakana::FileDialogStatus::accepted,
            .paths = {"diagnostics/editor-trace.txt"},
            .selected_filter = 0,
            .error = {},
        });
    MK_REQUIRE(!wrong_extension.accepted);
    MK_REQUIRE(wrong_extension.status_label == "Trace open dialog blocked");
    MK_REQUIRE(!wrong_extension.diagnostics.empty());
    MK_REQUIRE(wrong_extension.diagnostics[0] == "trace open dialog selection must end with .json");
}

MK_TEST("editor profiler native trace save dialog reviews selection") {
    const auto request = mirakana::editor::make_editor_profiler_trace_save_dialog_request();
    MK_REQUIRE(request.kind == mirakana::FileDialogKind::save_file);
    MK_REQUIRE(request.title == "Save Trace JSON");
    MK_REQUIRE(request.default_location == "diagnostics/editor-profiler-trace.json");
    MK_REQUIRE(!request.allow_many);
    MK_REQUIRE(request.accept_label == "Save");
    MK_REQUIRE(request.cancel_label == "Cancel");
    MK_REQUIRE(request.filters.size() == 1U);
    MK_REQUIRE(request.filters[0].name == "Trace JSON");
    MK_REQUIRE(request.filters[0].pattern == "json");

    const auto accepted = mirakana::editor::make_editor_profiler_trace_save_dialog_model(mirakana::FileDialogResult{
        .id = 13,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"diagnostics/editor-trace.json"},
        .selected_filter = 0,
        .error = {},
    });

    auto find_row = [](const mirakana::editor::EditorProfilerTraceSaveDialogModel& model,
                       std::string_view id) -> const mirakana::editor::EditorProfilerKeyValueRow* {
        const auto it = std::ranges::find_if(
            model.rows, [id](const mirakana::editor::EditorProfilerKeyValueRow& row) { return row.id == id; });
        return it == model.rows.end() ? nullptr : std::addressof(*it);
    };

    MK_REQUIRE(accepted.accepted);
    MK_REQUIRE(accepted.status_label == "Trace save dialog accepted");
    MK_REQUIRE(accepted.selected_path == "diagnostics/editor-trace.json");
    MK_REQUIRE(accepted.diagnostics.empty());
    MK_REQUIRE(find_row(accepted, "status")->value == "Trace save dialog accepted");
    MK_REQUIRE(find_row(accepted, "selected_path")->value == "diagnostics/editor-trace.json");
    MK_REQUIRE(find_row(accepted, "selected_count")->value == "1");
    MK_REQUIRE(find_row(accepted, "selected_filter")->value == "0");

    auto panel = mirakana::editor::make_editor_profiler_panel_model(mirakana::DiagnosticCapture{},
                                                                    mirakana::editor::EditorProfilerStatus{});
    panel.trace_save_dialog = accepted;
    const auto document = mirakana::editor::make_profiler_ui_model(panel);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_save_dialog.status.value"})->text.label ==
               "Trace save dialog accepted");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_save_dialog.selected_path.value"})->text.label ==
               "diagnostics/editor-trace.json");

    const auto canceled = mirakana::editor::make_editor_profiler_trace_save_dialog_model(mirakana::FileDialogResult{
        .id = 14,
        .status = mirakana::FileDialogStatus::canceled,
        .paths = {},
        .selected_filter = -1,
        .error = {},
    });
    MK_REQUIRE(!canceled.accepted);
    MK_REQUIRE(canceled.status_label == "Trace save dialog canceled");
    MK_REQUIRE(canceled.diagnostics.empty());

    const auto failed = mirakana::editor::make_editor_profiler_trace_save_dialog_model(mirakana::FileDialogResult{
        .id = 15,
        .status = mirakana::FileDialogStatus::failed,
        .paths = {},
        .selected_filter = -1,
        .error = "native file dialog failed",
    });
    MK_REQUIRE(!failed.accepted);
    MK_REQUIRE(failed.status_label == "Trace save dialog failed");
    MK_REQUIRE(failed.diagnostics.size() == 1U);
    MK_REQUIRE(failed.diagnostics[0] == "native file dialog failed");

    const auto empty_accepted =
        mirakana::editor::make_editor_profiler_trace_save_dialog_model(mirakana::FileDialogResult{
            .id = 16,
            .status = mirakana::FileDialogStatus::accepted,
            .paths = {},
            .selected_filter = 0,
            .error = {},
        });
    MK_REQUIRE(!empty_accepted.accepted);
    MK_REQUIRE(empty_accepted.status_label == "Trace save dialog blocked");
    MK_REQUIRE(!empty_accepted.diagnostics.empty());
    MK_REQUIRE(empty_accepted.diagnostics[0].contains("accepted file dialog results"));

    const auto multi = mirakana::editor::make_editor_profiler_trace_save_dialog_model(mirakana::FileDialogResult{
        .id = 17,
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"diagnostics/a.json", "diagnostics/b.json"},
        .selected_filter = 0,
        .error = {},
    });
    MK_REQUIRE(!multi.accepted);
    MK_REQUIRE(multi.status_label == "Trace save dialog blocked");
    MK_REQUIRE(!multi.diagnostics.empty());
    MK_REQUIRE(multi.diagnostics[0] == "trace save dialog requires exactly one selected path");

    const auto wrong_extension =
        mirakana::editor::make_editor_profiler_trace_save_dialog_model(mirakana::FileDialogResult{
            .id = 18,
            .status = mirakana::FileDialogStatus::accepted,
            .paths = {"diagnostics/editor-trace.txt"},
            .selected_filter = 0,
            .error = {},
        });
    MK_REQUIRE(!wrong_extension.accepted);
    MK_REQUIRE(wrong_extension.status_label == "Trace save dialog blocked");
    MK_REQUIRE(!wrong_extension.diagnostics.empty());
    MK_REQUIRE(wrong_extension.diagnostics[0] == "trace save dialog selection must end with .json");
}

MK_TEST("editor profiler panel model reports empty capture state") {
    const auto model = mirakana::editor::make_editor_profiler_panel_model(mirakana::DiagnosticCapture{},
                                                                          mirakana::editor::EditorProfilerStatus{
                                                                              .log_records = 0,
                                                                              .undo_stack = 0,
                                                                              .redo_stack = 0,
                                                                              .asset_imports = 0,
                                                                              .hot_reload_events = 0,
                                                                              .shader_compiles = 0,
                                                                              .dirty = false,
                                                                              .revision = 0,
                                                                          });

    MK_REQUIRE(model.capture_empty);
    MK_REQUIRE(model.profile_rows.empty());
    MK_REQUIRE(model.counter_rows.empty());
    MK_REQUIRE(model.event_rows.empty());
    MK_REQUIRE(model.summary_rows[0].value == "0");
    MK_REQUIRE(!model.trace_export.can_export);
    MK_REQUIRE(model.trace_export.diagnostics.size() == 1U);
    MK_REQUIRE(model.trace_export.diagnostics[0] == "trace export requires at least one diagnostic sample");
    const auto document = mirakana::editor::make_profiler_ui_model(model);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.empty"})->text.label ==
               "No diagnostic samples recorded");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"profiler.trace_export.diagnostics.0"})->text.label ==
               "trace export requires at least one diagnostic sample");
}

MK_TEST("editor profiler panel model uses deterministic tie breakers") {
    const mirakana::DiagnosticCapture capture{
        .events = {},
        .counters =
            {
                mirakana::CounterSample{.name = "renderer.same", .value = 2.0, .frame_index = 4},
                mirakana::CounterSample{.name = "renderer.same", .value = 1.0, .frame_index = 4},
                mirakana::CounterSample{.name = "renderer.large", .value = 1.0e300, .frame_index = 5},
            },
        .profiles =
            {
                mirakana::ProfileSample{
                    .name = "editor.same", .frame_index = 3, .start_time_ns = 10, .duration_ns = 200'000, .depth = 0},
                mirakana::ProfileSample{
                    .name = "editor.same", .frame_index = 3, .start_time_ns = 10, .duration_ns = 100'000, .depth = 0},
            },
    };

    const auto model =
        mirakana::editor::make_editor_profiler_panel_model(capture, mirakana::editor::EditorProfilerStatus{});

    MK_REQUIRE(model.profile_rows.size() == 2);
    MK_REQUIRE(model.profile_rows[0].duration == "0.100 ms");
    MK_REQUIRE(model.profile_rows[1].duration == "0.200 ms");
    MK_REQUIRE(model.counter_rows.size() == 3);
    MK_REQUIRE(model.counter_rows[0].name == "renderer.same");
    MK_REQUIRE(model.counter_rows[0].value == "1");
    MK_REQUIRE(model.counter_rows[1].name == "renderer.same");
    MK_REQUIRE(model.counter_rows[1].value == "2");
    MK_REQUIRE(model.counter_rows[2].name == "renderer.large");
    MK_REQUIRE(model.counter_rows[2].value.contains("e+300"));
}

MK_TEST("editor resource panel model summarizes ready rhi diagnostics") {
    mirakana::editor::EditorResourcePanelInput input;
    input.device_available = true;
    input.backend_id = "d3d12";
    input.backend_label = "D3D12";
    input.frame_index = 8;
    input.rhi_counters = {
        mirakana::editor::EditorResourceCounterInput{.id = "textures_created", .label = "Textures created", .value = 4},
        mirakana::editor::EditorResourceCounterInput{.id = "buffers_created", .label = "Buffers created", .value = 3},
    };
    input.memory.os_video_memory_budget_available = true;
    input.memory.local_video_memory_budget_bytes = 512ULL * 1024ULL * 1024ULL;
    input.memory.local_video_memory_usage_bytes = 256ULL * 1024ULL * 1024ULL;
    input.memory.non_local_video_memory_budget_bytes = 1024ULL * 1024ULL * 1024ULL;
    input.memory.non_local_video_memory_usage_bytes = 128ULL * 1024ULL * 1024ULL;
    input.memory.committed_resources_byte_estimate_available = true;
    input.memory.committed_resources_byte_estimate = 32ULL * 1024ULL * 1024ULL;
    input.lifetime.live_resources = 5;
    input.lifetime.deferred_release_resources = 2;
    input.lifetime.lifetime_events = 9;
    input.lifetime.resources_by_kind = {
        mirakana::editor::EditorResourceCounterInput{.id = "texture", .label = "Textures", .value = 3},
        mirakana::editor::EditorResourceCounterInput{.id = "buffer", .label = "Buffers", .value = 2},
    };

    const auto model = mirakana::editor::make_editor_resource_panel_model(input);

    MK_REQUIRE(model.device_available);
    MK_REQUIRE(model.status == "Ready");
    MK_REQUIRE(model.status_rows.size() == 3);
    MK_REQUIRE(model.status_rows[0].id == "device");
    MK_REQUIRE(model.status_rows[0].value == "available");
    MK_REQUIRE(model.status_rows[1].id == "backend");
    MK_REQUIRE(model.status_rows[1].value == "D3D12");
    MK_REQUIRE(model.status_rows[2].id == "frame");
    MK_REQUIRE(model.status_rows[2].value == "8");
    MK_REQUIRE(model.counter_rows.size() == 2);
    MK_REQUIRE(model.counter_rows[0].id == "buffers_created");
    MK_REQUIRE(model.counter_rows[0].value == "3");
    MK_REQUIRE(model.counter_rows[1].id == "textures_created");
    MK_REQUIRE(model.counter_rows[1].value == "4");

    const auto* local_memory = find_editor_resource_row(model.memory_rows, "local_video_memory");
    MK_REQUIRE(local_memory != nullptr);
    MK_REQUIRE(local_memory->available);
    MK_REQUIRE(local_memory->value == "256.000 MiB / 512.000 MiB (50.0%)");
    const auto* committed = find_editor_resource_row(model.memory_rows, "committed_resources");
    MK_REQUIRE(committed != nullptr);
    MK_REQUIRE(committed->available);
    MK_REQUIRE(committed->value == "32.000 MiB");

    const auto* live_resources = find_editor_resource_row(model.lifetime_rows, "live_resources");
    MK_REQUIRE(live_resources != nullptr);
    MK_REQUIRE(live_resources->value == "5");
    const auto* buffers = find_editor_resource_row(model.lifetime_rows, "kind.buffer");
    MK_REQUIRE(buffers != nullptr);
    MK_REQUIRE(buffers->label == "Buffers");
    MK_REQUIRE(buffers->value == "2");

    const auto document = mirakana::editor::make_resource_panel_ui_model(model);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.status.backend.value"})->text.label == "D3D12");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.memory.local_video_memory.value"})->text.label ==
               "256.000 MiB / 512.000 MiB (50.0%)");
}

MK_TEST("editor resource panel model reports no device explicitly") {
    const auto model = mirakana::editor::make_editor_resource_panel_model(mirakana::editor::EditorResourcePanelInput{});

    MK_REQUIRE(!model.device_available);
    MK_REQUIRE(model.status == "No RHI device");
    MK_REQUIRE(model.counter_rows.empty());
    MK_REQUIRE(model.lifetime_rows.empty());

    const auto* device = find_editor_resource_row(model.status_rows, "device");
    MK_REQUIRE(device != nullptr);
    MK_REQUIRE(!device->available);
    MK_REQUIRE(device->value == "unavailable");
    const auto* local_memory = find_editor_resource_row(model.memory_rows, "local_video_memory");
    MK_REQUIRE(local_memory != nullptr);
    MK_REQUIRE(!local_memory->available);
    MK_REQUIRE(local_memory->value == "unavailable");

    const auto document = mirakana::editor::make_resource_panel_ui_model(model);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.empty"})->text.label ==
               "No RHI device diagnostics available");
}

MK_TEST("editor resource capture request model exposes reviewed host gated requests") {
    mirakana::editor::EditorResourcePanelInput input;
    input.device_available = true;
    input.backend_id = "d3d12";
    input.backend_label = "D3D12";
    input.capture_requests = {
        mirakana::editor::EditorResourceCaptureRequestInput{
            .id = "pix_gpu_capture",
            .label = "PIX GPU\nCapture",
            .tool_label = "PIX on Windows",
            .action_label = "Request PIX Capture",
            .host_gates = {"pix-installed", "d3d12-windows-primary"},
            .available = true,
            .acknowledged = false,
            .diagnostic = "Launch or attach PIX externally; do not expose native handles",
        },
        mirakana::editor::EditorResourceCaptureRequestInput{
            .id = "debug_layer_gpu_validation",
            .label = "D3D12 Debug Layer=GPU Validation",
            .tool_label = "D3D12 Debug Layer",
            .action_label = "Request debug validation prep",
            .host_gates = {"d3d12-debug-layer"},
            .available = true,
            .acknowledged = true,
            .diagnostic = "Debug-layer/GPU validation prep remains an external host workflow",
        },
    };

    const auto model = mirakana::editor::make_editor_resource_panel_model(input);

    MK_REQUIRE(model.capture_request_rows.size() == 2);
    MK_REQUIRE(model.capture_request_rows[0].id == "debug_layer_gpu_validation");
    MK_REQUIRE(model.capture_request_rows[0].label == "D3D12 Debug Layer GPU Validation");
    MK_REQUIRE(model.capture_request_rows[0].host_gates.size() == 1);
    MK_REQUIRE(model.capture_request_rows[0].host_gates[0] == "d3d12-debug-layer");
    MK_REQUIRE(model.capture_request_rows[0].request_available);
    MK_REQUIRE(model.capture_request_rows[0].request_acknowledged);
    MK_REQUIRE(!model.capture_request_rows[0].host_gate_acknowledgement_required);
    MK_REQUIRE(model.capture_request_rows[0].acknowledgement_label == "acknowledged");

    const auto* pix = find_editor_resource_capture_request_row(model.capture_request_rows, "pix_gpu_capture");
    MK_REQUIRE(pix != nullptr);
    MK_REQUIRE(pix->label == "PIX GPU Capture");
    MK_REQUIRE(pix->tool_label == "PIX on Windows");
    MK_REQUIRE(pix->action_label == "Request PIX Capture");
    MK_REQUIRE(pix->host_gates.size() == 2);
    MK_REQUIRE(pix->host_gates[0] == "d3d12-windows-primary");
    MK_REQUIRE(pix->host_gates[1] == "pix-installed");
    MK_REQUIRE(pix->host_gates_label == "d3d12-windows-primary,pix-installed");
    MK_REQUIRE(pix->request_available);
    MK_REQUIRE(!pix->request_acknowledged);
    MK_REQUIRE(pix->host_gate_acknowledgement_required);
    MK_REQUIRE(pix->acknowledgement_label == "required");
    MK_REQUIRE(pix->status_label == "Ready");
    MK_REQUIRE(pix->diagnostic.contains("external host workflow"));

    const auto document = mirakana::editor::make_resource_panel_ui_model(model);
    MK_REQUIRE(
        document.find(mirakana::ui::ElementId{"resources.capture_requests.pix_gpu_capture.action"})->text.label ==
        "Request PIX Capture");
    MK_REQUIRE(
        document.find(mirakana::ui::ElementId{"resources.capture_requests.pix_gpu_capture.host_gates"})->text.label ==
        "d3d12-windows-primary,pix-installed");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.capture_requests.pix_gpu_capture.acknowledgement"})
                   ->text.label == "required");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.capture_requests.pix_gpu_capture.diagnostic"})
                   ->text.label.contains("external host workflow"));
}

MK_TEST("editor resource capture request model blocks unavailable device requests") {
    mirakana::editor::EditorResourcePanelInput input;
    input.capture_requests = {
        mirakana::editor::EditorResourceCaptureRequestInput{
            .id = "pix_gpu_capture",
            .label = "PIX GPU Capture",
            .tool_label = "PIX on Windows",
            .action_label = "Request PIX Capture",
            .host_gates = {"d3d12-windows-primary"},
            .available = true,
            .acknowledged = true,
            .diagnostic = "Operator requested PIX capture",
        },
    };

    const auto model = mirakana::editor::make_editor_resource_panel_model(input);

    MK_REQUIRE(!model.device_available);
    MK_REQUIRE(model.capture_request_rows.size() == 1);
    const auto& request = model.capture_request_rows[0];
    MK_REQUIRE(request.id == "pix_gpu_capture");
    MK_REQUIRE(!request.request_available);
    MK_REQUIRE(!request.request_acknowledged);
    MK_REQUIRE(!request.host_gate_acknowledgement_required);
    MK_REQUIRE(request.status_label == "Blocked");
    MK_REQUIRE(request.acknowledgement_label == "blocked");
    MK_REQUIRE(request.diagnostic.contains("no RHI device"));
    MK_REQUIRE(request.diagnostic.contains("host-gated external workflow"));

    const auto document = mirakana::editor::make_resource_panel_ui_model(model);
    MK_REQUIRE(
        document.find(mirakana::ui::ElementId{"resources.capture_requests.pix_gpu_capture.status"})->text.label ==
        "Blocked");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.capture_requests.pix_gpu_capture.acknowledgement"})
                   ->text.label == "blocked");
}

MK_TEST("editor resource capture execution evidence reports host owned snapshots") {
    mirakana::editor::EditorResourcePanelInput input;
    input.device_available = true;
    input.backend_id = "d3d12";
    input.backend_label = "D3D12";
    input.capture_execution_snapshots = {
        mirakana::editor::EditorResourceCaptureExecutionInput{
            .id = "pix_gpu_capture",
            .label = "PIX GPU\nCapture",
            .tool_label = "PIX on Windows",
            .requested = true,
            .host_gated = false,
            .capture_started = false,
            .capture_completed = true,
            .capture_failed = false,
            .editor_core_executed = false,
            .exposes_native_handles = false,
            .host_gates = {"pix-installed", "d3d12-windows-primary"},
            .artifact_path = std::string(320, 'x') + "/f.wpix",
            .diagnostic = "Capture completed externally",
        },
        mirakana::editor::EditorResourceCaptureExecutionInput{
            .id = "d3d12_debug_validation",
            .label = "D3D12 Debug Layer=GPU Validation",
            .tool_label = "Windows Graphics Tools",
            .requested = true,
            .host_gated = true,
            .capture_started = false,
            .capture_completed = false,
            .capture_failed = false,
            .editor_core_executed = false,
            .exposes_native_handles = false,
            .host_gates = {"d3d12-debug-layer"},
            .artifact_path = "",
            .diagnostic = std::string(600, 'a'),
        },
        mirakana::editor::EditorResourceCaptureExecutionInput{
            .id = "unsafe_native_capture",
            .label = "Unsafe Native Capture",
            .tool_label = "Native Debugger",
            .requested = true,
            .host_gated = false,
            .capture_started = false,
            .capture_completed = true,
            .capture_failed = false,
            .editor_core_executed = true,
            .exposes_native_handles = true,
            .host_gates = {"native-handle-exposure"},
            .artifact_path = "native://texture",
            .diagnostic = "Should be rejected",
        },
    };

    const auto model = mirakana::editor::make_editor_resource_panel_model(input);

    MK_REQUIRE(model.capture_execution_rows.size() == 3);
    MK_REQUIRE(model.capture_execution_rows[0].id == "d3d12_debug_validation");
    MK_REQUIRE(model.capture_execution_rows[0].label == "D3D12 Debug Layer GPU Validation");
    MK_REQUIRE(model.capture_execution_rows[0].phase_code == "host_gated");
    MK_REQUIRE(model.capture_execution_rows[0].status_label == "Host-gated");
    MK_REQUIRE(model.capture_execution_rows[0].host_gates_label == "d3d12-debug-layer");
    MK_REQUIRE(model.capture_execution_rows[0].artifact_path == "-");
    MK_REQUIRE(model.capture_execution_rows[0].diagnostic.size() == 512);
    MK_REQUIRE(model.capture_execution_rows[0].diagnostic.contains("aaa"));

    const auto* pix = find_editor_resource_capture_execution_row(model.capture_execution_rows, "pix_gpu_capture");
    MK_REQUIRE(pix != nullptr);
    MK_REQUIRE(pix->label == "PIX GPU Capture");
    MK_REQUIRE(pix->tool_label == "PIX on Windows");
    MK_REQUIRE(pix->phase_code == "captured");
    MK_REQUIRE(pix->status_label == "Captured");
    MK_REQUIRE(pix->host_gates_label == "d3d12-windows-primary,pix-installed");
    MK_REQUIRE(pix->artifact_path.size() == 260);
    MK_REQUIRE(pix->diagnostic == "Capture completed externally");

    const auto* unsafe =
        find_editor_resource_capture_execution_row(model.capture_execution_rows, "unsafe_native_capture");
    MK_REQUIRE(unsafe != nullptr);
    MK_REQUIRE(unsafe->phase_code == "blocked");
    MK_REQUIRE(unsafe->status_label == "Blocked");
    MK_REQUIRE(unsafe->artifact_path == "-");
    MK_REQUIRE(unsafe->diagnostic.contains("editor-core execution"));
    MK_REQUIRE(unsafe->diagnostic.contains("native handle"));

    const auto document = mirakana::editor::make_resource_panel_ui_model(model);
    // Retained agent needle (check-ai-integration): ge.editor.resources_capture_execution.v1
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.capture_execution.contract_label"}) != nullptr);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.capture_execution.contract_label"})->text.label ==
               std::string{mirakana::editor::editor_resources_capture_execution_contract_v1()});
    MK_REQUIRE(document.find(mirakana::ui::ElementId{
                   "resources.capture_execution.operator_validated_launch_workflow_contract_label"}) != nullptr);
    MK_REQUIRE(
        document
            .find(mirakana::ui::ElementId{
                "resources.capture_execution.operator_validated_launch_workflow_contract_label"})
            ->text.label ==
        std::string{mirakana::editor::editor_resources_capture_operator_validated_launch_workflow_contract_v1()});
    MK_REQUIRE(
        std::string{mirakana::editor::editor_resources_capture_operator_validated_launch_workflow_contract_v1()} ==
        "ge.editor.resources_capture_operator_validated_launch_workflow.v1");
    MK_REQUIRE(
        document.find(mirakana::ui::ElementId{"resources.capture_execution.pix_gpu_capture.phase"})->text.label ==
        "captured");
    MK_REQUIRE(
        document.find(mirakana::ui::ElementId{"resources.capture_execution.pix_gpu_capture.host_gates"})->text.label ==
        "d3d12-windows-primary,pix-installed");
    MK_REQUIRE(
        document.find(mirakana::ui::ElementId{"resources.capture_execution.pix_gpu_capture.status"})->text.label ==
        "Captured");
    MK_REQUIRE(
        document.find(mirakana::ui::ElementId{"resources.capture_execution.pix_gpu_capture.artifact"})->text.label ==
        pix->artifact_path);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.capture_execution.unsafe_native_capture.diagnostic"})
                   ->text.label.contains("native handle"));
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.capture_execution.pix_gpu_capture.host_helper_hint"}) !=
               nullptr);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"resources.capture_execution.pix_gpu_capture.host_helper_hint"})
                   ->text.label.contains("Host helper column"));
}

MK_TEST("editor timeline panel model exposes playback controls tracks and events") {
    const auto model = mirakana::editor::make_editor_timeline_panel_model(
        mirakana::AnimationAuthoredTimelineDesc{
            .duration_seconds = 2.0F,
            .looping = true,
            .tracks =
                {
                    mirakana::AnimationTimelineEventTrackDesc{
                        .name = "gameplay",
                        .events = {mirakana::AnimationTimelineEventDesc{
                            .time_seconds = 0.75F, .name = "spawn", .payload = "enemy"}},
                    },
                    mirakana::AnimationTimelineEventTrackDesc{
                        .name = "audio",
                        .events = {mirakana::AnimationTimelineEventDesc{
                            .time_seconds = 0.25F, .name = "play", .payload = "step"}},
                    },
                },
        },
        3.0F, true);

    MK_REQUIRE(model.duration_seconds == 2.0F);
    MK_REQUIRE(model.playhead_seconds == 2.0F);
    MK_REQUIRE(model.looping);
    MK_REQUIRE(model.playing);
    MK_REQUIRE(model.tracks.size() == 2);
    MK_REQUIRE(model.tracks[0].id == "audio");
    MK_REQUIRE(model.tracks[0].events[0].name == "play");
    MK_REQUIRE(model.tracks[1].id == "gameplay");

    const auto document = mirakana::editor::make_timeline_ui_model(model);
    MK_REQUIRE(!document.find(mirakana::ui::ElementId{"timeline.play"})->enabled);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"timeline.pause"})->enabled);
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"timeline.track.audio.event.1.name"})->text.label == "play");
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"timeline.track.gameplay.event.1.time"})->text.label == "0.75");

    const auto serialized = mirakana::editor::serialize_editor_ui_model(document);
    MK_REQUIRE(serialized.contains("element.1.id=timeline"));
    MK_REQUIRE(serialized.contains("timeline.track.audio.event.1.name"));
}

MK_TEST("editor_asset_path_supports_gltf_mesh_inspect matches gltf and glb suffixes case-insensitive") {
    MK_REQUIRE(mirakana::editor::editor_asset_path_supports_gltf_mesh_inspect("source/meshes/a.gltf"));
    MK_REQUIRE(mirakana::editor::editor_asset_path_supports_gltf_mesh_inspect("source/meshes/b.GLTF"));
    MK_REQUIRE(mirakana::editor::editor_asset_path_supports_gltf_mesh_inspect("c.glb"));
    MK_REQUIRE(mirakana::editor::editor_asset_path_supports_gltf_mesh_inspect("x.GLB"));
    MK_REQUIRE(!mirakana::editor::editor_asset_path_supports_gltf_mesh_inspect("assets/x.png"));
    MK_REQUIRE(!mirakana::editor::editor_asset_path_supports_gltf_mesh_inspect("file.gltf.backup"));
}

MK_TEST("editor gltf mesh catalog maps failed inspect to inspector status row") {
    mirakana::GltfMeshInspectReport report;
    report.parse_succeeded = false;
    report.diagnostic = "synthetic unit diagnostic";
    const auto rows = mirakana::editor::gltf_mesh_inspect_report_to_inspector_rows(report);
    MK_REQUIRE(rows.size() == 1);
    MK_REQUIRE(rows[0].id == "gltf.inspect.status");
    MK_REQUIRE(rows[0].value.contains("failed:"));

    const auto ui = mirakana::editor::make_gltf_mesh_inspect_ui_model(report);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"gltf_mesh_inspect"}) != nullptr);
    const auto* contract = ui.find(mirakana::ui::ElementId{"gltf_mesh_inspect.contract"});
    MK_REQUIRE(contract != nullptr);
    MK_REQUIRE(contract->text.label == "ge.editor.gltf_mesh_inspect.v1");
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"gltf_mesh_inspect.rows.gltf.inspect.status.caption"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"gltf_mesh_inspect.rows.gltf.inspect.status.text"}) != nullptr);

    const auto serialized = mirakana::editor::serialize_editor_ui_model(ui);
    MK_REQUIRE(serialized.contains("element.1.id=gltf_mesh_inspect"));
    MK_REQUIRE(serialized.contains("gltf_mesh_inspect.contract"));
}

MK_TEST("editor gltf mesh catalog maps triangle gltf inspect when importers are enabled") {
    if (!mirakana::external_asset_importers_available()) {
        return;
    }
    auto append_le_u16 = [](std::string& output, std::uint16_t value) {
        output.push_back(static_cast<char>(value & 0xFFU));
        output.push_back(static_cast<char>((value >> 8U) & 0xFFU));
    };
    auto append_le_u32 = [](std::string& output, std::uint32_t value) {
        output.push_back(static_cast<char>(value & 0xFFU));
        output.push_back(static_cast<char>((value >> 8U) & 0xFFU));
        output.push_back(static_cast<char>((value >> 16U) & 0xFFU));
        output.push_back(static_cast<char>((value >> 24U) & 0xFFU));
    };
    auto append_le_f32 = [&append_le_u32](std::string& output, float value) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(float));
        append_le_u32(output, bits);
    };
    auto base64_encode = [](std::string_view bytes) -> std::string {
        constexpr auto alphabet = std::string_view{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
        std::string encoded;
        encoded.reserve(((bytes.size() + 2U) / 3U) * 4U);
        for (std::size_t index = 0; index < bytes.size(); index += 3U) {
            const auto remaining = bytes.size() - index;
            const auto first = static_cast<unsigned char>(bytes[index]);
            const auto second = remaining > 1U ? static_cast<unsigned char>(bytes[index + 1U]) : 0U;
            const auto third = remaining > 2U ? static_cast<unsigned char>(bytes[index + 2U]) : 0U;
            const auto packed = (static_cast<std::uint32_t>(first) << 16U) |
                                (static_cast<std::uint32_t>(second) << 8U) | static_cast<std::uint32_t>(third);
            encoded.push_back(alphabet[(packed >> 18U) & 0x3FU]);
            encoded.push_back(alphabet[(packed >> 12U) & 0x3FU]);
            encoded.push_back(remaining > 1U ? alphabet[(packed >> 6U) & 0x3FU] : '=');
            encoded.push_back(remaining > 2U ? alphabet[packed & 0x3FU] : '=');
        }
        return encoded;
    };
    auto gltf_data_uri = [&base64_encode](std::string_view bytes) {
        return std::string{"data:application/octet-stream;base64,"} + base64_encode(bytes);
    };

    std::string triangle_buffer;
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 2.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 0.0F);
    append_le_f32(triangle_buffer, 3.0F);
    append_le_u16(triangle_buffer, 2U);
    append_le_u16(triangle_buffer, 1U);
    append_le_u16(triangle_buffer, 0U);
    const std::string document =
        std::string{"{\"asset\":{\"version\":\"2.0\"},"
                    "\"buffers\":[{\"byteLength\":"} +
        std::to_string(triangle_buffer.size()) + R"(,"uri":")" + gltf_data_uri(triangle_buffer) +
        "\"}],"
        "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":6}],"
        "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1}]}]}";

    const auto report = mirakana::inspect_gltf_mesh_primitives(document, "source/meshes/triangle.gltf");
    MK_REQUIRE(report.parse_succeeded);
    const auto rows = mirakana::editor::gltf_mesh_inspect_report_to_inspector_rows(report);
    MK_REQUIRE(rows.size() >= 3);
    MK_REQUIRE(rows[0].id == "gltf.inspect.status");
    MK_REQUIRE(rows[1].id == "gltf.primitive.0.0.mesh");
    MK_REQUIRE(rows[2].id == "gltf.primitive.0.0.layout");
    MK_REQUIRE(rows[2].value.contains("positions=3"));
    MK_REQUIRE(rows[2].value.contains("indexed=true"));

    const auto ui = mirakana::editor::make_gltf_mesh_inspect_ui_model(report);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"gltf_mesh_inspect.contract"}) != nullptr);
    MK_REQUIRE(ui.find(mirakana::ui::ElementId{"gltf_mesh_inspect.rows.gltf.primitive.0.0.mesh.text"}) != nullptr);
}

} // namespace

int main() {
    return mirakana::test::run_all();
}
