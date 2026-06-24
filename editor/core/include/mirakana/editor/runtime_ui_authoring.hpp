// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/history.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::editor {

struct EditorRuntimeUiElementRow {
    std::string id;
    std::string parent_id;
    mirakana::ui::SemanticRole role{mirakana::ui::SemanticRole::none};
    std::string label;
    std::string style_token;
};

struct EditorRuntimeUiStyleTokenRow {
    std::string id;
    std::string label;
    std::string foreground_token;
    std::string background_token;
    bool copied_external_visual_theme{false};
};

struct EditorRuntimeUiDocumentModel {
    std::string schema_id{"GameEngine.RuntimeUiDocument.v1"};
    std::string document_id;
    std::uint64_t revision{1};
    std::vector<EditorRuntimeUiElementRow> elements;
    std::string selected_element_id;
    bool exposes_native_handles{false};
    bool requests_renderer_execution{false};
    bool requests_package_script_execution{false};
    bool requests_validation_recipe_execution{false};
};

struct EditorRuntimeUiThemeModel {
    std::string schema_id{"GameEngine.RuntimeUiTheme.v1"};
    std::uint64_t revision{1};
    std::vector<EditorRuntimeUiStyleTokenRow> style_tokens;
    bool copied_external_visual_theme{false};
    bool exposes_native_handles{false};
};

struct EditorRuntimeUiHierarchyRow {
    std::string id;
    std::string parent_id;
    std::string label;
    mirakana::ui::SemanticRole role{mirakana::ui::SemanticRole::none};
    std::size_t depth{0};
    std::size_t order{0};
    bool selected{false};
};

struct EditorRuntimeUiInspectorRow {
    std::string id;
    std::string element_id;
    std::string label;
    std::string value;
    bool editable{true};
};

struct EditorRuntimeUiDiagnosticRow {
    std::string code;
    std::string message;
};

struct EditorRuntimeUiPreviewModel {
    mirakana::ui::UiDocument document;
    bool ready{false};
    bool native_handles_exposed{false};
    bool renderer_execution_requested{false};
    bool package_script_execution_requested{false};
    bool validation_recipe_execution_requested{false};
    std::vector<EditorRuntimeUiDiagnosticRow> diagnostics;
};

struct EditorRuntimeUiCommandRow {
    std::string id;
    std::string label;
    bool mutates_document{false};
    bool mutates_theme{false};
};

struct EditorRuntimeUiAuthoringModel {
    bool ready{false};
    std::vector<EditorRuntimeUiHierarchyRow> hierarchy_rows;
    std::vector<EditorRuntimeUiInspectorRow> inspector_rows;
    std::vector<EditorRuntimeUiStyleTokenRow> style_token_rows;
    std::vector<EditorRuntimeUiCommandRow> command_rows;
    EditorRuntimeUiPreviewModel preview;
    std::vector<EditorRuntimeUiDiagnosticRow> diagnostics;
    bool native_handles_exposed{false};
    bool renderer_execution_requested{false};
    bool package_script_execution_requested{false};
    bool validation_recipe_execution_requested{false};
};

struct EditorRuntimeUiAuthoringCommandRequest {
    std::string command_id;
    std::uint64_t expected_document_revision{0};
    std::uint64_t expected_theme_revision{0};
    std::string parent_id;
    std::string element_id;
    std::string before_element_id;
    mirakana::ui::SemanticRole role{mirakana::ui::SemanticRole::none};
    std::string label;
    std::string style_token_id;
    std::string foreground_token;
    std::string background_token;
    bool request_renderer_execution{false};
    bool request_package_script_execution{false};
    bool request_validation_recipe_execution{false};
    bool request_native_handle_exposure{false};
};

struct EditorRuntimeUiAuthoringCommandPlan {
    bool accepted{false};
    bool mutates_document{false};
    bool mutates_theme{false};
    std::uint64_t before_document_revision{0};
    std::uint64_t after_document_revision{0};
    std::uint64_t before_theme_revision{0};
    std::uint64_t after_theme_revision{0};
    std::string diagnostic_code;
    std::string diagnostic_message;
    bool renderer_execution_requested{false};
    bool package_script_execution_requested{false};
    bool validation_recipe_execution_requested{false};
    bool native_handle_exposure_requested{false};
    EditorRuntimeUiDocumentModel document;
    EditorRuntimeUiThemeModel theme;
};

[[nodiscard]] EditorRuntimeUiAuthoringModel
make_editor_runtime_ui_authoring_model(const EditorRuntimeUiDocumentModel& document,
                                       const EditorRuntimeUiThemeModel& theme);

[[nodiscard]] EditorRuntimeUiAuthoringCommandPlan
plan_editor_runtime_ui_authoring_command(const EditorRuntimeUiDocumentModel& document,
                                         const EditorRuntimeUiThemeModel& theme,
                                         const EditorRuntimeUiAuthoringCommandRequest& request);

[[nodiscard]] UndoableAction
make_editor_runtime_ui_authoring_command_action(EditorRuntimeUiDocumentModel& document,
                                                EditorRuntimeUiThemeModel& theme,
                                                const EditorRuntimeUiAuthoringCommandRequest& request);

} // namespace mirakana::editor
