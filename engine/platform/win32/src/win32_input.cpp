// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_input.hpp"

namespace mirakana::win32 {

Key win32_virtual_key_to_key(std::uint32_t virtual_key) noexcept {
    switch (virtual_key) {
    case win32_vk_left:
        return Key::left;
    case win32_vk_right:
        return Key::right;
    case win32_vk_up:
        return Key::up;
    case win32_vk_down:
        return Key::down;
    case win32_vk_space:
        return Key::space;
    case win32_vk_escape:
        return Key::escape;
    case win32_vk_back:
        return Key::backspace;
    case win32_vk_delete:
        return Key::delete_key;
    case win32_vk_home:
        return Key::home;
    case win32_vk_end:
        return Key::end;
    default:
        return Key::unknown;
    }
}

Win32InputEvent translate_win32_input_message(const Win32CopiedInputMessage& message) noexcept {
    Win32InputEvent event{
        .virtual_key = message.virtual_key,
        .scan_code = message.scan_code,
        .repeated = message.repeated,
        .modifiers = message.modifiers,
        .window_token = message.window_token,
    };

    switch (message.message) {
    case Win32InputMessageId::key_down:
    case Win32InputMessageId::system_key_down:
        event.kind = Win32InputEventKind::key_pressed;
        event.key = win32_virtual_key_to_key(message.virtual_key);
        return event;
    case Win32InputMessageId::key_up:
    case Win32InputMessageId::system_key_up:
        event.kind = Win32InputEventKind::key_released;
        event.key = win32_virtual_key_to_key(message.virtual_key);
        return event;
    case Win32InputMessageId::mouse_move:
        event.kind = Win32InputEventKind::pointer_moved;
        event.pointer = PointerSample{
            .id = primary_pointer_id,
            .kind = PointerKind::mouse,
            .position = Vec2{.x = static_cast<float>(message.low_word), .y = static_cast<float>(message.high_word)},
        };
        event.pointer_id = primary_pointer_id;
        return event;
    case Win32InputMessageId::left_button_down:
        event.kind = Win32InputEventKind::pointer_pressed;
        event.pointer = PointerSample{
            .id = primary_pointer_id,
            .kind = PointerKind::mouse,
            .position = Vec2{.x = static_cast<float>(message.low_word), .y = static_cast<float>(message.high_word)},
        };
        event.pointer_id = primary_pointer_id;
        return event;
    case Win32InputMessageId::left_button_up:
        event.kind = Win32InputEventKind::pointer_released;
        event.pointer_id = primary_pointer_id;
        return event;
    case Win32InputMessageId::mouse_wheel:
        event.kind = Win32InputEventKind::mouse_wheel;
        event.wheel_delta = Vec2{.x = 0.0F, .y = static_cast<float>(message.wheel_delta) / 120.0F};
        return event;
    case Win32InputMessageId::mouse_horizontal_wheel:
        event.kind = Win32InputEventKind::mouse_wheel;
        event.wheel_delta = Vec2{.x = static_cast<float>(message.wheel_delta) / 120.0F, .y = 0.0F};
        return event;
    case Win32InputMessageId::raw_input:
        event.kind = Win32InputEventKind::raw_input;
        return event;
    default:
        return event;
    }
}

void apply_win32_input_event(const Win32InputEvent& event, VirtualInput* input,
                             VirtualPointerInput* pointer_input) noexcept {
    if (input != nullptr && !event.repeated && event.key != Key::unknown) {
        if (event.kind == Win32InputEventKind::key_pressed) {
            input->press(event.key);
            return;
        }
        if (event.kind == Win32InputEventKind::key_released) {
            input->release(event.key);
            return;
        }
    }

    if (pointer_input == nullptr) {
        return;
    }
    if (event.kind == Win32InputEventKind::pointer_pressed) {
        pointer_input->press(event.pointer);
    } else if (event.kind == Win32InputEventKind::pointer_moved) {
        pointer_input->move(event.pointer);
    } else if (event.kind == Win32InputEventKind::pointer_released) {
        pointer_input->release(event.pointer_id);
    }
}

Win32RawInputRegistrationPlan plan_win32_raw_input_registration(Win32RawInputRequest request) {
    Win32RawInputRegistrationPlan plan;
    plan.register_keyboard = request.keyboard;
    plan.register_mouse = request.mouse;
    plan.capture_relative_mouse = request.mouse && request.relative_mouse;
    plan.register_hid_controllers = false;
    if (!request.keyboard && !request.mouse) {
        plan.diagnostic = "raw input registration must include keyboard or mouse";
    }
    return plan;
}

} // namespace mirakana::win32
