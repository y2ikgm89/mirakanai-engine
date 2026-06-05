// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/editor_rich_text.hpp"

#include "mirakana/editor/ai_command_panel.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

struct UnsafeNeedle {
    std::string_view token;
};

struct SelectionEndpoint {
    std::size_t paragraph_index{0U};
    std::size_t span_index{0U};
    const EditorRichTextSpan* span{nullptr};
};

struct ParagraphWindow {
    std::size_t first{0U};
    std::size_t count{0U};
    std::size_t skipped_before{0U};
    std::size_t skipped_after{0U};
    bool virtualized{false};
};

constexpr std::array<UnsafeNeedle, 10> unsafe_needles{{
    UnsafeNeedle{.token = "Dear"},
    UnsafeNeedle{.token = "ImGui"},
    UnsafeNeedle{.token = "Qt"},
    UnsafeNeedle{.token = "Slint"},
    UnsafeNeedle{.token = "RmlUi"},
    UnsafeNeedle{.token = "SDL"},
    UnsafeNeedle{.token = "HWND"},
    UnsafeNeedle{.token = "ID3D12"},
    UnsafeNeedle{.token = "DXGI"},
    UnsafeNeedle{.token = "native_handle"},
}};

constexpr std::array<std::string_view, 9> unsupported_markup_needles{{
    "<b>",
    "</b>",
    "<i>",
    "</i>",
    "<script",
    "[url",
    "[color=",
    "[/",
    "style=",
}};

[[nodiscard]] bool is_utf8_continuation(unsigned char value) noexcept {
    return (value & 0xC0U) == 0x80U;
}

[[nodiscard]] std::optional<std::size_t> strict_utf8_scalar_code_unit_count(std::string_view text,
                                                                            std::size_t offset) noexcept {
    if (offset >= text.size()) {
        return std::nullopt;
    }

    const auto first = static_cast<unsigned char>(text[offset]);
    if (first <= 0x7FU) {
        return 1U;
    }
    if (first >= 0xC2U && first <= 0xDFU) {
        if (offset + 1U >= text.size()) {
            return std::nullopt;
        }
        return is_utf8_continuation(static_cast<unsigned char>(text[offset + 1U])) ? std::optional<std::size_t>{2U}
                                                                                   : std::nullopt;
    }
    if (first >= 0xE0U && first <= 0xEFU) {
        if (offset + 2U >= text.size()) {
            return std::nullopt;
        }
        const auto second = static_cast<unsigned char>(text[offset + 1U]);
        const auto third = static_cast<unsigned char>(text[offset + 2U]);
        if (!is_utf8_continuation(third)) {
            return std::nullopt;
        }
        if (first == 0xE0U) {
            return second >= 0xA0U && second <= 0xBFU ? std::optional<std::size_t>{3U} : std::nullopt;
        }
        if (first == 0xEDU) {
            return second >= 0x80U && second <= 0x9FU ? std::optional<std::size_t>{3U} : std::nullopt;
        }
        return is_utf8_continuation(second) ? std::optional<std::size_t>{3U} : std::nullopt;
    }
    if (first >= 0xF0U && first <= 0xF4U) {
        if (offset + 3U >= text.size()) {
            return std::nullopt;
        }
        const auto second = static_cast<unsigned char>(text[offset + 1U]);
        const auto third = static_cast<unsigned char>(text[offset + 2U]);
        const auto fourth = static_cast<unsigned char>(text[offset + 3U]);
        if (!is_utf8_continuation(third) || !is_utf8_continuation(fourth)) {
            return std::nullopt;
        }
        if (first == 0xF0U) {
            return second >= 0x90U && second <= 0xBFU ? std::optional<std::size_t>{4U} : std::nullopt;
        }
        if (first == 0xF4U) {
            return second >= 0x80U && second <= 0x8FU ? std::optional<std::size_t>{4U} : std::nullopt;
        }
        return is_utf8_continuation(second) ? std::optional<std::size_t>{4U} : std::nullopt;
    }

    return std::nullopt;
}

[[nodiscard]] bool is_strict_utf8(std::string_view text) noexcept {
    for (std::size_t offset = 0U; offset < text.size();) {
        const auto count = strict_utf8_scalar_code_unit_count(text, offset);
        if (!count.has_value()) {
            return false;
        }
        offset += *count;
    }
    return true;
}

[[nodiscard]] bool is_utf8_scalar_boundary(std::string_view text, std::size_t offset) noexcept {
    if (offset > text.size()) {
        return false;
    }
    if (offset == 0U || offset == text.size()) {
        return true;
    }
    return !is_utf8_continuation(static_cast<unsigned char>(text[offset]));
}

[[nodiscard]] std::size_t previous_utf8_scalar_boundary(std::string_view text, std::size_t offset) noexcept {
    auto cursor = std::min(offset, text.size());
    if (cursor == 0U) {
        return 0U;
    }
    --cursor;
    while (cursor > 0U && is_utf8_continuation(static_cast<unsigned char>(text[cursor]))) {
        --cursor;
    }
    return cursor;
}

[[nodiscard]] std::size_t next_utf8_scalar_boundary(std::string_view text, std::size_t offset) noexcept {
    if (offset >= text.size()) {
        return text.size();
    }
    const auto count = strict_utf8_scalar_code_unit_count(text, offset);
    return count.has_value() ? std::min(text.size(), offset + *count) : offset;
}

[[nodiscard]] bool contains_string(const std::vector<std::string>& values, std::string_view value) noexcept {
    return std::ranges::any_of(values, [value](const std::string& candidate) { return candidate == value; });
}

[[nodiscard]] std::string join_values(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "-";
    }

    std::string text;
    for (const auto& value : values) {
        if (!text.empty()) {
            text += ", ";
        }
        text += value;
    }
    return text;
}

[[nodiscard]] bool contains_unsafe_token(std::string_view value) noexcept {
    return std::ranges::any_of(unsafe_needles, [value](const UnsafeNeedle& needle) {
        return value.find(needle.token) != std::string_view::npos;
    });
}

[[nodiscard]] bool contains_unsupported_markup(std::string_view value) noexcept {
    return std::ranges::any_of(unsupported_markup_needles, [value](std::string_view needle) {
        return value.find(needle) != std::string_view::npos;
    });
}

[[nodiscard]] bool contains_shell_execution_token(std::string_view value) noexcept {
    return value.find("shell.execute") != std::string_view::npos ||
           value.find("editor.shell.") != std::string_view::npos ||
           value.find("editor.process.") != std::string_view::npos;
}

void append_diagnostic(EditorRichTextValidation& validation, std::string code, std::string message) {
    validation.diagnostics.push_back(EditorRichTextDiagnostic{.code = std::move(code), .message = std::move(message)});
}

void append_diagnostic(std::vector<EditorRichTextDiagnostic>& diagnostics, std::string code, std::string message) {
    diagnostics.push_back(EditorRichTextDiagnostic{.code = std::move(code), .message = std::move(message)});
}

void validate_id_token(EditorRichTextValidation& validation, std::string_view value, std::string_view label) {
    if (value.empty()) {
        append_diagnostic(validation, "missing_id", std::string{label} + " id must not be empty");
        return;
    }
    if (value.find_first_of("\r\n=") != std::string_view::npos || contains_unsafe_token(value)) {
        append_diagnostic(validation, "unsafe_token",
                          std::string{label} + " id contains an unsafe token: " + std::string{value});
    }
}

void validate_optional_text_token(EditorRichTextValidation& validation, std::string_view value,
                                  std::string_view label) {
    if (value.find_first_of("\r\n=") != std::string_view::npos || contains_unsafe_token(value)) {
        append_diagnostic(validation, "unsafe_token",
                          std::string{label} + " contains an unsafe token: " + std::string{value});
    }
}

