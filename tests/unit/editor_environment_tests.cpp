// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "first_party_editor_document.hpp"
#include "native_editor_app.hpp"

#include "mirakana/editor/environment_authoring.hpp"
#include "mirakana/editor/history.hpp"
#include "mirakana/environment/environment_io.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::EnvironmentProfileDesc make_editor_environment_profile() {
    mirakana::EnvironmentProfileDesc profile;
    profile.id = "outdoor_storm";
    profile.sky_model = mirakana::EnvironmentSkyModel::physical_atmosphere;
    profile.weather = mirakana::EnvironmentWeatherKind::storm;
    profile.sun.direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    profile.sun.illuminance_lux = 85000.0F;
    profile.moon.illuminance_lux = 0.4F;
    profile.fog.enabled = true;
    profile.fog.density = 0.25F;
    profile.precipitation = mirakana::EnvironmentPrecipitationDesc{
        .kind = mirakana::EnvironmentPrecipitationKind::rain,
        .intensity = 0.8F,
        .particle_radius_mm = 0.8F,
        .fall_speed_mps = 7.0F,
        .wind_speed_mps = 5.0F,
    };
    return profile;
}

[[nodiscard]] mirakana::EnvironmentProfileDocumentV2 make_editor_environment_profile_v2() {
    mirakana::EnvironmentProfileDocumentV2 document;
    document.global_profile = make_editor_environment_profile();
    document.quality_preset = mirakana::EnvironmentQualityPreset::high;
    document.volumes.push_back(mirakana::EnvironmentVolumeDesc{
        .id = "storm_front",
        .shape = mirakana::EnvironmentVolumeShape::box,
        .center_m = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 8.0F},
        .extents_m = mirakana::Vec3{.x = 16.0F, .y = 6.0F, .z = 20.0F},
        .priority = 20,
        .blend_weight = 0.75F,
        .fade_distance_m = 12.0F,
        .blend_mode = mirakana::EnvironmentVolumeBlendMode::weighted_override,
        .profile = make_editor_environment_profile(),
    });
    document.weather_timeline.push_back(mirakana::EnvironmentWeatherKeyframeDesc{
        .time_of_day_hours = 6.0F,
        .weather = mirakana::EnvironmentWeatherKind::cloudy,
        .precipitation = mirakana::EnvironmentPrecipitationKind::none,
        .storm_intensity = 0.1F,
        .cloud_coverage = 0.55F,
        .fog_density = 0.08F,
        .quality_preset = mirakana::EnvironmentQualityPreset::medium,
    });
    document.weather_timeline.push_back(mirakana::EnvironmentWeatherKeyframeDesc{
        .time_of_day_hours = 18.0F,
        .weather = mirakana::EnvironmentWeatherKind::storm,
        .precipitation = mirakana::EnvironmentPrecipitationKind::rain,
        .storm_intensity = 0.9F,
        .cloud_coverage = 0.95F,
        .fog_density = 0.25F,
        .quality_preset = mirakana::EnvironmentQualityPreset::high,
    });
    return document;
}

