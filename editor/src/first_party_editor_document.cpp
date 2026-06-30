// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "first_party_editor_document.hpp"

#include "native_editor_app.hpp"

#include "mirakana/editor/asset_browser_production.hpp"
#include "mirakana/editor/editor_dock_layout.hpp"
#include "mirakana/editor/editor_rich_text.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

[[nodiscard]] ui::ElementId element_id(std::string value) {
    return ui::ElementId{.value = std::move(value)};
}

[[nodiscard]] ui::TextContent text(std::string label) {
    return ui::TextContent{.label = std::move(label), .font_family = "editor-ui"};
}

[[nodiscard]] ui::ElementDesc element(std::string id, ui::SemanticRole role) {
    ui::ElementDesc desc;
    desc.id = element_id(std::move(id));
    desc.role = role;
    desc.accessibility_label = desc.id.value;
    desc.style.layout = ui::LayoutMode::column;
    desc.style.anchor = ui::AnchorMode::fill;
    desc.style.gap = 4.0F;
    desc.style.padding = ui::EdgeInsets{.top = 4.0F, .right = 4.0F, .bottom = 4.0F, .left = 4.0F};
    desc.style.background_token = "editor.panel.background";
    desc.style.foreground_token = "editor.text";
    return desc;
}

[[nodiscard]] ui::ElementDesc child(std::string id, const ui::ElementId& parent, ui::SemanticRole role) {
    ui::ElementDesc desc = element(std::move(id), role);
    desc.parent = parent;
    return desc;
}

[[nodiscard]] std::string dock_element_id(std::string_view dock_node_id) {
    return "editor.dock." + std::string{dock_node_id};
}

[[nodiscard]] std::string dock_tab_bar_id(std::string_view stack_id) {
    return dock_element_id(stack_id) + ".tabs";
}

[[nodiscard]] std::string dock_tab_id(std::string_view stack_id, std::string_view panel_id) {
    return dock_element_id(stack_id) + ".tab." + std::string{panel_id};
}

[[nodiscard]] std::string dock_gutter_id(std::string_view split_id, std::size_t index) {
    return dock_element_id(split_id) + ".gutter." + std::to_string(index);
}

[[nodiscard]] std::string panel_label(std::string_view panel_id) {
    const auto catalog = editor_dock_panel_catalog();
    if (const auto* panel = find_editor_dock_panel(catalog, panel_id); panel != nullptr) {
        return panel->label;
    }
    return std::string{panel_id};
}

[[nodiscard]] bool panel_is_workspace_visible(const NativeEditorApp& app, std::string_view panel_id) {
    const auto catalog = editor_dock_panel_catalog();
    const auto* panel = find_editor_dock_panel(catalog, panel_id);
    return panel == nullptr || !panel->workspace_panel || app.is_panel_visible(panel->workspace_id);
}

void add_or_throw(ui::UiDocument& document, ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("first-party editor document element is invalid or duplicated");
    }
}

void append_label(ui::UiDocument& document, const ui::ElementId& parent, std::string id, std::string label) {
    ui::ElementDesc desc = child(std::move(id), parent, ui::SemanticRole::label);
    desc.text = text(std::move(label));
    desc.accessibility_label = desc.text.label;
    add_or_throw(document, std::move(desc));
}

void append_text_field(ui::UiDocument& document, const ui::ElementId& parent,
                       const NativeEditorTextInputState& text_input) {
    ui::ElementDesc desc = child(text_input.edit_state.target.value, parent, ui::SemanticRole::text_field);
    desc.text = text(text_input.edit_state.text);
    desc.accessibility_label = "Project Name";
    desc.bounds = text_input.caret_bounds;
    desc.enabled = text_input.target_registered;
    desc.style.background_token = "editor.input.background";
    desc.style.foreground_token = "editor.text";
    add_or_throw(document, std::move(desc));

    if (text_input.composition_active) {
        append_label(document, parent, text_input.edit_state.target.value + ".composition",
                     text_input.composition.composition_text);
    }
}

[[nodiscard]] ui::ElementDesc clone_element_desc(const ui::Element& element, const ui::ElementId& fallback_parent) {
    ui::ElementDesc desc;
    desc.id = element.id;
    desc.parent = ui::empty(element.parent) ? fallback_parent : element.parent;
    desc.role = element.role;
    desc.bounds = element.bounds;
    desc.visible = element.visible;
    desc.enabled = element.enabled;
    desc.text = element.text;
    desc.image = element.image;
    desc.accessibility_label = element.accessibility_label;
    desc.style = element.style;
    return desc;
}

void append_rich_text_document(ui::UiDocument& document, const ui::ElementId& parent,
                               const EditorRichTextDocument& rich_text) {
    const auto model = make_editor_rich_text_view_model(
        rich_text, EditorRichTextViewport{.enabled = true, .first_paragraph = 0U, .max_paragraphs = 64U});
    for (const auto& element : model.document.traverse()) {
        add_or_throw(document, clone_element_desc(element, parent));
    }
}

void append_ui_document(ui::UiDocument& document, const ui::ElementId& parent, const ui::UiDocument& source) {
    for (const auto& element : source.traverse()) {
        add_or_throw(document, clone_element_desc(element, parent));
    }
}

[[nodiscard]] std::string bool_label(bool value) {
    return value ? "true" : "false";
}

[[nodiscard]] std::string command_status_label(EnvironmentArtistWorkflowCommandStatus status) {
    switch (status) {
    case EnvironmentArtistWorkflowCommandStatus::accepted:
        return "accepted";
    case EnvironmentArtistWorkflowCommandStatus::rejected_invalid_request:
        return "rejected_invalid_request";
    case EnvironmentArtistWorkflowCommandStatus::rejected_stale_revision:
        return "rejected_stale_revision";
    case EnvironmentArtistWorkflowCommandStatus::rejected_unsafe_execution:
        return "rejected_unsafe_execution";
    }
    return "rejected_invalid_request";
}

