// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "first_party_editor_rich_text.hpp"

#include <stdexcept>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] bool contains_string(const std::vector<std::string>& values, const std::string& value) {
    for (const auto& candidate : values) {
        if (candidate == value) {
            return true;
        }
    }
    return false;
}

void append_diagnostic(FirstPartyEditorRichTextValidation& validation, std::string message) {
    validation.diagnostics.push_back(std::move(message));
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

void add_or_throw(ui::UiDocument& document, ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("first-party rich text element is invalid or duplicated");
    }
}

} // namespace

FirstPartyEditorRichTextValidation
validate_first_party_editor_rich_text_document(const FirstPartyEditorRichTextDocument& document) {
    FirstPartyEditorRichTextValidation validation{.valid = true};
    if (document.id.empty()) {
        append_diagnostic(validation, "missing rich text document id");
    }

    std::vector<std::string> paragraph_ids;
    std::vector<std::string> span_ids;
    for (const auto& paragraph : document.paragraphs) {
        if (paragraph.id.empty()) {
            append_diagnostic(validation, "missing rich text paragraph id");
        } else if (contains_string(paragraph_ids, paragraph.id)) {
            append_diagnostic(validation, "duplicate paragraph id: " + paragraph.id);
        } else {
            paragraph_ids.push_back(paragraph.id);
        }

        for (const auto& span : paragraph.spans) {
            const std::string qualified_span_id = paragraph.id + "." + span.id;
            if (span.id.empty()) {
                append_diagnostic(validation, "missing rich text span id in paragraph: " + paragraph.id);
            } else if (contains_string(span_ids, qualified_span_id)) {
                append_diagnostic(validation, "duplicate span id: " + qualified_span_id);
            } else {
                span_ids.push_back(qualified_span_id);
            }
            if (span.text.empty()) {
                append_diagnostic(validation, "missing rich text span text: " + qualified_span_id);
            }
        }
    }

    validation.valid = validation.diagnostics.empty();
    return validation;
}

ui::UiDocument make_first_party_editor_rich_text_ui_model(const FirstPartyEditorRichTextDocument& document) {
    ui::UiDocument ui_document;
    ui::ElementDesc root = element(document.id, ui::SemanticRole::root);
    root.accessibility_label = document.id;
    add_or_throw(ui_document, std::move(root));

    const ui::ElementId root_id = element_id(document.id);
    for (const auto& paragraph : document.paragraphs) {
        const std::string paragraph_id = document.id + ".paragraph." + paragraph.id;
        ui::ElementDesc paragraph_element = child(paragraph_id, root_id, ui::SemanticRole::panel);
        paragraph_element.accessibility_label = paragraph.id;
        add_or_throw(ui_document, std::move(paragraph_element));

        const ui::ElementId paragraph_element_id = element_id(paragraph_id);
        for (const auto& span : paragraph.spans) {
            ui::ElementDesc span_element =
                child(paragraph_id + ".span." + span.id, paragraph_element_id, ui::SemanticRole::label);
            span_element.text = ui::TextContent{.label = span.text, .font_family = "editor-ui"};
            span_element.style.foreground_token = span.style_token;
            span_element.accessibility_label = span.text;
            add_or_throw(ui_document, std::move(span_element));
        }
    }

    return ui_document;
}

} // namespace mirakana::editor