[[nodiscard]] const mirakana::editor::EnvironmentAuthoringInspectorRow*
find_environment_row(const mirakana::editor::EnvironmentAuthoringInspectorModel& model, std::string_view id) noexcept {
    const auto it = std::ranges::find_if(
        model.rows, [id](const mirakana::editor::EnvironmentAuthoringInspectorRow& row) { return row.id == id; });
    return it == model.rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::EnvironmentAuthoringDiagnosticRow*
find_environment_diagnostic(const mirakana::editor::EnvironmentAuthoringValidationModel& model,
                            mirakana::EnvironmentProfileDiagnosticCode code) noexcept {
    const auto it =
        std::ranges::find_if(model.diagnostics, [code](const mirakana::editor::EnvironmentAuthoringDiagnosticRow& row) {
            return row.profile_code == code;
        });
    return it == model.diagnostics.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::EnvironmentSettingsWorkflowSectionRow*
find_environment_settings_section(const mirakana::editor::EnvironmentSettingsWorkflowModel& model,
                                  std::string_view id) noexcept {
    const auto it = std::ranges::find_if(
        model.sections,
        [id](const mirakana::editor::EnvironmentSettingsWorkflowSectionRow& section) { return section.id == id; });
    return it == model.sections.end() ? nullptr : &(*it);
}

[[nodiscard]] bool contains_element(const mirakana::ui::UiDocument& document, std::string_view id) {
    return document.find(mirakana::ui::ElementId{.value = std::string{id}}) != nullptr;
}

} // namespace

MK_TEST("editor environment authoring emits deterministic inspector rows") {
    auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile_document_v2(
        make_editor_environment_profile_v2(), "assets/environment/outdoor.environment");

    const auto model = mirakana::editor::make_environment_authoring_inspector_model(
        mirakana::editor::EnvironmentAuthoringInspectorDesc{
            .document = document,
            .cloud_layer =
                mirakana::EnvironmentCloudLayerDesc{
                    .mode = mirakana::EnvironmentCloudLayerMode::equirectangular_2d,
                    .coverage = 0.7F,
                    .opacity = 0.85F,
                    .altitude_m = 2200.0F,
                    .wind_velocity_mps = mirakana::Vec2{.x = 4.0F, .y = 1.0F},
                    .cloud_map_asset_ref = "textures/clouds/storm_latlong",
                    .flow_map_asset_ref = "textures/clouds/storm_flow",
                },
            .volumetric_clouds_policy_available = true,
        });

    MK_REQUIRE(model.status == mirakana::editor::EnvironmentAuthoringStatus::ready);
    MK_REQUIRE(model.profile_id == "outdoor_storm");
    MK_REQUIRE(model.path == "assets/environment/outdoor.environment");
    MK_REQUIRE(!model.dirty);
    MK_REQUIRE(!model.invokes_backend);
    MK_REQUIRE(!model.exposes_native_handles);
    MK_REQUIRE(!model.executes_package_scripts);
    MK_REQUIRE(model.rows.size() >= 10U);

    MK_REQUIRE(find_environment_row(model, "environment.sky.model")->value == "physical_atmosphere");
    MK_REQUIRE(find_environment_row(model, "environment.sun.direction")->value == "0,-1,0");
    MK_REQUIRE(find_environment_row(model, "environment.moon.illuminance_lux")->value == "0.4");
    MK_REQUIRE(find_environment_row(model, "environment.atmosphere.planet_radius_km")->value == "6360");
    MK_REQUIRE(find_environment_row(model, "environment.fog.density")->value == "0.25");
    MK_REQUIRE(find_environment_row(model, "environment.cloud_layer.mode")->value == "equirectangular_2d");
    MK_REQUIRE(find_environment_row(model, "environment.volumetric_clouds.status")->value == "policy_available");
    MK_REQUIRE(find_environment_row(model, "environment.precipitation.kind")->value == "rain");
    MK_REQUIRE(find_environment_row(model, "environment.weather.preset")->value == "storm");
    MK_REQUIRE(find_environment_row(model, "environment.quality.tier")->value == "high");
    MK_REQUIRE(find_environment_row(model, "environment.profile_v2.volume_count")->value == "1");
    MK_REQUIRE(find_environment_row(model, "environment.profile_v2.weather_keyframes")->value == "2");
    MK_REQUIRE(find_environment_row(model, "environment.volume.0.id")->value == "storm_front");
    MK_REQUIRE(find_environment_row(model, "environment.volume.0.shape")->value == "box");
    MK_REQUIRE(find_environment_row(model, "environment.volume.0.priority")->value == "20");
    MK_REQUIRE(find_environment_row(model, "environment.weather_keyframe.1.weather")->value == "storm");
    MK_REQUIRE(find_environment_row(model, "environment.weather_keyframe.1.quality_preset")->value == "high");
    MK_REQUIRE(find_environment_row(model, "environment.capture.cubemap.request_status")->value == "available");

    const auto ui = mirakana::editor::make_environment_authoring_ui_model(model);
    MK_REQUIRE(contains_element(ui, "environment_authoring.inspector.rows.environment.sky.model.value"));
    MK_REQUIRE(contains_element(ui, "environment_authoring.inspector.rows.environment.quality.tier.value"));
}

MK_TEST("editor environment settings workflow exposes stable section tabs and package summary") {
    auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile_document_v2(
        make_editor_environment_profile_v2(), "assets/environment/outdoor.environment");

    const std::array existing_runtime_files = {std::string{"runtime/environment/outdoor.geindex"}};
    const std::array validation_recipe_ids = {std::string{"desktop-runtime-sample-game-environment-profile-v2"},
                                              std::string{"desktop-runtime-sample-game-environment-package-review"}};

    const auto model =
        mirakana::editor::make_environment_settings_workflow_model(mirakana::editor::EnvironmentSettingsWorkflowDesc{
            .inspector =
                mirakana::editor::EnvironmentAuthoringInspectorDesc{
                    .document = document,
                    .cloud_layer =
                        mirakana::EnvironmentCloudLayerDesc{
                            .mode = mirakana::EnvironmentCloudLayerMode::equirectangular_2d,
                            .coverage = 0.7F,
                            .opacity = 0.85F,
                            .altitude_m = 2200.0F,
                            .wind_velocity_mps = mirakana::Vec2{.x = 4.0F, .y = 1.0F},
                            .cloud_map_asset_ref = "textures/clouds/storm_latlong",
                            .flow_map_asset_ref = "textures/clouds/storm_flow",
                        },
                    .volumetric_clouds_policy_available = true,
                },
            .cooked_profile_path = "runtime/environment/outdoor.geenv",
            .package_index_path = "runtime/environment/outdoor.geindex",
            .project_root_path = "games/sample",
            .existing_runtime_files = existing_runtime_files,
            .validation_recipe_ids = validation_recipe_ids,
        });

    MK_REQUIRE(model.surface_id == "environment_settings");
    MK_REQUIRE(model.status == mirakana::editor::EnvironmentAuthoringStatus::ready);
    MK_REQUIRE(model.profile_id == "outdoor_storm");
    MK_REQUIRE(model.path == "assets/environment/outdoor.environment");
    MK_REQUIRE(!model.dirty);
    MK_REQUIRE(!model.invokes_backend);
    MK_REQUIRE(!model.exposes_native_handles);
    MK_REQUIRE(!model.executes_package_scripts);
    MK_REQUIRE(model.rows.size() >= 30U);
    MK_REQUIRE(model.package_draft_rows.size() == 3U);
    MK_REQUIRE(model.validation_recipe_ids.size() == validation_recipe_ids.size());
    MK_REQUIRE(model.validation_recipe_ids[0] == "desktop-runtime-sample-game-environment-profile-v2");
    MK_REQUIRE(std::ranges::any_of(model.rows, [](const mirakana::editor::EnvironmentAuthoringInspectorRow& row) {
        return row.id == "environment.profile_v2.volume_count" && row.value == "1" && !row.editable;
    }));
    MK_REQUIRE(std::ranges::any_of(model.rows, [](const mirakana::editor::EnvironmentAuthoringInspectorRow& row) {
        return row.id == "environment.profile_v2.weather_keyframes" && row.value == "2" && !row.editable;
    }));
    MK_REQUIRE(std::ranges::any_of(model.rows, [](const mirakana::editor::EnvironmentAuthoringInspectorRow& row) {
        return row.id == "environment.quality.tier" && row.value == "high" && row.editable;
    }));
    MK_REQUIRE(std::ranges::any_of(model.rows, [](const mirakana::editor::EnvironmentAuthoringInspectorRow& row) {
        return row.id == "environment.readiness.unsupported.environment_ready" && row.value == "unclaimed" &&
               !row.editable;
    }));

    const auto* global = find_environment_settings_section(model, "environment_settings.global");
    const auto* volumes = find_environment_settings_section(model, "environment_settings.volumes");
    const auto* weather = find_environment_settings_section(model, "environment_settings.weather");
    const auto* quality = find_environment_settings_section(model, "environment_settings.quality");
    const auto* preview = find_environment_settings_section(model, "environment_settings.preview");
    const auto* package = find_environment_settings_section(model, "environment_settings.package");
    const auto* readiness = find_environment_settings_section(model, "environment_settings.readiness");
    MK_REQUIRE(global != nullptr);
    MK_REQUIRE(volumes != nullptr);
    MK_REQUIRE(weather != nullptr);
    MK_REQUIRE(quality != nullptr);
    MK_REQUIRE(preview != nullptr);
    MK_REQUIRE(package != nullptr);
    MK_REQUIRE(readiness != nullptr);

    MK_REQUIRE(global->editable_row_count > 0U);
    MK_REQUIRE(volumes->row_count >= 6U);
    MK_REQUIRE(weather->row_count >= 9U);
    MK_REQUIRE(quality->row_count >= 1U);
    MK_REQUIRE(preview->read_only_row_count == 1U);
    MK_REQUIRE(package->row_count == 3U);
    MK_REQUIRE(readiness->read_only_row_count >= 10U);
}

MK_TEST("editor environment authoring exposes readiness and unsupported claim rows") {
    auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile_document_v2(
        make_editor_environment_profile_v2(), "assets/environment/outdoor.environment");

    const auto model = mirakana::editor::make_environment_authoring_inspector_model(
        mirakana::editor::EnvironmentAuthoringInspectorDesc{
            .document = document,
            .cloud_layer =
                mirakana::EnvironmentCloudLayerDesc{
                    .mode = mirakana::EnvironmentCloudLayerMode::equirectangular_2d,
                    .coverage = 0.7F,
                    .opacity = 0.85F,
                    .altitude_m = 2200.0F,
                    .wind_velocity_mps = mirakana::Vec2{.x = 4.0F, .y = 1.0F},
                    .cloud_map_asset_ref = "textures/clouds/storm_latlong",
                    .flow_map_asset_ref = "textures/clouds/storm_flow",
                },
            .volumetric_clouds_policy_available = true,
        });

    const auto* physical_sky = find_environment_row(model, "environment.readiness.physical_sky.package_status");
    const auto* height_fog = find_environment_row(model, "environment.readiness.height_fog.status");
    const auto* volumetric_fog = find_environment_row(model, "environment.readiness.volumetric_fog.package_status");
    const auto* cloud_layer = find_environment_row(model, "environment.readiness.cloud_layer.renderer_status");
    const auto* volumetric_clouds =
        find_environment_row(model, "environment.readiness.volumetric_clouds.renderer_status");
    const auto* rain = find_environment_row(model, "environment.readiness.precipitation.rain_renderer_status");
    const auto* snow = find_environment_row(model, "environment.readiness.precipitation.snow_renderer_status");
    const auto* ibl = find_environment_row(model, "environment.readiness.ibl.package_status");
    const auto* d3d12 = find_environment_row(model, "environment.readiness.backend.d3d12_status");
    const auto* vulkan = find_environment_row(model, "environment.readiness.backend.vulkan_status");
    const auto* metal = find_environment_row(model, "environment.readiness.backend.metal_status");
    const auto* unsupported_ready = find_environment_row(model, "environment.readiness.unsupported.environment_ready");
    const auto* unsupported_native_handles =
        find_environment_row(model, "environment.readiness.unsupported.public_backend_handles");

    MK_REQUIRE(physical_sky != nullptr);
    MK_REQUIRE(height_fog != nullptr);
    MK_REQUIRE(volumetric_fog != nullptr);
    MK_REQUIRE(cloud_layer != nullptr);
    MK_REQUIRE(volumetric_clouds != nullptr);
    MK_REQUIRE(rain != nullptr);
    MK_REQUIRE(snow != nullptr);
    MK_REQUIRE(ibl != nullptr);
    MK_REQUIRE(d3d12 != nullptr);
    MK_REQUIRE(vulkan != nullptr);
    MK_REQUIRE(metal != nullptr);
    MK_REQUIRE(unsupported_ready != nullptr);
    MK_REQUIRE(unsupported_native_handles != nullptr);

    MK_REQUIRE(physical_sky->value == "ready");
    MK_REQUIRE(height_fog->value == "ready");
    MK_REQUIRE(volumetric_fog->value == "ready");
    MK_REQUIRE(cloud_layer->value == "ready");
    MK_REQUIRE(volumetric_clouds->value == "ready");
    MK_REQUIRE(rain->value == "ready");
    MK_REQUIRE(snow->value == "ready");
    MK_REQUIRE(ibl->value == "ready");
    MK_REQUIRE(d3d12->value == "ready");
    MK_REQUIRE(vulkan->value == "host_gated");
    MK_REQUIRE(metal->value == "host_gated");
    MK_REQUIRE(unsupported_ready->value == "unclaimed");
    MK_REQUIRE(unsupported_native_handles->value == "unsupported");

    MK_REQUIRE(!physical_sky->editable);
    MK_REQUIRE(!unsupported_ready->editable);
    MK_REQUIRE(!model.invokes_backend);
    MK_REQUIRE(!model.exposes_native_handles);
    MK_REQUIRE(!model.executes_package_scripts);

    const auto ui = mirakana::editor::make_environment_authoring_ui_model(model);
    MK_REQUIRE(contains_element(
        ui, "environment_authoring.inspector.rows.environment.readiness.physical_sky.package_status.value"));
    MK_REQUIRE(
        contains_element(ui, "environment_authoring.inspector.rows.environment.readiness.backend.metal_status.value"));
    MK_REQUIRE(contains_element(
        ui, "environment_authoring.inspector.rows.environment.readiness.unsupported.environment_ready.value"));
}

MK_TEST("editor environment authoring document uses text store undo dirty and validation rows") {
    constexpr std::string_view path{"assets/environment/outdoor.environment"};
    mirakana::editor::MemoryTextStore store;
    store.write_text(path, mirakana::serialize_environment_profile_v2(make_editor_environment_profile_v2()));

    auto document = mirakana::editor::load_environment_authoring_document(store, path);
    MK_REQUIRE(document.path() == path);
    MK_REQUIRE(document.profile().id == "outdoor_storm");
    MK_REQUIRE(!document.dirty());
    MK_REQUIRE(document.revision() == document.saved_revision());

    auto invalid_profile = document.profile();
    invalid_profile.id = "native/d3d12/editor";

    mirakana::editor::UndoStack undo_stack;
    MK_REQUIRE(undo_stack.execute(
        mirakana::editor::make_environment_authoring_profile_edit_action(document, invalid_profile)));
    MK_REQUIRE(document.profile().id == "native/d3d12/editor");
    MK_REQUIRE(document.dirty());
    MK_REQUIRE(document.revision() != document.saved_revision());

    const auto invalid_validation = mirakana::editor::make_environment_authoring_validation_model(document);
    MK_REQUIRE(invalid_validation.status == mirakana::editor::EnvironmentAuthoringStatus::blocked);
    MK_REQUIRE(find_environment_diagnostic(invalid_validation,
                                           mirakana::EnvironmentProfileDiagnosticCode::forbidden_profile_id_token) !=
               nullptr);

    MK_REQUIRE(undo_stack.undo());
    MK_REQUIRE(document.profile().id == "outdoor_storm");
    MK_REQUIRE(!document.dirty());

    MK_REQUIRE(undo_stack.redo());
    MK_REQUIRE(document.dirty());
    invalid_profile.id = "outdoor_clear";
    invalid_profile.weather = mirakana::EnvironmentWeatherKind::clear;
    MK_REQUIRE(undo_stack.execute(mirakana::editor::make_environment_authoring_profile_edit_action(
        document, invalid_profile, "Set Clear Environment")));
    mirakana::editor::save_environment_authoring_document(store, path, document);
    MK_REQUIRE(!document.dirty());
    MK_REQUIRE(store.read_text(path).contains("format=GameEngine.EnvironmentProfile.v2\n"));
    MK_REQUIRE(store.read_text(path).contains("global.id=outdoor_clear\n"));
    MK_REQUIRE(store.read_text(path).contains("global.weather.kind=clear\n"));
    MK_REQUIRE(store.read_text(path).contains("weather_keyframe.1.weather=storm\n"));
}

MK_TEST("editor environment authoring plans and applies reviewed profile v2 commands") {
    auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile_document_v2(
        make_editor_environment_profile_v2(), "assets/environment/outdoor.environment");

    auto new_volume = document.profile_document_v2().volumes.front();
    new_volume.id = "snow_ridge";
    new_volume.shape = mirakana::EnvironmentVolumeShape::sphere;
    new_volume.radius_m = 24.0F;
    new_volume.priority = 40;
    const auto add_plan = mirakana::editor::plan_environment_authoring_command(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::add_volume,
                      .volume = new_volume,
                      .label = "Add Snow Ridge Volume",
                  });
    MK_REQUIRE(add_plan.status == mirakana::editor::EnvironmentAuthoringCommandStatus::accepted);
    MK_REQUIRE(add_plan.command_id == "environment.command.volume.add");
    MK_REQUIRE(add_plan.mutates_document);
    MK_REQUIRE(!add_plan.invokes_backend);
    MK_REQUIRE(!add_plan.executes_package_scripts);
    MK_REQUIRE(!add_plan.exposes_native_handles);

    const auto duplicate_volume_plan = mirakana::editor::plan_environment_authoring_command(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::add_volume,
                      .volume = document.profile_document_v2().volumes.front(),
                  });
    MK_REQUIRE(duplicate_volume_plan.status ==
               mirakana::editor::EnvironmentAuthoringCommandStatus::rejected_invalid_request);
    MK_REQUIRE(!duplicate_volume_plan.diagnostics.empty());

    mirakana::editor::UndoStack undo_stack;
    MK_REQUIRE(undo_stack.execute(mirakana::editor::make_environment_authoring_command_action(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::add_volume,
                      .volume = new_volume,
                      .label = "Add Snow Ridge Volume",
                  })));
    MK_REQUIRE(document.profile_document_v2().volumes.size() == 2U);
    MK_REQUIRE(document.profile_document_v2().volumes[1].id == "snow_ridge");
    MK_REQUIRE(document.dirty());

    MK_REQUIRE(undo_stack.undo());
    MK_REQUIRE(document.profile_document_v2().volumes.size() == 1U);
    MK_REQUIRE(!document.dirty());

    MK_REQUIRE(undo_stack.redo());
    const auto reorder_plan = mirakana::editor::plan_environment_authoring_command(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::reorder_volume,
                      .source_index = 1U,
                      .target_index = 0U,
                      .label = "Move Snow Ridge Volume",
                  });
    MK_REQUIRE(reorder_plan.status == mirakana::editor::EnvironmentAuthoringCommandStatus::accepted);
    const auto noop_reorder_plan = mirakana::editor::plan_environment_authoring_command(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::reorder_volume,
                      .source_index = 1U,
                      .target_index = 1U,
                  });
    MK_REQUIRE(noop_reorder_plan.status ==
               mirakana::editor::EnvironmentAuthoringCommandStatus::rejected_invalid_request);
    MK_REQUIRE(!noop_reorder_plan.mutates_document);
    MK_REQUIRE(undo_stack.execute(mirakana::editor::make_environment_authoring_command_action(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::reorder_volume,
                      .source_index = 1U,
                      .target_index = 0U,
                      .label = "Move Snow Ridge Volume",
                  })));
    MK_REQUIRE(document.profile_document_v2().volumes[0].id == "snow_ridge");

    auto edited_volume = document.profile_document_v2().volumes[1];
    edited_volume.priority = 55;
    edited_volume.blend_weight = 0.35F;
    const auto edit_volume_plan = mirakana::editor::plan_environment_authoring_command(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::edit_volume,
                      .volume = edited_volume,
                      .volume_id = "storm_front",
                      .label = "Edit Storm Front Volume",
                  });
    MK_REQUIRE(edit_volume_plan.status == mirakana::editor::EnvironmentAuthoringCommandStatus::accepted);
    MK_REQUIRE(edit_volume_plan.command_id == "environment.command.volume.edit");
    MK_REQUIRE(edit_volume_plan.mutates_document);
    MK_REQUIRE(undo_stack.execute(mirakana::editor::make_environment_authoring_command_action(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::edit_volume,
                      .volume = edited_volume,
                      .volume_id = "storm_front",
                      .label = "Edit Storm Front Volume",
                  })));
    MK_REQUIRE(document.profile_document_v2().volumes[1].priority == 55);
    MK_REQUIRE(document.profile_document_v2().volumes[1].blend_weight == 0.35F);

    auto evening = document.profile_document_v2().weather_timeline[1];
    evening.weather = mirakana::EnvironmentWeatherKind::snow;
    evening.precipitation = mirakana::EnvironmentPrecipitationKind::snow;
    evening.quality_preset = mirakana::EnvironmentQualityPreset::ultra;
    MK_REQUIRE(undo_stack.execute(mirakana::editor::make_environment_authoring_command_action(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::edit_weather_keyframe,
                      .weather_keyframe_index = 1U,
                      .weather_keyframe = evening,
                      .label = "Edit Evening Weather",
                  })));
    MK_REQUIRE(document.profile_document_v2().weather_timeline[1].weather == mirakana::EnvironmentWeatherKind::snow);
    MK_REQUIRE(document.profile_document_v2().weather_timeline[1].quality_preset ==
               mirakana::EnvironmentQualityPreset::ultra);

    auto noon = document.profile_document_v2().weather_timeline[0];
    noon.time_of_day_hours = 12.0F;
    noon.weather = mirakana::EnvironmentWeatherKind::clear;
    noon.precipitation = mirakana::EnvironmentPrecipitationKind::none;
    const auto add_weather_plan = mirakana::editor::plan_environment_authoring_command(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::add_weather_keyframe,
                      .weather_keyframe = noon,
                      .label = "Add Noon Weather",
                  });
    MK_REQUIRE(add_weather_plan.status == mirakana::editor::EnvironmentAuthoringCommandStatus::accepted);
    MK_REQUIRE(add_weather_plan.command_id == "environment.command.weather_keyframe.add");
    MK_REQUIRE(add_weather_plan.mutates_document);
    MK_REQUIRE(undo_stack.execute(mirakana::editor::make_environment_authoring_command_action(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::add_weather_keyframe,
                      .weather_keyframe = noon,
                      .label = "Add Noon Weather",
                  })));
    MK_REQUIRE(document.profile_document_v2().weather_timeline.size() == 3U);

    const auto reorder_weather_plan = mirakana::editor::plan_environment_authoring_command(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::reorder_weather_keyframe,
                      .source_index = 2U,
                      .target_index = 0U,
                      .label = "Move Noon Weather",
                  });
    MK_REQUIRE(reorder_weather_plan.status == mirakana::editor::EnvironmentAuthoringCommandStatus::accepted);
    MK_REQUIRE(reorder_weather_plan.command_id == "environment.command.weather_keyframe.reorder");
    MK_REQUIRE(undo_stack.execute(mirakana::editor::make_environment_authoring_command_action(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::reorder_weather_keyframe,
                      .source_index = 2U,
                      .target_index = 0U,
                      .label = "Move Noon Weather",
                  })));
    MK_REQUIRE(document.profile_document_v2().weather_timeline[0].time_of_day_hours == 12.0F);

    const auto remove_weather_plan = mirakana::editor::plan_environment_authoring_command(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::remove_weather_keyframe,
                      .weather_keyframe_index = 0U,
                      .label = "Remove Noon Weather",
                  });
    MK_REQUIRE(remove_weather_plan.status == mirakana::editor::EnvironmentAuthoringCommandStatus::accepted);
    MK_REQUIRE(remove_weather_plan.command_id == "environment.command.weather_keyframe.remove");
    MK_REQUIRE(undo_stack.execute(mirakana::editor::make_environment_authoring_command_action(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::remove_weather_keyframe,
                      .weather_keyframe_index = 0U,
                      .label = "Remove Noon Weather",
                  })));
    MK_REQUIRE(document.profile_document_v2().weather_timeline.size() == 2U);
    MK_REQUIRE(document.profile_document_v2().weather_timeline[0].time_of_day_hours != 12.0F);

    MK_REQUIRE(undo_stack.execute(mirakana::editor::make_environment_authoring_command_action(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::select_quality_preset,
                      .quality_preset = mirakana::EnvironmentQualityPreset::custom,
                      .label = "Select Custom Quality",
                  })));
    MK_REQUIRE(document.profile_document_v2().quality_preset == mirakana::EnvironmentQualityPreset::custom);

    MK_REQUIRE(undo_stack.execute(mirakana::editor::make_environment_authoring_command_action(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::remove_volume,
                      .volume_id = "storm_front",
                      .label = "Remove Storm Front Volume",
                  })));
    MK_REQUIRE(
        std::ranges::none_of(document.profile_document_v2().volumes,
                             [](const mirakana::EnvironmentVolumeDesc& volume) { return volume.id == "storm_front"; }));
}

