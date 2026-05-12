// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/input_rebinding.hpp"
#include "mirakana/editor/io.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

void push_review_row(EditorInputRebindingProfileReviewModel& model, EditorInputRebindingProfileReviewRow row) {
    if (row.status == EditorInputRebindingProfileReviewStatus::blocked) {
        model.has_blocking_diagnostics = true;
    }
    model.rows.push_back(std::move(row));
}

void reject_unsupported_claim(EditorInputRebindingProfileReviewModel& model, std::string claim,
                              std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(diagnostic);
    push_review_row(model, EditorInputRebindingProfileReviewRow{
                               .id = model.unsupported_claims.back(),
                               .surface = "editor-input-rebinding-profile",
                               .status = EditorInputRebindingProfileReviewStatus::blocked,
                               .diagnostic_code = std::nullopt,
                               .mutates = false,
                               .executes = false,
                               .diagnostic = std::move(diagnostic),
                           });
}

void reject_requested_unsupported_claims(EditorInputRebindingProfileReviewModel& model,
                                         const EditorInputRebindingProfileReviewDesc& desc) {
    if (desc.request_mutation) {
        reject_unsupported_claim(model, "mutation",
                                 "EditorInputRebindingProfileReviewModel is read-only and does not mutate files");
    }
    if (desc.request_execution) {
        reject_unsupported_claim(model, "execution",
                                 "EditorInputRebindingProfileReviewModel does not execute rebinding commands");
    }
    if (desc.request_native_handle_exposure) {
        reject_unsupported_claim(model, "native-handle-exposure",
                                 "Input rebinding review must keep native input handles private");
    }
    if (desc.request_sdl3_input_api) {
        reject_unsupported_claim(model, "sdl3-input-api",
                                 "Input rebinding profiles must use first-party runtime input contracts, not SDL3");
    }
    if (desc.request_dear_imgui_or_editor_private_runtime_dependency) {
        reject_unsupported_claim(
            model, "dear-imgui-editor-private-runtime-dependency",
            "Runtime input rebinding profiles must not depend on Dear ImGui or editor-private runtime APIs");
    }
    if (desc.request_ui_focus_consumption) {
        reject_unsupported_claim(model, "ui-focus-consumption",
                                 "UI focus, input consumption, and bubbling are outside this rebinding profile model");
    }
    if (desc.request_multiplayer_device_assignment) {
        reject_unsupported_claim(model, "multiplayer-device-assignment",
                                 "Multiplayer device assignment is outside this rebinding profile model");
    }
    if (desc.request_input_glyph_generation) {
        reject_unsupported_claim(model, "input-glyph-generation",
                                 "Input glyph generation is outside this rebinding profile model");
    }
}

void push_capture_row(EditorInputRebindingCaptureModel& model, EditorInputRebindingCaptureRow row) {
    model.rows.push_back(std::move(row));
}

void push_capture_row(EditorInputRebindingAxisCaptureModel& model, EditorInputRebindingCaptureRow row) {
    model.rows.push_back(std::move(row));
}

void reject_capture_unsupported_claim(EditorInputRebindingCaptureModel& model, std::string claim,
                                      std::string diagnostic) {
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(diagnostic);
    push_capture_row(model, EditorInputRebindingCaptureRow{
                                .id = model.unsupported_claims.back(),
                                .surface = "editor-input-rebinding-capture",
                                .status = EditorInputRebindingCaptureStatus::blocked,
                                .mutates_profile = false,
                                .mutates_files = false,
                                .executes = false,
                                .diagnostic = std::move(diagnostic),
                            });
}

void reject_requested_capture_unsupported_claims(EditorInputRebindingCaptureModel& model,
                                                 const EditorInputRebindingCaptureDesc& desc) {
    if (desc.request_file_mutation) {
        reject_capture_unsupported_claim(model, "file-mutation",
                                         "Input rebinding capture can update an in-memory candidate only");
    }
    if (desc.request_command_execution) {
        reject_capture_unsupported_claim(model, "command-execution",
                                         "Input rebinding capture must not execute commands");
    }
    if (desc.request_native_handle_exposure) {
        reject_capture_unsupported_claim(model, "native-handle-exposure",
                                         "Input rebinding capture must keep native input handles private");
    }
    if (desc.request_sdl3_input_api) {
        reject_capture_unsupported_claim(model, "sdl3-input-api",
                                         "Input rebinding capture must use first-party runtime input state, not SDL3");
    }
    if (desc.request_ui_focus_consumption) {
        reject_capture_unsupported_claim(model, "ui-focus-consumption",
                                         "Runtime UI focus, consumption, and bubbling are outside capture");
    }
    if (desc.request_multiplayer_device_assignment) {
        reject_capture_unsupported_claim(model, "multiplayer-device-assignment",
                                         "Multiplayer device assignment is outside input rebinding capture");
    }
    if (desc.request_input_glyph_generation) {
        reject_capture_unsupported_claim(model, "input-glyph-generation",
                                         "Input glyph generation is outside input rebinding capture");
    }
}

