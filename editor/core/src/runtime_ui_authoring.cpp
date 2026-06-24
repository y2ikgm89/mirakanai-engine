// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/runtime_ui_authoring.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {

namespace {

constexpr std::string_view kDocumentSchema = "GameEngine.RuntimeUiDocument.v1";
constexpr std::string_view kThemeSchema = "GameEngine.RuntimeUiTheme.v1";

[[nodiscard]] bool contains_line_separator(std::string_view value) noexcept {
    return value.find('\n') != std::string_view::npos || value.find('\r') != std::string_view::npos;
}

[[nodiscard]] bool is_safe_id(std::string_view value) noexcept {
    return !value.empty() && !contains_line_separator(value) && value.find('=') == std::string_view::npos;
}

[[nodiscard]] bool is_safe_optional_text(std::string_view value) noexcept {
    return !contains_line_separator(value) && value.find('=') == std::string_view::npos;
}

[[nodiscard]] bool schema_is_external_engine(std::string_view schema) noexcept {
    return schema.find("Unity") != std::string_view::npos || schema.find("Unreal") != std::string_view::npos ||
           schema.find("Godot") != std::string_view::npos || schema.find("UXML") != std::string_view::npos ||
           schema.find("UMG") != std::string_view::npos;
}

void add_diagnostic(std::vector<EditorRuntimeUiDiagnosticRow>& diagnostics, std::string code, std::string message) {
    diagnostics.push_back(EditorRuntimeUiDiagnosticRow{.code = std::move(code), .message = std::move(message)});
}

[[nodiscard]] const EditorRuntimeUiElementRow* find_element(const std::vector<EditorRuntimeUiElementRow>& rows,
                                                            std::string_view id) noexcept {
    const auto it = std::ranges::find_if(rows, [id](const EditorRuntimeUiElementRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &*it;
}

[[nodiscard]] EditorRuntimeUiElementRow* find_element_mutable(std::vector<EditorRuntimeUiElementRow>& rows,
                                                              std::string_view id) noexcept {
    const auto it = std::ranges::find_if(rows, [id](const EditorRuntimeUiElementRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &*it;
}

[[nodiscard]] const EditorRuntimeUiStyleTokenRow*
find_style_token(const std::vector<EditorRuntimeUiStyleTokenRow>& rows, std::string_view id) noexcept {
    const auto it = std::ranges::find_if(rows, [id](const EditorRuntimeUiStyleTokenRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &*it;
}

[[nodiscard]] EditorRuntimeUiStyleTokenRow* find_style_token_mutable(std::vector<EditorRuntimeUiStyleTokenRow>& rows,
                                                                     std::string_view id) noexcept {
    const auto it = std::ranges::find_if(rows, [id](const EditorRuntimeUiStyleTokenRow& row) { return row.id == id; });
    return it == rows.end() ? nullptr : &*it;
}

[[nodiscard]] std::optional<std::size_t> find_element_index(const std::vector<EditorRuntimeUiElementRow>& rows,
                                                            std::string_view id) noexcept {
    for (std::size_t index = 0; index < rows.size(); ++index) {
        if (rows[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

[[nodiscard]] bool contains_value(const std::vector<std::string_view>& values, std::string_view value) noexcept {
    return std::ranges::any_of(values, [value](std::string_view candidate) { return candidate == value; });
}

[[nodiscard]] bool has_child(const std::vector<EditorRuntimeUiElementRow>& rows, std::string_view parent_id) noexcept {
    return std::ranges::any_of(
        rows, [parent_id](const EditorRuntimeUiElementRow& row) { return row.parent_id == parent_id; });
}

[[nodiscard]] bool is_descendant_or_same(const std::vector<EditorRuntimeUiElementRow>& rows, std::string_view root_id,
                                         std::string_view candidate_id) noexcept {
    if (root_id == candidate_id) {
        return true;
    }

    const EditorRuntimeUiElementRow* cursor = find_element(rows, candidate_id);
    std::vector<std::string_view> visited;
    visited.reserve(rows.size());
    while (cursor != nullptr && !cursor->parent_id.empty()) {
        if (contains_value(visited, cursor->id)) {
            return false;
        }
        visited.push_back(cursor->id);
        if (cursor->parent_id == root_id) {
            return true;
        }
        cursor = find_element(rows, cursor->parent_id);
    }
    return false;
}

[[nodiscard]] std::size_t calculate_depth(const std::vector<EditorRuntimeUiElementRow>& rows,
                                          const EditorRuntimeUiElementRow& row) noexcept {
    std::size_t depth = 0;
    const EditorRuntimeUiElementRow* cursor = &row;
    std::vector<std::string_view> visited;
    visited.reserve(rows.size());
    while (cursor != nullptr && !cursor->parent_id.empty()) {
        if (contains_value(visited, cursor->id)) {
            break;
        }
        visited.push_back(cursor->id);
        cursor = find_element(rows, cursor->parent_id);
        if (cursor == nullptr) {
            break;
        }
        ++depth;
    }
    return depth;
}

[[nodiscard]] bool has_parent_cycle(const std::vector<EditorRuntimeUiElementRow>& rows,
                                    const EditorRuntimeUiElementRow& row) noexcept {
    const EditorRuntimeUiElementRow* cursor = &row;
    std::vector<std::string_view> visited;
    visited.reserve(rows.size());
    while (cursor != nullptr) {
        if (contains_value(visited, cursor->id)) {
            return true;
        }
        visited.push_back(cursor->id);
        if (cursor->parent_id.empty()) {
            return false;
        }
        cursor = find_element(rows, cursor->parent_id);
    }
    return false;
}

[[nodiscard]] std::string role_label(mirakana::ui::SemanticRole role) {
    return std::string{mirakana::ui::semantic_role_id(role)};
}

void validate_document(const EditorRuntimeUiDocumentModel& document,
                       std::vector<EditorRuntimeUiDiagnosticRow>& diagnostics) {
    if (document.schema_id != kDocumentSchema) {
        add_diagnostic(diagnostics,
                       schema_is_external_engine(document.schema_id) ? "external_engine_schema"
                                                                     : "invalid_document_schema",
                       "runtime UI document schema is not a first-party schema");
    }
    if (document.exposes_native_handles) {
        add_diagnostic(diagnostics, "native_handle_exposure", "runtime UI document exposed native handles");
    }
    if (document.requests_renderer_execution) {
        add_diagnostic(diagnostics, "renderer_execution_request",
                       "editor-core runtime UI document requested rendering");
    }
    if (document.requests_package_script_execution) {
        add_diagnostic(diagnostics, "package_script_execution_request",
                       "editor-core runtime UI document requested package script execution");
    }
    if (document.requests_validation_recipe_execution) {
        add_diagnostic(diagnostics, "validation_recipe_execution_request",
                       "editor-core runtime UI document requested validation execution");
    }

    std::vector<std::string_view> element_ids;
    element_ids.reserve(document.elements.size());
    for (const auto& element : document.elements) {
        if (!is_safe_id(element.id)) {
            add_diagnostic(diagnostics, "invalid_element_id", "runtime UI element id is invalid");
        } else if (contains_value(element_ids, element.id)) {
            add_diagnostic(diagnostics, "duplicate_element_id", "runtime UI element id is duplicated");
        } else {
            element_ids.push_back(element.id);
        }
        if (!element.parent_id.empty() && find_element(document.elements, element.parent_id) == nullptr) {
            add_diagnostic(diagnostics, "missing_parent", "runtime UI element parent is missing");
        }
        if (has_parent_cycle(document.elements, element)) {
            add_diagnostic(diagnostics, "parent_cycle", "runtime UI element hierarchy contains a cycle");
        }
        if (element.role == mirakana::ui::SemanticRole::none) {
            add_diagnostic(diagnostics, "invalid_role", "runtime UI element role is invalid");
        }
        if (!is_safe_optional_text(element.label) || !is_safe_optional_text(element.style_token)) {
            add_diagnostic(diagnostics, "invalid_element_text", "runtime UI element text field is invalid");
        }
    }

    if (!document.selected_element_id.empty() &&
        find_element(document.elements, document.selected_element_id) == nullptr) {
        add_diagnostic(diagnostics, "missing_selection", "runtime UI selected element is missing");
    }
}

void validate_theme(const EditorRuntimeUiThemeModel& theme, std::vector<EditorRuntimeUiDiagnosticRow>& diagnostics) {
    if (theme.schema_id != kThemeSchema) {
        add_diagnostic(diagnostics,
                       schema_is_external_engine(theme.schema_id) ? "external_engine_schema" : "invalid_theme_schema",
                       "runtime UI theme schema is not a first-party schema");
    }
    if (theme.exposes_native_handles) {
        add_diagnostic(diagnostics, "native_handle_exposure", "runtime UI theme exposed native handles");
    }
    if (theme.copied_external_visual_theme) {
        add_diagnostic(diagnostics, "copied_external_theme", "runtime UI theme copied an external visual theme");
    }

    std::vector<std::string_view> style_token_ids;
    style_token_ids.reserve(theme.style_tokens.size());
    for (const auto& token : theme.style_tokens) {
        if (!is_safe_id(token.id)) {
            add_diagnostic(diagnostics, "invalid_style_token", "runtime UI style token id is invalid");
        } else if (contains_value(style_token_ids, token.id)) {
            add_diagnostic(diagnostics, "duplicate_style_token", "runtime UI style token id is duplicated");
        } else {
            style_token_ids.push_back(token.id);
        }
        if (!is_safe_optional_text(token.label) || !is_safe_optional_text(token.foreground_token) ||
            !is_safe_optional_text(token.background_token)) {
            add_diagnostic(diagnostics, "invalid_style_token_value", "runtime UI style token value is invalid");
        }
        if (token.copied_external_visual_theme) {
            add_diagnostic(diagnostics, "copied_external_theme", "runtime UI style token copied an external theme");
        }
    }
}

[[nodiscard]] std::vector<EditorRuntimeUiHierarchyRow>
make_hierarchy_rows(const EditorRuntimeUiDocumentModel& document) {
    std::vector<EditorRuntimeUiHierarchyRow> rows;
    rows.reserve(document.elements.size());
    for (std::size_t index = 0; index < document.elements.size(); ++index) {
        const auto& element = document.elements[index];
        rows.push_back(EditorRuntimeUiHierarchyRow{
            .id = element.id,
            .parent_id = element.parent_id,
            .label = element.label,
            .role = element.role,
            .depth = calculate_depth(document.elements, element),
            .order = index,
            .selected = element.id == document.selected_element_id,
        });
    }
    return rows;
}

[[nodiscard]] std::vector<EditorRuntimeUiInspectorRow>
make_inspector_rows(const EditorRuntimeUiDocumentModel& document) {
    std::vector<EditorRuntimeUiInspectorRow> rows;
    const auto* selected =
        document.selected_element_id.empty() ? nullptr : find_element(document.elements, document.selected_element_id);
    if (selected == nullptr) {
        return rows;
    }

    rows.push_back(EditorRuntimeUiInspectorRow{
        .id = "selected.id", .element_id = selected->id, .label = "Id", .value = selected->id, .editable = false});
    rows.push_back(EditorRuntimeUiInspectorRow{.id = "selected.label",
                                               .element_id = selected->id,
                                               .label = "Text",
                                               .value = selected->label,
                                               .editable = true});
    rows.push_back(EditorRuntimeUiInspectorRow{.id = "selected.role",
                                               .element_id = selected->id,
                                               .label = "Role",
                                               .value = role_label(selected->role),
                                               .editable = false});
    rows.push_back(EditorRuntimeUiInspectorRow{.id = "selected.style",
                                               .element_id = selected->id,
                                               .label = "Style",
                                               .value = selected->style_token,
                                               .editable = true});
    return rows;
}

[[nodiscard]] mirakana::ui::ElementDesc make_preview_element(const EditorRuntimeUiElementRow& row,
                                                             const EditorRuntimeUiThemeModel& theme) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{row.id};
    desc.parent = mirakana::ui::ElementId{row.parent_id};
    desc.role = row.role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    desc.text.label = row.label;
    desc.text.font_family = "ui/body";
    desc.text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    desc.accessibility_label = row.label;

    if (const auto* token = find_style_token(theme.style_tokens, row.style_token); token != nullptr) {
        desc.style.foreground_token = token->foreground_token;
        desc.style.background_token = token->background_token;
    } else {
        desc.style.foreground_token = row.style_token;
    }
    return desc;
}

[[nodiscard]] EditorRuntimeUiPreviewModel make_preview_model(const EditorRuntimeUiDocumentModel& document,
                                                             const EditorRuntimeUiThemeModel& theme) {
    EditorRuntimeUiPreviewModel preview;
    preview.native_handles_exposed = document.exposes_native_handles || theme.exposes_native_handles;
    preview.renderer_execution_requested = document.requests_renderer_execution;
    preview.package_script_execution_requested = document.requests_package_script_execution;
    preview.validation_recipe_execution_requested = document.requests_validation_recipe_execution;

    if (preview.native_handles_exposed) {
        add_diagnostic(preview.diagnostics, "native_handle_exposure", "preview model exposed native handles");
    }
    if (preview.renderer_execution_requested) {
        add_diagnostic(preview.diagnostics, "renderer_execution_request", "preview requested renderer execution");
    }
    if (preview.package_script_execution_requested) {
        add_diagnostic(preview.diagnostics, "package_script_execution_request",
                       "preview requested package script execution");
    }
    if (preview.validation_recipe_execution_requested) {
        add_diagnostic(preview.diagnostics, "validation_recipe_execution_request",
                       "preview requested validation execution");
    }

    for (const auto& element : document.elements) {
        if (!preview.document.try_add_element(make_preview_element(element, theme))) {
            add_diagnostic(preview.diagnostics, "preview_document_invalid", "runtime UI preview element is invalid");
        }
    }

    preview.ready = preview.diagnostics.empty();
    return preview;
}

[[nodiscard]] std::vector<EditorRuntimeUiCommandRow> command_rows() {
    return {
        EditorRuntimeUiCommandRow{.id = "runtime_ui.element.add", .label = "Add element", .mutates_document = true},
        EditorRuntimeUiCommandRow{
            .id = "runtime_ui.element.remove", .label = "Remove element", .mutates_document = true},
        EditorRuntimeUiCommandRow{
            .id = "runtime_ui.element.reorder", .label = "Reorder element", .mutates_document = true},
        EditorRuntimeUiCommandRow{
            .id = "runtime_ui.element.select", .label = "Select element", .mutates_document = true},
        EditorRuntimeUiCommandRow{
            .id = "runtime_ui.property.edit_text", .label = "Edit text", .mutates_document = true},
        EditorRuntimeUiCommandRow{
            .id = "runtime_ui.style.set_token", .label = "Set style token", .mutates_theme = true},
        EditorRuntimeUiCommandRow{.id = "runtime_ui.preview.refresh", .label = "Refresh preview"},
    };
}

void reject(EditorRuntimeUiAuthoringCommandPlan& plan, std::string code, std::string message) {
    plan.accepted = false;
    plan.diagnostic_code = std::move(code);
    plan.diagnostic_message = std::move(message);
}

[[nodiscard]] bool command_is_known(std::string_view command_id) noexcept {
    return command_id == "runtime_ui.element.add" || command_id == "runtime_ui.element.remove" ||
           command_id == "runtime_ui.element.reorder" || command_id == "runtime_ui.element.select" ||
           command_id == "runtime_ui.property.edit_text" || command_id == "runtime_ui.style.set_token" ||
           command_id == "runtime_ui.preview.refresh";
}

[[nodiscard]] bool command_mutates_document(std::string_view command_id) noexcept {
    return command_id == "runtime_ui.element.add" || command_id == "runtime_ui.element.remove" ||
           command_id == "runtime_ui.element.reorder" || command_id == "runtime_ui.element.select" ||
           command_id == "runtime_ui.property.edit_text";
}

[[nodiscard]] bool command_mutates_theme(std::string_view command_id) noexcept {
    return command_id == "runtime_ui.style.set_token";
}

[[nodiscard]] std::vector<EditorRuntimeUiElementRow>
collect_subtree_rows(const std::vector<EditorRuntimeUiElementRow>& rows, std::string_view root_id) {
    std::vector<EditorRuntimeUiElementRow> subtree;
    for (const auto& row : rows) {
        if (is_descendant_or_same(rows, root_id, row.id)) {
            subtree.push_back(row);
        }
    }
    return subtree;
}

void erase_subtree_rows(std::vector<EditorRuntimeUiElementRow>& rows, std::string_view root_id) {
    const auto snapshot = rows;
    std::erase_if(rows, [&snapshot, root_id](const EditorRuntimeUiElementRow& row) {
        return is_descendant_or_same(snapshot, root_id, row.id);
    });
}

[[nodiscard]] std::size_t sibling_end_insertion_index(const std::vector<EditorRuntimeUiElementRow>& rows,
                                                      std::string_view parent_id) noexcept {
    std::optional<std::size_t> last_sibling_index;
    for (std::size_t index = 0; index < rows.size(); ++index) {
        if (rows[index].parent_id == parent_id) {
            last_sibling_index = index;
        }
    }

    if (last_sibling_index.has_value()) {
        std::size_t insertion_index = *last_sibling_index + 1U;
        const auto last_sibling_id = rows[*last_sibling_index].id;
        for (std::size_t index = insertion_index; index < rows.size(); ++index) {
            if (is_descendant_or_same(rows, last_sibling_id, rows[index].id)) {
                insertion_index = index + 1U;
            }
        }
        return insertion_index;
    }

    if (!parent_id.empty()) {
        const auto parent_index = find_element_index(rows, parent_id);
        if (parent_index.has_value()) {
            return *parent_index + 1U;
        }
    }

    return rows.size();
}

} // namespace

EditorRuntimeUiAuthoringModel make_editor_runtime_ui_authoring_model(const EditorRuntimeUiDocumentModel& document,
                                                                     const EditorRuntimeUiThemeModel& theme) {
    EditorRuntimeUiAuthoringModel model;
    validate_document(document, model.diagnostics);
    validate_theme(theme, model.diagnostics);

    model.native_handles_exposed = document.exposes_native_handles || theme.exposes_native_handles;
    model.renderer_execution_requested = document.requests_renderer_execution;
    model.package_script_execution_requested = document.requests_package_script_execution;
    model.validation_recipe_execution_requested = document.requests_validation_recipe_execution;
    model.hierarchy_rows = make_hierarchy_rows(document);
    model.inspector_rows = make_inspector_rows(document);
    model.style_token_rows = theme.style_tokens;
    model.command_rows = command_rows();
    model.preview = make_preview_model(document, theme);
    model.diagnostics.insert(model.diagnostics.end(), model.preview.diagnostics.begin(),
                             model.preview.diagnostics.end());
    model.ready = model.diagnostics.empty() && model.preview.ready && !model.native_handles_exposed &&
                  !model.renderer_execution_requested && !model.package_script_execution_requested &&
                  !model.validation_recipe_execution_requested;
    return model;
}

EditorRuntimeUiAuthoringCommandPlan
plan_editor_runtime_ui_authoring_command(const EditorRuntimeUiDocumentModel& document,
                                         const EditorRuntimeUiThemeModel& theme,
                                         const EditorRuntimeUiAuthoringCommandRequest& request) {
    EditorRuntimeUiAuthoringCommandPlan plan;
    plan.before_document_revision = document.revision;
    plan.after_document_revision = document.revision;
    plan.before_theme_revision = theme.revision;
    plan.after_theme_revision = theme.revision;
    plan.renderer_execution_requested = request.request_renderer_execution;
    plan.package_script_execution_requested = request.request_package_script_execution;
    plan.validation_recipe_execution_requested = request.request_validation_recipe_execution;
    plan.native_handle_exposure_requested = request.request_native_handle_exposure;
    plan.document = document;
    plan.theme = theme;

    if (request.request_renderer_execution || request.request_package_script_execution ||
        request.request_validation_recipe_execution || request.request_native_handle_exposure) {
        reject(plan, "unsafe_request", "runtime UI editor-core command requested unsafe execution or native handles");
        return plan;
    }

    std::vector<EditorRuntimeUiDiagnosticRow> validation_diagnostics;
    validate_document(document, validation_diagnostics);
    validate_theme(theme, validation_diagnostics);
    if (!validation_diagnostics.empty()) {
        reject(plan, validation_diagnostics.front().code, validation_diagnostics.front().message);
        return plan;
    }

    if (!command_is_known(request.command_id)) {
        reject(plan, "unknown_command", "runtime UI command id is unknown");
        return plan;
    }
    if (command_mutates_document(request.command_id)) {
        if (request.expected_document_revision == 0U) {
            reject(plan, "missing_revision", "runtime UI command must provide the expected document revision");
            return plan;
        }
        if (request.expected_document_revision != document.revision) {
            reject(plan, "stale_revision", "runtime UI command expected document revision does not match");
            return plan;
        }
    }
    if (command_mutates_theme(request.command_id)) {
        if (request.expected_theme_revision == 0U) {
            reject(plan, "missing_revision", "runtime UI command must provide the expected theme revision");
            return plan;
        }
        if (request.expected_theme_revision != theme.revision) {
            reject(plan, "stale_revision", "runtime UI command expected theme revision does not match");
            return plan;
        }
    }

    if (request.command_id == "runtime_ui.element.add") {
        if (!is_safe_id(request.element_id) || find_element(document.elements, request.element_id) != nullptr ||
            (!request.parent_id.empty() && find_element(document.elements, request.parent_id) == nullptr) ||
            request.role == mirakana::ui::SemanticRole::none || !is_safe_optional_text(request.label)) {
            reject(plan, "invalid_element_request", "runtime UI add-element command is invalid");
            return plan;
        }

        plan.document.elements.push_back(EditorRuntimeUiElementRow{
            .id = request.element_id,
            .parent_id = request.parent_id,
            .role = request.role,
            .label = request.label,
            .style_token = request.style_token_id,
        });
        plan.document.selected_element_id = request.element_id;
        plan.document.revision = document.revision + 1U;
        plan.after_document_revision = plan.document.revision;
        plan.mutates_document = true;
        plan.accepted = true;
        return plan;
    }

    if (request.command_id == "runtime_ui.element.remove") {
        const auto index = find_element_index(document.elements, request.element_id);
        if (!index.has_value() || request.element_id.empty() || has_child(document.elements, request.element_id)) {
            reject(plan, "invalid_remove_request", "runtime UI remove-element command is invalid");
            return plan;
        }

        plan.document.elements.erase(plan.document.elements.begin() + static_cast<std::ptrdiff_t>(*index));
        if (plan.document.selected_element_id == request.element_id ||
            is_descendant_or_same(document.elements, request.element_id, plan.document.selected_element_id)) {
            plan.document.selected_element_id.clear();
        }
        plan.document.revision = document.revision + 1U;
        plan.after_document_revision = plan.document.revision;
        plan.mutates_document = true;
        plan.accepted = true;
        return plan;
    }

    if (request.command_id == "runtime_ui.element.select") {
        if (find_element(document.elements, request.element_id) == nullptr) {
            reject(plan, "missing_element", "runtime UI select command target is missing");
            return plan;
        }
        plan.document.selected_element_id = request.element_id;
        plan.document.revision = document.revision + 1U;
        plan.after_document_revision = plan.document.revision;
        plan.mutates_document = true;
        plan.accepted = true;
        return plan;
    }

    if (request.command_id == "runtime_ui.property.edit_text") {
        auto* element = find_element_mutable(plan.document.elements, request.element_id);
        if (element == nullptr || !is_safe_optional_text(request.label)) {
            reject(plan, "invalid_property_request", "runtime UI text edit command is invalid");
            return plan;
        }
        element->label = request.label;
        plan.document.revision = document.revision + 1U;
        plan.after_document_revision = plan.document.revision;
        plan.mutates_document = true;
        plan.accepted = true;
        return plan;
    }

    if (request.command_id == "runtime_ui.element.reorder") {
        const auto index = find_element_index(plan.document.elements, request.element_id);
        if (!index.has_value()) {
            reject(plan, "missing_element", "runtime UI reorder command target is missing");
            return plan;
        }
        const auto target = plan.document.elements[*index];
        if (!request.before_element_id.empty()) {
            const auto* before = find_element(plan.document.elements, request.before_element_id);
            if (before == nullptr || before->parent_id != target.parent_id ||
                is_descendant_or_same(plan.document.elements, request.element_id, request.before_element_id)) {
                reject(plan, "invalid_reorder_request", "runtime UI reorder command must target a sibling boundary");
                return plan;
            }
        }

        auto moving_rows = collect_subtree_rows(plan.document.elements, request.element_id);
        erase_subtree_rows(plan.document.elements, request.element_id);
        auto insertion_index = sibling_end_insertion_index(plan.document.elements, target.parent_id);
        if (!request.before_element_id.empty()) {
            insertion_index = *find_element_index(plan.document.elements, request.before_element_id);
        }
        plan.document.elements.insert(plan.document.elements.begin() + static_cast<std::ptrdiff_t>(insertion_index),
                                      moving_rows.begin(), moving_rows.end());
        plan.document.revision = document.revision + 1U;
        plan.after_document_revision = plan.document.revision;
        plan.mutates_document = true;
        plan.accepted = true;
        return plan;
    }

    if (request.command_id == "runtime_ui.style.set_token") {
        auto* token = find_style_token_mutable(plan.theme.style_tokens, request.style_token_id);
        if (token == nullptr || !is_safe_optional_text(request.foreground_token) ||
            !is_safe_optional_text(request.background_token)) {
            reject(plan, "invalid_style_token_request", "runtime UI style-token command is invalid");
            return plan;
        }
        token->foreground_token = request.foreground_token;
        token->background_token = request.background_token;
        plan.theme.revision = theme.revision + 1U;
        plan.after_theme_revision = plan.theme.revision;
        plan.mutates_theme = true;
        plan.accepted = true;
        return plan;
    }

    plan.accepted = true;
    return plan;
}

UndoableAction make_editor_runtime_ui_authoring_command_action(EditorRuntimeUiDocumentModel& document,
                                                               EditorRuntimeUiThemeModel& theme,
                                                               const EditorRuntimeUiAuthoringCommandRequest& request) {
    const auto plan = plan_editor_runtime_ui_authoring_command(document, theme, request);
    if (!plan.accepted || (!plan.mutates_document && !plan.mutates_theme)) {
        return UndoableAction{};
    }

    auto before_document = document;
    auto before_theme = theme;
    auto after_document = plan.document;
    auto after_theme = plan.theme;
    return UndoableAction{
        .label = request.command_id,
        .redo =
            [&document, &theme, after_document = std::move(after_document), after_theme = std::move(after_theme)]() {
                document = after_document;
                theme = after_theme;
            },
        .undo =
            [&document, &theme, before_document = std::move(before_document),
             before_theme = std::move(before_theme)]() {
                document = before_document;
                theme = before_theme;
            },
    };
}

} // namespace mirakana::editor