[[nodiscard]] bool command_plan_executes_backend(const NativeEditorEnvironmentArtistWorkflowCommandPlanRow& row) {
    return row.dry_run.invokes_backend || row.apply.invokes_backend;
}

[[nodiscard]] bool
command_plan_executes_package_scripts(const NativeEditorEnvironmentArtistWorkflowCommandPlanRow& row) {
    return row.dry_run.executes_package_scripts || row.apply.executes_package_scripts;
}

[[nodiscard]] bool command_plan_exposes_native_handles(const NativeEditorEnvironmentArtistWorkflowCommandPlanRow& row) {
    return row.dry_run.exposes_native_handles || row.apply.exposes_native_handles;
}

[[nodiscard]] std::uint32_t count_legacy_asset_rows(const ui::UiDocument& document) {
    return static_cast<std::uint32_t>(std::ranges::count_if(
        document.traverse(), [](const ui::Element& element) { return element.id.value.starts_with("assets."); }));
}

[[nodiscard]] bool asset_browser_command_enabled(const EditorAssetBrowserProductionModel& model,
                                                 std::string_view command_id) noexcept {
    return std::ranges::any_of(model.command_rows,
                               [command_id](const auto& row) { return row.command_id == command_id && row.enabled; });
}

[[nodiscard]] std::uint32_t
count_asset_import_regression_failed_rows(const EditorAssetBrowserProductionModel& model) noexcept {
    const auto triage_failures = static_cast<std::uint32_t>(std::ranges::count_if(
        model.import_workflow_rows, [](const auto& row) { return row.category_label == "triage_failure"; }));
    if (triage_failures > 0U) {
        return triage_failures;
    }
    return static_cast<std::uint32_t>(std::ranges::count_if(
        model.import_workflow_rows, [](const auto& row) { return row.category_label == "report" && row.blocked; }));
}

[[nodiscard]] bool contains_external_engine_claim_label(std::string_view value) noexcept {
    constexpr std::array claim_terms{"Unity",         "Unreal",      "Godot",  "Asset Store", "Fab",
                                     "compatibility", "equivalence", "parity", "replacement"};
    return std::ranges::any_of(claim_terms,
                               [value](std::string_view term) { return value.find(term) != std::string_view::npos; });
}

[[nodiscard]] bool
asset_import_regression_external_engine_claim(const EditorAssetBrowserProductionModel& model) noexcept {
    return std::ranges::any_of(model.import_workflow_rows, [](const auto& row) {
        return contains_external_engine_claim_label(row.category_label) ||
               contains_external_engine_claim_label(row.asset_id) ||
               contains_external_engine_claim_label(row.asset_key_label) ||
               contains_external_engine_claim_label(row.source_path) ||
               contains_external_engine_claim_label(row.status_label) ||
               contains_external_engine_claim_label(row.detail_label) ||
               contains_external_engine_claim_label(row.diagnostic);
    });
}

void append_environment_artist_workflow_command_plan_rows(ui::UiDocument& document, const ui::ElementId& root,
                                                          const NativeEditorApp& app) {
    const std::string plans_id = "environment_artist_workflow_shell_execution_bridge.command_plans";
    auto plans_root = child(plans_id, root, ui::SemanticRole::list);
    plans_root.accessibility_label = "Environment Artist Workflow Command Plans";
    add_or_throw(document, std::move(plans_root));
    const ui::ElementId plans_root_id{plans_id};

    for (const auto& row : app.environment_artist_workflow_command_plans()) {
        const std::string row_id = plans_id + "." + row.command_id;
        auto item = child(row_id, plans_root_id, ui::SemanticRole::list_item);
        item.enabled = false;
        item.text = text(row.label);
        add_or_throw(document, std::move(item));
        const ui::ElementId item_id{row_id};
        append_label(document, item_id, row_id + ".dry_run_status", command_status_label(row.dry_run.status));
        append_label(document, item_id, row_id + ".dry_run_report_rows",
                     std::to_string(row.dry_run.report_rows.size()));
        append_label(document, item_id, row_id + ".apply_status", command_status_label(row.apply.status));
        append_label(document, item_id, row_id + ".apply_revision_checked", bool_label(row.apply.revision_checked));
        append_label(document, item_id, row_id + ".rollback_metadata_available",
                     bool_label(row.apply.rollback_metadata_available));
        append_label(document, item_id, row_id + ".requires_confirmation", bool_label(row.apply.requires_confirmation));
        append_label(document, item_id, row_id + ".before_revision", std::to_string(row.apply.before_revision));
        append_label(document, item_id, row_id + ".after_revision", std::to_string(row.apply.after_revision));
        append_label(document, item_id, row_id + ".executes_backend", bool_label(command_plan_executes_backend(row)));
        append_label(document, item_id, row_id + ".executes_package_scripts",
                     bool_label(command_plan_executes_package_scripts(row)));
        append_label(document, item_id, row_id + ".native_handles_exposed",
                     bool_label(command_plan_exposes_native_handles(row)));
    }
}

