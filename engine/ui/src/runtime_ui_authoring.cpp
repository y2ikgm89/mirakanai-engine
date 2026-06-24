// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui/runtime_ui_authoring.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace mirakana::ui {

namespace {

constexpr std::string_view kUiDocumentFormatId = "GameEngine.UiDocument.v1";
constexpr std::string_view kUiThemeFormatId = "GameEngine.UiTheme.v1";

struct Field {
    std::string_view key;
    std::string_view value;
};

struct ParsedLine {
    std::string_view row_type;
    std::vector<Field> fields;
    bool malformed{false};
};

void append_diagnostic(std::vector<RuntimeUiAuthoringDiagnostic>& diagnostics, RuntimeUiAuthoringDiagnosticCode code,
                       std::string subject, std::string message, std::size_t line) {
    diagnostics.push_back(RuntimeUiAuthoringDiagnostic{
        .code = code,
        .subject = std::move(subject),
        .message = std::move(message),
        .line = line,
    });
}

[[nodiscard]] std::string_view trim_line_end(std::string_view line) noexcept {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.remove_suffix(1U);
    }
    return line;
}

[[nodiscard]] std::vector<std::string_view> split_lines(std::string_view text) {
    std::vector<std::string_view> lines;
    std::size_t start = 0U;
    while (start <= text.size()) {
        const auto next = text.find('\n', start);
        const auto end = next == std::string_view::npos ? text.size() : next;
        lines.push_back(trim_line_end(text.substr(start, end - start)));
        if (next == std::string_view::npos) {
            break;
        }
        start = next + 1U;
    }
    return lines;
}

[[nodiscard]] ParsedLine parse_line(std::string_view line) {
    ParsedLine parsed;
    const auto first_space = line.find(' ');
    parsed.row_type = first_space == std::string_view::npos ? line : line.substr(0U, first_space);
    std::size_t cursor = first_space == std::string_view::npos ? line.size() : first_space + 1U;

    while (cursor < line.size()) {
        while (cursor < line.size() && line[cursor] == ' ') {
            ++cursor;
        }
        if (cursor >= line.size()) {
            break;
        }

        const auto next_space = line.find(' ', cursor);
        const auto token_end = next_space == std::string_view::npos ? line.size() : next_space;
        const auto token = line.substr(cursor, token_end - cursor);
        const auto equal = token.find('=');
        if (equal == std::string_view::npos) {
            parsed.malformed = true;
        } else {
            parsed.fields.push_back(Field{.key = token.substr(0U, equal), .value = token.substr(equal + 1U)});
        }
        cursor = token_end;
    }

    return parsed;
}

[[nodiscard]] std::optional<std::string_view> field_value(const ParsedLine& line, std::string_view key) noexcept {
    const auto it = std::ranges::find_if(line.fields, [key](const Field& field) { return field.key == key; });
    if (it == line.fields.end()) {
        return std::nullopt;
    }
    return it->value;
}

[[nodiscard]] std::string value_or_empty(const ParsedLine& line, std::string_view key) {
    if (const auto value = field_value(line, key); value.has_value()) {
        return std::string{*value};
    }
    return {};
}

[[nodiscard]] std::string ascii_lower(std::string_view value) {
    std::string lowered;
    lowered.reserve(value.size());
    for (const auto character : value) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }
    return lowered;
}

[[nodiscard]] bool contains_unsafe_path(std::string_view value) noexcept {
    return value.find("..") != std::string_view::npos || value.find('\\') != std::string_view::npos ||
           value.find('/') != std::string_view::npos || value.find(':') != std::string_view::npos;
}

[[nodiscard]] bool contains_external_or_native_token(std::string_view value) {
    const auto lowered = ascii_lower(value);
    return lowered.find("unity") != std::string::npos || lowered.find("unityengine") != std::string::npos ||
           lowered.find("unreal") != std::string::npos || lowered.find("godot") != std::string::npos ||
           lowered.find("uxml") != std::string::npos || lowered.find("uss") != std::string::npos ||
           lowered.find("umg") != std::string::npos || lowered.find("slate") != std::string::npos ||
           lowered.find("visualelement") != std::string::npos || lowered.find("uidocument") != std::string::npos ||
           lowered.find("imgui") != std::string::npos || lowered.find("qt") != std::string::npos ||
           lowered.find("slint") != std::string::npos || lowered.find("rmlui") != std::string::npos ||
           lowered.find("noesis") != std::string::npos || lowered.find("hwnd") != std::string::npos ||
           lowered.find("native_handle") != std::string::npos || lowered.find("rhi_handle") != std::string::npos;
}

