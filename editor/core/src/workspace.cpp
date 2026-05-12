// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/workspace.hpp"

#include <algorithm>
#include <array>
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

int parse_workspace_format_version(std::string_view value) {
    if (value == "GameEngine.Workspace.v0") {
        return 0;
    }
    if (value == "GameEngine.Workspace.v1") {
        return 1;
    }
    throw std::invalid_argument("unsupported workspace format");
}

bool contains_panel(const std::vector<PanelState>& panels, PanelId id) {
    return std::ranges::any_of(panels, [id](const PanelState& panel) { return panel.id == id; });
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

void Workspace::set_panel_visible(PanelId id, bool visible) {
    auto* panel = find_panel(id);
    if (panel == nullptr) {
        throw std::invalid_argument("unknown editor panel");
    }
    panel->visible = visible;
}

Workspace::Workspace(ProjectInfo project) : project_(std::move(project)) {
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
    output << "format=GameEngine.Workspace.v1\n";
    output << "project.name=" << workspace.project().name << '\n';
    output << "project.root=" << workspace.project().root_path << '\n';
    for (const auto& panel : workspace.panels()) {
        output << "panel." << panel_id_to_string(panel.id) << '=' << (panel.visible ? "visible" : "hidden") << '\n';
    }
    return output.str();
}

WorkspaceMigrationResult migrate_workspace(std::string_view text) {
    bool has_format = false;
    int source_version = 1;
    ProjectInfo project;
    std::vector<PanelState> panel_overrides;
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
            source_version = parse_workspace_format_version(value);
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
    return WorkspaceMigrationResult{
        .workspace = std::move(workspace),
        .source_version = source_version,
        .target_version = 1,
        .migrated = source_version != 1,
    };
}

Workspace deserialize_workspace(std::string_view text) {
    auto migrated = migrate_workspace(text);
    return std::move(migrated.workspace);
}

} // namespace mirakana::editor