void append_environment_artist_workflow_shell_execution_bridge(ui::UiDocument& document, const NativeEditorApp& app,
                                                               const ui::ElementId& parent) {
    auto root = child("environment_artist_workflow_shell_execution_bridge", parent, ui::SemanticRole::panel);
    root.accessibility_label = "Environment Artist Workflow Execution Bridge";
    root.enabled = false;
    add_or_throw(document, std::move(root));
    const ui::ElementId root_id{"environment_artist_workflow_shell_execution_bridge"};
    const auto& review = app.environment_artist_workflow_execution_review();

    append_label(document, root_id, "environment_artist_workflow_shell_execution_bridge.status",
                 review.status == EnvironmentAuthoringStatus::ready ? "ready" : "blocked");
    append_label(document, root_id, "environment_artist_workflow_shell_execution_bridge.visible_workflow_wired",
                 bool_label(review.visible_first_party_workflow_wired));
    append_label(document, root_id, "environment_artist_workflow_shell_execution_bridge.complete_ready_claimed",
                 bool_label(review.complete_artist_workflow_ready_claimed));
    append_label(document, root_id, "environment_artist_workflow_shell_execution_bridge.executes_backend",
                 bool_label(review.invokes_backend));
    append_label(document, root_id, "environment_artist_workflow_shell_execution_bridge.executes_package_scripts",
                 bool_label(review.executes_package_scripts));
    append_label(document, root_id, "environment_artist_workflow_shell_execution_bridge.executes_validation_recipes",
                 bool_label(review.executes_validation_recipes));
    append_label(document, root_id, "environment_artist_workflow_shell_execution_bridge.native_handles_exposed",
                 bool_label(review.exposes_native_handles));

    append_environment_artist_workflow_command_plan_rows(document, root_id, app);
    append_ui_document(document, root_id, make_environment_artist_workflow_execution_review_ui_model(review));
}

void append_runtime_ui_hierarchy_rows(ui::UiDocument& document, const ui::ElementId& root, std::string_view prefix,
                                      const EditorRuntimeUiAuthoringModel& model) {
    const std::string list_id = std::string{prefix} + ".hierarchy";
    auto list = child(list_id, root, ui::SemanticRole::list);
    list.accessibility_label = "Runtime UI Hierarchy";
    add_or_throw(document, std::move(list));
    const ui::ElementId list_root{list_id};

    for (const auto& row : model.hierarchy_rows) {
        const std::string row_id = list_id + "." + row.id;
        auto item = child(row_id, list_root, ui::SemanticRole::list_item);
        item.text = text(row.label);
        item.accessibility_label = row.label;
        item.enabled = true;
        if (row.selected) {
            item.style.background_token = "editor.panel.focused";
        }
        add_or_throw(document, std::move(item));
        const ui::ElementId item_id{row_id};
        append_label(document, item_id, row_id + ".role", std::string{ui::semantic_role_id(row.role)});
        append_label(document, item_id, row_id + ".depth", std::to_string(row.depth));
        append_label(document, item_id, row_id + ".order", std::to_string(row.order));
    }
}

void append_runtime_ui_inspector_rows(ui::UiDocument& document, const ui::ElementId& root, std::string_view prefix,
                                      const EditorRuntimeUiAuthoringModel& model) {
    const std::string list_id = std::string{prefix} + ".inspector";
    auto list = child(list_id, root, ui::SemanticRole::list);
    list.accessibility_label = "Runtime UI Inspector";
    add_or_throw(document, std::move(list));
    const ui::ElementId list_root{list_id};

    for (const auto& row : model.inspector_rows) {
        const std::string row_id = list_id + "." + row.id;
        auto item = child(row_id, list_root, ui::SemanticRole::list_item);
        item.text = text(row.label + ": " + row.value);
        item.accessibility_label = row.label;
        item.enabled = row.editable;
        add_or_throw(document, std::move(item));
        const ui::ElementId item_id{row_id};
        append_label(document, item_id, row_id + ".element", row.element_id);
        append_label(document, item_id, row_id + ".editable", bool_label(row.editable));
    }
}

void append_runtime_ui_style_rows(ui::UiDocument& document, const ui::ElementId& root, std::string_view prefix,
                                  const EditorRuntimeUiAuthoringModel& model) {
    const std::string list_id = std::string{prefix} + ".style_tokens";
    auto list = child(list_id, root, ui::SemanticRole::list);
    list.accessibility_label = "Runtime UI Style Tokens";
    add_or_throw(document, std::move(list));
    const ui::ElementId list_root{list_id};

    for (const auto& row : model.style_token_rows) {
        const std::string row_id = list_id + "." + row.id;
        auto item = child(row_id, list_root, ui::SemanticRole::list_item);
        item.text = text(row.label);
        item.accessibility_label = row.label;
        add_or_throw(document, std::move(item));
        const ui::ElementId item_id{row_id};
        append_label(document, item_id, row_id + ".foreground", row.foreground_token);
        append_label(document, item_id, row_id + ".background", row.background_token);
        append_label(document, item_id, row_id + ".copied_external_visual_theme",
                     bool_label(row.copied_external_visual_theme));
    }
}

void append_runtime_ui_preview_rows(ui::UiDocument& document, const ui::ElementId& root, std::string_view prefix,
                                    const EditorRuntimeUiAuthoringModel& model) {
    const std::string preview_id = std::string{prefix} + ".preview";
    auto preview = child(preview_id, root, ui::SemanticRole::panel);
    preview.accessibility_label = "Runtime UI Preview";
    add_or_throw(document, std::move(preview));
    const ui::ElementId preview_root{preview_id};

    append_label(document, preview_root, preview_id + ".ready", bool_label(model.preview.ready));
    append_label(document, preview_root, preview_id + ".renderer_upload_status", "not_ready_selected_task_pending");
    append_label(document, preview_root, preview_id + ".renderer_execution_requested",
                 bool_label(model.preview.renderer_execution_requested));
    append_label(document, preview_root, preview_id + ".package_script_execution_requested",
                 bool_label(model.preview.package_script_execution_requested));
    append_label(document, preview_root, preview_id + ".validation_recipe_execution_requested",
                 bool_label(model.preview.validation_recipe_execution_requested));

    const std::string document_id = preview_id + ".document";
    auto preview_document = child(document_id, preview_root, ui::SemanticRole::panel);
    preview_document.accessibility_label = "Runtime UI Preview Document";
    add_or_throw(document, std::move(preview_document));
    append_ui_document(document, ui::ElementId{document_id}, model.preview.document);

    const std::string diagnostics_id = preview_id + ".diagnostics";
    auto diagnostics = child(diagnostics_id, preview_root, ui::SemanticRole::list);
    diagnostics.accessibility_label = "Runtime UI Preview Diagnostics";
    add_or_throw(document, std::move(diagnostics));
    const ui::ElementId diagnostics_root{diagnostics_id};
    for (std::size_t index = 0; index < model.preview.diagnostics.size(); ++index) {
        const auto& diagnostic = model.preview.diagnostics[index];
        append_label(document, diagnostics_root, diagnostics_id + "." + std::to_string(index),
                     diagnostic.code + ": " + diagnostic.message);
    }
}

