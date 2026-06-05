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

constexpr std::array<PanelToken, 11> panel_tokens{{
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

void validate_workspace_v3_format(std::string_view value) {
    if (value == "GameEngine.Workspace.v3") {
        return;
    }
    throw std::invalid_argument("unsupported workspace v3 format");
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

struct ParsedDockWindow {
    EditorDockWindowRow window;
    bool touched{false};
    bool has_id{false};
    bool has_title{false};
    bool has_monitor{false};
    bool has_dpi{false};
    bool has_bounds{false};
    bool has_root{false};
    bool has_focus{false};
};

[[nodiscard]] std::size_t parse_dock_node_index(std::string_view value) {
    const auto parsed = parse_u64(value, "workspace dock node index");
    if (parsed > 1024U) {
        throw std::invalid_argument("workspace dock node index is too large");
    }
    return static_cast<std::size_t>(parsed);
}

[[nodiscard]] EditorDockWindowBounds parse_window_bounds(std::string_view value) {
    const auto tokens = split_tokens(value);
    if (tokens.size() != 4U) {
        throw std::invalid_argument("workspace dock window bounds must contain x,y,width,height");
    }
    return EditorDockWindowBounds{
        .x = parse_float(tokens[0], "workspace dock window bounds x"),
        .y = parse_float(tokens[1], "workspace dock window bounds y"),
        .width = parse_float(tokens[2], "workspace dock window bounds width"),
        .height = parse_float(tokens[3], "workspace dock window bounds height"),
    };
}

[[nodiscard]] std::string format_window_bounds(const EditorDockWindowBounds& bounds) {
    std::ostringstream output;
    output << bounds.x << ',' << bounds.y << ',' << bounds.width << ',' << bounds.height;
    return output.str();
}

[[nodiscard]] ParsedDockNode& ensure_parsed_dock_node(std::vector<ParsedDockNode>& nodes, std::size_t index) {
    if (nodes.size() <= index) {
        nodes.resize(index + 1U);
    }
    nodes[index].touched = true;
    return nodes[index];
}

[[nodiscard]] ParsedDockWindow& ensure_parsed_dock_window(std::vector<ParsedDockWindow>& windows, std::size_t index) {
    if (windows.size() <= index) {
        windows.resize(index + 1U);
    }
    windows[index].touched = true;
    return windows[index];
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

void parse_dock_window_field(std::vector<ParsedDockWindow>& windows, std::string_view key, std::string_view value) {
    constexpr std::string_view prefix{"dock.window."};
    const auto rest = key.substr(prefix.size());
    const auto separator = rest.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("workspace dock window key is missing a field");
    }

    auto& parsed = ensure_parsed_dock_window(windows, parse_dock_node_index(rest.substr(0, separator)));
    const auto field = rest.substr(separator + 1U);
    if (field == "id") {
        if (parsed.has_id) {
            throw std::invalid_argument("workspace dock window id must not be duplicated");
        }
        parsed.window.id = std::string{value};
        parsed.has_id = true;
    } else if (field == "title") {
        if (parsed.has_title) {
            throw std::invalid_argument("workspace dock window title must not be duplicated");
        }
        parsed.window.title = std::string{value};
        parsed.has_title = true;
    } else if (field == "monitor") {
        if (parsed.has_monitor) {
            throw std::invalid_argument("workspace dock window monitor must not be duplicated");
        }
        parsed.window.monitor_id = std::string{value};
        parsed.has_monitor = true;
    } else if (field == "dpi") {
        if (parsed.has_dpi) {
            throw std::invalid_argument("workspace dock window dpi must not be duplicated");
        }
        parsed.window.dpi_scale = parse_float(value, "workspace dock window dpi");
        parsed.has_dpi = true;
    } else if (field == "bounds") {
        if (parsed.has_bounds) {
            throw std::invalid_argument("workspace dock window bounds must not be duplicated");
        }
        parsed.window.bounds = parse_window_bounds(value);
        parsed.has_bounds = true;
    } else if (field == "root") {
        if (parsed.has_root) {
            throw std::invalid_argument("workspace dock window root must not be duplicated");
        }
        parsed.window.root_id = std::string{value};
        parsed.has_root = true;
    } else if (field == "focus") {
        if (parsed.has_focus) {
            throw std::invalid_argument("workspace dock window focus must not be duplicated");
        }
        parsed.window.focused_panel_id = std::string{value};
        parsed.has_focus = true;
    } else {
        throw std::invalid_argument("unknown workspace dock window field");
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

[[nodiscard]] std::vector<EditorDockNode> finish_parsed_dock_nodes(std::vector<ParsedDockNode> parsed_nodes) {
    if (parsed_nodes.empty()) {
        throw std::invalid_argument("workspace dock nodes are missing");
    }
    std::vector<EditorDockNode> nodes;
    nodes.reserve(parsed_nodes.size());
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
        nodes.push_back(parsed.node);
    }
    return nodes;
}

[[nodiscard]] EditorDockMultiWindowLayout
finish_parsed_multi_window_dock_layout(std::string focused_window_id, std::optional<std::uint64_t> revision,
                                       std::vector<ParsedDockWindow> parsed_windows,
                                       std::vector<ParsedDockNode> parsed_nodes) {
    if (focused_window_id.empty()) {
        throw std::invalid_argument("workspace dock focused window is missing");
    }
    if (!revision.has_value()) {
        throw std::invalid_argument("workspace dock revision is missing");
    }
    if (parsed_windows.empty()) {
        throw std::invalid_argument("workspace dock windows are missing");
    }

    EditorDockMultiWindowLayout layout;
    layout.focused_window_id = std::move(focused_window_id);
    layout.layout_revision = *revision;
    layout.native_handles_public = false;
    layout.unsupported_capabilities = make_editor_ui_low_level_unsupported_capabilities();
    layout.nodes = finish_parsed_dock_nodes(std::move(parsed_nodes));

    layout.windows.reserve(parsed_windows.size());
    for (auto& parsed : parsed_windows) {
        if (!parsed.touched) {
            throw std::invalid_argument("workspace dock window indexes must be contiguous");
        }
        if (!parsed.has_id || !parsed.has_monitor || !parsed.has_dpi || !parsed.has_bounds || !parsed.has_root ||
            !parsed.has_focus) {
            throw std::invalid_argument("workspace dock window id monitor dpi bounds root and focus are required");
        }
        if (!parsed.has_title) {
            parsed.window.title = parsed.window.id;
        }
        layout.windows.push_back(std::move(parsed.window));
    }

    const auto validation = validate_editor_dock_multi_window_layout(layout);
    if (!validation.valid) {
        throw std::invalid_argument("workspace multi-window dock layout is invalid");
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
    };
    workspace.dock_layout_ = make_default_editor_dock_layout();
    workspace.multi_window_dock_layout_ = make_default_editor_dock_multi_window_layout();
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

const EditorDockMultiWindowLayout& Workspace::multi_window_dock_layout() const noexcept {
    return multi_window_dock_layout_;
}

EditorDockMultiWindowLayout& Workspace::multi_window_dock_layout() noexcept {
    return multi_window_dock_layout_;
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

void Workspace::set_multi_window_dock_layout(EditorDockMultiWindowLayout layout) {
    const auto validation = validate_editor_dock_multi_window_layout(layout);
    if (!validation.valid) {
        throw std::invalid_argument("workspace multi-window dock layout is invalid");
    }
    multi_window_dock_layout_ = std::move(layout);
}

Workspace::Workspace(ProjectInfo project)
    : project_(std::move(project)), dock_layout_(make_default_editor_dock_layout()),
      multi_window_dock_layout_(make_default_editor_dock_multi_window_layout()) {
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

std::string serialize_workspace_v3(const Workspace& workspace) {
    std::ostringstream output;
    output << "format=GameEngine.Workspace.v3\n";
    output << "project.name=" << workspace.project().name << '\n';
    output << "project.root=" << workspace.project().root_path << '\n';
    for (const auto& panel : workspace.panels()) {
        output << "panel." << panel_id_to_string(panel.id) << '=' << (panel.visible ? "visible" : "hidden") << '\n';
    }
    const auto& dock = workspace.multi_window_dock_layout();
    output << "dock.revision=" << dock.layout_revision << '\n';
    output << "dock.window.focus=" << dock.focused_window_id << '\n';
    for (std::size_t index = 0; index < dock.windows.size(); ++index) {
        const auto& window = dock.windows[index];
        const auto prefix = "dock.window." + std::to_string(index) + ".";
        output << prefix << "id=" << window.id << '\n';
        output << prefix << "title=" << window.title << '\n';
        output << prefix << "monitor=" << window.monitor_id << '\n';
        output << prefix << "dpi=" << window.dpi_scale << '\n';
        output << prefix << "bounds=" << format_window_bounds(window.bounds) << '\n';
        output << prefix << "root=" << window.root_id << '\n';
        output << prefix << "focus=" << window.focused_panel_id << '\n';
    }
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

Workspace deserialize_workspace_v3(std::string_view text) {
    bool has_format = false;
    ProjectInfo project;
    std::vector<PanelState> panel_overrides;
    std::optional<std::uint64_t> dock_revision;
    std::string focused_window_id;
    std::vector<ParsedDockWindow> dock_windows;
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
            validate_workspace_v3_format(value);
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
        } else if (key == "dock.revision") {
            if (dock_revision.has_value()) {
                throw std::invalid_argument("workspace dock revision must not be duplicated");
            }
            dock_revision = parse_u64(value, "workspace dock revision");
        } else if (key == "dock.window.focus") {
            if (!focused_window_id.empty()) {
                throw std::invalid_argument("workspace dock focused window must not be duplicated");
            }
            focused_window_id = std::string{value};
        } else if (key.starts_with("dock.window.")) {
            parse_dock_window_field(dock_windows, key, value);
        } else if (key.starts_with("dock.node.")) {
            parse_dock_node_field(dock_nodes, key, value);
        } else {
            throw std::invalid_argument("unknown workspace v3 key");
        }
    }

    if (!has_format) {
        throw std::invalid_argument("workspace format is missing");
    }

    auto workspace = Workspace::create_default(std::move(project));
    for (const auto& panel : panel_overrides) {
        workspace.set_panel_visible(panel.id, panel.visible);
    }
    workspace.set_multi_window_dock_layout(finish_parsed_multi_window_dock_layout(
        std::move(focused_window_id), dock_revision, std::move(dock_windows), std::move(dock_nodes)));
    return workspace;
}

} // namespace mirakana::editor
