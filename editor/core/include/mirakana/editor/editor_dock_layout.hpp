// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/workspace.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorDockNodeKind : std::uint8_t {
    split,
    tab_stack,
};

enum class EditorDockSplitAxis : std::uint8_t {
    horizontal,
    vertical,
};

struct EditorDockUnsupportedCapability {
    std::string id;
    std::string official_boundary;
    bool implemented{false};
    bool native_handles_public{false};
};

struct EditorDockPanelCatalogRow {
    std::string id;
    std::string label;
    PanelId workspace_id{PanelId::scene};
    bool shell_chrome{false};
    bool workspace_panel{true};
    bool native_shell_panel{true};
};

struct EditorDockNode {
    std::string id;
    EditorDockNodeKind kind{EditorDockNodeKind::tab_stack};
    EditorDockSplitAxis axis{EditorDockSplitAxis::horizontal};
    float split_ratio{0.5F};
    std::vector<std::string> children;
    std::vector<std::string> tabs;
    std::string active_tab_id;
};

struct EditorDockLayout {
    std::string root_id;
    std::vector<EditorDockNode> nodes;
    std::string focused_panel_id;
    std::uint64_t layout_revision{1};
    bool persist_to_workspace{true};
    std::vector<EditorDockUnsupportedCapability> unsupported_capabilities;
};

struct EditorDockLayoutDiagnostic {
    std::string code;
    std::string message;
};

struct EditorDockLayoutValidation {
    bool valid{false};
    std::vector<EditorDockLayoutDiagnostic> diagnostics;
};

enum class EditorDockCommandKind : std::uint8_t {
    show_panel,
    hide_panel,
    activate_tab,
    move_panel_to_stack,
    reset_layout,
};

struct EditorDockCommandRequest {
    EditorDockCommandKind kind{EditorDockCommandKind::show_panel};
    std::string panel_id;
    std::string target_stack_id;
    bool user_confirmed{false};
};

struct EditorDockCommandDiagnostic {
    std::string code;
    std::string message;
};

struct EditorDockCommandPlan {
    bool accepted{false};
    bool would_mutate{false};
    bool requires_confirmation{false};
    std::uint64_t before_revision{0};
    std::uint64_t after_revision{0};
    EditorDockLayout result_layout;
    std::vector<EditorDockCommandDiagnostic> diagnostics;
};

[[nodiscard]] std::vector<EditorDockPanelCatalogRow> editor_dock_panel_catalog();
[[nodiscard]] const EditorDockPanelCatalogRow*
find_editor_dock_panel(const std::vector<EditorDockPanelCatalogRow>& catalog, std::string_view id) noexcept;

[[nodiscard]] std::vector<EditorDockUnsupportedCapability> make_editor_ui_low_level_unsupported_capabilities();

[[nodiscard]] EditorDockLayout make_default_editor_dock_layout();
[[nodiscard]] EditorDockLayoutValidation validate_editor_dock_layout(const EditorDockLayout& layout);

[[nodiscard]] const EditorDockNode* find_editor_dock_node(const EditorDockLayout& layout, std::string_view id) noexcept;
[[nodiscard]] EditorDockNode* find_editor_dock_node(EditorDockLayout& layout, std::string_view id) noexcept;

[[nodiscard]] EditorDockCommandPlan plan_editor_dock_command(const EditorDockLayout& layout,
                                                             const EditorDockCommandRequest& request);
[[nodiscard]] EditorDockCommandPlan apply_editor_dock_command(EditorDockLayout& layout,
                                                              const EditorDockCommandRequest& request);

} // namespace mirakana::editor