void reject_axis_capture_unsupported_claim(EditorInputRebindingAxisCaptureModel& model, std::string claim,
                                           std::string diagnostic) {
    model.unsupported_claims.push_back(std::move(claim));
    model.diagnostics.push_back(diagnostic);
    push_capture_row(model, EditorInputRebindingCaptureRow{
                                .id = model.unsupported_claims.back(),
                                .surface = "editor-input-rebinding-axis-capture",
                                .status = EditorInputRebindingCaptureStatus::blocked,
                                .mutates_profile = false,
                                .mutates_files = false,
                                .executes = false,
                                .diagnostic = std::move(diagnostic),
                            });
}

void reject_requested_axis_capture_unsupported_claims(EditorInputRebindingAxisCaptureModel& model,
                                                      const EditorInputRebindingAxisCaptureDesc& desc) {
    if (desc.request_file_mutation) {
        reject_axis_capture_unsupported_claim(model, "file-mutation",
                                              "Input rebinding axis capture can update an in-memory candidate only");
    }
    if (desc.request_command_execution) {
        reject_axis_capture_unsupported_claim(model, "command-execution",
                                              "Input rebinding axis capture must not execute commands");
    }
    if (desc.request_native_handle_exposure) {
        reject_axis_capture_unsupported_claim(model, "native-handle-exposure",
                                              "Input rebinding axis capture must keep native input handles private");
    }
    if (desc.request_sdl3_input_api) {
        reject_axis_capture_unsupported_claim(
            model, "sdl3-input-api", "Input rebinding axis capture must use first-party runtime input state, not SDL3");
    }
    if (desc.request_ui_focus_consumption) {
        reject_axis_capture_unsupported_claim(model, "ui-focus-consumption",
                                              "Runtime UI focus, consumption, and bubbling are outside capture");
    }
    if (desc.request_multiplayer_device_assignment) {
        reject_axis_capture_unsupported_claim(model, "multiplayer-device-assignment",
                                              "Multiplayer device assignment is outside input rebinding axis capture");
    }
    if (desc.request_input_glyph_generation) {
        reject_axis_capture_unsupported_claim(model, "input-glyph-generation",
                                              "Input glyph generation is outside input rebinding axis capture");
    }
}

[[nodiscard]] EditorInputRebindingCaptureStatus
capture_status_from_runtime(runtime::RuntimeInputRebindingCaptureStatus status) noexcept {
    switch (status) {
    case runtime::RuntimeInputRebindingCaptureStatus::waiting:
        return EditorInputRebindingCaptureStatus::waiting;
    case runtime::RuntimeInputRebindingCaptureStatus::captured:
        return EditorInputRebindingCaptureStatus::captured;
    case runtime::RuntimeInputRebindingCaptureStatus::blocked:
        return EditorInputRebindingCaptureStatus::blocked;
    }
    return EditorInputRebindingCaptureStatus::blocked;
}

[[nodiscard]] std::string diagnostic_text(const runtime::RuntimeInputRebindingDiagnostic& diagnostic) {
    if (diagnostic.path.empty()) {
        return diagnostic.message;
    }
    return diagnostic.path + ": " + diagnostic.message;
}

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
    return text.empty() ? "row" : text;
}

[[nodiscard]] std::string_view key_label(Key key) noexcept {
    switch (key) {
    case Key::left:
        return "left";
    case Key::right:
        return "right";
    case Key::up:
        return "up";
    case Key::down:
        return "down";
    case Key::space:
        return "space";
    case Key::escape:
        return "escape";
    case Key::unknown:
    case Key::count:
        break;
    }
    return "unknown";
}

