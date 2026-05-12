// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/play_in_editor.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string sanitize_text(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        if (character == '\n' || character == '\r' || character == '=') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text.empty() ? "-" : text;
}

[[nodiscard]] std::string sanitize_element_id(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::isalnum(byte) != 0 || character == '_' || character == '-' || character == '.') {
            text.push_back(character);
        } else {
            text.push_back('_');
        }
    }
    return text.empty() ? "runtime_host" : text;
}

[[nodiscard]] bool safe_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool contains_string(const std::vector<std::string>& values, std::string_view expected) {
    return std::ranges::find(values, expected) != values.end();
}

void append_unique(std::vector<std::string>& values, std::string value) {
    if (value.empty()) {
        return;
    }
    if (std::ranges::find(values, value) == values.end()) {
        values.push_back(std::move(value));
    }
}

void append_blocker(EditorRuntimeHostPlaytestLaunchModel& model, std::string blocker, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.blocked_by, std::move(blocker));
    model.diagnostics.push_back(std::move(diagnostic));
}

void append_blocker(EditorInProcessRuntimeHostModel& model, std::string blocker, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.blocked_by, std::move(blocker));
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorRuntimeHostPlaytestLaunchModel& model, std::string claim, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.unsupported_claims, std::move(claim));
    append_unique(model.blocked_by, "unsupported-runtime-host-launch-claim");
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorInProcessRuntimeHostModel& model, std::string claim, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.unsupported_claims, std::move(claim));
    append_unique(model.blocked_by, "unsupported-in-process-runtime-host-claim");
    model.diagnostics.push_back(std::move(diagnostic));
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("editor runtime host playtest launch ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

[[nodiscard]] mirakana::ui::ElementDesc make_root(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_child(std::string id, mirakana::ui::ElementId parent,
                                                   mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.parent = std::move(parent);
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

[[nodiscard]] std::string make_process_command_display(const mirakana::ProcessCommand& command) {
    if (command.executable.empty()) {
        return "-";
    }
    std::string text = command.executable;
    for (const auto& argument : command.arguments) {
        text += " ";
        text += argument;
    }
    return text;
}

[[nodiscard]] std::string join_values(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "-";
    }
    std::string text;
    for (const auto& value : values) {
        if (!text.empty()) {
            text += ", ";
        }
        text += value;
    }
    return text;
}

} // namespace

void IEditorPlaySessionDriver::on_play_begin(Scene& /*unused*/) {}

void IEditorPlaySessionDriver::on_play_end(Scene& /*unused*/) {}

EditorPlaySessionState EditorPlaySession::state() const noexcept {
    return state_;
}

bool EditorPlaySession::active() const noexcept {
    return state_ != EditorPlaySessionState::edit && simulation_scene_.has_value();
}

bool EditorPlaySession::source_scene_edits_blocked() const noexcept {
    return active();
}

bool EditorPlaySession::gameplay_driver_attached() const noexcept {
    return active() && gameplay_driver_ != nullptr;
}

std::uint64_t EditorPlaySession::simulation_frame_count() const noexcept {
    return simulation_frame_count_;
}

double EditorPlaySession::last_delta_seconds() const noexcept {
    return last_delta_seconds_;
}

const Scene* EditorPlaySession::simulation_scene() const noexcept {
    return simulation_scene_ ? &*simulation_scene_ : nullptr;
}

Scene* EditorPlaySession::mutable_simulation_scene() noexcept {
    return simulation_scene_ ? &*simulation_scene_ : nullptr;
}

EditorPlaySessionActionStatus EditorPlaySession::begin(const SceneAuthoringDocument& source_document) {
    return begin_impl(source_document, nullptr);
}

EditorPlaySessionActionStatus EditorPlaySession::begin(const SceneAuthoringDocument& source_document,
                                                       IEditorPlaySessionDriver& driver) {
    return begin_impl(source_document, &driver);
}

EditorPlaySessionActionStatus EditorPlaySession::begin_impl(const SceneAuthoringDocument& source_document,
                                                            IEditorPlaySessionDriver* driver) {
    if (active()) {
        return EditorPlaySessionActionStatus::rejected_active;
    }
    if (source_document.scene().nodes().empty()) {
        return EditorPlaySessionActionStatus::rejected_empty_scene;
    }

    auto simulation_scene = source_document.scene();
    if (driver != nullptr) {
        driver->on_play_begin(simulation_scene);
    }

    simulation_scene_ = std::move(simulation_scene);
    gameplay_driver_ = driver;
    simulation_frame_count_ = 0;
    last_delta_seconds_ = 0.0;
    state_ = EditorPlaySessionState::play;
    return EditorPlaySessionActionStatus::applied;
}

EditorPlaySessionActionStatus EditorPlaySession::pause() noexcept {
    if (!active()) {
        return EditorPlaySessionActionStatus::rejected_inactive;
    }
    if (state_ != EditorPlaySessionState::play) {
        return EditorPlaySessionActionStatus::rejected_wrong_state;
    }

    state_ = EditorPlaySessionState::paused;
    return EditorPlaySessionActionStatus::applied;
}

EditorPlaySessionActionStatus EditorPlaySession::resume() noexcept {
    if (!active()) {
        return EditorPlaySessionActionStatus::rejected_inactive;
    }
    if (state_ != EditorPlaySessionState::paused) {
        return EditorPlaySessionActionStatus::rejected_wrong_state;
    }

    state_ = EditorPlaySessionState::play;
    return EditorPlaySessionActionStatus::applied;
}

EditorPlaySessionActionStatus EditorPlaySession::tick(double delta_seconds) {
    if (!active()) {
        return EditorPlaySessionActionStatus::rejected_inactive;
    }
    if (state_ != EditorPlaySessionState::play) {
        return EditorPlaySessionActionStatus::rejected_wrong_state;
    }
    if (!std::isfinite(delta_seconds) || delta_seconds <= 0.0) {
        return EditorPlaySessionActionStatus::rejected_invalid_delta;
    }

    if (gameplay_driver_ != nullptr) {
        gameplay_driver_->on_play_tick(
            *simulation_scene_,
            EditorPlaySessionTickContext{.frame_index = simulation_frame_count_, .delta_seconds = delta_seconds});
    }
    ++simulation_frame_count_;
    last_delta_seconds_ = delta_seconds;
    return EditorPlaySessionActionStatus::applied;
}

EditorPlaySessionActionStatus EditorPlaySession::stop() {
    if (!active()) {
        return EditorPlaySessionActionStatus::rejected_inactive;
    }

    if (gameplay_driver_ != nullptr && simulation_scene_.has_value()) {
        gameplay_driver_->on_play_end(*simulation_scene_);
    }

    simulation_scene_.reset();
    gameplay_driver_ = nullptr;
    simulation_frame_count_ = 0;
    last_delta_seconds_ = 0.0;
    state_ = EditorPlaySessionState::edit;
    return EditorPlaySessionActionStatus::applied;
}

EditorPlaySessionReport make_editor_play_session_report(const EditorPlaySession& session,
                                                        const SceneAuthoringDocument& source_document) {
    const auto* simulation = session.simulation_scene();
    EditorPlaySessionReport report;
    report.state = session.state();
    report.active = session.active();
    report.source_scene_edits_blocked = session.source_scene_edits_blocked();
    report.gameplay_driver_attached = session.gameplay_driver_attached();
    report.simulation_frame_count = session.simulation_frame_count();
    report.last_delta_seconds = session.last_delta_seconds();
    report.source_node_count = source_document.scene().nodes().size();
    report.simulation_node_count = simulation == nullptr ? 0U : simulation->nodes().size();
    report.selected_node = source_document.selected_node();
    if (!report.active) {
        report.diagnostic = "play session is inactive";
    } else if (report.gameplay_driver_attached) {
        report.diagnostic = "play session is using an isolated simulation scene with a gameplay driver";
    } else {
        report.diagnostic = "play session is using an isolated simulation scene";
    }
    return report;
}

EditorPlaySessionControlsModel make_editor_play_session_controls_model(const EditorPlaySession& session,
                                                                       const SceneAuthoringDocument& source_document) {
    const auto report = make_editor_play_session_report(session, source_document);
    const auto state = report.state;
    const bool source_has_scene = !source_document.scene().nodes().empty();

    EditorPlaySessionControlsModel model;
    model.report = report;
    model.viewport_uses_simulation_scene = report.active && session.simulation_scene() != nullptr;
    model.controls = {
        EditorPlaySessionControlRow{
            .command = EditorPlaySessionControlCommand::play,
            .id = editor_play_session_control_command_id(EditorPlaySessionControlCommand::play),
            .label = editor_play_session_control_command_label(EditorPlaySessionControlCommand::play),
            .enabled = !report.active && source_has_scene,
        },
        EditorPlaySessionControlRow{
            .command = EditorPlaySessionControlCommand::pause,
            .id = editor_play_session_control_command_id(EditorPlaySessionControlCommand::pause),
            .label = editor_play_session_control_command_label(EditorPlaySessionControlCommand::pause),
            .enabled = report.active && state == EditorPlaySessionState::play,
        },
        EditorPlaySessionControlRow{
            .command = EditorPlaySessionControlCommand::resume,
            .id = editor_play_session_control_command_id(EditorPlaySessionControlCommand::resume),
            .label = editor_play_session_control_command_label(EditorPlaySessionControlCommand::resume),
            .enabled = report.active && state == EditorPlaySessionState::paused,
        },
        EditorPlaySessionControlRow{
            .command = EditorPlaySessionControlCommand::stop,
            .id = editor_play_session_control_command_id(EditorPlaySessionControlCommand::stop),
            .label = editor_play_session_control_command_label(EditorPlaySessionControlCommand::stop),
            .enabled = report.active,
        },
    };
    return model;
}

std::string_view editor_play_session_state_label(EditorPlaySessionState state) noexcept {
    switch (state) {
    case EditorPlaySessionState::edit:
        return "Edit";
    case EditorPlaySessionState::play:
        return "Play";
    case EditorPlaySessionState::paused:
        return "Paused";
    }
    return "Unknown";
}

std::string_view editor_play_session_control_command_id(EditorPlaySessionControlCommand command) noexcept {
    switch (command) {
    case EditorPlaySessionControlCommand::play:
        return "play";
    case EditorPlaySessionControlCommand::pause:
        return "pause";
    case EditorPlaySessionControlCommand::resume:
        return "resume";
    case EditorPlaySessionControlCommand::stop:
        return "stop";
    }
    return "unknown";
}

std::string_view editor_play_session_control_command_label(EditorPlaySessionControlCommand command) noexcept {
    switch (command) {
    case EditorPlaySessionControlCommand::play:
        return "Play";
    case EditorPlaySessionControlCommand::pause:
        return "Pause";
    case EditorPlaySessionControlCommand::resume:
        return "Resume";
    case EditorPlaySessionControlCommand::stop:
        return "Stop";
    }
    return "Unknown";
}

std::string_view editor_play_session_action_status_label(EditorPlaySessionActionStatus status) noexcept {
    switch (status) {
    case EditorPlaySessionActionStatus::applied:
        return "Applied";
    case EditorPlaySessionActionStatus::rejected_inactive:
        return "RejectedInactive";
    case EditorPlaySessionActionStatus::rejected_active:
        return "RejectedActive";
    case EditorPlaySessionActionStatus::rejected_empty_scene:
        return "RejectedEmptyScene";
    case EditorPlaySessionActionStatus::rejected_wrong_state:
        return "RejectedWrongState";
    case EditorPlaySessionActionStatus::rejected_invalid_delta:
        return "RejectedInvalidDelta";
    }
    return "Unknown";
}

std::string_view editor_in_process_runtime_host_status_label(EditorInProcessRuntimeHostStatus status) noexcept {
    switch (status) {
    case EditorInProcessRuntimeHostStatus::ready:
        return "ready";
    case EditorInProcessRuntimeHostStatus::blocked:
        return "blocked";
    case EditorInProcessRuntimeHostStatus::active:
        return "active";
    }
    return "unknown";
}

EditorInProcessRuntimeHostModel make_editor_in_process_runtime_host_model(const EditorPlaySession& session,
                                                                          const SceneAuthoringDocument& source_document,
                                                                          const EditorInProcessRuntimeHostDesc& desc) {
    EditorInProcessRuntimeHostModel model;
    model.id = sanitize_element_id(desc.id);
    model.label = desc.label.empty() ? model.id : sanitize_text(desc.label);
    model.source_scene_available = !source_document.scene().nodes().empty();
    model.linked_gameplay_driver_available = desc.linked_gameplay_driver_available;
    model.gameplay_driver_attached = session.gameplay_driver_attached();
    model.viewport_uses_simulation_scene = session.active() && session.simulation_scene() != nullptr;

    if (desc.request_dynamic_game_module_loading) {
        reject_unsupported_claim(model, "dynamic game module loading",
                                 "in-process runtime host review requires an already-linked gameplay driver and does "
                                 "not load dynamic game modules");
    }
    if (desc.request_external_process_execution) {
        reject_unsupported_claim(model, "external process execution",
                                 "in-process runtime host review does not execute external runtime-host processes");
    }
    if (desc.request_editor_core_execution) {
        reject_unsupported_claim(model, "editor core execution",
                                 "editor core reviews in-process runtime host readiness but must not execute hosts");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell",
                                 "in-process runtime host review rejects arbitrary shell execution");
    }
    if (desc.request_package_script_execution) {
        reject_unsupported_claim(model, "package script execution",
                                 "in-process runtime host review rejects package script execution");
    }
    if (desc.request_validation_recipe_execution) {
        reject_unsupported_claim(model, "validation recipe execution",
                                 "in-process runtime host review does not run validation recipes");
    }
    if (desc.request_hot_reload) {
        reject_unsupported_claim(model, "hot reload",
                                 "in-process runtime host review does not hot reload game code or modules");
    }
    if (desc.request_renderer_rhi_uploads) {
        reject_unsupported_claim(
            model, "renderer/RHI uploads",
            "in-process runtime host review does not create renderer/RHI uploads from editor core");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "in-process runtime host review must not expose renderer/RHI handles");
    }
    if (desc.request_package_streaming) {
        reject_unsupported_claim(model, "package streaming",
                                 "in-process runtime host review does not make package streaming ready");
    }

    if (!model.source_scene_available) {
        append_blocker(model, "missing-source-scene",
                       "in-process runtime host review requires a non-empty source scene");
    }
    if (!model.linked_gameplay_driver_available && !session.gameplay_driver_attached()) {
        append_blocker(model, "missing-linked-gameplay-driver",
                       "in-process runtime host review requires an already-linked gameplay driver");
    }

    if (session.active()) {
        model.status = EditorInProcessRuntimeHostStatus::active;
        model.can_tick = session.state() == EditorPlaySessionState::play && session.gameplay_driver_attached();
        model.can_stop = true;
        if (!model.gameplay_driver_attached) {
            append_blocker(model, "missing-active-gameplay-driver",
                           "active play session is not using a linked gameplay driver");
        }
    } else if (model.has_blocking_diagnostics) {
        model.status = EditorInProcessRuntimeHostStatus::blocked;
    } else {
        model.status = EditorInProcessRuntimeHostStatus::ready;
        model.can_begin = true;
    }

    if (model.status == EditorInProcessRuntimeHostStatus::active && !model.gameplay_driver_attached) {
        model.status = EditorInProcessRuntimeHostStatus::blocked;
        model.can_tick = false;
        model.can_stop = session.active();
    }

    if (model.status == EditorInProcessRuntimeHostStatus::ready) {
        model.diagnostics.emplace_back("in-process runtime host can begin with linked gameplay driver");
    } else if (model.status == EditorInProcessRuntimeHostStatus::active) {
        model.diagnostics.emplace_back("in-process runtime host is active on isolated simulation scene");
    }

    model.status_label = std::string(editor_in_process_runtime_host_status_label(model.status));
    return model;
}

