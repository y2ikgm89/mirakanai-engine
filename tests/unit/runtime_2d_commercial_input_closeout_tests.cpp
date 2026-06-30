// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/two_d_commercial_input_closeout.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace {

using mirakana::runtime::Runtime2DCommercialInputCloseoutDesc;
using mirakana::runtime::Runtime2DCommercialInputCloseoutDiagnostic;
using mirakana::runtime::Runtime2DCommercialInputCloseoutDiagnosticCode;
using mirakana::runtime::Runtime2DCommercialInputNavigationRow;
using mirakana::runtime::Runtime2DCommercialInputOfficialSourceKind;
using mirakana::runtime::Runtime2DCommercialInputOfficialSourceRow;

[[nodiscard]] mirakana::runtime::RuntimeInputActionTrigger key_trigger(mirakana::Key key) noexcept {
    return mirakana::runtime::RuntimeInputActionTrigger{.kind = mirakana::runtime::RuntimeInputActionTriggerKind::key,
                                                        .key = key};
}

[[nodiscard]] mirakana::runtime::RuntimeInputActionTrigger
gamepad_button_trigger(mirakana::GamepadId gamepad_id, mirakana::GamepadButton button) noexcept {
    return mirakana::runtime::RuntimeInputActionTrigger{
        .kind = mirakana::runtime::RuntimeInputActionTriggerKind::gamepad_button,
        .gamepad_id = gamepad_id,
        .gamepad_button = button,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeInputAxisSource
gamepad_axis_source(mirakana::GamepadId gamepad_id, mirakana::GamepadAxis axis, float scale, float deadzone) noexcept {
    return mirakana::runtime::RuntimeInputAxisSource{
        .kind = mirakana::runtime::RuntimeInputAxisSourceKind::gamepad_axis,
        .gamepad_id = gamepad_id,
        .gamepad_axis = axis,
        .scale = scale,
        .deadzone = deadzone,
    };
}

[[nodiscard]] bool has_diagnostic(const std::vector<Runtime2DCommercialInputCloseoutDiagnostic>& diagnostics,
                                  Runtime2DCommercialInputCloseoutDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] mirakana::runtime::RuntimeInputActionMap make_base_actions() {
    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_key_in_context("gameplay", "cancel", mirakana::Key::escape);
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);
    base.bind_gamepad_button_in_context("gameplay", "dash", mirakana::GamepadId{1}, mirakana::GamepadButton::south);
    return base;
}

[[nodiscard]] mirakana::runtime::RuntimeInputRebindingProfile make_profile() {
    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {gamepad_button_trigger(mirakana::GamepadId{1}, mirakana::GamepadButton::north),
                     key_trigger(mirakana::Key::space)}});
    profile.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move_x",
        .sources = {gamepad_axis_source(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x, 1.0F, 0.25F)}});
    return profile;
}