[[nodiscard]] std::optional<SelectionEndpoint> find_selection_endpoint(const EditorRichTextDocument& document,
                                                                       std::string_view paragraph_id,
                                                                       std::string_view span_id) noexcept {
    for (std::size_t paragraph_index = 0U; paragraph_index < document.paragraphs.size(); ++paragraph_index) {
        const auto& paragraph = document.paragraphs[paragraph_index];
        if (paragraph.id != paragraph_id) {
            continue;
        }
        for (std::size_t span_index = 0U; span_index < paragraph.spans.size(); ++span_index) {
            const auto& span = paragraph.spans[span_index];
            if (span.id == span_id) {
                return SelectionEndpoint{.paragraph_index = paragraph_index, .span_index = span_index, .span = &span};
            }
        }
    }
    return std::nullopt;
}

[[nodiscard]] bool is_after(const SelectionEndpoint& lhs, std::size_t lhs_offset, const SelectionEndpoint& rhs,
                            std::size_t rhs_offset) noexcept {
    if (lhs.paragraph_index != rhs.paragraph_index) {
        return lhs.paragraph_index > rhs.paragraph_index;
    }
    if (lhs.span_index != rhs.span_index) {
        return lhs.span_index > rhs.span_index;
    }
    return lhs_offset > rhs_offset;
}

[[nodiscard]] bool is_before(const SelectionEndpoint& lhs, std::size_t lhs_offset, const SelectionEndpoint& rhs,
                             std::size_t rhs_offset) noexcept {
    if (lhs.paragraph_index != rhs.paragraph_index) {
        return lhs.paragraph_index < rhs.paragraph_index;
    }
    if (lhs.span_index != rhs.span_index) {
        return lhs.span_index < rhs.span_index;
    }
    return lhs_offset < rhs_offset;
}

[[nodiscard]] bool span_intersects_selection(const EditorRichTextDocument& document, std::size_t paragraph_index,
                                             std::size_t span_index) noexcept {
    if (!document.selection.active) {
        return false;
    }

    const auto start =
        find_selection_endpoint(document, document.selection.start_paragraph_id, document.selection.start_span_id);
    const auto end =
        find_selection_endpoint(document, document.selection.end_paragraph_id, document.selection.end_span_id);
    if (!start.has_value() || !end.has_value()) {
        return false;
    }

    const SelectionEndpoint span_start{.paragraph_index = paragraph_index, .span_index = span_index, .span = nullptr};
    const SelectionEndpoint span_end{.paragraph_index = paragraph_index, .span_index = span_index, .span = nullptr};
    const auto& span = document.paragraphs[paragraph_index].spans[span_index];
    return is_before(span_start, 0U, *end, document.selection.end_byte_offset) &&
           is_after(span_end, span.text.size(), *start, document.selection.start_byte_offset);
}

void validate_selection(EditorRichTextValidation& validation, const EditorRichTextDocument& document) {
    if (!document.selection.active) {
        return;
    }

    validate_id_token(validation, document.selection.start_paragraph_id, "rich text selection start paragraph");
    validate_id_token(validation, document.selection.start_span_id, "rich text selection start span");
    validate_id_token(validation, document.selection.end_paragraph_id, "rich text selection end paragraph");
    validate_id_token(validation, document.selection.end_span_id, "rich text selection end span");

    const auto start =
        find_selection_endpoint(document, document.selection.start_paragraph_id, document.selection.start_span_id);
    const auto end =
        find_selection_endpoint(document, document.selection.end_paragraph_id, document.selection.end_span_id);

    if (!start.has_value()) {
        append_diagnostic(validation, "unknown_selection_start",
                          "rich text selection start references a missing span: " +
                              document.selection.start_paragraph_id + "." + document.selection.start_span_id);
    }
    if (!end.has_value()) {
        append_diagnostic(validation, "unknown_selection_end",
                          "rich text selection end references a missing span: " + document.selection.end_paragraph_id +
                              "." + document.selection.end_span_id);
    }
    if (!start.has_value() || !end.has_value()) {
        return;
    }

    if (document.selection.start_byte_offset > start->span->text.size()) {
        append_diagnostic(validation, "selection_offset_out_of_bounds",
                          "rich text selection start offset exceeds span text length");
    } else if (!is_utf8_scalar_boundary(start->span->text, document.selection.start_byte_offset)) {
        append_diagnostic(validation, "selection_offset_not_scalar_boundary",
                          "rich text selection start offset must be on a UTF-8 scalar boundary");
    }
    if (document.selection.end_byte_offset > end->span->text.size()) {
        append_diagnostic(validation, "selection_offset_out_of_bounds",
                          "rich text selection end offset exceeds span text length");
    } else if (!is_utf8_scalar_boundary(end->span->text, document.selection.end_byte_offset)) {
        append_diagnostic(validation, "selection_offset_not_scalar_boundary",
                          "rich text selection end offset must be on a UTF-8 scalar boundary");
    }
    if (is_after(*start, document.selection.start_byte_offset, *end, document.selection.end_byte_offset)) {
        append_diagnostic(validation, "invalid_selection_order",
                          "rich text selection end must not appear before the start");
    }
}

void validate_inline_object(EditorRichTextValidation& validation, const EditorRichTextInlineObject& object,
                            std::string_view qualified_span_id, std::vector<std::string>& inline_object_ids) {
    const std::string qualified_object_id = std::string{qualified_span_id} + "." + object.id;
    validate_id_token(validation, object.id, "rich text inline object");
    if (!object.id.empty() && contains_string(inline_object_ids, qualified_object_id)) {
        append_diagnostic(validation, "duplicate_inline_object_id",
                          "duplicate rich text inline object id: " + qualified_object_id);
    } else if (!object.id.empty()) {
        inline_object_ids.push_back(qualified_object_id);
    }

    validate_optional_text_token(validation, object.accessibility_label, "rich text inline object accessibility label");
    if (object.kind == EditorRichTextInlineObjectKind::command_link) {
        validate_id_token(validation, object.command_id, "rich text inline command");
    } else {
        validate_id_token(validation, object.resource_id, "rich text inline resource");
    }
}

[[nodiscard]] ui::ElementId element_id(std::string value) {
    return ui::ElementId{.value = std::move(value)};
}

[[nodiscard]] ui::ElementDesc element(std::string id, ui::SemanticRole role) {
    ui::ElementDesc desc;
    desc.id = element_id(std::move(id));
    desc.role = role;
    desc.accessibility_label = desc.id.value;
    desc.style.layout = ui::LayoutMode::column;
    desc.style.anchor = ui::AnchorMode::fill;
    desc.style.gap = 2.0F;
    desc.style.foreground_token = "editor.text";
    return desc;
}

[[nodiscard]] ui::ElementDesc child(std::string id, const ui::ElementId& parent, ui::SemanticRole role) {
    ui::ElementDesc desc = element(std::move(id), role);
    desc.parent = parent;
    return desc;
}

[[nodiscard]] std::string severity_style_token(EditorDiagnosticSeverity severity) {
    switch (severity) {
    case EditorDiagnosticSeverity::info:
        return "editor.info";
    case EditorDiagnosticSeverity::warning:
        return "editor.warning";
    case EditorDiagnosticSeverity::error:
        return "editor.error";
    }
    return "editor.text";
}

[[nodiscard]] std::string ai_status_style_token(std::string_view status_label) {
    if (status_label == "ready") {
        return "editor.info";
    }
    if (status_label == "host_gated" || status_label == "external_action_required") {
        return "editor.warning";
    }
    return "editor.error";
}

[[nodiscard]] std::string safe_rich_text(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const char character : value) {
        if (character == '\n' || character == '\r' || character == '=') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text.empty() ? "-" : text;
}

