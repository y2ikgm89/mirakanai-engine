// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "native_asset_import_copy.hpp"
#include "native_editor_launch.hpp"
#include "native_editor_text_atlas_handoff.hpp"
#include "native_editor_text_input.hpp"
#include "native_editor_uia_provider.hpp"
#include "native_material_preview_cache.hpp"
#include "native_viewport_surface.hpp"

#include "mirakana/editor/ai_command_panel.hpp"
#include "mirakana/editor/asset_browser_production.hpp"
#include "mirakana/editor/asset_import_jobs.hpp"
#include "mirakana/editor/asset_import_review.hpp"
#include "mirakana/editor/content_browser_import_panel.hpp"
#include "mirakana/editor/environment_authoring.hpp"
#include "mirakana/editor/material_asset_preview_panel.hpp"
#include "mirakana/editor/profiler.hpp"
#include "mirakana/editor/project.hpp"
#include "mirakana/editor/resource_panel.hpp"
#include "mirakana/editor/runtime_ui_authoring.hpp"
#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/editor/ui_model.hpp"
#include "mirakana/editor/workspace.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/platform/filesystem.hpp"
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
    std::string platform_text_input_service_id{"memory"};
    std::string ime_service_id{"memory"};
    std::string accessibility_service_id{"memory"};
    std::string asset_import_filesystem_id{"unbound"};
    bool file_dialog_available{true};
    bool clipboard_available{true};
    bool reviewed_process_runner_available{true};
    bool platform_text_input_available{true};
    bool ime_available{true};
    bool accessibility_available{false};
    bool asset_import_filesystem_available{false};
    bool user_confirmation_required_for_process_execution{true};
    std::uint32_t file_dialog_requests_routed{0};
    std::uint32_t clipboard_operations_routed{0};
    std::uint32_t reviewed_process_plans{0};
    std::uint32_t reviewed_process_executions{0};
    std::uint32_t platform_text_input_sessions_started{0};
    std::uint32_t platform_text_input_sessions_ended{0};
    std::uint32_t ime_composition_updates{0};
    std::uint32_t committed_text_inputs{0};
    std::uint32_t accessibility_publish_requests{0};
    std::uint32_t asset_import_executions{0};
    std::string reviewed_process_status_label{"confirmation required"};
};

