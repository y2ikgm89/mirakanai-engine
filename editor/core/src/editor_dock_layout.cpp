// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/editor_dock_layout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

struct UnsafeNeedle {
    std::string_view token;
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

[[nodiscard]] bool contains_string(const std::vector<std::string>& values, std::string_view value) noexcept {
    return std::ranges::any_of(values, [value](const std::string& candidate) { return candidate == value; });
}

[[nodiscard]] bool contains_unsafe_token(std::string_view value) noexcept {
    return std::ranges::any_of(unsafe_needles, [value](const UnsafeNeedle& needle) {
        return value.find(needle.token) != std::string_view::npos;
    });
}

void append_diagnostic(EditorDockLayoutValidation& validation, std::string code, std::string message) {
    validation.diagnostics.push_back(
        EditorDockLayoutDiagnostic{.code = std::move(code), .message = std::move(message)});
}

void append_diagnostic(EditorDockCommandPlan& plan, std::string code, std::string message) {
    plan.diagnostics.push_back(EditorDockCommandDiagnostic{.code = std::move(code), .message = std::move(message)});
}

void append_diagnostic(EditorDockWindowCommandPlan& plan, std::string code, std::string message) {
    plan.diagnostics.push_back(EditorDockCommandDiagnostic{.code = std::move(code), .message = std::move(message)});
}

void validate_id_token(EditorDockLayoutValidation& validation, std::string_view id, std::string_view label) {
    if (id.empty()) {
        append_diagnostic(validation, "missing_id", std::string{label} + " id must not be empty");
        return;
    }
    if (id.find_first_of("\r\n=,") != std::string_view::npos || contains_unsafe_token(id)) {
        append_diagnostic(validation, "unsafe_token",
                          std::string{label} + " id contains an unsafe token: " + std::string{id});
    }
}

void append_diagnostic(EditorDockCommandPlan& plan, const EditorDockLayoutDiagnostic& diagnostic) {
    append_diagnostic(plan, diagnostic.code, diagnostic.message);
}

void append_diagnostic(EditorDockWindowCommandPlan& plan, const EditorDockLayoutDiagnostic& diagnostic) {
    append_diagnostic(plan, diagnostic.code, diagnostic.message);
}

[[nodiscard]] EditorDockNode* find_tab_stack_containing_panel(EditorDockLayout& layout,
                                                              std::string_view panel_id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [panel_id](const EditorDockNode& node) {
        return node.kind == EditorDockNodeKind::tab_stack && contains_string(node.tabs, panel_id);
    });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

[[nodiscard]] const EditorDockNode* find_tab_stack_containing_panel(const EditorDockLayout& layout,
                                                                    std::string_view panel_id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [panel_id](const EditorDockNode& node) {
        return node.kind == EditorDockNodeKind::tab_stack && contains_string(node.tabs, panel_id);
    });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

[[nodiscard]] EditorDockNode* find_tab_stack_containing_panel(EditorDockMultiWindowLayout& layout,
                                                              std::string_view panel_id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [panel_id](const EditorDockNode& node) {
        return node.kind == EditorDockNodeKind::tab_stack && contains_string(node.tabs, panel_id);
    });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

[[nodiscard]] const EditorDockNode* find_tab_stack_containing_panel(const EditorDockMultiWindowLayout& layout,
                                                                    std::string_view panel_id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [panel_id](const EditorDockNode& node) {
        return node.kind == EditorDockNodeKind::tab_stack && contains_string(node.tabs, panel_id);
    });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

[[nodiscard]] EditorDockNode* find_parent_split_containing_child(EditorDockLayout& layout,
                                                                 std::string_view child_id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [child_id](const EditorDockNode& node) {
        return node.kind == EditorDockNodeKind::split && contains_string(node.children, child_id);
    });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

[[nodiscard]] EditorDockNode* find_parent_split_containing_child(EditorDockMultiWindowLayout& layout,
                                                                 std::string_view child_id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [child_id](const EditorDockNode& node) {
        return node.kind == EditorDockNodeKind::split && contains_string(node.children, child_id);
    });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

[[nodiscard]] bool window_bounds_are_valid(const EditorDockWindowBounds& bounds) noexcept {
    return std::isfinite(bounds.x) && std::isfinite(bounds.y) && std::isfinite(bounds.width) &&
           std::isfinite(bounds.height) && bounds.width > 0.0F && bounds.height > 0.0F;
}

[[nodiscard]] bool window_dpi_is_valid(float dpi_scale) noexcept {
    return std::isfinite(dpi_scale) && dpi_scale > 0.0F;
}

[[nodiscard]] std::size_t find_node_index(const EditorDockLayout& layout, std::string_view id) noexcept {
    for (std::size_t index = 0; index < layout.nodes.size(); ++index) {
        if (layout.nodes[index].id == id) {
            return index;
        }
    }
    return layout.nodes.size();
}

[[nodiscard]] std::size_t find_node_index(const EditorDockMultiWindowLayout& layout, std::string_view id) noexcept {
    for (std::size_t index = 0; index < layout.nodes.size(); ++index) {
        if (layout.nodes[index].id == id) {
            return index;
        }
    }
    return layout.nodes.size();
}

enum class DockGraphVisitState : std::uint8_t {
    unvisited,
    visiting,
    visited,
};

void validate_reachable_dock_node(EditorDockLayoutValidation& validation, const EditorDockLayout& layout,
                                  std::string_view id, std::vector<DockGraphVisitState>& visit_states) {
    const auto index = find_node_index(layout, id);
    if (index >= layout.nodes.size()) {
        return;
    }
    if (visit_states[index] == DockGraphVisitState::visiting) {
        append_diagnostic(validation, "dock_graph_cycle", "dock graph contains a cycle at node: " + std::string{id});
        return;
    }
    if (visit_states[index] == DockGraphVisitState::visited) {
        return;
    }

    visit_states[index] = DockGraphVisitState::visiting;
    const auto& node = layout.nodes[index];
    if (node.kind == EditorDockNodeKind::split) {
        for (const auto& child : node.children) {
            validate_reachable_dock_node(validation, layout, child, visit_states);
        }
    }
    visit_states[index] = DockGraphVisitState::visited;
}

void validate_reachable_dock_node(EditorDockLayoutValidation& validation, const EditorDockMultiWindowLayout& layout,
                                  std::string_view id, std::vector<DockGraphVisitState>& visit_states) {
    const auto index = find_node_index(layout, id);
    if (index >= layout.nodes.size()) {
        return;
    }
    if (visit_states[index] == DockGraphVisitState::visiting) {
        append_diagnostic(validation, "dock_graph_cycle", "dock graph contains a cycle at node: " + std::string{id});
        return;
    }
    if (visit_states[index] == DockGraphVisitState::visited) {
        return;
    }

    visit_states[index] = DockGraphVisitState::visiting;
    const auto& node = layout.nodes[index];
    if (node.kind == EditorDockNodeKind::split) {
        for (const auto& child : node.children) {
            validate_reachable_dock_node(validation, layout, child, visit_states);
        }
    }
    visit_states[index] = DockGraphVisitState::visited;
}

[[nodiscard]] bool validate_new_dock_node_id(EditorDockCommandPlan& plan, const EditorDockLayout& layout,
                                             std::string_view id, std::string_view code, std::string_view label) {
    EditorDockLayoutValidation id_validation{.valid = true};
    validate_id_token(id_validation, id, label);
    if (!id_validation.diagnostics.empty()) {
        append_diagnostic(plan, std::string{code}, std::string{label} + " is not a safe dock node id");
        return false;
    }
    if (find_editor_dock_node(layout, id) != nullptr) {
        append_diagnostic(plan, std::string{code}, std::string{label} + " already exists: " + std::string{id});
        return false;
    }
    return true;
}

[[nodiscard]] bool validate_new_dock_node_id(EditorDockWindowCommandPlan& plan,
                                             const EditorDockMultiWindowLayout& layout, std::string_view id,
                                             std::string_view code, std::string_view label) {
    EditorDockLayoutValidation id_validation{.valid = true};
    validate_id_token(id_validation, id, label);
    if (!id_validation.diagnostics.empty()) {
        append_diagnostic(plan, std::string{code}, std::string{label} + " is not a safe dock node id");
        return false;
    }
    if (find_editor_dock_node(layout, id) != nullptr) {
        append_diagnostic(plan, std::string{code}, std::string{label} + " already exists: " + std::string{id});
        return false;
    }
    return true;
}

[[nodiscard]] const EditorDockPanelCatalogRow*
validate_command_panel(EditorDockCommandPlan& plan, const std::vector<EditorDockPanelCatalogRow>& catalog,
                       std::string_view panel_id) {
    if (panel_id.empty()) {
        append_diagnostic(plan, "missing_panel", "dock command panel id is required");
        return nullptr;
    }
    const auto* panel = find_editor_dock_panel(catalog, panel_id);
    if (panel == nullptr) {
        append_diagnostic(plan, "unknown_panel",
                          "dock command panel is not in the panel catalog: " + std::string{panel_id});
        return nullptr;
    }
    if (!panel->native_shell_panel) {
        append_diagnostic(plan, "panel_not_native_shell",
                          "dock command panel is outside the native shell: " + std::string{panel_id});
        return nullptr;
    }
    return panel;
}

[[nodiscard]] const EditorDockPanelCatalogRow*
validate_command_panel(EditorDockWindowCommandPlan& plan, const std::vector<EditorDockPanelCatalogRow>& catalog,
                       std::string_view panel_id) {
    if (panel_id.empty()) {
        append_diagnostic(plan, "missing_panel", "dock window command panel id is required");
        return nullptr;
    }
    const auto* panel = find_editor_dock_panel(catalog, panel_id);
    if (panel == nullptr) {
        append_diagnostic(plan, "unknown_panel",
                          "dock window command panel is not in the panel catalog: " + std::string{panel_id});
        return nullptr;
    }
    if (!panel->native_shell_panel) {
        append_diagnostic(plan, "panel_not_native_shell",
                          "dock window command panel is outside the native shell: " + std::string{panel_id});
        return nullptr;
    }
    return panel;
}

void accept_dock_noop(EditorDockCommandPlan& plan) noexcept {
    plan.accepted = true;
    plan.after_revision = plan.before_revision;
}

void accept_dock_mutation(EditorDockCommandPlan& plan) {
    plan.result_layout.layout_revision = plan.before_revision + 1U;
    const auto result_validation = validate_editor_dock_layout(plan.result_layout);
    if (!result_validation.valid) {
        append_diagnostic(plan, "invalid_result_layout", "dock command result layout is invalid");
        return;
    }
    plan.accepted = true;
    plan.would_mutate = true;
    plan.after_revision = plan.result_layout.layout_revision;
}

void accept_dock_window_noop(EditorDockWindowCommandPlan& plan) noexcept {
    plan.accepted = true;
    plan.after_revision = plan.before_revision;
}

void accept_dock_window_mutation(EditorDockWindowCommandPlan& plan) {
    plan.result_layout.layout_revision = plan.before_revision + 1U;
    const auto result_validation = validate_editor_dock_multi_window_layout(plan.result_layout);
    if (!result_validation.valid) {
        append_diagnostic(plan, "invalid_result_layout", "dock window command result layout is invalid");
        return;
    }
    plan.accepted = true;
    plan.would_mutate = true;
    plan.after_revision = plan.result_layout.layout_revision;
}

[[nodiscard]] std::string make_window_stack_id(std::string_view window_id) {
    return "dock." + std::string{window_id} + ".stack";
}

void erase_dock_node_subtree(EditorDockMultiWindowLayout& layout, std::string_view root_id) {
    const auto* node = find_editor_dock_node(layout, root_id);
    if (node == nullptr) {
        return;
    }
    std::vector<std::string> children = node->children;
    for (const auto& child : children) {
        erase_dock_node_subtree(layout, child);
    }
    std::erase_if(layout.nodes, [root_id](const EditorDockNode& candidate) { return candidate.id == root_id; });
}

void collect_tabs_from_subtree(const EditorDockMultiWindowLayout& layout, std::string_view root_id,
                               std::vector<std::string>& tabs) {
    const auto* node = find_editor_dock_node(layout, root_id);
    if (node == nullptr) {
        return;
    }
    if (node->kind == EditorDockNodeKind::tab_stack) {
        tabs.insert(tabs.end(), node->tabs.begin(), node->tabs.end());
        return;
    }
    for (const auto& child : node->children) {
        collect_tabs_from_subtree(layout, child, tabs);
    }
}

[[nodiscard]] bool subtree_contains_panel(const EditorDockMultiWindowLayout& layout, std::string_view root_id,
                                          std::string_view panel_id) {
    std::vector<std::string> tabs;
    collect_tabs_from_subtree(layout, root_id, tabs);
    return contains_string(tabs, panel_id);
}

void remove_window_by_id(EditorDockMultiWindowLayout& layout, std::string_view window_id) {
    const auto* window = find_editor_dock_window(layout, window_id);
    if (window == nullptr) {
        return;
    }
    const std::string root_id = window->root_id;
    std::erase_if(layout.windows, [window_id](const EditorDockWindowRow& row) { return row.id == window_id; });
    erase_dock_node_subtree(layout, root_id);
    if (!layout.windows.empty() && layout.focused_window_id == window_id) {
        layout.focused_window_id = layout.windows.front().id;
    }
}

void remove_panel_from_stack(EditorDockNode& stack, std::string_view panel_id) {
    std::erase(stack.tabs, std::string{panel_id});
    if (stack.active_tab_id == panel_id) {
        stack.active_tab_id = stack.tabs.empty() ? std::string{} : stack.tabs.front();
    }
}

} // namespace

