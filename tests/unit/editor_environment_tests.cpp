// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "first_party_editor_document.hpp"
#include "native_editor_app.hpp"

#include "mirakana/editor/environment_authoring.hpp"
#include "mirakana/editor/history.hpp"
#include "mirakana/environment/environment_io.hpp"
#include "mirakana/environment/environment_preset_pack.hpp"

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

[[nodiscard]] mirakana::EnvironmentPresetPackDocumentV1 make_editor_environment_preset_pack() {
    mirakana::EnvironmentPresetPackDocumentV1 pack{};
    pack.id = "sample_commercial_environment_presets";
    pack.provenance_id = "provenance.environment.sample_commercial_presets";
    pack.license_id = "LicenseRef-Proprietary";
    pack.art_direction = "first_party_commercial_environment_baseline";
    pack.quality_tier = mirakana::EnvironmentQualityPreset::high;
    pack.package_size_budget_bytes = 12000U;
    pack.installed_size_budget_bytes = 12000U;
    pack.decoded_memory_budget_bytes = 56000U;
    pack.gpu_memory_budget_bytes = 44000U;
    pack.required_backend_feature_rows.push_back("environment_platform_windows_d3d12_ready");

    constexpr std::array<std::string_view, 7U> required_ids{
        "clear_noon",
        "overcast_storm",
        "night_moonlit",
        "snowfield",
        "foggy_valley",
        "cinematic_sunset",
        "indoor_to_outdoor_transition",
    };
    for (const auto id : required_ids) {
        pack.presets.push_back(mirakana::EnvironmentPresetPackPresetV1{
            .id = std::string{id},
            .profile_asset_path = "runtime/assets/desktop_runtime/default_outdoor.geenv",
            .art_direction = "sample_art_direction_" + std::string{id},
            .sky_atmosphere_values = "physical_atmosphere_profile_" + std::string{id},
            .fog_cloud_weather_timeline = "weather_timeline_" + std::string{id},
            .ibl_asset_ref = "runtime/assets/desktop_runtime/environment_ibl.geasset",
            .material_weathering_ref = "runtime/assets/desktop_runtime/material_weathering.geasset",
            .audio_trigger_intent = "environment_audio_trigger_" + std::string{id},
            .quality_budget_id = "environment_quality_budget.high",
            .quality_tier = mirakana::EnvironmentQualityPreset::high,
            .package_size_bytes = 1200U,
            .installed_size_bytes = 1200U,
            .estimated_decoded_memory_bytes = 5000U,
            .estimated_gpu_memory_bytes = 4000U,
            .validation_recipe_id = "desktop-runtime-sample-game-environment-ready-aggregate",
        });
    }
    return pack;
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

[[nodiscard]] bool contains_element(const mirakana::ui::UiDocument& document, std::string_view id) {
    return document.find(mirakana::ui::ElementId{.value = std::string{id}}) != nullptr;
}

[[nodiscard]] const mirakana::editor::EnvironmentArtistWorkflowCommandRow*
find_artist_workflow_command_row(const mirakana::editor::EnvironmentArtistWorkflowCommandCatalog& catalog,
                                 std::string_view id) noexcept {
    const auto it =
        std::ranges::find_if(catalog.commands, [id](const mirakana::editor::EnvironmentArtistWorkflowCommandRow& row) {
            return row.command_id == id;
        });
    return it == catalog.commands.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::EnvironmentArtistWorkflowCommandReportRow*
find_artist_workflow_report_row(const mirakana::editor::EnvironmentArtistWorkflowCommandPlan& plan,
                                std::string_view id) noexcept {
    const auto it = std::ranges::find_if(
        plan.report_rows,
        [id](const mirakana::editor::EnvironmentArtistWorkflowCommandReportRow& row) { return row.id == id; });
    return it == plan.report_rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::EnvironmentArtistWorkflowAssetBrowserRow*
find_artist_workflow_asset_row(const mirakana::editor::EnvironmentArtistWorkflowAssetBrowserModel& model,
                               std::string_view id) noexcept {
    const auto it =
        std::ranges::find_if(model.rows, [id](const mirakana::editor::EnvironmentArtistWorkflowAssetBrowserRow& row) {
            return row.row_id == id;
        });
    return it == model.rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::editor::EnvironmentArtistWorkflowPreviewRow*
find_artist_workflow_preview_row(const mirakana::editor::EnvironmentArtistWorkflowPreviewModel& model,
                                 std::string_view id) noexcept {
    const auto it =
        std::ranges::find_if(model.rows, [id](const mirakana::editor::EnvironmentArtistWorkflowPreviewRow& row) {
            return row.row_id == id;
        });
    return it == model.rows.end() ? nullptr : &(*it);
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

MK_TEST("editor environment artist workflow exposes reviewed command catalog rows") {
    const auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile_document_v2(
        make_editor_environment_profile_v2(), "assets/environment/outdoor.environment");

    const auto catalog = mirakana::editor::make_environment_artist_workflow_command_catalog(document);

    MK_REQUIRE(catalog.revision == document.revision());
    MK_REQUIRE(catalog.commands.size() == 11U);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.preset.import") != nullptr);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.source_asset.review") != nullptr);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.cook.preview") != nullptr);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.profile_graph.edit") != nullptr);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.weather_timeline.edit") != nullptr);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.local_volume.edit") != nullptr);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.simulation_parameter.edit") != nullptr);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.quality_budget.edit") != nullptr);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.package.preview") != nullptr);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.validation.remediation") != nullptr);
    MK_REQUIRE(find_artist_workflow_command_row(catalog, "environment.command.publish.package") != nullptr);

    const auto* local_volume = find_artist_workflow_command_row(catalog, "environment.command.local_volume.edit");
    MK_REQUIRE(local_volume != nullptr);
    MK_REQUIRE(local_volume->mutates_document);
    MK_REQUIRE(local_volume->supports_dry_run);
    MK_REQUIRE(local_volume->supports_revision_checked_apply);
    MK_REQUIRE(local_volume->supports_undo_metadata);
    MK_REQUIRE(!local_volume->invokes_backend);
    MK_REQUIRE(!local_volume->executes_package_scripts);
    MK_REQUIRE(!local_volume->exposes_native_handles);

    const auto* source_review = find_artist_workflow_command_row(catalog, "environment.command.source_asset.review");
    MK_REQUIRE(source_review != nullptr);
    MK_REQUIRE(!source_review->mutates_document);
    MK_REQUIRE(source_review->supports_dry_run);
    MK_REQUIRE(!source_review->requires_confirmation);
}

