// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/ui_model.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::editor {

struct EditorAiCommandPanelModel;

enum class EditorRichTextInlineObjectKind : std::uint8_t {
    command_link,
    resource_icon,
};

struct EditorRichTextInlineObject {
    std::string id;
    EditorRichTextInlineObjectKind kind{EditorRichTextInlineObjectKind::command_link};
    std::string command_id;
    std::string resource_id;
    std::string accessibility_label;
};

struct EditorRichTextSpan {
    std::string id;
    std::string style_token;
    std::string text;
    std::vector<EditorRichTextInlineObject> inline_objects;
};

struct EditorRichTextParagraph {
    std::string id;
    std::vector<EditorRichTextSpan> spans;
};

struct EditorRichTextSelection {
    bool active{false};
    std::string start_paragraph_id;
    std::string start_span_id;
    std::size_t start_byte_offset{0U};
    std::string end_paragraph_id;
    std::string end_span_id;
    std::size_t end_byte_offset{0U};
};

struct EditorRichTextUnsupportedCapability {
    std::string id;
    std::string official_boundary;
    bool implemented{false};
    bool native_handles_public{false};
};

struct EditorRichTextDocument {
    std::string id;
    std::vector<EditorRichTextParagraph> paragraphs;
    EditorRichTextSelection selection;
    std::vector<EditorRichTextUnsupportedCapability> unsupported_capabilities;
};

struct EditorRichTextDiagnostic {
    std::string code;
    std::string message;
};

struct EditorRichTextValidation {
    bool valid{false};
    std::vector<EditorRichTextDiagnostic> diagnostics;
};

struct EditorRichTextCopyResult {
    bool valid{false};
    std::string text;
    std::vector<EditorRichTextDiagnostic> diagnostics;
};

struct EditorRichTextViewport {
    bool enabled{false};
    std::size_t first_paragraph{0U};
    std::size_t max_paragraphs{0U};
};

struct EditorRichTextUiModel {
    ui::UiDocument document;
    std::size_t total_paragraph_count{0U};
    std::size_t visible_paragraph_count{0U};
    std::size_t skipped_before_count{0U};
    std::size_t skipped_after_count{0U};
    bool virtualized{false};
    bool plain_utf8_clipboard_only{true};
    std::string copyable_plain_text;
    std::string selected_plain_text;
    std::vector<EditorRichTextDiagnostic> diagnostics;
};

struct EditorRichTextAiRow {
    std::string id;
    std::string role;
    std::string paragraph_id;
    std::string span_id;
    std::string style_token;
    std::string text;
    std::string command_id;
    std::string resource_id;
    std::string accessibility_label;
    bool visible{true};
    bool selected{false};
    bool copyable{false};
};

struct EditorRichTextAiSnapshot {
    std::string document_id;
    std::uint64_t revision{0};
    std::vector<EditorRichTextAiRow> rows;
    std::string copyable_plain_text;
    std::string selected_plain_text;
    bool plain_utf8_clipboard_only{true};
    std::size_t total_paragraph_count{0U};
    std::size_t visible_paragraph_count{0U};
    std::size_t skipped_before_count{0U};
    std::size_t skipped_after_count{0U};
    std::vector<EditorRichTextDiagnostic> diagnostics;
};

[[nodiscard]] std::vector<EditorRichTextUnsupportedCapability>
make_editor_rich_text_low_level_unsupported_capabilities();

[[nodiscard]] EditorRichTextDocument
make_editor_console_rich_text_document(std::span<const EditorDiagnosticRow> rows,
                                       std::string document_id = "editor.rich_text.console");
[[nodiscard]] EditorRichTextDocument
make_editor_ai_command_panel_rich_text_document(const EditorAiCommandPanelModel& model,
                                                std::string document_id = "editor.rich_text.ai_commands");
[[nodiscard]] EditorRichTextDocument
make_editor_inspector_rich_text_document(std::span<const EditorPropertyRow> rows,
                                         std::string document_id = "editor.rich_text.inspector");
[[nodiscard]] EditorRichTextValidation validate_editor_rich_text_document(const EditorRichTextDocument& document);
[[nodiscard]] EditorRichTextCopyResult copy_editor_rich_text_plain_text(const EditorRichTextDocument& document);
[[nodiscard]] EditorRichTextCopyResult
copy_editor_rich_text_selection_plain_text(const EditorRichTextDocument& document);
[[nodiscard]] EditorRichTextUiModel make_editor_rich_text_view_model(const EditorRichTextDocument& document,
                                                                     EditorRichTextViewport viewport = {});
[[nodiscard]] EditorRichTextAiSnapshot make_editor_rich_text_ai_snapshot(const EditorRichTextDocument& document,
                                                                         EditorRichTextViewport viewport = {});
[[nodiscard]] ui::UiDocument make_editor_rich_text_ui_model(const EditorRichTextDocument& document);

} // namespace mirakana::editor
