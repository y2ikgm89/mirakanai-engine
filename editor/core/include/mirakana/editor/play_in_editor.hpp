// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/platform/process.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorPlaySessionState {
    edit,
    play,
    paused,
};

enum class EditorPlaySessionActionStatus {
    applied,
    rejected_inactive,
    rejected_active,
    rejected_empty_scene,
    rejected_wrong_state,
    rejected_invalid_delta,
};

struct EditorPlaySessionTickContext {
    std::uint64_t frame_index{0};
    double delta_seconds{0.0};
};

class IEditorPlaySessionDriver {
  public:
    virtual ~IEditorPlaySessionDriver() = default;

    virtual void on_play_begin(Scene& scene);
    virtual void on_play_tick(Scene& scene, const EditorPlaySessionTickContext& context) = 0;
    virtual void on_play_end(Scene& scene);
};

struct EditorPlaySessionReport {
    EditorPlaySessionState state{EditorPlaySessionState::edit};
    bool active{false};
    bool source_scene_edits_blocked{false};
    bool gameplay_driver_attached{false};
    std::uint64_t simulation_frame_count{0};
    double last_delta_seconds{0.0};
    std::size_t source_node_count{0};
    std::size_t simulation_node_count{0};
    SceneNodeId selected_node{null_scene_node};
    std::string diagnostic;
};

enum class EditorPlaySessionControlCommand {
    play,
    pause,
    resume,
    stop,
};

struct EditorPlaySessionControlRow {
    EditorPlaySessionControlCommand command{EditorPlaySessionControlCommand::play};
    std::string_view id;
    std::string_view label;
    bool enabled{false};
};

struct EditorPlaySessionControlsModel {
    EditorPlaySessionReport report;
    std::vector<EditorPlaySessionControlRow> controls;
    bool viewport_uses_simulation_scene{false};
};

enum class EditorInProcessRuntimeHostStatus {
    ready,
    blocked,
    active,
};

struct EditorInProcessRuntimeHostDesc {
    std::string id;
    std::string label;
    bool linked_gameplay_driver_available{false};
    bool request_dynamic_game_module_loading{false};
    bool request_external_process_execution{false};
    bool request_editor_core_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_package_script_execution{false};
    bool request_validation_recipe_execution{false};
    bool request_hot_reload{false};
    bool request_renderer_rhi_uploads{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_package_streaming{false};
};

struct EditorInProcessRuntimeHostModel {
    std::string id;
    std::string label;
    EditorInProcessRuntimeHostStatus status{EditorInProcessRuntimeHostStatus::blocked};
    std::string status_label;
    bool can_begin{false};
    bool can_tick{false};
    bool can_stop{false};
    bool source_scene_available{false};
    bool linked_gameplay_driver_available{false};
    bool gameplay_driver_attached{false};
    bool viewport_uses_simulation_scene{false};
    bool has_blocking_diagnostics{false};
    std::vector<std::string> blocked_by;
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorInProcessRuntimeHostBeginResult {
    EditorInProcessRuntimeHostStatus status{EditorInProcessRuntimeHostStatus::blocked};
    EditorPlaySessionActionStatus action_status{EditorPlaySessionActionStatus::rejected_wrong_state};
    EditorInProcessRuntimeHostModel model;
    std::string diagnostic;
};

enum class EditorRuntimeHostPlaytestLaunchStatus {
    ready,
    blocked,
    host_gated,
};

struct EditorRuntimeHostPlaytestLaunchDesc {
    std::string id;
    std::string label;
    std::string working_directory;
    std::vector<std::string> argv;
    std::vector<std::string> host_gates;
    std::vector<std::string> acknowledged_host_gates;
    bool acknowledge_host_gates{false};
    bool request_dynamic_game_module_loading{false};
    bool request_editor_core_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_raw_manifest_command_evaluation{false};
    bool request_package_script_execution{false};
    bool request_free_form_manifest_edit{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_package_streaming{false};
};

struct EditorRuntimeHostPlaytestLaunchModel {
    std::string id;
    std::string label;
    EditorRuntimeHostPlaytestLaunchStatus status{EditorRuntimeHostPlaytestLaunchStatus::blocked};
    std::string status_label;
    bool can_execute{false};
    bool has_blocking_diagnostics{false};
    bool has_host_gates{false};
    bool host_gate_acknowledgement_required{false};
    bool host_gates_acknowledged{false};
    mirakana::ProcessCommand command;
    std::vector<std::string> host_gates;
    std::vector<std::string> acknowledged_host_gates;
    std::vector<std::string> blocked_by;
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

class EditorPlaySession {
  public:
    [[nodiscard]] EditorPlaySessionState state() const noexcept;
    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] bool source_scene_edits_blocked() const noexcept;
    [[nodiscard]] bool gameplay_driver_attached() const noexcept;
    [[nodiscard]] std::uint64_t simulation_frame_count() const noexcept;
    [[nodiscard]] double last_delta_seconds() const noexcept;
    [[nodiscard]] const Scene* simulation_scene() const noexcept;
    [[nodiscard]] Scene* mutable_simulation_scene() noexcept;