std::vector<EditorDockPanelCatalogRow> editor_dock_panel_catalog() {
    return {
        EditorDockPanelCatalogRow{.id = "main_menu",
                                  .label = "Main Menu",
                                  .workspace_id = PanelId::scene,
                                  .shell_chrome = true,
                                  .workspace_panel = false,
                                  .native_shell_panel = true},
        EditorDockPanelCatalogRow{.id = "scene",
                                  .label = "Scene",
                                  .workspace_id = PanelId::scene,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = true},
        EditorDockPanelCatalogRow{.id = "inspector",
                                  .label = "Inspector",
                                  .workspace_id = PanelId::inspector,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = true},
        EditorDockPanelCatalogRow{.id = "assets",
                                  .label = "Assets",
                                  .workspace_id = PanelId::assets,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = true},
        EditorDockPanelCatalogRow{.id = "console",
                                  .label = "Console",
                                  .workspace_id = PanelId::console,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = true},
        EditorDockPanelCatalogRow{.id = "viewport",
                                  .label = "Viewport",
                                  .workspace_id = PanelId::viewport,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = true},
        EditorDockPanelCatalogRow{.id = "resources",
                                  .label = "Resources",
                                  .workspace_id = PanelId::resources,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = true},
        EditorDockPanelCatalogRow{.id = "ai_commands",
                                  .label = "AI Commands",
                                  .workspace_id = PanelId::ai_commands,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = true},
        EditorDockPanelCatalogRow{.id = "input_rebinding",
                                  .label = "Input Rebinding",
                                  .workspace_id = PanelId::input_rebinding,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = false},
        EditorDockPanelCatalogRow{.id = "profiler",
                                  .label = "Profiler",
                                  .workspace_id = PanelId::profiler,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = true},
        EditorDockPanelCatalogRow{.id = "project_settings",
                                  .label = "Project Settings",
                                  .workspace_id = PanelId::project_settings,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = true},
        EditorDockPanelCatalogRow{.id = "timeline",
                                  .label = "Timeline",
                                  .workspace_id = PanelId::timeline,
                                  .shell_chrome = false,
                                  .workspace_panel = true,
                                  .native_shell_panel = true},
    };
}

