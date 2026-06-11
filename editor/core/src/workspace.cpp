// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/workspace.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

void validate_project(const ProjectInfo& project) {
    if (project.name.empty()) {
        throw std::invalid_argument("project name must not be empty");
    }
    if (project.root_path.empty()) {
        throw std::invalid_argument("project root path must not be empty");
    }
    if (project.name.find_first_of("\r\n=") != std::string::npos ||
        project.root_path.find_first_of("\r\n=") != std::string::npos) {
        throw std::invalid_argument("project fields must not contain newlines or '='");
    }
}

struct PanelToken {
    PanelId id;
    std::string_view token;
};

constexpr std::array<PanelToken, 12> panel_tokens{{
    PanelToken{.id = PanelId::scene, .token = "scene"},
    PanelToken{.id = PanelId::inspector, .token = "inspector"},
    PanelToken{.id = PanelId::assets, .token = "assets"},
    PanelToken{.id = PanelId::console, .token = "console"},
    PanelToken{.id = PanelId::viewport, .token = "viewport"},
    PanelToken{.id = PanelId::resources, .token = "resources"},
    PanelToken{.id = PanelId::ai_commands, .token = "ai_commands"},
    PanelToken{.id = PanelId::input_rebinding, .token = "input_rebinding"},
    PanelToken{.id = PanelId::profiler, .token = "profiler"},
    PanelToken{.id = PanelId::project_settings, .token = "project_settings"},
    PanelToken{.id = PanelId::timeline, .token = "timeline"},
    PanelToken{.id = PanelId::environment_settings, .token = "environment_settings"},
}};

PanelId panel_id_from_string(std::string_view token) {
    const auto it =
        std::ranges::find_if(panel_tokens, [token](const PanelToken& panel) { return panel.token == token; });
    if (it == panel_tokens.end()) {
        throw std::invalid_argument("unknown workspace panel id");
    }
    return it->id;
}

void validate_workspace_format(std::string_view value) {
    if (value == "GameEngine.Workspace.v2") {
        return;
    }
    throw std::invalid_argument("unsupported workspace format");
}

bool contains_panel(const std::vector<PanelState>& panels, PanelId id) {
    return std::ranges::any_of(panels, [id](const PanelState& panel) { return panel.id == id; });
}

[[nodiscard]] std::string join_tokens(const std::vector<std::string>& values) {
    std::ostringstream output;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0U) {
            output << ',';
        }
        output << values[index];
    }
    return output.str();
}

[[nodiscard]] std::vector<std::string> split_tokens(std::string_view value) {
    std::vector<std::string> tokens;
    std::size_t begin = 0;
    while (begin <= value.size()) {
        const auto separator = value.find(',', begin);
        const auto end = separator == std::string_view::npos ? value.size() : separator;
        const auto token = value.substr(begin, end - begin);
        if (token.empty()) {
            throw std::invalid_argument("workspace dock token list must not contain empty entries");
        }
        tokens.emplace_back(token);
        if (separator == std::string_view::npos) {
            break;
        }
        begin = separator + 1U;
    }
    return tokens;
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value, const char* field) {
    std::size_t consumed = 0;
    const auto parsed = std::stoull(std::string{value}, &consumed);
    if (consumed != value.size()) {
        throw std::invalid_argument(std::string{field} + " must be an unsigned integer");
    }
    return parsed;
}

[[nodiscard]] float parse_float(std::string_view value, const char* field) {
    std::size_t consumed = 0;
    const auto parsed = std::stof(std::string{value}, &consumed);
    if (consumed != value.size()) {
        throw std::invalid_argument(std::string{field} + " must be a finite float");
    }
    return parsed;
}

[[nodiscard]] bool parse_bool(std::string_view value, const char* field) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument(std::string{field} + " must be true or false");
}

[[nodiscard]] std::string_view dock_node_kind_to_string(EditorDockNodeKind kind) noexcept {
    return kind == EditorDockNodeKind::split ? "split" : "tab_stack";
}

[[nodiscard]] EditorDockNodeKind dock_node_kind_from_string(std::string_view value) {
    if (value == "split") {
        return EditorDockNodeKind::split;
    }
    if (value == "tab_stack") {
        return EditorDockNodeKind::tab_stack;
    }
    throw std::invalid_argument("workspace dock node kind is invalid");
}

[[nodiscard]] std::string_view dock_split_axis_to_string(EditorDockSplitAxis axis) noexcept {
    return axis == EditorDockSplitAxis::horizontal ? "horizontal" : "vertical";
}

