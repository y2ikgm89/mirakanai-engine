// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class PanelId {
    scene = 0,
    inspector,
    assets,
    console,
    viewport,
    resources,
    ai_commands,
    input_rebinding,
    profiler,
    project_settings,
    timeline
};

struct ProjectInfo {
    std::string name;
    std::string root_path;
};

struct PanelState {
    PanelId id;
    bool visible{true};
};

class Workspace {
  public:
    [[nodiscard]] static Workspace create_default(ProjectInfo project);

    [[nodiscard]] const ProjectInfo& project() const noexcept;
    [[nodiscard]] std::size_t panel_count() const noexcept;
    [[nodiscard]] const std::vector<PanelState>& panels() const noexcept;
    [[nodiscard]] bool is_panel_visible(PanelId id) const noexcept;

    void set_panel_visible(PanelId id, bool visible);

  private:
    explicit Workspace(ProjectInfo project);

    [[nodiscard]] PanelState* find_panel(PanelId id) noexcept;
    [[nodiscard]] const PanelState* find_panel(PanelId id) const noexcept;

    ProjectInfo project_;
    std::vector<PanelState> panels_;
};

struct WorkspaceMigrationResult {
    Workspace workspace;
    int source_version{1};
    int target_version{1};
    bool migrated{false};
};

[[nodiscard]] std::string_view panel_id_to_string(PanelId id) noexcept;
[[nodiscard]] std::string serialize_workspace(const Workspace& workspace);
[[nodiscard]] WorkspaceMigrationResult migrate_workspace(std::string_view text);
[[nodiscard]] Workspace deserialize_workspace(std::string_view text);

} // namespace mirakana::editor
