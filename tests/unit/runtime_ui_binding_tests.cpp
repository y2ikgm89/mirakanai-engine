// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ui/runtime_ui_binding.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::ui::ElementId element_id(std::string_view value) {
    return mirakana::ui::ElementId{.value = std::string{value}};
}

void add_element(mirakana::ui::UiDocument& document, std::string_view id, std::string_view parent,
                 mirakana::ui::SemanticRole role) {
    const auto added = document.try_add_element(mirakana::ui::ElementDesc{
        .id = element_id(id),
        .parent = element_id(parent),
        .role = role,
        .bounds = {.x = 0.0F, .y = 0.0F, .width = 120.0F, .height = 32.0F},
        .visible = true,
        .enabled = true,
        .text = {.label = std::string{id}, .localization_key = {}, .font_family = "default"},
        .image = {},
        .accessibility_label = std::string{id},
        .style = {},
    });
    MK_REQUIRE(added);
}

[[nodiscard]] mirakana::ui::UiDocument make_runtime_menu_document() {
    using mirakana::ui::SemanticRole;

    mirakana::ui::UiDocument document;
    add_element(document, "root", {}, SemanticRole::root);
    add_element(document, "hud.hp", "root", SemanticRole::label);
    add_element(document, "hud.prompt", "root", SemanticRole::label);
    add_element(document, "pause.resume", "root", SemanticRole::button);
    add_element(document, "settings.volume", "root", SemanticRole::slider);
    add_element(document, "settings.apply", "root", SemanticRole::button);
    add_element(document, "controller.accept", "root", SemanticRole::image);
    add_element(document, "confirm.modal", "root", SemanticRole::dialog);
    add_element(document, "confirm.ok", "confirm.modal", SemanticRole::button);
    add_element(document, "world.inspect", "root", SemanticRole::button);
    return document;
}

[[nodiscard]] bool has_diagnostic(const std::vector<mirakana::ui::RuntimeUiBindingDiagnostic>& diagnostics,
                                  mirakana::ui::RuntimeUiBindingDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] mirakana::ui::RuntimeUiBindingDocument make_valid_binding_document() {
    using namespace mirakana::ui;

    return RuntimeUiBindingDocument{
        .document = make_runtime_menu_document(),
        .values =
            {
                {.key = "hud.hp.text", .type = RuntimeUiBindingValueType::text, .text = "HP 90 / 100"},
                {.key = "hud.prompt.visible", .type = RuntimeUiBindingValueType::boolean, .boolean = true},
                {.key = "settings.apply.enabled", .type = RuntimeUiBindingValueType::boolean, .boolean = false},
            },
        .binding_rows =
            {
                {.id = "bind.hp.text",
                 .element = element_id("hud.hp"),
                 .source_key = "hud.hp.text",
                 .target = RuntimeUiBindingTarget::text,
                 .expected_type = RuntimeUiBindingValueType::text},
                {.id = "bind.prompt.visible",
                 .element = element_id("hud.prompt"),
                 .source_key = "hud.prompt.visible",
                 .target = RuntimeUiBindingTarget::visible,
                 .expected_type = RuntimeUiBindingValueType::boolean},
                {.id = "bind.apply.enabled",
                 .element = element_id("settings.apply"),
                 .source_key = "settings.apply.enabled",
                 .target = RuntimeUiBindingTarget::enabled,
                 .expected_type = RuntimeUiBindingValueType::boolean},
            },
        .command_rows =
            {
                {.id = "cmd.resume",
                 .command_id = "game.resume",
                 .element = element_id("pause.resume"),
                 .available = true},
                {.id = "cmd.apply",
                 .command_id = "settings.apply",
                 .element = element_id("settings.apply"),
                 .available = false},
            },
        .focus_scopes =
            {
                {.id = "main", .root = element_id("root"), .initial_focus = element_id("pause.resume"), .modal = false},
                {.id = "confirm",
                 .root = element_id("confirm.modal"),
                 .initial_focus = element_id("confirm.ok"),
                 .modal = true},
            },
        .navigation_edges =
            {
                {.id = "nav.resume.volume",
                 .scope_id = "main",
                 .from = element_id("pause.resume"),
                 .to = element_id("settings.volume"),
                 .direction = NavigationDirection::down},
                {.id = "nav.volume.apply",
                 .scope_id = "main",
                 .from = element_id("settings.volume"),
                 .to = element_id("settings.apply"),
                 .direction = NavigationDirection::down},
                {.id = "nav.confirm.ok",
                 .scope_id = "confirm",
                 .from = element_id("confirm.ok"),
                 .to = element_id("confirm.ok"),
                 .direction = NavigationDirection::next},
            },
        .controller_glyph_refs =
            {
                {.id = "glyph.accept",
                 .element = element_id("controller.accept"),
                 .glyph_id = "gamepad.south",
                 .input_source_id = "gamepad"},
            },
        .known_controller_glyph_ids = {"gamepad.south"},
        .pointer_captures =
            {
                {.id = "capture.mouse0", .element = element_id("pause.resume"), .pointer_id = 0U},
            },
        .command_invocations = {},
    };
}

} // namespace