const EditorDockPanelCatalogRow* find_editor_dock_panel(const std::vector<EditorDockPanelCatalogRow>& catalog,
                                                        std::string_view id) noexcept {
    const auto it = std::ranges::find_if(catalog, [id](const EditorDockPanelCatalogRow& row) { return row.id == id; });
    return it == catalog.end() ? nullptr : &(*it);
}

std::vector<EditorDockUnsupportedCapability> make_editor_ui_low_level_unsupported_capabilities() {
    return {
        EditorDockUnsupportedCapability{.id = "text_shaping",
                                        .official_boundary = "DirectWrite/TextServer-class adapter",
                                        .implemented = false,
                                        .native_handles_public = false},
        EditorDockUnsupportedCapability{.id = "font_rasterization",
                                        .official_boundary = "DirectWrite/Direct2D or audited font adapter",
                                        .implemented = false,
                                        .native_handles_public = false},
        EditorDockUnsupportedCapability{.id = "ime_text_services",
                                        .official_boundary = "Text Services Framework adapter",
                                        .implemented = false,
                                        .native_handles_public = false},
        EditorDockUnsupportedCapability{.id = "accessibility_publication",
                                        .official_boundary = "UI Automation provider adapter",
                                        .implemented = false,
                                        .native_handles_public = false},
    };
}

EditorDockLayout make_default_editor_dock_layout() {
    EditorDockLayout layout;
    layout.root_id = "dock.root";
    layout.focused_panel_id = "viewport";
    layout.layout_revision = 1U;
    layout.persist_to_workspace = true;
    layout.unsupported_capabilities = make_editor_ui_low_level_unsupported_capabilities();
    layout.nodes = {
        EditorDockNode{
            .id = "dock.root",
            .kind = EditorDockNodeKind::split,
            .axis = EditorDockSplitAxis::horizontal,
            .split_ratio = 0.2F,
            .children = {"dock.left_stack", "dock.center_split", "dock.right_stack"},
            .tabs = {},
            .active_tab_id = {},
        },
        EditorDockNode{
            .id = "dock.left_stack",
            .kind = EditorDockNodeKind::tab_stack,
            .axis = EditorDockSplitAxis::horizontal,
            .split_ratio = 0.5F,
            .children = {},
            .tabs = {"main_menu", "scene", "assets", "resources"},
            .active_tab_id = "scene",
        },
        EditorDockNode{
            .id = "dock.center_split",
            .kind = EditorDockNodeKind::split,
            .axis = EditorDockSplitAxis::vertical,
            .split_ratio = 0.72F,
            .children = {"dock.viewport_stack", "dock.bottom_stack"},
            .tabs = {},
            .active_tab_id = {},
        },
        EditorDockNode{
            .id = "dock.viewport_stack",
            .kind = EditorDockNodeKind::tab_stack,
            .axis = EditorDockSplitAxis::horizontal,
            .split_ratio = 0.5F,
            .children = {},
            .tabs = {"viewport"},
            .active_tab_id = "viewport",
        },
        EditorDockNode{
            .id = "dock.bottom_stack",
            .kind = EditorDockNodeKind::tab_stack,
            .axis = EditorDockSplitAxis::horizontal,
            .split_ratio = 0.5F,
            .children = {},
            .tabs = {"console", "profiler", "timeline"},
            .active_tab_id = "console",
        },
        EditorDockNode{
            .id = "dock.right_stack",
            .kind = EditorDockNodeKind::tab_stack,
            .axis = EditorDockSplitAxis::horizontal,
            .split_ratio = 0.5F,
            .children = {},
            .tabs = {"inspector", "ai_commands", "project_settings"},
            .active_tab_id = "inspector",
        },
    };
    return layout;
}

EditorDockMultiWindowLayout make_default_editor_dock_multi_window_layout() {
    const auto single_window = make_default_editor_dock_layout();
    EditorDockMultiWindowLayout layout;
    layout.nodes = single_window.nodes;
    layout.windows = {
        EditorDockWindowRow{
            .id = "window.main",
            .title = "MIRAIKANAI Editor",
            .monitor_id = "monitor.primary",
            .bounds = EditorDockWindowBounds{.x = 0.0F, .y = 0.0F, .width = 1280.0F, .height = 720.0F},
            .dpi_scale = 1.0F,
            .root_id = single_window.root_id,
            .focused_panel_id = single_window.focused_panel_id,
        },
    };
    layout.focused_window_id = "window.main";
    layout.layout_revision = single_window.layout_revision;
    layout.native_handles_public = false;
    layout.unsupported_capabilities = single_window.unsupported_capabilities;
    return layout;
}

const EditorDockNode* find_editor_dock_node(const EditorDockLayout& layout, std::string_view id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [id](const EditorDockNode& node) { return node.id == id; });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

