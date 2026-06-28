// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_app.hpp"

#include "mirakana/assets/material.hpp"
#include "mirakana/assets/source_asset_registry.hpp"
#include "mirakana/core/diagnostics.hpp"
#include "mirakana/editor/editor_dock_layout.hpp"
#include "mirakana/editor/shader_compile.hpp"
#include "mirakana/environment/environment_profile.hpp"
#include "mirakana/platform/clipboard.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/tools/asset_import_adapters.hpp"
#include "mirakana/tools/asset_import_tool.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

[[nodiscard]] ProjectDocument make_default_project_document() {
    return ProjectDocument{
        .name = "MIRAIKANAI Editor",
        .root_path = ".",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
    };
}

[[nodiscard]] Workspace make_default_workspace(const ProjectDocument& project) {
    auto workspace = Workspace::create_default(ProjectInfo{.name = project.name, .root_path = project.root_path});
    workspace.set_panel_visible(PanelId::resources, true);
    workspace.set_panel_visible(PanelId::ai_commands, true);
    workspace.set_panel_visible(PanelId::profiler, true);
    workspace.set_panel_visible(PanelId::project_settings, true);
    workspace.set_panel_visible(PanelId::timeline, true);
    workspace.set_panel_visible(PanelId::input_rebinding, false);
    workspace.set_panel_visible(PanelId::runtime_ui_editor, true);
    return workspace;
}

[[nodiscard]] EditorRuntimeUiDocumentModel make_default_runtime_ui_document() {
    EditorRuntimeUiDocumentModel document;
    document.document_id = "editor_runtime_hud";
    document.revision = 1U;
    document.elements = {
        EditorRuntimeUiElementRow{
            .id = "root",
            .role = ui::SemanticRole::panel,
            .label = "Runtime HUD",
            .style_token = "panel.surface",
        },
        EditorRuntimeUiElementRow{
            .id = "score_label",
            .parent_id = "root",
            .role = ui::SemanticRole::label,
            .label = "Score 0000",
            .style_token = "text.body",
        },
        EditorRuntimeUiElementRow{
            .id = "start_button",
            .parent_id = "root",
            .role = ui::SemanticRole::button,
            .label = "Start",
            .style_token = "button.primary",
        },
    };
    document.selected_element_id = "start_button";
    return document;
}

[[nodiscard]] EditorRuntimeUiThemeModel make_default_runtime_ui_theme() {
    EditorRuntimeUiThemeModel theme;
    theme.revision = 1U;
    theme.style_tokens = {
        EditorRuntimeUiStyleTokenRow{.id = "panel.surface",
                                     .label = "Panel surface",
                                     .foreground_token = "editor.text",
                                     .background_token = "editor.panel.background"},
        EditorRuntimeUiStyleTokenRow{.id = "button.primary",
                                     .label = "Primary button",
                                     .foreground_token = "editor.text",
                                     .background_token = "editor.action.primary"},
        EditorRuntimeUiStyleTokenRow{.id = "text.body",
                                     .label = "Body text",
                                     .foreground_token = "editor.text",
                                     .background_token = "editor.panel.background"},
    };
    return theme;
}

[[nodiscard]] SceneAuthoringDocument make_default_scene_document() {
    mirakana::Scene scene{"EditorScene"};
    const auto root = scene.create_node("Root");
    const auto camera = scene.create_node("Camera");
    const auto light = scene.create_node("KeyLight");
    scene.set_parent(camera, root);
    scene.set_parent(light, root);

    auto document = SceneAuthoringDocument::from_scene(std::move(scene), "scenes/start.scene");
    (void)document.select_node(root);
    return document;
}

[[nodiscard]] EnvironmentAuthoringDocument make_default_environment_authoring_document() {
    EnvironmentProfileDocumentV2 document;
    document.global_profile = EnvironmentProfileDesc{};
    document.quality_preset = EnvironmentQualityPreset::high;
    return EnvironmentAuthoringDocument::from_profile_document_v2(std::move(document),
                                                                  "assets/environment/default.environment");
}

[[nodiscard]] EnvironmentCloudLayerDesc make_default_environment_cloud_layer() {
    return EnvironmentCloudLayerDesc{
        .mode = EnvironmentCloudLayerMode::equirectangular_2d,
        .coverage = 0.35F,
        .opacity = 0.75F,
        .altitude_m = 2400.0F,
        .wind_velocity_mps = Vec2{.x = 2.0F, .y = 0.0F},
        .cloud_map_asset_ref = "textures/environment/default_clouds",
        .flow_map_asset_ref = "textures/environment/default_cloud_flow",
        .sky_tint_response = Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .time_of_day_response = 0.5F,
        .ibl_contribution_mode = EnvironmentCloudIblContributionMode::sky_tint_only,
        .ibl_contribution = 0.25F,
    };
}

[[nodiscard]] EnvironmentAuthoringInspectorModel
make_default_environment_authoring_inspector(const EnvironmentAuthoringDocument& document) {
    return make_environment_authoring_inspector_model(EnvironmentAuthoringInspectorDesc{
        .document = document,
        .cloud_layer = make_default_environment_cloud_layer(),
        .volumetric_clouds_policy_available = true,
    });
}

[[nodiscard]] std::vector<EditorPropertyRow>
make_default_inspector_rows(const ProjectDocument& project, const EnvironmentAuthoringInspectorModel& environment) {
    std::vector<EditorPropertyRow> rows{
        EditorPropertyRow{.id = "project", .label = "Project", .value = project.name, .editable = false},
        EditorPropertyRow{.id = "scene", .label = "Startup Scene", .value = project.startup_scene_path},
        EditorPropertyRow{.id = "asset_root", .label = "Asset Root", .value = project.asset_root},
        EditorPropertyRow{.id = "renderer", .label = "Renderer", .value = "native_win32_d3d12", .editable = false},
    };
    auto environment_rows = make_environment_authoring_editor_property_rows(environment);
    rows.insert(rows.end(), environment_rows.begin(), environment_rows.end());
    return rows;
}

[[nodiscard]] SourceAssetRegistryRowV1 make_default_asset_browser_source_row(AssetKeyV2 key, AssetKind kind,
                                                                             std::string source_path,
                                                                             std::string imported_path) {
    return SourceAssetRegistryRowV1{
        .key = std::move(key),
        .kind = kind,
        .source_path = std::move(source_path),
        .source_format = std::string{expected_source_asset_format_v1(kind)},
        .imported_path = std::move(imported_path),
    };
}

[[nodiscard]] SourceAssetRegistryDocumentV1 make_default_asset_browser_source_registry() {
    return SourceAssetRegistryDocumentV1{
        .assets =
            {
                make_default_asset_browser_source_row(AssetKeyV2{"assets/scenes/start"}, AssetKind::scene,
                                                      "source/scenes/start.scene", "assets/scenes/start.scene"),
                make_default_asset_browser_source_row(AssetKeyV2{"assets/materials/default"}, AssetKind::material,
                                                      "source/materials/default.material",
                                                      "assets/materials/default.material"),
                make_default_asset_browser_source_row(AssetKeyV2{"assets/textures/editor_preview"}, AssetKind::texture,
                                                      "source/textures/editor_preview.texture",
                                                      "assets/textures/editor_preview.texture"),
            },
    };
}

[[nodiscard]] ContentBrowserState
make_default_asset_browser_content_browser(const SourceAssetRegistryDocumentV1& source_registry) {
    ContentBrowserState browser;
    browser.refresh_from(source_registry);
    return browser;
}

[[nodiscard]] AssetImportActionKind asset_import_action_kind_for_asset_browser(AssetKind kind) noexcept {
    switch (kind) {
    case AssetKind::texture:
        return AssetImportActionKind::texture;
    case AssetKind::material:
        return AssetImportActionKind::material;
    case AssetKind::scene:
        return AssetImportActionKind::scene;
    default:
        return AssetImportActionKind::unknown;
    }
}