[[nodiscard]] std::string rich_text_id_component(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const char character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::isalnum(byte) != 0 || character == '_' || character == '-' || character == '.') {
            text.push_back(character);
        } else {
            text.push_back('_');
        }
    }
    return text.empty() ? "row" : text;
}

[[nodiscard]] ParagraphWindow resolve_viewport(std::size_t paragraph_count, EditorRichTextViewport viewport) noexcept {
    ParagraphWindow window;
    window.count = paragraph_count;
    if (!viewport.enabled || viewport.max_paragraphs == 0U || paragraph_count <= viewport.max_paragraphs) {
        return window;
    }

    window.first = std::min(viewport.first_paragraph, paragraph_count);
    window.count = std::min(viewport.max_paragraphs, paragraph_count - window.first);
    window.skipped_before = window.first;
    window.skipped_after = paragraph_count - window.first - window.count;
    window.virtualized = true;
    return window;
}

[[nodiscard]] EditorRichTextDocumentState document_state(const EditorRichTextDocument& document) {
    return EditorRichTextDocumentState{.paragraphs = document.paragraphs, .selection = document.selection};
}

void restore_document_state(EditorRichTextDocument& document, const EditorRichTextDocumentState& state) {
    document.paragraphs = state.paragraphs;
    document.selection = state.selection;
}

void advance_document_revision(EditorRichTextDocument& document) noexcept {
    if (document.revision == std::numeric_limits<std::uint64_t>::max()) {
        document.revision = 1U;
    } else {
        ++document.revision;
    }
}

[[nodiscard]] std::uint64_t rich_text_revision_value(const EditorRichTextDocument& document) noexcept {
    std::uint64_t revision = 1469598103934665603ULL;
    const auto hash_byte = [&revision](std::uint8_t value) noexcept {
        revision ^= value;
        revision *= 1099511628211ULL;
    };
    const auto hash_string = [&hash_byte](std::string_view value) noexcept {
        for (const char ch : value) {
            hash_byte(static_cast<std::uint8_t>(ch));
        }
        hash_byte(0U);
    };
    const auto hash_size = [&hash_byte](std::size_t value) noexcept {
        for (std::size_t shift = 0U; shift < sizeof(std::size_t) * 8U; shift += 8U) {
            hash_byte(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
        }
    };

    hash_string(document.id);
    hash_byte(document.editable ? 1U : 0U);
    hash_size(static_cast<std::size_t>(document.revision));
    hash_size(document.paragraphs.size());
    for (const auto& paragraph : document.paragraphs) {
        hash_string(paragraph.id);
        hash_size(paragraph.spans.size());
        for (const auto& span : paragraph.spans) {
            hash_string(span.id);
            hash_string(span.style_token);
            hash_string(span.text);
            hash_size(span.inline_objects.size());
            for (const auto& object : span.inline_objects) {
                hash_string(object.id);
                hash_string(object.command_id);
                hash_string(object.resource_id);
                hash_string(object.accessibility_label);
                hash_byte(static_cast<std::uint8_t>(object.kind));
            }
        }
    }
    hash_byte(document.selection.active ? 1U : 0U);
    hash_string(document.selection.start_paragraph_id);
    hash_string(document.selection.start_span_id);
    hash_size(document.selection.start_byte_offset);
    hash_string(document.selection.end_paragraph_id);
    hash_string(document.selection.end_span_id);
    hash_size(document.selection.end_byte_offset);
    return revision == 0U ? 1U : revision;
}

void add_or_throw(ui::UiDocument& document, ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("editor rich text element is invalid or duplicated");
    }
}

void append_inline_object_ui(ui::UiDocument& document, const ui::ElementId& parent, std::string id,
                             const EditorRichTextInlineObject& object) {
    const auto role = object.kind == EditorRichTextInlineObjectKind::command_link ? ui::SemanticRole::button
                                                                                  : ui::SemanticRole::image;
    ui::ElementDesc desc = child(std::move(id), parent, role);
    desc.accessibility_label = object.accessibility_label.empty() ? desc.id.value : object.accessibility_label;
    if (object.kind == EditorRichTextInlineObjectKind::command_link) {
        desc.text = ui::TextContent{.label = desc.accessibility_label, .font_family = "editor-ui"};
    } else {
        desc.image = ui::ImageContent{.resource_id = object.resource_id};
    }
    add_or_throw(document, std::move(desc));
}

[[nodiscard]] EditorRichTextSpan rich_text_span(std::string id, std::string style_token, std::string text) {
    return EditorRichTextSpan{
        .id = std::move(id),
        .style_token = std::move(style_token),
        .text = safe_rich_text(text),
        .inline_objects = {},
    };
}

void append_ai_status_paragraph(EditorRichTextDocument& document, std::string id, std::string label, std::string value,
                                std::string style_token) {
    document.paragraphs.push_back(EditorRichTextParagraph{
        .id = std::move(id),
        .spans =
            {
                rich_text_span("label", "editor.text", std::move(label)),
                rich_text_span("value", std::move(style_token), std::move(value)),
            },
    });
}

void append_ai_command_row_paragraph(EditorRichTextDocument& document, const EditorAiCommandPanelCommandRow& row) {
    document.paragraphs.push_back(EditorRichTextParagraph{
        .id = "command." + rich_text_id_component(row.recipe_id),
        .spans =
            {
                rich_text_span("recipe", ai_status_style_token(row.status_label),
                               "Command: " + safe_rich_text(row.recipe_id)),
                rich_text_span("status", ai_status_style_token(row.status_label),
                               " status " + safe_rich_text(row.status_label)),
                rich_text_span("display", "editor.text", " display " + safe_rich_text(row.command_display)),
                rich_text_span("host_gates", "editor.text",
                               " host gates " + safe_rich_text(join_values(row.host_gates))),
                rich_text_span("blocked_by", "editor.warning",
                               " blocked by " + safe_rich_text(join_values(row.blocked_by))),
            },
    });
}

void append_ai_evidence_row_paragraph(EditorRichTextDocument& document, const EditorAiCommandPanelEvidenceRow& row) {
    document.paragraphs.push_back(EditorRichTextParagraph{
        .id = "evidence." + rich_text_id_component(row.recipe_id),
        .spans =
            {
                rich_text_span("recipe", ai_status_style_token(row.status_label),
                               "Evidence: " + safe_rich_text(row.recipe_id)),
                rich_text_span("status", ai_status_style_token(row.status_label),
                               " status " + safe_rich_text(row.status_label)),
                rich_text_span("summary", "editor.text", " summary " + safe_rich_text(row.summary)),
            },
    });
}

void append_ai_diagnostic_paragraph(EditorRichTextDocument& document, std::size_t index, std::string_view diagnostic) {
    document.paragraphs.push_back(EditorRichTextParagraph{
        .id = "diagnostic." + std::to_string(index),
        .spans =
            {
                rich_text_span("label", "editor.error", "Diagnostic: "),
                rich_text_span("message", "editor.error", safe_rich_text(diagnostic)),
            },
    });
}

void append_inspector_property_paragraph(EditorRichTextDocument& document, const EditorPropertyRow& row) {
    document.paragraphs.push_back(EditorRichTextParagraph{
        .id = "property." + rich_text_id_component(row.id),
        .spans =
            {
                rich_text_span("label", "editor.text", safe_rich_text(row.label) + ": "),
                rich_text_span("value", "editor.text", row.value),
                rich_text_span("state", row.editable ? "editor.info" : "editor.text",
                               row.editable ? " editable" : " readonly"),
            },
    });
}

} // namespace