MK_TEST("editor environment artist workflow plans dry run and revision checked apply metadata") {
    const auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile_document_v2(
        make_editor_environment_profile_v2(), "assets/environment/outdoor.environment");

    const auto dry_run = mirakana::editor::plan_environment_artist_workflow_command(
        document, mirakana::editor::EnvironmentArtistWorkflowCommandRequest{
                      .kind = mirakana::editor::EnvironmentArtistWorkflowCommandKind::local_volume_edit,
                      .mode = mirakana::editor::EnvironmentArtistWorkflowCommandMode::dry_run,
                  });

    MK_REQUIRE(dry_run.status == mirakana::editor::EnvironmentArtistWorkflowCommandStatus::accepted);
    MK_REQUIRE(dry_run.command_id == "environment.command.local_volume.edit");
    MK_REQUIRE(dry_run.dry_run);
    MK_REQUIRE(!dry_run.apply);
    MK_REQUIRE(dry_run.mutates_document);
    MK_REQUIRE(dry_run.before_revision == document.revision());
    MK_REQUIRE(dry_run.after_revision == document.revision());
    MK_REQUIRE(!dry_run.revision_checked);
    MK_REQUIRE(dry_run.undo_supported);
    MK_REQUIRE(dry_run.rollback_metadata_available);
    MK_REQUIRE(!dry_run.invokes_backend);
    MK_REQUIRE(!dry_run.executes_package_scripts);
    MK_REQUIRE(!dry_run.exposes_native_handles);
    MK_REQUIRE(dry_run.diagnostics.empty());
    MK_REQUIRE(find_artist_workflow_report_row(dry_run, "environment.workflow.command_id")->value ==
               "environment.command.local_volume.edit");
    MK_REQUIRE(find_artist_workflow_report_row(dry_run, "environment.workflow.mode")->value == "dry_run");
    MK_REQUIRE(find_artist_workflow_report_row(dry_run, "environment.workflow.undo_supported")->value == "true");

    const auto apply = mirakana::editor::plan_environment_artist_workflow_command(
        document, mirakana::editor::EnvironmentArtistWorkflowCommandRequest{
                      .kind = mirakana::editor::EnvironmentArtistWorkflowCommandKind::weather_timeline_edit,
                      .mode = mirakana::editor::EnvironmentArtistWorkflowCommandMode::apply,
                      .expected_revision = document.revision(),
                  });

    MK_REQUIRE(apply.status == mirakana::editor::EnvironmentArtistWorkflowCommandStatus::accepted);
    MK_REQUIRE(apply.command_id == "environment.command.weather_timeline.edit");
    MK_REQUIRE(!apply.dry_run);
    MK_REQUIRE(apply.apply);
    MK_REQUIRE(apply.revision_checked);
    MK_REQUIRE(apply.before_revision == document.revision());
    MK_REQUIRE(apply.after_revision == document.revision() + 1U);
    MK_REQUIRE(apply.undo_supported);
    MK_REQUIRE(apply.rollback_metadata_available);
    MK_REQUIRE(find_artist_workflow_report_row(apply, "environment.workflow.revision_checked")->value == "true");

    const auto stale = mirakana::editor::plan_environment_artist_workflow_command(
        document, mirakana::editor::EnvironmentArtistWorkflowCommandRequest{
                      .kind = mirakana::editor::EnvironmentArtistWorkflowCommandKind::quality_budget_edit,
                      .mode = mirakana::editor::EnvironmentArtistWorkflowCommandMode::apply,
                      .expected_revision = document.revision() + 10U,
                  });
    MK_REQUIRE(stale.status == mirakana::editor::EnvironmentArtistWorkflowCommandStatus::rejected_stale_revision);
    MK_REQUIRE(!stale.apply);
    MK_REQUIRE(!stale.diagnostics.empty());

    const auto unsafe = mirakana::editor::plan_environment_artist_workflow_command(
        document, mirakana::editor::EnvironmentArtistWorkflowCommandRequest{
                      .kind = mirakana::editor::EnvironmentArtistWorkflowCommandKind::publish_package,
                      .mode = mirakana::editor::EnvironmentArtistWorkflowCommandMode::apply,
                      .expected_revision = document.revision(),
                      .request_backend_execution = true,
                      .request_package_script_execution = true,
                      .request_native_handle_access = true,
                  });
    MK_REQUIRE(unsafe.status == mirakana::editor::EnvironmentArtistWorkflowCommandStatus::rejected_unsafe_execution);
    MK_REQUIRE(!unsafe.invokes_backend);
    MK_REQUIRE(!unsafe.executes_package_scripts);
    MK_REQUIRE(!unsafe.exposes_native_handles);
}