[[nodiscard]] AssetImportPlan make_default_asset_browser_import_plan(const SourceAssetRegistryDocumentV1& registry) {
    AssetImportPlan plan;
    for (const auto& row : registry.assets) {
        const auto kind = asset_import_action_kind_for_asset_browser(row.kind);
        if (kind == AssetImportActionKind::unknown) {
            continue;
        }
        plan.actions.push_back(AssetImportAction{
            .id = asset_id_from_key_v2(row.key),
            .kind = kind,
            .source_path = row.source_path,
            .output_path = row.imported_path,
        });
    }
    return plan;
}

[[nodiscard]] EditorAssetBrowserProductionModel make_default_asset_browser_model(const ProjectDocument& project,
                                                                                 const ContentBrowserState& browser,
                                                                                 const AssetImportPlan& import_plan) {
    return make_editor_asset_browser_production_model(EditorAssetBrowserProductionDesc{
        .browser = &browser,
        .import_plan = &import_plan,
        .project_root = project.root_path,
        .asset_root = project.asset_root,
        .source_registry_path = project.source_registry_path,
        .generation = 1U,
    });
}

[[nodiscard]] std::vector<EditorAssetBrowserCommandPlan>
make_default_asset_browser_command_plans(const EditorAssetBrowserProductionModel& model) {
    constexpr std::array kinds{
        EditorAssetBrowserCommandKind::reload_source_registry,
        EditorAssetBrowserCommandKind::review_import_sources,
        EditorAssetBrowserCommandKind::copy_external_sources,
        EditorAssetBrowserCommandKind::execute_reviewed_import_plan,
        EditorAssetBrowserCommandKind::preview_cooked_package,
        EditorAssetBrowserCommandKind::stage_hot_reload_recook,
        EditorAssetBrowserCommandKind::inspect_selection,
        EditorAssetBrowserCommandKind::apply_package_registration,
    };

    std::vector<EditorAssetBrowserCommandPlan> plans;
    plans.reserve(kinds.size());
    for (const auto kind : kinds) {
        plans.push_back(plan_editor_asset_browser_command(EditorAssetBrowserCommandRequest{
            .kind = kind,
            .mode = EditorAssetBrowserCommandMode::dry_run,
            .expected_generation = model.generation,
            .current_generation = model.generation,
        }));
    }
    return plans;
}

[[nodiscard]] NativeEditorEnvironmentArtistWorkflowCommandPlanRow
make_environment_artist_workflow_shell_command_row(const EnvironmentAuthoringDocument& document,
                                                   EnvironmentArtistWorkflowCommandKind kind) {
    auto dry_run =
        plan_environment_artist_workflow_command(document, EnvironmentArtistWorkflowCommandRequest{
                                                               .kind = kind,
                                                               .mode = EnvironmentArtistWorkflowCommandMode::dry_run,
                                                               .expected_revision = document.revision(),
                                                           });
    auto apply =
        plan_environment_artist_workflow_command(document, EnvironmentArtistWorkflowCommandRequest{
                                                               .kind = kind,
                                                               .mode = EnvironmentArtistWorkflowCommandMode::apply,
                                                               .expected_revision = document.revision(),
                                                               .user_confirmed = true,
                                                           });
    return NativeEditorEnvironmentArtistWorkflowCommandPlanRow{
        .command_id = dry_run.command_id,
        .label = dry_run.label,
        .dry_run = std::move(dry_run),
        .apply = std::move(apply),
    };
}

[[nodiscard]] std::vector<NativeEditorEnvironmentArtistWorkflowCommandPlanRow>
make_default_environment_artist_workflow_command_plans(const EnvironmentAuthoringDocument& document) {
    constexpr std::array kinds{
        EnvironmentArtistWorkflowCommandKind::source_asset_review,
        EnvironmentArtistWorkflowCommandKind::cook_preview,
        EnvironmentArtistWorkflowCommandKind::package_preview,
        EnvironmentArtistWorkflowCommandKind::validation_remediation,
        EnvironmentArtistWorkflowCommandKind::publish_package,
    };

    std::vector<NativeEditorEnvironmentArtistWorkflowCommandPlanRow> rows;
    rows.reserve(kinds.size());
    for (const auto kind : kinds) {
        rows.push_back(make_environment_artist_workflow_shell_command_row(document, kind));
    }
    return rows;
}

[[nodiscard]] EnvironmentArtistWorkflowAssetBrowserModel make_default_environment_artist_workflow_asset_browser() {
    return make_environment_artist_workflow_asset_browser_model(EnvironmentArtistWorkflowAssetBrowserDesc{
        .assets =
            {
                EnvironmentArtistWorkflowAssetBrowserInputRow{
                    .kind = EnvironmentArtistWorkflowAssetKind::preset_library,
                    .path = "runtime/assets/desktop_runtime/environment_presets.gepresetpack",
                    .available = true,
                    .package_visible = true,
                    .provenance_recorded = true,
                    .budget_recorded = true,
                    .validation_recipe_id = "desktop-runtime-sample-game-environment-preset-library-package",
                },
                EnvironmentArtistWorkflowAssetBrowserInputRow{
                    .kind = EnvironmentArtistWorkflowAssetKind::openexr_source,
                    .path = "assets/source/environment/storm.exr",
                    .available = true,
                    .provenance_recorded = true,
                    .validation_recipe_id = "asset-importers",
                },
                EnvironmentArtistWorkflowAssetBrowserInputRow{
                    .kind = EnvironmentArtistWorkflowAssetKind::ktx2_basis_source,
                    .path = "assets/source/environment/clouds.ktx2",
                    .available = true,
                    .provenance_recorded = true,
                    .validation_recipe_id = "asset-importers",
                },
                EnvironmentArtistWorkflowAssetBrowserInputRow{
                    .kind = EnvironmentArtistWorkflowAssetKind::cooked_texture,
                    .path = "runtime/assets/desktop_runtime/environment_clouds.geasset",
                    .available = true,
                    .package_visible = true,
                    .budget_recorded = true,
                    .validation_recipe_id = "desktop-runtime-sample-game-environment-texture-package",
                },
                EnvironmentArtistWorkflowAssetBrowserInputRow{
                    .kind = EnvironmentArtistWorkflowAssetKind::environment_profile,
                    .path = "runtime/assets/desktop_runtime/default_outdoor.geenv",
                    .available = true,
                    .package_visible = true,
                    .validation_recipe_id = "desktop-runtime-sample-game-environment-ready-aggregate",
                },
                EnvironmentArtistWorkflowAssetBrowserInputRow{
                    .kind = EnvironmentArtistWorkflowAssetKind::simulation_preset,
                    .path = "runtime/assets/desktop_runtime/weather_simulation.geweather",
                    .available = true,
                    .package_visible = true,
                    .validation_recipe_id = "desktop-runtime-sample-game-environment-weather-simulation-package",
                },
                EnvironmentArtistWorkflowAssetBrowserInputRow{
                    .kind = EnvironmentArtistWorkflowAssetKind::validation_report,
                    .path = "artifacts/environment/validation-report.txt",
                    .available = true,
                    .validation_recipe_id = "agent-contract",
                },
                EnvironmentArtistWorkflowAssetBrowserInputRow{
                    .kind = EnvironmentArtistWorkflowAssetKind::package_artifact,
                    .path = "out/package/sample_desktop_runtime_game",
                    .available = true,
                    .package_visible = true,
                    .validation_recipe_id = "desktop-game-runtime",
                },
            },
    });
}

[[nodiscard]] EnvironmentArtistWorkflowPreviewModel make_default_environment_artist_workflow_preview() {
    return make_environment_artist_workflow_preview_model(EnvironmentArtistWorkflowPreviewDesc{
        .selected_backend = "d3d12",
        .quality_tier = "ultra",
        .package_budget_bytes = 4194304U,
        .memory_budget_bytes = 67108864U,
        .unsupported_claim_reason = "complete artist workflow readiness waits for full commercial closeout",
    });
}