void append_runtime_ui_platform_readiness_rows(ui::UiDocument& document, const NativeEditorApp& app,
                                               const ui::ElementId& root, std::string_view prefix,
                                               const EditorRuntimeUiAuthoringModel& model) {
    const std::string readiness_id = std::string{prefix} + ".platform_readiness";
    auto readiness = child(readiness_id, root, ui::SemanticRole::panel);
    readiness.accessibility_label = "Runtime UI Platform Readiness";
    readiness.enabled = false;
    add_or_throw(document, std::move(readiness));
    const ui::ElementId readiness_root{readiness_id};

    const auto& text_atlas = app.text_atlas_handoff_evidence();
    const auto& text_input = app.text_input_state();
    const auto& accessibility = app.accessibility_state();
    append_label(document, readiness_root, readiness_id + ".text_font_status", text_atlas.status);
    append_label(document, readiness_root, readiness_id + ".text_font_adapter_invoked",
                 bool_label(text_atlas.text_shaping_adapter_invoked && text_atlas.font_rasterizer_adapter_invoked));
    append_label(document, readiness_root, readiness_id + ".text_font_glyphs_ready",
                 bool_label(text_atlas.glyphs_ready));
    append_label(document, readiness_root, readiness_id + ".text_font_native_handles_exposed",
                 bool_label(text_atlas.native_handles_exposed));
    append_label(document, readiness_root, readiness_id + ".ime_status", native_editor_text_input_status(text_input));
    append_label(document, readiness_root, readiness_id + ".ime_caret_rect_ready",
                 bool_label(text_input.caret_rect_ready));
    append_label(document, readiness_root, readiness_id + ".ime_surrounding_text_ready",
                 bool_label(text_input.surrounding_text_ready));
    append_label(document, readiness_root, readiness_id + ".ime_native_handles_exposed",
                 bool_label(text_input.native_handles_exposed));
    append_label(document, readiness_root, readiness_id + ".accessibility_status", accessibility.status_id);
    append_label(document, readiness_root, readiness_id + ".accessibility_nodes",
                 std::to_string(accessibility.nodes.size()));
    append_label(document, readiness_root, readiness_id + ".accessibility_native_handles_exposed",
                 bool_label(accessibility.native_handles_exposed));
    append_label(document, readiness_root, readiness_id + ".renderer_upload_status",
                 model.preview.renderer_execution_requested ? "blocked_renderer_execution_requested"
                                                            : "selected_d3d12_runtime_ui_atlas_upload_ready");
    append_label(document, readiness_root, readiness_id + ".renderer_upload_native_handles_exposed",
                 bool_label(model.preview.native_handles_exposed));
}

void append_runtime_ui_clean_room_rows(ui::UiDocument& document, const ui::ElementId& root, std::string_view prefix,
                                       const EditorRuntimeUiAuthoringModel& model) {
    const std::string clean_room_id = std::string{prefix} + ".clean_room";
    auto clean_room = child(clean_room_id, root, ui::SemanticRole::panel);
    clean_room.accessibility_label = "Runtime UI Clean Room Review";
    clean_room.enabled = false;
    add_or_throw(document, std::move(clean_room));
    const ui::ElementId clean_room_root{clean_room_id};

    append_label(document, clean_room_root, clean_room_id + ".external_engine_parity_claim", "false");
    append_label(document, clean_room_root, clean_room_id + ".native_handles_exposed",
                 bool_label(model.native_handles_exposed || model.preview.native_handles_exposed));
    append_label(document, clean_room_root, clean_room_id + ".renderer_execution_requested",
                 bool_label(model.renderer_execution_requested || model.preview.renderer_execution_requested));
    append_label(
        document, clean_room_root, clean_room_id + ".package_script_execution_requested",
        bool_label(model.package_script_execution_requested || model.preview.package_script_execution_requested));
    append_label(
        document, clean_room_root, clean_room_id + ".validation_recipe_execution_requested",
        bool_label(model.validation_recipe_execution_requested || model.preview.validation_recipe_execution_requested));
}

void append_runtime_ui_editor_panel(ui::UiDocument& document, const NativeEditorApp& app,
                                    const ui::ElementId& panel_root) {
    const std::string prefix = "editor.panel.runtime_ui_editor.runtime_ui";
    const auto& model = app.runtime_ui_authoring();

    append_label(document, panel_root, prefix + ".status", model.ready ? "ready" : "blocked");
    append_label(document, panel_root, prefix + ".document_id", app.runtime_ui_document().document_id);
    append_label(document, panel_root, prefix + ".document_revision",
                 std::to_string(app.runtime_ui_document().revision));
    append_label(document, panel_root, prefix + ".theme_revision", std::to_string(app.runtime_ui_theme().revision));
    append_runtime_ui_hierarchy_rows(document, panel_root, prefix, model);
    append_runtime_ui_inspector_rows(document, panel_root, prefix, model);
    append_runtime_ui_style_rows(document, panel_root, prefix, model);
    append_runtime_ui_preview_rows(document, panel_root, prefix, model);
    append_runtime_ui_platform_readiness_rows(document, app, panel_root, prefix, model);
    append_runtime_ui_clean_room_rows(document, panel_root, prefix, model);
}

