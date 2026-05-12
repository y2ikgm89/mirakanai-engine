// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/render_backend.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/tools/shader_toolchain.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct ProjectShaderToolSettings {
    std::string executable{"dxc"};
    std::string working_directory{"."};
    std::string artifact_output_root{"out/editor/shaders"};
    std::string cache_index_path{"out/editor/shaders/shader-cache.gecache"};
};

struct ProjectDocument {
    std::string name;
    std::string root_path;
    std::string asset_root;
    std::string source_registry_path;
    std::string game_manifest_path;
    std::string startup_scene_path;
    ProjectShaderToolSettings shader_tool;
    EditorRenderBackend render_backend{EditorRenderBackend::automatic};
};

struct ProjectMigrationResult {
    ProjectDocument document;
    int source_version{1};
    int target_version{1};
    bool migrated{false};
};

enum class EditorProjectFileDialogMode { open, save };

struct EditorProjectFileDialogRow {
    std::string id;
    std::string label;
    std::string value;
};

struct EditorProjectFileDialogModel {
    EditorProjectFileDialogMode mode{EditorProjectFileDialogMode::open};
    std::string status_label;
    std::string selected_path;
    bool accepted{false};
    std::vector<EditorProjectFileDialogRow> rows;
    std::vector<std::string> diagnostics;
};

struct ProjectSettingsError {
    std::string field;
    std::string message;
};

class ProjectSettingsDraft {
  public:
    [[nodiscard]] static ProjectSettingsDraft from_project(const ProjectDocument& document);

    [[nodiscard]] const ProjectDocument& document() const noexcept;
    [[nodiscard]] const ProjectShaderToolSettings& shader_tool() const noexcept;
    [[nodiscard]] EditorRenderBackend render_backend() const noexcept;
    [[nodiscard]] std::vector<ProjectSettingsError> validation_errors() const;
    [[nodiscard]] bool can_apply() const;
    [[nodiscard]] ProjectDocument apply() const;

    void set_shader_tool_executable(std::string value);
    void set_shader_tool_working_directory(std::string value);
    void set_shader_artifact_output_root(std::string value);
    void set_shader_cache_index_path(std::string value);
    void set_shader_tool_descriptor(const ShaderToolDescriptor& descriptor);
    void set_render_backend(EditorRenderBackend backend) noexcept;

  private:
    explicit ProjectSettingsDraft(ProjectDocument document);

    ProjectDocument document_;
};

void validate_project_shader_tool_settings(const ProjectShaderToolSettings& settings);
void validate_project_document(const ProjectDocument& document);

[[nodiscard]] std::string serialize_project_document(const ProjectDocument& document);
[[nodiscard]] ProjectMigrationResult migrate_project_document(std::string_view text);
[[nodiscard]] ProjectDocument deserialize_project_document(std::string_view text);

[[nodiscard]] mirakana::FileDialogRequest make_project_open_dialog_request(std::string_view default_location = ".");
[[nodiscard]] mirakana::FileDialogRequest
make_project_save_dialog_request(std::string_view default_location = "GameEngine.geproject");
[[nodiscard]] EditorProjectFileDialogModel make_project_open_dialog_model(const mirakana::FileDialogResult& result);
[[nodiscard]] EditorProjectFileDialogModel make_project_save_dialog_model(const mirakana::FileDialogResult& result);
[[nodiscard]] mirakana::ui::UiDocument make_project_file_dialog_ui_model(const EditorProjectFileDialogModel& model);

} // namespace mirakana::editor
