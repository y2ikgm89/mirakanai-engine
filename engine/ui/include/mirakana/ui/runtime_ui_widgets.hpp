// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::ui {

enum class RuntimeUiWidgetKind : std::uint8_t {
    button,
    toggle,
    slider,
    text_field,
    menu_stack,
    modal_layer,
    list,
    tree,
    hud_prompt,
    controller_glyph,
};

enum class RuntimeUiWidgetPlanStatus : std::uint8_t {
    invalid_request,
    ready,
    diagnostics,
};

enum class RuntimeUiWidgetDiagnosticCode : std::uint8_t {
    missing_widget_id,
    duplicate_widget_id,
    missing_label,
    invalid_slider_range,
    missing_input_source_id,
    duplicate_state_id,
    missing_state_target,
    duplicate_command_id,
    missing_command_id,
    missing_command_target,
    public_native_handle,
    ui_middleware_token,
    native_or_external_token,
};

struct RuntimeUiWidgetDiagnostic {
    RuntimeUiWidgetDiagnosticCode code{RuntimeUiWidgetDiagnosticCode::missing_widget_id};
    std::string subject;
};

struct RuntimeUiWidgetRow {
    std::string id;
    RuntimeUiWidgetKind kind{RuntimeUiWidgetKind::button};
    std::string label;
    std::string localization_key;
    std::string accessibility_label;
    bool focusable{false};
    bool enabled{true};
    bool visible{true};
    float minimum_value{0.0F};
    float maximum_value{1.0F};
    float value{0.0F};
    std::string input_source_id;
    bool public_native_handle_exposed{false};
    std::string middleware_token;
};

struct RuntimeUiWidgetStateRow {
    std::string id;
    std::string widget_id;
    bool checked{false};
    float value{0.0F};
    std::string text;
};

struct RuntimeUiWidgetCommandRow {
    std::string id;
    std::string widget_id;
    std::string command_id;
    std::string label;
    bool enabled{true};
};

struct RuntimeUiWidgetPlan {
    RuntimeUiWidgetPlanStatus status{RuntimeUiWidgetPlanStatus::invalid_request};
    bool ready{false};
    std::vector<RuntimeUiWidgetRow> widget_rows;
    std::vector<RuntimeUiWidgetStateRow> state_rows;
    std::vector<RuntimeUiWidgetCommandRow> command_rows;
    std::vector<RuntimeUiWidgetDiagnostic> diagnostics;
    std::size_t focusable_widget_rows{0U};
};

[[nodiscard]] std::string_view runtime_ui_widget_kind_name(RuntimeUiWidgetKind kind) noexcept;
[[nodiscard]] std::string_view runtime_ui_widget_plan_status_name(RuntimeUiWidgetPlanStatus status) noexcept;
[[nodiscard]] RuntimeUiWidgetPlan plan_runtime_ui_widgets(std::vector<RuntimeUiWidgetRow> widget_rows,
                                                          std::vector<RuntimeUiWidgetStateRow> state_rows = {},
                                                          std::vector<RuntimeUiWidgetCommandRow> command_rows = {});

} // namespace mirakana::ui