struct NativeEditorServiceBindings {
    IFileDialogService* file_dialog_service{nullptr};
    ui::IClipboardTextAdapter* clipboard_text_adapter{nullptr};
    IProcessRunner* reviewed_process_runner{nullptr};
    ui::IPlatformIntegrationAdapter* platform_text_input_adapter{nullptr};
    ui::IImeAdapter* ime_adapter{nullptr};
    ui::IAccessibilityAdapter* accessibility_adapter{nullptr};
    IFileSystem* asset_import_filesystem{nullptr};
    std::string file_dialog_service_id;
    std::string clipboard_service_id;
    std::string reviewed_process_runner_id;
    std::string platform_text_input_service_id;
    std::string ime_service_id;
    std::string accessibility_service_id;
    std::string asset_import_filesystem_id;
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

struct NativeEditorEnvironmentArtistWorkflowCommandPlanRow {
    std::string command_id;
    std::string label;
    EnvironmentArtistWorkflowCommandPlan dry_run;
    EnvironmentArtistWorkflowCommandPlan apply;
};

struct NativeEditorAssetBrowserImportSourcesDialogReview {
    EditorContentBrowserImportOpenDialogModel dialog;
    std::vector<std::string> accepted_project_paths;
    std::vector<std::string> diagnostics;
    bool accepted{false};
};

struct NativeEditorAssetBrowserExternalSourceCopyRequest {
    std::vector<std::string> source_paths;
    std::vector<std::string> existing_source_paths;
    std::vector<std::string> existing_project_paths;
};

struct NativeEditorAssetBrowserExternalSourceCopyReview {
    EditorContentBrowserImportExternalSourceCopyModel copy;
    std::vector<std::string> diagnostics;
};

struct NativeEditorAssetBrowserSourceRegistrationRequest {
    std::uint64_t expected_generation{0};
    std::vector<std::string> project_source_paths;
    std::vector<EditorAssetBrowserLegalProvenanceRow> provenance_rows;
    bool user_confirmed{false};
};

struct NativeEditorAssetBrowserSourceRegistrationResult {
    EditorAssetBrowserCommandPlan command;
    EditorAssetImportReviewModel review;
    bool applied{false};
    std::size_t registered_count{0};
    std::vector<std::string> diagnostics;
};

struct NativeEditorAssetBrowserExternalSourceCopyExecutionRequest {
    std::uint64_t expected_generation{0};
    std::vector<std::string> absolute_source_paths;
    std::vector<EditorAssetBrowserLegalProvenanceRow> provenance_rows;
    bool user_confirmed{false};
};

struct NativeEditorAssetBrowserExternalSourceCopyExecutionResult {
    EditorAssetBrowserCommandPlan command;
    NativeEditorAssetBrowserExternalSourceCopyReview review;
    NativeAssetImportExternalCopyResult copy;
    NativeEditorAssetBrowserSourceRegistrationResult source_registration;
    std::string job_id;
    std::vector<std::string> target_project_paths;
    std::vector<std::string> diagnostics;
    std::size_t copied_count{0};
    bool copied{false};
    bool registered_sources{false};
    bool succeeded{false};
};

struct NativeEditorAssetBrowserImportExecutionRequest {
    std::uint64_t expected_generation{0};
    bool user_confirmed{false};
};

struct NativeEditorAssetBrowserImportExecutionResult {
    EditorAssetBrowserCommandPlan command;
    std::string job_id;
    bool executed{false};
    bool import_tools_invoked{false};
    std::size_t imported_count{0};
    std::size_t import_failure_count{0};
    std::size_t registered_imported_count{0};
    std::string diagnostic;
    bool browser_refreshed{false};
};

struct NativeEditorAssetImportJobCommandRequest {
    std::uint64_t expected_generation{0};
    std::string job_id;
    bool user_confirmed{false};
};

struct NativeEditorAssetImportJobCommandResult {
    EditorAssetImportJobCommandResult command;
    std::vector<std::string> diagnostics;
    bool applied{false};
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
    [[nodiscard]] const EnvironmentAuthoringDocument& environment_authoring_document() const noexcept;
    [[nodiscard]] std::span<const EnvironmentAuthoringInspectorRow>
    environment_authoring_inspector_rows() const noexcept;
    [[nodiscard]] std::span<const EditorPropertyRow> inspector_rows() const noexcept;
    [[nodiscard]] const EditorAssetBrowserProductionModel& asset_browser() const noexcept;
    [[nodiscard]] std::span<const EditorAssetBrowserCommandPlan> asset_browser_command_plans() const noexcept;
    [[nodiscard]] const EditorAssetImportJobSnapshot& asset_import_jobs() const noexcept;
    [[nodiscard]] std::span<const EditorDiagnosticRow> console_rows() const noexcept;
    [[nodiscard]] const EditorResourcePanelModel& resources() const noexcept;
    [[nodiscard]] const EditorAiCommandPanelModel& ai_commands() const noexcept;
    [[nodiscard]] const EditorRuntimeUiDocumentModel& runtime_ui_document() const noexcept;
    [[nodiscard]] const EditorRuntimeUiThemeModel& runtime_ui_theme() const noexcept;
    [[nodiscard]] const EditorRuntimeUiAuthoringModel& runtime_ui_authoring() const noexcept;
    [[nodiscard]] std::span<const NativeEditorEnvironmentArtistWorkflowCommandPlanRow>
    environment_artist_workflow_command_plans() const noexcept;
    [[nodiscard]] const EnvironmentArtistWorkflowExecutionReviewModel&
    environment_artist_workflow_execution_review() const noexcept;
    [[nodiscard]] const EditorProfilerPanelModel& profiler() const noexcept;
    [[nodiscard]] const EditorTimelinePanelModel& timeline() const noexcept;
    [[nodiscard]] std::vector<ProjectSettingsError> project_settings_errors() const;
    [[nodiscard]] const NativeEditorServiceStatus& services() const noexcept;
    [[nodiscard]] const ViewportState& viewport() const noexcept;
    [[nodiscard]] const NativeViewportDisplayPlan& viewport_display() const noexcept;
    [[nodiscard]] const NativeEditorTextAtlasHandoffEvidence& text_atlas_handoff_evidence() const noexcept;
    [[nodiscard]] const NativeEditorTextInputState& text_input_state() const noexcept;
    [[nodiscard]] const NativeEditorUiaProviderState& accessibility_state() const noexcept;
    [[nodiscard]] const EditorMaterialAssetPreviewPanelModel& material_preview() const noexcept;
    [[nodiscard]] const NativeMaterialPreviewDisplayPlan& material_preview_display() const noexcept;