[[nodiscard]] bool is_unsafe_token(std::string_view value) {
    return contains_unsafe_path(value) || contains_external_or_native_token(value);
}

void validate_safe_token(std::vector<RuntimeUiAuthoringDiagnostic>& diagnostics, std::string_view value,
                         std::string_view subject, std::size_t line) {
    if (value.empty()) {
        return;
    }
    if (is_unsafe_token(value)) {
        append_diagnostic(diagnostics, RuntimeUiAuthoringDiagnosticCode::unsafe_token, std::string{subject},
                          "runtime UI authoring token must stay first-party and path-safe", line);
    }
}

[[nodiscard]] bool contains_string(const std::vector<std::string>& values, std::string_view value) noexcept {
    return std::ranges::find(values, value) != values.end();
}

[[nodiscard]] bool has_element(const RuntimeUiAuthoringDocument& document, std::string_view id) noexcept {
    return std::ranges::any_of(document.elements,
                               [id](const RuntimeUiAuthoredElementRow& row) { return row.id == id; });
}

[[nodiscard]] bool has_widget(const RuntimeUiAuthoringDocument& document, std::string_view id) noexcept {
    return std::ranges::any_of(document.widgets, [id](const RuntimeUiAuthoredWidgetRow& row) { return row.id == id; });
}