[[nodiscard]] EnvironmentArtistWorkflowWalkthroughModel make_default_environment_artist_workflow_walkthrough() {
    return make_environment_artist_workflow_walkthrough_model(EnvironmentArtistWorkflowWalkthroughDesc{
        .steps =
            {
                EnvironmentArtistWorkflowWalkthroughStepInputRow{
                    .kind = EnvironmentArtistWorkflowWalkthroughStepKind::import_source_assets,
                    .evidence_id = "source-assets-reviewed",
                    .completed = true,
                    .reviewed = true,
                    .validation_recipe_id = "asset-importers",
                },
                EnvironmentArtistWorkflowWalkthroughStepInputRow{
                    .kind = EnvironmentArtistWorkflowWalkthroughStepKind::cook_assets,
                    .evidence_id = "environment-cook-preview",
                    .completed = true,
                    .reviewed = true,
                    .package_visible = true,
                    .validation_recipe_id = "desktop-runtime-sample-game-environment-texture-package",
                },
                EnvironmentArtistWorkflowWalkthroughStepInputRow{
                    .kind = EnvironmentArtistWorkflowWalkthroughStepKind::assemble_preset,
                    .evidence_id = "preset-pack-assembled",
                    .completed = true,
                    .reviewed = true,
                    .package_visible = true,
                    .validation_recipe_id = "desktop-runtime-sample-game-environment-preset-library-package",
                },
                EnvironmentArtistWorkflowWalkthroughStepInputRow{
                    .kind = EnvironmentArtistWorkflowWalkthroughStepKind::edit_weather_timeline,
                    .evidence_id = "weather-timeline-edited",
                    .completed = true,
                    .reviewed = true,
                    .validation_recipe_id = "agent-contract",
                },
                EnvironmentArtistWorkflowWalkthroughStepInputRow{
                    .kind = EnvironmentArtistWorkflowWalkthroughStepKind::run_simulation_preview,
                    .evidence_id = "weather-simulation-preview",
                    .completed = true,
                    .reviewed = true,
                    .validation_recipe_id = "desktop-runtime-sample-game-environment-weather-simulation-package",
                },
                EnvironmentArtistWorkflowWalkthroughStepInputRow{
                    .kind = EnvironmentArtistWorkflowWalkthroughStepKind::package_sample,
                    .evidence_id = "sample-package-built",
                    .completed = true,
                    .reviewed = true,
                    .package_visible = true,
                    .validation_recipe_id = "desktop-game-runtime",
                },
                EnvironmentArtistWorkflowWalkthroughStepInputRow{
                    .kind = EnvironmentArtistWorkflowWalkthroughStepKind::run_installed_validation,
                    .evidence_id = "installed-validation-run",
                    .completed = true,
                    .reviewed = true,
                    .package_visible = true,
                    .validation_recipe_id = "desktop-runtime-sample-game-environment-ready-aggregate",
                },
                EnvironmentArtistWorkflowWalkthroughStepInputRow{
                    .kind = EnvironmentArtistWorkflowWalkthroughStepKind::inspect_report,
                    .evidence_id = "artist-report-reviewed",
                    .completed = true,
                    .reviewed = true,
                    .validation_recipe_id = "agent-contract",
                },
            },
    });
}

[[nodiscard]] EnvironmentArtistWorkflowExecutionReviewModel
make_default_environment_artist_workflow_execution_review(const EnvironmentAuthoringDocument& document) {
    return make_environment_artist_workflow_execution_review_model(EnvironmentArtistWorkflowExecutionReviewDesc{
        .command_catalog = make_environment_artist_workflow_command_catalog(document),
        .asset_browser = make_default_environment_artist_workflow_asset_browser(),
        .preview = make_default_environment_artist_workflow_preview(),
        .walkthrough = make_default_environment_artist_workflow_walkthrough(),
        .evidence_rows =
            {
                EnvironmentArtistWorkflowExecutionEvidenceRow{
                    .recipe_id = "asset-importers",
                    .passed = true,
                    .summary = "source asset importer lane passed",
                },
                EnvironmentArtistWorkflowExecutionEvidenceRow{
                    .recipe_id = "desktop-runtime-sample-game-environment-texture-package",
                    .passed = true,
                    .summary = "texture package lane passed",
                },
                EnvironmentArtistWorkflowExecutionEvidenceRow{
                    .recipe_id = "desktop-runtime-sample-game-environment-preset-library-package",
                    .passed = true,
                    .summary = "preset package lane passed",
                },
                EnvironmentArtistWorkflowExecutionEvidenceRow{
                    .recipe_id = "agent-contract",
                    .passed = true,
                    .summary = "agent contract check passed",
                },
                EnvironmentArtistWorkflowExecutionEvidenceRow{
                    .recipe_id = "desktop-runtime-sample-game-environment-weather-simulation-package",
                    .passed = true,
                    .summary = "weather simulation package lane passed",
                },
                EnvironmentArtistWorkflowExecutionEvidenceRow{
                    .recipe_id = "desktop-game-runtime",
                    .passed = true,
                    .summary = "desktop runtime package lane passed",
                },
                EnvironmentArtistWorkflowExecutionEvidenceRow{
                    .recipe_id = "desktop-runtime-sample-game-environment-ready-aggregate",
                    .passed = true,
                    .summary = "installed aggregate lane passed",
                },
            },
        .operator_reviewed = true,
        .operator_review_id = "environment-artist-workflow-visible-review",
    });
}