    [[nodiscard]] EditorPlaySessionActionStatus begin(const SceneAuthoringDocument& source_document);
    [[nodiscard]] EditorPlaySessionActionStatus begin(const SceneAuthoringDocument& source_document,
                                                      IEditorPlaySessionDriver& driver);
    [[nodiscard]] EditorPlaySessionActionStatus pause() noexcept;
    [[nodiscard]] EditorPlaySessionActionStatus resume() noexcept;
    [[nodiscard]] EditorPlaySessionActionStatus tick(double delta_seconds = 1.0 / 60.0);
    [[nodiscard]] EditorPlaySessionActionStatus stop();

  private:
    [[nodiscard]] EditorPlaySessionActionStatus begin_impl(const SceneAuthoringDocument& source_document,
                                                           IEditorPlaySessionDriver* driver);

    EditorPlaySessionState state_{EditorPlaySessionState::edit};
    std::optional<Scene> simulation_scene_;
    IEditorPlaySessionDriver* gameplay_driver_{nullptr};
    std::uint64_t simulation_frame_count_{0};
    double last_delta_seconds_{0.0};
};

[[nodiscard]] EditorPlaySessionReport make_editor_play_session_report(const EditorPlaySession& session,
                                                                      const SceneAuthoringDocument& source_document);

[[nodiscard]] EditorPlaySessionControlsModel
make_editor_play_session_controls_model(const EditorPlaySession& session,
                                        const SceneAuthoringDocument& source_document);

[[nodiscard]] std::string_view editor_play_session_state_label(EditorPlaySessionState state) noexcept;
[[nodiscard]] std::string_view editor_play_session_action_status_label(EditorPlaySessionActionStatus status) noexcept;
[[nodiscard]] std::string_view editor_play_session_control_command_id(EditorPlaySessionControlCommand command) noexcept;
[[nodiscard]] std::string_view
editor_play_session_control_command_label(EditorPlaySessionControlCommand command) noexcept;
[[nodiscard]] std::string_view
editor_in_process_runtime_host_status_label(EditorInProcessRuntimeHostStatus status) noexcept;
[[nodiscard]] EditorInProcessRuntimeHostModel
make_editor_in_process_runtime_host_model(const EditorPlaySession& session,
                                          const SceneAuthoringDocument& source_document,
                                          const EditorInProcessRuntimeHostDesc& desc);
[[nodiscard]] EditorInProcessRuntimeHostBeginResult
begin_editor_in_process_runtime_host_session(EditorPlaySession& session, const SceneAuthoringDocument& source_document,
                                             IEditorPlaySessionDriver& driver,
                                             const EditorInProcessRuntimeHostDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_in_process_runtime_host_ui_model(const EditorInProcessRuntimeHostModel& model);
[[nodiscard]] std::string_view
editor_runtime_host_playtest_launch_status_label(EditorRuntimeHostPlaytestLaunchStatus status) noexcept;
[[nodiscard]] EditorRuntimeHostPlaytestLaunchModel
make_editor_runtime_host_playtest_launch_model(const EditorRuntimeHostPlaytestLaunchDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_runtime_host_playtest_launch_ui_model(const EditorRuntimeHostPlaytestLaunchModel& model);

} // namespace mirakana::editor
