// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/io.hpp"
#include "mirakana/runtime/session_services.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorInputRebindingProfileReviewStatus : std::uint8_t { ready, blocked };

struct EditorInputRebindingProfileReviewRow {
    std::string id;
    std::string surface;
    EditorInputRebindingProfileReviewStatus status{EditorInputRebindingProfileReviewStatus::blocked};
    std::optional<runtime::RuntimeInputRebindingDiagnosticCode> diagnostic_code;
    bool mutates{false};
    bool executes{false};
    std::string diagnostic;
};

struct EditorInputRebindingProfileReviewDesc {
    runtime::RuntimeInputActionMap base_actions;
    runtime::RuntimeInputRebindingProfile profile;
    bool request_mutation{false};
    bool request_execution{false};
    bool request_native_handle_exposure{false};
    bool request_sdl3_input_api{false};
    bool request_dear_imgui_or_editor_private_runtime_dependency{false};
    bool request_ui_focus_consumption{false};
    bool request_multiplayer_device_assignment{false};
    bool request_input_glyph_generation{false};
};

struct EditorInputRebindingProfileReviewModel {
    std::vector<EditorInputRebindingProfileReviewRow> rows;
    bool ready_for_save{false};
    bool has_blocking_diagnostics{false};
    bool has_conflicts{false};
    bool mutates{false};
    bool executes{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

enum class EditorInputRebindingProfilePanelStatus : std::uint8_t { ready, blocked };
enum class EditorInputRebindingCaptureStatus : std::uint8_t { waiting, captured, blocked };

struct EditorInputRebindingProfileBindingRow {
    std::string id;
    std::string kind_label;
    std::string context;
    std::string action;
    std::string current_binding;
    std::string profile_binding;
    std::string status_label;
    std::string diagnostic;
    bool overridden{false};
    bool ready{false};
};

struct EditorInputRebindingProfilePanelModel {
    EditorInputRebindingProfilePanelStatus status{EditorInputRebindingProfilePanelStatus::blocked};
    std::string status_label;
    std::string profile_id;
    std::size_t action_binding_count{0};
    std::size_t axis_binding_count{0};
    std::size_t action_override_count{0};
    std::size_t axis_override_count{0};
    bool ready_for_save{false};
    bool has_blocking_diagnostics{false};
    bool has_conflicts{false};
    bool mutates{false};
    bool executes{false};
    std::vector<EditorInputRebindingProfileBindingRow> binding_rows;
    std::vector<EditorInputRebindingProfileReviewRow> review_rows;
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorInputRebindingCaptureRow {
    std::string id;
    std::string surface;
    EditorInputRebindingCaptureStatus status{EditorInputRebindingCaptureStatus::blocked};
    bool mutates_profile{false};
    bool mutates_files{false};
    bool executes{false};
    std::string diagnostic;
};

struct EditorInputRebindingCaptureDesc {
    runtime::RuntimeInputActionMap base_actions;
    runtime::RuntimeInputRebindingProfile profile;
    std::string context;
    std::string action;
    runtime::RuntimeInputStateView state;
    bool capture_keyboard{true};
    bool capture_pointer{true};
    bool capture_gamepad_buttons{true};
    bool request_file_mutation{false};
    bool request_command_execution{false};
    bool request_native_handle_exposure{false};
    bool request_sdl3_input_api{false};
    bool request_ui_focus_consumption{false};
    bool request_multiplayer_device_assignment{false};
    bool request_input_glyph_generation{false};
};

struct EditorInputRebindingCaptureModel {
    EditorInputRebindingCaptureStatus status{EditorInputRebindingCaptureStatus::waiting};
    std::string status_label;
    std::string context;
    std::string action;
    std::optional<runtime::RuntimeInputActionTrigger> trigger;
    std::string trigger_label;
    runtime::RuntimeInputRebindingProfile candidate_profile;
    bool mutates_profile{false};
    bool mutates_files{false};
    bool executes{false};
    std::vector<EditorInputRebindingCaptureRow> rows;
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorInputRebindingAxisCaptureDesc {
    runtime::RuntimeInputActionMap base_actions;
    runtime::RuntimeInputRebindingProfile profile;
    std::string context;
    std::string action;
    runtime::RuntimeInputStateView state;
    float capture_deadzone{0.25F};
    bool capture_gamepad_axes{true};
    bool capture_keyboard_key_pair_axes{true};
    bool request_file_mutation{false};
    bool request_command_execution{false};
    bool request_native_handle_exposure{false};
    bool request_sdl3_input_api{false};
    bool request_ui_focus_consumption{false};
    bool request_multiplayer_device_assignment{false};
    bool request_input_glyph_generation{false};
};

struct EditorInputRebindingAxisCaptureModel {
    EditorInputRebindingCaptureStatus status{EditorInputRebindingCaptureStatus::waiting};
    std::string status_label;
    std::string context;
    std::string action;
    std::optional<runtime::RuntimeInputAxisSource> axis_source;
    std::string axis_source_label;
    runtime::RuntimeInputRebindingProfile candidate_profile;
    bool mutates_profile{false};
    bool mutates_files{false};
    bool executes{false};
    std::vector<EditorInputRebindingCaptureRow> rows;
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

/// Optional retained-document payload for explicit profile file Save/Load review rows
/// (`input_rebinding.persistence.*`).
struct EditorInputRebindingProfilePersistenceUiModel {
    std::string path_display;
    std::string last_status;
    std::vector<std::string> diagnostics;
};

struct EditorInputRebindingProfileSaveToProjectStoreResult {
    bool succeeded{false};
    std::string diagnostic;
};

struct EditorInputRebindingProfileLoadFromProjectStoreResult {
    bool succeeded{false};
    std::string diagnostic;
    runtime::RuntimeInputRebindingProfile profile{};
};

/// Returns empty string when `path` is accepted for `.inputrebinding` project-store persistence.
[[nodiscard]] std::string validate_editor_input_rebinding_profile_store_path(std::string_view path);

[[nodiscard]] EditorInputRebindingProfileSaveToProjectStoreResult
save_editor_input_rebinding_profile_to_project_store(ITextStore& store, std::string_view project_relative_path,
                                                     const runtime::RuntimeInputActionMap& base_actions,
                                                     const runtime::RuntimeInputRebindingProfile& profile);

[[nodiscard]] EditorInputRebindingProfileLoadFromProjectStoreResult
load_editor_input_rebinding_profile_from_project_store(ITextStore& store, std::string_view project_relative_path,
                                                       const runtime::RuntimeInputActionMap& base_actions);

[[nodiscard]] std::string_view
editor_input_rebinding_profile_review_status_label(EditorInputRebindingProfileReviewStatus status) noexcept;
[[nodiscard]] std::string_view
editor_input_rebinding_profile_panel_status_label(EditorInputRebindingProfilePanelStatus status) noexcept;
[[nodiscard]] std::string_view
editor_input_rebinding_capture_status_label(EditorInputRebindingCaptureStatus status) noexcept;

[[nodiscard]] EditorInputRebindingProfileReviewModel
make_editor_input_rebinding_profile_review_model(const EditorInputRebindingProfileReviewDesc& desc);
[[nodiscard]] EditorInputRebindingProfilePanelModel
make_editor_input_rebinding_profile_panel_model(const EditorInputRebindingProfileReviewDesc& desc);
[[nodiscard]] EditorInputRebindingCaptureModel
make_editor_input_rebinding_capture_action_model(const EditorInputRebindingCaptureDesc& desc);
[[nodiscard]] EditorInputRebindingAxisCaptureModel
make_editor_input_rebinding_capture_axis_model(const EditorInputRebindingAxisCaptureDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_input_rebinding_profile_panel_ui_model(const EditorInputRebindingProfilePanelModel& model,
                                            const EditorInputRebindingProfilePersistenceUiModel* persistence = nullptr);
[[nodiscard]] mirakana::ui::UiDocument
make_input_rebinding_capture_action_ui_model(const EditorInputRebindingCaptureModel& model);
[[nodiscard]] mirakana::ui::UiDocument
make_input_rebinding_capture_axis_ui_model(const EditorInputRebindingAxisCaptureModel& model);

} // namespace mirakana::editor
