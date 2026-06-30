// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/two_d_commercial_input_closeout.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {

namespace {

constexpr std::size_t kOfficialSourceKindCount{5U};

void append_diagnostic(std::vector<Runtime2DCommercialInputCloseoutDiagnostic>& diagnostics,
                       Runtime2DCommercialInputCloseoutDiagnosticCode code, std::string row_id, std::string message) {
    diagnostics.push_back(Runtime2DCommercialInputCloseoutDiagnostic{
        .code = code,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool action_or_axis_exists(const RuntimeInputActionMap& actions,
                                         const Runtime2DCommercialInputNavigationRow& row) noexcept {
    return actions.find(row.context, row.action) != nullptr || actions.find_axis(row.context, row.action) != nullptr;
}

[[nodiscard]] std::size_t count_device_ux_diagnostic(const RuntimeInputDeviceProductionUxPlan& plan,
                                                     RuntimeInputDeviceProductionUxDiagnosticCode code) {
    return static_cast<std::size_t>(std::ranges::count_if(
        plan.diagnostics, [code](const auto& diagnostic) noexcept { return diagnostic.code == code; }));
}

[[nodiscard]] bool row_ready(const RuntimeInputDeviceAssignmentRow& row) noexcept {
    return row.ready && row.diagnostic == "device assigned to runtime input profile";
}

[[nodiscard]] bool row_ready(const RuntimeInputGestureBindingRow& row) noexcept {
    return row.ready && row.mapped_to_action && row.matching_event_rows > 0U;
}

void evaluate_action_map(const Runtime2DCommercialInputCloseoutDesc& desc,
                         Runtime2DCommercialInputCloseoutResult& result) {
    result.action_binding_rows = desc.base_actions.bindings().size();
    result.axis_binding_rows = desc.base_actions.axis_bindings().size();
    try {
        const auto serialized = serialize_runtime_input_actions(desc.base_actions);
        result.action_map_ready =
            !serialized.empty() && result.action_binding_rows > 0U && result.axis_binding_rows > 0U;
    } catch (const std::exception& error) {
        append_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::action_map_not_ready, {},
                          error.what());
        result.action_map_ready = false;
    }
    if (!result.action_map_ready) {
        append_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::action_map_not_ready, {},
                          "2D commercial input closeout requires reviewed action and axis bindings");
    }
}

void evaluate_rebinding(const Runtime2DCommercialInputCloseoutDesc& desc,
                        Runtime2DCommercialInputCloseoutResult& result) {
    result.profile_overlay_rows =
        desc.rebinding_profile.action_overrides.size() + desc.rebinding_profile.axis_overrides.size();

    const auto applied = apply_runtime_input_rebinding_profile(desc.base_actions, desc.rebinding_profile);
    const auto presentation = make_runtime_input_rebinding_presentation(desc.base_actions, desc.rebinding_profile);
    result.presentation_rows = presentation.rows.size();
    for (const auto& row : presentation.rows) {
        const auto count_token = [&result](const RuntimeInputRebindingPresentationToken& token) {
            if (!token.glyph_lookup_key.empty()) {
                ++result.presentation_glyph_lookup_keys;
            }
        };
        std::ranges::for_each(row.base_tokens, count_token);
        std::ranges::for_each(row.profile_tokens, count_token);
    }

    result.rebinding_ready = applied.succeeded() && presentation.ready() && result.profile_overlay_rows > 0U &&
                             result.presentation_rows > 0U && result.presentation_glyph_lookup_keys > 0U;
    if (!result.rebinding_ready) {
        append_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::rebinding_not_ready, {},
                          "2D commercial input closeout requires applied rebinding overlays and symbolic "
                          "presentation glyph keys");
    }
}

void evaluate_device_ux(const Runtime2DCommercialInputCloseoutDesc& desc,
                        Runtime2DCommercialInputCloseoutResult& result) {
    const auto& plan = desc.device_ux_plan;
    for (const auto& row : plan.device_assignment_rows) {
        if (!row_ready(row)) {
            continue;
        }
        ++result.multiplayer_device_assignment_rows;
        switch (row.device_kind) {
        case RuntimeInputDeviceKind::keyboard_mouse:
            ++result.keyboard_mouse_device_rows;
            break;
        case RuntimeInputDeviceKind::gamepad:
            ++result.gamepad_device_rows;
            break;
        case RuntimeInputDeviceKind::touch:
        case RuntimeInputDeviceKind::unknown:
            break;
        }
    }
    result.touch_gesture_rows = static_cast<std::size_t>(
        std::ranges::count_if(plan.gesture_binding_rows, [](const auto& row) noexcept { return row_ready(row); }));
    result.per_device_profile_rows = static_cast<std::size_t>(std::ranges::count_if(
        plan.per_device_profile_rows, [](const auto& row) noexcept { return row.ready && !row.clamped_deadzone; }));
    result.glyph_asset_lookup_rows =
        static_cast<std::size_t>(std::ranges::count_if(plan.glyph_asset_lookup_rows, [](const auto& row) noexcept {
            return row.ready && !row.renders_glyph && !row.creates_ui_widget;
        }));
    result.keyboard_layout_label_rows = static_cast<std::size_t>(
        std::ranges::count_if(plan.keyboard_layout_label_rows, [](const auto& row) noexcept { return row.ready; }));
    result.native_handle_access_rows = plan.native_handle_access_rows;
    result.input_middleware_rows = plan.input_middleware_rows;
    result.ui_rendering_rows = plan.ui_rendering_rows;
    result.glyph_rendering_rows =
        count_device_ux_diagnostic(plan, RuntimeInputDeviceProductionUxDiagnosticCode::glyph_rendering_requested);
    result.ui_widget_rows =
        count_device_ux_diagnostic(plan, RuntimeInputDeviceProductionUxDiagnosticCode::ui_widget_requested);

    result.device_ux_ready = plan.ready() && result.keyboard_mouse_device_rows > 0U &&
                             result.gamepad_device_rows > 0U && result.touch_gesture_rows > 0U &&
                             result.multiplayer_device_assignment_rows >= 2U && result.per_device_profile_rows > 0U &&
                             result.glyph_asset_lookup_rows > 0U && result.keyboard_layout_label_rows > 0U &&
                             result.native_handle_access_rows == 0U && result.input_middleware_rows == 0U &&
                             result.ui_rendering_rows == 0U && result.glyph_rendering_rows == 0U &&
                             result.ui_widget_rows == 0U;
    if (!result.device_ux_ready) {
        append_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::device_ux_not_ready, {},
                          "2D commercial input closeout requires keyboard/mouse, gamepad, touch gesture, "
                          "multiplayer assignment, per-device profile, glyph, and keyboard label rows");
    }
}

