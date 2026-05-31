// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/editor_dock_layout.hpp"

#include <algorithm>
#include <array>
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

void validate_id_token(EditorDockLayoutValidation& validation, std::string_view id, std::string_view label) {
    if (id.empty()) {
        append_diagnostic(validation, "missing_id", std::string{label} + " id must not be empty");
        return;
    }
    if (id.find_first_of("\r\n=") != std::string_view::npos || contains_unsafe_token(id)) {
        append_diagnostic(validation, "unsafe_token",
                          std::string{label} + " id contains an unsafe token: " + std::string{id});
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

const EditorDockNode* find_editor_dock_node(const EditorDockLayout& layout, std::string_view id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [id](const EditorDockNode& node) { return node.id == id; });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

EditorDockNode* find_editor_dock_node(EditorDockLayout& layout, std::string_view id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [id](const EditorDockNode& node) { return node.id == id; });
    return it == layout.nodes.end() ? nullptr : &(*it);
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

} // namespace mirakana::editor
