// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ui/runtime_ui_standard_widgets.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] bool
has_provenance_diagnostic(const std::vector<mirakana::ui::RuntimeUiProvenanceDiagnostic>& diagnostics,
                          mirakana::ui::RuntimeUiProvenanceDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] bool has_text_diagnostic(const std::vector<std::string>& diagnostics, std::string_view text) {
    return std::ranges::any_of(diagnostics, [text](const auto& diagnostic) { return diagnostic == text; });
}

[[nodiscard]] mirakana::ui::RuntimeUiStandardWidgetProvenanceDesc make_clean_provenance() {
    using namespace mirakana::ui;

    RuntimeUiStandardWidgetProvenanceDesc desc;
    desc.feature_id = "first_party_runtime_ui_standard_widgets_v1";
    desc.source_references = {
        {.kind = RuntimeUiSourceReferenceKind::official_documentation,
         .name = "unity.runtime-ui.official-category-review",
         .source_url = "https://docs.unity3d.com/6000.0/Documentation/Manual/UIE-get-started-with-runtime-ui.html",
         .license_id = "official-doc-reference-only",
         .contains_copied_expression = false,
         .distributed_with_runtime = false},
        {.kind = RuntimeUiSourceReferenceKind::official_documentation,
         .name = "unreal.runtime-ui.official-category-review",
         .source_url = "https://dev.epicgames.com/documentation/en-us/unreal-engine/building-your-ui-in-unreal-engine",
         .license_id = "official-doc-reference-only",
         .contains_copied_expression = false,
         .distributed_with_runtime = false},
        {.kind = RuntimeUiSourceReferenceKind::official_documentation,
         .name = "godot.runtime-ui.official-category-review",
         .source_url = "https://docs.godotengine.org/en/stable/tutorials/ui/index.html",
         .license_id = "official-doc-reference-only",
         .contains_copied_expression = false,
         .distributed_with_runtime = false},
        {.kind = RuntimeUiSourceReferenceKind::first_party_design,
         .name = "mirakana.runtime-ui-standard-widgets.first-party-design",
         .source_url = {},
         .license_id = "LicenseRef-Proprietary",
         .contains_copied_expression = false,
         .distributed_with_runtime = true},
    };
    return desc;
}

[[nodiscard]] mirakana::ui::RuntimeUiMenuStackDesc make_pause_menu_stack() {
    using namespace mirakana::ui;

    RuntimeUiMenuStackDesc desc;
    desc.id = "runtime_menu";
    desc.active_screen_id = "pause_menu";
    desc.screens = {
        {.id = "pause_menu",
         .title_localization_key = "ui.pause.title",
         .accessibility_label = "Pause menu",
         .actions = {{.id = "resume_game",
                      .label = "Resume",
                      .localization_key = "ui.pause.resume",
                      .intent = RuntimeUiMenuActionIntent::resume_game,
                      .target_screen_id = {},
                      .enabled = true},
                     {.id = "open_settings",
                      .label = "Settings",
                      .localization_key = "ui.pause.settings",
                      .intent = RuntimeUiMenuActionIntent::open_screen,
                      .target_screen_id = "settings_menu",
                      .enabled = true},
                     {.id = "restart_session",
                      .label = "Restart",
                      .localization_key = "ui.pause.restart",
                      .intent = RuntimeUiMenuActionIntent::restart_session,
                      .target_screen_id = {},
                      .enabled = true}}},
        {.id = "settings_menu",
         .title_localization_key = "ui.settings.title",
         .accessibility_label = "Settings menu",
         .actions = {{.id = "close_settings",
                      .label = "Back",
                      .localization_key = "ui.settings.back",
                      .intent = RuntimeUiMenuActionIntent::close_screen,
                      .target_screen_id = "pause_menu",
                      .enabled = true}}},
    };
    return desc;
}