EditorDockNode* find_editor_dock_node(EditorDockLayout& layout, std::string_view id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [id](const EditorDockNode& node) { return node.id == id; });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

const EditorDockNode* find_editor_dock_node(const EditorDockMultiWindowLayout& layout, std::string_view id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [id](const EditorDockNode& node) { return node.id == id; });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

EditorDockNode* find_editor_dock_node(EditorDockMultiWindowLayout& layout, std::string_view id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [id](const EditorDockNode& node) { return node.id == id; });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

const EditorDockWindowRow* find_editor_dock_window(const EditorDockMultiWindowLayout& layout,
                                                   std::string_view id) noexcept {
    const auto it = std::ranges::find_if(layout.windows, [id](const EditorDockWindowRow& row) { return row.id == id; });
    return it == layout.windows.end() ? nullptr : &(*it);
}

EditorDockWindowRow* find_editor_dock_window(EditorDockMultiWindowLayout& layout, std::string_view id) noexcept {
    const auto it = std::ranges::find_if(layout.windows, [id](const EditorDockWindowRow& row) { return row.id == id; });
    return it == layout.windows.end() ? nullptr : &(*it);
}

EditorDockLayoutValidation validate_editor_dock_layout(const EditorDockLayout& layout) {
    EditorDockLayoutValidation validation{.valid = true};
    const auto catalog = editor_dock_panel_catalog();

    validate_id_token(validation, layout.root_id, "dock root");
    validate_id_token(validation, layout.focused_panel_id, "focused panel");
    if (layout.nodes.empty()) {
        append_diagnostic(validation, "empty_dock_layout", "dock layout must have at least one node");
    }
    if (!layout.root_id.empty() && find_editor_dock_node(layout, layout.root_id) == nullptr) {
        append_diagnostic(validation, "missing_root_node", "dock layout root node is missing: " + layout.root_id);
    }
    if (!layout.focused_panel_id.empty() && find_editor_dock_panel(catalog, layout.focused_panel_id) == nullptr) {
        append_diagnostic(validation, "unknown_focused_panel",
                          "dock layout focused panel is not in the panel catalog: " + layout.focused_panel_id);
    } else if (!layout.focused_panel_id.empty()) {
        const auto* focused_panel = find_editor_dock_panel(catalog, layout.focused_panel_id);
        if (focused_panel != nullptr && !focused_panel->native_shell_panel) {
            append_diagnostic(validation, "focused_panel_not_native_shell",
                              "dock layout focused panel is outside the native shell: " + layout.focused_panel_id);
        }
    }

    for (std::size_t index = 0; index < layout.nodes.size(); ++index) {
        const auto& node = layout.nodes[index];
        validate_id_token(validation, node.id, "dock node");
        for (std::size_t other = index + 1U; other < layout.nodes.size(); ++other) {
            if (node.id == layout.nodes[other].id) {
                append_diagnostic(validation, "duplicate_dock_node_id", "duplicate dock node id: " + node.id);
            }
        }
    }

    std::vector<std::string> seen_tabs;
    for (const auto& node : layout.nodes) {
        if (node.id.empty()) {
            continue;
        }
        if (node.kind == EditorDockNodeKind::split) {
            if (node.children.size() < 2U) {
                append_diagnostic(validation, "invalid_split_children",
                                  "split dock node requires at least two children: " + node.id);
            }
            if (node.split_ratio <= 0.0F || node.split_ratio >= 1.0F) {
                append_diagnostic(validation, "invalid_split_ratio",
                                  "split dock node ratio must be between zero and one: " + node.id);
            }
            for (const auto& child : node.children) {
                validate_id_token(validation, child, "dock child");
                if (std::ranges::count(node.children, child) > 1) {
                    append_diagnostic(validation, "duplicate_split_child",
                                      "split dock node references a child more than once: " + child);
                }
                if (find_editor_dock_node(layout, child) == nullptr) {
                    append_diagnostic(validation, "missing_child_node",
                                      "split dock node references missing child: " + child);
                }
            }
        } else {
            if (node.tabs.empty()) {
                append_diagnostic(validation, "missing_tabs", "tab stack requires at least one tab: " + node.id);
            }
            if (node.active_tab_id.empty() || !contains_string(node.tabs, node.active_tab_id)) {
                append_diagnostic(validation, "missing_active_tab",
                                  "tab stack active tab is missing from tabs: " + node.id);
            }
            for (const auto& tab : node.tabs) {
                validate_id_token(validation, tab, "dock tab");
                if (contains_string(seen_tabs, tab)) {
                    append_diagnostic(validation, "duplicate_dock_tab",
                                      "dock tab appears in more than one stack: " + tab);
                } else {
                    seen_tabs.push_back(tab);
                }
                const auto* panel = find_editor_dock_panel(catalog, tab);
                if (panel == nullptr) {
                    append_diagnostic(validation, "unknown_panel", "dock tab references unknown panel: " + tab);
                } else if (!panel->native_shell_panel) {
                    append_diagnostic(validation, "panel_not_native_shell",
                                      "dock tab references a panel outside the native shell: " + tab);
                }
            }
        }
    }

    if (!layout.nodes.empty() && find_editor_dock_node(layout, layout.root_id) != nullptr) {
        std::vector<DockGraphVisitState> visit_states(layout.nodes.size(), DockGraphVisitState::unvisited);
        validate_reachable_dock_node(validation, layout, layout.root_id, visit_states);
        for (std::size_t index = 0; index < visit_states.size(); ++index) {
            if (visit_states[index] == DockGraphVisitState::unvisited) {
                append_diagnostic(validation, "unreachable_dock_node",
                                  "dock node is not reachable from root: " + layout.nodes[index].id);
            }
        }
    }

    for (const auto& capability : layout.unsupported_capabilities) {
        validate_id_token(validation, capability.id, "unsupported capability");
        if (capability.implemented || capability.native_handles_public) {
            append_diagnostic(validation, "unsupported_capability_claim",
                              "low-level UI capability must remain unsupported in dock layout foundation: " +
                                  capability.id);
        }
    }

    validation.valid = validation.diagnostics.empty();
    return validation;
}

