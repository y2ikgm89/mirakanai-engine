// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "native_editor_launch.hpp"

#include "mirakana/editor/ai_command_panel.hpp"
#include "mirakana/editor/profiler.hpp"
#include "mirakana/editor/project.hpp"
#include "mirakana/editor/resource_panel.hpp"
#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/editor/ui_model.hpp"
#include "mirakana/editor/workspace.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace mirakana::editor {

class NativeEditorApp {
  public:
    explicit NativeEditorApp(NativeEditorLaunchOptions options);
    ~NativeEditorApp();

    NativeEditorApp(const NativeEditorApp&) = delete;
    NativeEditorApp& operator=(const NativeEditorApp&) = delete;
    NativeEditorApp(NativeEditorApp&&) noexcept;
    NativeEditorApp& operator=(NativeEditorApp&&) noexcept;

    [[nodiscard]] const NativeEditorLaunchOptions& options() const noexcept;
    [[nodiscard]] std::uint32_t frames_recorded() const noexcept;
    [[nodiscard]] std::uint32_t native_panel_count() const noexcept;
    [[nodiscard]] bool has_native_panel(std::string_view id) const noexcept;
    [[nodiscard]] std::uint32_t panels_rendered_last_frame() const noexcept;

    [[nodiscard]] const Workspace& workspace() const noexcept;
    [[nodiscard]] bool is_panel_visible(PanelId id) const noexcept;
    void set_panel_visible(PanelId id, bool visible);

    [[nodiscard]] const ProjectDocument& project() const noexcept;
    [[nodiscard]] const SceneAuthoringDocument& scene_document() const noexcept;
    [[nodiscard]] std::span<const EditorPropertyRow> inspector_rows() const noexcept;
    [[nodiscard]] std::span<const EditorAssetListRow> asset_rows() const noexcept;
    [[nodiscard]] std::span<const EditorDiagnosticRow> console_rows() const noexcept;
    [[nodiscard]] const EditorResourcePanelModel& resources() const noexcept;
    [[nodiscard]] const EditorAiCommandPanelModel& ai_commands() const noexcept;
    [[nodiscard]] const EditorProfilerPanelModel& profiler() const noexcept;
    [[nodiscard]] const EditorTimelinePanelModel& timeline() const noexcept;
    [[nodiscard]] std::vector<ProjectSettingsError> project_settings_errors() const;

    [[nodiscard]] int run();
    void record_native_frame() noexcept;
    void record_native_panels_rendered(std::uint32_t count) noexcept;
    void record_native_resource_device_ready(std::uint64_t frame_index);

  private:
    NativeEditorLaunchOptions options_;
    std::uint32_t frames_recorded_{0};
    std::uint32_t panels_rendered_last_frame_{0};
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana::editor