MK_TEST("editor environment authoring cubemap capture command is reviewed but never executed in editor core") {
    auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile_document_v2(
        make_editor_environment_profile_v2(), "assets/environment/outdoor.environment");

    const auto capture_plan = mirakana::editor::plan_environment_authoring_command(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::request_cubemap_capture,
                      .label = "Request Cubemap Capture",
                  });
    MK_REQUIRE(capture_plan.status == mirakana::editor::EnvironmentAuthoringCommandStatus::accepted);
    MK_REQUIRE(capture_plan.command_id == "environment.command.capture.cubemap.request");
    MK_REQUIRE(capture_plan.requests_cubemap_capture);
    MK_REQUIRE(!capture_plan.mutates_document);
    MK_REQUIRE(!capture_plan.invokes_backend);
    MK_REQUIRE(!capture_plan.executes_package_scripts);
    MK_REQUIRE(!capture_plan.exposes_native_handles);

    const auto rejected_backend = mirakana::editor::plan_environment_authoring_command(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::request_cubemap_capture,
                      .request_backend_execution = true,
                      .request_package_script_execution = true,
                      .request_validation_recipe_execution = true,
                      .request_shell_execution = true,
                      .request_native_handle_access = true,
                      .label = "Unsafe Capture",
                  });
    MK_REQUIRE(rejected_backend.status ==
               mirakana::editor::EnvironmentAuthoringCommandStatus::rejected_unsafe_execution);
    MK_REQUIRE(!rejected_backend.invokes_backend);
    MK_REQUIRE(!rejected_backend.executes_package_scripts);
    MK_REQUIRE(!rejected_backend.exposes_native_handles);
    mirakana::editor::UndoStack undo_stack;
    MK_REQUIRE(!undo_stack.execute(mirakana::editor::make_environment_authoring_command_action(
        document, mirakana::editor::EnvironmentAuthoringCommandRequest{
                      .kind = mirakana::editor::EnvironmentAuthoringCommandKind::request_cubemap_capture,
                      .label = "Request Cubemap Capture",
                  })));
}