[[nodiscard]] std::optional<SemanticRole> parse_semantic_role(std::string_view value) noexcept {
    if (value == "none") {
        return SemanticRole::none;
    }
    if (value == "root") {
        return SemanticRole::root;
    }
    if (value == "panel") {
        return SemanticRole::panel;
    }
    if (value == "button") {
        return SemanticRole::button;
    }
    if (value == "label") {
        return SemanticRole::label;
    }
    if (value == "text_field") {
        return SemanticRole::text_field;
    }
    if (value == "list") {
        return SemanticRole::list;
    }
    if (value == "list_item") {
        return SemanticRole::list_item;
    }
    if (value == "image") {
        return SemanticRole::image;
    }
    if (value == "checkbox") {
        return SemanticRole::checkbox;
    }
    if (value == "slider") {
        return SemanticRole::slider;
    }
    if (value == "meter") {
        return SemanticRole::meter;
    }
    if (value == "dialog") {
        return SemanticRole::dialog;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<RuntimeUiWidgetKind> parse_widget_kind(std::string_view value) noexcept {
    if (value == "button") {
        return RuntimeUiWidgetKind::button;
    }
    if (value == "toggle") {
        return RuntimeUiWidgetKind::toggle;
    }
    if (value == "slider") {
        return RuntimeUiWidgetKind::slider;
    }
    if (value == "text_field") {
        return RuntimeUiWidgetKind::text_field;
    }
    if (value == "menu_stack") {
        return RuntimeUiWidgetKind::menu_stack;
    }
    if (value == "modal_layer") {
        return RuntimeUiWidgetKind::modal_layer;
    }
    if (value == "list") {
        return RuntimeUiWidgetKind::list;
    }
    if (value == "tree") {
        return RuntimeUiWidgetKind::tree;
    }
    if (value == "hud_prompt") {
        return RuntimeUiWidgetKind::hud_prompt;
    }
    if (value == "controller_glyph") {
        return RuntimeUiWidgetKind::controller_glyph;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<RuntimeUiBindingTarget> parse_binding_target(std::string_view value) noexcept {
    if (value == "text") {
        return RuntimeUiBindingTarget::text;
    }
    if (value == "enabled") {
        return RuntimeUiBindingTarget::enabled;
    }
    if (value == "visible") {
        return RuntimeUiBindingTarget::visible;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<RuntimeUiThemeTokenKind> parse_theme_token_kind(std::string_view value) noexcept {
    if (value == "color") {
        return RuntimeUiThemeTokenKind::color;
    }
    if (value == "spacing") {
        return RuntimeUiThemeTokenKind::spacing;
    }
    if (value == "typography") {
        return RuntimeUiThemeTokenKind::typography;
    }
    if (value == "border") {
        return RuntimeUiThemeTokenKind::border;
    }
    if (value == "opacity") {
        return RuntimeUiThemeTokenKind::opacity;
    }
    if (value == "transition") {
        return RuntimeUiThemeTokenKind::transition;
    }
    if (value == "controller_glyph") {
        return RuntimeUiThemeTokenKind::controller_glyph;
    }
    return std::nullopt;
}

[[nodiscard]] bool is_hex_digit(char value) noexcept {
    return std::isxdigit(static_cast<unsigned char>(value)) != 0;
}

[[nodiscard]] bool is_color_value(std::string_view value) noexcept {
    if ((value.size() != 7U && value.size() != 9U) || value.front() != '#') {
        return false;
    }
    return std::ranges::all_of(value.substr(1U), is_hex_digit);
}

[[nodiscard]] bool parse_non_negative_float(std::string_view value, float& out) noexcept {
    if (value.empty() || value.front() == '-') {
        return false;
    }
    const auto* first = value.data();
    const auto* last = value.data() + value.size();
    const auto result = std::from_chars(first, last, out);
    return result.ec == std::errc{} && result.ptr == last && out >= 0.0F;
}

[[nodiscard]] bool is_integer_or_float(std::string_view value) noexcept {
    float parsed = 0.0F;
    return parse_non_negative_float(value, parsed);
}

[[nodiscard]] bool is_opacity_value(std::string_view value) noexcept {
    float parsed = 0.0F;
    return parse_non_negative_float(value, parsed) && parsed >= 0.0F && parsed <= 1.0F;
}

[[nodiscard]] bool is_transition_value(std::string_view value) noexcept {
    constexpr std::string_view suffix = "ms";
    if (value.size() <= suffix.size() || !value.ends_with(suffix)) {
        return false;
    }
    return is_integer_or_float(value.substr(0U, value.size() - suffix.size()));
}

[[nodiscard]] bool is_theme_token_value(RuntimeUiThemeTokenKind kind, std::string_view value) noexcept {
    switch (kind) {
    case RuntimeUiThemeTokenKind::color:
        return is_color_value(value);
    case RuntimeUiThemeTokenKind::spacing:
    case RuntimeUiThemeTokenKind::border:
        return is_integer_or_float(value);
    case RuntimeUiThemeTokenKind::typography:
        return value.starts_with("font.") && value.size() > std::string_view{"font."}.size();
    case RuntimeUiThemeTokenKind::opacity:
        return is_opacity_value(value);
    case RuntimeUiThemeTokenKind::transition:
        return is_transition_value(value);
    case RuntimeUiThemeTokenKind::controller_glyph:
        return value.starts_with("gamepad.") || value.starts_with("keyboard.");
    }
    return false;
}

void parse_element_row(RuntimeUiAuthoringDocumentParseResult& result, const ParsedLine& line, std::size_t line_number) {
    RuntimeUiAuthoredElementRow row{
        .id = value_or_empty(line, "id"),
        .parent_id = value_or_empty(line, "parent"),
        .role = SemanticRole::none,
        .widget_id = value_or_empty(line, "widget"),
        .localization_key = value_or_empty(line, "localization"),
        .accessibility_name = value_or_empty(line, "accessibility"),
        .style_token = value_or_empty(line, "style"),
    };

    if (const auto role = parse_semantic_role(value_or_empty(line, "role")); role.has_value()) {
        row.role = *role;
    } else {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::malformed_row, row.id,
                          "runtime UI element row uses an unknown semantic role", line_number);
    }

    for (const auto& field : line.fields) {
        validate_safe_token(result.diagnostics, field.value, field.value, line_number);
    }

    if (row.id.empty()) {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::malformed_row, "element",
                          "runtime UI element row requires id", line_number);
    } else {
        const auto duplicate =
            std::ranges::any_of(result.document.elements,
                                [&row](const RuntimeUiAuthoredElementRow& existing) { return existing.id == row.id; });
        if (duplicate) {
            append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::duplicate_element_id, row.id,
                              "runtime UI element ids must be unique", line_number);
        }
    }
    if (row.localization_key.empty()) {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::missing_localization_key, row.id,
                          "runtime UI authored elements require localization keys", line_number);
    }
    if (row.accessibility_name.empty()) {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::missing_accessibility_name, row.id,
                          "runtime UI authored elements require accessibility names", line_number);
    }

    result.document.elements.push_back(std::move(row));
}

void parse_widget_row(RuntimeUiAuthoringDocumentParseResult& result, const ParsedLine& line, std::size_t line_number) {
    RuntimeUiAuthoredWidgetRow row{
        .id = value_or_empty(line, "id"),
        .kind = RuntimeUiWidgetKind::button,
        .label = value_or_empty(line, "label"),
    };

    if (const auto kind = parse_widget_kind(value_or_empty(line, "kind")); kind.has_value()) {
        row.kind = *kind;
    } else {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::malformed_row, row.id,
                          "runtime UI widget row uses an unknown widget kind", line_number);
    }

    for (const auto& field : line.fields) {
        validate_safe_token(result.diagnostics, field.value, field.value, line_number);
    }

    if (row.id.empty()) {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::missing_widget_id, "widget",
                          "runtime UI widget row requires id", line_number);
    } else {
        const auto duplicate =
            std::ranges::any_of(result.document.widgets,
                                [&row](const RuntimeUiAuthoredWidgetRow& existing) { return existing.id == row.id; });
        if (duplicate) {
            append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::duplicate_widget_id, row.id,
                              "runtime UI widget ids must be unique", line_number);
        }
    }

    result.document.widgets.push_back(std::move(row));
}