void evaluate_navigation(const Runtime2DCommercialInputCloseoutDesc& desc,
                         Runtime2DCommercialInputCloseoutResult& result) {
    bool navigation_rows_ready = true;
    for (const auto& row : desc.navigation_rows) {
        if (!row.selected) {
            continue;
        }
        ++result.accessibility_navigation_rows;
        if (row.reachable_by_keyboard) {
            ++result.keyboard_accessible_rows;
        }
        if (row.reachable_by_controller) {
            ++result.controller_accessible_rows;
        }
        const bool valid_row = row.ready && !row.id.empty() && !row.focus_scope_id.empty() &&
                               !row.accessibility_label.empty() && !row.keyboard_glyph_lookup_key.empty() &&
                               !row.controller_glyph_lookup_key.empty() && row.reachable_by_keyboard &&
                               row.reachable_by_controller && action_or_axis_exists(desc.base_actions, row);
        if (!valid_row) {
            navigation_rows_ready = false;
            append_diagnostic(result.diagnostics,
                              Runtime2DCommercialInputCloseoutDiagnosticCode::accessibility_navigation_not_ready,
                              row.id,
                              "2D commercial input navigation rows must be selected, action-backed, keyboard and "
                              "controller reachable, glyph-backed, and accessibility-labelled");
        }
    }
    result.accessibility_navigation_ready = navigation_rows_ready && result.accessibility_navigation_rows > 0U &&
                                            result.keyboard_accessible_rows > 0U &&
                                            result.controller_accessible_rows > 0U;
    if (!result.accessibility_navigation_ready && result.accessibility_navigation_rows == 0U) {
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialInputCloseoutDiagnosticCode::accessibility_navigation_not_ready, {},
                          "2D commercial input closeout requires selected accessibility-friendly navigation rows");
    }
}

