// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ui/runtime_ui_widgets.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] bool has_diagnostic(const std::vector<mirakana::ui::RuntimeUiWidgetDiagnostic>& diagnostics,
                                  mirakana::ui::RuntimeUiWidgetDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] std::vector<mirakana::ui::RuntimeUiWidgetRow> make_complete_widget_rows() {
    using namespace mirakana::ui;

    return {
        {.id = "pause.resume",
         .kind = RuntimeUiWidgetKind::button,
         .label = "Resume",
         .localization_key = "ui.pause.resume",
         .accessibility_label = "Resume game",
         .focusable = true},
        {.id = "settings.fullscreen",
         .kind = RuntimeUiWidgetKind::toggle,
         .label = "Fullscreen",
         .localization_key = "ui.settings.fullscreen",
         .accessibility_label = "Fullscreen",
         .focusable = true},
        {.id = "settings.volume",
         .kind = RuntimeUiWidgetKind::slider,
         .label = "Volume",
         .localization_key = "ui.settings.volume",
         .accessibility_label = "Master volume",
         .focusable = true,
         .minimum_value = 0.0F,
         .maximum_value = 1.0F,
         .value = 0.75F},
        {.id = "profile.name",
         .kind = RuntimeUiWidgetKind::text_field,
         .label = "Name",
         .localization_key = "ui.profile.name",
         .accessibility_label = "Profile name",
         .focusable = true},
        {.id = "pause.stack",
         .kind = RuntimeUiWidgetKind::menu_stack,
         .label = "Pause",
         .localization_key = "ui.pause.title",
         .accessibility_label = "Pause menu",
         .focusable = false},
        {.id = "confirm.modal",
         .kind = RuntimeUiWidgetKind::modal_layer,
         .label = "Confirm",
         .localization_key = "ui.confirm.title",
         .accessibility_label = "Confirmation",
         .focusable = false},
        {.id = "inventory.list",
         .kind = RuntimeUiWidgetKind::list,
         .label = "Inventory",
         .localization_key = "ui.inventory.title",
         .accessibility_label = "Inventory list",
         .focusable = true},
        {.id = "quests.tree",
         .kind = RuntimeUiWidgetKind::tree,
         .label = "Quests",
         .localization_key = "ui.quests.title",
         .accessibility_label = "Quest tree",
         .focusable = true},
        {.id = "hud.prompt",
         .kind = RuntimeUiWidgetKind::hud_prompt,
         .label = "Interact",
         .localization_key = "ui.prompt.interact",
         .accessibility_label = "Interact prompt",
         .focusable = false},
        {.id = "controller.accept",
         .kind = RuntimeUiWidgetKind::controller_glyph,
         .label = "Accept",
         .localization_key = "ui.controller.accept",
         .accessibility_label = "Accept button",
         .focusable = false,
         .input_source_id = "gamepad.south"},
    };
}

[[nodiscard]] std::vector<mirakana::ui::RuntimeUiWidgetStateRow> make_state_rows() {
    return {
        {.id = "state.fullscreen", .widget_id = "settings.fullscreen", .checked = true},
        {.id = "state.volume", .widget_id = "settings.volume", .value = 0.75F},
        {.id = "state.profile.name", .widget_id = "profile.name", .text = "Player"},
    };
}

[[nodiscard]] std::vector<mirakana::ui::RuntimeUiWidgetCommandRow> make_command_rows() {
    return {
        {.id = "cmd.resume", .widget_id = "pause.resume", .command_id = "game.resume", .label = "Resume"},
        {.id = "cmd.apply-volume",
         .widget_id = "settings.volume",
         .command_id = "settings.apply_volume",
         .label = "Apply volume"},
    };
}

} // namespace

MK_TEST("runtime ui widget vocabulary accepts all first party widget kinds") {
    const auto plan =
        mirakana::ui::plan_runtime_ui_widgets(make_complete_widget_rows(), make_state_rows(), make_command_rows());

    MK_REQUIRE(plan.ready);
    MK_REQUIRE(plan.widget_rows.size() == 10U);
    MK_REQUIRE(plan.state_rows.size() == 3U);
    MK_REQUIRE(plan.command_rows.size() == 2U);
    MK_REQUIRE(plan.focusable_widget_rows == 6U);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime ui widget vocabulary rejects invalid widget rows") {
    auto widgets = make_complete_widget_rows();
    widgets.push_back(widgets.front());
    widgets[1].label.clear();
    widgets[2].minimum_value = 10.0F;
    widgets[2].maximum_value = 1.0F;
    widgets[8].public_native_handle_exposed = true;
    widgets[9].input_source_id.clear();

    const auto plan = mirakana::ui::plan_runtime_ui_widgets(widgets, make_state_rows(), make_command_rows());

    MK_REQUIRE(!plan.ready);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWidgetDiagnosticCode::duplicate_widget_id));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWidgetDiagnosticCode::missing_label));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWidgetDiagnosticCode::invalid_slider_range));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWidgetDiagnosticCode::public_native_handle));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWidgetDiagnosticCode::missing_input_source_id));
}

MK_TEST("runtime ui widget vocabulary rejects unsafe command and middleware rows") {
    auto widgets = make_complete_widget_rows();
    widgets[0].middleware_token = "RmlUi.Widget";

    auto commands = make_command_rows();
    commands.push_back({.id = "cmd.missing", .widget_id = "missing.widget", .command_id = "game.missing"});
    commands.push_back({.id = "cmd.native", .widget_id = "pause.resume", .command_id = "native.HWND.focus"});

    const auto plan = mirakana::ui::plan_runtime_ui_widgets(widgets, make_state_rows(), commands);

    MK_REQUIRE(!plan.ready);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWidgetDiagnosticCode::ui_middleware_token));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWidgetDiagnosticCode::missing_command_target));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWidgetDiagnosticCode::native_or_external_token));
}

int main() {
    return mirakana::test::run_all();
}