void append_panel_status(ui::UiDocument& document, const NativeEditorApp& app, std::string_view panel_id,
                         const ui::ElementId& panel_root) {
    const std::string prefix = "editor.panel." + std::string{panel_id};
    if (panel_id == "viewport") {
        append_label(document, panel_root, prefix + ".status", "viewport " + app.viewport_display().status_id);
    } else if (panel_id == "inspector") {
        append_rich_text_document(
            document, panel_root,
            make_editor_inspector_rich_text_document(app.inspector_rows(), "editor.panel.inspector.rich_text"));
    } else if (panel_id == "console") {
        append_rich_text_document(
            document, panel_root,
            make_editor_console_rich_text_document(app.console_rows(), "editor.panel.console.rich_text"));
    } else if (panel_id == "resources") {
        append_label(document, panel_root, prefix + ".status", "resources " + app.resources().status);
    } else if (panel_id == "assets") {
        append_ui_document(document, panel_root, make_editor_asset_browser_production_ui_model(app.asset_browser()));
    } else if (panel_id == "ai_commands") {
        append_rich_text_document(
            document, panel_root,
            make_editor_ai_command_panel_rich_text_document(app.ai_commands(), "editor.panel.ai_commands.rich_text"));
    } else if (panel_id == "project_settings") {
        append_label(document, panel_root, prefix + ".status",
                     "project settings diagnostics " + std::to_string(app.project_settings_errors().size()));
        append_text_field(document, panel_root, app.text_input_state());
    } else if (panel_id == "runtime_ui_editor") {
        append_runtime_ui_editor_panel(document, app, panel_root);
    }
}

[[nodiscard]] std::vector<EditorRichTextDocument>
make_first_party_editor_rich_text_documents(const NativeEditorApp& app) {
    return {
        make_editor_console_rich_text_document(app.console_rows(), "editor.panel.console.rich_text"),
        make_editor_ai_command_panel_rich_text_document(app.ai_commands(), "editor.panel.ai_commands.rich_text"),
        make_editor_inspector_rich_text_document(app.inspector_rows(), "editor.panel.inspector.rich_text"),
    };
}

void append_panel_root(ui::UiDocument& document, const NativeEditorApp& app, std::string_view panel_id,
                       const EditorDockLayout& layout, const EditorDockNode& stack, const ui::ElementId& parent,
                       FirstPartyEditorDocument& result) {
    if (!app.has_native_panel(panel_id)) {
        return;
    }
    const auto catalog = editor_dock_panel_catalog();
    const auto* panel = find_editor_dock_panel(catalog, panel_id);
    if (panel != nullptr && panel->workspace_panel && !app.is_panel_visible(panel->workspace_id)) {
        return;
    }

    const std::string panel_root_id = "editor.panel." + std::string{panel_id};
    ui::ElementDesc panel_root = child(panel_root_id, parent, ui::SemanticRole::panel);
    panel_root.accessibility_label = panel_label(panel_id);
    const bool active_panel = stack.active_tab_id == panel_id;
    const bool focused_panel = layout.focused_panel_id == panel_id;
    panel_root.visible = active_panel;
    if (focused_panel) {
        panel_root.style.background_token = "editor.panel.focused";
    } else if (active_panel) {
        panel_root.style.background_token = "editor.panel.active";
    } else {
        panel_root.style.background_token = "editor.panel.inactive";
    }
    add_or_throw(document, std::move(panel_root));
    ++result.panel_root_count;
    if (active_panel) {
        ++result.active_panel_count;
    }

    const ui::ElementId panel_element_id = element_id(panel_root_id);
    append_label(document, panel_element_id, panel_root_id + ".title", panel_label(panel_id));
    append_panel_status(document, app, panel_id, panel_element_id);
}

void append_tab_bar(ui::UiDocument& document, const NativeEditorApp& app, const EditorDockLayout& layout,
                    const EditorDockNode& stack, const ui::ElementId& parent, FirstPartyEditorDocument& result) {
    ui::ElementDesc tab_bar = child(dock_tab_bar_id(stack.id), parent, ui::SemanticRole::list);
    tab_bar.accessibility_label = "Tabs " + stack.id;
    tab_bar.style.layout = ui::LayoutMode::row;
    tab_bar.style.gap = 2.0F;
    tab_bar.style.padding = ui::EdgeInsets{.top = 2.0F, .right = 2.0F, .bottom = 2.0F, .left = 2.0F};
    tab_bar.style.background_token = "editor.dock.tabbar";
    tab_bar.bounds.height = 28.0F;
    add_or_throw(document, std::move(tab_bar));

    const ui::ElementId tab_bar_parent = element_id(dock_tab_bar_id(stack.id));
    for (const auto& tab : stack.tabs) {
        if (!app.has_native_panel(tab)) {
            continue;
        }

        const bool enabled = panel_is_workspace_visible(app, tab);
        const bool active = stack.active_tab_id == tab;
        const bool focused = layout.focused_panel_id == tab;
        ui::ElementDesc tab_button = child(dock_tab_id(stack.id, tab), tab_bar_parent, ui::SemanticRole::button);
        tab_button.accessibility_label = panel_label(tab);
        tab_button.enabled = enabled;
        tab_button.text = text(panel_label(tab));
        tab_button.bounds = ui::Rect{.width = 128.0F, .height = 24.0F};
        if (!enabled) {
            tab_button.style.background_token = "editor.dock.tab.disabled";
        } else if (focused) {
            tab_button.style.background_token = "editor.dock.tab.focused";
        } else if (active) {
            tab_button.style.background_token = "editor.dock.tab.active";
        } else {
            tab_button.style.background_token = "editor.dock.tab.inactive";
        }
        if (enabled) {
            ++result.focusable_dock_control_count;
        }
        if (enabled && focused) {
            result.focused_element = tab_button.id;
        }
        add_or_throw(document, std::move(tab_button));
        ++result.tab_header_count;
    }
}