EditorDockLayoutValidation validate_editor_dock_multi_window_layout(const EditorDockMultiWindowLayout& layout) {
    EditorDockLayoutValidation validation{.valid = true};
    const auto catalog = editor_dock_panel_catalog();

    validate_id_token(validation, layout.focused_window_id, "focused dock window");
    if (layout.native_handles_public) {
        append_diagnostic(validation, "native_handles_public",
                          "multi-window dock layout must not expose native platform handles");
    }
    if (layout.windows.empty()) {
        append_diagnostic(validation, "empty_window_layout", "multi-window dock layout must have at least one window");
    }
    if (!layout.focused_window_id.empty() && find_editor_dock_window(layout, layout.focused_window_id) == nullptr) {
        append_diagnostic(validation, "missing_focused_window",
                          "multi-window dock focused window is missing: " + layout.focused_window_id);
    }
    if (layout.nodes.empty()) {
        append_diagnostic(validation, "empty_dock_layout", "multi-window dock layout must have at least one node");
    }

    for (std::size_t index = 0; index < layout.windows.size(); ++index) {
        const auto& window = layout.windows[index];
        validate_id_token(validation, window.id, "dock window");
        validate_id_token(validation, window.monitor_id, "dock monitor");
        validate_id_token(validation, window.root_id, "dock window root");
        validate_id_token(validation, window.focused_panel_id, "dock window focus");
        if (window.title.find_first_of("\r\n=") != std::string::npos || contains_unsafe_token(window.title)) {
            append_diagnostic(validation, "unsafe_token",
                              "dock window title contains an unsafe token: " + window.title);
        }
        if (!window_dpi_is_valid(window.dpi_scale)) {
            append_diagnostic(validation, "invalid_window_dpi",
                              "dock window dpi scale must be a positive finite value: " + window.id);
        }
        if (!window_bounds_are_valid(window.bounds)) {
            append_diagnostic(validation, "invalid_window_bounds",
                              "dock window bounds must have finite position and positive size: " + window.id);
        }
        if (find_editor_dock_node(layout, window.root_id) == nullptr) {
            append_diagnostic(validation, "missing_window_root", "dock window root node is missing: " + window.root_id);
        }
        if (!window.focused_panel_id.empty()) {
            const auto* focused_panel = find_editor_dock_panel(catalog, window.focused_panel_id);
            if (focused_panel == nullptr) {
                append_diagnostic(validation, "unknown_focused_panel",
                                  "dock window focused panel is not in the panel catalog: " + window.focused_panel_id);
            } else if (!focused_panel->native_shell_panel) {
                append_diagnostic(validation, "focused_panel_not_native_shell",
                                  "dock window focused panel is outside the native shell: " + window.focused_panel_id);
            }
        }
        for (std::size_t other = index + 1U; other < layout.windows.size(); ++other) {
            if (window.id == layout.windows[other].id) {
                append_diagnostic(validation, "duplicate_dock_window_id", "duplicate dock window id: " + window.id);
            }
            if (window.root_id == layout.windows[other].root_id) {
                append_diagnostic(validation, "duplicate_window_root",
                                  "dock windows must not share a root node: " + window.root_id);
            }
        }
    }

    for (std::size_t index = 0; index < layout.nodes.size(); ++index) {
        const auto& node = layout.nodes[index];
        validate_id_token(validation, node.id, "dock node");
        for (std::size_t other = index + 1U; other < layout.nodes.size(); ++other) {
            if (node.id == layout.nodes[other].id) {
                append_diagnostic(validation, "duplicate_dock_node_id", "duplicate dock node id: " + node.id);
            }
        }
    }

    std::vector<std::string> seen_tabs;
    for (const auto& node : layout.nodes) {
        if (node.id.empty()) {
            continue;
        }
        if (node.kind == EditorDockNodeKind::split) {
            if (node.children.size() < 2U) {
                append_diagnostic(validation, "invalid_split_children",
                                  "split dock node requires at least two children: " + node.id);
            }
            if (node.split_ratio <= 0.0F || node.split_ratio >= 1.0F) {
                append_diagnostic(validation, "invalid_split_ratio",
                                  "split dock node ratio must be between zero and one: " + node.id);
            }
            for (const auto& child : node.children) {
                validate_id_token(validation, child, "dock child");
                if (std::ranges::count(node.children, child) > 1) {
                    append_diagnostic(validation, "duplicate_split_child",
                                      "split dock node references a child more than once: " + child);
                }
                if (find_editor_dock_node(layout, child) == nullptr) {
                    append_diagnostic(validation, "missing_child_node",
                                      "split dock node references missing child: " + child);
                }
            }
        } else {
            if (node.tabs.empty()) {
                append_diagnostic(validation, "missing_tabs", "tab stack requires at least one tab: " + node.id);
            }
            if (node.active_tab_id.empty() || !contains_string(node.tabs, node.active_tab_id)) {
                append_diagnostic(validation, "missing_active_tab",
                                  "tab stack active tab is missing from tabs: " + node.id);
            }
            for (const auto& tab : node.tabs) {
                validate_id_token(validation, tab, "dock tab");
                if (contains_string(seen_tabs, tab)) {
                    append_diagnostic(validation, "duplicate_dock_tab",
                                      "dock tab appears in more than one stack: " + tab);
                } else {
                    seen_tabs.push_back(tab);
                }
                const auto* panel = find_editor_dock_panel(catalog, tab);
                if (panel == nullptr) {
                    append_diagnostic(validation, "unknown_panel", "dock tab references unknown panel: " + tab);
                } else if (!panel->native_shell_panel) {
                    append_diagnostic(validation, "panel_not_native_shell",
                                      "dock tab references a panel outside the native shell: " + tab);
                }
            }
        }
    }

    if (!layout.nodes.empty()) {
        std::vector<DockGraphVisitState> visit_states(layout.nodes.size(), DockGraphVisitState::unvisited);
        for (const auto& window : layout.windows) {
            if (find_editor_dock_node(layout, window.root_id) != nullptr) {
                validate_reachable_dock_node(validation, layout, window.root_id, visit_states);
                if (!window.focused_panel_id.empty() &&
                    !subtree_contains_panel(layout, window.root_id, window.focused_panel_id)) {
                    append_diagnostic(validation, "window_focus_not_docked",
                                      "dock window focused panel is not reachable from its root: " +
                                          window.focused_panel_id);
                }
            }
        }
        for (std::size_t index = 0; index < visit_states.size(); ++index) {
            if (visit_states[index] == DockGraphVisitState::unvisited) {
                append_diagnostic(validation, "unreachable_dock_node",
                                  "dock node is not reachable from a window root: " + layout.nodes[index].id);
            }
        }
    }

    for (const auto& capability : layout.unsupported_capabilities) {
        validate_id_token(validation, capability.id, "unsupported capability");
        if (capability.implemented || capability.native_handles_public) {
            append_diagnostic(validation, "unsupported_capability_claim",
                              "low-level UI capability must remain unsupported in multi-window dock foundation: " +
                                  capability.id);
        }
    }

    validation.valid = validation.diagnostics.empty();
    return validation;
}