[[nodiscard]] EditorMaterialAssetPreviewPanelModel make_default_material_preview_panel_model() {
    constexpr std::string_view shader_output_root{"out/editor/shaders"};
    const auto material_id = AssetId::from_name("materials/default");

    MemoryFileSystem filesystem;
    AssetRegistry registry;
    registry.add(AssetRecord{
        .id = material_id,
        .kind = AssetKind::material,
        .path = "assets/materials/default.material",
    });
    const MaterialDefinition material{
        .id = material_id,
        .name = "Default Material",
        .shading_model = MaterialShadingModel::lit,
        .surface_mode = MaterialSurfaceMode::opaque,
        .factors =
            MaterialFactors{
                .base_color = {0.8F, 0.8F, 0.8F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 1.0F,
            },
        .texture_bindings = {},
        .double_sided = false,
    };
    filesystem.write_text("assets/materials/default.material", serialize_material_definition(material));

    const auto shader_requests = make_material_preview_shader_compile_requests(shader_output_root);
    filesystem.write_text("out/editor/shaders/editor-material-preview-factor.vs.dxil", "native factor vertex");
    filesystem.write_text("out/editor/shaders/editor-material-preview-factor.ps.dxil", "native factor fragment");
    ViewportShaderArtifactState shader_artifacts;
    shader_artifacts.refresh_from(filesystem, shader_requests);

    return make_editor_material_asset_preview_panel_model(filesystem, registry, material_id, shader_artifacts);
}

[[nodiscard]] std::vector<EditorDiagnosticRow> make_default_console_rows() {
    return {
        EditorDiagnosticRow{
            .id = "native_shell", .severity = EditorDiagnosticSeverity::info, .message = "Native editor shell ready"},
        EditorDiagnosticRow{.id = "legacy_middleware_removed",
                            .severity = EditorDiagnosticSeverity::info,
                            .message = "Legacy middleware dependency absent"},
    };
}

[[nodiscard]] EditorResourcePanelInput make_native_resource_panel_input(bool device_available,
                                                                        std::uint64_t frame_index) {
    return EditorResourcePanelInput{
        .device_available = device_available,
        .backend_id = "d3d12",
        .backend_label = "Native Win32/D3D12 host",
        .frame_index = frame_index,
        .rhi_counters =
            {
                EditorResourceCounterInput{
                    .id = "swapchain_back_buffers", .label = "Swapchain back buffers", .value = 2U},
                EditorResourceCounterInput{
                    .id = "first_party_ui_renderer", .label = "First-party UI renderer", .value = 1U},
            },
        .capture_requests =
            {
                EditorResourceCaptureRequestInput{
                    .id = "pix_gpu_capture",
                    .label = "PIX GPU capture",
                    .tool_label = "PIX",
                    .action_label = "Request external capture",
                    .host_gates = {"windows", "pix"},
                    .available = false,
                    .diagnostic = "external host workflow only",
                },
            },
    };
}

[[nodiscard]] EditorResourcePanelModel make_native_resource_panel_model(bool device_available,
                                                                        std::uint64_t frame_index) {
    return make_editor_resource_panel_model(make_native_resource_panel_input(device_available, frame_index));
}

[[nodiscard]] EditorAiCommandPanelModel make_default_ai_command_model() {
    EditorAiCommandPanelDesc desc;
    return make_editor_ai_command_panel_model(desc);
}

[[nodiscard]] EditorProfilerPanelModel make_default_profiler_model(std::span<const EditorDiagnosticRow> console_rows) {
    mirakana::DiagnosticCapture capture;
    return make_editor_profiler_panel_model(
        capture, EditorProfilerStatus{.log_records = console_rows.size(), .dirty = false, .revision = 1});
}

[[nodiscard]] EditorTimelinePanelModel make_default_timeline_model() {
    mirakana::AnimationAuthoredTimelineDesc desc{
        .duration_seconds = 2.0F,
        .looping = true,
        .tracks =
            {
                mirakana::AnimationTimelineEventTrackDesc{
                    .name = "editor",
                    .events =
                        {
                            mirakana::AnimationTimelineEventDesc{
                                .time_seconds = 0.25F,
                                .name = "open",
                                .payload = "shell",
                                .track = "editor",
                            },
                            mirakana::AnimationTimelineEventDesc{
                                .time_seconds = 1.0F,
                                .name = "smoke",
                                .payload = "frames",
                                .track = "editor",
                            },
                        },
                },
            },
    };
    return make_editor_timeline_panel_model(desc, 0.0F, false);
}

class NativeEditorClipboardTextAdapter final : public mirakana::ui::IClipboardTextAdapter {
  public:
    explicit NativeEditorClipboardTextAdapter(IClipboard& clipboard) noexcept : clipboard_(&clipboard) {}

    void set_clipboard_text(std::string_view text) override {
        if (text.empty()) {
            clipboard_->clear();
            return;
        }
        clipboard_->set_text(text);
    }

    [[nodiscard]] bool has_clipboard_text() const override {
        return clipboard_->has_text();
    }

    [[nodiscard]] std::string clipboard_text() const override {
        return clipboard_->text();
    }

  private:
    IClipboard* clipboard_{nullptr};
};

class NativeEditorPlatformTextInputAdapter final : public mirakana::ui::IPlatformIntegrationAdapter {
  public:
    void begin_text_input(const mirakana::ui::PlatformTextInputRequest& request) override {
        active_request_ = request;
    }

    void end_text_input(const mirakana::ui::ElementId& target) override {
        if (active_request_.has_value() && active_request_->target == target) {
            active_request_.reset();
        }
    }

  private:
    std::optional<mirakana::ui::PlatformTextInputRequest> active_request_;
};

class NativeEditorImeAdapter final : public mirakana::ui::IImeAdapter {
  public:
    void update_composition(const mirakana::ui::ImeComposition& composition) override {
        composition_ = composition;
    }

  private:
    mirakana::ui::ImeComposition composition_;
};

void append_diagnostics(std::vector<ui::AdapterPayloadDiagnostic>& target,
                        const std::vector<ui::AdapterPayloadDiagnostic>& source) {
    target.insert(target.end(), source.begin(), source.end());
}

[[nodiscard]] ui::AdapterPayloadDiagnostic
make_text_input_diagnostic(ui::ElementId id, ui::AdapterPayloadDiagnosticCode code, std::string message) {
    return ui::AdapterPayloadDiagnostic{.id = std::move(id), .code = code, .message = std::move(message)};
}

[[nodiscard]] bool is_win32_tsf_service_id(std::string_view id) noexcept {
    return id == "win32_tsf";
}

[[nodiscard]] bool contains_invalid_path_characters(std::string_view path) noexcept {
    return path.empty() || path.find('\n') != std::string_view::npos || path.find('\r') != std::string_view::npos ||
           path.find('=') != std::string_view::npos || path.find(';') != std::string_view::npos;
}

[[nodiscard]] bool contains_invalid_project_path_characters(std::string_view path) noexcept {
    return contains_invalid_path_characters(path) || path.find('\\') != std::string_view::npos ||
           path.find(':') != std::string_view::npos;
}

[[nodiscard]] bool is_device_path(std::string_view path) {
    auto normalized = std::string(path);
    std::ranges::replace(normalized, '\\', '/');
    return normalized.starts_with("//./") || normalized.starts_with("//?/");
}

[[nodiscard]] bool has_parent_segment(const std::filesystem::path& path) {
    return std::ranges::any_of(path, [](const std::filesystem::path& segment) { return segment == ".."; });
}

[[nodiscard]] std::filesystem::path normalized_project_root(std::string_view root_path) {
    return std::filesystem::absolute(std::filesystem::path{std::string(root_path)}).lexically_normal();
}

[[nodiscard]] std::string normalize_selected_import_path(std::string_view project_root, std::string_view selected_path,
                                                         std::string& diagnostic) {
    if (contains_invalid_path_characters(selected_path)) {
        diagnostic = "asset browser import source selection contains invalid characters";
        return {};
    }
    if (is_device_path(selected_path)) {
        diagnostic = "asset browser import source selection must not be a device path";
        return {};
    }

    const auto root = normalized_project_root(project_root);
    const auto selected = std::filesystem::path{std::string(selected_path)};
    const auto normalized = (selected.is_absolute() ? selected : root / selected).lexically_normal();
    const auto relative = normalized.lexically_relative(root);
    auto project_path = relative.generic_string();
    if (project_path.empty() || project_path == "." || has_parent_segment(relative)) {
        diagnostic = "asset browser import source selection must resolve inside the project root";
        return {};
    }
    if (contains_invalid_project_path_characters(project_path)) {
        diagnostic = "asset browser import source selection contains invalid characters";
        return {};
    }
    return project_path;
}

[[nodiscard]] std::string imported_source_target_path(std::string_view asset_root, std::string_view source_path,
                                                      std::string& diagnostic) {
    if (contains_invalid_path_characters(source_path)) {
        diagnostic = "external import source copy source path contains invalid characters";
        return {};
    }
    if (is_device_path(source_path)) {
        diagnostic = "external import source copy source path must not be a device path";
        return {};
    }

    const auto filename = std::filesystem::path{std::string(source_path)}.filename().generic_string();
    if (filename.empty() || filename == "." || filename == ".." || contains_invalid_project_path_characters(filename)) {
        diagnostic = "external import source copy source path filename is invalid";
        return {};
    }

    return (std::filesystem::path{std::string(asset_root)} / "imported_sources" / filename).generic_string();
}

[[nodiscard]] bool contains_string(std::span<const std::string> values, std::string_view value) {
    return std::ranges::any_of(values, [value](const std::string& candidate) { return candidate == value; });
}

} // namespace