EditorInProcessRuntimeHostBeginResult
begin_editor_in_process_runtime_host_session(EditorPlaySession& session, const SceneAuthoringDocument& source_document,
                                             IEditorPlaySessionDriver& driver,
                                             const EditorInProcessRuntimeHostDesc& desc) {
    EditorInProcessRuntimeHostBeginResult result;
    auto ready_model = make_editor_in_process_runtime_host_model(session, source_document, desc);
    if (!ready_model.can_begin) {
        result.status = ready_model.status;
        result.action_status = session.active() ? EditorPlaySessionActionStatus::rejected_active
                                                : EditorPlaySessionActionStatus::rejected_wrong_state;
        result.diagnostic = "in-process runtime host review is not ready";
        result.model = std::move(ready_model);
        return result;
    }

    result.action_status = session.begin(source_document, driver);
    result.model = make_editor_in_process_runtime_host_model(session, source_document, desc);
    result.status = result.model.status;
    result.diagnostic = result.action_status == EditorPlaySessionActionStatus::applied
                            ? "in-process runtime host session started"
                            : "in-process runtime host session failed to start";
    return result;
}

mirakana::ui::UiDocument make_editor_in_process_runtime_host_ui_model(const EditorInProcessRuntimeHostModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("play_in_editor.in_process_runtime_host", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"play_in_editor.in_process_runtime_host"};

    const std::string row_id = "play_in_editor.in_process_runtime_host." + sanitize_element_id(model.id);
    mirakana::ui::ElementDesc row = make_child(row_id, root, mirakana::ui::SemanticRole::list_item);
    row.text = make_text(model.label);
    row.enabled = model.can_begin || model.can_tick || model.can_stop;
    add_or_throw(document, std::move(row));

    const mirakana::ui::ElementId row_element{row_id};
    append_label(document, row_element, row_id + ".status", model.status_label);
    append_label(document, row_element, row_id + ".driver",
                 model.linked_gameplay_driver_available ? "linked" : "missing");
    append_label(document, row_element, row_id + ".source_scene",
                 model.source_scene_available ? "available" : "missing");
    append_label(document, row_element, row_id + ".simulation",
                 model.viewport_uses_simulation_scene ? "active" : "inactive");
    append_label(document, row_element, row_id + ".blocked_by", join_values(model.blocked_by));
    append_label(document, row_element, row_id + ".unsupported_claims", join_values(model.unsupported_claims));
    append_label(document, row_element, row_id + ".diagnostic", join_values(model.diagnostics));
    return document;
}

