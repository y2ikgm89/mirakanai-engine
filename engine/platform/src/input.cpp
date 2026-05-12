// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/input.hpp"

#include "mirakana/math/vec.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

namespace mirakana {

namespace {

[[nodiscard]] bool valid_pointer_kind(PointerKind kind) noexcept {
    return kind != PointerKind::unknown;
}

[[nodiscard]] bool finite_position(Vec2 position) noexcept {
    return std::isfinite(position.x) && std::isfinite(position.y);
}

[[nodiscard]] bool valid_pointer_sample(PointerSample sample) noexcept {
    return sample.id != 0 && valid_pointer_kind(sample.kind) && finite_position(sample.position);
}

[[nodiscard]] bool active_touch_pointer(const PointerState& pointer) noexcept {
    return pointer.down && pointer.kind == PointerKind::touch;
}

[[nodiscard]] float positive_finite_seconds(float seconds) noexcept {
    return std::isfinite(seconds) && seconds > 0.0F ? seconds : 0.0F;
}

[[nodiscard]] float distance(Vec2 lhs, Vec2 rhs) noexcept {
    return length(lhs - rhs);
}

[[nodiscard]] Vec2 velocity_for(Vec2 delta, float seconds) noexcept {
    return seconds > 0.0F ? delta * (1.0F / seconds) : Vec2{};
}

[[nodiscard]] float angle_between(Vec2 lhs, Vec2 rhs) noexcept {
    const auto delta = rhs - lhs;
    return std::atan2(delta.y, delta.x);
}

[[nodiscard]] float normalized_radians(float radians) noexcept {
    constexpr float pi = std::numbers::pi_v<float>;
    constexpr float two_pi = pi * 2.0F;
    while (radians > pi) {
        radians -= two_pi;
    }
    while (radians < -pi) {
        radians += two_pi;
    }
    return radians;
}

struct ActiveTouchPair {
    const PointerState* first{nullptr};
    const PointerState* second{nullptr};
    std::size_t active_count{0};
};

[[nodiscard]] ActiveTouchPair active_touch_pair(const VirtualPointerInput& input) noexcept {
    ActiveTouchPair pair;
    for (const auto& pointer : input.pointers()) {
        if (!active_touch_pointer(pointer)) {
            continue;
        }

        if (pair.first == nullptr) {
            pair.first = &pointer;
        } else if (pair.second == nullptr) {
            pair.second = &pointer;
        }
        ++pair.active_count;
    }
    return pair;
}

[[nodiscard]] const PointerState* touch_pointer_row(const VirtualPointerInput& input, PointerId id) noexcept {
    for (const auto& pointer : input.pointers()) {
        if (pointer.id == id && pointer.kind == PointerKind::touch) {
            return &pointer;
        }
    }
    return nullptr;
}

[[nodiscard]] Vec2 midpoint(Vec2 lhs, Vec2 rhs) noexcept {
    return (lhs + rhs) * 0.5F;
}

[[nodiscard]] TouchGestureEvent touch_event(TouchGestureKind kind, TouchGesturePhase phase, PointerId primary_id,
                                            PointerId secondary_id, std::size_t touch_count, Vec2 centroid, Vec2 delta,
                                            Vec2 velocity, float scale, float rotation_radians,
                                            float elapsed_seconds) noexcept {
    return TouchGestureEvent{.kind = kind,
                             .phase = phase,
                             .primary_pointer_id = primary_id,
                             .secondary_pointer_id = secondary_id,
                             .touch_count = touch_count,
                             .centroid = centroid,
                             .delta = delta,
                             .velocity = velocity,
                             .scale = scale,
                             .rotation_radians = rotation_radians,
                             .elapsed_seconds = elapsed_seconds};
}

} // namespace

void VirtualInput::begin_frame() noexcept {
    for (auto& key : keys_) {
        key.pressed = false;
        key.released = false;
    }
}

void VirtualInput::press(Key key) noexcept {
    if (!valid_key(key)) {
        return;
    }

    auto& state = keys_[key_index(key)];
    if (!state.down) {
        state.pressed = true;
    }
    state.down = true;
    state.released = false;
}

void VirtualInput::release(Key key) noexcept {
    if (!valid_key(key)) {
        return;
    }

    auto& state = keys_[key_index(key)];
    if (state.down) {
        state.released = true;
    }
    state.down = false;
    state.pressed = false;
}

bool VirtualInput::key_down(Key key) const noexcept {
    return valid_key(key) && keys_[key_index(key)].down;
}

bool VirtualInput::key_pressed(Key key) const noexcept {
    return valid_key(key) && keys_[key_index(key)].pressed;
}

bool VirtualInput::key_released(Key key) const noexcept {
    return valid_key(key) && keys_[key_index(key)].released;
}

Vec2 VirtualInput::digital_axis(Key negative_x, Key positive_x, Key negative_y, Key positive_y) const noexcept {
    const float x = (key_down(positive_x) ? 1.0F : 0.0F) - (key_down(negative_x) ? 1.0F : 0.0F);
    const float y = (key_down(positive_y) ? 1.0F : 0.0F) - (key_down(negative_y) ? 1.0F : 0.0F);
    return Vec2{.x = x, .y = y};
}

void VirtualPointerInput::begin_frame() noexcept {
    const auto tail = std::ranges::remove_if(pointers_, [](const PointerState& pointer) { return !pointer.down; });
    pointers_.erase(tail.begin(), tail.end());

    for (auto& pointer : pointers_) {
        pointer.previous_position = pointer.position;
        pointer.delta = Vec2{};
        pointer.pressed = false;
        pointer.released = false;
        pointer.canceled = false;
    }
}

void VirtualPointerInput::press(PointerSample sample) {
    if (!valid_pointer_sample(sample)) {
        return;
    }

    if (auto* pointer = find_pointer(sample.id)) {
        if (pointer->down) {
            pointer->delta = pointer->delta + (sample.position - pointer->position);
        } else {
            pointer->previous_position = sample.position;
            pointer->delta = Vec2{};
            pointer->pressed = true;
        }

        pointer->kind = sample.kind;
        pointer->position = sample.position;
        pointer->down = true;
        pointer->released = false;
        pointer->canceled = false;
        return;
    }

    pointers_.push_back(PointerState{
        .id = sample.id,
        .kind = sample.kind,
        .position = sample.position,
        .previous_position = sample.position,
        .delta = Vec2{},
        .down = true,
        .pressed = true,
        .released = false,
        .canceled = false,
    });
    sort_pointers_by_id();
}

void VirtualPointerInput::move(PointerSample sample) noexcept {
    if (!valid_pointer_sample(sample)) {
        return;
    }

    auto* pointer = find_pointer(sample.id);
    if (pointer == nullptr || !pointer->down) {
        return;
    }

    pointer->kind = sample.kind;
    pointer->previous_position = pointer->position;
    pointer->delta = pointer->delta + (sample.position - pointer->position);
    pointer->position = sample.position;
}

void VirtualPointerInput::release(PointerId id) noexcept {
    auto* pointer = find_pointer(id);
    if (pointer == nullptr || !pointer->down) {
        return;
    }

    pointer->down = false;
    pointer->pressed = false;
    pointer->released = true;
    pointer->canceled = false;
}

void VirtualPointerInput::cancel(PointerId id) noexcept {
    auto* pointer = find_pointer(id);
    if (pointer == nullptr || !pointer->down) {
        return;
    }

    pointer->down = false;
    pointer->pressed = false;
    pointer->released = false;
    pointer->canceled = true;
}

bool VirtualPointerInput::pointer_down(PointerId id) const noexcept {
    const auto* pointer = find_pointer(id);
    return pointer != nullptr && pointer->down;
}

bool VirtualPointerInput::pointer_pressed(PointerId id) const noexcept {
    const auto* pointer = find_pointer(id);
    return pointer != nullptr && pointer->pressed;
}

bool VirtualPointerInput::pointer_released(PointerId id) const noexcept {
    const auto* pointer = find_pointer(id);
    return pointer != nullptr && pointer->released;
}

bool VirtualPointerInput::pointer_canceled(PointerId id) const noexcept {
    const auto* pointer = find_pointer(id);
    return pointer != nullptr && pointer->canceled;
}

Vec2 VirtualPointerInput::pointer_position(PointerId id) const noexcept {
    const auto* pointer = find_pointer(id);
    return pointer != nullptr ? pointer->position : Vec2{};
}

Vec2 VirtualPointerInput::pointer_delta(PointerId id) const noexcept {
    const auto* pointer = find_pointer(id);
    return pointer != nullptr ? pointer->delta : Vec2{};
}

const std::vector<PointerState>& VirtualPointerInput::pointers() const noexcept {
    return pointers_;
}

TouchGestureSnapshot VirtualPointerInput::touch_gesture() const noexcept {
    TouchGestureSnapshot snapshot;
    Vec2 centroid_sum{};
    Vec2 previous_centroid_sum{};
    const PointerState* first_touch = nullptr;
    const PointerState* second_touch = nullptr;

    for (const auto& pointer : pointers_) {
        if (!active_touch_pointer(pointer)) {
            continue;
        }

        ++snapshot.touch_count;
        centroid_sum = centroid_sum + pointer.position;
        previous_centroid_sum = previous_centroid_sum + pointer.previous_position;
        if (first_touch == nullptr) {
            first_touch = &pointer;
        } else if (second_touch == nullptr) {
            second_touch = &pointer;
        }
    }

    if (snapshot.touch_count == 0) {
        return snapshot;
    }

    const auto scale = 1.0F / static_cast<float>(snapshot.touch_count);
    snapshot.centroid = centroid_sum * scale;
    snapshot.centroid_delta = (centroid_sum - previous_centroid_sum) * scale;

    if (snapshot.touch_count == 2 && first_touch != nullptr && second_touch != nullptr) {
        const auto previous_distance = length(first_touch->previous_position - second_touch->previous_position);
        if (std::isfinite(previous_distance) && previous_distance > 0.0F) {
            const auto current_distance = length(first_touch->position - second_touch->position);
            if (std::isfinite(current_distance)) {
                snapshot.pinch_scale_available = true;
                snapshot.pinch_scale = current_distance / previous_distance;
            }
        }
    }

    return snapshot;
}

PointerState* VirtualPointerInput::find_pointer(PointerId id) noexcept {
    const auto it = std::ranges::find_if(pointers_, [id](const PointerState& pointer) { return pointer.id == id; });
    return it != pointers_.end() ? &(*it) : nullptr;
}

const PointerState* VirtualPointerInput::find_pointer(PointerId id) const noexcept {
    const auto it = std::ranges::find_if(pointers_, [id](const PointerState& pointer) { return pointer.id == id; });
    return it != pointers_.end() ? &(*it) : nullptr;
}

void VirtualPointerInput::sort_pointers_by_id() noexcept {
    std::ranges::sort(pointers_, [](const PointerState& lhs, const PointerState& rhs) { return lhs.id < rhs.id; });
}

TouchGestureRecognizer::TouchGestureRecognizer(TouchGestureRecognizerConfig config) noexcept : config_(config) {}

void TouchGestureRecognizer::reset() noexcept {
    primary_tracking_ = false;
    primary_pointer_id_ = 0;
    primary_start_position_ = Vec2{};
    primary_last_position_ = Vec2{};
    primary_elapsed_seconds_ = 0.0F;
    primary_max_distance_ = 0.0F;
    primary_pan_active_ = false;
    primary_long_press_active_ = false;

    last_tap_valid_ = false;
    last_tap_position_ = Vec2{};
    last_tap_age_seconds_ = 0.0F;

    two_touch_tracking_ = false;
    pair_first_pointer_id_ = 0;
    pair_second_pointer_id_ = 0;
    pair_last_centroid_ = Vec2{};
    pair_start_distance_ = 0.0F;
    pair_start_angle_radians_ = 0.0F;
    pair_elapsed_seconds_ = 0.0F;
    pair_pinch_active_ = false;
    pair_rotate_active_ = false;
}

const TouchGestureRecognizerConfig& TouchGestureRecognizer::config() const noexcept {
    return config_;
}

std::vector<TouchGestureEvent> TouchGestureRecognizer::update(const VirtualPointerInput& input, float delta_seconds) {
    const float frame_seconds = positive_finite_seconds(delta_seconds);
    std::vector<TouchGestureEvent> events;

    if (last_tap_valid_) {
        last_tap_age_seconds_ += frame_seconds;
        if (last_tap_age_seconds_ > config_.double_tap_max_seconds) {
            last_tap_valid_ = false;
        }
    }

    const auto active_pair = active_touch_pair(input);

    if (two_touch_tracking_ && active_pair.active_count != 2) {
        const auto* first_row = touch_pointer_row(input, pair_first_pointer_id_);
        const auto* second_row = touch_pointer_row(input, pair_second_pointer_id_);
        const bool canceled =
            (first_row != nullptr && first_row->canceled) || (second_row != nullptr && second_row->canceled);
        const auto phase = canceled ? TouchGesturePhase::canceled : TouchGesturePhase::ended;

        if (pair_pinch_active_) {
            events.push_back(touch_event(TouchGestureKind::pinch, phase, pair_first_pointer_id_,
                                         pair_second_pointer_id_, 2, pair_last_centroid_, Vec2{}, Vec2{}, 1.0F, 0.0F,
                                         pair_elapsed_seconds_));
        }
        if (pair_rotate_active_) {
            events.push_back(touch_event(TouchGestureKind::rotate, phase, pair_first_pointer_id_,
                                         pair_second_pointer_id_, 2, pair_last_centroid_, Vec2{}, Vec2{}, 1.0F, 0.0F,
                                         pair_elapsed_seconds_));
        }

        two_touch_tracking_ = false;
        pair_pinch_active_ = false;
        pair_rotate_active_ = false;
        return events;
    }

    if (active_pair.active_count == 2 && active_pair.first != nullptr && active_pair.second != nullptr) {
        if (primary_tracking_) {
            if (primary_pan_active_) {
                events.push_back(touch_event(TouchGestureKind::pan, TouchGesturePhase::canceled, primary_pointer_id_, 0,
                                             1, primary_last_position_, Vec2{}, Vec2{}, 1.0F, 0.0F,
                                             primary_elapsed_seconds_));
            }
            if (primary_long_press_active_) {
                events.push_back(touch_event(TouchGestureKind::long_press, TouchGesturePhase::canceled,
                                             primary_pointer_id_, 0, 1, primary_last_position_, Vec2{}, Vec2{}, 1.0F,
                                             0.0F, primary_elapsed_seconds_));
            }
            primary_tracking_ = false;
            primary_pan_active_ = false;
            primary_long_press_active_ = false;
        }

        const auto first_id = active_pair.first->id;
        const auto second_id = active_pair.second->id;
        const auto centroid = midpoint(active_pair.first->position, active_pair.second->position);
        const auto current_distance = distance(active_pair.first->position, active_pair.second->position);
        const auto current_angle = angle_between(active_pair.first->position, active_pair.second->position);

        if (!two_touch_tracking_ || pair_first_pointer_id_ != first_id || pair_second_pointer_id_ != second_id) {
            two_touch_tracking_ = true;
            pair_first_pointer_id_ = first_id;
            pair_second_pointer_id_ = second_id;
            pair_last_centroid_ = centroid;
            pair_start_distance_ = current_distance;
            pair_start_angle_radians_ = current_angle;
            pair_elapsed_seconds_ = 0.0F;
            pair_pinch_active_ = false;
            pair_rotate_active_ = false;
            return events;
        }

        pair_elapsed_seconds_ += frame_seconds;
        const auto centroid_delta = centroid - pair_last_centroid_;

        if (std::isfinite(current_distance) && std::isfinite(pair_start_distance_) && pair_start_distance_ > 0.0F) {
            const auto scale = current_distance / pair_start_distance_;
            const bool should_begin_pinch =
                !pair_pinch_active_ && std::fabs(scale - 1.0F) >= config_.pinch_scale_threshold;
            if (should_begin_pinch || pair_pinch_active_) {
                events.push_back(touch_event(
                    TouchGestureKind::pinch, should_begin_pinch ? TouchGesturePhase::began : TouchGesturePhase::changed,
                    first_id, second_id, 2, centroid, centroid_delta, velocity_for(centroid_delta, frame_seconds),
                    scale, 0.0F, pair_elapsed_seconds_));
                pair_pinch_active_ = true;
            }
        }

        const auto rotation = normalized_radians(current_angle - pair_start_angle_radians_);
        const bool should_begin_rotate =
            !pair_rotate_active_ && std::fabs(rotation) >= config_.rotate_radians_threshold;
        if (should_begin_rotate || pair_rotate_active_) {
            events.push_back(touch_event(
                TouchGestureKind::rotate, should_begin_rotate ? TouchGesturePhase::began : TouchGesturePhase::changed,
                first_id, second_id, 2, centroid, centroid_delta, velocity_for(centroid_delta, frame_seconds), 1.0F,
                rotation, pair_elapsed_seconds_));
            pair_rotate_active_ = true;
        }

        pair_last_centroid_ = centroid;
        return events;
    }

    if (!primary_tracking_ && active_pair.active_count == 1 && active_pair.first != nullptr) {
        primary_tracking_ = true;
        primary_pointer_id_ = active_pair.first->id;
        primary_start_position_ = active_pair.first->position;
        primary_last_position_ = active_pair.first->position;
        primary_elapsed_seconds_ = 0.0F;
        primary_max_distance_ = 0.0F;
        primary_pan_active_ = false;
        primary_long_press_active_ = false;
        return events;
    }

    if (!primary_tracking_) {
        return events;
    }

    const auto* primary = touch_pointer_row(input, primary_pointer_id_);
    if (primary == nullptr) {
        primary_tracking_ = false;
        primary_pan_active_ = false;
        primary_long_press_active_ = false;
        return events;
    }

    primary_elapsed_seconds_ += frame_seconds;
    const auto total_delta = primary->position - primary_start_position_;
    const auto total_distance = length(total_delta);
    if (std::isfinite(total_distance)) {
        primary_max_distance_ = std::max(primary_max_distance_, total_distance);
    }

    if (primary->canceled) {
        if (primary_pan_active_) {
            events.push_back(touch_event(TouchGestureKind::pan, TouchGesturePhase::canceled, primary_pointer_id_, 0, 1,
                                         primary->position, Vec2{}, Vec2{}, 1.0F, 0.0F, primary_elapsed_seconds_));
        }
        if (primary_long_press_active_) {
            events.push_back(touch_event(TouchGestureKind::long_press, TouchGesturePhase::canceled, primary_pointer_id_,
                                         0, 1, primary->position, Vec2{}, Vec2{}, 1.0F, 0.0F,
                                         primary_elapsed_seconds_));
        }
        primary_tracking_ = false;
        primary_pan_active_ = false;
        primary_long_press_active_ = false;
        return events;
    }

    if (primary->released) {
        if (primary_pan_active_) {
            events.push_back(touch_event(TouchGestureKind::pan, TouchGesturePhase::ended, primary_pointer_id_, 0, 1,
                                         primary->position, total_delta,
                                         velocity_for(total_delta, primary_elapsed_seconds_), 1.0F, 0.0F,
                                         primary_elapsed_seconds_));
            const auto speed = length(velocity_for(total_delta, primary_elapsed_seconds_));
            if (total_distance >= config_.swipe_min_distance && speed >= config_.swipe_min_velocity) {
                events.push_back(touch_event(TouchGestureKind::swipe, TouchGesturePhase::ended, primary_pointer_id_, 0,
                                             1, primary->position, total_delta,
                                             velocity_for(total_delta, primary_elapsed_seconds_), 1.0F, 0.0F,
                                             primary_elapsed_seconds_));
            }
        } else if (primary_long_press_active_) {
            events.push_back(touch_event(TouchGestureKind::long_press, TouchGesturePhase::ended, primary_pointer_id_, 0,
                                         1, primary->position, Vec2{}, Vec2{}, 1.0F, 0.0F, primary_elapsed_seconds_));
        } else if (primary_elapsed_seconds_ <= config_.tap_max_seconds && primary_max_distance_ <= config_.tap_slop) {
            if (last_tap_valid_ && last_tap_age_seconds_ <= config_.double_tap_max_seconds &&
                distance(last_tap_position_, primary->position) <= config_.tap_slop) {
                events.push_back(touch_event(TouchGestureKind::double_tap, TouchGesturePhase::ended,
                                             primary_pointer_id_, 0, 1, primary->position, Vec2{}, Vec2{}, 1.0F, 0.0F,
                                             primary_elapsed_seconds_));
                last_tap_valid_ = false;
            } else {
                events.push_back(touch_event(TouchGestureKind::tap, TouchGesturePhase::ended, primary_pointer_id_, 0, 1,
                                             primary->position, Vec2{}, Vec2{}, 1.0F, 0.0F, primary_elapsed_seconds_));
                last_tap_valid_ = true;
                last_tap_position_ = primary->position;
                last_tap_age_seconds_ = 0.0F;
            }
        }

        primary_tracking_ = false;
        primary_pan_active_ = false;
        primary_long_press_active_ = false;
        return events;
    }

    if (active_pair.active_count != 1) {
        primary_tracking_ = false;
        primary_pan_active_ = false;
        primary_long_press_active_ = false;
        return events;
    }

    if (!primary_pan_active_ && primary_max_distance_ >= config_.pan_start_slop) {
        primary_pan_active_ = true;
        events.push_back(touch_event(TouchGestureKind::pan, TouchGesturePhase::began, primary_pointer_id_, 0, 1,
                                     primary->position, primary->delta, velocity_for(primary->delta, frame_seconds),
                                     1.0F, 0.0F, primary_elapsed_seconds_));
    } else if (primary_pan_active_ && primary->delta != Vec2{}) {
        events.push_back(touch_event(TouchGestureKind::pan, TouchGesturePhase::changed, primary_pointer_id_, 0, 1,
                                     primary->position, primary->delta, velocity_for(primary->delta, frame_seconds),
                                     1.0F, 0.0F, primary_elapsed_seconds_));
    }

    if (!primary_pan_active_ && !primary_long_press_active_ && primary_elapsed_seconds_ >= config_.long_press_seconds &&
        primary_max_distance_ <= config_.tap_slop) {
        primary_long_press_active_ = true;
        events.push_back(touch_event(TouchGestureKind::long_press, TouchGesturePhase::began, primary_pointer_id_, 0, 1,
                                     primary->position, Vec2{}, Vec2{}, 1.0F, 0.0F, primary_elapsed_seconds_));
    }

    primary_last_position_ = primary->position;
    return events;
}

void VirtualGamepadInput::begin_frame() noexcept {
    for (auto& gamepad : gamepads_) {
        for (auto& button : gamepad.buttons) {
            button.pressed = false;
            button.released = false;
        }
    }
}

void VirtualGamepadInput::connect(GamepadId id) {
    (void)find_or_add_gamepad(id);
}

void VirtualGamepadInput::disconnect(GamepadId id) noexcept {
    const auto tail = std::ranges::remove_if(gamepads_, [id](const GamepadState& gamepad) { return gamepad.id == id; });
    gamepads_.erase(tail.begin(), tail.end());
}

void VirtualGamepadInput::press(GamepadId id, GamepadButton button) {
    if (!valid_button(button)) {
        return;
    }

    auto* gamepad = find_or_add_gamepad(id);
    if (gamepad == nullptr) {
        return;
    }

    auto& state = gamepad->buttons[button_index(button)];
    if (!state.down) {
        state.pressed = true;
    }
    state.down = true;
    state.released = false;
}

void VirtualGamepadInput::release(GamepadId id, GamepadButton button) noexcept {
    if (!valid_button(button)) {
        return;
    }

    auto* gamepad = find_gamepad(id);
    if (gamepad == nullptr) {
        return;
    }

    auto& state = gamepad->buttons[button_index(button)];
    if (state.down) {
        state.released = true;
    }
    state.down = false;
    state.pressed = false;
}

void VirtualGamepadInput::set_axis(GamepadId id, GamepadAxis axis, float value) {
    if (!valid_axis(axis) || !std::isfinite(value)) {
        return;
    }

    auto* gamepad = find_or_add_gamepad(id);
    if (gamepad == nullptr) {
        return;
    }

    gamepad->axes[axis_index(axis)] = std::clamp(value, -1.0F, 1.0F);
}

bool VirtualGamepadInput::gamepad_connected(GamepadId id) const noexcept {
    return find_gamepad(id) != nullptr;
}

bool VirtualGamepadInput::button_down(GamepadId id, GamepadButton button) const noexcept {
    const auto* gamepad = find_gamepad(id);
    return gamepad != nullptr && valid_button(button) && gamepad->buttons[button_index(button)].down;
}

bool VirtualGamepadInput::button_pressed(GamepadId id, GamepadButton button) const noexcept {
    const auto* gamepad = find_gamepad(id);
    return gamepad != nullptr && valid_button(button) && gamepad->buttons[button_index(button)].pressed;
}

bool VirtualGamepadInput::button_released(GamepadId id, GamepadButton button) const noexcept {
    const auto* gamepad = find_gamepad(id);
    return gamepad != nullptr && valid_button(button) && gamepad->buttons[button_index(button)].released;
}

float VirtualGamepadInput::axis_value(GamepadId id, GamepadAxis axis) const noexcept {
    const auto* gamepad = find_gamepad(id);
    return gamepad != nullptr && valid_axis(axis) ? gamepad->axes[axis_index(axis)] : 0.0F;
}

Vec2 VirtualGamepadInput::stick(GamepadId id, GamepadAxis x_axis, GamepadAxis y_axis) const noexcept {
    return Vec2{.x = axis_value(id, x_axis), .y = axis_value(id, y_axis)};
}

const std::vector<GamepadState>& VirtualGamepadInput::gamepads() const noexcept {
    return gamepads_;
}

GamepadState* VirtualGamepadInput::find_gamepad(GamepadId id) noexcept {
    const auto it = std::ranges::find_if(gamepads_, [id](const GamepadState& gamepad) { return gamepad.id == id; });
    return it != gamepads_.end() ? &(*it) : nullptr;
}

const GamepadState* VirtualGamepadInput::find_gamepad(GamepadId id) const noexcept {
    const auto it = std::ranges::find_if(gamepads_, [id](const GamepadState& gamepad) { return gamepad.id == id; });
    return it != gamepads_.end() ? &(*it) : nullptr;
}

GamepadState* VirtualGamepadInput::find_or_add_gamepad(GamepadId id) {
    if (id == 0) {
        return nullptr;
    }

    if (auto* gamepad = find_gamepad(id)) {
        return gamepad;
    }

    gamepads_.push_back(GamepadState{.id = id});
    sort_gamepads_by_id();
    return find_gamepad(id);
}

void VirtualGamepadInput::sort_gamepads_by_id() noexcept {
    std::ranges::sort(gamepads_, [](const GamepadState& lhs, const GamepadState& rhs) { return lhs.id < rhs.id; });
}

} // namespace mirakana