MK_TEST("editor environment package registration draft reviews runtime additions only") {
    const auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile_document_v2(
        make_editor_environment_profile_v2(), "assets/environment/outdoor.environment");

    const auto candidates = mirakana::editor::make_environment_package_candidate_rows(
        document, "runtime/environment/outdoor.geenv", "runtime/environment/environment.geindex");
    const std::vector<std::string> existing_runtime_files{"runtime/environment/environment.geindex"};
    const auto rows = mirakana::editor::make_environment_package_registration_draft_rows(candidates, "games/sample",
                                                                                         existing_runtime_files);

    MK_REQUIRE(rows.size() == 3U);
    MK_REQUIRE(rows[0].kind == mirakana::editor::EnvironmentPackageCandidateKind::profile_source);
    MK_REQUIRE(rows[0].status == mirakana::editor::EnvironmentPackageRegistrationDraftStatus::rejected_source_file);
    MK_REQUIRE(rows[1].kind == mirakana::editor::EnvironmentPackageCandidateKind::profile_cooked);
    MK_REQUIRE(rows[1].runtime_package_path == "runtime/environment/outdoor.geenv");
    MK_REQUIRE(rows[1].status == mirakana::editor::EnvironmentPackageRegistrationDraftStatus::add_runtime_file);
    MK_REQUIRE(rows[2].kind == mirakana::editor::EnvironmentPackageCandidateKind::package_index);
    MK_REQUIRE(rows[2].status == mirakana::editor::EnvironmentPackageRegistrationDraftStatus::already_registered);
    MK_REQUIRE(mirakana::editor::environment_package_registration_draft_status_label(rows[1].status) ==
               "add_runtime_file");

    const std::array duplicate_candidates = {
        mirakana::editor::EnvironmentPackageCandidateRow{
            .kind = mirakana::editor::EnvironmentPackageCandidateKind::profile_cooked,
            .path = "runtime/environment/outdoor.geenv",
            .runtime_file = true,
        },
        mirakana::editor::EnvironmentPackageCandidateRow{
            .kind = mirakana::editor::EnvironmentPackageCandidateKind::profile_cooked,
            .path = "runtime/environment/outdoor.geenv",
            .runtime_file = true,
        },
        mirakana::editor::EnvironmentPackageCandidateRow{
            .kind = mirakana::editor::EnvironmentPackageCandidateKind::profile_cooked,
            .path = "../outside.geenv",
            .runtime_file = true,
        },
    };
    const auto rejected =
        mirakana::editor::make_environment_package_registration_draft_rows(duplicate_candidates, "games/sample", {});
    MK_REQUIRE(rejected[1].status == mirakana::editor::EnvironmentPackageRegistrationDraftStatus::rejected_duplicate);
    MK_REQUIRE(rejected[2].status == mirakana::editor::EnvironmentPackageRegistrationDraftStatus::rejected_unsafe_path);
}