std::vector<EditorRichTextUnsupportedCapability> make_editor_rich_text_low_level_unsupported_capabilities() {
    return {
        EditorRichTextUnsupportedCapability{.id = "text_shaping",
                                            .official_boundary = "DirectWrite/TextServer-class adapter",
                                            .implemented = false,
                                            .native_handles_public = false},
        EditorRichTextUnsupportedCapability{.id = "bidi_line_breaking",
                                            .official_boundary = "TextServer/HarfBuzz/ICU-class adapter",
                                            .implemented = false,
                                            .native_handles_public = false},
        EditorRichTextUnsupportedCapability{.id = "font_fallback",
                                            .official_boundary = "DirectWrite/TextServer-class adapter",
                                            .implemented = false,
                                            .native_handles_public = false},
        EditorRichTextUnsupportedCapability{.id = "font_rasterization",
                                            .official_boundary = "DirectWrite/Direct2D or audited font adapter",
                                            .implemented = false,
                                            .native_handles_public = false},
        EditorRichTextUnsupportedCapability{.id = "ime_composition",
                                            .official_boundary = "Text Services Framework adapter",
                                            .implemented = false,
                                            .native_handles_public = false},
        EditorRichTextUnsupportedCapability{.id = "accessibility_publication",
                                            .official_boundary = "UI Automation provider adapter",
                                            .implemented = false,
                                            .native_handles_public = false},
        EditorRichTextUnsupportedCapability{.id = "rich_clipboard",
                                            .official_boundary = "Platform clipboard adapter",
                                            .implemented = false,
                                            .native_handles_public = false},
    };
}

EditorRichTextDocument make_editor_console_rich_text_document(std::span<const EditorDiagnosticRow> rows,
                                                              std::string document_id) {
    EditorRichTextDocument document;
    document.id = std::move(document_id);
    document.paragraphs.reserve(rows.size());
    for (const auto& row : rows) {
        const std::string severity_label{editor_diagnostic_severity_label(row.severity)};
        const std::string style_token = severity_style_token(row.severity);
        document.paragraphs.push_back(EditorRichTextParagraph{
            .id = row.id,
            .spans =
                {
                    EditorRichTextSpan{
                        .id = "severity",
                        .style_token = style_token,
                        .text = severity_label + ": ",
                        .inline_objects = {},
                    },
                    EditorRichTextSpan{
                        .id = "message",
                        .style_token = style_token,
                        .text = row.message,
                        .inline_objects = {},
                    },
                },
        });
    }
    document.unsupported_capabilities = make_editor_rich_text_low_level_unsupported_capabilities();
    return document;
}

EditorRichTextDocument make_editor_ai_command_panel_rich_text_document(const EditorAiCommandPanelModel& model,
                                                                       std::string document_id) {
    EditorRichTextDocument document;
    document.id = std::move(document_id);

    append_ai_status_paragraph(document, "status", "Status: ", model.status_label,
                               ai_status_style_token(model.status_label));
    append_ai_status_paragraph(document, "operator_handoff",
                               "Operator handoff: ", model.ready_for_operator_handoff ? "ready" : "not ready",
                               model.ready_for_operator_handoff ? "editor.info" : "editor.warning");
    append_ai_status_paragraph(document, "evidence",
                               "Evidence: ", model.all_required_evidence_passed ? "passed" : "not complete",
                               model.all_required_evidence_passed ? "editor.info" : "editor.warning");

    for (const auto& row : model.command_rows) {
        append_ai_command_row_paragraph(document, row);
    }
    for (const auto& row : model.evidence_rows) {
        append_ai_evidence_row_paragraph(document, row);
    }
    for (std::size_t index = 0U; index < model.diagnostics.size(); ++index) {
        append_ai_diagnostic_paragraph(document, index, model.diagnostics[index]);
    }

    document.unsupported_capabilities = make_editor_rich_text_low_level_unsupported_capabilities();
    return document;
}

EditorRichTextDocument make_editor_inspector_rich_text_document(std::span<const EditorPropertyRow> rows,
                                                                std::string document_id) {
    EditorRichTextDocument document;
    document.id = std::move(document_id);
    document.editable = std::ranges::any_of(rows, [](const EditorPropertyRow& row) { return row.editable; });
    for (const auto& row : rows) {
        append_inspector_property_paragraph(document, row);
    }
    document.unsupported_capabilities = make_editor_rich_text_low_level_unsupported_capabilities();
    return document;
}

EditorRichTextValidation validate_editor_rich_text_document(const EditorRichTextDocument& document) {
    EditorRichTextValidation validation{.valid = true};
    validate_id_token(validation, document.id, "rich text document");

    std::vector<std::string> paragraph_ids;
    std::vector<std::string> span_ids;
    std::vector<std::string> inline_object_ids;
    for (const auto& paragraph : document.paragraphs) {
        validate_id_token(validation, paragraph.id, "rich text paragraph");
        if (!paragraph.id.empty() && contains_string(paragraph_ids, paragraph.id)) {
            append_diagnostic(validation, "duplicate_paragraph_id",
                              "duplicate rich text paragraph id: " + paragraph.id);
        } else if (!paragraph.id.empty()) {
            paragraph_ids.push_back(paragraph.id);
        }

        for (const auto& span : paragraph.spans) {
            const std::string qualified_span_id = paragraph.id + "." + span.id;
            validate_id_token(validation, span.id, "rich text span");
            if (!span.id.empty() && contains_string(span_ids, qualified_span_id)) {
                append_diagnostic(validation, "duplicate_span_id", "duplicate rich text span id: " + qualified_span_id);
            } else if (!span.id.empty()) {
                span_ids.push_back(qualified_span_id);
            }
            if (span.style_token.empty()) {
                append_diagnostic(validation, "missing_style_token",
                                  "rich text span style token must not be empty: " + qualified_span_id);
            } else {
                validate_optional_text_token(validation, span.style_token, "rich text span style token");
            }
            if (span.text.empty() && !document.editable) {
                append_diagnostic(validation, "missing_span_text",
                                  "rich text span text must not be empty: " + qualified_span_id);
            }
            if (!is_strict_utf8(span.text)) {
                append_diagnostic(validation, "invalid_utf8",
                                  "rich text span text must be well-formed UTF-8: " + qualified_span_id);
            }
            if (contains_unsupported_markup(span.text)) {
                append_diagnostic(validation, "unsupported_markup",
                                  "rich text span text must use structured spans, not inline markup: " +
                                      qualified_span_id);
            }
            if (contains_unsafe_token(span.text)) {
                append_diagnostic(validation, "unsafe_token",
                                  "rich text span text contains an unsafe token: " + qualified_span_id);
            }
            for (const auto& object : span.inline_objects) {
                validate_inline_object(validation, object, qualified_span_id, inline_object_ids);
            }
        }
    }

    validate_selection(validation, document);

    for (const auto& capability : document.unsupported_capabilities) {
        validate_id_token(validation, capability.id, "rich text unsupported capability");
        if (capability.official_boundary.empty()) {
            append_diagnostic(validation, "missing_official_boundary",
                              "rich text unsupported capability must name its adapter boundary: " + capability.id);
        } else {
            validate_optional_text_token(validation, capability.official_boundary,
                                         "rich text unsupported capability boundary");
        }
        if (capability.implemented || capability.native_handles_public) {
            append_diagnostic(validation, "unsupported_capability_claim",
                              "low-level rich text capability must remain behind adapters: " + capability.id);
        }
    }

    validation.valid = validation.diagnostics.empty();
    return validation;
}

std::uint64_t editor_rich_text_revision(const EditorRichTextDocument& document) noexcept {
    return rich_text_revision_value(document);
}