    void bind_native_services(NativeEditorServiceBindings services);
    [[nodiscard]] NativeEditorTextInputFocusResult focus_text_input_target(NativeEditorTextInputTargetDesc target);
    [[nodiscard]] NativeEditorTextInputEndResult end_text_input_session();
    [[nodiscard]] NativeEditorImeCompositionResult update_ime_composition(ui::ImeComposition composition);
    [[nodiscard]] NativeEditorImeCompositionResult cancel_ime_composition();
    [[nodiscard]] NativeEditorTextInputCommitResult commit_text_input(ui::CommittedTextInput input);
    [[nodiscard]] ui::AccessibilityPublishResult
    publish_native_accessibility_payload(const ui::AccessibilityPayload& payload, const ui::ElementId& focused);
    [[nodiscard]] FileDialogId show_file_dialog(FileDialogRequest request);
    [[nodiscard]] std::optional<FileDialogResult> poll_file_dialog_result(FileDialogId id);
    [[nodiscard]] FileDialogId show_asset_browser_import_sources_dialog();
    [[nodiscard]] NativeEditorAssetBrowserImportSourcesDialogReview
    poll_asset_browser_import_sources_dialog(FileDialogId id);
    [[nodiscard]] NativeEditorAssetBrowserExternalSourceCopyReview
    review_asset_browser_external_source_copy(const NativeEditorAssetBrowserExternalSourceCopyRequest& request) const;
    [[nodiscard]] NativeEditorAssetBrowserExternalSourceCopyExecutionResult
    copy_reviewed_asset_browser_external_sources(NativeEditorAssetBrowserExternalSourceCopyExecutionRequest request);
    [[nodiscard]] NativeEditorAssetBrowserSourceRegistrationResult
    apply_reviewed_asset_browser_import_sources(NativeEditorAssetBrowserSourceRegistrationRequest request);
    [[nodiscard]] NativeEditorAssetBrowserImportExecutionResult
    execute_reviewed_asset_browser_import_plan(NativeEditorAssetBrowserImportExecutionRequest request);
    [[nodiscard]] NativeEditorAssetImportJobCommandResult
    cancel_asset_import_job(NativeEditorAssetImportJobCommandRequest request);
    [[nodiscard]] NativeEditorAssetImportJobCommandResult
    retry_asset_import_job(NativeEditorAssetImportJobCommandRequest request);
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
    void record_native_viewport_texture_display(NativeViewportDisplayPlan plan);
    void record_native_material_preview_texture_display(NativeMaterialPreviewDisplayPlan plan);
    void record_native_text_atlas_handoff_evidence(NativeEditorTextAtlasHandoffEvidence evidence);

  private:
    NativeEditorLaunchOptions options_;
    std::uint32_t frames_recorded_{0};
    std::uint32_t panels_rendered_last_frame_{0};
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana::editor