std::string_view
editor_runtime_host_playtest_launch_status_label(EditorRuntimeHostPlaytestLaunchStatus status) noexcept {
    switch (status) {
    case EditorRuntimeHostPlaytestLaunchStatus::ready:
        return "ready";
    case EditorRuntimeHostPlaytestLaunchStatus::blocked:
        return "blocked";
    case EditorRuntimeHostPlaytestLaunchStatus::host_gated:
        return "host_gated";
    }
    return "unknown";
}

EditorRuntimeHostPlaytestLaunchModel
make_editor_runtime_host_playtest_launch_model(const EditorRuntimeHostPlaytestLaunchDesc& desc) {
    EditorRuntimeHostPlaytestLaunchModel model;
    model.id = sanitize_element_id(desc.id);
    model.label = desc.label.empty() ? model.id : sanitize_text(desc.label);
    model.host_gates = desc.host_gates;

    if (desc.request_dynamic_game_module_loading) {
        reject_unsupported_claim(model, "dynamic game module loading",
                                 "runtime host playtest launch does not load dynamic game modules into the editor");
    }
    if (desc.request_editor_core_execution) {
        reject_unsupported_claim(model, "editor core execution",
                                 "editor core reviews runtime host launch commands but must not execute processes");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell",
                                 "runtime host playtest launch rejects arbitrary shell execution");
    }
    if (desc.request_raw_manifest_command_evaluation) {
        reject_unsupported_claim(model, "raw manifest command evaluation",
                                 "runtime host playtest launch accepts reviewed argv tokens, not raw manifest text");
    }
    if (desc.request_package_script_execution) {
        reject_unsupported_claim(model, "package script execution",
                                 "runtime host playtest launch rejects broad package script execution");
    }
    if (desc.request_free_form_manifest_edit) {
        reject_unsupported_claim(model, "free-form manifest edits",
                                 "runtime host playtest launch must not edit manifests");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handle exposure",
                                 "runtime host playtest launch must not expose renderer/RHI handles");
    }
    if (desc.request_package_streaming) {
        reject_unsupported_claim(model, "package streaming",
                                 "runtime host playtest launch does not make package streaming ready");
    }

    if (desc.argv.empty()) {
        append_blocker(model, "missing-reviewed-argv", "runtime host playtest launch requires reviewed argv tokens");
    }
    for (const auto& token : desc.argv) {
        if (!safe_token(token)) {
            append_blocker(model, "unsafe-argv-token",
                           "runtime host playtest launch rejects empty or control-character argv tokens");
            break;
        }
    }
    if (!desc.working_directory.empty() && (desc.working_directory.find('\n') != std::string::npos ||
                                            desc.working_directory.find('\r') != std::string::npos ||
                                            desc.working_directory.find('\0') != std::string::npos)) {
        append_blocker(model, "unsafe-working-directory",
                       "runtime host playtest launch rejects unsafe working directory text");
    }

    if (!desc.host_gates.empty()) {
        model.has_host_gates = true;
        model.host_gate_acknowledgement_required = true;
        if (!desc.acknowledge_host_gates) {
            model.diagnostics.emplace_back("runtime host playtest launch requires host-gate acknowledgement");
        } else {
            for (const auto& host_gate : desc.host_gates) {
                if (!safe_token(host_gate)) {
                    append_blocker(model, "unsafe-host-gate",
                                   "runtime host playtest launch rejects unsafe host-gate tokens");
                } else if (!contains_string(desc.acknowledged_host_gates, host_gate)) {
                    append_blocker(model, "missing-host-gate-acknowledgement",
                                   "missing host-gate acknowledgement: " + host_gate);
                }
            }
            for (const auto& acknowledged_host_gate : desc.acknowledged_host_gates) {
                if (!safe_token(acknowledged_host_gate)) {
                    append_blocker(model, "unsafe-host-gate-acknowledgement",
                                   "runtime host playtest launch rejects unsafe host-gate acknowledgement tokens");
                } else if (!contains_string(desc.host_gates, acknowledged_host_gate)) {
                    append_blocker(model, "unknown-host-gate-acknowledgement",
                                   "unknown host-gate acknowledgement: " + acknowledged_host_gate);
                }
            }
            if (!model.has_blocking_diagnostics) {
                model.host_gates_acknowledged = true;
                model.acknowledged_host_gates = desc.host_gates;
            }
        }
    }

    if (!model.has_blocking_diagnostics &&
        (!model.host_gate_acknowledgement_required || model.host_gates_acknowledged)) {
        std::vector<std::string> arguments;
        if (desc.argv.size() > 1U) {
            arguments.assign(desc.argv.begin() + 1, desc.argv.end());
        }
        mirakana::ProcessCommand command{
            .executable = desc.argv[0],
            .arguments = std::move(arguments),
            .working_directory = desc.working_directory,
        };
        if (!mirakana::is_safe_process_command(command)) {
            append_blocker(model, "unsafe-process-command",
                           "reviewed runtime host argv produced an unsafe process command");
        } else {
            model.command = std::move(command);
        }
    }

    if (model.has_blocking_diagnostics) {
        model.status = EditorRuntimeHostPlaytestLaunchStatus::blocked;
    } else if (model.host_gate_acknowledgement_required && !model.host_gates_acknowledged) {
        model.status = EditorRuntimeHostPlaytestLaunchStatus::host_gated;
    } else {
        model.status = EditorRuntimeHostPlaytestLaunchStatus::ready;
        model.can_execute = true;
    }
    model.status_label = std::string(editor_runtime_host_playtest_launch_status_label(model.status));
    return model;
}

