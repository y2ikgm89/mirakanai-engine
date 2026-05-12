// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/project.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class ProjectCreationStep { identity = 0, paths, review };

struct ProjectCreationDraft {
    std::string name;
    std::string root_path;
    std::string asset_root{"assets"};
    std::string source_registry_path{"source/assets/package.geassets"};
    std::string game_manifest_path{"game.agent.json"};
    std::string startup_scene_path{"scenes/start.scene"};
};

struct ProjectCreationError {
    std::string field;
    std::string message;
};

class ProjectCreationWizard {
  public:
    [[nodiscard]] static ProjectCreationWizard begin();

    [[nodiscard]] ProjectCreationStep step() const noexcept;
    [[nodiscard]] const ProjectCreationDraft& draft() const noexcept;
    [[nodiscard]] std::vector<ProjectCreationError> validation_errors() const;
    [[nodiscard]] bool can_advance() const;
    [[nodiscard]] ProjectDocument create_project_document() const;

    void set_name(std::string value);
    void set_root_path(std::string value);
    void set_asset_root(std::string value);
    void set_source_registry_path(std::string value);
    void set_game_manifest_path(std::string value);
    void set_startup_scene_path(std::string value);

    [[nodiscard]] bool advance();
    [[nodiscard]] bool back();
    void reset();

  private:
    ProjectCreationWizard() = default;

    ProjectCreationStep step_{ProjectCreationStep::identity};
    ProjectCreationDraft draft_{};
};

[[nodiscard]] std::string_view project_creation_step_name(ProjectCreationStep step) noexcept;

} // namespace mirakana::editor
