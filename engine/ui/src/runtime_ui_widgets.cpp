// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui/runtime_ui_widgets.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::ui {

namespace {

void append_diagnostic(std::vector<RuntimeUiWidgetDiagnostic>& diagnostics, RuntimeUiWidgetDiagnosticCode code,
                       std::string subject) {
    diagnostics.push_back(RuntimeUiWidgetDiagnostic{.code = code, .subject = std::move(subject)});
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) noexcept {
    return std::ranges::find(ids, id) != ids.end();
}

[[nodiscard]] bool is_valid_float(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool contains_any_token(std::string_view value, const std::span<const std::string_view> tokens) noexcept {
    return std::ranges::any_of(tokens, [value](std::string_view token) {
        return !token.empty() && value.find(token) != std::string_view::npos;
    });
}

[[nodiscard]] bool contains_middleware_token(std::string_view value) noexcept {
    constexpr std::array<std::string_view, 12U> tokens{
        "DearImGui", "ImGui",   "RmlUi", "Noesis",  "NoesisGUI",  "Slint",
        "Qt",        "QWidget", "SDL3",  "Nuklear", "Ultralight", "CEF",
    };
    return contains_any_token(value, tokens);
}

[[nodiscard]] bool contains_native_or_external_token(std::string_view value) noexcept {
    constexpr std::array<std::string_view, 24U> tokens{
        "native.", "external.", "HWND",    "HANDLE", "IUnknown", "ID3D12", "D3D12", "DXGI",
        "Vk",      "Vulkan",    "MTL",     "Metal",  "Unity",    "Unreal", "Godot", "uGUI",
        "UMG",     "Slate",     "UWidget", "UXML",   "USS",      "Qt",     "RmlUi", "ImGui",
    };
    return contains_any_token(value, tokens);
}

void validate_widget_row(const RuntimeUiWidgetRow& row, std::vector<std::string>& widget_ids,
                         RuntimeUiWidgetPlan& plan) {
    if (row.id.empty()) {
        append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::missing_widget_id, row.label);
    } else if (contains_id(widget_ids, row.id)) {
        append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::duplicate_widget_id, row.id);
    } else {
        widget_ids.push_back(row.id);
    }

    if (row.label.empty()) {
        append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::missing_label, row.id);
    }
    if (row.public_native_handle_exposed) {
        append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::public_native_handle, row.id);
    }
    if (contains_middleware_token(row.middleware_token)) {
        append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::ui_middleware_token, row.id);
    }
    if (contains_native_or_external_token(row.middleware_token)) {
        append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::native_or_external_token, row.id);
    }

    if (row.kind == RuntimeUiWidgetKind::slider &&
        (!is_valid_float(row.minimum_value) || !is_valid_float(row.maximum_value) || !is_valid_float(row.value) ||
         row.maximum_value <= row.minimum_value || row.value < row.minimum_value || row.value > row.maximum_value)) {
        append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::invalid_slider_range, row.id);
    }

    if (row.kind == RuntimeUiWidgetKind::controller_glyph && row.input_source_id.empty()) {
        append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::missing_input_source_id, row.id);
    }

    if (row.focusable) {
        ++plan.focusable_widget_rows;
    }
}

void validate_state_rows(const std::vector<RuntimeUiWidgetStateRow>& rows, const std::vector<std::string>& widget_ids,
                         RuntimeUiWidgetPlan& plan) {
    std::vector<std::string> state_ids;
    state_ids.reserve(rows.size());
    for (const auto& row : rows) {
        if (!row.id.empty() && contains_id(state_ids, row.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::duplicate_state_id, row.id);
        } else if (!row.id.empty()) {
            state_ids.push_back(row.id);
        }
        if (row.widget_id.empty() || !contains_id(widget_ids, row.widget_id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::missing_state_target, row.id);
        }
    }
}

void validate_command_rows(const std::vector<RuntimeUiWidgetCommandRow>& rows,
                           const std::vector<std::string>& widget_ids, RuntimeUiWidgetPlan& plan) {
    std::vector<std::string> command_ids;
    command_ids.reserve(rows.size());
    for (const auto& row : rows) {
        const auto subject = row.id.empty() ? row.command_id : row.id;
        if (!row.id.empty() && contains_id(command_ids, row.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::duplicate_command_id, row.id);
        } else if (!row.id.empty()) {
            command_ids.push_back(row.id);
        }
        if (row.command_id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::missing_command_id, subject);
        }
        if (row.widget_id.empty() || !contains_id(widget_ids, row.widget_id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::missing_command_target, subject);
        }
        if (contains_native_or_external_token(row.command_id) || contains_native_or_external_token(row.label)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::native_or_external_token, subject);
        }
        if (contains_middleware_token(row.command_id) || contains_middleware_token(row.label)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWidgetDiagnosticCode::ui_middleware_token, subject);
        }
    }
}

} // namespace

std::string_view runtime_ui_widget_kind_name(RuntimeUiWidgetKind kind) noexcept {
    switch (kind) {
    case RuntimeUiWidgetKind::button:
        return "button";
    case RuntimeUiWidgetKind::toggle:
        return "toggle";
    case RuntimeUiWidgetKind::slider:
        return "slider";
    case RuntimeUiWidgetKind::text_field:
        return "text_field";
    case RuntimeUiWidgetKind::menu_stack:
        return "menu_stack";
    case RuntimeUiWidgetKind::modal_layer:
        return "modal_layer";
    case RuntimeUiWidgetKind::list:
        return "list";
    case RuntimeUiWidgetKind::tree:
        return "tree";
    case RuntimeUiWidgetKind::hud_prompt:
        return "hud_prompt";
    case RuntimeUiWidgetKind::controller_glyph:
        return "controller_glyph";
    }
    return "unknown";
}

std::string_view runtime_ui_widget_plan_status_name(RuntimeUiWidgetPlanStatus status) noexcept {
    switch (status) {
    case RuntimeUiWidgetPlanStatus::invalid_request:
        return "invalid_request";
    case RuntimeUiWidgetPlanStatus::ready:
        return "ready";
    case RuntimeUiWidgetPlanStatus::diagnostics:
        return "diagnostics";
    }
    return "unknown";
}

RuntimeUiWidgetPlan plan_runtime_ui_widgets(std::vector<RuntimeUiWidgetRow> widget_rows,
                                            std::vector<RuntimeUiWidgetStateRow> state_rows,
                                            std::vector<RuntimeUiWidgetCommandRow> command_rows) {
    RuntimeUiWidgetPlan plan;
    std::vector<std::string> widget_ids;
    widget_ids.reserve(widget_rows.size());

    for (const auto& row : widget_rows) {
        validate_widget_row(row, widget_ids, plan);
    }
    validate_state_rows(state_rows, widget_ids, plan);
    validate_command_rows(command_rows, widget_ids, plan);

    plan.widget_rows = std::move(widget_rows);
    plan.state_rows = std::move(state_rows);
    plan.command_rows = std::move(command_rows);
    plan.ready = plan.diagnostics.empty();
    plan.status = plan.ready ? RuntimeUiWidgetPlanStatus::ready : RuntimeUiWidgetPlanStatus::diagnostics;
    return plan;
}

} // namespace mirakana::ui