[[nodiscard]] mirakana::runtime::RuntimeInputDeviceProductionUxPlan
make_device_ux_plan(const mirakana::runtime::RuntimeInputActionMap& base) {
    mirakana::runtime::RuntimeInputDeviceProductionUxRequest request;
    request.base_actions = base;
    request.gesture_bindings = {
        mirakana::runtime::RuntimeInputGestureBindingDesc{.context = "gameplay",
                                                          .action = "confirm",
                                                          .gesture = mirakana::TouchGestureKind::tap,
                                                          .phase = mirakana::TouchGesturePhase::ended},
        mirakana::runtime::RuntimeInputGestureBindingDesc{.context = "gameplay",
                                                          .action = "move_x",
                                                          .gesture = mirakana::TouchGestureKind::pan,
                                                          .phase = mirakana::TouchGesturePhase::changed},
    };
    request.gesture_events = {
        mirakana::TouchGestureEvent{.kind = mirakana::TouchGestureKind::tap,
                                    .phase = mirakana::TouchGesturePhase::ended,
                                    .primary_pointer_id = mirakana::primary_pointer_id,
                                    .touch_count = 1U},
        mirakana::TouchGestureEvent{.kind = mirakana::TouchGestureKind::pan,
                                    .phase = mirakana::TouchGesturePhase::changed,
                                    .primary_pointer_id = mirakana::primary_pointer_id,
                                    .touch_count = 1U,
                                    .delta = mirakana::Vec2{.x = 6.0F, .y = 0.0F}},
    };
    request.device_assignments = {
        mirakana::runtime::RuntimeInputDeviceAssignmentDesc{
            .player_id = "player_one",
            .device_id = "keyboard_mouse:0",
            .device_kind = mirakana::runtime::RuntimeInputDeviceKind::keyboard_mouse,
            .profile_id = "player_one_keyboard"},
        mirakana::runtime::RuntimeInputDeviceAssignmentDesc{.player_id = "player_two",
                                                            .device_id = "gamepad:1",
                                                            .device_kind =
                                                                mirakana::runtime::RuntimeInputDeviceKind::gamepad,
                                                            .profile_id = "player_two_gamepad"},
    };
    request.per_device_profiles = {
        mirakana::runtime::RuntimeInputPerDeviceProfileDesc{
            .profile_id = "player_two_gamepad",
            .device_id = "gamepad:1",
            .device_kind = mirakana::runtime::RuntimeInputDeviceKind::gamepad,
            .left_stick_radial_deadzone = 0.35F,
            .right_stick_radial_deadzone = 0.20F,
            .response_curve = mirakana::runtime::RuntimeInputStickResponseCurve::squared},
    };
    request.glyph_assets = {
        mirakana::runtime::RuntimeInputGlyphAssetDesc{
            .glyph_lookup_key = "gamepad.1.button.south", .platform = "xbox", .asset_id = "ui/input/xbox_button_south"},
        mirakana::runtime::RuntimeInputGlyphAssetDesc{
            .glyph_lookup_key = "keyboard.key.space", .platform = "keyboard", .asset_id = "ui/input/key_space"},
    };
    request.keyboard_layout_labels = {
        mirakana::runtime::RuntimeInputKeyboardLayoutLabelDesc{.layout_id = "en-US",
                                                               .physical_key = mirakana::Key::space,
                                                               .logical_key = mirakana::Key::space,
                                                               .display_label = "Space"},
        mirakana::runtime::RuntimeInputKeyboardLayoutLabelDesc{.layout_id = "jp-JIS",
                                                               .physical_key = mirakana::Key::space,
                                                               .logical_key = mirakana::Key::space,
                                                               .display_label = "Space"},
    };
    return mirakana::runtime::plan_runtime_input_device_production_ux(request);
}

[[nodiscard]] std::vector<Runtime2DCommercialInputNavigationRow> make_navigation_rows() {
    return {
        Runtime2DCommercialInputNavigationRow{
            .id = "input.nav.pause.resume",
            .context = "gameplay",
            .action = "confirm",
            .focus_scope_id = "pause.menu",
            .keyboard_glyph_lookup_key = "keyboard.key.space",
            .controller_glyph_lookup_key = "gamepad.1.button.south",
            .accessibility_label = "Resume",
            .reachable_by_keyboard = true,
            .reachable_by_controller = true,
            .selected = true,
            .ready = true,
        },
        Runtime2DCommercialInputNavigationRow{
            .id = "input.nav.pause.cancel",
            .context = "gameplay",
            .action = "cancel",
            .focus_scope_id = "pause.menu",
            .keyboard_glyph_lookup_key = "keyboard.key.escape",
            .controller_glyph_lookup_key = "gamepad.1.button.east",
            .accessibility_label = "Cancel",
            .reachable_by_keyboard = true,
            .reachable_by_controller = true,
            .selected = true,
            .ready = true,
        },
    };
}

