// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mirakana {

enum class Key : std::uint8_t {
    unknown = 0,
    left,
    right,
    up,
    down,
    space,
    escape,
    backspace,
    delete_key,
    home,
    end,
    count
};

class VirtualInput {
  public:
    void begin_frame() noexcept;
    void press(Key key) noexcept;
    void release(Key key) noexcept;

    [[nodiscard]] bool key_down(Key key) const noexcept;
    [[nodiscard]] bool key_pressed(Key key) const noexcept;
    [[nodiscard]] bool key_released(Key key) const noexcept;
    [[nodiscard]] Vec2 digital_axis(Key negative_x, Key positive_x, Key negative_y, Key positive_y) const noexcept;

  private:
    struct ButtonState {
        bool down{false};
        bool pressed{false};
        bool released{false};
    };

    [[nodiscard]] static constexpr std::size_t key_index(Key key) noexcept {
        return static_cast<std::size_t>(key);
    }

    [[nodiscard]] static constexpr bool valid_key(Key key) noexcept {
        return key != Key::unknown && key_index(key) < key_index(Key::count);
    }

    static constexpr std::size_t key_count = static_cast<std::size_t>(Key::count);

    std::array<ButtonState, key_count> keys_{};
};

using PointerId = std::uint32_t;

inline constexpr PointerId primary_pointer_id = 1;

enum class PointerKind : std::uint8_t { unknown = 0, mouse, touch, pen };

struct PointerSample {
    PointerId id{0};
    PointerKind kind{PointerKind::unknown};
    Vec2 position{};
};

struct PointerState {
    PointerId id{0};
    PointerKind kind{PointerKind::unknown};
    Vec2 position{};
    Vec2 previous_position{};
    Vec2 delta{};
    bool down{false};
    bool pressed{false};
    bool released{false};
    bool canceled{false};
};

struct TouchGestureSnapshot {
    std::size_t touch_count{0};
    Vec2 centroid{};
    Vec2 centroid_delta{};
    bool pinch_scale_available{false};
    float pinch_scale{1.0F};
};

class VirtualPointerInput {
  public:
    void begin_frame() noexcept;
    void press(PointerSample sample);
    void move(PointerSample sample) noexcept;
    void release(PointerId id) noexcept;
    void cancel(PointerId id) noexcept;

    [[nodiscard]] bool pointer_down(PointerId id) const noexcept;
    [[nodiscard]] bool pointer_pressed(PointerId id) const noexcept;
    [[nodiscard]] bool pointer_released(PointerId id) const noexcept;
    [[nodiscard]] bool pointer_canceled(PointerId id) const noexcept;
    [[nodiscard]] Vec2 pointer_position(PointerId id) const noexcept;
    [[nodiscard]] Vec2 pointer_delta(PointerId id) const noexcept;
    [[nodiscard]] const std::vector<PointerState>& pointers() const noexcept;
    [[nodiscard]] TouchGestureSnapshot touch_gesture() const noexcept;

  private:
    [[nodiscard]] PointerState* find_pointer(PointerId id) noexcept;
    [[nodiscard]] const PointerState* find_pointer(PointerId id) const noexcept;
    void sort_pointers_by_id() noexcept;

    std::vector<PointerState> pointers_;
};

enum class TouchGestureKind : std::uint8_t { unknown = 0, tap, double_tap, long_press, pan, swipe, pinch, rotate };

enum class TouchGesturePhase : std::uint8_t { unknown = 0, began, changed, ended, canceled };

struct TouchGestureEvent {
    TouchGestureKind kind{TouchGestureKind::unknown};
    TouchGesturePhase phase{TouchGesturePhase::unknown};
    PointerId primary_pointer_id{0};
    PointerId secondary_pointer_id{0};
    std::size_t touch_count{0};
    Vec2 centroid{};
    Vec2 delta{};
    Vec2 velocity{};
    float scale{1.0F};
    float rotation_radians{0.0F};
    float elapsed_seconds{0.0F};
};

struct TouchGestureRecognizerConfig {
    float tap_max_seconds{0.25F};
    float double_tap_max_seconds{0.30F};
    float long_press_seconds{0.50F};
    float tap_slop{6.0F};
    float pan_start_slop{8.0F};
    float swipe_min_distance{40.0F};
    float swipe_min_velocity{500.0F};
    float pinch_scale_threshold{0.10F};
    float rotate_radians_threshold{0.20F};
};