[[nodiscard]] std::vector<mirakana::ui::RuntimeUiMeterDesc> make_standard_meters() {
    using namespace mirakana::ui;

    return {
        {.id = "health",
         .kind = RuntimeUiMeterKind::health,
         .label = "HP",
         .localization_key = "ui.meter.health",
         .accessibility_label = "Health",
         .value = 75.0F,
         .maximum = 100.0F,
         .warning_threshold = 0.25F,
         .fill_direction = RuntimeUiMeterFillDirection::left_to_right,
         .track_token = "hud.meter.track",
         .fill_token = "hud.meter.health.fill",
         .warning_token = "hud.meter.warning.fill",
         .visible = true},
        {.id = "mana",
         .kind = RuntimeUiMeterKind::mana,
         .label = "MP",
         .localization_key = "ui.meter.mana",
         .accessibility_label = "Mana",
         .value = 40.0F,
         .maximum = 100.0F,
         .warning_threshold = 0.25F,
         .fill_direction = RuntimeUiMeterFillDirection::left_to_right,
         .track_token = "hud.meter.track",
         .fill_token = "hud.meter.mana.fill",
         .warning_token = "hud.meter.warning.fill",
         .visible = true},
        {.id = "stamina",
         .kind = RuntimeUiMeterKind::stamina,
         .label = "SP",
         .localization_key = "ui.meter.stamina",
         .accessibility_label = "Stamina",
         .value = 10.0F,
         .maximum = 100.0F,
         .warning_threshold = 0.25F,
         .fill_direction = RuntimeUiMeterFillDirection::left_to_right,
         .track_token = "hud.meter.track",
         .fill_token = "hud.meter.stamina.fill",
         .warning_token = "hud.meter.warning.fill",
         .visible = true},
    };
}

} // namespace

MK_TEST("runtime ui standard widgets accept official docs and first party design only") {
    const auto plan = mirakana::ui::review_runtime_ui_standard_widget_provenance(make_clean_provenance());

    MK_REQUIRE(plan.ready);
    MK_REQUIRE(plan.official_documentation_rows == 3U);
    MK_REQUIRE(plan.first_party_design_rows == 1U);
    MK_REQUIRE(plan.third_party_rows == 0U);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime ui standard widgets reject copied external expression") {
    auto desc = make_clean_provenance();
    desc.source_references.front().contains_copied_expression = true;

    const auto plan = mirakana::ui::review_runtime_ui_standard_widget_provenance(desc);

    MK_REQUIRE(!plan.ready);
    MK_REQUIRE(has_provenance_diagnostic(plan.diagnostics,
                                         mirakana::ui::RuntimeUiProvenanceDiagnosticCode::copied_external_expression));
}

MK_TEST("runtime ui standard widgets reject external engine code and assets") {
    auto desc = make_clean_provenance();
    desc.uses_external_engine_code = true;
    desc.uses_external_engine_assets = true;

    const auto plan = mirakana::ui::review_runtime_ui_standard_widget_provenance(desc);

    MK_REQUIRE(!plan.ready);
    MK_REQUIRE(has_provenance_diagnostic(
        plan.diagnostics, mirakana::ui::RuntimeUiProvenanceDiagnosticCode::third_party_code_without_notice));
    MK_REQUIRE(has_provenance_diagnostic(
        plan.diagnostics, mirakana::ui::RuntimeUiProvenanceDiagnosticCode::third_party_asset_without_notice));
}

MK_TEST("runtime ui standard widgets reject external public UI names and middleware") {
    auto desc = make_clean_provenance();
    desc.uses_external_engine_public_names = true;
    desc.uses_ui_middleware = true;
    desc.source_references.push_back({.kind = mirakana::ui::RuntimeUiSourceReferenceKind::official_documentation,
                                      .name = "UMG Slate uGUI VisualElement Control CanvasLayer",
                                      .source_url = "https://example.invalid/reference-only",
                                      .license_id = "official-doc-reference-only",
                                      .contains_copied_expression = false,
                                      .distributed_with_runtime = false});

    const auto plan = mirakana::ui::review_runtime_ui_standard_widget_provenance(desc);

    MK_REQUIRE(!plan.ready);
    MK_REQUIRE(has_provenance_diagnostic(plan.diagnostics,
                                         mirakana::ui::RuntimeUiProvenanceDiagnosticCode::external_engine_public_name));
    MK_REQUIRE(has_provenance_diagnostic(
        plan.diagnostics, mirakana::ui::RuntimeUiProvenanceDiagnosticCode::unsupported_ui_middleware_reference));
}