[[nodiscard]] std::string_view gamepad_button_label(GamepadButton button) noexcept {
    switch (button) {
    case GamepadButton::south:
        return "south";
    case GamepadButton::east:
        return "east";
    case GamepadButton::west:
        return "west";
    case GamepadButton::north:
        return "north";
    case GamepadButton::back:
        return "back";
    case GamepadButton::guide:
        return "guide";
    case GamepadButton::start:
        return "start";
    case GamepadButton::left_stick:
        return "left_stick";
    case GamepadButton::right_stick:
        return "right_stick";
    case GamepadButton::left_shoulder:
        return "left_shoulder";
    case GamepadButton::right_shoulder:
        return "right_shoulder";
    case GamepadButton::dpad_up:
        return "dpad_up";
    case GamepadButton::dpad_down:
        return "dpad_down";
    case GamepadButton::dpad_left:
        return "dpad_left";
    case GamepadButton::dpad_right:
        return "dpad_right";
    case GamepadButton::unknown:
    case GamepadButton::count:
        break;
    }
    return "unknown";
}

[[nodiscard]] std::string_view gamepad_axis_label(GamepadAxis axis) noexcept {
    switch (axis) {
    case GamepadAxis::left_x:
        return "left_x";
    case GamepadAxis::left_y:
        return "left_y";
    case GamepadAxis::right_x:
        return "right_x";
    case GamepadAxis::right_y:
        return "right_y";
    case GamepadAxis::left_trigger:
        return "left_trigger";
    case GamepadAxis::right_trigger:
        return "right_trigger";
    case GamepadAxis::unknown:
    case GamepadAxis::count:
        break;
    }
    return "unknown";
}

[[nodiscard]] std::string format_float(float value) {
    char buffer[32]{};
    (void)std::snprintf(buffer, sizeof(buffer), "%.3f", static_cast<double>(value));
    return std::string(buffer);
}

[[nodiscard]] std::string format_trigger(const runtime::RuntimeInputActionTrigger& trigger) {
    switch (trigger.kind) {
    case runtime::RuntimeInputActionTriggerKind::key:
        return "key:" + std::string(key_label(trigger.key));
    case runtime::RuntimeInputActionTriggerKind::pointer:
        return "pointer:" + std::to_string(trigger.pointer_id);
    case runtime::RuntimeInputActionTriggerKind::gamepad_button:
        return "gamepad_button:" + std::to_string(trigger.gamepad_id) + ":" +
               std::string(gamepad_button_label(trigger.gamepad_button));
    }
    return "unknown";
}

[[nodiscard]] std::string format_axis_source(const runtime::RuntimeInputAxisSource& source) {
    switch (source.kind) {
    case runtime::RuntimeInputAxisSourceKind::key_pair:
        return "key_pair:" + std::string(key_label(source.negative_key)) + "/" +
               std::string(key_label(source.positive_key));
    case runtime::RuntimeInputAxisSourceKind::gamepad_axis:
        return "gamepad_axis:" + std::to_string(source.gamepad_id) + ":" +
               std::string(gamepad_axis_label(source.gamepad_axis)) + " scale=" + format_float(source.scale) +
               " deadzone=" + format_float(source.deadzone);
    }
    return "unknown";
}

template <typename T, typename FormatFn>
[[nodiscard]] std::string join_formatted_rows(const std::vector<T>& rows, FormatFn format) {
    if (rows.empty()) {
        return "-";
    }
    std::string result;
    for (const auto& row : rows) {
        if (!result.empty()) {
            result += ", ";
        }
        result += format(row);
    }
    return result;
}

[[nodiscard]] const runtime::RuntimeInputRebindingActionOverride*
find_action_override(const runtime::RuntimeInputRebindingProfile& profile, std::string_view context,
                     std::string_view action) noexcept {
    const auto it = std::ranges::find_if(profile.action_overrides, [context, action](const auto& override_row) {
        return override_row.context == context && override_row.action == action;
    });
    return it == profile.action_overrides.end() ? nullptr : &(*it);
}

[[nodiscard]] const runtime::RuntimeInputRebindingAxisOverride*
find_axis_override(const runtime::RuntimeInputRebindingProfile& profile, std::string_view context,
                   std::string_view action) noexcept {
    const auto it = std::ranges::find_if(profile.axis_overrides, [context, action](const auto& override_row) {
        return override_row.context == context && override_row.action == action;
    });
    return it == profile.axis_overrides.end() ? nullptr : &(*it);
}

[[nodiscard]] std::string binding_row_id(std::string_view prefix, std::string_view context, std::string_view action) {
    return std::string(prefix) + "." + sanitize_element_id(context) + "." + sanitize_element_id(action);
}