EditorRichTextSelection normalize_editor_rich_text_selection(const EditorRichTextDocument& document,
                                                             EditorRichTextSelection selection) {
    if (!selection.active) {
        return selection;
    }

    const auto start = find_selection_endpoint(document, selection.start_paragraph_id, selection.start_span_id);
    const auto end = find_selection_endpoint(document, selection.end_paragraph_id, selection.end_span_id);
    if (!start.has_value() || !end.has_value()) {
        return selection;
    }

    if (is_after(*start, selection.start_byte_offset, *end, selection.end_byte_offset)) {
        std::swap(selection.start_paragraph_id, selection.end_paragraph_id);
        std::swap(selection.start_span_id, selection.end_span_id);
        std::swap(selection.start_byte_offset, selection.end_byte_offset);
    }
    return selection;
}

EditorRichTextCopyResult copy_editor_rich_text_plain_text(const EditorRichTextDocument& document) {
    const auto validation = validate_editor_rich_text_document(document);
    EditorRichTextCopyResult result{.valid = validation.valid, .text = {}, .diagnostics = validation.diagnostics};
    if (!validation.valid) {
        return result;
    }

    for (std::size_t paragraph_index = 0U; paragraph_index < document.paragraphs.size(); ++paragraph_index) {
        if (paragraph_index > 0U) {
            result.text.push_back('\n');
        }
        for (const auto& span : document.paragraphs[paragraph_index].spans) {
            result.text += span.text;
        }
    }
    return result;
}

EditorRichTextCopyResult copy_editor_rich_text_selection_plain_text(const EditorRichTextDocument& document) {
    const auto validation = validate_editor_rich_text_document(document);
    EditorRichTextCopyResult result{.valid = validation.valid, .text = {}, .diagnostics = validation.diagnostics};
    if (!validation.valid || !document.selection.active) {
        return result;
    }

    const auto start =
        find_selection_endpoint(document, document.selection.start_paragraph_id, document.selection.start_span_id);
    const auto end =
        find_selection_endpoint(document, document.selection.end_paragraph_id, document.selection.end_span_id);
    if (!start.has_value() || !end.has_value()) {
        result.valid = false;
        return result;
    }

    for (std::size_t paragraph_index = start->paragraph_index; paragraph_index <= end->paragraph_index;
         ++paragraph_index) {
        if (paragraph_index > start->paragraph_index) {
            result.text.push_back('\n');
        }
        const auto& paragraph = document.paragraphs[paragraph_index];
        const std::size_t first_span = paragraph_index == start->paragraph_index ? start->span_index : 0U;
        const std::size_t last_span =
            paragraph_index == end->paragraph_index ? end->span_index : paragraph.spans.size() - 1U;
        for (std::size_t span_index = first_span; span_index <= last_span; ++span_index) {
            const auto& span = paragraph.spans[span_index];
            const std::size_t first_offset =
                paragraph_index == start->paragraph_index && span_index == start->span_index
                    ? document.selection.start_byte_offset
                    : 0U;
            const std::size_t last_offset = paragraph_index == end->paragraph_index && span_index == end->span_index
                                                ? document.selection.end_byte_offset
                                                : span.text.size();
            result.text += span.text.substr(first_offset, last_offset - first_offset);
        }
    }
    return result;
}