class TouchGestureRecognizer {
  public:
    explicit TouchGestureRecognizer(TouchGestureRecognizerConfig config = {}) noexcept;

    void reset() noexcept;

    [[nodiscard]] const TouchGestureRecognizerConfig& config() const noexcept;
    [[nodiscard]] std::vector<TouchGestureEvent> update(const VirtualPointerInput& input, float delta_seconds);

  private:
    TouchGestureRecognizerConfig config_{};

    bool primary_tracking_{false};
    PointerId primary_pointer_id_{0};
    Vec2 primary_start_position_{};
    Vec2 primary_last_position_{};
    float primary_elapsed_seconds_{0.0F};
    float primary_max_distance_{0.0F};
    bool primary_pan_active_{false};
    bool primary_long_press_active_{false};

    bool last_tap_valid_{false};
    Vec2 last_tap_position_{};
    float last_tap_age_seconds_{0.0F};

    bool two_touch_tracking_{false};
    PointerId pair_first_pointer_id_{0};
    PointerId pair_second_pointer_id_{0};
    Vec2 pair_last_centroid_{};
    float pair_start_distance_{0.0F};
    float pair_start_angle_radians_{0.0F};
    float pair_elapsed_seconds_{0.0F};
    bool pair_pinch_active_{false};
    bool pair_rotate_active_{false};
};

using GamepadId = std::uint32_t;

enum class GamepadButton : std::uint8_t {
    unknown = 0,
    south,
    east,
    west,
    north,
    back,
    guide,
    start,
    left_stick,
    right_stick,
    left_shoulder,
    right_shoulder,
    dpad_up,
    dpad_down,
    dpad_left,
    dpad_right,
    count
};

enum class GamepadAxis : std::uint8_t {
    unknown = 0,
    left_x,
    left_y,
    right_x,
    right_y,
    left_trigger,
    right_trigger,
    count
};

struct GamepadButtonState {
    bool down{false};
    bool pressed{false};
    bool released{false};
};

struct GamepadState {
    GamepadId id{0};
    std::array<GamepadButtonState, static_cast<std::size_t>(GamepadButton::count)> buttons{};
    std::array<float, static_cast<std::size_t>(GamepadAxis::count)> axes{};
};

class VirtualGamepadInput {
  public:
    void begin_frame() noexcept;
    void connect(GamepadId id);
    void disconnect(GamepadId id) noexcept;
    void press(GamepadId id, GamepadButton button);
    void release(GamepadId id, GamepadButton button) noexcept;
    void set_axis(GamepadId id, GamepadAxis axis, float value);

    [[nodiscard]] bool gamepad_connected(GamepadId id) const noexcept;
    [[nodiscard]] bool button_down(GamepadId id, GamepadButton button) const noexcept;
    [[nodiscard]] bool button_pressed(GamepadId id, GamepadButton button) const noexcept;
    [[nodiscard]] bool button_released(GamepadId id, GamepadButton button) const noexcept;
    [[nodiscard]] float axis_value(GamepadId id, GamepadAxis axis) const noexcept;
    [[nodiscard]] Vec2 stick(GamepadId id, GamepadAxis x_axis, GamepadAxis y_axis) const noexcept;
    [[nodiscard]] const std::vector<GamepadState>& gamepads() const noexcept;

  private:
    [[nodiscard]] static constexpr std::size_t button_index(GamepadButton button) noexcept {
        return static_cast<std::size_t>(button);
    }

    [[nodiscard]] static constexpr std::size_t axis_index(GamepadAxis axis) noexcept {
        return static_cast<std::size_t>(axis);
    }

    [[nodiscard]] static constexpr bool valid_button(GamepadButton button) noexcept {
        return button != GamepadButton::unknown && button_index(button) < button_index(GamepadButton::count);
    }

    [[nodiscard]] static constexpr bool valid_axis(GamepadAxis axis) noexcept {
        return axis != GamepadAxis::unknown && axis_index(axis) < axis_index(GamepadAxis::count);
    }

    [[nodiscard]] GamepadState* find_gamepad(GamepadId id) noexcept;
    [[nodiscard]] const GamepadState* find_gamepad(GamepadId id) const noexcept;
    [[nodiscard]] GamepadState* find_or_add_gamepad(GamepadId id);
    void sort_gamepads_by_id() noexcept;

    std::vector<GamepadState> gamepads_;
};

} // namespace mirakana
