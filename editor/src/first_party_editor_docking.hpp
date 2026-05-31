// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::editor {

enum class FirstPartyEditorDockNodeKind : std::uint8_t {
    split,
    tab_stack,
    panel,
};

enum class FirstPartyEditorDockSplitAxis : std::uint8_t {
    horizontal,
    vertical,
};

struct FirstPartyEditorDockNode {
    std::string id;
    FirstPartyEditorDockNodeKind kind{FirstPartyEditorDockNodeKind::panel};
    FirstPartyEditorDockSplitAxis axis{FirstPartyEditorDockSplitAxis::horizontal};
    float split_ratio{0.5F};
    std::vector<std::string> children;
    std::vector<std::string> tabs;
    std::string active_tab;
};

struct FirstPartyEditorDockGraph {
    std::string root_id;
    std::vector<FirstPartyEditorDockNode> nodes;
};

struct FirstPartyEditorDockValidation {
    bool valid{false};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] FirstPartyEditorDockGraph make_default_first_party_editor_dock_graph();

[[nodiscard]] FirstPartyEditorDockValidation
validate_first_party_editor_dock_graph(const FirstPartyEditorDockGraph& graph);

} // namespace mirakana::editor