namespace {

[[nodiscard]] std::vector<std::string_view> split_edit_lines(std::string_view text) {
    std::vector<std::string_view> lines;
    std::size_t begin = 0U;
    while (begin <= text.size()) {
        const auto separator = text.find('\n', begin);
        const auto end = separator == std::string_view::npos ? text.size() : separator;
        lines.push_back(text.substr(begin, end - begin));
        if (separator == std::string_view::npos) {
            break;
        }
        begin = separator + 1U;
    }
    return lines;
}

[[nodiscard]] std::string generated_paragraph_id(std::string_view base_id, std::size_t ordinal) {
    return std::string{base_id} + ".edit." + std::to_string(ordinal);
}

[[nodiscard]] std::string generated_span_id(std::string_view base_id, std::size_t ordinal) {
    return std::string{base_id} + ".edit." + std::to_string(ordinal);
}

[[nodiscard]] std::string paragraphs_plain_text(std::span<const EditorRichTextParagraph> paragraphs) {
    std::string text;
    for (std::size_t paragraph_index = 0U; paragraph_index < paragraphs.size(); ++paragraph_index) {
        if (paragraph_index > 0U) {
            text.push_back('\n');
        }
        for (const auto& span : paragraphs[paragraph_index].spans) {
            text += span.text;
        }
    }
    return text;
}

[[nodiscard]] bool command_mutates_document(EditorRichTextEditCommandKind kind) noexcept {
    switch (kind) {
    case EditorRichTextEditCommandKind::insert_text:
    case EditorRichTextEditCommandKind::delete_selection:
    case EditorRichTextEditCommandKind::replace_selection:
    case EditorRichTextEditCommandKind::toggle_bold:
    case EditorRichTextEditCommandKind::toggle_italic:
    case EditorRichTextEditCommandKind::move_cursor_backward:
    case EditorRichTextEditCommandKind::move_cursor_forward:
    case EditorRichTextEditCommandKind::undo:
    case EditorRichTextEditCommandKind::redo:
    case EditorRichTextEditCommandKind::cut_selection:
    case EditorRichTextEditCommandKind::paste_plain_text:
    case EditorRichTextEditCommandKind::paste_rich_text:
        return true;
    case EditorRichTextEditCommandKind::copy_plain_text:
    case EditorRichTextEditCommandKind::copy_selection_plain_text:
    case EditorRichTextEditCommandKind::copy_rich_text:
        return false;
    }
    return false;
}

[[nodiscard]] bool command_uses_request_text(EditorRichTextEditCommandKind kind) noexcept {
    return kind == EditorRichTextEditCommandKind::insert_text ||
           kind == EditorRichTextEditCommandKind::replace_selection;
}

[[nodiscard]] bool command_requires_selection(EditorRichTextEditCommandKind kind) noexcept {
    switch (kind) {
    case EditorRichTextEditCommandKind::delete_selection:
    case EditorRichTextEditCommandKind::replace_selection:
    case EditorRichTextEditCommandKind::toggle_bold:
    case EditorRichTextEditCommandKind::toggle_italic:
    case EditorRichTextEditCommandKind::copy_selection_plain_text:
    case EditorRichTextEditCommandKind::copy_rich_text:
    case EditorRichTextEditCommandKind::cut_selection:
        return true;
    default:
        return false;
    }
}

void append_diagnostic(EditorRichTextEditResult& result, std::string code, std::string message) {
    result.diagnostics.push_back(EditorRichTextDiagnostic{.code = std::move(code), .message = std::move(message)});
}

void append_diagnostics(EditorRichTextEditResult& result, const std::vector<EditorRichTextDiagnostic>& diagnostics) {
    result.diagnostics.insert(result.diagnostics.end(), diagnostics.begin(), diagnostics.end());
}

void append_validation_diagnostics(EditorRichTextEditResult& result, const EditorRichTextValidation& validation) {
    append_diagnostics(result, validation.diagnostics);
}

[[nodiscard]] EditorRichTextSelection active_selection_or_end_cursor(const EditorRichTextDocument& document) {
    if (document.selection.active) {
        return normalize_editor_rich_text_selection(document, document.selection);
    }

    EditorRichTextSelection selection{.active = true};
    if (document.paragraphs.empty() || document.paragraphs.back().spans.empty()) {
        return selection;
    }
    const auto& paragraph = document.paragraphs.back();
    const auto& span = paragraph.spans.back();
    selection.start_paragraph_id = paragraph.id;
    selection.start_span_id = span.id;
    selection.start_byte_offset = span.text.size();
    selection.end_paragraph_id = paragraph.id;
    selection.end_span_id = span.id;
    selection.end_byte_offset = span.text.size();
    return selection;
}

[[nodiscard]] bool selection_is_collapsed(const EditorRichTextSelection& selection) noexcept {
    return selection.active && selection.start_paragraph_id == selection.end_paragraph_id &&
           selection.start_span_id == selection.end_span_id && selection.start_byte_offset == selection.end_byte_offset;
}

void validate_edit_text(EditorRichTextEditResult& result, std::string_view text) {
    if (!is_strict_utf8(text)) {
        append_diagnostic(result, "invalid_utf8", "rich text edit input must be well-formed UTF-8");
    }
    if (contains_unsupported_markup(text)) {
        append_diagnostic(result, "unsupported_markup",
                          "rich text edit input must use structured spans, not inline markup");
    }
    if (contains_unsafe_token(text)) {
        append_diagnostic(result, "unsafe_token", "rich text edit input contains an unsafe native/API token");
    }
    if (contains_shell_execution_token(text)) {
        append_diagnostic(result, "shell_execution_unsupported",
                          "rich text edit input must not contain shell execution tokens");
    }
}

[[nodiscard]] std::optional<EditorRichTextSelection>
normalized_request_selection(const EditorRichTextDocument& document, const EditorRichTextEditRequest& request) {
    auto selection =
        request.use_selection_override ? request.selection_override : active_selection_or_end_cursor(document);
    selection = normalize_editor_rich_text_selection(document, std::move(selection));
    if (!selection.active) {
        return std::nullopt;
    }
    return selection;
}

[[nodiscard]] EditorRichTextCopyResult
copy_selection_plain_text_with_selection(const EditorRichTextDocument& document,
                                         const EditorRichTextSelection& selection) {
    auto copy_source = document;
    copy_source.selection = selection;
    return copy_editor_rich_text_selection_plain_text(copy_source);
}

[[nodiscard]] EditorRichTextClipboardPayload copy_selection_rich_payload(const EditorRichTextDocument& document,
                                                                         const EditorRichTextSelection& selection) {
    EditorRichTextClipboardPayload payload;
    const auto plain = copy_selection_plain_text_with_selection(document, selection);
    if (plain.valid) {
        payload.has_plain_text = true;
        payload.plain_text = plain.text;
    }

    const auto start = find_selection_endpoint(document, selection.start_paragraph_id, selection.start_span_id);
    const auto end = find_selection_endpoint(document, selection.end_paragraph_id, selection.end_span_id);
    if (!start.has_value() || !end.has_value()) {
        return payload;
    }

    for (std::size_t paragraph_index = start->paragraph_index; paragraph_index <= end->paragraph_index;
         ++paragraph_index) {
        const auto& source_paragraph = document.paragraphs[paragraph_index];
        EditorRichTextParagraph paragraph{.id = source_paragraph.id};
        const std::size_t first_span = paragraph_index == start->paragraph_index ? start->span_index : 0U;
        const std::size_t last_span =
            paragraph_index == end->paragraph_index ? end->span_index : source_paragraph.spans.size() - 1U;
        for (std::size_t span_index = first_span; span_index <= last_span; ++span_index) {
            auto span = source_paragraph.spans[span_index];
            const auto first_offset = paragraph_index == start->paragraph_index && span_index == start->span_index
                                          ? selection.start_byte_offset
                                          : 0U;
            const auto last_offset = paragraph_index == end->paragraph_index && span_index == end->span_index
                                         ? selection.end_byte_offset
                                         : span.text.size();
            span.text = span.text.substr(first_offset, last_offset - first_offset);
            paragraph.spans.push_back(std::move(span));
        }
        payload.rich_paragraphs.push_back(std::move(paragraph));
    }
    payload.has_rich_text = true;
    return payload;
}

void push_undo_state(EditorRichTextDocument& document) {
    document.undo_stack.push_back(document_state(document));
    document.redo_stack.clear();
}

void replace_selection_with_plain_text(EditorRichTextDocument& document, const EditorRichTextSelection& selection,
                                       std::string_view replacement) {
    const auto start = find_selection_endpoint(document, selection.start_paragraph_id, selection.start_span_id);
    const auto end = find_selection_endpoint(document, selection.end_paragraph_id, selection.end_span_id);
    if (!start.has_value() || !end.has_value()) {
        return;
    }

    const auto lines = split_edit_lines(replacement);
    const auto& source_start_paragraph = document.paragraphs[start->paragraph_index];
    const auto& source_end_paragraph = document.paragraphs[end->paragraph_index];
    auto start_span = source_start_paragraph.spans[start->span_index];
    const auto& end_span = source_end_paragraph.spans[end->span_index];
    const std::string before = start_span.text.substr(0U, selection.start_byte_offset);
    const std::string after = end_span.text.substr(selection.end_byte_offset);

    std::vector<EditorRichTextParagraph> paragraphs;
    paragraphs.reserve(document.paragraphs.size() + lines.size());
    paragraphs.insert(paragraphs.end(), document.paragraphs.begin(),
                      document.paragraphs.begin() + static_cast<std::ptrdiff_t>(start->paragraph_index));

    EditorRichTextParagraph first_paragraph{.id = source_start_paragraph.id};
    first_paragraph.spans.insert(first_paragraph.spans.end(), source_start_paragraph.spans.begin(),
                                 source_start_paragraph.spans.begin() + static_cast<std::ptrdiff_t>(start->span_index));
    start_span.text = before + std::string{lines.empty() ? std::string_view{} : lines.front()};
    if (lines.size() <= 1U) {
        start_span.text += after;
    }
    first_paragraph.spans.push_back(std::move(start_span));

    if (lines.size() <= 1U) {
        if (start->paragraph_index == end->paragraph_index) {
            first_paragraph.spans.insert(first_paragraph.spans.end(),
                                         source_start_paragraph.spans.begin() +
                                             static_cast<std::ptrdiff_t>(end->span_index + 1U),
                                         source_start_paragraph.spans.end());
        } else {
            first_paragraph.spans.insert(first_paragraph.spans.end(),
                                         source_end_paragraph.spans.begin() +
                                             static_cast<std::ptrdiff_t>(end->span_index + 1U),
                                         source_end_paragraph.spans.end());
        }
        paragraphs.push_back(std::move(first_paragraph));
    } else {
        paragraphs.push_back(std::move(first_paragraph));
        for (std::size_t line_index = 1U; line_index < lines.size(); ++line_index) {
            EditorRichTextParagraph paragraph{.id = generated_paragraph_id(source_start_paragraph.id, line_index)};
            auto span = source_start_paragraph.spans[start->span_index];
            span.id = line_index + 1U == lines.size() ? source_start_paragraph.spans[start->span_index].id
                                                      : generated_span_id(span.id, line_index);
            span.text = std::string{lines[line_index]};
            if (line_index + 1U == lines.size()) {
                span.text += after;
            }
            paragraph.spans.push_back(std::move(span));
            if (line_index + 1U == lines.size()) {
                paragraph.spans.insert(paragraph.spans.end(),
                                       source_end_paragraph.spans.begin() +
                                           static_cast<std::ptrdiff_t>(end->span_index + 1U),
                                       source_end_paragraph.spans.end());
            }
            paragraphs.push_back(std::move(paragraph));
        }
    }

    paragraphs.insert(paragraphs.end(),
                      document.paragraphs.begin() + static_cast<std::ptrdiff_t>(end->paragraph_index + 1U),
                      document.paragraphs.end());
    document.paragraphs = std::move(paragraphs);

    const auto cursor_paragraph_index =
        lines.size() <= 1U ? start->paragraph_index : start->paragraph_index + lines.size() - 1U;
    const auto& cursor_paragraph =
        document.paragraphs[std::min(cursor_paragraph_index, document.paragraphs.size() - 1U)];
    const auto& cursor_span = cursor_paragraph.spans.front();
    const auto cursor_offset = lines.size() <= 1U ? before.size() + replacement.size() : lines.back().size();
    document.selection = EditorRichTextSelection{
        .active = true,
        .start_paragraph_id = cursor_paragraph.id,
        .start_span_id = cursor_span.id,
        .start_byte_offset = cursor_offset,
        .end_paragraph_id = cursor_paragraph.id,
        .end_span_id = cursor_span.id,
        .end_byte_offset = cursor_offset,
    };
}

[[nodiscard]] std::string toggle_style_token(std::string_view style_token, bool bold) {
    const bool has_bold = style_token.find("bold") != std::string_view::npos;
    const bool has_italic = style_token.find("italic") != std::string_view::npos;
    const bool next_bold = bold ? !has_bold : has_bold;
    const bool next_italic = bold ? has_italic : !has_italic;
    if (next_bold && next_italic) {
        return "editor.bold_italic";
    }
    if (next_bold) {
        return "editor.bold";
    }
    if (next_italic) {
        return "editor.italic";
    }
    return "editor.text";
}

void toggle_selection_style(EditorRichTextDocument& document, const EditorRichTextSelection& selection, bool bold) {
    document.selection = selection;
    for (std::size_t paragraph_index = 0U; paragraph_index < document.paragraphs.size(); ++paragraph_index) {
        auto& paragraph = document.paragraphs[paragraph_index];
        for (std::size_t span_index = 0U; span_index < paragraph.spans.size(); ++span_index) {
            if (span_intersects_selection(document, paragraph_index, span_index)) {
                paragraph.spans[span_index].style_token =
                    toggle_style_token(paragraph.spans[span_index].style_token, bold);
            }
        }
    }
}

void move_cursor(EditorRichTextDocument& document, const EditorRichTextSelection& selection, bool forward) {
    const auto endpoint = find_selection_endpoint(document, selection.start_paragraph_id, selection.start_span_id);
    if (!endpoint.has_value()) {
        return;
    }
    const auto& span = document.paragraphs[endpoint->paragraph_index].spans[endpoint->span_index];
    const auto offset = forward ? next_utf8_scalar_boundary(span.text, selection.start_byte_offset)
                                : previous_utf8_scalar_boundary(span.text, selection.start_byte_offset);
    document.selection = EditorRichTextSelection{
        .active = true,
        .start_paragraph_id = selection.start_paragraph_id,
        .start_span_id = selection.start_span_id,
        .start_byte_offset = offset,
        .end_paragraph_id = selection.start_paragraph_id,
        .end_span_id = selection.start_span_id,
        .end_byte_offset = offset,
    };
}

} // namespace

