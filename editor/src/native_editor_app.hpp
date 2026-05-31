// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "native_editor_launch.hpp"
#include "native_material_preview_cache.hpp"
#include "native_viewport_surface.hpp"

#include "mirakana/editor/ai_command_panel.hpp"
#include "mirakana/editor/material_asset_preview_panel.hpp"
#include "mirakana/editor/profiler.hpp"
#include "mirakana/editor/project.hpp"
#include "mirakana/editor/resource_panel.hpp"
#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/editor/ui_model.hpp"
#include "mirakana/editor/workspace.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/platform/process.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct NativeEditorServiceStatus {
    std::string file_dialog_service_id{"memory"};
    std::string clipboard_service_id{"memory"};
    std::string reviewed_process_runner_id{"recording"};
    bool file_dialog_available{true};
    bool clipboard_available{true};
    bool reviewed_process_runner_available{true};
    bool user_confirmation_required_for_process_execution{true};
    std::uint32_t file_dialog_requests_routed{0};
    std::uint32_t clipboard_operations_routed{0};
    std::uint32_t reviewed_process_plans{0};
    std::uint32_t reviewed_process_executions{0};
    std::string reviewed_process_status_label{"confirmation required"};
};

struct NativeEditorServiceBindings {
    IFileDialogService* file_dialog_service{nullptr};
    ui::IClipboardTextAdapter* clipboard_text_adapter{nullptr};
    IProcessRunner* reviewed_process_runner{nullptr};
    std::string file_dialog_service_id;
    std::string clipboard_service_id;
    std::string reviewed_process_runner_id;
};

struct NativeEditorReviewedProcessRequest {
    EditorAiReviewedValidationExecutionModel plan;
    bool user_confirmed{false};
};

struct NativeEditorReviewedProcessResult {
    bool reviewed{false};
    bool user_confirmed{false};
    bool executed{false};
    ProcessResult process;
    std::string diagnostic;
};

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
    [[nodiscard]] std::string_view docking_status_last_frame() const noexcept;
    [[nodiscard]] std::uint32_t dock_tab_headers_last_frame() const noexcept;
    [[nodiscard]] std::uint32_t dock_split_gutters_last_frame() const noexcept;
    [[nodiscard]] std::uint32_t dock_active_panels_last_frame() const noexcept;
    [[nodiscard]] std::uint32_t dock_focusable_controls_last_frame() const noexcept;

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
    [[nodiscard]] const NativeEditorServiceStatus& services() const noexcept;
    [[nodiscard]] const ViewportState& viewport() const noexcept;
    [[nodiscard]] const NativeViewportDisplayPlan& viewport_display() const noexcept;
    [[nodiscard]] const EditorMaterialAssetPreviewPanelModel& material_preview() const noexcept;
    [[nodiscard]] const NativeMaterialPreviewDisplayPlan& material_preview_display() const noexcept;

    void bind_native_services(NativeEditorServiceBindings services);
    [[nodiscard]] FileDialogId show_file_dialog(FileDialogRequest request);
    [[nodiscard]] std::optional<FileDialogResult> poll_file_dialog_result(FileDialogId id);
    [[nodiscard]] ui::ClipboardTextWriteResult write_clipboard_text(ui::ClipboardTextWriteRequest request);
    [[nodiscard]] ui::ClipboardTextReadResult read_clipboard_text(ui::ClipboardTextReadRequest request);
    [[nodiscard]] EditorAiReviewedValidationExecutionModel
    reviewed_validation_execution_plan(const EditorAiReviewedValidationExecutionDesc& desc);
    [[nodiscard]] NativeEditorReviewedProcessResult run_reviewed_process(NativeEditorReviewedProcessRequest request);

    [[nodiscard]] int run();
    void record_native_frame() noexcept;
    void record_native_panels_rendered(std::uint32_t count) noexcept;
    void record_native_docking_frame(std::string status, std::uint32_t tab_header_count,
                                     std::uint32_t split_gutter_count, std::uint32_t active_panel_count,
                                     std::uint32_t focusable_control_count);
    void record_native_resource_device_ready(std::uint64_t frame_index);
    void record_native_viewport_d3d12_host_ready(std::uint64_t frame_index);
    void record_native_material_preview_d3d12_host_ready(std::uint64_t frame_index);

  private:
    NativeEditorLaunchOptions options_;
    std::uint32_t frames_recorded_{0};
    std::uint32_t panels_rendered_last_frame_{0};
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana::editor