MK_TEST("editor environment artist workflow exposes deterministic asset browser rows") {
    const auto
        model =
            mirakana::editor::make_environment_artist_workflow_asset_browser_model(
                mirakana::editor::EnvironmentArtistWorkflowAssetBrowserDesc{
                    .assets =
                        {
                            mirakana::editor::EnvironmentArtistWorkflowAssetBrowserInputRow{
                                .kind = mirakana::editor::EnvironmentArtistWorkflowAssetKind::preset_library,
                                .path = "runtime/assets/desktop_runtime/environment_presets.gepresetpack",
                                .available = true,
                                .package_visible = true,
                                .provenance_recorded = true,
                                .budget_recorded = true,
                                .validation_recipe_id =
                                    "desktop-runtime-sample-game-environment-preset-library-package",
                            },
                            mirakana::editor::EnvironmentArtistWorkflowAssetBrowserInputRow{
                                .kind = mirakana::editor::EnvironmentArtistWorkflowAssetKind::openexr_source,
                                .path = "source/assets/environment/sky_reference.exr",
                                .available = true,
                                .provenance_recorded = true,
                                .validation_recipe_id = "asset-importers",
                            },
                            mirakana::editor::EnvironmentArtistWorkflowAssetBrowserInputRow{
                                .kind = mirakana::editor::EnvironmentArtistWorkflowAssetKind::ktx2_basis_source,
                                .path = "source/assets/environment/cloud_noise.ktx2",
                                .available = true,
                                .provenance_recorded = true,
                                .validation_recipe_id = "asset-importers",
                            },
                            mirakana::editor::EnvironmentArtistWorkflowAssetBrowserInputRow{
                                .kind = mirakana::editor::EnvironmentArtistWorkflowAssetKind::cooked_texture,
                                .path = "runtime/assets/desktop_runtime/environment_ibl.geasset",
                                .available = true,
                                .package_visible = true,
                                .budget_recorded = true,
                                .validation_recipe_id = "desktop-runtime-sample-game-environment-texture-package",
                            },
                            mirakana::editor::EnvironmentArtistWorkflowAssetBrowserInputRow{
                                .kind = mirakana::editor::EnvironmentArtistWorkflowAssetKind::environment_profile,
                                .path = "runtime/assets/desktop_runtime/default_outdoor.geenv",
                                .available = true,
                                .package_visible = true,
                                .budget_recorded = true,
                                .validation_recipe_id = "desktop-runtime-sample-game-environment-ready-aggregate",
                            },
                            mirakana::editor::EnvironmentArtistWorkflowAssetBrowserInputRow{
                                .kind = mirakana::editor::EnvironmentArtistWorkflowAssetKind::simulation_preset,
                                .path = "source/assets/environment/weather_simulation_preset.geweather",
                                .available = true,
                                .provenance_recorded = true,
                                .validation_recipe_id =
                                    "desktop-runtime-sample-game-environment-weather-simulation-package",
                            },
                            mirakana::editor::EnvironmentArtistWorkflowAssetBrowserInputRow{
                                .kind = mirakana::editor::EnvironmentArtistWorkflowAssetKind::validation_report,
                                .path = "out/validation/sample_desktop_runtime_game/environment_report.json",
                                .available = true,
                                .validation_recipe_id = "agent-contract",
                            },
                            mirakana::editor::EnvironmentArtistWorkflowAssetBrowserInputRow{
                                .kind = mirakana::editor::EnvironmentArtistWorkflowAssetKind::package_artifact,
                                .path = "out/package/sample_desktop_runtime_game/environment_package.zip",
                                .available = true,
                                .package_visible = true,
                                .budget_recorded = true,
                                .requires_host_gate = true,
                                .host_gate = "desktop-runtime-package",
                                .validation_recipe_id = "desktop-game-runtime",
                            },
                        },
                });

    MK_REQUIRE(model.status == mirakana::editor::EnvironmentAuthoringStatus::ready);
    MK_REQUIRE(model.rows.size() == 8U);
    MK_REQUIRE(model.ready_rows == 8U);
    MK_REQUIRE(!model.complete_artist_workflow_ready_claimed);
    MK_REQUIRE(!model.invokes_backend);
    MK_REQUIRE(!model.executes_package_scripts);
    MK_REQUIRE(!model.exposes_native_handles);
    MK_REQUIRE(model.diagnostics.empty());

    const auto* preset = find_artist_workflow_asset_row(model, "environment.workflow.asset.preset_library");
    const auto* exr = find_artist_workflow_asset_row(model, "environment.workflow.asset.openexr_source");
    const auto* ktx = find_artist_workflow_asset_row(model, "environment.workflow.asset.ktx2_basis_source");
    const auto* package = find_artist_workflow_asset_row(model, "environment.workflow.asset.package_artifact");

    MK_REQUIRE(preset != nullptr);
    MK_REQUIRE(exr != nullptr);
    MK_REQUIRE(ktx != nullptr);
    MK_REQUIRE(package != nullptr);
    MK_REQUIRE(preset->kind == mirakana::editor::EnvironmentArtistWorkflowAssetKind::preset_library);
    MK_REQUIRE(preset->package_visible);
    MK_REQUIRE(preset->provenance_recorded);
    MK_REQUIRE(preset->budget_recorded);
    MK_REQUIRE(preset->validation_recipe_id == "desktop-runtime-sample-game-environment-preset-library-package");
    MK_REQUIRE(exr->label == "OpenEXR Source");
    MK_REQUIRE(exr->provenance_recorded);
    MK_REQUIRE(!exr->package_visible);
    MK_REQUIRE(ktx->label == "KTX2/Basis Source");
    MK_REQUIRE(package->requires_host_gate);
    MK_REQUIRE(package->host_gate == "desktop-runtime-package");

    const auto ui = mirakana::editor::make_environment_artist_workflow_asset_browser_ui_model(model);
    MK_REQUIRE(contains_element(ui, "environment_artist_workflow_asset_browser.rows.environment.workflow.asset."
                                    "preset_library.value"));
    MK_REQUIRE(contains_element(ui, "environment_artist_workflow_asset_browser.rows.environment.workflow.asset."
                                    "package_artifact.host_gate"));
}