EditorRichTextEditResult apply_editor_rich_text_edit_command(const EditorRichTextDocument& document,
                                                             const EditorRichTextEditRequest& request) {
    EditorRichTextEditResult result;
    result.document = document;
    result.before_revision = editor_rich_text_revision(document);
    result.after_revision = result.before_revision;

    if (request.document_id != document.id) {
        append_diagnostic(result, "target_mismatch", "rich text edit command document_id must match the document");
        return result;
    }
    if (request.expected_revision != 0U && request.expected_revision != result.before_revision) {
        append_diagnostic(result, "stale_revision", "rich text edit expected_revision does not match the document");
        return result;
    }
    if (command_mutates_document(request.kind) && !document.editable) {
        append_diagnostic(result, "document_readonly", "rich text document is not editable");
        return result;
    }
    if (command_uses_request_text(request.kind)) {
        validate_edit_text(result, request.text);
        if (!result.diagnostics.empty()) {
            return result;
        }
    }

    auto working = document;
    if (request.use_selection_override) {
        working.selection = normalize_editor_rich_text_selection(working, request.selection_override);
    }
    const auto validation = validate_editor_rich_text_document(working);
    if (!validation.valid) {
        append_validation_diagnostics(result, validation);
        return result;
    }

    const auto selection = normalized_request_selection(working, request);
    if (command_requires_selection(request.kind) && (!selection.has_value() || selection_is_collapsed(*selection))) {
        append_diagnostic(result, "missing_selection", "rich text edit command requires a non-empty selection");
        return result;
    }

    const auto copy_all = [&]() {
        const auto copy = copy_editor_rich_text_plain_text(working);
        if (!copy.valid) {
            append_diagnostics(result, copy.diagnostics);
            return;
        }
        result.clipboard.has_plain_text = true;
        result.clipboard.plain_text = copy.text;
        result.output_text = copy.text;
        result.output_mime_type = "text/plain;charset=utf-8";
        result.accepted = true;
    };
    const auto copy_selection_plain = [&]() {
        const auto copy = copy_selection_plain_text_with_selection(working, *selection);
        if (!copy.valid) {
            append_diagnostics(result, copy.diagnostics);
            return;
        }
        result.clipboard.has_plain_text = true;
        result.clipboard.plain_text = copy.text;
        result.output_text = copy.text;
        result.output_mime_type = "text/plain;charset=utf-8";
        result.accepted = true;
    };

    switch (request.kind) {
    case EditorRichTextEditCommandKind::copy_plain_text:
        copy_all();
        break;
    case EditorRichTextEditCommandKind::copy_selection_plain_text:
        copy_selection_plain();
        break;
    case EditorRichTextEditCommandKind::copy_rich_text:
        result.clipboard = copy_selection_rich_payload(working, *selection);
        result.output_text = result.clipboard.plain_text;
        result.output_mime_type = "application/vnd.mirakanai.rich-text+json";
        result.accepted = result.clipboard.has_rich_text;
        break;
    case EditorRichTextEditCommandKind::undo:
        if (working.undo_stack.empty()) {
            append_diagnostic(result, "history_empty", "rich text undo history is empty");
            break;
        }
        working.redo_stack.push_back(document_state(working));
        restore_document_state(working, working.undo_stack.back());
        working.undo_stack.pop_back();
        advance_document_revision(working);
        result.accepted = true;
        result.applied = true;
        break;
    case EditorRichTextEditCommandKind::redo:
        if (working.redo_stack.empty()) {
            append_diagnostic(result, "history_empty", "rich text redo history is empty");
            break;
        }
        working.undo_stack.push_back(document_state(working));
        restore_document_state(working, working.redo_stack.back());
        working.redo_stack.pop_back();
        advance_document_revision(working);
        result.accepted = true;
        result.applied = true;
        break;
    case EditorRichTextEditCommandKind::insert_text:
    case EditorRichTextEditCommandKind::replace_selection:
        push_undo_state(working);
        replace_selection_with_plain_text(working, *selection, request.text);
        advance_document_revision(working);
        result.accepted = true;
        result.applied = true;
        break;
    case EditorRichTextEditCommandKind::delete_selection:
        push_undo_state(working);
        replace_selection_with_plain_text(working, *selection, {});
        advance_document_revision(working);
        result.accepted = true;
        result.applied = true;
        break;
    case EditorRichTextEditCommandKind::toggle_bold:
    case EditorRichTextEditCommandKind::toggle_italic:
        push_undo_state(working);
        toggle_selection_style(working, *selection, request.kind == EditorRichTextEditCommandKind::toggle_bold);
        advance_document_revision(working);
        result.accepted = true;
        result.applied = true;
        break;
    case EditorRichTextEditCommandKind::move_cursor_backward:
    case EditorRichTextEditCommandKind::move_cursor_forward:
        push_undo_state(working);
        move_cursor(working, *selection, request.kind == EditorRichTextEditCommandKind::move_cursor_forward);
        advance_document_revision(working);
        result.accepted = true;
        result.applied = true;
        break;
    case EditorRichTextEditCommandKind::cut_selection:
        result.clipboard = copy_selection_rich_payload(working, *selection);
        push_undo_state(working);
        replace_selection_with_plain_text(working, *selection, {});
        advance_document_revision(working);
        result.accepted = true;
        result.applied = true;
        break;
    case EditorRichTextEditCommandKind::paste_plain_text:
        if (!request.clipboard.has_plain_text) {
            append_diagnostic(result, "clipboard_plain_text_missing", "rich text paste plain requires plain text");
            break;
        }
        validate_edit_text(result, request.clipboard.plain_text);
        if (!result.diagnostics.empty()) {
            break;
        }
        push_undo_state(working);
        replace_selection_with_plain_text(working, *selection, request.clipboard.plain_text);
        advance_document_revision(working);
        result.accepted = true;
        result.applied = true;
        break;
    case EditorRichTextEditCommandKind::paste_rich_text: {
        if (!request.clipboard.has_rich_text) {
            append_diagnostic(result, "clipboard_rich_text_missing", "rich text paste rich requires rich text");
            break;
        }
        const auto plain_text = paragraphs_plain_text(request.clipboard.rich_paragraphs);
        validate_edit_text(result, plain_text);
        if (!result.diagnostics.empty()) {
            break;
        }
        push_undo_state(working);
        replace_selection_with_plain_text(working, *selection, plain_text);
        advance_document_revision(working);
        result.accepted = true;
        result.applied = true;
        break;
    }
    }

    if (result.accepted && result.applied) {
        result.document = std::move(working);
        result.after_revision = editor_rich_text_revision(result.document);
    }
    return result;
}