[[nodiscard]] EditorDockSplitAxis dock_split_axis_from_string(std::string_view value) {
    if (value == "horizontal") {
        return EditorDockSplitAxis::horizontal;
    }
    if (value == "vertical") {
        return EditorDockSplitAxis::vertical;
    }
    throw std::invalid_argument("workspace dock split axis is invalid");
}

struct ParsedDockNode {
    EditorDockNode node;
    bool touched{false};
    bool has_id{false};
    bool has_kind{false};
    bool has_axis{false};
    bool has_ratio{false};
    bool has_children{false};
    bool has_tabs{false};
    bool has_active{false};
};

[[nodiscard]] std::size_t parse_dock_node_index(std::string_view value) {
    const auto parsed = parse_u64(value, "workspace dock node index");
    if (parsed > 1024U) {
        throw std::invalid_argument("workspace dock node index is too large");
    }
    return static_cast<std::size_t>(parsed);
}

[[nodiscard]] ParsedDockNode& ensure_parsed_dock_node(std::vector<ParsedDockNode>& nodes, std::size_t index) {
    if (nodes.size() <= index) {
        nodes.resize(index + 1U);
    }
    nodes[index].touched = true;
    return nodes[index];
}

void parse_dock_node_field(std::vector<ParsedDockNode>& nodes, std::string_view key, std::string_view value) {
    constexpr std::string_view prefix{"dock.node."};
    const auto rest = key.substr(prefix.size());
    const auto separator = rest.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("workspace dock node key is missing a field");
    }

    auto& parsed = ensure_parsed_dock_node(nodes, parse_dock_node_index(rest.substr(0, separator)));
    const auto field = rest.substr(separator + 1U);
    if (field == "id") {
        if (parsed.has_id) {
            throw std::invalid_argument("workspace dock node id must not be duplicated");
        }
        parsed.node.id = std::string{value};
        parsed.has_id = true;
    } else if (field == "kind") {
        if (parsed.has_kind) {
            throw std::invalid_argument("workspace dock node kind must not be duplicated");
        }
        parsed.node.kind = dock_node_kind_from_string(value);
        parsed.has_kind = true;
    } else if (field == "axis") {
        if (parsed.has_axis) {
            throw std::invalid_argument("workspace dock node axis must not be duplicated");
        }
        parsed.node.axis = dock_split_axis_from_string(value);
        parsed.has_axis = true;
    } else if (field == "ratio") {
        if (parsed.has_ratio) {
            throw std::invalid_argument("workspace dock node ratio must not be duplicated");
        }
        parsed.node.split_ratio = parse_float(value, "workspace dock node ratio");
        parsed.has_ratio = true;
    } else if (field == "children") {
        if (parsed.has_children) {
            throw std::invalid_argument("workspace dock node children must not be duplicated");
        }
        parsed.node.children = split_tokens(value);
        parsed.has_children = true;
    } else if (field == "tabs") {
        if (parsed.has_tabs) {
            throw std::invalid_argument("workspace dock node tabs must not be duplicated");
        }
        parsed.node.tabs = split_tokens(value);
        parsed.has_tabs = true;
    } else if (field == "active") {
        if (parsed.has_active) {
            throw std::invalid_argument("workspace dock node active tab must not be duplicated");
        }
        parsed.node.active_tab_id = std::string{value};
        parsed.has_active = true;
    } else {
        throw std::invalid_argument("unknown workspace dock node field");
    }
}