EditorDockCommandPlan plan_editor_dock_command(const EditorDockLayout& layout,
                                               const EditorDockCommandRequest& request) {
    EditorDockCommandPlan plan{
        .accepted = false,
        .would_mutate = false,
        .requires_confirmation = false,
        .before_revision = layout.layout_revision,
        .after_revision = layout.layout_revision,
        .result_layout = layout,
        .diagnostics = {},
    };

    if (request.kind == EditorDockCommandKind::reset_layout) {
        plan.requires_confirmation = true;
        plan.would_mutate = true;
        if (!request.user_confirmed) {
            append_diagnostic(plan, "confirmation_required", "dock reset command requires explicit confirmation");
            return plan;
        }
        plan.result_layout = make_default_editor_dock_layout();
        accept_dock_mutation(plan);
        return plan;
    }

    const auto source_validation = validate_editor_dock_layout(layout);
    if (!source_validation.valid) {
        append_diagnostic(plan, "invalid_layout", "dock command source layout is invalid");
        return plan;
    }

    const auto catalog = editor_dock_panel_catalog();
    const auto* panel = validate_command_panel(plan, catalog, request.panel_id);
    if (panel == nullptr) {
        return plan;
    }

    switch (request.kind) {
    case EditorDockCommandKind::show_panel: {
        auto* existing_stack = find_tab_stack_containing_panel(plan.result_layout, request.panel_id);
        if (existing_stack != nullptr) {
            if (existing_stack->active_tab_id == request.panel_id &&
                plan.result_layout.focused_panel_id == request.panel_id) {
                accept_dock_noop(plan);
                return plan;
            }
            existing_stack->active_tab_id = request.panel_id;
            plan.result_layout.focused_panel_id = request.panel_id;
            accept_dock_mutation(plan);
            return plan;
        }

        auto* target_stack = find_editor_dock_node(plan.result_layout, request.target_stack_id);
        if (target_stack == nullptr || target_stack->kind != EditorDockNodeKind::tab_stack) {
            append_diagnostic(plan, "missing_target_stack",
                              "dock show command target stack is missing: " + request.target_stack_id);
            return plan;
        }
        target_stack->tabs.push_back(request.panel_id);
        target_stack->active_tab_id = request.panel_id;
        plan.result_layout.focused_panel_id = request.panel_id;
        accept_dock_mutation(plan);
        return plan;
    }

    case EditorDockCommandKind::hide_panel: {
        if (panel->shell_chrome) {
            append_diagnostic(plan, "cannot_hide_shell_chrome",
                              "dock hide command cannot hide shell chrome: " + request.panel_id);
            return plan;
        }
        auto* stack = find_tab_stack_containing_panel(plan.result_layout, request.panel_id);
        if (stack == nullptr) {
            accept_dock_noop(plan);
            return plan;
        }
        if (stack->tabs.size() <= 1U) {
            append_diagnostic(plan, "empty_stack_after_hide",
                              "dock hide command would leave a tab stack empty: " + stack->id);
            return plan;
        }
        std::erase(stack->tabs, request.panel_id);
        if (stack->active_tab_id == request.panel_id) {
            stack->active_tab_id = stack->tabs.front();
        }
        if (plan.result_layout.focused_panel_id == request.panel_id) {
            plan.result_layout.focused_panel_id = stack->active_tab_id;
        }
        accept_dock_mutation(plan);
        return plan;
    }

    case EditorDockCommandKind::activate_tab: {
        auto* stack = find_tab_stack_containing_panel(plan.result_layout, request.panel_id);
        if (stack == nullptr) {
            append_diagnostic(plan, "panel_not_docked",
                              "dock activate command panel is not docked: " + request.panel_id);
            return plan;
        }
        if (stack->active_tab_id == request.panel_id && plan.result_layout.focused_panel_id == request.panel_id) {
            accept_dock_noop(plan);
            return plan;
        }
        stack->active_tab_id = request.panel_id;
        plan.result_layout.focused_panel_id = request.panel_id;
        accept_dock_mutation(plan);
        return plan;
    }

    case EditorDockCommandKind::move_panel_to_stack: {
        auto* source_stack = find_tab_stack_containing_panel(plan.result_layout, request.panel_id);
        auto* target_stack = find_editor_dock_node(plan.result_layout, request.target_stack_id);
        if (source_stack == nullptr) {
            append_diagnostic(plan, "panel_not_docked", "dock move command panel is not docked: " + request.panel_id);
            return plan;
        }
        if (target_stack == nullptr || target_stack->kind != EditorDockNodeKind::tab_stack) {
            append_diagnostic(plan, "missing_target_stack",
                              "dock move command target stack is missing: " + request.target_stack_id);
            return plan;
        }
        if (source_stack == target_stack) {
            if (target_stack->active_tab_id == request.panel_id &&
                plan.result_layout.focused_panel_id == request.panel_id) {
                accept_dock_noop(plan);
                return plan;
            }
            target_stack->active_tab_id = request.panel_id;
            plan.result_layout.focused_panel_id = request.panel_id;
            accept_dock_mutation(plan);
            return plan;
        }
        if (source_stack->tabs.size() <= 1U) {
            append_diagnostic(plan, "empty_stack_after_move",
                              "dock move command would leave a tab stack empty: " + source_stack->id);
            return plan;
        }
        std::erase(source_stack->tabs, request.panel_id);
        if (source_stack->active_tab_id == request.panel_id) {
            source_stack->active_tab_id = source_stack->tabs.front();
        }
        if (!contains_string(target_stack->tabs, request.panel_id)) {
            target_stack->tabs.push_back(request.panel_id);
        }
        target_stack->active_tab_id = request.panel_id;
        plan.result_layout.focused_panel_id = request.panel_id;
        accept_dock_mutation(plan);
        return plan;
    }

    case EditorDockCommandKind::split_panel_to_stack: {
        if (panel->shell_chrome) {
            append_diagnostic(plan, "cannot_split_shell_chrome",
                              "dock split command cannot split shell chrome: " + request.panel_id);
            return plan;
        }
        auto* source_stack = find_tab_stack_containing_panel(plan.result_layout, request.panel_id);
        if (source_stack == nullptr) {
            append_diagnostic(plan, "panel_not_docked", "dock split command panel is not docked: " + request.panel_id);
            return plan;
        }
        if (!request.source_stack_id.empty() && source_stack->id != request.source_stack_id) {
            append_diagnostic(plan, "source_stack_mismatch",
                              "dock split command source stack does not contain panel: " + request.source_stack_id);
            return plan;
        }
        if (source_stack->tabs.size() <= 1U) {
            append_diagnostic(plan, "empty_stack_after_split",
                              "dock split command would leave a tab stack empty: " + source_stack->id);
            return plan;
        }
        if (request.split_ratio <= 0.0F || request.split_ratio >= 1.0F) {
            append_diagnostic(plan, "invalid_split_ratio", "dock split command ratio must be between zero and one");
            return plan;
        }
        if (!validate_new_dock_node_id(plan, plan.result_layout, request.new_stack_id, "invalid_new_stack_id",
                                       "dock split new stack")) {
            return plan;
        }

        const std::string source_stack_id = source_stack->id;
        const std::string new_split_id = source_stack_id + ".split." + request.new_stack_id;
        if (!validate_new_dock_node_id(plan, plan.result_layout, new_split_id, "invalid_new_split_id",
                                       "dock split node")) {
            return plan;
        }

        auto* parent_split = find_parent_split_containing_child(plan.result_layout, source_stack_id);
        if (parent_split == nullptr && plan.result_layout.root_id != source_stack_id) {
            append_diagnostic(plan, "missing_source_parent",
                              "dock split command source stack is not reachable from a split parent: " +
                                  source_stack_id);
            return plan;
        }

        std::erase(source_stack->tabs, request.panel_id);
        if (source_stack->active_tab_id == request.panel_id) {
            source_stack->active_tab_id = source_stack->tabs.front();
        }

        if (parent_split != nullptr) {
            std::ranges::replace(parent_split->children, source_stack_id, new_split_id);
        } else {
            plan.result_layout.root_id = new_split_id;
        }
        plan.result_layout.nodes.push_back(EditorDockNode{
            .id = new_split_id,
            .kind = EditorDockNodeKind::split,
            .axis = request.split_axis,
            .split_ratio = request.split_ratio,
            .children = {source_stack_id, request.new_stack_id},
            .tabs = {},
            .active_tab_id = {},
        });
        plan.result_layout.nodes.push_back(EditorDockNode{
            .id = request.new_stack_id,
            .kind = EditorDockNodeKind::tab_stack,
            .axis = EditorDockSplitAxis::horizontal,
            .split_ratio = 0.5F,
            .children = {},
            .tabs = {request.panel_id},
            .active_tab_id = request.panel_id,
        });
        plan.result_layout.focused_panel_id = request.panel_id;
        accept_dock_mutation(plan);
        return plan;
    }

    case EditorDockCommandKind::reset_layout:
        break;
    }

    append_diagnostic(plan, "unsupported_command", "dock command kind is not implemented");
    return plan;
}