void evaluate_official_sources(const Runtime2DCommercialInputCloseoutDesc& desc,
                               Runtime2DCommercialInputCloseoutResult& result) {
    result.official_source_rows = desc.official_source_rows.size();
    std::array<bool, kOfficialSourceKindCount> seen_kinds{};
    result.official_source_ready = true;

    for (const auto& row : desc.official_source_rows) {
        const auto index = static_cast<std::size_t>(row.kind);
        if (index < seen_kinds.size()) {
            seen_kinds[index] = true;
        }
        if (row.id.empty() || !row.ready || !row.official || !row.public_docs_only) {
            result.official_source_ready = false;
            append_diagnostic(result.diagnostics,
                              Runtime2DCommercialInputCloseoutDiagnosticCode::official_source_not_ready, row.id,
                              "2D commercial input closeout official source rows must be ready, official, and "
                              "public-docs-only");
        }
    }

    for (std::size_t index = 0U; index < seen_kinds.size(); ++index) {
        if (seen_kinds[index]) {
            continue;
        }
        result.official_source_ready = false;
        const auto kind = static_cast<Runtime2DCommercialInputOfficialSourceKind>(index);
        append_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::official_source_not_ready,
                          {},
                          "2D commercial input closeout is missing official source row " +
                              std::string{runtime_2d_commercial_input_official_source_kind_name(kind)});
    }
}

void evaluate_unsafe_claims(const Runtime2DCommercialInputCloseoutDesc& desc,
                            Runtime2DCommercialInputCloseoutResult& result) {
    if (!desc.selected_2d_package_ready_claim) {
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialInputCloseoutDiagnosticCode::selected_package_claim_missing, {},
                          "2D commercial input closeout must explicitly scope the ready claim to the selected 2D "
                          "package evidence");
    }
    if (desc.public_native_handles) {
        ++result.native_handle_access_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::public_native_handles, {},
                          "2D commercial input closeout must not expose platform, device, or middleware native "
                          "handles");
    }
    if (desc.input_middleware_claim) {
        ++result.input_middleware_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::input_middleware_claim,
                          {}, "2D commercial input closeout must not claim input middleware integration");
    }
    if (desc.external_engine_compatibility_claim) {
        ++result.external_engine_claim_rows;
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialInputCloseoutDiagnosticCode::external_engine_compatibility_claim, {},
                          "2D commercial input closeout must not claim Unity, Unreal Engine, Godot, or third-party "
                          "engine compatibility");
    }
    if (desc.cross_platform_parity_claim) {
        ++result.cross_platform_parity_claim_rows;
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialInputCloseoutDiagnosticCode::cross_platform_parity_claim, {},
                          "2D commercial input closeout must not infer cross-platform input parity from selected "
                          "package evidence");
    }
    if (desc.legal_approval_claim) {
        ++result.legal_approval_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialInputCloseoutDiagnosticCode::legal_approval_claim, {},
                          "2D commercial input closeout can provide engineering review input but not legal approval");
    }
}

} // namespace

bool Runtime2DCommercialInputCloseoutResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

std::string_view
runtime_2d_commercial_input_official_source_kind_name(Runtime2DCommercialInputOfficialSourceKind kind) noexcept {
    switch (kind) {
    case Runtime2DCommercialInputOfficialSourceKind::microsoft_gameinput:
        return "microsoft_gameinput";
    case Runtime2DCommercialInputOfficialSourceKind::microsoft_raw_input:
        return "microsoft_raw_input";
    case Runtime2DCommercialInputOfficialSourceKind::microsoft_pointer_input:
        return "microsoft_pointer_input";
    case Runtime2DCommercialInputOfficialSourceKind::microsoft_uia:
        return "microsoft_uia";
    case Runtime2DCommercialInputOfficialSourceKind::repository_legal_policy:
        return "repository_legal_policy";
    }
    return "unknown";
}

Runtime2DCommercialInputCloseoutResult
evaluate_runtime_2d_commercial_input_closeout(const Runtime2DCommercialInputCloseoutDesc& desc) {
    Runtime2DCommercialInputCloseoutResult result;
    evaluate_action_map(desc, result);
    evaluate_rebinding(desc, result);
    evaluate_device_ux(desc, result);
    evaluate_navigation(desc, result);
    evaluate_official_sources(desc, result);
    evaluate_unsafe_claims(desc, result);
    result.ready = result.action_map_ready && result.rebinding_ready && result.device_ux_ready &&
                   result.accessibility_navigation_ready && result.official_source_ready &&
                   desc.selected_2d_package_ready_claim && result.native_handle_access_rows == 0U &&
                   result.input_middleware_rows == 0U && result.external_engine_claim_rows == 0U &&
                   result.cross_platform_parity_claim_rows == 0U && result.legal_approval_claim_rows == 0U &&
                   result.diagnostics.empty();
    return result;
}

} // namespace mirakana::runtime