void parse_binding_row(RuntimeUiAuthoringDocumentParseResult& result, const ParsedLine& line, std::size_t line_number) {
    RuntimeUiAuthoredBindingRow row{
        .id = value_or_empty(line, "id"),
        .element_id = value_or_empty(line, "element"),
        .source_key = value_or_empty(line, "source"),
        .target = RuntimeUiBindingTarget::text,
    };

    if (const auto target = parse_binding_target(value_or_empty(line, "target")); target.has_value()) {
        row.target = *target;
    } else {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::malformed_row, row.id,
                          "runtime UI binding row uses an unknown target", line_number);
    }

    for (const auto& field : line.fields) {
        validate_safe_token(result.diagnostics, field.value, field.value, line_number);
    }

    if (row.id.empty()) {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::malformed_row, "binding",
                          "runtime UI binding row requires id", line_number);
    } else {
        const auto duplicate =
            std::ranges::any_of(result.document.bindings,
                                [&row](const RuntimeUiAuthoredBindingRow& existing) { return existing.id == row.id; });
        if (duplicate) {
            append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::duplicate_binding_id, row.id,
                              "runtime UI binding ids must be unique", line_number);
        }
    }

    result.document.bindings.push_back(std::move(row));
}

void validate_document_references(RuntimeUiAuthoringDocumentParseResult& result) {
    for (const auto& element : result.document.elements) {
        if (!element.parent_id.empty() && !has_element(result.document, element.parent_id)) {
            append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::unknown_parent_id, element.id,
                              "runtime UI element parent id must reference another element", 0U);
        }
        if (element.widget_id.empty() || !has_widget(result.document, element.widget_id)) {
            append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::unknown_widget_id, element.id,
                              "runtime UI element widget id must reference a declared widget row", 0U);
        }
    }

    for (const auto& binding : result.document.bindings) {
        if (binding.element_id.empty() || !has_element(result.document, binding.element_id)) {
            append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::unknown_element_id, binding.id,
                              "runtime UI binding row must reference a declared element", 0U);
        }
    }
}

void parse_theme_token_row(RuntimeUiThemeParseResult& result, const ParsedLine& line, std::size_t line_number) {
    const auto kind = parse_theme_token_kind(line.row_type);
    if (!kind.has_value()) {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::invalid_theme_token,
                          std::string{line.row_type}, "runtime UI theme token kind is unknown", line_number);
        return;
    }

    RuntimeUiThemeTokenRow row{
        .kind = *kind,
        .id = value_or_empty(line, "id"),
        .value = value_or_empty(line, "value"),
    };

    for (const auto& field : line.fields) {
        validate_safe_token(result.diagnostics, field.value, field.value, line_number);
    }

    if (row.id.empty() || row.value.empty() || !is_theme_token_value(row.kind, row.value)) {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::invalid_theme_token, row.id,
                          "runtime UI theme token value is invalid for its token kind", line_number);
    }

    result.theme.tokens.push_back(std::move(row));
}

} // namespace

std::string_view runtime_ui_theme_token_kind_name(RuntimeUiThemeTokenKind kind) noexcept {
    switch (kind) {
    case RuntimeUiThemeTokenKind::color:
        return "color";
    case RuntimeUiThemeTokenKind::spacing:
        return "spacing";
    case RuntimeUiThemeTokenKind::typography:
        return "typography";
    case RuntimeUiThemeTokenKind::border:
        return "border";
    case RuntimeUiThemeTokenKind::opacity:
        return "opacity";
    case RuntimeUiThemeTokenKind::transition:
        return "transition";
    case RuntimeUiThemeTokenKind::controller_glyph:
        return "controller_glyph";
    }
    return "unknown";
}