EditorDockCommandPlan apply_editor_dock_command(EditorDockLayout& layout, const EditorDockCommandRequest& request) {
    auto plan = plan_editor_dock_command(layout, request);
    if (plan.accepted) {
        layout = plan.result_layout;
    }
    return plan;
}

EditorDockWindowCommandPlan plan_editor_dock_window_command(const EditorDockMultiWindowLayout& layout,
                                                            const EditorDockWindowCommandRequest& request) {
    EditorDockWindowCommandPlan plan{
        .accepted = false,
        .would_mutate = false,
        .requires_confirmation = false,
        .before_revision = layout.layout_revision,
        .after_revision = layout.layout_revision,
        .result_layout = layout,
        .diagnostics = {},
    };

    if (request.kind == EditorDockWindowCommandKind::reset_all_windows) {
        plan.requires_confirmation = true;
        plan.would_mutate = true;
        if (!request.user_confirmed) {
            append_diagnostic(plan, "confirmation_required", "dock window reset command requires confirmation");
            return plan;
        }
        plan.result_layout = make_default_editor_dock_multi_window_layout();
        accept_dock_window_mutation(plan);
        return plan;
    }

    const auto source_validation = validate_editor_dock_multi_window_layout(layout);
    if (!source_validation.valid) {
        append_diagnostic(plan, "invalid_layout", "dock window command source layout is invalid");
        return plan;
    }

    const auto catalog = editor_dock_panel_catalog();

    switch (request.kind) {
    case EditorDockWindowCommandKind::create_window:
    case EditorDockWindowCommandKind::tear_off_panel: {
        const auto* panel = validate_command_panel(plan, catalog, request.panel_id);
        if (panel == nullptr) {
            return plan;
        }
        auto* source_window = find_editor_dock_window(plan.result_layout, request.window_id);
        if (source_window == nullptr) {
            append_diagnostic(plan, "missing_source_window",
                              "dock tear-off command source window is missing: " + request.window_id);
            return plan;
        }
        if (!subtree_contains_panel(plan.result_layout, source_window->root_id, request.panel_id)) {
            append_diagnostic(plan, "panel_not_in_source_window",
                              "dock tear-off command panel is not in the source window: " + request.panel_id);
            return plan;
        }
        auto* source_stack = find_tab_stack_containing_panel(plan.result_layout, request.panel_id);
        if (source_stack == nullptr) {
            append_diagnostic(plan, "panel_not_docked",
                              "dock tear-off command panel is not docked: " + request.panel_id);
            return plan;
        }
        if (!request.source_stack_id.empty() && source_stack->id != request.source_stack_id) {
            append_diagnostic(plan, "source_stack_mismatch",
                              "dock tear-off command source stack does not contain panel: " + request.source_stack_id);
            return plan;
        }
        if (source_stack->tabs.size() <= 1U) {
            append_diagnostic(plan, "empty_stack_after_tear_off",
                              "dock tear-off command would leave a tab stack empty: " + source_stack->id);
            return plan;
        }
        EditorDockLayoutValidation token_validation{.valid = true};
        validate_id_token(token_validation, request.new_window_id, "new dock window");
        validate_id_token(token_validation, request.monitor_id, "dock monitor");
        if (!token_validation.diagnostics.empty()) {
            append_diagnostic(plan, "invalid_window_request", "dock tear-off command window or monitor id is unsafe");
            return plan;
        }
        if (find_editor_dock_window(plan.result_layout, request.new_window_id) != nullptr) {
            append_diagnostic(plan, "duplicate_dock_window_id",
                              "dock tear-off command window already exists: " + request.new_window_id);
            return plan;
        }
        if (!window_dpi_is_valid(request.dpi_scale)) {
            append_diagnostic(plan, "invalid_window_dpi", "dock tear-off command requires a positive finite dpi scale");
            return plan;
        }
        if (!window_bounds_are_valid(request.bounds)) {
            append_diagnostic(plan, "invalid_window_bounds",
                              "dock tear-off command requires finite window bounds with positive size");
            return plan;
        }
        const auto new_stack_id = make_window_stack_id(request.new_window_id);
        if (!validate_new_dock_node_id(plan, plan.result_layout, new_stack_id, "invalid_new_window_stack_id",
                                       "dock tear-off window stack")) {
            return plan;
        }

        remove_panel_from_stack(*source_stack, request.panel_id);
        if (source_window->focused_panel_id == request.panel_id) {
            source_window->focused_panel_id = source_stack->active_tab_id;
        }
        plan.result_layout.nodes.push_back(EditorDockNode{
            .id = new_stack_id,
            .kind = EditorDockNodeKind::tab_stack,
            .axis = EditorDockSplitAxis::horizontal,
            .split_ratio = 0.5F,
            .children = {},
            .tabs = {request.panel_id},
            .active_tab_id = request.panel_id,
        });
        plan.result_layout.windows.push_back(EditorDockWindowRow{
            .id = request.new_window_id,
            .title = panel->label,
            .monitor_id = request.monitor_id,
            .bounds = request.bounds,
            .dpi_scale = request.dpi_scale,
            .root_id = new_stack_id,
            .focused_panel_id = request.panel_id,
        });
        plan.result_layout.focused_window_id = request.new_window_id;
        accept_dock_window_mutation(plan);
        return plan;
    }

    case EditorDockWindowCommandKind::move_panel_to_window: {
        const auto* panel = validate_command_panel(plan, catalog, request.panel_id);
        if (panel == nullptr) {
            return plan;
        }
        auto* source_window = find_editor_dock_window(plan.result_layout, request.window_id);
        auto* target_window = find_editor_dock_window(plan.result_layout, request.target_window_id);
        auto* source_stack = find_tab_stack_containing_panel(plan.result_layout, request.panel_id);
        auto* target_stack = find_editor_dock_node(plan.result_layout, request.target_stack_id);
        if (source_window == nullptr) {
            append_diagnostic(plan, "missing_source_window",
                              "dock move command source window is missing: " + request.window_id);
            return plan;
        }
        if (target_window == nullptr) {
            append_diagnostic(plan, "missing_target_window",
                              "dock move command target window is missing: " + request.target_window_id);
            return plan;
        }
        if (source_stack == nullptr ||
            !subtree_contains_panel(plan.result_layout, source_window->root_id, request.panel_id)) {
            append_diagnostic(plan, "panel_not_in_source_window",
                              "dock move command panel is not in the source window: " + request.panel_id);
            return plan;
        }
        if (target_stack == nullptr || target_stack->kind != EditorDockNodeKind::tab_stack ||
            !subtree_contains_panel(plan.result_layout, target_window->root_id, target_stack->active_tab_id)) {
            append_diagnostic(plan, "missing_target_stack",
                              "dock move command target stack is missing from target window: " +
                                  request.target_stack_id);
            return plan;
        }
        if (source_stack == target_stack) {
            if (target_stack->active_tab_id == request.panel_id &&
                target_window->focused_panel_id == request.panel_id &&
                plan.result_layout.focused_window_id == target_window->id) {
                accept_dock_window_noop(plan);
                return plan;
            }
            target_stack->active_tab_id = request.panel_id;
            target_window->focused_panel_id = request.panel_id;
            plan.result_layout.focused_window_id = target_window->id;
            accept_dock_window_mutation(plan);
            return plan;
        }
        const bool source_root_stack = source_window->root_id == source_stack->id;
        if (source_stack->tabs.size() <= 1U && !source_root_stack) {
            append_diagnostic(plan, "empty_stack_after_move",
                              "dock move command would leave a nested tab stack empty: " + source_stack->id);
            return plan;
        }
        const std::string source_window_id = source_window->id;
        remove_panel_from_stack(*source_stack, request.panel_id);
        if (!contains_string(target_stack->tabs, request.panel_id)) {
            target_stack->tabs.push_back(request.panel_id);
        }
        target_stack->active_tab_id = request.panel_id;
        if (source_root_stack && source_stack->tabs.empty()) {
            remove_window_by_id(plan.result_layout, source_window_id);
        } else {
            source_window = find_editor_dock_window(plan.result_layout, source_window_id);
            if (source_window != nullptr && source_window->focused_panel_id == request.panel_id) {
                source_window->focused_panel_id = source_stack->active_tab_id;
            }
        }
        target_window = find_editor_dock_window(plan.result_layout, request.target_window_id);
        if (target_window != nullptr) {
            target_window->focused_panel_id = request.panel_id;
            plan.result_layout.focused_window_id = target_window->id;
        }
        accept_dock_window_mutation(plan);
        return plan;
    }

    case EditorDockWindowCommandKind::merge_window: {
        auto* source_window = find_editor_dock_window(plan.result_layout, request.window_id);
        auto* target_window = find_editor_dock_window(plan.result_layout, request.target_window_id);
        auto* target_stack = find_editor_dock_node(plan.result_layout, request.target_stack_id);
        if (source_window == nullptr) {
            append_diagnostic(plan, "missing_source_window",
                              "dock merge command source window is missing: " + request.window_id);
            return plan;
        }
        if (target_window == nullptr) {
            append_diagnostic(plan, "missing_target_window",
                              "dock merge command target window is missing: " + request.target_window_id);
            return plan;
        }
        if (source_window == target_window) {
            accept_dock_window_noop(plan);
            return plan;
        }
        if (target_stack == nullptr || target_stack->kind != EditorDockNodeKind::tab_stack ||
            !subtree_contains_panel(plan.result_layout, target_window->root_id, target_stack->active_tab_id)) {
            append_diagnostic(plan, "missing_target_stack",
                              "dock merge command target stack is missing from target window: " +
                                  request.target_stack_id);
            return plan;
        }
        std::vector<std::string> moved_tabs;
        collect_tabs_from_subtree(plan.result_layout, source_window->root_id, moved_tabs);
        if (moved_tabs.empty()) {
            append_diagnostic(plan, "empty_source_window", "dock merge command source window has no docked panels");
            return plan;
        }
        for (const auto& tab : moved_tabs) {
            if (!contains_string(target_stack->tabs, tab)) {
                target_stack->tabs.push_back(tab);
            }
        }
        target_stack->active_tab_id = moved_tabs.front();
        const std::string target_window_id = target_window->id;
        const std::string source_window_id = source_window->id;
        remove_window_by_id(plan.result_layout, source_window_id);
        target_window = find_editor_dock_window(plan.result_layout, target_window_id);
        if (target_window != nullptr) {
            target_window->focused_panel_id = moved_tabs.front();
            plan.result_layout.focused_window_id = target_window->id;
        }
        accept_dock_window_mutation(plan);
        return plan;
    }

    case EditorDockWindowCommandKind::close_window: {
        const auto* source_window = find_editor_dock_window(plan.result_layout, request.window_id);
        if (source_window == nullptr) {
            append_diagnostic(plan, "missing_source_window",
                              "dock close command source window is missing: " + request.window_id);
            return plan;
        }
        if (plan.result_layout.windows.size() <= 1U) {
            append_diagnostic(plan, "cannot_close_last_window", "dock close command cannot close the last window");
            return plan;
        }
        remove_window_by_id(plan.result_layout, request.window_id);
        if (auto* focused_window = find_editor_dock_window(plan.result_layout, plan.result_layout.focused_window_id);
            focused_window != nullptr &&
            !subtree_contains_panel(plan.result_layout, focused_window->root_id, focused_window->focused_panel_id)) {
            std::vector<std::string> tabs;
            collect_tabs_from_subtree(plan.result_layout, focused_window->root_id, tabs);
            if (!tabs.empty()) {
                focused_window->focused_panel_id = tabs.front();
            }
        }
        accept_dock_window_mutation(plan);
        return plan;
    }

    case EditorDockWindowCommandKind::reset_all_windows:
        break;
    }

    append_diagnostic(plan, "unsupported_command", "dock window command kind is not implemented");
    return plan;
}

EditorDockWindowCommandPlan apply_editor_dock_window_command(EditorDockMultiWindowLayout& layout,
                                                             const EditorDockWindowCommandRequest& request) {
    auto plan = plan_editor_dock_window_command(layout, request);
    if (plan.accepted) {
        layout = plan.result_layout;
    }
    return plan;
}

} // namespace mirakana::editor
