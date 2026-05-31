// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <string>
#include <vector>

namespace mirakana::editor {

struct FirstPartyEditorRichTextSpan {
    std::string id;
    std::string style_token;
    std::string text;
};

struct FirstPartyEditorRichTextParagraph {
    std::string id;
    std::vector<FirstPartyEditorRichTextSpan> spans;
};

struct FirstPartyEditorRichTextDocument {
    std::string id;
    std::vector<FirstPartyEditorRichTextParagraph> paragraphs;
};

struct FirstPartyEditorRichTextValidation {
    bool valid{false};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] FirstPartyEditorRichTextValidation
validate_first_party_editor_rich_text_document(const FirstPartyEditorRichTextDocument& document);

[[nodiscard]] ui::UiDocument
make_first_party_editor_rich_text_ui_model(const FirstPartyEditorRichTextDocument& document);

} // namespace mirakana::editor
