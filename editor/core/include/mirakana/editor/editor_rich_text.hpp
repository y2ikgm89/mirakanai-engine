// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::editor {

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

[[nodiscard]] std::vector<EditorRichTextUnsupportedCapability>
make_editor_rich_text_low_level_unsupported_capabilities();

[[nodiscard]] EditorRichTextValidation validate_editor_rich_text_document(const EditorRichTextDocument& document);
[[nodiscard]] EditorRichTextCopyResult copy_editor_rich_text_plain_text(const EditorRichTextDocument& document);
[[nodiscard]] EditorRichTextCopyResult
copy_editor_rich_text_selection_plain_text(const EditorRichTextDocument& document);
[[nodiscard]] ui::UiDocument make_editor_rich_text_ui_model(const EditorRichTextDocument& document);

} // namespace mirakana::editor