void append_action_binding_rows(EditorInputRebindingProfilePanelModel& model,
                                const runtime::RuntimeInputActionMap& base_actions,
                                const runtime::RuntimeInputRebindingProfile& profile) {
    for (const auto& binding : base_actions.bindings()) {
        const auto* override_row = find_action_override(profile, binding.context, binding.action);
        model.binding_rows.push_back(EditorInputRebindingProfileBindingRow{
            .id = binding_row_id("action", binding.context, binding.action),
            .kind_label = "action",
            .context = sanitize_text(binding.context),
            .action = sanitize_text(binding.action),
            .current_binding = join_formatted_rows(binding.triggers, format_trigger),
            .profile_binding =
                override_row == nullptr ? "-" : join_formatted_rows(override_row->triggers, format_trigger),
            .status_label = model.ready_for_save ? "ready" : "blocked",
            .diagnostic = override_row == nullptr ? "base action binding" : "profile action override reviewed",
            .overridden = override_row != nullptr,
            .ready = model.ready_for_save,
        });
    }
}

void append_axis_binding_rows(EditorInputRebindingProfilePanelModel& model,
                              const runtime::RuntimeInputActionMap& base_actions,
                              const runtime::RuntimeInputRebindingProfile& profile) {
    for (const auto& binding : base_actions.axis_bindings()) {
        const auto* override_row = find_axis_override(profile, binding.context, binding.action);
        model.binding_rows.push_back(EditorInputRebindingProfileBindingRow{
            .id = binding_row_id("axis", binding.context, binding.action),
            .kind_label = "axis",
            .context = sanitize_text(binding.context),
            .action = sanitize_text(binding.action),
            .current_binding = join_formatted_rows(binding.sources, format_axis_source),
            .profile_binding =
                override_row == nullptr ? "-" : join_formatted_rows(override_row->sources, format_axis_source),
            .status_label = model.ready_for_save ? "ready" : "blocked",
            .diagnostic = override_row == nullptr ? "base axis binding" : "profile axis override reviewed",
            .overridden = override_row != nullptr,
            .ready = model.ready_for_save,
        });
    }
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("input rebinding ui element could not be added");
    }
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

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

} // namespace

