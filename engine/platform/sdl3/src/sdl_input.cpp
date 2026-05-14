// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/sdl3/sdl_input.hpp"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keycode.h>

namespace mirakana {

namespace {

inline constexpr PointerId touch_pointer_id_mask = 0x80000000U;
inline constexpr PointerId touch_pointer_payload_mask = 0x7FFFFFFFU;

} // namespace

Key sdl3_key_to_key(std::uint32_t keycode) noexcept {
    switch (keycode) {
    case SDLK_LEFT:
        return Key::left;
    case SDLK_RIGHT:
        return Key::right;
    case SDLK_UP:
        return Key::up;
    case SDLK_DOWN:
        return Key::down;
    case SDLK_SPACE:
        return Key::space;
    case SDLK_ESCAPE:
        return Key::escape;
    case SDLK_BACKSPACE:
        return Key::backspace;
    case SDLK_DELETE:
        return Key::delete_key;
    case SDLK_HOME:
        return Key::home;
    case SDLK_END:
        return Key::end;
    default:
        return Key::unknown;
    }
}

PointerId sdl3_touch_pointer_id(std::uint64_t finger_id) noexcept {
    const auto folded = static_cast<PointerId>((finger_id ^ (finger_id >> 32U)) & touch_pointer_payload_mask);
    return touch_pointer_id_mask | folded;
}

PointerSample sdl3_mouse_pointer_sample(float x, float y) noexcept {
    return PointerSample{.id = primary_pointer_id, .kind = PointerKind::mouse, .position = Vec2{.x = x, .y = y}};
}

PointerSample sdl3_touch_pointer_sample(std::uint64_t finger_id, float normalized_x, float normalized_y,
                                        WindowExtent extent) noexcept {
    return PointerSample{
        .id = sdl3_touch_pointer_id(finger_id),
        .kind = PointerKind::touch,
        .position =
            Vec2{
                .x = normalized_x * static_cast<float>(extent.width),
                .y = normalized_y * static_cast<float>(extent.height),
            },
    };
}

GamepadButton sdl3_gamepad_button_to_button(std::int32_t button) noexcept {
    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return GamepadButton::south;
    case SDL_GAMEPAD_BUTTON_EAST:
        return GamepadButton::east;
    case SDL_GAMEPAD_BUTTON_WEST:
        return GamepadButton::west;
    case SDL_GAMEPAD_BUTTON_NORTH:
        return GamepadButton::north;
    case SDL_GAMEPAD_BUTTON_BACK:
        return GamepadButton::back;
    case SDL_GAMEPAD_BUTTON_GUIDE:
        return GamepadButton::guide;
    case SDL_GAMEPAD_BUTTON_START:
        return GamepadButton::start;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:
        return GamepadButton::left_stick;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
        return GamepadButton::right_stick;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        return GamepadButton::left_shoulder;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        return GamepadButton::right_shoulder;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return GamepadButton::dpad_up;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return GamepadButton::dpad_down;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return GamepadButton::dpad_left;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return GamepadButton::dpad_right;
    default:
        return GamepadButton::unknown;
    }
}

GamepadAxis sdl3_gamepad_axis_to_axis(std::int32_t axis) noexcept {
    switch (axis) {
    case SDL_GAMEPAD_AXIS_LEFTX:
        return GamepadAxis::left_x;
    case SDL_GAMEPAD_AXIS_LEFTY:
        return GamepadAxis::left_y;
    case SDL_GAMEPAD_AXIS_RIGHTX:
        return GamepadAxis::right_x;
    case SDL_GAMEPAD_AXIS_RIGHTY:
        return GamepadAxis::right_y;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
        return GamepadAxis::left_trigger;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
        return GamepadAxis::right_trigger;
    default:
        return GamepadAxis::unknown;
    }
}

float sdl3_gamepad_axis_value(GamepadAxis axis, std::int16_t value) noexcept {
    if (axis == GamepadAxis::left_trigger || axis == GamepadAxis::right_trigger) {
        return value > 0 ? static_cast<float>(value) / 32767.0F : 0.0F;
    }

    return value < 0 ? static_cast<float>(value) / 32768.0F : static_cast<float>(value) / 32767.0F;
}

} // namespace mirakana
