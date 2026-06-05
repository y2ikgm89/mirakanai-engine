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

} // namespace

MK_TEST("editor environment authoring emits deterministic inspector rows") {
    const auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile(
        make_editor_environment_profile(), "assets/environment/outdoor.environment");

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
            .quality_tier = mirakana::editor::EnvironmentAuthoringQualityTier::high,
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

    const auto ui = mirakana::editor::make_environment_authoring_ui_model(model);
    MK_REQUIRE(contains_element(ui, "environment_authoring.inspector.rows.environment.sky.model.value"));
    MK_REQUIRE(contains_element(ui, "environment_authoring.inspector.rows.environment.quality.tier.value"));
}

MK_TEST("editor environment authoring exposes readiness and unsupported claim rows") {
    const auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile(
        make_editor_environment_profile(), "assets/environment/outdoor.environment");

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
            .quality_tier = mirakana::editor::EnvironmentAuthoringQualityTier::high,
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
    store.write_text(path, mirakana::serialize_environment_profile(make_editor_environment_profile()));

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
    MK_REQUIRE(store.read_text(path).contains("id=outdoor_clear\n"));
    MK_REQUIRE(store.read_text(path).contains("weather.kind=clear\n"));
}

MK_TEST("editor environment package registration draft reviews runtime additions only") {
    const auto document = mirakana::editor::EnvironmentAuthoringDocument::from_profile(
        make_editor_environment_profile(), "assets/environment/outdoor.environment");

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
