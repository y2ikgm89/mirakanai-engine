// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/input.hpp"
#include "mirakana/platform/window.hpp"

#include <cstdint>

namespace mirakana {

[[nodiscard]] Key sdl3_key_to_key(std::int32_t keycode) noexcept;
[[nodiscard]] PointerId sdl3_touch_pointer_id(std::uint64_t finger_id) noexcept;
[[nodiscard]] PointerSample sdl3_mouse_pointer_sample(float x, float y) noexcept;
[[nodiscard]] PointerSample sdl3_touch_pointer_sample(std::uint64_t finger_id, float normalized_x, float normalized_y,
                                                      WindowExtent extent) noexcept;
[[nodiscard]] GamepadButton sdl3_gamepad_button_to_button(std::int32_t button) noexcept;
[[nodiscard]] GamepadAxis sdl3_gamepad_axis_to_axis(std::int32_t axis) noexcept;
[[nodiscard]] float sdl3_gamepad_axis_value(GamepadAxis axis, std::int16_t value) noexcept;

} // namespace mirakana