MK_TEST("native editor inspector surfaces environment authoring without middleware or backend execution") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto& environment = app.environment_authoring_document();
    MK_REQUIRE(environment.profile().id == "default_environment");

    const auto rows = app.environment_authoring_inspector_rows();
    MK_REQUIRE(std::ranges::any_of(rows, [](const mirakana::editor::EnvironmentAuthoringInspectorRow& row) {
        return row.id == "environment.weather.preset" && row.value == "clear";
    }));

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    MK_REQUIRE(!shell_document.native_handles_exposed);
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.inspector.rich_text.paragraph.property.environment.sky.model"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.inspector.rich_text.paragraph.property.environment.weather.preset"));
    MK_REQUIRE(contains_element(
        shell_document.document,
        "editor.panel.inspector.rich_text.paragraph.property.environment.readiness.physical_sky.package_status"));
    MK_REQUIRE(contains_element(
        shell_document.document,
        "editor.panel.inspector.rich_text.paragraph.property.environment.readiness.unsupported.environment_ready"));

    const auto counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);
    MK_REQUIRE(!counters.imgui_enabled);
    MK_REQUIRE(!counters.sdl3_enabled);
    MK_REQUIRE(counters.panel_count == app.native_panel_count());
}

int main() {
    return mirakana::test::run_all();
}