MK_TEST("editor environment artist workflow asset browser fails closed on unsafe execution requests") {
    const auto model = mirakana::editor::make_environment_artist_workflow_asset_browser_model(
        mirakana::editor::EnvironmentArtistWorkflowAssetBrowserDesc{
            .assets =
                {
                    mirakana::editor::EnvironmentArtistWorkflowAssetBrowserInputRow{
                        .kind = mirakana::editor::EnvironmentArtistWorkflowAssetKind::preset_library,
                        .path = "runtime/assets/desktop_runtime/environment_presets.gepresetpack",
                        .available = true,
                    },
                },
            .request_backend_execution = true,
            .request_package_script_execution = true,
            .request_native_handle_access = true,
        });

    MK_REQUIRE(model.status == mirakana::editor::EnvironmentAuthoringStatus::blocked);
    MK_REQUIRE(!model.invokes_backend);
    MK_REQUIRE(!model.executes_package_scripts);
    MK_REQUIRE(!model.exposes_native_handles);
    MK_REQUIRE(!model.diagnostics.empty());
}

MK_TEST("editor environment artist workflow exposes deterministic preview rows") {
    const auto model = mirakana::editor::make_environment_artist_workflow_preview_model(
        mirakana::editor::EnvironmentArtistWorkflowPreviewDesc{
            .selected_backend = "d3d12",
            .quality_tier = "ultra",
            .package_budget_bytes = 4194304U,
            .memory_budget_bytes = 67108864U,
            .unsupported_claim_reason = "visible workflow and production walkthrough validation are not complete",
        });

    MK_REQUIRE(model.status == mirakana::editor::EnvironmentAuthoringStatus::ready);
    MK_REQUIRE(model.rows.size() == 7U);
    MK_REQUIRE(model.ready_rows == 7U);
    MK_REQUIRE(!model.complete_artist_workflow_ready_claimed);
    MK_REQUIRE(!model.invokes_backend);
    MK_REQUIRE(!model.executes_package_scripts);
    MK_REQUIRE(!model.exposes_native_handles);
    MK_REQUIRE(model.diagnostics.empty());

    const auto* backend = find_artist_workflow_preview_row(model, "environment.workflow.preview.selected_backend");
    const auto* quality = find_artist_workflow_preview_row(model, "environment.workflow.preview.quality_tier");
    const auto* host_gate = find_artist_workflow_preview_row(model, "environment.workflow.preview.missing_host_gate");
    const auto* package_budget = find_artist_workflow_preview_row(model, "environment.workflow.preview.package_budget");
    const auto* memory_budget = find_artist_workflow_preview_row(model, "environment.workflow.preview.memory_budget");
    const auto* diagnostics = find_artist_workflow_preview_row(model, "environment.workflow.preview.diagnostics");
    const auto* unsupported =
        find_artist_workflow_preview_row(model, "environment.workflow.preview.unsupported_claim_reason");

    MK_REQUIRE(backend != nullptr);
    MK_REQUIRE(quality != nullptr);
    MK_REQUIRE(host_gate != nullptr);
    MK_REQUIRE(package_budget != nullptr);
    MK_REQUIRE(memory_budget != nullptr);
    MK_REQUIRE(diagnostics != nullptr);
    MK_REQUIRE(unsupported != nullptr);
    MK_REQUIRE(backend->kind == mirakana::editor::EnvironmentArtistWorkflowPreviewRowKind::selected_backend);
    MK_REQUIRE(backend->value == "d3d12");
    MK_REQUIRE(quality->value == "ultra");
    MK_REQUIRE(host_gate->value == "none");
    MK_REQUIRE(package_budget->value == "4194304");
    MK_REQUIRE(memory_budget->value == "67108864");
    MK_REQUIRE(diagnostics->value == "0");
    MK_REQUIRE(unsupported->value == "visible workflow and production walkthrough validation are not complete");
    MK_REQUIRE(unsupported->status == mirakana::editor::EnvironmentArtistWorkflowPreviewRowStatus::ready);

    const auto ui = mirakana::editor::make_environment_artist_workflow_preview_ui_model(model);
    MK_REQUIRE(contains_element(ui, "environment_artist_workflow_preview.rows.environment.workflow.preview."
                                    "selected_backend.value"));
    MK_REQUIRE(contains_element(ui, "environment_artist_workflow_preview.rows.environment.workflow.preview."
                                    "unsupported_claim_reason.value"));
}