struct NativeEditorApp::Impl {
    explicit Impl(const NativeEditorLaunchOptions& options)
        : project(make_default_project_document()), workspace(make_default_workspace(project)),
          scene(make_default_scene_document()), environment_authoring(make_default_environment_authoring_document()),
          environment_authoring_inspector(make_default_environment_authoring_inspector(environment_authoring)),
          inspector_rows(make_default_inspector_rows(project, environment_authoring_inspector)),
          asset_browser_source_registry(make_default_asset_browser_source_registry()),
          asset_browser_content_browser(make_default_asset_browser_content_browser(asset_browser_source_registry)),
          asset_browser_import_plan(make_default_asset_browser_import_plan(asset_browser_source_registry)),
          asset_browser(
              make_default_asset_browser_model(project, asset_browser_content_browser, asset_browser_import_plan)),
          asset_browser_command_plans(make_default_asset_browser_command_plans(asset_browser)),
          console_rows(make_default_console_rows()),
          environment_artist_workflow_command_plans(
              make_default_environment_artist_workflow_command_plans(environment_authoring)),
          environment_artist_workflow_execution_review(
              make_default_environment_artist_workflow_execution_review(environment_authoring)),
          resources(make_native_resource_panel_model(false, 0U)), ai_commands(make_default_ai_command_model()),
          runtime_ui_document(make_default_runtime_ui_document()), runtime_ui_theme(make_default_runtime_ui_theme()),
          runtime_ui_authoring(make_editor_runtime_ui_authoring_model(runtime_ui_document, runtime_ui_theme)),
          profiler(make_default_profiler_model(console_rows)), timeline(make_default_timeline_model()),
          material_preview(make_default_material_preview_panel_model()),
          material_preview_display(plan_native_material_preview_display(NativeMaterialPreviewDisplayDesc{
              .shader_artifacts_available = material_preview.required_shader_ready,
              .gpu_payload_available = material_preview.gpu_payload_ready,
          })),
          viewport_display(plan_native_viewport_display(NativeViewportDisplayDesc{
              .extent = ViewportExtent{.width = options.width, .height = options.height},
          })),
          text_input_state(
              make_native_editor_text_input_state(make_native_editor_project_name_text_input_target(project.name))),
          clipboard_text_adapter(memory_clipboard) {
        file_dialog_service = &memory_file_dialog_service;
        clipboard_adapter = &clipboard_text_adapter;
        process_runner = &recording_process_runner;
        platform_text_input_adapter = &memory_text_input_adapter;
        ime_adapter = &memory_ime_adapter;
    }

    ProjectDocument project;
    Workspace workspace;
    SceneAuthoringDocument scene;
    EnvironmentAuthoringDocument environment_authoring;
    EnvironmentAuthoringInspectorModel environment_authoring_inspector;
    std::vector<EditorPropertyRow> inspector_rows;
    SourceAssetRegistryDocumentV1 asset_browser_source_registry;
    ContentBrowserState asset_browser_content_browser;
    AssetImportPlan asset_browser_import_plan;
    EditorAssetBrowserProductionModel asset_browser;
    std::vector<EditorAssetBrowserCommandPlan> asset_browser_command_plans;
    std::vector<EditorDiagnosticRow> console_rows;
    std::vector<NativeEditorEnvironmentArtistWorkflowCommandPlanRow> environment_artist_workflow_command_plans;
    EnvironmentArtistWorkflowExecutionReviewModel environment_artist_workflow_execution_review;
    EditorResourcePanelModel resources;
    EditorAiCommandPanelModel ai_commands;
    EditorRuntimeUiDocumentModel runtime_ui_document;
    EditorRuntimeUiThemeModel runtime_ui_theme;
    EditorRuntimeUiAuthoringModel runtime_ui_authoring;
    EditorProfilerPanelModel profiler;
    EditorTimelinePanelModel timeline;
    EditorMaterialAssetPreviewPanelModel material_preview;
    ViewportState viewport;
    NativeMaterialPreviewDisplayPlan material_preview_display;
    NativeViewportDisplayPlan viewport_display;
    NativeEditorTextAtlasHandoffEvidence text_atlas_handoff_evidence;
    NativeEditorTextInputState text_input_state;
    MemoryFileDialogService memory_file_dialog_service;
    MemoryClipboard memory_clipboard;
    NativeEditorClipboardTextAdapter clipboard_text_adapter;
    NativeEditorPlatformTextInputAdapter memory_text_input_adapter;
    NativeEditorImeAdapter memory_ime_adapter;
    RecordingProcessRunner recording_process_runner;
    IFileDialogService* file_dialog_service{nullptr};
    ui::IClipboardTextAdapter* clipboard_adapter{nullptr};
    IProcessRunner* process_runner{nullptr};
    IFileSystem* asset_import_filesystem{nullptr};
    ui::IPlatformIntegrationAdapter* platform_text_input_adapter{nullptr};
    ui::IImeAdapter* ime_adapter{nullptr};
    ui::IAccessibilityAdapter* accessibility_adapter{nullptr};
    NativeEditorUiaProviderState accessibility_state;
    NativeEditorServiceStatus service_status;
    std::string docking_status_last_frame{"not_rendered"};
    std::uint32_t dock_tab_headers_last_frame{0};
    std::uint32_t dock_split_gutters_last_frame{0};
    std::uint32_t dock_active_panels_last_frame{0};
    std::uint32_t dock_focusable_controls_last_frame{0};
};

NativeEditorApp::NativeEditorApp(NativeEditorLaunchOptions options)
    : options_{options}, impl_{std::make_unique<Impl>(options_)} {}

NativeEditorApp::~NativeEditorApp() = default;

NativeEditorApp::NativeEditorApp(NativeEditorApp&&) noexcept = default;

NativeEditorApp& NativeEditorApp::operator=(NativeEditorApp&&) noexcept = default;

const NativeEditorLaunchOptions& NativeEditorApp::options() const noexcept {
    return options_;
}

std::uint32_t NativeEditorApp::frames_recorded() const noexcept {
    return frames_recorded_;
}

std::uint32_t NativeEditorApp::native_panel_count() const noexcept {
    const auto catalog = editor_dock_panel_catalog();
    return static_cast<std::uint32_t>(std::ranges::count_if(
        catalog, [](const EditorDockPanelCatalogRow& panel) { return panel.native_shell_panel; }));
}

bool NativeEditorApp::has_native_panel(std::string_view id) const noexcept {
    const auto catalog = editor_dock_panel_catalog();
    const auto* panel = find_editor_dock_panel(catalog, id);
    return panel != nullptr && panel->native_shell_panel;
}

std::uint32_t NativeEditorApp::panels_rendered_last_frame() const noexcept {
    return panels_rendered_last_frame_;
}

std::string_view NativeEditorApp::docking_status_last_frame() const noexcept {
    return impl_->docking_status_last_frame;
}

std::uint32_t NativeEditorApp::dock_tab_headers_last_frame() const noexcept {
    return impl_->dock_tab_headers_last_frame;
}

std::uint32_t NativeEditorApp::dock_split_gutters_last_frame() const noexcept {
    return impl_->dock_split_gutters_last_frame;
}

std::uint32_t NativeEditorApp::dock_active_panels_last_frame() const noexcept {
    return impl_->dock_active_panels_last_frame;
}

std::uint32_t NativeEditorApp::dock_focusable_controls_last_frame() const noexcept {
    return impl_->dock_focusable_controls_last_frame;
}

const Workspace& NativeEditorApp::workspace() const noexcept {
    return impl_->workspace;
}

bool NativeEditorApp::is_panel_visible(PanelId id) const noexcept {
    return impl_->workspace.is_panel_visible(id);
}

void NativeEditorApp::set_panel_visible(PanelId id, bool visible) {
    impl_->workspace.set_panel_visible(id, visible);
}

const ProjectDocument& NativeEditorApp::project() const noexcept {
    return impl_->project;
}

const SceneAuthoringDocument& NativeEditorApp::scene_document() const noexcept {
    return impl_->scene;
}

const EnvironmentAuthoringDocument& NativeEditorApp::environment_authoring_document() const noexcept {
    return impl_->environment_authoring;
}

std::span<const EnvironmentAuthoringInspectorRow>
NativeEditorApp::environment_authoring_inspector_rows() const noexcept {
    return impl_->environment_authoring_inspector.rows;
}

std::span<const EditorPropertyRow> NativeEditorApp::inspector_rows() const noexcept {
    return impl_->inspector_rows;
}

const EditorAssetBrowserProductionModel& NativeEditorApp::asset_browser() const noexcept {
    return impl_->asset_browser;
}

std::span<const EditorAssetBrowserCommandPlan> NativeEditorApp::asset_browser_command_plans() const noexcept {
    return impl_->asset_browser_command_plans;
}

std::span<const EditorDiagnosticRow> NativeEditorApp::console_rows() const noexcept {
    return impl_->console_rows;
}

const EditorResourcePanelModel& NativeEditorApp::resources() const noexcept {
    return impl_->resources;
}

const EditorAiCommandPanelModel& NativeEditorApp::ai_commands() const noexcept {
    return impl_->ai_commands;
}

