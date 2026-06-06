// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "first_party_editor_document.hpp"

#include "native_editor_app.hpp"

#include "mirakana/editor/editor_dock_layout.hpp"
#include "mirakana/editor/editor_rich_text.hpp"
#include "mirakana/editor/editor_ui_performance.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

using EditorUiPerformanceClock = std::chrono::steady_clock;

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

[[nodiscard]] std::uint64_t count_to_u64(std::size_t value) noexcept {
    return static_cast<std::uint64_t>(value);
}

[[nodiscard]] double elapsed_us(EditorUiPerformanceClock::time_point begin,
                                EditorUiPerformanceClock::time_point end) noexcept {
    const auto value = std::chrono::duration<double, std::micro>(end - begin).count();
    return value > 0.0 ? value : 1.0;
}

[[nodiscard]] std::uint64_t payload_bytes(std::size_t count, std::size_t element_size) noexcept {
    return static_cast<std::uint64_t>(count) * static_cast<std::uint64_t>(element_size);
}

[[nodiscard]] std::uint64_t retained_payload_high_water_bytes(const FirstPartyEditorDocument& document) noexcept {
    return payload_bytes(document.document.size(), sizeof(ui::Element)) +
           payload_bytes(document.layout.elements.size(), sizeof(ui::ElementLayout)) +
           payload_bytes(document.renderer_submission.elements.size(), sizeof(ui::Element)) +
           payload_bytes(document.renderer_submission.layouts.size(), sizeof(ui::ElementLayout)) +
           payload_bytes(document.renderer_submission.boxes.size(), sizeof(ui::RendererBox)) +
           payload_bytes(document.renderer_submission.text_runs.size(), sizeof(ui::RendererTextRun)) +
           payload_bytes(document.renderer_submission.image_placeholders.size(), sizeof(ui::RendererImagePlaceholder)) +
           payload_bytes(document.renderer_submission.accessibility_nodes.size(), sizeof(ui::AccessibilityNode));
}

[[nodiscard]] EditorUiPerformanceSummary
summarize_first_party_editor_ui_performance(const NativeEditorApp& app, const FirstPartyEditorDocument& document) {
    const auto text_run_count = count_to_u64(document.renderer_submission.text_runs.size());
    const auto renderer_box_count = count_to_u64(document.renderer_submission.boxes.size());
    const auto visible_texture_composites =
        app.viewport_display().visible_texture_composites + app.material_preview_display().visible_texture_composites;
    const std::array samples{
        EditorUiPerformanceSample{
            .layout_us = document.layout_us,
            .document_build_us = document.document_build_us,
            .renderer_submission_us = document.renderer_submission_us,
            .text_runs = text_run_count,
            .renderer_boxes = renderer_box_count,
            .visible_texture_composites = visible_texture_composites,
            .memory_high_water_bytes = document.retained_memory_high_water_bytes,
        },
    };
    const auto budgets = make_default_editor_ui_performance_budgets();
    return summarize_editor_ui_performance(samples, budgets);
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

[[nodiscard]] bool vulkan_backend(std::string_view backend_id) noexcept {
    return backend_id == "vulkan";
}

[[nodiscard]] bool metal_backend(std::string_view backend_id) noexcept {
    return backend_id == "metal";
}

void add_or_throw(ui::UiDocument& document, ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("first-party editor document element is invalid or duplicated");
    }
}

void append_label(ui::UiDocument& document, const ui::ElementId& parent, std::string id, std::string label,
                  bool live_region = false) {
    ui::ElementDesc desc = child(std::move(id), parent, ui::SemanticRole::label);
    desc.text = text(std::move(label));
    desc.accessibility_label = desc.text.label;
    desc.accessibility_live_region = live_region;
    add_or_throw(document, std::move(desc));
}

void append_checkbox(ui::UiDocument& document, const ui::ElementId& parent, std::string id, std::string label,
                     bool enabled = true) {
    ui::ElementDesc desc = child(std::move(id), parent, ui::SemanticRole::checkbox);
    desc.text = text(std::move(label));
    desc.accessibility_label = desc.text.label;
    desc.enabled = enabled;
    desc.bounds = ui::Rect{.width = 160.0F, .height = 24.0F};
    desc.style.background_token = "editor.checkbox.background";
    desc.style.foreground_token = "editor.text";
    add_or_throw(document, std::move(desc));
}

