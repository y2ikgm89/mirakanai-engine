// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"
#include "mirakana/platform/input.hpp"

#include <cstdint>
#include <string>

namespace mirakana::win32 {

inline constexpr std::uint32_t win32_vk_back = 0x08;
inline constexpr std::uint32_t win32_vk_escape = 0x1B;
inline constexpr std::uint32_t win32_vk_space = 0x20;
inline constexpr std::uint32_t win32_vk_end = 0x23;
inline constexpr std::uint32_t win32_vk_home = 0x24;
inline constexpr std::uint32_t win32_vk_left = 0x25;
inline constexpr std::uint32_t win32_vk_up = 0x26;
inline constexpr std::uint32_t win32_vk_right = 0x27;
inline constexpr std::uint32_t win32_vk_down = 0x28;
inline constexpr std::uint32_t win32_vk_delete = 0x2E;
inline constexpr std::uint32_t win32_vk_c = 0x43;
inline constexpr std::uint32_t win32_vk_v = 0x56;
inline constexpr std::uint32_t win32_vk_x = 0x58;

enum class Win32InputMessageId : std::uint8_t {
    unknown = 0,
    key_down,
    key_up,
    system_key_down,
    system_key_up,
    mouse_move,
    left_button_down,
    left_button_up,
    right_button_down,
    right_button_up,
    middle_button_down,
    middle_button_up,
    mouse_wheel,
    mouse_horizontal_wheel,
    raw_input,
};

struct Win32ModifierState {
    bool shift{false};
    bool control{false};
    bool alt{false};
    bool super{false};
};

struct Win32CopiedInputMessage {
    Win32InputMessageId message{Win32InputMessageId::unknown};
    std::uint32_t virtual_key{0};
    std::uint32_t scan_code{0};
    bool repeated{false};
    Win32ModifierState modifiers{};
    std::uint16_t low_word{0};
    std::uint16_t high_word{0};
    std::int16_t wheel_delta{0};
    std::uintptr_t window_token{0};
};

enum class Win32InputEventKind : std::uint8_t {
    unknown = 0,
    key_pressed,
    key_released,
    pointer_pressed,
    pointer_moved,
    pointer_released,
    mouse_wheel,
    raw_input,
};

struct Win32InputEvent {
    Win32InputEventKind kind{Win32InputEventKind::unknown};
    Key key{Key::unknown};
    std::uint32_t virtual_key{0};
    std::uint32_t scan_code{0};
    bool repeated{false};
    Win32ModifierState modifiers{};
    PointerSample pointer{};
    PointerId pointer_id{0};
    Vec2 wheel_delta{};
    std::uintptr_t window_token{0};
};

struct Win32RawInputRequest {
    bool keyboard{true};
    bool mouse{true};
    bool relative_mouse{false};
};

struct Win32RawInputRegistrationPlan {
    bool register_keyboard{false};
    bool register_mouse{false};
    bool capture_relative_mouse{false};
    bool register_hid_controllers{false};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

[[nodiscard]] Key win32_virtual_key_to_key(std::uint32_t virtual_key) noexcept;
[[nodiscard]] Win32InputEvent translate_win32_input_message(const Win32CopiedInputMessage& message) noexcept;
void apply_win32_input_event(const Win32InputEvent& event, VirtualInput* input,
                             VirtualPointerInput* pointer_input) noexcept;
[[nodiscard]] Win32RawInputRegistrationPlan plan_win32_raw_input_registration(Win32RawInputRequest request);

} // namespace mirakana::win32
