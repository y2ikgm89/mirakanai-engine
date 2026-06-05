// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/editor_dock_layout.hpp"
#include "mirakana/editor/editor_panel.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

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
    [[nodiscard]] const EditorDockLayout& dock_layout() const noexcept;
    [[nodiscard]] EditorDockLayout& dock_layout() noexcept;
    [[nodiscard]] const EditorDockMultiWindowLayout& multi_window_dock_layout() const noexcept;
    [[nodiscard]] EditorDockMultiWindowLayout& multi_window_dock_layout() noexcept;

    void set_panel_visible(PanelId id, bool visible);
    void set_dock_layout(EditorDockLayout layout);
    void set_multi_window_dock_layout(EditorDockMultiWindowLayout layout);

  private:
    explicit Workspace(ProjectInfo project);

    [[nodiscard]] PanelState* find_panel(PanelId id) noexcept;
    [[nodiscard]] const PanelState* find_panel(PanelId id) const noexcept;

    ProjectInfo project_;
    std::vector<PanelState> panels_;
    EditorDockLayout dock_layout_;
    EditorDockMultiWindowLayout multi_window_dock_layout_;
};

[[nodiscard]] std::string_view panel_id_to_string(PanelId id) noexcept;
[[nodiscard]] std::string serialize_workspace(const Workspace& workspace);
[[nodiscard]] Workspace deserialize_workspace(std::string_view text);
[[nodiscard]] std::string serialize_workspace_v3(const Workspace& workspace);
[[nodiscard]] Workspace deserialize_workspace_v3(std::string_view text);

} // namespace mirakana::editor
