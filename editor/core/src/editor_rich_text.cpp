// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/editor_rich_text.hpp"

#include "mirakana/editor/ai_command_panel.hpp"

#include <algorithm>
#include <array>
#include <cctype>
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
    }
    if (document.selection.end_byte_offset > end->span->text.size()) {
        append_diagnostic(validation, "selection_offset_out_of_bounds",
                          "rich text selection end offset exceeds span text length");
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

[[nodiscard]] std::uint64_t rich_text_revision(const EditorRichTextDocument& document) noexcept {
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
            if (span.text.empty()) {
                append_diagnostic(validation, "missing_span_text",
                                  "rich text span text must not be empty: " + qualified_span_id);
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
    snapshot.revision = rich_text_revision(document);
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