const EditorRuntimeUiDocumentModel& NativeEditorApp::runtime_ui_document() const noexcept {
    return impl_->runtime_ui_document;
}

const EditorRuntimeUiThemeModel& NativeEditorApp::runtime_ui_theme() const noexcept {
    return impl_->runtime_ui_theme;
}

const EditorRuntimeUiAuthoringModel& NativeEditorApp::runtime_ui_authoring() const noexcept {
    return impl_->runtime_ui_authoring;
}

std::span<const NativeEditorEnvironmentArtistWorkflowCommandPlanRow>
NativeEditorApp::environment_artist_workflow_command_plans() const noexcept {
    return impl_->environment_artist_workflow_command_plans;
}

const EnvironmentArtistWorkflowExecutionReviewModel&
NativeEditorApp::environment_artist_workflow_execution_review() const noexcept {
    return impl_->environment_artist_workflow_execution_review;
}

const EditorProfilerPanelModel& NativeEditorApp::profiler() const noexcept {
    return impl_->profiler;
}

const EditorTimelinePanelModel& NativeEditorApp::timeline() const noexcept {
    return impl_->timeline;
}

std::vector<ProjectSettingsError> NativeEditorApp::project_settings_errors() const {
    return ProjectSettingsDraft::from_project(impl_->project).validation_errors();
}

const NativeEditorServiceStatus& NativeEditorApp::services() const noexcept {
    return impl_->service_status;
}

const ViewportState& NativeEditorApp::viewport() const noexcept {
    return impl_->viewport;
}

const NativeViewportDisplayPlan& NativeEditorApp::viewport_display() const noexcept {
    return impl_->viewport_display;
}

const NativeEditorTextAtlasHandoffEvidence& NativeEditorApp::text_atlas_handoff_evidence() const noexcept {
    return impl_->text_atlas_handoff_evidence;
}

const NativeEditorTextInputState& NativeEditorApp::text_input_state() const noexcept {
    return impl_->text_input_state;
}

const NativeEditorUiaProviderState& NativeEditorApp::accessibility_state() const noexcept {
    return impl_->accessibility_state;
}

const EditorMaterialAssetPreviewPanelModel& NativeEditorApp::material_preview() const noexcept {
    return impl_->material_preview;
}

const NativeMaterialPreviewDisplayPlan& NativeEditorApp::material_preview_display() const noexcept {
    return impl_->material_preview_display;
}

void NativeEditorApp::bind_native_services(NativeEditorServiceBindings services) {
    if (services.file_dialog_service != nullptr) {
        impl_->file_dialog_service = services.file_dialog_service;
        impl_->service_status.file_dialog_service_id =
            services.file_dialog_service_id.empty() ? "external" : std::move(services.file_dialog_service_id);
        impl_->service_status.file_dialog_available = true;
    }
    if (services.clipboard_text_adapter != nullptr) {
        impl_->clipboard_adapter = services.clipboard_text_adapter;
        impl_->service_status.clipboard_service_id =
            services.clipboard_service_id.empty() ? "external" : std::move(services.clipboard_service_id);
        impl_->service_status.clipboard_available = true;
    }
    if (services.reviewed_process_runner != nullptr) {
        impl_->process_runner = services.reviewed_process_runner;
        impl_->service_status.reviewed_process_runner_id =
            services.reviewed_process_runner_id.empty() ? "external" : std::move(services.reviewed_process_runner_id);
        impl_->service_status.reviewed_process_runner_available = true;
    }
    if (services.platform_text_input_adapter != nullptr) {
        impl_->platform_text_input_adapter = services.platform_text_input_adapter;
        impl_->service_status.platform_text_input_service_id = services.platform_text_input_service_id.empty()
                                                                   ? "external"
                                                                   : std::move(services.platform_text_input_service_id);
        impl_->service_status.platform_text_input_available = true;
        impl_->text_input_state.tsf_adapter_selected =
            is_win32_tsf_service_id(impl_->service_status.platform_text_input_service_id);
    }
    if (services.ime_adapter != nullptr) {
        impl_->ime_adapter = services.ime_adapter;
        impl_->service_status.ime_service_id =
            services.ime_service_id.empty() ? "external" : std::move(services.ime_service_id);
        impl_->service_status.ime_available = true;
        impl_->text_input_state.tsf_adapter_selected = impl_->text_input_state.tsf_adapter_selected ||
                                                       is_win32_tsf_service_id(impl_->service_status.ime_service_id);
    }
    if (services.accessibility_adapter != nullptr) {
        impl_->accessibility_adapter = services.accessibility_adapter;
        impl_->service_status.accessibility_service_id =
            services.accessibility_service_id.empty() ? "external" : std::move(services.accessibility_service_id);
        impl_->service_status.accessibility_available = true;
    }
    if (services.asset_import_filesystem != nullptr) {
        impl_->asset_import_filesystem = services.asset_import_filesystem;
        impl_->service_status.asset_import_filesystem_id =
            services.asset_import_filesystem_id.empty() ? "external" : std::move(services.asset_import_filesystem_id);
        impl_->service_status.asset_import_filesystem_available = true;
    }
}

NativeEditorTextInputFocusResult NativeEditorApp::focus_text_input_target(NativeEditorTextInputTargetDesc target) {
    const auto plan = plan_native_editor_text_input_focus_change(impl_->text_input_state, target);
    NativeEditorTextInputFocusResult result;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }
    if (impl_->platform_text_input_adapter == nullptr) {
        impl_->service_status.platform_text_input_available = false;
        result.diagnostics.push_back(make_text_input_diagnostic(
            plan.request.target, ui::AdapterPayloadDiagnosticCode::invalid_platform_text_input_target,
            "native editor platform text input adapter is unavailable"));
        return result;
    }

    if (plan.end_previous_session) {
        const auto end_result = ui::end_platform_text_input(*impl_->platform_text_input_adapter, plan.previous_target);
        append_diagnostics(result.diagnostics, end_result.diagnostics);
        if (!end_result.succeeded()) {
            return result;
        }
        result.previous_session_ended = true;
        ++impl_->service_status.platform_text_input_sessions_ended;
    }
    if (plan.begin_session) {
        const auto begin_result = ui::begin_platform_text_input(*impl_->platform_text_input_adapter, plan.request);
        append_diagnostics(result.diagnostics, begin_result.diagnostics);
        if (!begin_result.succeeded()) {
            return result;
        }
        result.session_begun = true;
        ++impl_->service_status.platform_text_input_sessions_started;
    }

    impl_->text_input_state = make_native_editor_text_input_state(target);
    impl_->text_input_state.tsf_adapter_selected =
        is_win32_tsf_service_id(impl_->service_status.platform_text_input_service_id) ||
        is_win32_tsf_service_id(impl_->service_status.ime_service_id);
    impl_->text_input_state.session_active = true;
    result.accepted = true;
    return result;
}

NativeEditorTextInputEndResult NativeEditorApp::end_text_input_session() {
    NativeEditorTextInputEndResult result;
    if (!impl_->text_input_state.session_active) {
        result.accepted = true;
        return result;
    }
    if (impl_->platform_text_input_adapter == nullptr) {
        impl_->service_status.platform_text_input_available = false;
        result.diagnostics.push_back(
            make_text_input_diagnostic(impl_->text_input_state.edit_state.target,
                                       ui::AdapterPayloadDiagnosticCode::invalid_platform_text_input_target,
                                       "native editor platform text input adapter is unavailable"));
        return result;
    }

    const auto end_result =
        ui::end_platform_text_input(*impl_->platform_text_input_adapter, impl_->text_input_state.edit_state.target);
    result.diagnostics = end_result.diagnostics;
    if (!end_result.succeeded()) {
        return result;
    }

    impl_->text_input_state.session_active = false;
    impl_->text_input_state.composition_active = false;
    impl_->text_input_state.composition = ui::ImeComposition{.target = impl_->text_input_state.edit_state.target};
    result.session_ended = true;
    result.accepted = true;
    ++impl_->service_status.platform_text_input_sessions_ended;
    return result;
}

