// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "first_party_editor_document.hpp"

#include "native_editor_app.hpp"

#include "mirakana/editor/editor_dock_layout.hpp"
#include "mirakana/editor/editor_rich_text.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

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
    } else if (panel_id == "ai_commands") {
        append_rich_text_document(
            document, panel_root,
            make_editor_ai_command_panel_rich_text_document(app.ai_commands(), "editor.panel.ai_commands.rich_text"));
    } else if (panel_id == "project_settings") {
        append_label(document, panel_root, prefix + ".status",
                     "project settings diagnostics " + std::to_string(app.project_settings_errors().size()));
    }
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
    return FirstPartyEditorShellSmokeCounters{
        .ui = "first_party",
        .backend = "d3d12",
        .panel_count = document.panel_root_count,
        .imgui_enabled = false,
        .sdl3_enabled = false,
        .viewport_native_handles_exposed = app.viewport_display().native_texture_handles_exposed,
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
        .docking_status = document.docking_status,
        .dock_tab_header_count = document.tab_header_count,
        .dock_split_gutter_count = document.split_gutter_count,
        .dock_active_panel_count = document.active_panel_count,
        .dock_focusable_control_count = document.focusable_dock_control_count,
    };
}

} // namespace mirakana::editor