RuntimeUiAuthoringDocumentParseResult parse_runtime_ui_document(std::string_view text) {
    RuntimeUiAuthoringDocumentParseResult result;
    const auto lines = split_lines(text);
    if (lines.empty() || lines.front() != kUiDocumentFormatId) {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::invalid_format_header,
                          "GameEngine.UiDocument.v1", "runtime UI document must start with GameEngine.UiDocument.v1",
                          1U);
        return result;
    }

    result.document.format_id = std::string{kUiDocumentFormatId};
    for (std::size_t index = 1U; index < lines.size(); ++index) {
        const auto line = lines[index];
        const auto line_number = index + 1U;
        if (line.empty()) {
            continue;
        }

        const auto parsed = parse_line(line);
        if (parsed.malformed) {
            append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::malformed_row,
                              std::string{parsed.row_type}, "runtime UI document row contains malformed fields",
                              line_number);
        }

        if (parsed.row_type == "element") {
            parse_element_row(result, parsed, line_number);
        } else if (parsed.row_type == "widget") {
            parse_widget_row(result, parsed, line_number);
        } else if (parsed.row_type == "binding") {
            parse_binding_row(result, parsed, line_number);
        } else {
            append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::malformed_row,
                              std::string{parsed.row_type}, "runtime UI document row kind is unknown", line_number);
        }
    }

    validate_document_references(result);
    result.ready = result.diagnostics.empty();
    return result;
}

std::string write_runtime_ui_document(const RuntimeUiAuthoringDocument& document) {
    std::string text;
    text.append(std::string{kUiDocumentFormatId});
    text.push_back('\n');

    for (const auto& row : document.elements) {
        text.append("element id=");
        text.append(row.id);
        text.append(" parent=");
        text.append(row.parent_id);
        text.append(" role=");
        text.append(std::string{semantic_role_id(row.role)});
        text.append(" widget=");
        text.append(row.widget_id);
        text.append(" localization=");
        text.append(row.localization_key);
        text.append(" accessibility=");
        text.append(row.accessibility_name);
        text.append(" style=");
        text.append(row.style_token);
        text.push_back('\n');
    }

    for (const auto& row : document.widgets) {
        text.append("widget id=");
        text.append(row.id);
        text.append(" kind=");
        text.append(std::string{runtime_ui_widget_kind_name(row.kind)});
        text.append(" label=");
        text.append(row.label);
        text.push_back('\n');
    }

    for (const auto& row : document.bindings) {
        text.append("binding id=");
        text.append(row.id);
        text.append(" element=");
        text.append(row.element_id);
        text.append(" source=");
        text.append(row.source_key);
        text.append(" target=");
        text.append(std::string{runtime_ui_binding_target_name(row.target)});
        text.push_back('\n');
    }

    return text;
}

RuntimeUiThemeParseResult parse_runtime_ui_theme(std::string_view text) {
    RuntimeUiThemeParseResult result;
    const auto lines = split_lines(text);
    if (lines.empty() || lines.front() != kUiThemeFormatId) {
        append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::invalid_format_header,
                          "GameEngine.UiTheme.v1", "runtime UI theme must start with GameEngine.UiTheme.v1", 1U);
        return result;
    }

    result.theme.format_id = std::string{kUiThemeFormatId};
    for (std::size_t index = 1U; index < lines.size(); ++index) {
        const auto line = lines[index];
        const auto line_number = index + 1U;
        if (line.empty()) {
            continue;
        }

        const auto parsed = parse_line(line);
        if (parsed.malformed) {
            append_diagnostic(result.diagnostics, RuntimeUiAuthoringDiagnosticCode::malformed_row,
                              std::string{parsed.row_type}, "runtime UI theme row contains malformed fields",
                              line_number);
        }
        parse_theme_token_row(result, parsed, line_number);
    }

    result.ready = result.diagnostics.empty();
    return result;
}

std::string write_runtime_ui_theme(const RuntimeUiThemeDocument& theme) {
    std::string text;
    text.append(std::string{kUiThemeFormatId});
    text.push_back('\n');

    for (const auto& row : theme.tokens) {
        text.append(std::string{runtime_ui_theme_token_kind_name(row.kind)});
        text.append(" id=");
        text.append(row.id);
        text.append(" value=");
        text.append(row.value);
        text.push_back('\n');
    }

    return text;
}

} // namespace mirakana::ui