[[nodiscard]] EditorDockLayout finish_parsed_dock_layout(std::string root_id, std::string focused_panel_id,
                                                         std::optional<std::uint64_t> revision,
                                                         std::optional<bool> persist_to_workspace,
                                                         std::vector<ParsedDockNode> parsed_nodes) {
    if (root_id.empty()) {
        throw std::invalid_argument("workspace dock root is missing");
    }
    if (focused_panel_id.empty()) {
        throw std::invalid_argument("workspace dock focus is missing");
    }
    if (!revision.has_value()) {
        throw std::invalid_argument("workspace dock revision is missing");
    }
    if (!persist_to_workspace.has_value()) {
        throw std::invalid_argument("workspace dock persist flag is missing");
    }

    EditorDockLayout layout;
    layout.root_id = std::move(root_id);
    layout.focused_panel_id = std::move(focused_panel_id);
    layout.layout_revision = *revision;
    layout.persist_to_workspace = *persist_to_workspace;
    layout.unsupported_capabilities = make_editor_ui_low_level_unsupported_capabilities();

    if (parsed_nodes.empty()) {
        throw std::invalid_argument("workspace dock nodes are missing");
    }
    layout.nodes.reserve(parsed_nodes.size());
    for (const auto& parsed : parsed_nodes) {
        if (!parsed.touched) {
            throw std::invalid_argument("workspace dock node indexes must be contiguous");
        }
        if (!parsed.has_id || !parsed.has_kind) {
            throw std::invalid_argument("workspace dock node id and kind are required");
        }
        if (parsed.node.kind == EditorDockNodeKind::split) {
            if (!parsed.has_axis || !parsed.has_ratio || !parsed.has_children) {
                throw std::invalid_argument("workspace split dock node axis ratio and children are required");
            }
        } else if (!parsed.has_tabs || !parsed.has_active) {
            throw std::invalid_argument("workspace tab stack dock node tabs and active tab are required");
        }
        layout.nodes.push_back(parsed.node);
    }

    const auto validation = validate_editor_dock_layout(layout);
    if (!validation.valid) {
        throw std::invalid_argument("workspace dock layout is invalid");
    }
    return layout;
}

} // namespace

Workspace Workspace::create_default(ProjectInfo project) {
    Workspace workspace(std::move(project));
    workspace.panels_ = {
        PanelState{.id = PanelId::scene, .visible = true},
        PanelState{.id = PanelId::inspector, .visible = true},
        PanelState{.id = PanelId::assets, .visible = true},
        PanelState{.id = PanelId::console, .visible = true},
        PanelState{.id = PanelId::viewport, .visible = true},
        PanelState{.id = PanelId::resources, .visible = false},
        PanelState{.id = PanelId::ai_commands, .visible = false},
        PanelState{.id = PanelId::input_rebinding, .visible = false},
        PanelState{.id = PanelId::profiler, .visible = false},
        PanelState{.id = PanelId::project_settings, .visible = false},
        PanelState{.id = PanelId::timeline, .visible = false},
        PanelState{.id = PanelId::environment_settings, .visible = false},
    };
    workspace.dock_layout_ = make_default_editor_dock_layout();
    return workspace;
}

const ProjectInfo& Workspace::project() const noexcept {
    return project_;
}

std::size_t Workspace::panel_count() const noexcept {
    return panels_.size();
}

const std::vector<PanelState>& Workspace::panels() const noexcept {
    return panels_;
}

bool Workspace::is_panel_visible(PanelId id) const noexcept {
    const auto* panel = find_panel(id);
    return panel != nullptr && panel->visible;
}

const EditorDockLayout& Workspace::dock_layout() const noexcept {
    return dock_layout_;
}

EditorDockLayout& Workspace::dock_layout() noexcept {
    return dock_layout_;
}

void Workspace::set_panel_visible(PanelId id, bool visible) {
    auto* panel = find_panel(id);
    if (panel == nullptr) {
        throw std::invalid_argument("unknown editor panel");
    }
    panel->visible = visible;
}

void Workspace::set_dock_layout(EditorDockLayout layout) {
    const auto validation = validate_editor_dock_layout(layout);
    if (!validation.valid) {
        throw std::invalid_argument("workspace dock layout is invalid");
    }
    dock_layout_ = std::move(layout);
}

Workspace::Workspace(ProjectInfo project)
    : project_(std::move(project)), dock_layout_(make_default_editor_dock_layout()) {
    validate_project(project_);
}

PanelState* Workspace::find_panel(PanelId id) noexcept {
    const auto it = std::ranges::find_if(panels_, [id](const PanelState& panel) { return panel.id == id; });
    return it == panels_.end() ? nullptr : &(*it);
}

const PanelState* Workspace::find_panel(PanelId id) const noexcept {
    const auto it = std::ranges::find_if(panels_, [id](const PanelState& panel) { return panel.id == id; });
    return it == panels_.end() ? nullptr : &(*it);
}

std::string_view panel_id_to_string(PanelId id) noexcept {
    const auto it = std::ranges::find_if(panel_tokens, [id](const PanelToken& panel) { return panel.id == id; });
    return it == panel_tokens.end() ? "unknown" : it->token;
}