MK_TEST("runtime ui standard widgets bound external public UI name matching") {
    auto desc = make_clean_provenance();
    desc.source_references.push_back({.kind = mirakana::ui::RuntimeUiSourceReferenceKind::first_party_design,
                                      .name = "mirakana.ControlPanel.LayoutSlateTheme",
                                      .source_url = {},
                                      .license_id = "LicenseRef-Proprietary",
                                      .contains_copied_expression = false,
                                      .distributed_with_runtime = true});

    const auto plan = mirakana::ui::review_runtime_ui_standard_widget_provenance(desc);

    MK_REQUIRE(plan.ready);
    MK_REQUIRE(!has_provenance_diagnostic(
        plan.diagnostics, mirakana::ui::RuntimeUiProvenanceDiagnosticCode::external_engine_public_name));
}

MK_TEST("runtime ui standard widgets plan health mana and stamina meters") {
    const auto plan = mirakana::ui::plan_runtime_ui_meters(make_standard_meters());

    MK_REQUIRE(plan.ready);
    MK_REQUIRE(plan.rows.size() == 3U);
    MK_REQUIRE(plan.rows[0].id == "health");
    MK_REQUIRE(plan.rows[0].normalized_value == 0.75F);
    MK_REQUIRE(!plan.rows[0].warning);
    MK_REQUIRE(!plan.rows[0].depleted);
    MK_REQUIRE(plan.rows[1].id == "mana");
    MK_REQUIRE(plan.rows[1].normalized_value == 0.40F);
    MK_REQUIRE(!plan.rows[1].warning);
    MK_REQUIRE(!plan.rows[1].depleted);
    MK_REQUIRE(plan.rows[2].id == "stamina");
    MK_REQUIRE(plan.rows[2].normalized_value == 0.10F);
    MK_REQUIRE(plan.rows[2].warning);
    MK_REQUIRE(!plan.rows[2].depleted);
}

MK_TEST("runtime ui standard widgets mark depleted meter") {
    auto meters = make_standard_meters();
    meters[0].value = 0.0F;

    const auto plan = mirakana::ui::plan_runtime_ui_meters(meters);

    MK_REQUIRE(plan.ready);
    MK_REQUIRE(plan.rows[0].normalized_value == 0.0F);
    MK_REQUIRE(plan.rows[0].warning);
    MK_REQUIRE(plan.rows[0].depleted);
}

MK_TEST("runtime ui standard widgets reject invalid and duplicate meter ids") {
    auto meters = make_standard_meters();
    meters[0].maximum = 0.0F;
    meters[2].id = "mana";

    const auto plan = mirakana::ui::plan_runtime_ui_meters(meters);

    MK_REQUIRE(!plan.ready);
    MK_REQUIRE(has_text_diagnostic(plan.diagnostics, "invalid_meter_maximum"));
    MK_REQUIRE(has_text_diagnostic(plan.diagnostics, "duplicate_meter_id"));
}

MK_TEST("runtime ui standard widgets build meter document with accessibility labels") {
    const auto plan = mirakana::ui::plan_runtime_ui_meters(make_standard_meters());
    const auto document = mirakana::ui::build_runtime_ui_meter_document(
        plan, mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 640.0F, .height = 360.0F});

    MK_REQUIRE(plan.ready);
    MK_REQUIRE(document.size() >= 13U);
    const auto elements = document.traverse();
    MK_REQUIRE(elements.front().role == mirakana::ui::SemanticRole::root);
    MK_REQUIRE(std::ranges::any_of(elements, [](const auto& element) {
        return element.role == mirakana::ui::SemanticRole::meter && element.accessibility_label == "Health";
    }));
    MK_REQUIRE(std::ranges::any_of(elements, [](const auto& element) {
        return element.role == mirakana::ui::SemanticRole::label && element.text.localization_key == "ui.meter.mana";
    }));
    MK_REQUIRE(std::ranges::any_of(elements, [](const auto& element) {
        return element.role == mirakana::ui::SemanticRole::meter && element.accessibility_label == "Stamina";
    }));
    MK_REQUIRE(std::ranges::any_of(elements, [](const auto& element) {
        return element.role == mirakana::ui::SemanticRole::label && element.text.label == "SP" &&
               element.text.localization_key == "ui.meter.stamina";
    }));
    MK_REQUIRE(std::ranges::any_of(elements, [](const auto& element) {
        return element.id.value == "runtime_ui.meters.root.meter.stamina.fill" &&
               element.style.background_token == "hud.meter.warning.fill";
    }));
}