EditorRichTextUiModel make_editor_rich_text_view_model(const EditorRichTextDocument& document,
                                                       EditorRichTextViewport viewport) {
    const auto validation = validate_editor_rich_text_document(document);
    EditorRichTextUiModel model;
    model.total_paragraph_count = document.paragraphs.size();
    model.diagnostics = validation.diagnostics;
    model.plain_utf8_clipboard_only = true;
    if (!validation.valid) {
        throw std::invalid_argument("editor rich text document is invalid");
    }

    if (const auto copy = copy_editor_rich_text_plain_text(document); copy.valid) {
        model.copyable_plain_text = copy.text;
    }
    if (const auto copy = copy_editor_rich_text_selection_plain_text(document); copy.valid) {
        model.selected_plain_text = copy.text;
    }

    const ParagraphWindow window = resolve_viewport(document.paragraphs.size(), viewport);
    model.visible_paragraph_count = window.count;
    model.skipped_before_count = window.skipped_before;
    model.skipped_after_count = window.skipped_after;
    model.virtualized = window.virtualized;

    ui::UiDocument ui_document;
    ui::ElementDesc root = element(document.id, ui::SemanticRole::root);
    root.accessibility_label = document.id;
    add_or_throw(ui_document, std::move(root));

    const ui::ElementId root_id = element_id(document.id);
    for (std::size_t paragraph_index = window.first; paragraph_index < window.first + window.count; ++paragraph_index) {
        const auto& paragraph = document.paragraphs[paragraph_index];
        const std::string paragraph_id = document.id + ".paragraph." + paragraph.id;
        ui::ElementDesc paragraph_element = child(paragraph_id, root_id, ui::SemanticRole::panel);
        paragraph_element.accessibility_label = paragraph.id;
        add_or_throw(ui_document, std::move(paragraph_element));

        const ui::ElementId paragraph_element_id = element_id(paragraph_id);
        for (const auto& span : paragraph.spans) {
            const std::string span_id = paragraph_id + ".span." + span.id;
            ui::ElementDesc span_element = child(span_id, paragraph_element_id, ui::SemanticRole::label);
            span_element.text = ui::TextContent{.label = span.text, .font_family = "editor-ui"};
            span_element.style.foreground_token = span.style_token;
            span_element.accessibility_label = span.text;
            add_or_throw(ui_document, std::move(span_element));

            const ui::ElementId span_element_id = element_id(span_id);
            for (const auto& object : span.inline_objects) {
                append_inline_object_ui(ui_document, span_element_id, span_id + ".inline." + object.id, object);
            }
        }
    }

    model.document = std::move(ui_document);
    return model;
}

EditorRichTextAiSnapshot make_editor_rich_text_ai_snapshot(const EditorRichTextDocument& document,
                                                           EditorRichTextViewport viewport) {
    const auto validation = validate_editor_rich_text_document(document);
    EditorRichTextAiSnapshot snapshot;
    snapshot.document_id = document.id;
    snapshot.revision = editor_rich_text_revision(document);
    snapshot.total_paragraph_count = document.paragraphs.size();
    snapshot.diagnostics = validation.diagnostics;
    if (!validation.valid) {
        return snapshot;
    }

    if (const auto copy = copy_editor_rich_text_plain_text(document); copy.valid) {
        snapshot.copyable_plain_text = copy.text;
    }
    if (const auto copy = copy_editor_rich_text_selection_plain_text(document); copy.valid) {
        snapshot.selected_plain_text = copy.text;
    }

    const ParagraphWindow window = resolve_viewport(document.paragraphs.size(), viewport);
    snapshot.visible_paragraph_count = window.count;
    snapshot.skipped_before_count = window.skipped_before;
    snapshot.skipped_after_count = window.skipped_after;

    snapshot.rows.push_back(EditorRichTextAiRow{
        .id = document.id,
        .role = "rich_text_document",
        .accessibility_label = document.id,
        .visible = true,
        .selected = false,
        .copyable = true,
        .editable = document.editable,
    });

    for (std::size_t paragraph_index = window.first; paragraph_index < window.first + window.count; ++paragraph_index) {
        const auto& paragraph = document.paragraphs[paragraph_index];
        const std::string paragraph_row_id = document.id + ".paragraph." + paragraph.id;
        snapshot.rows.push_back(EditorRichTextAiRow{
            .id = paragraph_row_id,
            .role = "rich_text_paragraph",
            .paragraph_id = paragraph.id,
            .accessibility_label = paragraph.id,
            .visible = true,
            .selected = false,
            .copyable = true,
            .editable = document.editable,
        });

        for (std::size_t span_index = 0U; span_index < paragraph.spans.size(); ++span_index) {
            const auto& span = paragraph.spans[span_index];
            const std::string span_row_id = paragraph_row_id + ".span." + span.id;
            const bool selected = span_intersects_selection(document, paragraph_index, span_index);
            snapshot.rows.push_back(EditorRichTextAiRow{
                .id = span_row_id,
                .role = "rich_text_span",
                .paragraph_id = paragraph.id,
                .span_id = span.id,
                .style_token = span.style_token,
                .text = span.text,
                .accessibility_label = span.text,
                .visible = true,
                .selected = selected,
                .copyable = true,
                .editable = document.editable,
            });

            for (const auto& object : span.inline_objects) {
                const bool is_command = object.kind == EditorRichTextInlineObjectKind::command_link;
                snapshot.rows.push_back(EditorRichTextAiRow{
                    .id = span_row_id + ".inline." + object.id,
                    .role = is_command ? "rich_text_inline_command" : "rich_text_inline_resource",
                    .paragraph_id = paragraph.id,
                    .span_id = span.id,
                    .command_id = object.command_id,
                    .resource_id = object.resource_id,
                    .accessibility_label = object.accessibility_label.empty() ? object.id : object.accessibility_label,
                    .visible = true,
                    .selected = selected,
                    .copyable = false,
                    .editable = false,
                });
            }
        }
    }

    for (const auto& capability : document.unsupported_capabilities) {
        if (!capability.implemented) {
            append_diagnostic(snapshot.diagnostics, "unsupported_capability",
                              "rich text capability remains adapter-owned: " + capability.id + " via " +
                                  capability.official_boundary);
        }
    }

    return snapshot;
}

ui::UiDocument make_editor_rich_text_ui_model(const EditorRichTextDocument& document) {
    return make_editor_rich_text_view_model(document).document;
}

} // namespace mirakana::editor