void append_split_gutter(ui::UiDocument& document, const EditorDockNode& split, const ui::ElementId& parent,
                         std::size_t index, FirstPartyEditorDocument& result) {
    ui::ElementDesc gutter = child(dock_gutter_id(split.id, index), parent, ui::SemanticRole::slider);
    gutter.accessibility_label = "Dock splitter " + split.id + " " + std::to_string(index);
    gutter.enabled = false;
    gutter.style.background_token =
        split.axis == EditorDockSplitAxis::horizontal ? "editor.dock.gutter.horizontal" : "editor.dock.gutter.vertical";
    if (split.axis == EditorDockSplitAxis::horizontal) {
        gutter.bounds = ui::Rect{.width = 6.0F};
    } else {
        gutter.bounds = ui::Rect{.height = 6.0F};
    }
    add_or_throw(document, std::move(gutter));
    ++result.split_gutter_count;
}

void append_dock_node(ui::UiDocument& document, const NativeEditorApp& app, const EditorDockLayout& layout,
                      const EditorDockNode& node, const ui::ElementId& parent, FirstPartyEditorDocument& result) {
    const std::string node_element_id = dock_element_id(node.id);
    ui::ElementDesc node_element = child(node_element_id, parent, ui::SemanticRole::panel);
    node_element.accessibility_label = node.id;
    if (node.kind == EditorDockNodeKind::split) {
        node_element.style.background_token = node.axis == EditorDockSplitAxis::horizontal
                                                  ? "editor.dock.split.horizontal"
                                                  : "editor.dock.split.vertical";
        if (node.axis == EditorDockSplitAxis::horizontal) {
            node_element.style.layout = ui::LayoutMode::row;
        }
    } else {
        node_element.style.background_token = "editor.dock.stack";
    }
    add_or_throw(document, std::move(node_element));

    const ui::ElementId node_parent_id = element_id(node_element_id);
    if (node.kind == EditorDockNodeKind::split) {
        for (std::size_t index = 0; index < node.children.size(); ++index) {
            const auto& child_node_id = node.children[index];
            const EditorDockNode* child_node = find_editor_dock_node(layout, child_node_id);
            if (child_node != nullptr) {
                append_dock_node(document, app, layout, *child_node, node_parent_id, result);
            }
            if (index + 1U < node.children.size()) {
                append_split_gutter(document, node, node_parent_id, index + 1U, result);
            }
        }
    } else if (node.kind == EditorDockNodeKind::tab_stack) {
        append_tab_bar(document, app, layout, node, node_parent_id, result);
        for (const auto& tab : node.tabs) {
            append_panel_root(document, app, tab, layout, node, node_parent_id, result);
        }
    }
}

} // namespace

FirstPartyEditorDocument make_first_party_editor_document(const NativeEditorApp& app) {
    FirstPartyEditorDocument result;
    ui::UiDocument& document = result.document;

    const ui::Rect root_bounds{.x = 0.0F,
                               .y = 0.0F,
                               .width = static_cast<float>(app.options().width),
                               .height = static_cast<float>(app.options().height)};
    ui::ElementDesc root = element("editor.root", ui::SemanticRole::root);
    root.accessibility_label = "MIRAIKANAI Editor";
    root.bounds = root_bounds;
    add_or_throw(document, std::move(root));

    const ui::ElementId root_id = element_id("editor.root");
    ui::ElementDesc dock = child("editor.dock", root_id, ui::SemanticRole::panel);
    dock.accessibility_label = "Editor Dock";
    add_or_throw(document, std::move(dock));

    const EditorDockLayout& layout = app.workspace().dock_layout();
    const auto validation = validate_editor_dock_layout(layout);
    if (!validation.valid) {
        throw std::invalid_argument("first-party editor core dock layout is invalid");
    }
    const EditorDockNode* dock_root = find_editor_dock_node(layout, layout.root_id);
    if (dock_root == nullptr) {
        throw std::invalid_argument("first-party editor core dock layout is missing its root node");
    }
    result.docking_status = "single_window_ready";
    append_dock_node(document, app, layout, *dock_root, element_id("editor.dock"), result);
    append_environment_artist_workflow_shell_execution_bridge(document, app, root_id);
    if (ui::empty(result.focused_element)) {
        for (const auto& element : document.traverse()) {
            if (element.role == ui::SemanticRole::button && element.enabled) {
                result.focused_element = element.id;
                break;
            }
        }
    }

    result.layout = ui::solve_layout(document, root_id, root_bounds);
    result.renderer_submission = ui::build_renderer_submission(document, result.layout);
    return result;
}