MK_TEST("runtime ui standard widgets plan pause menu stack") {
    const auto menu_stack = make_pause_menu_stack();
    const auto plan = mirakana::ui::plan_runtime_ui_menu_stack(menu_stack);

    MK_REQUIRE(plan.ready);
    MK_REQUIRE(plan.active_screen_id == "pause_menu");
    MK_REQUIRE(plan.screen_count == 2U);
    MK_REQUIRE(plan.action_count == 4U);
    MK_REQUIRE(plan.focus_order.size() == 3U);
    MK_REQUIRE(plan.focus_order[0] == "resume_game");
    MK_REQUIRE(plan.focus_order[1] == "open_settings");
    MK_REQUIRE(plan.focus_order[2] == "restart_session");
}

MK_TEST("runtime ui standard widgets reject invalid menu stack") {
    auto menu_stack = make_pause_menu_stack();
    menu_stack.active_screen_id = "missing_screen";
    menu_stack.screens.push_back(menu_stack.screens.front());
    menu_stack.screens.front().actions.push_back(menu_stack.screens.front().actions.front());
    menu_stack.screens.front().actions[1].target_screen_id = "unknown_target";
    menu_stack.screens.front().actions[2].id = "UMG";

    const auto plan = mirakana::ui::plan_runtime_ui_menu_stack(menu_stack);

    MK_REQUIRE(!plan.ready);
    MK_REQUIRE(has_text_diagnostic(plan.diagnostics, "unknown_active_screen"));
    MK_REQUIRE(has_text_diagnostic(plan.diagnostics, "duplicate_screen_id"));
    MK_REQUIRE(has_text_diagnostic(plan.diagnostics, "duplicate_action_id"));
    MK_REQUIRE(has_text_diagnostic(plan.diagnostics, "unknown_target_screen"));
    MK_REQUIRE(has_text_diagnostic(plan.diagnostics, "external_engine_public_name"));
}

MK_TEST("runtime ui standard widgets compose standard hud only with clean inputs") {
    mirakana::ui::RuntimeUiStandardHudDesc desc;
    desc.id = "standard_hud";
    desc.viewport = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1280.0F, .height = 720.0F};
    desc.meters = make_standard_meters();
    desc.menu_stack = make_pause_menu_stack();
    desc.provenance = make_clean_provenance();

    const auto plan = mirakana::ui::plan_runtime_ui_standard_hud(desc);

    MK_REQUIRE(plan.ready);
    MK_REQUIRE(plan.meter_plan.rows.size() == 3U);
    MK_REQUIRE(plan.menu_plan.screen_count == 2U);
    MK_REQUIRE(plan.menu_plan.action_count == 4U);
    MK_REQUIRE(plan.provenance_plan.ready);
    MK_REQUIRE(plan.document.size() > 13U);
}

MK_TEST("runtime ui standard widgets reject dirty standard hud provenance") {
    mirakana::ui::RuntimeUiStandardHudDesc desc;
    desc.id = "standard_hud";
    desc.viewport = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1280.0F, .height = 720.0F};
    desc.meters = make_standard_meters();
    desc.menu_stack = make_pause_menu_stack();
    desc.provenance = make_clean_provenance();
    desc.provenance.uses_external_engine_assets = true;

    const auto plan = mirakana::ui::plan_runtime_ui_standard_hud(desc);

    MK_REQUIRE(!plan.ready);
    MK_REQUIRE(!plan.provenance_plan.ready);
    MK_REQUIRE(plan.document.size() == 0U);
}

int main() {
    return mirakana::test::run_all();
}