std::string serialize_workspace(const Workspace& workspace) {
    std::ostringstream output;
    output << "format=GameEngine.Workspace.v2\n";
    output << "project.name=" << workspace.project().name << '\n';
    output << "project.root=" << workspace.project().root_path << '\n';
    for (const auto& panel : workspace.panels()) {
        output << "panel." << panel_id_to_string(panel.id) << '=' << (panel.visible ? "visible" : "hidden") << '\n';
    }
    const auto& dock = workspace.dock_layout();
    output << "dock.root=" << dock.root_id << '\n';
    output << "dock.focus=" << dock.focused_panel_id << '\n';
    output << "dock.revision=" << dock.layout_revision << '\n';
    output << "dock.persist=" << (dock.persist_to_workspace ? "true" : "false") << '\n';
    for (std::size_t index = 0; index < dock.nodes.size(); ++index) {
        const auto& node = dock.nodes[index];
        const auto prefix = "dock.node." + std::to_string(index) + ".";
        output << prefix << "id=" << node.id << '\n';
        output << prefix << "kind=" << dock_node_kind_to_string(node.kind) << '\n';
        if (node.kind == EditorDockNodeKind::split) {
            output << prefix << "axis=" << dock_split_axis_to_string(node.axis) << '\n';
            output << prefix << "ratio=" << node.split_ratio << '\n';
            output << prefix << "children=" << join_tokens(node.children) << '\n';
        } else {
            output << prefix << "tabs=" << join_tokens(node.tabs) << '\n';
            output << prefix << "active=" << node.active_tab_id << '\n';
        }
    }
    return output.str();
}

Workspace deserialize_workspace(std::string_view text) {
    bool has_format = false;
    ProjectInfo project;
    std::vector<PanelState> panel_overrides;
    std::string dock_root_id;
    std::string dock_focused_panel_id;
    std::optional<std::uint64_t> dock_revision;
    std::optional<bool> dock_persist_to_workspace;
    std::vector<ParsedDockNode> dock_nodes;
    std::istringstream input(std::string{text});
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::invalid_argument("workspace line must contain '='");
        }

        const auto key = std::string_view(line).substr(0, separator);
        const auto value = std::string_view(line).substr(separator + 1);
        if (key == "format") {
            validate_workspace_format(value);
            has_format = true;
        } else if (key == "project.name") {
            project.name = std::string(value);
        } else if (key == "project.root") {
            project.root_path = std::string(value);
        } else if (key.starts_with("panel.")) {
            const auto panel_id = panel_id_from_string(key.substr(6));
            if (value != "visible" && value != "hidden") {
                throw std::invalid_argument("workspace panel state must be visible or hidden");
            }
            if (contains_panel(panel_overrides, panel_id)) {
                throw std::invalid_argument("workspace panel state must not be duplicated");
            }
            panel_overrides.push_back(PanelState{.id = panel_id, .visible = value == "visible"});
        } else if (key == "dock.root") {
            if (!dock_root_id.empty()) {
                throw std::invalid_argument("workspace dock root must not be duplicated");
            }
            dock_root_id = std::string{value};
        } else if (key == "dock.focus") {
            if (!dock_focused_panel_id.empty()) {
                throw std::invalid_argument("workspace dock focus must not be duplicated");
            }
            dock_focused_panel_id = std::string{value};
        } else if (key == "dock.revision") {
            if (dock_revision.has_value()) {
                throw std::invalid_argument("workspace dock revision must not be duplicated");
            }
            dock_revision = parse_u64(value, "workspace dock revision");
        } else if (key == "dock.persist") {
            if (dock_persist_to_workspace.has_value()) {
                throw std::invalid_argument("workspace dock persist flag must not be duplicated");
            }
            dock_persist_to_workspace = parse_bool(value, "workspace dock persist flag");
        } else if (key.starts_with("dock.node.")) {
            parse_dock_node_field(dock_nodes, key, value);
        } else {
            throw std::invalid_argument("unknown workspace key");
        }
    }

    if (!has_format) {
        throw std::invalid_argument("workspace format is missing");
    }

    auto workspace = Workspace::create_default(std::move(project));
    for (const auto& panel : panel_overrides) {
        workspace.set_panel_visible(panel.id, panel.visible);
    }
    workspace.set_dock_layout(finish_parsed_dock_layout(std::move(dock_root_id), std::move(dock_focused_panel_id),
                                                        dock_revision, dock_persist_to_workspace,
                                                        std::move(dock_nodes)));
    return workspace;
}

} // namespace mirakana::editor