NativeEditorImeCompositionResult NativeEditorApp::update_ime_composition(ui::ImeComposition composition) {
    NativeEditorImeCompositionResult result;
    if (!impl_->text_input_state.session_active || composition.target != impl_->text_input_state.edit_state.target) {
        result.diagnostics.push_back(make_text_input_diagnostic(
            composition.target, ui::AdapterPayloadDiagnosticCode::invalid_ime_target,
            "native editor IME composition target must match the active text input session"));
        return result;
    }
    if (impl_->ime_adapter == nullptr) {
        impl_->service_status.ime_available = false;
        result.diagnostics.push_back(make_text_input_diagnostic(composition.target,
                                                                ui::AdapterPayloadDiagnosticCode::invalid_ime_target,
                                                                "native editor IME adapter is unavailable"));
        return result;
    }

    const auto publish_result = ui::publish_ime_composition(*impl_->ime_adapter, composition);
    result.diagnostics = publish_result.diagnostics;
    if (!publish_result.succeeded()) {
        return result;
    }

    impl_->text_input_state.composition = std::move(composition);
    impl_->text_input_state.composition_active = !impl_->text_input_state.composition.composition_text.empty();
    impl_->text_input_state.commit_applied = false;
    result.accepted = true;
    result.published = true;
    ++impl_->service_status.ime_composition_updates;
    return result;
}

NativeEditorImeCompositionResult NativeEditorApp::cancel_ime_composition() {
    return update_ime_composition(ui::ImeComposition{
        .target = impl_->text_input_state.edit_state.target,
        .composition_text = {},
        .cursor_index = 0U,
    });
}

NativeEditorTextInputCommitResult NativeEditorApp::commit_text_input(ui::CommittedTextInput input) {
    NativeEditorTextInputCommitResult result;
    result.state = impl_->text_input_state.edit_state;
    if (!impl_->text_input_state.session_active || input.target != impl_->text_input_state.edit_state.target) {
        result.diagnostics.push_back(
            make_text_input_diagnostic(input.target, ui::AdapterPayloadDiagnosticCode::mismatched_committed_text_target,
                                       "native editor committed text target must match the active text input session"));
        return result;
    }

    const auto commit_result = ui::apply_committed_text_input(impl_->text_input_state.edit_state, input);
    result.diagnostics = commit_result.diagnostics;
    result.state = commit_result.state;
    if (!commit_result.succeeded()) {
        return result;
    }

    impl_->text_input_state.edit_state = commit_result.state;
    impl_->text_input_state.surrounding_text = commit_result.state.text;
    impl_->text_input_state.composition = ui::ImeComposition{.target = commit_result.state.target};
    impl_->text_input_state.composition_active = false;
    impl_->text_input_state.commit_applied = true;
    result.accepted = true;
    result.committed = true;
    ++impl_->service_status.committed_text_inputs;
    return result;
}

ui::AccessibilityPublishResult
NativeEditorApp::publish_native_accessibility_payload(const ui::AccessibilityPayload& payload,
                                                      const ui::ElementId& focused) {
    impl_->accessibility_state = plan_native_editor_uia_provider_tree(payload, focused);
    ui::AccessibilityPublishResult result;
    result.diagnostics = impl_->accessibility_state.diagnostics;
    if (!result.diagnostics.empty()) {
        return result;
    }
    if (impl_->accessibility_adapter == nullptr) {
        impl_->service_status.accessibility_available = false;
        result.diagnostics.push_back(ui::AdapterPayloadDiagnostic{
            .id = focused,
            .code = ui::AdapterPayloadDiagnosticCode::invalid_accessibility_bounds,
            .message = "native editor accessibility adapter is unavailable",
        });
        return result;
    }

    if (auto* native_adapter = dynamic_cast<NativeEditorUiaAccessibilityAdapter*>(impl_->accessibility_adapter);
        native_adapter != nullptr) {
        native_adapter->set_focused_element(focused);
    }

    result = ui::publish_accessibility_payload(*impl_->accessibility_adapter, payload);
    if (result.succeeded()) {
        ++impl_->service_status.accessibility_publish_requests;
        if (const auto* native_adapter =
                dynamic_cast<const NativeEditorUiaAccessibilityAdapter*>(impl_->accessibility_adapter);
            native_adapter != nullptr) {
            impl_->accessibility_state = native_adapter->state();
        }
    }
    return result;
}

FileDialogId NativeEditorApp::show_file_dialog(FileDialogRequest request) {
    if (impl_->file_dialog_service == nullptr) {
        impl_->service_status.file_dialog_available = false;
        return 0;
    }
    ++impl_->service_status.file_dialog_requests_routed;
    return impl_->file_dialog_service->show(std::move(request));
}

std::optional<FileDialogResult> NativeEditorApp::poll_file_dialog_result(FileDialogId id) {
    if (impl_->file_dialog_service == nullptr) {
        impl_->service_status.file_dialog_available = false;
        return std::nullopt;
    }
    return impl_->file_dialog_service->poll_result(id);
}

FileDialogId NativeEditorApp::show_asset_browser_import_sources_dialog() {
    return show_file_dialog(make_content_browser_import_open_dialog_request(impl_->project.asset_root));
}

NativeEditorAssetBrowserImportSourcesDialogReview
NativeEditorApp::poll_asset_browser_import_sources_dialog(FileDialogId id) {
    NativeEditorAssetBrowserImportSourcesDialogReview review;
    const auto result = poll_file_dialog_result(id);
    if (!result.has_value()) {
        review.dialog = make_content_browser_import_open_dialog_model(FileDialogResult{
            .id = id == 0U ? 1U : id,
            .status = FileDialogStatus::failed,
            .error = "asset browser import source dialog result is unavailable",
        });
        review.diagnostics = review.dialog.diagnostics;
        return review;
    }

    if (result->status != FileDialogStatus::accepted) {
        review.dialog = make_content_browser_import_open_dialog_model(*result);
        review.diagnostics = review.dialog.diagnostics;
        review.accepted = review.dialog.accepted;
        return review;
    }

    FileDialogResult normalized_result = *result;
    normalized_result.paths.clear();
    normalized_result.paths.reserve(result->paths.size());
    for (const auto& path : result->paths) {
        std::string diagnostic;
        auto project_path = normalize_selected_import_path(impl_->project.root_path, path, diagnostic);
        if (!diagnostic.empty()) {
            review.dialog = make_content_browser_import_open_dialog_model(FileDialogResult{
                .id = result->id,
                .status = FileDialogStatus::failed,
                .paths = result->paths,
                .selected_filter = result->selected_filter,
                .error = diagnostic,
            });
            review.diagnostics = review.dialog.diagnostics;
            return review;
        }
        normalized_result.paths.push_back(std::move(project_path));
    }

    review.dialog = make_content_browser_import_open_dialog_model(normalized_result);
    review.diagnostics = review.dialog.diagnostics;
    review.accepted = review.dialog.accepted;
    if (review.accepted) {
        review.accepted_project_paths = review.dialog.selected_paths;
    }
    return review;
}

NativeEditorAssetBrowserExternalSourceCopyReview NativeEditorApp::review_asset_browser_external_source_copy(
    const NativeEditorAssetBrowserExternalSourceCopyRequest& request) const {
    std::vector<EditorContentBrowserImportExternalSourceCopyInput> inputs;
    inputs.reserve(request.source_paths.size());
    for (const auto& source_path : request.source_paths) {
        std::string diagnostic;
        const auto target_path = imported_source_target_path(impl_->project.asset_root, source_path, diagnostic);
        inputs.push_back(EditorContentBrowserImportExternalSourceCopyInput{
            .source_path = source_path,
            .target_project_path = target_path,
            .diagnostic = std::move(diagnostic),
            .source_exists = contains_string(request.existing_source_paths, source_path),
            .target_exists = contains_string(request.existing_project_paths, target_path),
        });
    }

    NativeEditorAssetBrowserExternalSourceCopyReview review;
    review.copy = make_content_browser_import_external_source_copy_model(inputs);
    review.diagnostics = review.copy.diagnostics;
    return review;
}