void append_text_field_row(ui::UiDocument& document, const ui::ElementId& parent, std::string id, std::string label,
                           std::string value) {
    ui::ElementDesc desc = child(std::move(id), parent, ui::SemanticRole::text_field);
    desc.text = text(std::move(value));
    desc.accessibility_label = std::move(label);
    desc.bounds = ui::Rect{.width = 240.0F, .height = 24.0F};
    desc.enabled = true;
    desc.style.background_token = "editor.input.background";
    desc.style.foreground_token = "editor.text";
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
    desc.accessibility_live_region = element.accessibility_live_region;
    desc.style = element.style;
    return desc;
}

void append_rich_text_document(ui::UiDocument& document, const ui::ElementId& parent,
                               const EditorRichTextDocument& rich_text) {
    const auto model = make_editor_rich_text_view_model(
        rich_text, EditorRichTextViewport{.enabled = true, .first_paragraph = 0U, .max_paragraphs = 64U});
    const bool live_region =
        rich_text.id.find("console") != std::string::npos || rich_text.id.find("ai_commands") != std::string::npos;
    for (const auto& element : model.document.traverse()) {
        auto desc = clone_element_desc(element, parent);
        desc.accessibility_live_region = desc.accessibility_live_region || live_region;
        add_or_throw(document, std::move(desc));
    }
    if (!rich_text.editable) {
        return;
    }

    const ui::ElementId rich_text_root = element_id(rich_text.id);
    ui::ElementDesc command_bar = child(rich_text.id + ".commands", rich_text_root, ui::SemanticRole::list);
    command_bar.accessibility_label = "Rich text edit commands";
    command_bar.style.layout = ui::LayoutMode::row;
    command_bar.style.gap = 2.0F;
    add_or_throw(document, std::move(command_bar));

    const ui::ElementId command_bar_id = element_id(rich_text.id + ".commands");
    const std::array commands{
        std::pair{".insert_text", "Insert"},         std::pair{".delete_selection", "Delete"},
        std::pair{".replace_selection", "Replace"},  std::pair{".toggle_bold", "Bold"},
        std::pair{".toggle_italic", "Italic"},       std::pair{".copy_rich_text", "Copy Rich"},
        std::pair{".cut_selection", "Cut"},          std::pair{".paste_plain_text", "Paste Plain"},
        std::pair{".paste_rich_text", "Paste Rich"},
    };
    for (const auto& [suffix, label] : commands) {
        ui::ElementDesc button =
            child(rich_text.id + ".command" + std::string{suffix}, command_bar_id, ui::SemanticRole::button);
        button.text = text(label);
        button.accessibility_label = button.text.label;
        button.enabled = true;
        button.bounds = ui::Rect{.width = 96.0F, .height = 24.0F};
        add_or_throw(document, std::move(button));
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
        append_text_field_row(document, panel_root, prefix + ".filter.text_field", "Inspector Filter", "filter");
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
        append_text_field(document, panel_root, app.text_input_state());
        append_checkbox(document, panel_root, prefix + ".autosave", "Autosave enabled");
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
    const auto document_build_begin = EditorUiPerformanceClock::now();
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

    const auto document_build_end = EditorUiPerformanceClock::now();
    result.document_build_us = elapsed_us(document_build_begin, document_build_end);

    const auto layout_begin = EditorUiPerformanceClock::now();
    result.layout = ui::solve_layout(document, root_id, root_bounds);
    const auto layout_end = EditorUiPerformanceClock::now();
    result.layout_us = elapsed_us(layout_begin, layout_end);

    const auto renderer_submission_begin = EditorUiPerformanceClock::now();
    result.renderer_submission = ui::build_renderer_submission(document, result.layout);
    const auto renderer_submission_end = EditorUiPerformanceClock::now();
    result.renderer_submission_us = elapsed_us(renderer_submission_begin, renderer_submission_end);
    result.retained_memory_high_water_bytes = retained_payload_high_water_bytes(result);
    return result;
}

FirstPartyEditorShellSmokeCounters
make_first_party_editor_shell_smoke_counters(const NativeEditorApp& app, const FirstPartyEditorDocument& document) {
    const auto& text_atlas = app.text_atlas_handoff_evidence();
    const auto& text_input = app.text_input_state();
    const auto& accessibility = app.accessibility_state();
    const auto& viewport_display = app.viewport_display();
    const auto& material_preview_display = app.material_preview_display();
    const auto& retained_diff = app.retained_ui_diff();
    const auto performance = summarize_first_party_editor_ui_performance(app, document);
    const auto& multi_window_layout = app.workspace().multi_window_dock_layout();
    const auto multi_window_validation = validate_editor_dock_multi_window_layout(multi_window_layout);
    auto tear_off_layout = multi_window_layout;
    const auto tear_off_plan = plan_editor_dock_window_command(
        tear_off_layout,
        EditorDockWindowCommandRequest{
            .kind = EditorDockWindowCommandKind::tear_off_panel,
            .window_id = "window.main",
            .new_window_id = "window.assets",
            .panel_id = "assets",
            .source_stack_id = "dock.left_stack",
            .monitor_id = "monitor.side",
            .bounds = EditorDockWindowBounds{.x = 100.0F, .y = 120.0F, .width = 640.0F, .height = 480.0F},
            .dpi_scale = 1.5F,
        });
    if (tear_off_plan.accepted) {
        tear_off_layout = tear_off_plan.result_layout;
    }
    const auto merge_plan =
        plan_editor_dock_window_command(tear_off_layout, EditorDockWindowCommandRequest{
                                                             .kind = EditorDockWindowCommandKind::merge_window,
                                                             .window_id = "window.assets",
                                                             .target_window_id = "window.main",
                                                             .target_stack_id = "dock.right_stack",
                                                         });
    bool workspace_v3_ready = false;
    try {
        const auto round_trip = deserialize_workspace_v3(serialize_workspace_v3(app.workspace()));
        workspace_v3_ready = validate_editor_dock_multi_window_layout(round_trip.multi_window_dock_layout()).valid;
    } catch (const std::exception&) {
        workspace_v3_ready = false;
    }
    const bool multi_window_handles_exposed =
        multi_window_layout.native_handles_public ||
        std::ranges::any_of(multi_window_layout.unsupported_capabilities,
                            [](const auto& capability) { return capability.native_handles_public; });
    const bool multi_window_ready = multi_window_validation.valid && tear_off_plan.accepted && merge_plan.accepted &&
                                    workspace_v3_ready && !multi_window_handles_exposed;
    const auto rich_text_documents = make_first_party_editor_rich_text_documents(app);
    const auto rich_text_editable_documents = static_cast<std::uint32_t>(std::ranges::count_if(
        rich_text_documents, [](const EditorRichTextDocument& rich_text) { return rich_text.editable; }));
    const bool rich_text_native_handles_exposed = std::ranges::any_of(rich_text_documents, [](const auto& rich_text) {
        return std::ranges::any_of(rich_text.unsupported_capabilities,
                                   [](const auto& capability) { return capability.native_handles_public; });
    });
    constexpr std::uint32_t rich_text_commands_per_editable_document = 9U;
    const bool viewport_vulkan_selected = vulkan_backend(viewport_display.backend_id);
    const bool material_preview_vulkan_selected = vulkan_backend(material_preview_display.backend_id);
    const bool viewport_metal_selected = metal_backend(viewport_display.backend_id);
    const bool material_preview_metal_selected = metal_backend(material_preview_display.backend_id);
    const bool vulkan_native_handles_exposed =
        (viewport_vulkan_selected && viewport_display.native_texture_handles_exposed) ||
        (material_preview_vulkan_selected && material_preview_display.native_texture_handles_exposed);
    const bool metal_native_handles_exposed =
        (viewport_metal_selected && viewport_display.native_texture_handles_exposed) ||
        (material_preview_metal_selected && material_preview_display.native_texture_handles_exposed);
    return FirstPartyEditorShellSmokeCounters{
        .ui = "first_party",
        .backend = "d3d12",
        .panel_count = document.panel_root_count,
        .imgui_enabled = false,
        .sdl3_enabled = false,
        .viewport_status = viewport_display.status_id,
        .viewport_visible_texture_composites = viewport_display.visible_texture_composites,
        .viewport_native_handles_exposed = viewport_display.native_texture_handles_exposed,
        .viewport_vulkan_status = viewport_vulkan_selected ? viewport_display.status_id : "host_gated",
        .viewport_vulkan_visible_texture_composites =
            viewport_vulkan_selected ? viewport_display.visible_texture_composites : 0U,
        .viewport_metal_status = viewport_metal_selected ? viewport_display.status_id : "host_gated",
        .viewport_metal_visible_texture_composites =
            viewport_metal_selected ? viewport_display.visible_texture_composites : 0U,
        .material_preview_status = material_preview_display.status_id,
        .material_preview_visible_texture_composites = material_preview_display.visible_texture_composites,
        .material_preview_native_handles_exposed = app.material_preview_display().native_texture_handles_exposed,
        .material_preview_vulkan_status =
            material_preview_vulkan_selected ? material_preview_display.status_id : "host_gated",
        .material_preview_vulkan_visible_texture_composites =
            material_preview_vulkan_selected ? material_preview_display.visible_texture_composites : 0U,
        .material_preview_metal_status =
            material_preview_metal_selected ? material_preview_display.status_id : "host_gated",
        .material_preview_metal_visible_texture_composites =
            material_preview_metal_selected ? material_preview_display.visible_texture_composites : 0U,
        .vulkan_validation_layer_ready = viewport_vulkan_selected && material_preview_vulkan_selected &&
                                         viewport_display.vulkan_validation_layer_ready &&
                                         material_preview_display.vulkan_validation_layer_ready,
        .vulkan_native_handles_exposed = vulkan_native_handles_exposed,
        .metal_command_queue_ready = viewport_metal_selected && material_preview_metal_selected &&
                                     viewport_display.metal_command_queue_ready &&
                                     material_preview_display.metal_command_queue_ready,
        .metal_metallib_ready = viewport_metal_selected && material_preview_metal_selected &&
                                viewport_display.metal_shader_library_ready &&
                                material_preview_display.metal_shader_library_ready,
        .metal_feature_set_ready = viewport_metal_selected && material_preview_metal_selected &&
                                   viewport_display.metal_feature_set_ready &&
                                   material_preview_display.metal_feature_set_ready,
        .metal_feature_family_ready = viewport_metal_selected && material_preview_metal_selected &&
                                      viewport_display.metal_feature_set_ready &&
                                      material_preview_display.metal_feature_set_ready,
        .metal_render_pipeline_ready = viewport_metal_selected && material_preview_metal_selected &&
                                       viewport_display.metal_render_pipeline_ready &&
                                       material_preview_display.metal_render_pipeline_ready,
        .metal_render_pass_ready = viewport_metal_selected && material_preview_metal_selected &&
                                   viewport_display.metal_render_pass_ready &&
                                   material_preview_display.metal_render_pass_ready,
        .metal_texture_render_target_ready = viewport_metal_selected && material_preview_metal_selected &&
                                             viewport_display.metal_texture_render_target_ready &&
                                             material_preview_display.metal_texture_render_target_ready,
        .metal_shader_read_sampling_ready = viewport_metal_selected && material_preview_metal_selected &&
                                            viewport_display.metal_texture_shader_read_ready &&
                                            material_preview_display.metal_texture_shader_read_ready,
        .metal_sampler_state_ready = viewport_metal_selected && material_preview_metal_selected &&
                                     viewport_display.metal_sampler_state_ready &&
                                     material_preview_display.metal_sampler_state_ready,
        .metal_drawable_present_ready = viewport_metal_selected && material_preview_metal_selected &&
                                        viewport_display.metal_drawable_present_ready &&
                                        material_preview_display.metal_drawable_present_ready,
        .metal_command_buffer_completed = viewport_metal_selected && material_preview_metal_selected &&
                                          viewport_display.metal_command_buffer_completed &&
                                          material_preview_display.metal_command_buffer_completed,
        .metal_native_handles_exposed = metal_native_handles_exposed,
        .text_atlas_handoff_status = text_atlas.status,
        .text_font_adapter_invoked =
            text_atlas.text_shaping_adapter_invoked && text_atlas.font_rasterizer_adapter_invoked,
        .text_font_glyphs_ready = text_atlas.glyphs_ready,
        .text_font_fallback_used = text_atlas.fallback_used,
        .text_atlas_handoff_ready = text_atlas.atlas_handoff_ready,
        .text_font_native_handles_exposed = text_atlas.native_handles_exposed,
        .text_atlas_handoff_host_gated_rows = text_atlas.host_gated_rows,
        .text_atlas_handoff_unsupported_rows = text_atlas.unsupported_rows,
        .text_shaping_status = text_atlas.text_shaping_status,
        .text_font_fallback_status = text_atlas.font_fallback_status,
        .text_glyph_atlas_status = text_atlas.glyph_atlas_status,
        .text_bidi_status = text_atlas.bidi_status,
        .text_line_break_status = text_atlas.line_break_status,
        .text_dependency_license_records = text_atlas.dependency_license_records_status,
        .text_harfbuzz_dependency_status = text_atlas.harf_buzz_dependency_status,
        .text_freetype_dependency_status = text_atlas.free_type_dependency_status,
        .text_icu_dependency_status = text_atlas.icu_dependency_status,
        .text_shaping_segment_rows = text_atlas.shaping_segment_rows,
        .text_glyph_cluster_rows = text_atlas.glyph_cluster_rows,
        .text_glyph_advance_offset_rows = text_atlas.glyph_advance_offset_rows,
        .text_bidi_boundary_rows = text_atlas.bidi_boundary_rows,
        .text_word_boundary_rows = text_atlas.word_boundary_rows,
        .text_line_break_boundary_rows = text_atlas.line_break_boundary_rows,
        .text_font_face_rows = text_atlas.font_face_rows,
        .text_glyph_metric_rows = text_atlas.glyph_metric_rows,
        .text_glyph_bitmap_format_rows = text_atlas.glyph_bitmap_format_rows,
        .text_glyph_atlas_allocation_rows = text_atlas.glyph_atlas_allocation_rows,
        .text_font_license_provenance_rows = text_atlas.font_license_provenance_rows,
        .text_dependency_gated_rows = text_atlas.dependency_gated_rows,
        .text_native_handles_exposed = text_atlas.native_handles_exposed,
        .ime_status = native_editor_text_input_status(text_input),
        .ime_text_input_session_rows = text_input.session_active ? 1U : 0U,
        .ime_composition_rows = text_input.composition_active ? 1U : 0U,
        .ime_committed_text_rows = text_input.commit_applied ? 1U : 0U,
        .ime_caret_rect_rows = text_input.caret_rect_ready ? 1U : 0U,
        .ime_surrounding_text_rows = text_input.surrounding_text_ready ? 1U : 0U,
        .ime_candidate_ui_host_owned = text_input.candidate_ui_host_owned,
        .ime_parity_status = text_input.parity_evidence.status,
        .ime_windows_tsf_status =
            text_input.tsf_adapter_selected && text_input.parity_evidence.ready() && !text_input.native_handles_exposed
                ? "ready"
                : "host_gated",
        .ime_macos_status = "host_gated",
        .ime_linux_ibus_status = "host_gated",
        .ime_linux_fcitx_status = "host_gated",
        .ime_android_status = "host_gated",
        .ime_ios_status = "host_gated",
        .ime_grapheme_boundary_rows = text_input.parity_evidence.grapheme_boundary_rows,
        .ime_grapheme_cursor_rows = text_input.parity_evidence.grapheme_cursor_rows,
        .ime_grapheme_selection_rows = text_input.parity_evidence.grapheme_selection_rows,
        .ime_composition_range_rows = text_input.parity_evidence.composition_range_rows,
        .ime_candidate_selection_rows = text_input.parity_evidence.candidate_selection_rows,
        .ime_reconversion_request_rows = text_input.parity_evidence.reconversion_request_rows,
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
        .accessibility_parity_status = accessibility.parity_status_id,
        .accessibility_windows_uia_patterns_ready = accessibility.windows_uia_patterns_ready,
        .accessibility_windows_uia_events_ready = accessibility.windows_uia_events_ready,
        .accessibility_macos_status = accessibility.macos_status,
        .accessibility_linux_at_spi_status = accessibility.linux_at_spi_status,
        .accessibility_android_status = accessibility.android_status,
        .accessibility_ios_status = accessibility.ios_status,
        .accessibility_live_region_rows = accessibility.live_region_rows,
        .accessibility_uia_pattern_rows = accessibility.uia_pattern_rows,
        .accessibility_uia_event_rows = accessibility.uia_event_rows,
        .accessibility_native_handles_exposed = accessibility.native_handles_exposed,
        .docking_status = document.docking_status,
        .dock_tab_header_count = document.tab_header_count,
        .dock_split_gutter_count = document.split_gutter_count,
        .dock_active_panel_count = document.active_panel_count,
        .dock_focusable_control_count = document.focusable_dock_control_count,
        .multi_window_docking_status = multi_window_ready ? "ready" : "not_ready",
        .dock_window_count = static_cast<std::uint32_t>(multi_window_layout.windows.size()),
        .dock_tear_off_command_count = tear_off_plan.accepted ? 1U : 0U,
        .dock_window_merge_command_count = merge_plan.accepted ? 1U : 0U,
        .workspace_v3_status = workspace_v3_ready ? "ready" : "not_ready",
        .multi_window_native_handles_exposed = multi_window_handles_exposed,
        .rich_text_edit_status =
            rich_text_editable_documents > 0U && !rich_text_native_handles_exposed ? "ready" : "not_ready",
        .rich_text_editable_documents = rich_text_editable_documents,
        .rich_text_command_rows = rich_text_editable_documents * rich_text_commands_per_editable_document,
        .rich_text_clipboard_plain_ready = rich_text_editable_documents > 0U,
        .rich_text_clipboard_rich_ready = rich_text_editable_documents > 0U,
        .rich_text_native_handles_exposed = rich_text_native_handles_exposed,
        .ui_performance_budget_status = std::string(editor_ui_performance_budget_status_id(performance.status)),
        .ui_performance_layout_us_p95 = performance.layout_us_p95,
        .ui_performance_document_build_us_p95 = performance.document_build_us_p95,
        .ui_performance_renderer_submission_us_p95 = performance.renderer_submission_us_p95,
        .ui_performance_text_runs = performance.text_runs_p95,
        .ui_performance_renderer_boxes = performance.renderer_boxes_p95,
        .ui_performance_visible_texture_composites = performance.visible_texture_composites_p95,
        .ui_performance_memory_high_water_bytes = performance.memory_high_water_bytes,
        .ui_performance_budget_violations = performance.budget_violations,
        .ui_performance_diagnostics = static_cast<std::uint32_t>(performance.diagnostics.size()),
        .ui_performance_broad_optimization_claimed = performance.broad_optimization_claimed,
        .ui_retained_diff_status = std::string(ui::retained_ui_diff_status_id(retained_diff.status)),
        .ui_retained_dirty_rows = count_to_u64(retained_diff.dirty_rows),
        .ui_retained_layout_cache_hits = count_to_u64(retained_diff.layout_cache_hits),
        .ui_retained_layout_cache_misses = count_to_u64(retained_diff.layout_cache_misses),
        .ui_retained_text_cache_hits = count_to_u64(retained_diff.text_cache_hits),
        .ui_retained_text_cache_misses = count_to_u64(retained_diff.text_cache_misses),
        .ui_retained_submission_reused_rows = count_to_u64(retained_diff.submission_reused_rows),
        .ui_retained_submission_rebuilt_rows = count_to_u64(retained_diff.submission_rebuilt_rows),
        .ui_retained_cache_native_handle_access = retained_diff.native_handle_access,
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
        .ime_parity_status = counters.ime_parity_status,
        .ime_windows_tsf_status = counters.ime_windows_tsf_status,
        .ime_macos_status = counters.ime_macos_status,
        .ime_linux_ibus_status = counters.ime_linux_ibus_status,
        .ime_linux_fcitx_status = counters.ime_linux_fcitx_status,
        .ime_android_status = counters.ime_android_status,
        .ime_ios_status = counters.ime_ios_status,
        .ime_grapheme_boundary_rows = counters.ime_grapheme_boundary_rows,
        .ime_grapheme_cursor_rows = counters.ime_grapheme_cursor_rows,
        .ime_grapheme_selection_rows = counters.ime_grapheme_selection_rows,
        .ime_composition_range_rows = counters.ime_composition_range_rows,
        .ime_candidate_selection_rows = counters.ime_candidate_selection_rows,
        .ime_reconversion_request_rows = counters.ime_reconversion_request_rows,
        .ime_native_handles_exposed = counters.ime_native_handles_exposed,
        .text_atlas_handoff_status = counters.text_atlas_handoff_status,
        .text_font_adapter_invoked = counters.text_font_adapter_invoked,
        .text_font_glyphs_ready = counters.text_font_glyphs_ready,
        .text_font_fallback_used = counters.text_font_fallback_used,
        .text_atlas_handoff_ready = counters.text_atlas_handoff_ready,
        .text_font_native_handles_exposed = counters.text_font_native_handles_exposed,
        .text_atlas_handoff_host_gated_rows = counters.text_atlas_handoff_host_gated_rows,
        .text_atlas_handoff_unsupported_rows = counters.text_atlas_handoff_unsupported_rows,
        .text_shaping_status = counters.text_shaping_status,
        .text_font_fallback_status = counters.text_font_fallback_status,
        .text_glyph_atlas_status = counters.text_glyph_atlas_status,
        .text_bidi_status = counters.text_bidi_status,
        .text_line_break_status = counters.text_line_break_status,
        .text_dependency_license_records = counters.text_dependency_license_records,
        .text_harfbuzz_dependency_status = counters.text_harfbuzz_dependency_status,
        .text_freetype_dependency_status = counters.text_freetype_dependency_status,
        .text_icu_dependency_status = counters.text_icu_dependency_status,
        .text_shaping_segment_rows = counters.text_shaping_segment_rows,
        .text_glyph_cluster_rows = counters.text_glyph_cluster_rows,
        .text_glyph_advance_offset_rows = counters.text_glyph_advance_offset_rows,
        .text_bidi_boundary_rows = counters.text_bidi_boundary_rows,
        .text_word_boundary_rows = counters.text_word_boundary_rows,
        .text_line_break_boundary_rows = counters.text_line_break_boundary_rows,
        .text_font_face_rows = counters.text_font_face_rows,
        .text_glyph_metric_rows = counters.text_glyph_metric_rows,
        .text_glyph_bitmap_format_rows = counters.text_glyph_bitmap_format_rows,
        .text_glyph_atlas_allocation_rows = counters.text_glyph_atlas_allocation_rows,
        .text_font_license_provenance_rows = counters.text_font_license_provenance_rows,
        .text_dependency_gated_rows = counters.text_dependency_gated_rows,
        .text_native_handles_exposed = counters.text_native_handles_exposed,
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
        .accessibility_parity_status = counters.accessibility_parity_status,
        .accessibility_windows_uia_patterns_ready = counters.accessibility_windows_uia_patterns_ready,
        .accessibility_windows_uia_events_ready = counters.accessibility_windows_uia_events_ready,
        .accessibility_macos_status = counters.accessibility_macos_status,
        .accessibility_linux_at_spi_status = counters.accessibility_linux_at_spi_status,
        .accessibility_android_status = counters.accessibility_android_status,
        .accessibility_ios_status = counters.accessibility_ios_status,
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