[[nodiscard]] std::array<Runtime2DCommercialInputOfficialSourceRow, 5U> make_official_source_rows() {
    return {
        Runtime2DCommercialInputOfficialSourceRow{
            .id = "microsoft.learn.gameinput",
            .kind = Runtime2DCommercialInputOfficialSourceKind::microsoft_gameinput,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialInputOfficialSourceRow{
            .id = "microsoft.learn.raw-input",
            .kind = Runtime2DCommercialInputOfficialSourceKind::microsoft_raw_input,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialInputOfficialSourceRow{
            .id = "microsoft.learn.pointer-input",
            .kind = Runtime2DCommercialInputOfficialSourceKind::microsoft_pointer_input,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialInputOfficialSourceRow{
            .id = "microsoft.learn.uia",
            .kind = Runtime2DCommercialInputOfficialSourceKind::microsoft_uia,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialInputOfficialSourceRow{
            .id = "repository.legal-policy",
            .kind = Runtime2DCommercialInputOfficialSourceKind::repository_legal_policy,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
    };
}

[[nodiscard]] Runtime2DCommercialInputCloseoutDesc make_ready_desc() {
    auto base = make_base_actions();
    return Runtime2DCommercialInputCloseoutDesc{
        .base_actions = base,
        .rebinding_profile = make_profile(),
        .device_ux_plan = make_device_ux_plan(base),
        .navigation_rows = make_navigation_rows(),
        .official_source_rows = make_official_source_rows(),
        .selected_2d_package_ready_claim = true,
    };
}

} // namespace

MK_TEST("runtime 2d commercial input closeout accepts selected package evidence") {
    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_input_closeout(make_ready_desc());

    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.action_map_ready);
    MK_REQUIRE(result.rebinding_ready);
    MK_REQUIRE(result.device_ux_ready);
    MK_REQUIRE(result.accessibility_navigation_ready);
    MK_REQUIRE(result.official_source_ready);
    MK_REQUIRE(result.action_binding_rows == 3U);
    MK_REQUIRE(result.axis_binding_rows == 1U);
    MK_REQUIRE(result.profile_overlay_rows == 2U);
    MK_REQUIRE(result.presentation_rows == 4U);
    MK_REQUIRE(result.presentation_glyph_lookup_keys >= 5U);
    MK_REQUIRE(result.keyboard_mouse_device_rows == 1U);
    MK_REQUIRE(result.gamepad_device_rows == 1U);
    MK_REQUIRE(result.touch_gesture_rows == 2U);
    MK_REQUIRE(result.multiplayer_device_assignment_rows == 2U);
    MK_REQUIRE(result.accessibility_navigation_rows == 2U);
    MK_REQUIRE(result.keyboard_accessible_rows == 2U);
    MK_REQUIRE(result.controller_accessible_rows == 2U);
    MK_REQUIRE(result.native_handle_access_rows == 0U);
    MK_REQUIRE(result.input_middleware_rows == 0U);
    MK_REQUIRE(result.external_engine_claim_rows == 0U);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime 2d commercial input closeout rejects missing navigation and official source rows") {
    auto desc = make_ready_desc();
    desc.navigation_rows[0].ready = false;
    desc.official_source_rows[2].ready = false;

    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_input_closeout(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              Runtime2DCommercialInputCloseoutDiagnosticCode::accessibility_navigation_not_ready));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::official_source_not_ready));
}

MK_TEST("runtime 2d commercial input closeout rejects unsafe platform and legal claims") {
    auto desc = make_ready_desc();
    desc.public_native_handles = true;
    desc.input_middleware_claim = true;
    desc.external_engine_compatibility_claim = true;
    desc.cross_platform_parity_claim = true;
    desc.legal_approval_claim = true;

    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_input_closeout(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(result.native_handle_access_rows == 1U);
    MK_REQUIRE(result.input_middleware_rows == 1U);
    MK_REQUIRE(result.external_engine_claim_rows == 1U);
    MK_REQUIRE(result.cross_platform_parity_claim_rows == 1U);
    MK_REQUIRE(result.legal_approval_claim_rows == 1U);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::public_native_handles));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::input_middleware_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              Runtime2DCommercialInputCloseoutDiagnosticCode::external_engine_compatibility_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              Runtime2DCommercialInputCloseoutDiagnosticCode::cross_platform_parity_claim));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::legal_approval_claim));
}

int main() {
    return mirakana::test::run_all();
}