NativeEditorAssetBrowserImportExecutionResult
NativeEditorApp::execute_reviewed_asset_browser_import_plan(NativeEditorAssetBrowserImportExecutionRequest request) {
    NativeEditorAssetBrowserImportExecutionResult result;
    result.command = plan_editor_asset_browser_command(EditorAssetBrowserCommandRequest{
        .kind = EditorAssetBrowserCommandKind::execute_reviewed_import_plan,
        .mode = EditorAssetBrowserCommandMode::apply,
        .expected_generation = request.expected_generation,
        .current_generation = impl_->asset_browser.generation,
        .user_confirmed = request.user_confirmed,
    });

    if (result.command.status != EditorAssetBrowserCommandStatus::ready || !result.command.executes_import_tools) {
        result.diagnostic = result.command.diagnostics.empty()
                                ? "asset browser import execution requires a reviewed ready command"
                                : result.command.diagnostics.front();
        return result;
    }
    if (impl_->asset_import_filesystem == nullptr) {
        impl_->service_status.asset_import_filesystem_available = false;
        result.diagnostic = "asset browser import execution requires an asset import filesystem service";
        return result;
    }

    ExternalAssetImportAdapters adapters;
    const auto import_result = execute_asset_import_plan(*impl_->asset_import_filesystem,
                                                         impl_->asset_browser_import_plan, adapters.options());
    result.executed = true;
    result.import_tools_invoked = true;
    result.imported_count = import_result.imported.size();
    result.import_failure_count = import_result.failures.size();
    result.diagnostic = import_result.failures.empty() ? "asset browser import execution succeeded"
                                                       : import_result.failures.front().diagnostic;
    ++impl_->service_status.asset_import_executions;
    return result;
}

ui::ClipboardTextWriteResult NativeEditorApp::write_clipboard_text(ui::ClipboardTextWriteRequest request) {
    if (impl_->clipboard_adapter == nullptr) {
        impl_->service_status.clipboard_available = false;
        ui::ClipboardTextWriteResult result;
        result.diagnostics.push_back(ui::AdapterPayloadDiagnostic{
            .id = request.target,
            .code = ui::AdapterPayloadDiagnosticCode::invalid_clipboard_text,
            .message = "native editor clipboard service is unavailable",
        });
        return result;
    }

    auto result = ui::write_clipboard_text(*impl_->clipboard_adapter, request);
    if (result.succeeded()) {
        ++impl_->service_status.clipboard_operations_routed;
    }
    return result;
}

ui::ClipboardTextReadResult NativeEditorApp::read_clipboard_text(ui::ClipboardTextReadRequest request) {
    if (impl_->clipboard_adapter == nullptr) {
        impl_->service_status.clipboard_available = false;
        ui::ClipboardTextReadResult result;
        result.diagnostics.push_back(ui::AdapterPayloadDiagnostic{
            .id = request.target,
            .code = ui::AdapterPayloadDiagnosticCode::invalid_clipboard_text_result,
            .message = "native editor clipboard service is unavailable",
        });
        return result;
    }

    auto result = ui::read_clipboard_text(*impl_->clipboard_adapter, request);
    if (result.succeeded()) {
        ++impl_->service_status.clipboard_operations_routed;
    }
    return result;
}

EditorAiReviewedValidationExecutionModel
NativeEditorApp::reviewed_validation_execution_plan(const EditorAiReviewedValidationExecutionDesc& desc) {
    auto model = make_editor_ai_reviewed_validation_execution_plan(desc);
    ++impl_->service_status.reviewed_process_plans;
    impl_->service_status.reviewed_process_status_label = model.status_label;
    return model;
}

NativeEditorReviewedProcessResult NativeEditorApp::run_reviewed_process(NativeEditorReviewedProcessRequest request) {
    NativeEditorReviewedProcessResult result{
        .reviewed = request.plan.can_execute && is_allowed_process_command(request.plan.command),
        .user_confirmed = request.user_confirmed,
        .executed = false,
        .process = {},
        .diagnostic = {},
    };

    if (!result.reviewed) {
        result.diagnostic = "reviewed process execution requires a ready allowlisted command";
        impl_->service_status.reviewed_process_status_label = "blocked";
        return result;
    }
    if (!request.user_confirmed) {
        result.diagnostic = "reviewed process execution requires user confirmation before launch";
        impl_->service_status.reviewed_process_status_label = "confirmation required";
        return result;
    }
    if (impl_->process_runner == nullptr) {
        impl_->service_status.reviewed_process_runner_available = false;
        result.diagnostic = "reviewed process runner is unavailable";
        return result;
    }

    result.process = run_process_command(*impl_->process_runner, request.plan.command);
    result.executed = result.process.launched;
    ++impl_->service_status.reviewed_process_executions;
    impl_->service_status.reviewed_process_status_label = result.process.succeeded() ? "executed" : "failed";
    return result;
}

int NativeEditorApp::run() {
    return 0;
}

void NativeEditorApp::record_native_frame() noexcept {
    ++frames_recorded_;
}

void NativeEditorApp::record_native_panels_rendered(std::uint32_t count) noexcept {
    panels_rendered_last_frame_ = count;
}

void NativeEditorApp::record_native_docking_frame(std::string status, std::uint32_t tab_header_count,
                                                  std::uint32_t split_gutter_count, std::uint32_t active_panel_count,
                                                  std::uint32_t focusable_control_count) {
    impl_->docking_status_last_frame = std::move(status);
    impl_->dock_tab_headers_last_frame = tab_header_count;
    impl_->dock_split_gutters_last_frame = split_gutter_count;
    impl_->dock_active_panels_last_frame = active_panel_count;
    impl_->dock_focusable_controls_last_frame = focusable_control_count;
}

void NativeEditorApp::record_native_resource_device_ready(std::uint64_t frame_index) {
    impl_->resources = make_native_resource_panel_model(true, frame_index);
}

void NativeEditorApp::record_native_viewport_d3d12_host_ready(std::uint64_t frame_index) {
    const ViewportExtent extent{.width = options_.width, .height = options_.height};
    impl_->viewport.set_renderer("d3d12");
    impl_->viewport.resize(extent);
    impl_->viewport.mark_frame_rendered();
    impl_->viewport_display = plan_native_viewport_display(NativeViewportDisplayDesc{
        .d3d12_host_available = true,
        .renderer_output_available = false,
        .extent = extent,
        .frame_index = frame_index,
        .backend_id = "d3d12",
    });
}

void NativeEditorApp::record_native_material_preview_d3d12_host_ready(std::uint64_t frame_index) {
    impl_->material_preview_display = plan_native_material_preview_display(NativeMaterialPreviewDisplayDesc{
        .d3d12_host_available = true,
        .shader_artifacts_available = impl_->material_preview.required_shader_ready,
        .gpu_payload_available = impl_->material_preview.gpu_payload_ready,
        .frame_index = frame_index,
        .backend_id = "d3d12",
    });
    apply_editor_material_gpu_preview_execution_snapshot(impl_->material_preview,
                                                         impl_->material_preview_display.execution_snapshot);
}

void NativeEditorApp::record_native_viewport_texture_display(NativeViewportDisplayPlan plan) {
    impl_->viewport.set_renderer(plan.backend_id);
    impl_->viewport.resize(plan.extent);
    if (plan.texture_display_ready) {
        impl_->viewport.mark_frame_rendered();
    }
    impl_->viewport_display = std::move(plan);
}

void NativeEditorApp::record_native_material_preview_texture_display(NativeMaterialPreviewDisplayPlan plan) {
    impl_->material_preview_display = std::move(plan);
    apply_editor_material_gpu_preview_execution_snapshot(impl_->material_preview,
                                                         impl_->material_preview_display.execution_snapshot);
}

void NativeEditorApp::record_native_text_atlas_handoff_evidence(NativeEditorTextAtlasHandoffEvidence evidence) {
    impl_->text_atlas_handoff_evidence = std::move(evidence);
}

} // namespace mirakana::editor
