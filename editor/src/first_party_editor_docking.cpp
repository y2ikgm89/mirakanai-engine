// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "first_party_editor_docking.hpp"

#include <string_view>

namespace mirakana::editor {
namespace {

[[nodiscard]] const FirstPartyEditorDockNode* find_node(const FirstPartyEditorDockGraph& graph,
                                                        std::string_view id) noexcept {
    for (const auto& node : graph.nodes) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

[[nodiscard]] bool contains_string(const std::vector<std::string>& values, std::string_view value) noexcept {
    for (const auto& candidate : values) {
        if (candidate == value) {
            return true;
        }
    }
    return false;
}

void append_diagnostic(FirstPartyEditorDockValidation& validation, std::string message) {
    validation.diagnostics.push_back(std::move(message));
}

} // namespace

FirstPartyEditorDockGraph make_default_first_party_editor_dock_graph() {
    FirstPartyEditorDockGraph graph;
    graph.root_id = "dock.root";
    graph.nodes = {
        FirstPartyEditorDockNode{
            .id = "dock.root",
            .kind = FirstPartyEditorDockNodeKind::split,
            .axis = FirstPartyEditorDockSplitAxis::horizontal,
            .split_ratio = 0.2F,
            .children = {"dock.left_stack", "dock.center_split", "dock.right_stack"},
        },
        FirstPartyEditorDockNode{
            .id = "dock.left_stack",
            .kind = FirstPartyEditorDockNodeKind::tab_stack,
            .tabs = {"main_menu", "scene", "assets", "resources"},
            .active_tab = "scene",
        },
        FirstPartyEditorDockNode{
            .id = "dock.center_split",
            .kind = FirstPartyEditorDockNodeKind::split,
            .axis = FirstPartyEditorDockSplitAxis::vertical,
            .split_ratio = 0.72F,
            .children = {"dock.viewport_stack", "dock.bottom_stack"},
        },
        FirstPartyEditorDockNode{
            .id = "dock.viewport_stack",
            .kind = FirstPartyEditorDockNodeKind::tab_stack,
            .tabs = {"viewport"},
            .active_tab = "viewport",
        },
        FirstPartyEditorDockNode{
            .id = "dock.bottom_stack",
            .kind = FirstPartyEditorDockNodeKind::tab_stack,
            .tabs = {"console", "profiler", "timeline"},
            .active_tab = "console",
        },
        FirstPartyEditorDockNode{
            .id = "dock.right_stack",
            .kind = FirstPartyEditorDockNodeKind::tab_stack,
            .tabs = {"inspector", "ai_commands", "project_settings"},
            .active_tab = "inspector",
        },
    };
    return graph;
}

FirstPartyEditorDockValidation validate_first_party_editor_dock_graph(const FirstPartyEditorDockGraph& graph) {
    FirstPartyEditorDockValidation validation{.valid = true};
    if (graph.root_id.empty()) {
        append_diagnostic(validation, "missing dock root id");
    }
    if (graph.nodes.empty()) {
        append_diagnostic(validation, "dock graph has no nodes");
    }

    for (std::size_t index = 0; index < graph.nodes.size(); ++index) {
        const auto& node = graph.nodes[index];
        if (node.id.empty()) {
            append_diagnostic(validation, "missing dock node id");
            continue;
        }
        for (std::size_t other = index + 1; other < graph.nodes.size(); ++other) {
            if (node.id == graph.nodes[other].id) {
                append_diagnostic(validation, "duplicate dock node id: " + node.id);
            }
        }
    }

    if (!graph.root_id.empty() && find_node(graph, graph.root_id) == nullptr) {
        append_diagnostic(validation, "missing dock root node: " + graph.root_id);
    }

    for (const auto& node : graph.nodes) {
        if (node.id.empty()) {
            continue;
        }
        if (node.kind == FirstPartyEditorDockNodeKind::split) {
            if (node.children.size() < 2U) {
                append_diagnostic(validation, "split dock node requires at least two children: " + node.id);
            }
            if (node.split_ratio <= 0.0F || node.split_ratio >= 1.0F) {
                append_diagnostic(validation, "split dock node ratio must be between zero and one: " + node.id);
            }
            for (const auto& child : node.children) {
                if (find_node(graph, child) == nullptr) {
                    append_diagnostic(validation, "split dock node references missing child: " + child);
                }
            }
        } else if (node.kind == FirstPartyEditorDockNodeKind::tab_stack) {
            if (node.tabs.empty()) {
                append_diagnostic(validation, "tab stack requires at least one tab: " + node.id);
            }
            if (node.active_tab.empty() || !contains_string(node.tabs, node.active_tab)) {
                append_diagnostic(validation, "tab stack active tab is missing from tabs: " + node.id);
            }
        }
    }

    validation.valid = validation.diagnostics.empty();
    return validation;
}

} // namespace mirakana::editor
