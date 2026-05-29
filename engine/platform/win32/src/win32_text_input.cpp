// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_text_input.hpp"

#include "win32_utf.hpp"

#include <stdexcept>

namespace mirakana::win32 {
namespace {

[[nodiscard]] std::string first_diagnostic_message(const std::vector<ui::AdapterPayloadDiagnostic>& diagnostics,
                                                   std::string fallback) {
    if (!diagnostics.empty()) {
        return diagnostics.front().message;
    }
    return fallback;
}

[[nodiscard]] std::optional<ui::TextEditCommandKind> text_edit_command_kind_from_key(Key key) noexcept {
    switch (key) {
    case Key::left:
        return ui::TextEditCommandKind::move_cursor_backward;
    case Key::right:
        return ui::TextEditCommandKind::move_cursor_forward;
    case Key::home:
        return ui::TextEditCommandKind::move_cursor_to_start;
    case Key::end:
        return ui::TextEditCommandKind::move_cursor_to_end;
    case Key::backspace:
        return ui::TextEditCommandKind::delete_backward;
    case Key::delete_key:
        return ui::TextEditCommandKind::delete_forward;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] bool has_text_edit_shortcut_modifier(const Win32ModifierState& modifiers) noexcept {
    return (modifiers.control || modifiers.super) && !modifiers.alt;
}

[[nodiscard]] std::optional<ui::TextEditClipboardCommandKind>
text_edit_clipboard_command_kind_from_virtual_key(std::uint32_t virtual_key) noexcept {
    switch (virtual_key) {
    case win32_vk_c:
        return ui::TextEditClipboardCommandKind::copy_selection;
    case win32_vk_x:
        return ui::TextEditClipboardCommandKind::cut_selection;
    case win32_vk_v:
        return ui::TextEditClipboardCommandKind::paste_text;
    default:
        return std::nullopt;
    }
}

} // namespace

Win32TextInputSessionPlan plan_win32_text_input_begin(const ui::PlatformTextInputRequest& request) {
    Win32TextInputSessionPlan plan{.request = request};
    const auto ui_plan = ui::plan_platform_text_input_session(request);
    if (!ui_plan.ready()) {
        plan.diagnostic = first_diagnostic_message(ui_plan.diagnostics, "platform text input request is invalid");
        return plan;
    }

    plan.start_session = true;
    plan.update_composition_window = true;
    return plan;
}

Win32TextInputEndPlan plan_win32_text_input_end(const ui::ElementId& target) {
    Win32TextInputEndPlan plan{.target = target};
    const auto ui_plan = ui::plan_platform_text_input_end(target);
    if (!ui_plan.ready()) {
        plan.diagnostic = first_diagnostic_message(ui_plan.diagnostics, "platform text input target is invalid");
        return plan;
    }

    plan.end_session = true;
    return plan;
}

std::optional<ui::CommittedTextInput> win32_committed_text_from_message(const ui::ElementId& target,
                                                                        const Win32CopiedTextInputMessage& message) {
    if (message.message != Win32TextInputMessageId::char_input || message.utf16_text.empty()) {
        return std::nullopt;
    }

    try {
        return ui::CommittedTextInput{
            .target = target,
            .text = detail::utf8_from_utf16(message.utf16_text),
        };
    } catch (...) {
        return std::nullopt;
    }
}

ui::TextEditCommitResult apply_win32_committed_text_message(const ui::TextEditState& state, const ui::ElementId& target,
                                                            const Win32CopiedTextInputMessage& message) {
    const auto committed = win32_committed_text_from_message(target, message);
    if (!committed.has_value()) {
        return ui::TextEditCommitResult{.committed = false, .state = state, .diagnostics = {}};
    }

    return ui::apply_committed_text_input(state, *committed);
}

std::optional<ui::TextEditCommand> win32_text_edit_command_from_input_event(const ui::ElementId& target,
                                                                            const Win32InputEvent& event) {
    if (event.kind != Win32InputEventKind::key_pressed || event.repeated) {
        return std::nullopt;
    }

    const auto command_kind = text_edit_command_kind_from_key(event.key);
    if (!command_kind.has_value()) {
        return std::nullopt;
    }

    return ui::TextEditCommand{.target = target, .kind = *command_kind};
}

ui::TextEditCommandResult apply_win32_text_edit_command_event(const ui::TextEditState& state,
                                                              const ui::ElementId& target,
                                                              const Win32InputEvent& event) {
    const auto command = win32_text_edit_command_from_input_event(target, event);
    if (!command.has_value()) {
        return ui::TextEditCommandResult{.applied = false, .state = state, .diagnostics = {}};
    }

    return ui::apply_text_edit_command(state, *command);
}

std::optional<ui::TextEditClipboardCommand>
win32_text_edit_clipboard_command_from_input_event(const ui::ElementId& target, const Win32InputEvent& event) {
    if (event.kind != Win32InputEventKind::key_pressed || event.repeated) {
        return std::nullopt;
    }
    if (!has_text_edit_shortcut_modifier(event.modifiers)) {
        return std::nullopt;
    }

    const auto command_kind = text_edit_clipboard_command_kind_from_virtual_key(event.virtual_key);
    if (!command_kind.has_value()) {
        return std::nullopt;
    }

    return ui::TextEditClipboardCommand{.target = target, .kind = *command_kind};
}

ui::TextEditClipboardCommandResult apply_win32_text_edit_clipboard_command_event(ui::IClipboardTextAdapter& adapter,
                                                                                 const ui::TextEditState& state,
                                                                                 const ui::ElementId& target,
                                                                                 const Win32InputEvent& event) {
    const auto command = win32_text_edit_clipboard_command_from_input_event(target, event);
    if (!command.has_value()) {
        return ui::TextEditClipboardCommandResult{.applied = false, .state = state, .diagnostics = {}};
    }

    return ui::apply_text_edit_clipboard_command(adapter, state, *command);
}

void Win32PlatformIntegrationAdapter::begin_text_input(const ui::PlatformTextInputRequest& request) {
    const auto plan = plan_win32_text_input_begin(request);
    if (!plan.succeeded()) {
        throw std::invalid_argument(plan.diagnostic);
    }

    active_request_ = plan.request;
}

void Win32PlatformIntegrationAdapter::end_text_input(const ui::ElementId& target) {
    const auto plan = plan_win32_text_input_end(target);
    if (!plan.succeeded()) {
        throw std::invalid_argument(plan.diagnostic);
    }

    if (active_request_.has_value() && active_request_->target.value == target.value) {
        active_request_.reset();
    }
}

const std::optional<ui::PlatformTextInputRequest>& Win32PlatformIntegrationAdapter::active_request() const noexcept {
    return active_request_;
}

} // namespace mirakana::win32