MK_TEST("editor environment artist workflow preview rows expose host gate and fail closed") {
    const auto model = mirakana::editor::make_environment_artist_workflow_preview_model(
        mirakana::editor::EnvironmentArtistWorkflowPreviewDesc{
            .selected_backend = "metal",
            .quality_tier = "ultra",
            .missing_host_gate = "macos-metal-host",
            .package_budget_bytes = 4194304U,
            .memory_budget_bytes = 67108864U,
            .diagnostics = 2U,
            .unsupported_claim_reason = "macOS Metal host evidence and walkthrough validation are missing",
            .request_backend_execution = true,
            .request_package_script_execution = true,
            .request_native_handle_access = true,
        });

    MK_REQUIRE(model.status == mirakana::editor::EnvironmentAuthoringStatus::blocked);
    MK_REQUIRE(!model.complete_artist_workflow_ready_claimed);
    MK_REQUIRE(!model.invokes_backend);
    MK_REQUIRE(!model.executes_package_scripts);
    MK_REQUIRE(!model.exposes_native_handles);
    MK_REQUIRE(!model.diagnostics.empty());

    const auto* host_gate = find_artist_workflow_preview_row(model, "environment.workflow.preview.missing_host_gate");
    const auto* diagnostics = find_artist_workflow_preview_row(model, "environment.workflow.preview.diagnostics");
    MK_REQUIRE(host_gate != nullptr);
    MK_REQUIRE(diagnostics != nullptr);
    MK_REQUIRE(host_gate->status == mirakana::editor::EnvironmentArtistWorkflowPreviewRowStatus::blocked);
    MK_REQUIRE(host_gate->value == "macos-metal-host");
    MK_REQUIRE(diagnostics->status == mirakana::editor::EnvironmentArtistWorkflowPreviewRowStatus::blocked);
    MK_REQUIRE(diagnostics->value == "2");
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

MK_TEST("editor environment preset library exposes browsing rows and package evidence candidates") {
    const auto model =
        mirakana::editor::make_environment_preset_library_model(mirakana::editor::EnvironmentPresetLibraryDesc{
            .pack = make_editor_environment_preset_pack(),
            .path = "runtime/assets/desktop_runtime/environment_presets.gepresetpack",
            .runtime_package_path = "runtime/assets/desktop_runtime/environment_presets.gepresetpack",
            .package_index_registered = true,
            .sample_consumption_evidence = true,
        });

    MK_REQUIRE(model.status == mirakana::editor::EnvironmentAuthoringStatus::ready);
    MK_REQUIRE(model.pack_id == "sample_commercial_environment_presets");
    MK_REQUIRE(model.path == "runtime/assets/desktop_runtime/environment_presets.gepresetpack");
    MK_REQUIRE(!model.invokes_backend);
    MK_REQUIRE(!model.exposes_native_handles);
    MK_REQUIRE(!model.executes_package_scripts);
    MK_REQUIRE(find_environment_row(model, "environment.preset_library.pack.id")->value ==
               "sample_commercial_environment_presets");
    MK_REQUIRE(find_environment_row(model, "environment.preset_library.pack.provenance_id")->value ==
               "provenance.environment.sample_commercial_presets");
    MK_REQUIRE(find_environment_row(model, "environment.preset_library.pack.license_id")->value ==
               "LicenseRef-Proprietary");
    MK_REQUIRE(find_environment_row(model, "environment.preset_library.pack.preset_count")->value == "7");
    MK_REQUIRE(find_environment_row(model, "environment.preset_library.package.index_registered")->value == "true");
    MK_REQUIRE(find_environment_row(model, "environment.preset_library.sample.consumption_evidence")->value == "ready");
    MK_REQUIRE(find_environment_row(model, "environment.preset_library.preset.0.id")->value == "clear_noon");
    MK_REQUIRE(find_environment_row(model, "environment.preset_library.preset.6.id")->value ==
               "indoor_to_outdoor_transition");
    MK_REQUIRE(
        find_environment_row(model, "environment.readiness.unsupported.environment_aaa_preset_library_ready")->value ==
        "unclaimed");

    const auto ui = mirakana::editor::make_environment_preset_library_ui_model(model);
    MK_REQUIRE(contains_element(ui, "environment_preset_library.rows.environment.preset_library.pack.id.value"));
    MK_REQUIRE(contains_element(ui, "environment_preset_library.rows.environment.readiness.unsupported.environment_aaa_"
                                    "preset_library_ready.value"));

    const auto candidates = mirakana::editor::make_environment_preset_library_package_candidate_rows(
        "runtime/assets/desktop_runtime/environment_presets.gepresetpack");
    MK_REQUIRE(candidates.size() == 1U);
    MK_REQUIRE(candidates[0].kind == mirakana::editor::EnvironmentPackageCandidateKind::preset_pack);
    MK_REQUIRE(candidates[0].runtime_file);
    MK_REQUIRE(mirakana::editor::environment_package_candidate_kind_label(candidates[0].kind) == "preset_pack");

    const auto rows =
        mirakana::editor::make_environment_package_registration_draft_rows(candidates, "games/sample", {});
    MK_REQUIRE(rows.size() == 1U);
    MK_REQUIRE(rows[0].status == mirakana::editor::EnvironmentPackageRegistrationDraftStatus::add_runtime_file);
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