FirstPartyEditorShellSmokeCounters
make_first_party_editor_shell_smoke_counters(const NativeEditorApp& app, const FirstPartyEditorDocument& document) {
    const auto& text_atlas = app.text_atlas_handoff_evidence();
    const auto& text_input = app.text_input_state();
    const auto& accessibility = app.accessibility_state();
    const auto& viewport_display = app.viewport_display();
    const auto& material_preview_display = app.material_preview_display();
    const auto workflow_command_plans = app.environment_artist_workflow_command_plans();
    const auto& workflow_review = app.environment_artist_workflow_execution_review();
    const auto& asset_browser = app.asset_browser();
    const auto workflow_executes_backend =
        workflow_review.invokes_backend ||
        std::ranges::any_of(workflow_command_plans, [](const auto& row) { return command_plan_executes_backend(row); });
    const auto workflow_executes_package_scripts =
        workflow_review.executes_package_scripts || std::ranges::any_of(workflow_command_plans, [](const auto& row) {
            return command_plan_executes_package_scripts(row);
        });
    const auto workflow_native_handles_exposed =
        workflow_review.exposes_native_handles || std::ranges::any_of(workflow_command_plans, [](const auto& row) {
            return command_plan_exposes_native_handles(row);
        });
    const auto& runtime_ui = app.runtime_ui_authoring();
    const bool runtime_ui_native_handles_exposed =
        runtime_ui.native_handles_exposed || runtime_ui.preview.native_handles_exposed;
    const auto runtime_ui_preview_rows = static_cast<std::uint32_t>(runtime_ui.preview.document.traverse().size());
    return FirstPartyEditorShellSmokeCounters{
        .ui = "first_party",
        .backend = "d3d12",
        .panel_count = document.panel_root_count,
        .imgui_enabled = false,
        .sdl3_enabled = false,
        .viewport_status = viewport_display.status_id,
        .viewport_visible_texture_composites = viewport_display.visible_texture_composites,
        .viewport_native_handles_exposed = viewport_display.native_texture_handles_exposed,
        .material_preview_status = material_preview_display.status_id,
        .material_preview_visible_texture_composites = material_preview_display.visible_texture_composites,
        .material_preview_native_handles_exposed = app.material_preview_display().native_texture_handles_exposed,
        .text_atlas_handoff_status = text_atlas.status,
        .text_font_adapter_invoked =
            text_atlas.text_shaping_adapter_invoked && text_atlas.font_rasterizer_adapter_invoked,
        .text_font_glyphs_ready = text_atlas.glyphs_ready,
        .text_font_fallback_used = text_atlas.fallback_used,
        .text_atlas_handoff_ready = text_atlas.atlas_handoff_ready,
        .text_font_native_handles_exposed = text_atlas.native_handles_exposed,
        .text_atlas_handoff_host_gated_rows = text_atlas.host_gated_rows,
        .text_atlas_handoff_unsupported_rows = text_atlas.unsupported_rows,
        .ime_status = native_editor_text_input_status(text_input),
        .ime_text_input_session_rows = text_input.session_active ? 1U : 0U,
        .ime_composition_rows = text_input.composition_active ? 1U : 0U,
        .ime_committed_text_rows = text_input.commit_applied ? 1U : 0U,
        .ime_caret_rect_rows = text_input.caret_rect_ready ? 1U : 0U,
        .ime_surrounding_text_rows = text_input.surrounding_text_ready ? 1U : 0U,
        .ime_candidate_ui_host_owned = text_input.candidate_ui_host_owned,
        .ime_native_handles_exposed = text_input.native_handles_exposed,
        .accessibility_status = accessibility.status_id,
        .accessibility_nodes = static_cast<std::uint32_t>(accessibility.nodes.size()),
        .accessibility_role_rows = accessibility.role_rows,
        .accessibility_name_rows = accessibility.name_rows,
        .accessibility_state_rows = accessibility.state_rows,
        .accessibility_focus_rows = accessibility.focus_rows,
        .accessibility_action_rows = accessibility.action_rows,
        .accessibility_relationship_rows = accessibility.relationship_rows,
        .accessibility_tree_navigation_rows = accessibility.tree_navigation_rows,
        .accessibility_diagnostics = static_cast<std::uint32_t>(accessibility.diagnostics.size()),
        .accessibility_missing_name_diagnostics = accessibility.missing_name_diagnostics,
        .accessibility_missing_role_diagnostics = accessibility.missing_role_diagnostics,
        .accessibility_invalid_bounds_diagnostics = accessibility.invalid_bounds_diagnostics,
        .accessibility_hidden_nodes = accessibility.hidden_nodes,
        .accessibility_unsupported_pattern_diagnostics = accessibility.unsupported_pattern_diagnostics,
        .accessibility_native_handles_exposed = accessibility.native_handles_exposed,
        .environment_artist_workflow_command_plan_rows = static_cast<std::uint32_t>(workflow_command_plans.size()),
        .environment_artist_workflow_execution_review_rows =
            static_cast<std::uint32_t>(workflow_review.stage_rows.size()),
        .environment_artist_workflow_external_execution_rows = static_cast<std::uint32_t>(std::ranges::count_if(
            workflow_review.stage_rows, [](const auto& row) { return row.external_execution_required; })),
        .environment_artist_workflow_operator_review_rows = static_cast<std::uint32_t>(
            std::ranges::count_if(workflow_review.stage_rows, [](const auto& row) { return row.operator_reviewed; })),
        .environment_artist_workflow_executes_backend = workflow_executes_backend,
        .environment_artist_workflow_executes_package_scripts = workflow_executes_package_scripts,
        .environment_artist_workflow_executes_validation_recipes = workflow_review.executes_validation_recipes,
        .environment_artist_workflow_native_handles_exposed = workflow_native_handles_exposed,
        .environment_artist_workflow_ready_claimed = workflow_review.complete_artist_workflow_ready_claimed,
        .editor_asset_browser_visible = app.has_native_panel("assets") && app.is_panel_visible(PanelId::assets) &&
                                        document.document.find(ui::ElementId{"asset_browser"}) != nullptr,
        .editor_asset_browser_source_pulse_rows = static_cast<std::uint32_t>(asset_browser.rows.size()),
        .editor_asset_browser_hardcoded_rows = count_legacy_asset_rows(document.document),
        .editor_asset_browser_native_handles_exposed = asset_browser.exposes_native_handles,
        .editor_asset_import_regression_workflow_visible =
            !asset_browser.import_workflow_rows.empty() &&
            document.document.find(ui::ElementId{"asset_browser.import_workflow"}) != nullptr,
        .editor_asset_import_regression_workflow_rows =
            static_cast<std::uint32_t>(asset_browser.import_workflow_rows.size()),
        .editor_asset_import_regression_failed_rows = count_asset_import_regression_failed_rows(asset_browser),
        .editor_asset_import_regression_reimport_command_enabled =
            asset_browser_command_enabled(asset_browser, "asset_browser.import.batch_reimport"),
        .editor_asset_import_regression_preset_diff_command_enabled =
            asset_browser_command_enabled(asset_browser, "asset_browser.import.preset_diff"),
        .editor_asset_import_regression_axis_unit_preview_command_enabled =
            asset_browser_command_enabled(asset_browser, "asset_browser.import.axis_unit_preview"),
        .editor_asset_import_regression_importers_executed_in_core = std::ranges::any_of(
            asset_browser.import_workflow_rows, [](const auto& row) { return row.executes_import_tools; }),
        .editor_asset_import_regression_native_handles_exposed = std::ranges::any_of(
            asset_browser.import_workflow_rows, [](const auto& row) { return row.exposes_native_handles; }),
        .editor_asset_import_regression_external_engine_claim =
            asset_import_regression_external_engine_claim(asset_browser),
        .runtime_ui_editor_panel_visible =
            app.has_native_panel("runtime_ui_editor") && app.is_panel_visible(PanelId::runtime_ui_editor),
        .runtime_ui_editor_hierarchy_rows = static_cast<std::uint32_t>(runtime_ui.hierarchy_rows.size()),
        .runtime_ui_editor_inspector_rows = static_cast<std::uint32_t>(runtime_ui.inspector_rows.size()),
        .runtime_ui_editor_style_rows = static_cast<std::uint32_t>(runtime_ui.style_token_rows.size()),
        .runtime_ui_editor_preview_rows = runtime_ui_preview_rows,
        .runtime_ui_editor_external_engine_parity_claim = false,
        .runtime_ui_editor_native_handles_exposed = runtime_ui_native_handles_exposed,
        .docking_status = document.docking_status,
        .dock_tab_header_count = document.tab_header_count,
        .dock_split_gutter_count = document.split_gutter_count,
        .dock_active_panel_count = document.active_panel_count,
        .dock_focusable_control_count = document.focusable_dock_control_count,
    };
}

