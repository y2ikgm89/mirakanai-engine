// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/runtime_ui_binding.hpp"
#include "mirakana/ui/runtime_ui_widgets.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::ui {

enum class RuntimeUiAuthoringDiagnosticCode : std::uint8_t {
    invalid_format_header,
    malformed_row,
    duplicate_element_id,
    unknown_parent_id,
    invalid_theme_token,
    unsafe_token,
    missing_localization_key,
    missing_accessibility_name,
    unknown_widget_id,
    missing_widget_id,
    duplicate_widget_id,
    duplicate_binding_id,
    unknown_element_id,
};

enum class RuntimeUiThemeTokenKind : std::uint8_t {
    color,
    spacing,
    typography,
    border,
    opacity,
    transition,
    controller_glyph,
};

struct RuntimeUiAuthoringDiagnostic {
    RuntimeUiAuthoringDiagnosticCode code{RuntimeUiAuthoringDiagnosticCode::malformed_row};
    std::string subject;
    std::string message;
    std::size_t line{0U};
};

struct RuntimeUiAuthoredElementRow {
    std::string id;
    std::string parent_id;
    SemanticRole role{SemanticRole::none};
    std::string widget_id;
    std::string localization_key;
    std::string accessibility_name;
    std::string style_token;
};

struct RuntimeUiAuthoredWidgetRow {
    std::string id;
    RuntimeUiWidgetKind kind{RuntimeUiWidgetKind::button};
    std::string label;
};

struct RuntimeUiAuthoredBindingRow {
    std::string id;
    std::string element_id;
    std::string source_key;
    RuntimeUiBindingTarget target{RuntimeUiBindingTarget::text};
};

struct RuntimeUiThemeTokenRow {
    RuntimeUiThemeTokenKind kind{RuntimeUiThemeTokenKind::color};
    std::string id;
    std::string value;
};

struct RuntimeUiAuthoringDocument {
    std::string format_id{"GameEngine.UiDocument.v1"};
    std::vector<RuntimeUiAuthoredElementRow> elements;
    std::vector<RuntimeUiAuthoredWidgetRow> widgets;
    std::vector<RuntimeUiAuthoredBindingRow> bindings;
};

struct RuntimeUiThemeDocument {
    std::string format_id{"GameEngine.UiTheme.v1"};
    std::vector<RuntimeUiThemeTokenRow> tokens;
};

struct RuntimeUiAuthoringDocumentParseResult {
    bool ready{false};
    RuntimeUiAuthoringDocument document;
    std::vector<RuntimeUiAuthoringDiagnostic> diagnostics;
};

struct RuntimeUiThemeParseResult {
    bool ready{false};
    RuntimeUiThemeDocument theme;
    std::vector<RuntimeUiAuthoringDiagnostic> diagnostics;
};

[[nodiscard]] std::string_view runtime_ui_theme_token_kind_name(RuntimeUiThemeTokenKind kind) noexcept;
[[nodiscard]] RuntimeUiAuthoringDocumentParseResult parse_runtime_ui_document(std::string_view text);
[[nodiscard]] std::string write_runtime_ui_document(const RuntimeUiAuthoringDocument& document);
[[nodiscard]] RuntimeUiThemeParseResult parse_runtime_ui_theme(std::string_view text);
[[nodiscard]] std::string write_runtime_ui_theme(const RuntimeUiThemeDocument& theme);

} // namespace mirakana::ui