MK_TEST("runtime ui binding applies typed values to document without executing gameplay commands") {
    const auto plan = mirakana::ui::plan_runtime_ui_binding(make_valid_binding_document());

    MK_REQUIRE(plan.ready);
    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiBindingPlanStatus::ready);
    MK_REQUIRE(plan.binding_rows == 3U);
    MK_REQUIRE(plan.command_rows == 2U);
    MK_REQUIRE(plan.focus_scopes == 2U);
    MK_REQUIRE(plan.navigation_edges == 3U);
    MK_REQUIRE(plan.controller_glyph_refs == 1U);
    MK_REQUIRE(plan.input_routing_ready);
    MK_REQUIRE(plan.gameplay_commands_executed == 0U);
    MK_REQUIRE(plan.diagnostics.empty());

    const auto* hp = plan.document.find(element_id("hud.hp"));
    MK_REQUIRE(hp != nullptr);
    MK_REQUIRE(hp->text.label == "HP 90 / 100");

    const auto* prompt = plan.document.find(element_id("hud.prompt"));
    MK_REQUIRE(prompt != nullptr);
    MK_REQUIRE(prompt->visible);

    const auto* apply = plan.document.find(element_id("settings.apply"));
    MK_REQUIRE(apply != nullptr);
    MK_REQUIRE(!apply->enabled);
}

MK_TEST("runtime ui binding reports missing keys type mismatches and unsafe input routes") {
    auto desc = make_valid_binding_document();
    desc.values[0].type = mirakana::ui::RuntimeUiBindingValueType::boolean;
    desc.binding_rows.push_back(mirakana::ui::RuntimeUiBindingRow{
        .id = "bind.missing",
        .element = element_id("hud.hp"),
        .source_key = "missing.hp.text",
        .target = mirakana::ui::RuntimeUiBindingTarget::text,
        .expected_type = mirakana::ui::RuntimeUiBindingValueType::text,
    });
    desc.command_invocations.push_back(mirakana::ui::RuntimeUiCommandInvocationRow{
        .id = "invoke.apply",
        .command_id = "settings.apply",
        .source_element = element_id("settings.apply"),
    });
    desc.navigation_edges.push_back(mirakana::ui::RuntimeUiNavigationEdge{
        .id = "nav.apply.resume",
        .scope_id = "main",
        .from = element_id("settings.apply"),
        .to = element_id("pause.resume"),
        .direction = mirakana::ui::NavigationDirection::down,
    });
    desc.navigation_edges.push_back(mirakana::ui::RuntimeUiNavigationEdge{
        .id = "nav.modal.escape",
        .scope_id = "confirm",
        .from = element_id("confirm.ok"),
        .to = element_id("world.inspect"),
        .direction = mirakana::ui::NavigationDirection::right,
    });
    desc.controller_glyph_refs[0].glyph_id = "gamepad.unknown";
    desc.pointer_captures.push_back(mirakana::ui::RuntimeUiPointerCaptureRow{
        .id = "capture.mouse0.conflict",
        .element = element_id("world.inspect"),
        .pointer_id = 0U,
    });

    const auto plan = mirakana::ui::plan_runtime_ui_binding(std::move(desc));

    MK_REQUIRE(!plan.ready);
    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiBindingPlanStatus::diagnostics);
    MK_REQUIRE(!plan.input_routing_ready);
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiBindingDiagnosticCode::missing_binding_key));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiBindingDiagnosticCode::type_mismatch));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiBindingDiagnosticCode::disabled_command_invocation));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiBindingDiagnosticCode::navigation_cycle));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiBindingDiagnosticCode::modal_focus_escape));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiBindingDiagnosticCode::unknown_controller_glyph_ref));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiBindingDiagnosticCode::pointer_capture_conflict));
    MK_REQUIRE(plan.gameplay_commands_executed == 0U);
}

MK_TEST("runtime ui input routing publishes command focus navigation and capture rows") {
    const auto plan = mirakana::ui::plan_runtime_ui_input_routing(make_valid_binding_document());

    MK_REQUIRE(plan.ready);
    MK_REQUIRE(plan.input_routing_ready);
    MK_REQUIRE(plan.command_rows == 2U);
    MK_REQUIRE(plan.focus_scopes == 2U);
    MK_REQUIRE(plan.navigation_edges == 3U);
    MK_REQUIRE(plan.controller_glyph_refs == 1U);
    MK_REQUIRE(plan.pointer_captures == 1U);
    MK_REQUIRE(plan.gameplay_commands_executed == 0U);
    MK_REQUIRE(plan.diagnostics.empty());
}

int main() {
    return mirakana::test::run_all();
}