std::string_view
editor_input_rebinding_profile_review_status_label(EditorInputRebindingProfileReviewStatus status) noexcept {
    switch (status) {
    case EditorInputRebindingProfileReviewStatus::ready:
        return "ready";
    case EditorInputRebindingProfileReviewStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

std::string_view
editor_input_rebinding_profile_panel_status_label(EditorInputRebindingProfilePanelStatus status) noexcept {
    switch (status) {
    case EditorInputRebindingProfilePanelStatus::ready:
        return "ready";
    case EditorInputRebindingProfilePanelStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

std::string_view editor_input_rebinding_capture_status_label(EditorInputRebindingCaptureStatus status) noexcept {
    switch (status) {
    case EditorInputRebindingCaptureStatus::waiting:
        return "waiting";
    case EditorInputRebindingCaptureStatus::captured:
        return "captured";
    case EditorInputRebindingCaptureStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

EditorInputRebindingProfileReviewModel
make_editor_input_rebinding_profile_review_model(const EditorInputRebindingProfileReviewDesc& desc) {
    EditorInputRebindingProfileReviewModel model;

    for (const auto& diagnostic : runtime::validate_runtime_input_rebinding_profile(desc.base_actions, desc.profile)) {
        const auto text = diagnostic_text(diagnostic);
        if (diagnostic.code == runtime::RuntimeInputRebindingDiagnosticCode::trigger_conflict) {
            model.has_conflicts = true;
        }
        model.diagnostics.push_back(text);
        push_review_row(model, EditorInputRebindingProfileReviewRow{
                                   .id = diagnostic.path,
                                   .surface = "runtime-input-rebinding-profile",
                                   .status = EditorInputRebindingProfileReviewStatus::blocked,
                                   .diagnostic_code = diagnostic.code,
                                   .mutates = false,
                                   .executes = false,
                                   .diagnostic = text,
                               });
    }

    reject_requested_unsupported_claims(model, desc);

    model.ready_for_save = !model.has_blocking_diagnostics;
    if (model.ready_for_save) {
        push_review_row(model, EditorInputRebindingProfileReviewRow{
                                   .id = "profile",
                                   .surface = "runtime-input-rebinding-profile",
                                   .status = EditorInputRebindingProfileReviewStatus::ready,
                                   .diagnostic_code = std::nullopt,
                                   .mutates = false,
                                   .executes = false,
                                   .diagnostic = "runtime input rebinding profile is ready for save",
                               });
    }

    return model;
}

EditorInputRebindingProfilePanelModel
make_editor_input_rebinding_profile_panel_model(const EditorInputRebindingProfileReviewDesc& desc) {
    const auto review = make_editor_input_rebinding_profile_review_model(desc);
    EditorInputRebindingProfilePanelModel model;
    model.status = review.ready_for_save ? EditorInputRebindingProfilePanelStatus::ready
                                         : EditorInputRebindingProfilePanelStatus::blocked;
    model.status_label = std::string(editor_input_rebinding_profile_panel_status_label(model.status));
    model.profile_id = sanitize_text(desc.profile.profile_id);
    model.action_binding_count = desc.base_actions.bindings().size();
    model.axis_binding_count = desc.base_actions.axis_bindings().size();
    model.action_override_count = desc.profile.action_overrides.size();
    model.axis_override_count = desc.profile.axis_overrides.size();
    model.ready_for_save = review.ready_for_save;
    model.has_blocking_diagnostics = review.has_blocking_diagnostics;
    model.has_conflicts = review.has_conflicts;
    model.mutates = review.mutates;
    model.executes = review.executes;
    model.review_rows = review.rows;
    model.unsupported_claims = review.unsupported_claims;
    model.diagnostics = review.diagnostics;

    append_action_binding_rows(model, desc.base_actions, desc.profile);
    append_axis_binding_rows(model, desc.base_actions, desc.profile);
    return model;
}

EditorInputRebindingCaptureModel
make_editor_input_rebinding_capture_action_model(const EditorInputRebindingCaptureDesc& desc) {
    EditorInputRebindingCaptureModel model;
    model.context = sanitize_text(desc.context);
    model.action = sanitize_text(desc.action);
    model.trigger_label = "-";
    model.candidate_profile = desc.profile;

    reject_requested_capture_unsupported_claims(model, desc);
    if (!model.unsupported_claims.empty()) {
        model.status = EditorInputRebindingCaptureStatus::blocked;
        model.status_label = std::string(editor_input_rebinding_capture_status_label(model.status));
        return model;
    }

    runtime::RuntimeInputRebindingCaptureRequest request;
    request.context = desc.context;
    request.action = desc.action;
    request.state = desc.state;
    request.capture_keyboard = desc.capture_keyboard;
    request.capture_pointer = desc.capture_pointer;
    request.capture_gamepad_buttons = desc.capture_gamepad_buttons;

    const auto result = runtime::capture_runtime_input_rebinding_action(desc.base_actions, desc.profile, request);
    model.status = capture_status_from_runtime(result.status);
    model.status_label = std::string(editor_input_rebinding_capture_status_label(model.status));
    model.trigger = result.trigger;
    if (model.trigger.has_value()) {
        model.trigger_label = format_trigger(*model.trigger);
    }
    for (const auto& diagnostic : result.diagnostics) {
        model.diagnostics.push_back(diagnostic_text(diagnostic));
    }

    switch (model.status) {
    case EditorInputRebindingCaptureStatus::captured:
        model.candidate_profile = result.candidate_profile;
        model.mutates_profile = true;
        push_capture_row(model, EditorInputRebindingCaptureRow{
                                    .id = "capture",
                                    .surface = "editor-input-rebinding-capture",
                                    .status = model.status,
                                    .mutates_profile = true,
                                    .mutates_files = false,
                                    .executes = false,
                                    .diagnostic = "captured action binding candidate",
                                });
        break;
    case EditorInputRebindingCaptureStatus::waiting:
        push_capture_row(model, EditorInputRebindingCaptureRow{
                                    .id = "capture",
                                    .surface = "editor-input-rebinding-capture",
                                    .status = model.status,
                                    .mutates_profile = false,
                                    .mutates_files = false,
                                    .executes = false,
                                    .diagnostic = "waiting for an allowed input press",
                                });
        break;
    case EditorInputRebindingCaptureStatus::blocked:
        push_capture_row(
            model, EditorInputRebindingCaptureRow{
                       .id = "capture",
                       .surface = "editor-input-rebinding-capture",
                       .status = model.status,
                       .mutates_profile = false,
                       .mutates_files = false,
                       .executes = false,
                       .diagnostic = model.diagnostics.empty() ? "capture candidate is blocked" : model.diagnostics[0],
                   });
        break;
    }

    return model;
}

EditorInputRebindingAxisCaptureModel
make_editor_input_rebinding_capture_axis_model(const EditorInputRebindingAxisCaptureDesc& desc) {
    EditorInputRebindingAxisCaptureModel model;
    model.context = sanitize_text(desc.context);
    model.action = sanitize_text(desc.action);
    model.axis_source_label = "-";
    model.candidate_profile = desc.profile;

    reject_requested_axis_capture_unsupported_claims(model, desc);
    if (!model.unsupported_claims.empty()) {
        model.status = EditorInputRebindingCaptureStatus::blocked;
        model.status_label = std::string(editor_input_rebinding_capture_status_label(model.status));
        return model;
    }

    runtime::RuntimeInputRebindingAxisCaptureRequest request;
    request.context = desc.context;
    request.action = desc.action;
    request.state = desc.state;
    request.capture_deadzone = desc.capture_deadzone;
    request.capture_gamepad_axes = desc.capture_gamepad_axes;
    request.capture_keyboard_key_pair_axes = desc.capture_keyboard_key_pair_axes;

    const auto result = runtime::capture_runtime_input_rebinding_axis(desc.base_actions, desc.profile, request);
    model.status = capture_status_from_runtime(result.status);
    model.status_label = std::string(editor_input_rebinding_capture_status_label(model.status));
    model.axis_source = result.source;
    if (model.axis_source.has_value()) {
        model.axis_source_label = format_axis_source(*model.axis_source);
    }
    for (const auto& diagnostic : result.diagnostics) {
        model.diagnostics.push_back(diagnostic_text(diagnostic));
    }

    switch (model.status) {
    case EditorInputRebindingCaptureStatus::captured:
        model.candidate_profile = result.candidate_profile;
        model.mutates_profile = true;
        {
            const char* cap_msg = "captured axis binding candidate";
            if (model.axis_source.has_value()) {
                cap_msg = model.axis_source->kind == runtime::RuntimeInputAxisSourceKind::gamepad_axis
                              ? "captured axis binding candidate (gamepad)"
                              : "captured axis binding candidate (keyboard key pair)";
            }
            push_capture_row(model, EditorInputRebindingCaptureRow{
                                        .id = "capture.axis",
                                        .surface = "editor-input-rebinding-axis-capture",
                                        .status = model.status,
                                        .mutates_profile = true,
                                        .mutates_files = false,
                                        .executes = false,
                                        .diagnostic = cap_msg,
                                    });
        }
        break;
    case EditorInputRebindingCaptureStatus::waiting:
        push_capture_row(model, EditorInputRebindingCaptureRow{
                                    .id = "capture.axis",
                                    .surface = "editor-input-rebinding-axis-capture",
                                    .status = model.status,
                                    .mutates_profile = false,
                                    .mutates_files = false,
                                    .executes = false,
                                    .diagnostic = "waiting for an allowed gamepad axis past the deadzone, or two held "
                                                  "keyboard keys for a key-pair axis",
                                });
        break;
    case EditorInputRebindingCaptureStatus::blocked:
        push_capture_row(
            model, EditorInputRebindingCaptureRow{
                       .id = "capture.axis",
                       .surface = "editor-input-rebinding-axis-capture",
                       .status = model.status,
                       .mutates_profile = false,
                       .mutates_files = false,
                       .executes = false,
                       .diagnostic = model.diagnostics.empty() ? "capture candidate is blocked" : model.diagnostics[0],
                   });
        break;
    }

    return model;
}

namespace {

[[nodiscard]] bool is_drive_absolute_path(std::string_view path) noexcept {
    if (path.size() < 2 || path[1] != ':') {
        return false;
    }
    const char prefix = path[0];
    return (prefix >= 'A' && prefix <= 'Z') || (prefix >= 'a' && prefix <= 'z');
}

[[nodiscard]] bool is_absolute_like_path(std::string_view path) noexcept {
    return path.starts_with('/') || path.starts_with('\\') || is_drive_absolute_path(path);
}

[[nodiscard]] bool has_parent_segment(std::string_view path) noexcept {
    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto separator = path.find('/', begin);
        const auto end = separator == std::string_view::npos ? path.size() : separator;
        if (path.substr(begin, end - begin) == "..") {
            return true;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        begin = separator + 1U;
    }
    return false;
}

[[nodiscard]] std::string validate_editor_input_rebinding_profile_store_path_impl(std::string_view path) {
    if (path.empty()) {
        return "input rebinding profile path must not be empty";
    }
    if (path.find_first_of("\r\n=") != std::string_view::npos) {
        return "input rebinding profile path must not contain newlines or '='";
    }
    if (path.find(';') != std::string_view::npos) {
        return "input rebinding profile path must not contain ';'";
    }
    if (path.find('\\') != std::string_view::npos) {
        return "input rebinding profile path must use forward slashes only";
    }
    if (is_absolute_like_path(path)) {
        return "input rebinding profile path must be project-relative";
    }
    if (has_parent_segment(path)) {
        return "input rebinding profile path must not contain '..'";
    }
    std::size_t segment_start = 0;
    while (segment_start <= path.size()) {
        const auto segment_end = path.find('/', segment_start);
        const auto segment = segment_end == std::string_view::npos
                                 ? path.substr(segment_start)
                                 : path.substr(segment_start, segment_end - segment_start);
        if (segment.empty() || segment == ".") {
            return "input rebinding profile path contains an invalid segment";
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_start = segment_end + 1U;
    }
    if (!path.ends_with(".inputrebinding")) {
        return "input rebinding profile path must end with .inputrebinding";
    }
    return {};
}

} // namespace

std::string validate_editor_input_rebinding_profile_store_path(std::string_view path) {
    return validate_editor_input_rebinding_profile_store_path_impl(path);
}

EditorInputRebindingProfileSaveToProjectStoreResult
save_editor_input_rebinding_profile_to_project_store(ITextStore& store, const std::string_view project_relative_path,
                                                     const runtime::RuntimeInputActionMap& base_actions,
                                                     const runtime::RuntimeInputRebindingProfile& profile) {
    EditorInputRebindingProfileSaveToProjectStoreResult out{};
    if (const std::string path_diag = validate_editor_input_rebinding_profile_store_path(project_relative_path);
        !path_diag.empty()) {
        out.diagnostic = path_diag;
        return out;
    }

    const auto review = make_editor_input_rebinding_profile_review_model(EditorInputRebindingProfileReviewDesc{
        .base_actions = base_actions,
        .profile = profile,
    });
    if (!review.ready_for_save) {
        out.diagnostic = review.diagnostics.empty() ? "runtime input rebinding profile is not ready for save"
                                                    : review.diagnostics.front();
        return out;
    }

    try {
        const std::string payload = runtime::serialize_runtime_input_rebinding_profile(profile);
        store.write_text(project_relative_path, payload);
    } catch (const std::exception& error) {
        out.diagnostic = error.what();
        return out;
    }
    out.succeeded = true;
    return out;
}

EditorInputRebindingProfileLoadFromProjectStoreResult
load_editor_input_rebinding_profile_from_project_store(ITextStore& store, const std::string_view project_relative_path,
                                                       const runtime::RuntimeInputActionMap& base_actions) {
    EditorInputRebindingProfileLoadFromProjectStoreResult out{};
    if (const std::string path_diag = validate_editor_input_rebinding_profile_store_path(project_relative_path);
        !path_diag.empty()) {
        out.diagnostic = path_diag;
        return out;
    }

    std::string text;
    try {
        text = store.read_text(project_relative_path);
    } catch (const std::exception& error) {
        out.diagnostic = error.what();
        return out;
    }

    const auto parsed = runtime::deserialize_runtime_input_rebinding_profile(text);
    if (!parsed.succeeded()) {
        out.diagnostic =
            parsed.diagnostic.empty() ? "runtime input rebinding profile deserialize failed" : parsed.diagnostic;
        return out;
    }

    const auto issues = runtime::validate_runtime_input_rebinding_profile(base_actions, parsed.profile);
    if (!issues.empty()) {
        out.diagnostic = issues.front().message;
        return out;
    }

    out.profile = parsed.profile;
    out.succeeded = true;
    return out;
}

mirakana::ui::UiDocument
make_input_rebinding_profile_panel_ui_model(const EditorInputRebindingProfilePanelModel& model,
                                            const EditorInputRebindingProfilePersistenceUiModel* persistence) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("input_rebinding", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"input_rebinding"};

    append_label(document, root, "input_rebinding.status", model.status_label);
    append_label(document, root, "input_rebinding.profile.id", model.profile_id);
    append_label(document, root, "input_rebinding.summary.actions", std::to_string(model.action_binding_count));
    append_label(document, root, "input_rebinding.summary.axes", std::to_string(model.axis_binding_count));
    append_label(document, root, "input_rebinding.summary.action_overrides",
                 std::to_string(model.action_override_count));
    append_label(document, root, "input_rebinding.summary.axis_overrides", std::to_string(model.axis_override_count));

    add_or_throw(document, make_child("input_rebinding.bindings", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId bindings_root{"input_rebinding.bindings"};
    for (const auto& row : model.binding_rows) {
        const auto prefix = "input_rebinding.bindings." + sanitize_element_id(row.id);
        mirakana::ui::ElementDesc item = make_child(prefix, bindings_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.kind_label + " " + row.context + "/" + row.action);
        item.enabled = row.ready;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".current", row.current_binding);
        append_label(document, item_root, prefix + ".profile", row.profile_binding);
        append_label(document, item_root, prefix + ".status", row.status_label);
    }

    add_or_throw(document, make_child("input_rebinding.review", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId review_root{"input_rebinding.review"};
    for (const auto& row : model.review_rows) {
        const auto prefix = "input_rebinding.review." + sanitize_element_id(row.id);
        mirakana::ui::ElementDesc item = make_child(prefix, review_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.surface);
        item.enabled = row.status == EditorInputRebindingProfileReviewStatus::ready;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".status",
                     std::string(editor_input_rebinding_profile_review_status_label(row.status)));
        append_label(document, item_root, prefix + ".diagnostic", row.diagnostic);
    }

    add_or_throw(document, make_child("input_rebinding.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{"input_rebinding.diagnostics"};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, "input_rebinding.diagnostics." + std::to_string(index),
                     model.diagnostics[index]);
    }

    if (persistence != nullptr) {
        add_or_throw(document, make_child("input_rebinding.persistence", root, mirakana::ui::SemanticRole::list));
        const mirakana::ui::ElementId persistence_root{"input_rebinding.persistence"};
        append_label(document, persistence_root, "input_rebinding.persistence.path", persistence->path_display);
        append_label(document, persistence_root, "input_rebinding.persistence.last_status", persistence->last_status);
        if (!persistence->diagnostics.empty()) {
            add_or_throw(document, make_child("input_rebinding.persistence.diagnostics", persistence_root,
                                              mirakana::ui::SemanticRole::list));
            const mirakana::ui::ElementId persistence_diag_root{"input_rebinding.persistence.diagnostics"};
            for (std::size_t index = 0; index < persistence->diagnostics.size(); ++index) {
                append_label(document, persistence_diag_root,
                             "input_rebinding.persistence.diagnostics." + std::to_string(index),
                             persistence->diagnostics[index]);
            }
        }
    }

    return document;
}

mirakana::ui::UiDocument make_input_rebinding_capture_action_ui_model(const EditorInputRebindingCaptureModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("input_rebinding.capture", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"input_rebinding.capture"};

    append_label(document, root, "input_rebinding.capture.status", model.status_label);
    append_label(document, root, "input_rebinding.capture.context", model.context);
    append_label(document, root, "input_rebinding.capture.action", model.action);
    append_label(document, root, "input_rebinding.capture.trigger", model.trigger_label);

    add_or_throw(document, make_child("input_rebinding.capture.rows", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId rows_root{"input_rebinding.capture.rows"};
    for (const auto& row : model.rows) {
        const auto prefix = "input_rebinding.capture.rows." + sanitize_element_id(row.id);
        mirakana::ui::ElementDesc item = make_child(prefix, rows_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.surface);
        item.enabled = row.status == EditorInputRebindingCaptureStatus::captured;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".status",
                     std::string(editor_input_rebinding_capture_status_label(row.status)));
        append_label(document, item_root, prefix + ".diagnostic", row.diagnostic);
    }

    add_or_throw(document, make_child("input_rebinding.capture.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{"input_rebinding.capture.diagnostics"};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, "input_rebinding.capture.diagnostics." + std::to_string(index),
                     model.diagnostics[index]);
    }

    return document;
}

mirakana::ui::UiDocument make_input_rebinding_capture_axis_ui_model(const EditorInputRebindingAxisCaptureModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("input_rebinding.capture.axis", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"input_rebinding.capture.axis"};

    append_label(document, root, "input_rebinding.capture.axis.status", model.status_label);
    append_label(document, root, "input_rebinding.capture.axis.context", model.context);
    append_label(document, root, "input_rebinding.capture.axis.action", model.action);
    append_label(document, root, "input_rebinding.capture.axis.source", model.axis_source_label);

    add_or_throw(document, make_child("input_rebinding.capture.axis.rows", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId rows_root{"input_rebinding.capture.axis.rows"};
    for (const auto& row : model.rows) {
        const auto prefix = "input_rebinding.capture.axis.rows." + sanitize_element_id(row.id);
        mirakana::ui::ElementDesc item = make_child(prefix, rows_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.surface);
        item.enabled = row.status == EditorInputRebindingCaptureStatus::captured;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".status",
                     std::string(editor_input_rebinding_capture_status_label(row.status)));
        append_label(document, item_root, prefix + ".diagnostic", row.diagnostic);
    }

    add_or_throw(document,
                 make_child("input_rebinding.capture.axis.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{"input_rebinding.capture.axis.diagnostics"};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, "input_rebinding.capture.axis.diagnostics." + std::to_string(index),
                     model.diagnostics[index]);
    }

    return document;
}

} // namespace mirakana::editor