mirakana::ui::UiDocument
make_editor_runtime_host_playtest_launch_ui_model(const EditorRuntimeHostPlaytestLaunchModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("play_in_editor.runtime_host", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"play_in_editor.runtime_host"};

    const std::string row_id = "play_in_editor.runtime_host." + sanitize_element_id(model.id);
    mirakana::ui::ElementDesc row = make_child(row_id, root, mirakana::ui::SemanticRole::list_item);
    row.text = make_text(model.label);
    row.enabled = model.can_execute;
    add_or_throw(document, std::move(row));

    const mirakana::ui::ElementId row_element{row_id};
    append_label(document, row_element, row_id + ".status", model.status_label);
    append_label(document, row_element, row_id + ".command", make_process_command_display(model.command));
    append_label(document, row_element, row_id + ".host_gates", join_values(model.host_gates));
    append_label(document, row_element, row_id + ".acknowledged_host_gates",
                 join_values(model.acknowledged_host_gates));
    append_label(document, row_element, row_id + ".blocked_by", join_values(model.blocked_by));
    append_label(document, row_element, row_id + ".unsupported_claims", join_values(model.unsupported_claims));
    append_label(document, row_element, row_id + ".diagnostic", join_values(model.diagnostics));
    return document;
}

} // namespace mirakana::editor