EditorAiOperationUxStatusDesc
make_first_party_editor_ai_operation_ux_status_desc(const NativeEditorApp& app,
                                                    const FirstPartyEditorDocument& document) {
    const auto counters = make_first_party_editor_shell_smoke_counters(app, document);
    return EditorAiOperationUxStatusDesc{
        .selected_dock_panel_id = app.workspace().dock_layout().focused_panel_id,
        .rich_text_document_count = make_first_party_editor_rich_text_documents(app).size(),
        .focused_text_target_id = app.text_input_state().edit_state.target.value,
        .text_input_status = counters.ime_status,
        .ime_service_id = app.services().ime_service_id,
        .ime_text_input_session_rows = counters.ime_text_input_session_rows,
        .ime_composition_rows = counters.ime_composition_rows,
        .ime_committed_text_rows = counters.ime_committed_text_rows,
        .ime_caret_rect_rows = counters.ime_caret_rect_rows,
        .ime_surrounding_text_rows = counters.ime_surrounding_text_rows,
        .ime_candidate_ui_host_owned = counters.ime_candidate_ui_host_owned,
        .ime_native_handles_exposed = counters.ime_native_handles_exposed,
        .text_atlas_handoff_status = counters.text_atlas_handoff_status,
        .text_font_adapter_invoked = counters.text_font_adapter_invoked,
        .text_font_glyphs_ready = counters.text_font_glyphs_ready,
        .text_font_fallback_used = counters.text_font_fallback_used,
        .text_atlas_handoff_ready = counters.text_atlas_handoff_ready,
        .text_font_native_handles_exposed = counters.text_font_native_handles_exposed,
        .text_atlas_handoff_host_gated_rows = counters.text_atlas_handoff_host_gated_rows,
        .text_atlas_handoff_unsupported_rows = counters.text_atlas_handoff_unsupported_rows,
        .accessibility_status = counters.accessibility_status,
        .accessibility_nodes = counters.accessibility_nodes,
        .accessibility_role_rows = counters.accessibility_role_rows,
        .accessibility_name_rows = counters.accessibility_name_rows,
        .accessibility_state_rows = counters.accessibility_state_rows,
        .accessibility_focus_rows = counters.accessibility_focus_rows,
        .accessibility_action_rows = counters.accessibility_action_rows,
        .accessibility_relationship_rows = counters.accessibility_relationship_rows,
        .accessibility_tree_navigation_rows = counters.accessibility_tree_navigation_rows,
        .accessibility_diagnostics = counters.accessibility_diagnostics,
        .accessibility_missing_name_diagnostics = counters.accessibility_missing_name_diagnostics,
        .accessibility_missing_role_diagnostics = counters.accessibility_missing_role_diagnostics,
        .accessibility_invalid_bounds_diagnostics = counters.accessibility_invalid_bounds_diagnostics,
        .accessibility_hidden_nodes = counters.accessibility_hidden_nodes,
        .accessibility_unsupported_pattern_diagnostics = counters.accessibility_unsupported_pattern_diagnostics,
        .accessibility_native_handles_exposed = counters.accessibility_native_handles_exposed,
        .viewport_status = counters.viewport_status,
        .viewport_visible_texture_composites = counters.viewport_visible_texture_composites,
        .viewport_native_handles_exposed = counters.viewport_native_handles_exposed,
        .material_preview_status = counters.material_preview_status,
        .material_preview_visible_texture_composites = counters.material_preview_visible_texture_composites,
        .material_preview_native_handles_exposed = counters.material_preview_native_handles_exposed,
    };
}

EditorAiOperationSnapshot make_first_party_editor_ai_operation_snapshot(const NativeEditorApp& app,
                                                                        const FirstPartyEditorDocument& document) {
    const auto ux_status = make_first_party_editor_ai_operation_ux_status_desc(app, document);
    const auto status_rows = make_editor_ai_operation_ux_status_rows(ux_status);
    const auto rich_text_documents = make_first_party_editor_rich_text_documents(app);
    return make_editor_ai_operation_snapshot(
        app.workspace(), app.workspace().dock_layout(), rich_text_documents, status_rows,
        EditorRichTextViewport{.enabled = true, .first_paragraph = 0U, .max_paragraphs = 64U});
}

} // namespace mirakana::editor
