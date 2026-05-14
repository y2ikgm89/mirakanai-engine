// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/session_services.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <functional>
#include <limits>
#include <locale>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

constexpr std::string_view save_data_format = "GameEngine.RuntimeSaveData.v1";
constexpr std::string_view settings_format = "GameEngine.RuntimeSettings.v1";
constexpr std::string_view localization_format = "GameEngine.RuntimeLocalizationCatalog.v1";
constexpr std::string_view input_actions_format = "GameEngine.RuntimeInputActions.v4";
constexpr std::string_view input_rebinding_profile_format = "GameEngine.RuntimeInputRebindingProfile.v1";

[[nodiscard]] bool contains_control_characters(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char character) {
        const auto byte = static_cast<unsigned char>(character);
        return byte < 0x20U || byte == 0x7FU;
    });
}

void validate_session_path(std::string_view path) {
    if (path.empty()) {
        throw std::invalid_argument("runtime session path must not be empty");
    }
    if (contains_control_characters(path)) {
        throw std::invalid_argument("runtime session path must not contain control characters");
    }
    if (path.front() == '/' || path.find('\\') != std::string_view::npos || path.find(':') != std::string_view::npos ||
        path.find("..") != std::string_view::npos) {
        throw std::invalid_argument("runtime session path must be project relative");
    }
}

void validate_entry_key(std::string_view key, std::string_view diagnostic_name) {
    if (key.empty()) {
        throw std::invalid_argument(std::string(diagnostic_name) + " key must not be empty");
    }
    if (contains_control_characters(key) || key.find('=') != std::string_view::npos ||
        key.find(',') != std::string_view::npos) {
        throw std::invalid_argument(std::string(diagnostic_name) + " key contains an unsupported character");
    }
}

void validate_entry_value(std::string_view value, std::string_view diagnostic_name) {
    if (contains_control_characters(value)) {
        throw std::invalid_argument(std::string(diagnostic_name) + " value contains a control character");
    }
}

void validate_locale(std::string_view locale) {
    if (locale.empty()) {
        throw std::invalid_argument("runtime localization locale must not be empty");
    }
    if (contains_control_characters(locale) || locale.find('=') != std::string_view::npos) {
        throw std::invalid_argument("runtime localization locale contains an unsupported character");
    }
}

[[nodiscard]] std::uint32_t parse_u32(std::string_view value, std::string_view diagnostic_name) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size() ||
        parsed > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(std::string(diagnostic_name) + " integer value is invalid");
    }
    return static_cast<std::uint32_t>(parsed);
}

[[nodiscard]] float parse_float(std::string_view value, std::string_view diagnostic_name) {
    const auto valid_character = [](char character) noexcept {
        return (character >= '0' && character <= '9') || character == '+' || character == '-' || character == '.' ||
               character == 'e' || character == 'E';
    };
    if (value.empty() || !std::ranges::all_of(value, valid_character)) {
        throw std::invalid_argument(std::string(diagnostic_name) + " float value is invalid");
    }

    float parsed = 0.0F;
    std::istringstream stream{std::string{value}};
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> parsed;

    char trailing = '\0';
    if (!stream || (stream >> trailing) || !std::isfinite(parsed)) {
        throw std::invalid_argument(std::string(diagnostic_name) + " float value is invalid");
    }
    return parsed;
}

[[nodiscard]] std::string format_float(float value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("runtime input action axis float value is invalid");
    }
    if (value == 0.0F) {
        value = 0.0F;
    }
    std::array<char, 32> buffer{};
    const auto [end, error] = std::to_chars(std::to_address(buffer.begin()), std::to_address(buffer.end()), value);
    if (error != std::errc{}) {
        throw std::invalid_argument("runtime input action axis float value could not be serialized");
    }
    return std::string(buffer.data(), end);
}

[[nodiscard]] std::string make_line(std::string_view key, std::string_view value) {
    std::string line(key);
    line.push_back('=');
    line.append(value);
    line.push_back('\n');
    return line;
}

template <typename Entry> void sort_entries_by_key(std::vector<Entry>& entries) {
    std::ranges::sort(entries, [](const Entry& lhs, const Entry& rhs) { return lhs.key < rhs.key; });
}

template <typename Entry>
[[nodiscard]] const std::string* find_entry_value(const std::vector<Entry>& entries, std::string_view key) noexcept {
    const auto it = std::ranges::lower_bound(entries, key, std::ranges::less{}, &Entry::key);
    if (it == entries.end() || it->key != key) {
        return nullptr;
    }
    if constexpr (requires { it->value; }) {
        return &it->value;
    } else {
        return &it->text;
    }
}

template <typename Entry>
void set_sorted_entry(std::vector<Entry>& entries, std::string key, std::string value,
                      std::string_view diagnostic_name) {
    validate_entry_key(key, diagnostic_name);
    validate_entry_value(value, diagnostic_name);

    const auto it = std::ranges::lower_bound(entries, key, std::ranges::less{}, &Entry::key);
    if (it != entries.end() && it->key == key) {
        if constexpr (requires { it->value; }) {
            it->value = std::move(value);
        } else {
            it->text = std::move(value);
        }
        return;
    }
    if constexpr (requires { Entry{std::string{}, std::string{}}; }) {
        entries.insert(it, Entry{std::move(key), std::move(value)});
    }
}

template <typename Document>
[[nodiscard]] std::string serialize_key_value_document(std::string_view format, const Document& document) {
    std::string text;
    text.append(make_line("format", format));
    text.append(make_line("schema.version", std::to_string(document.schema_version)));
    for (const auto& entry : document.entries()) {
        validate_entry_key(entry.key, "runtime session");
        validate_entry_value(entry.value, "runtime session");
        std::string key("entry.");
        key.append(entry.key);
        text.append(make_line(key, entry.value));
    }
    return text;
}

template <typename Document>
[[nodiscard]] Document deserialize_key_value_document(std::string_view text, std::string_view format,
                                                      std::string_view diagnostic_name) {
    Document document;
    bool saw_format = false;
    bool saw_schema = false;
    std::unordered_set<std::string> seen_entries;

    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto raw_line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                         : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
        if (raw_line.empty()) {
            continue;
        }
        if (raw_line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument(std::string(diagnostic_name) + " line contains carriage return");
        }
        const auto separator = raw_line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument(std::string(diagnostic_name) + " line is missing '='");
        }
        const auto key = raw_line.substr(0, separator);
        const auto value = raw_line.substr(separator + 1U);
        validate_entry_value(value, diagnostic_name);

        if (key == "format") {
            if (saw_format) {
                throw std::invalid_argument(std::string(diagnostic_name) + " format is duplicate");
            }
            saw_format = true;
            if (value != format) {
                throw std::invalid_argument(std::string(diagnostic_name) + " format is unsupported");
            }
            continue;
        }
        if (key == "schema.version") {
            if (saw_schema) {
                throw std::invalid_argument(std::string(diagnostic_name) + " schema version is duplicate");
            }
            saw_schema = true;
            document.schema_version = parse_u32(value, diagnostic_name);
            continue;
        }
        if (key.starts_with("entry.")) {
            const auto entry_key = key.substr(std::string_view("entry.").size());
            validate_entry_key(entry_key, diagnostic_name);
            if (!seen_entries.insert(std::string(entry_key)).second) {
                throw std::invalid_argument(std::string(diagnostic_name) + " contains duplicate entry key");
            }
            document.set_value(std::string(entry_key), std::string(value));
            continue;
        }
        throw std::invalid_argument(std::string(diagnostic_name) + " contains an unsupported key");
    }

    if (!saw_format) {
        throw std::invalid_argument(std::string(diagnostic_name) + " format is missing");
    }
    if (!saw_schema) {
        throw std::invalid_argument(std::string(diagnostic_name) + " schema version is missing");
    }
    return document;
}

[[nodiscard]] std::string_view key_name(Key key) noexcept {
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
    case Key::backspace:
        return "backspace";
    case Key::delete_key:
        return "delete";
    case Key::home:
        return "home";
    case Key::end:
        return "end";
    case Key::unknown:
    case Key::count:
        return {};
    }
    return {};
}

[[nodiscard]] Key parse_key_name(std::string_view name) {
    if (name == "left") {
        return Key::left;
    }
    if (name == "right") {
        return Key::right;
    }
    if (name == "up") {
        return Key::up;
    }
    if (name == "down") {
        return Key::down;
    }
    if (name == "space") {
        return Key::space;
    }
    if (name == "escape") {
        return Key::escape;
    }
    if (name == "backspace") {
        return Key::backspace;
    }
    if (name == "delete") {
        return Key::delete_key;
    }
    if (name == "home") {
        return Key::home;
    }
    if (name == "end") {
        return Key::end;
    }
    throw std::invalid_argument("runtime input action key name is unsupported");
}

[[nodiscard]] std::string_view gamepad_button_name(GamepadButton button) noexcept {
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
        return {};
    }
    return {};
}

[[nodiscard]] GamepadButton parse_gamepad_button_name(std::string_view name) {
    if (name == "south") {
        return GamepadButton::south;
    }
    if (name == "east") {
        return GamepadButton::east;
    }
    if (name == "west") {
        return GamepadButton::west;
    }
    if (name == "north") {
        return GamepadButton::north;
    }
    if (name == "back") {
        return GamepadButton::back;
    }
    if (name == "guide") {
        return GamepadButton::guide;
    }
    if (name == "start") {
        return GamepadButton::start;
    }
    if (name == "left_stick") {
        return GamepadButton::left_stick;
    }
    if (name == "right_stick") {
        return GamepadButton::right_stick;
    }
    if (name == "left_shoulder") {
        return GamepadButton::left_shoulder;
    }
    if (name == "right_shoulder") {
        return GamepadButton::right_shoulder;
    }
    if (name == "dpad_up") {
        return GamepadButton::dpad_up;
    }
    if (name == "dpad_down") {
        return GamepadButton::dpad_down;
    }
    if (name == "dpad_left") {
        return GamepadButton::dpad_left;
    }
    if (name == "dpad_right") {
        return GamepadButton::dpad_right;
    }
    throw std::invalid_argument("runtime input action gamepad button name is unsupported");
}

[[nodiscard]] std::string_view gamepad_axis_name(GamepadAxis axis) noexcept {
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
        return {};
    }
    return {};
}

[[nodiscard]] GamepadAxis parse_gamepad_axis_name(std::string_view name) {
    if (name == "left_x") {
        return GamepadAxis::left_x;
    }
    if (name == "left_y") {
        return GamepadAxis::left_y;
    }
    if (name == "right_x") {
        return GamepadAxis::right_x;
    }
    if (name == "right_y") {
        return GamepadAxis::right_y;
    }
    if (name == "left_trigger") {
        return GamepadAxis::left_trigger;
    }
    if (name == "right_trigger") {
        return GamepadAxis::right_trigger;
    }
    throw std::invalid_argument("runtime input action gamepad axis name is unsupported");
}

void validate_action_name(std::string_view action) {
    validate_entry_key(action, "runtime input action");
    if (action.find('.') != std::string_view::npos || action.find(' ') != std::string_view::npos) {
        throw std::invalid_argument("runtime input action name contains an unsupported character");
    }
}

void validate_context_name(std::string_view context) {
    validate_entry_key(context, "runtime input action context");
    if (context.find('.') != std::string_view::npos || context.find(' ') != std::string_view::npos) {
        throw std::invalid_argument("runtime input action context contains an unsupported character");
    }
}

void validate_key(Key key) {
    if (key_name(key).empty()) {
        throw std::invalid_argument("runtime input action key is unsupported");
    }
}

void validate_pointer_id(PointerId pointer_id) {
    if (pointer_id == 0) {
        throw std::invalid_argument("runtime input action pointer id is invalid");
    }
}

void validate_gamepad_button(GamepadId gamepad_id, GamepadButton button) {
    if (gamepad_id == 0) {
        throw std::invalid_argument("runtime input action gamepad id is invalid");
    }
    if (gamepad_button_name(button).empty()) {
        throw std::invalid_argument("runtime input action gamepad button is unsupported");
    }
}

void validate_key_axis(Key negative_key, Key positive_key) {
    validate_key(negative_key);
    validate_key(positive_key);
    if (negative_key == positive_key) {
        throw std::invalid_argument("runtime input action key axis must use distinct keys");
    }
}

void validate_gamepad_axis(GamepadId gamepad_id, GamepadAxis axis, float scale, float deadzone) {
    if (gamepad_id == 0) {
        throw std::invalid_argument("runtime input action gamepad id is invalid");
    }
    if (gamepad_axis_name(axis).empty()) {
        throw std::invalid_argument("runtime input action gamepad axis is unsupported");
    }
    if (!std::isfinite(scale) || scale < -1.0F || scale > 1.0F || scale == 0.0F) {
        throw std::invalid_argument("runtime input action axis scale is invalid");
    }
    if (!std::isfinite(deadzone) || deadzone < 0.0F || deadzone >= 1.0F) {
        throw std::invalid_argument("runtime input action axis deadzone is invalid");
    }
}

void validate_trigger(const RuntimeInputActionTrigger& trigger) {
    switch (trigger.kind) {
    case RuntimeInputActionTriggerKind::key:
        validate_key(trigger.key);
        return;
    case RuntimeInputActionTriggerKind::pointer:
        validate_pointer_id(trigger.pointer_id);
        return;
    case RuntimeInputActionTriggerKind::gamepad_button:
        validate_gamepad_button(trigger.gamepad_id, trigger.gamepad_button);
        return;
    }
    throw std::invalid_argument("runtime input action trigger kind is unsupported");
}

void validate_axis_source(const RuntimeInputAxisSource& source) {
    switch (source.kind) {
    case RuntimeInputAxisSourceKind::key_pair:
        validate_key_axis(source.negative_key, source.positive_key);
        return;
    case RuntimeInputAxisSourceKind::gamepad_axis:
        validate_gamepad_axis(source.gamepad_id, source.gamepad_axis, source.scale, source.deadzone);
        return;
    }
    throw std::invalid_argument("runtime input action axis source kind is unsupported");
}

[[nodiscard]] int trigger_kind_order(RuntimeInputActionTriggerKind kind) noexcept {
    switch (kind) {
    case RuntimeInputActionTriggerKind::key:
        return 0;
    case RuntimeInputActionTriggerKind::pointer:
        return 1;
    case RuntimeInputActionTriggerKind::gamepad_button:
        return 2;
    }
    return 3;
}

[[nodiscard]] bool same_trigger(const RuntimeInputActionTrigger& lhs, const RuntimeInputActionTrigger& rhs) noexcept {
    if (lhs.kind != rhs.kind) {
        return false;
    }
    switch (lhs.kind) {
    case RuntimeInputActionTriggerKind::key:
        return lhs.key == rhs.key;
    case RuntimeInputActionTriggerKind::pointer:
        return lhs.pointer_id == rhs.pointer_id;
    case RuntimeInputActionTriggerKind::gamepad_button:
        return lhs.gamepad_id == rhs.gamepad_id && lhs.gamepad_button == rhs.gamepad_button;
    }
    return false;
}

[[nodiscard]] bool trigger_less(const RuntimeInputActionTrigger& lhs, const RuntimeInputActionTrigger& rhs) noexcept {
    const auto lhs_kind = trigger_kind_order(lhs.kind);
    const auto rhs_kind = trigger_kind_order(rhs.kind);
    if (lhs_kind != rhs_kind) {
        return lhs_kind < rhs_kind;
    }
    switch (lhs.kind) {
    case RuntimeInputActionTriggerKind::key:
        return key_name(lhs.key) < key_name(rhs.key);
    case RuntimeInputActionTriggerKind::pointer:
        return lhs.pointer_id < rhs.pointer_id;
    case RuntimeInputActionTriggerKind::gamepad_button:
        if (lhs.gamepad_id != rhs.gamepad_id) {
            return lhs.gamepad_id < rhs.gamepad_id;
        }
        return gamepad_button_name(lhs.gamepad_button) < gamepad_button_name(rhs.gamepad_button);
    }
    return false;
}

[[nodiscard]] bool trigger_down(const RuntimeInputActionTrigger& trigger, RuntimeInputStateView state) noexcept {
    switch (trigger.kind) {
    case RuntimeInputActionTriggerKind::key:
        return state.keyboard != nullptr && state.keyboard->key_down(trigger.key);
    case RuntimeInputActionTriggerKind::pointer:
        return state.pointer != nullptr && state.pointer->pointer_down(trigger.pointer_id);
    case RuntimeInputActionTriggerKind::gamepad_button:
        return state.gamepad != nullptr && state.gamepad->button_down(trigger.gamepad_id, trigger.gamepad_button);
    }
    return false;
}

[[nodiscard]] bool trigger_previously_down(const RuntimeInputActionTrigger& trigger,
                                           RuntimeInputStateView state) noexcept {
    switch (trigger.kind) {
    case RuntimeInputActionTriggerKind::key:
        return state.keyboard != nullptr &&
               ((state.keyboard->key_down(trigger.key) && !state.keyboard->key_pressed(trigger.key)) ||
                state.keyboard->key_released(trigger.key));
    case RuntimeInputActionTriggerKind::pointer:
        return state.pointer != nullptr && ((state.pointer->pointer_down(trigger.pointer_id) &&
                                             !state.pointer->pointer_pressed(trigger.pointer_id)) ||
                                            state.pointer->pointer_released(trigger.pointer_id));
    case RuntimeInputActionTriggerKind::gamepad_button:
        return state.gamepad != nullptr &&
               ((state.gamepad->button_down(trigger.gamepad_id, trigger.gamepad_button) &&
                 !state.gamepad->button_pressed(trigger.gamepad_id, trigger.gamepad_button)) ||
                state.gamepad->button_released(trigger.gamepad_id, trigger.gamepad_button));
    }
    return false;
}

[[nodiscard]] std::string serialize_trigger(const RuntimeInputActionTrigger& trigger) {
    validate_trigger(trigger);
    switch (trigger.kind) {
    case RuntimeInputActionTriggerKind::key: {
        std::string value("key:");
        value.append(key_name(trigger.key));
        return value;
    }
    case RuntimeInputActionTriggerKind::pointer:
        return "pointer:" + std::to_string(trigger.pointer_id);
    case RuntimeInputActionTriggerKind::gamepad_button: {
        std::string value("gamepad:");
        value.append(std::to_string(trigger.gamepad_id));
        value.push_back(':');
        value.append(gamepad_button_name(trigger.gamepad_button));
        return value;
    }
    }
    throw std::invalid_argument("runtime input action trigger kind is unsupported");
}

[[nodiscard]] RuntimeInputActionTrigger parse_trigger(std::string_view value) {
    const auto kind_end = value.find(':');
    if (kind_end == std::string_view::npos) {
        throw std::invalid_argument("runtime input action trigger is missing kind separator");
    }

    const auto kind = value.substr(0, kind_end);
    const auto payload = value.substr(kind_end + 1U);
    if (kind == "key") {
        if (payload.empty() || payload.find(':') != std::string_view::npos) {
            throw std::invalid_argument("runtime input action key trigger is invalid");
        }
        return RuntimeInputActionTrigger{
            .kind = RuntimeInputActionTriggerKind::key,
            .key = parse_key_name(payload),
        };
    }
    if (kind == "pointer") {
        if (payload.empty() || payload.find(':') != std::string_view::npos) {
            throw std::invalid_argument("runtime input action pointer trigger is invalid");
        }
        const auto pointer_id = static_cast<PointerId>(parse_u32(payload, "runtime input action pointer id"));
        validate_pointer_id(pointer_id);
        return RuntimeInputActionTrigger{
            .kind = RuntimeInputActionTriggerKind::pointer,
            .key = Key::unknown,
            .pointer_id = pointer_id,
        };
    }
    if (kind == "gamepad") {
        const auto button_separator = payload.find(':');
        if (button_separator == std::string_view::npos || button_separator == 0 ||
            button_separator + 1U >= payload.size() ||
            payload.find(':', button_separator + 1U) != std::string_view::npos) {
            throw std::invalid_argument("runtime input action gamepad trigger is invalid");
        }
        const auto gamepad_id =
            static_cast<GamepadId>(parse_u32(payload.substr(0, button_separator), "runtime input action gamepad id"));
        const auto button = parse_gamepad_button_name(payload.substr(button_separator + 1U));
        validate_gamepad_button(gamepad_id, button);
        return RuntimeInputActionTrigger{
            .kind = RuntimeInputActionTriggerKind::gamepad_button,
            .key = Key::unknown,
            .pointer_id = 0,
            .gamepad_id = gamepad_id,
            .gamepad_button = button,
        };
    }
    throw std::invalid_argument("runtime input action trigger kind is unsupported");
}

[[nodiscard]] int axis_source_kind_order(RuntimeInputAxisSourceKind kind) noexcept {
    switch (kind) {
    case RuntimeInputAxisSourceKind::key_pair:
        return 0;
    case RuntimeInputAxisSourceKind::gamepad_axis:
        return 1;
    }
    return 2;
}

[[nodiscard]] bool same_axis_source(const RuntimeInputAxisSource& lhs, const RuntimeInputAxisSource& rhs) noexcept {
    if (lhs.kind != rhs.kind) {
        return false;
    }
    switch (lhs.kind) {
    case RuntimeInputAxisSourceKind::key_pair:
        return lhs.negative_key == rhs.negative_key && lhs.positive_key == rhs.positive_key;
    case RuntimeInputAxisSourceKind::gamepad_axis:
        return lhs.gamepad_id == rhs.gamepad_id && lhs.gamepad_axis == rhs.gamepad_axis;
    }
    return false;
}

[[nodiscard]] bool axis_source_less(const RuntimeInputAxisSource& lhs, const RuntimeInputAxisSource& rhs) noexcept {
    const auto lhs_kind = axis_source_kind_order(lhs.kind);
    const auto rhs_kind = axis_source_kind_order(rhs.kind);
    if (lhs_kind != rhs_kind) {
        return lhs_kind < rhs_kind;
    }
    switch (lhs.kind) {
    case RuntimeInputAxisSourceKind::key_pair:
        if (key_name(lhs.negative_key) != key_name(rhs.negative_key)) {
            return key_name(lhs.negative_key) < key_name(rhs.negative_key);
        }
        return key_name(lhs.positive_key) < key_name(rhs.positive_key);
    case RuntimeInputAxisSourceKind::gamepad_axis:
        if (lhs.gamepad_id != rhs.gamepad_id) {
            return lhs.gamepad_id < rhs.gamepad_id;
        }
        if (gamepad_axis_name(lhs.gamepad_axis) != gamepad_axis_name(rhs.gamepad_axis)) {
            return gamepad_axis_name(lhs.gamepad_axis) < gamepad_axis_name(rhs.gamepad_axis);
        }
        if (lhs.scale != rhs.scale) {
            return lhs.scale < rhs.scale;
        }
        return lhs.deadzone < rhs.deadzone;
    }
    return false;
}

[[nodiscard]] float apply_axis_deadzone(float value, float deadzone) noexcept {
    value = std::clamp(value, -1.0F, 1.0F);
    const auto magnitude = std::abs(value);
    if (magnitude <= deadzone) {
        return 0.0F;
    }
    const auto normalized = (magnitude - deadzone) / (1.0F - deadzone);
    return value < 0.0F ? -normalized : normalized;
}

[[nodiscard]] float axis_source_value(const RuntimeInputAxisSource& source, RuntimeInputStateView state) noexcept {
    switch (source.kind) {
    case RuntimeInputAxisSourceKind::key_pair:
        if (state.keyboard == nullptr) {
            return 0.0F;
        }
        return (state.keyboard->key_down(source.positive_key) ? 1.0F : 0.0F) -
               (state.keyboard->key_down(source.negative_key) ? 1.0F : 0.0F);
    case RuntimeInputAxisSourceKind::gamepad_axis:
        if (state.gamepad == nullptr) {
            return 0.0F;
        }
        return apply_axis_deadzone(state.gamepad->axis_value(source.gamepad_id, source.gamepad_axis) * source.scale,
                                   source.deadzone);
    }
    return 0.0F;
}

[[nodiscard]] std::string serialize_axis_source(const RuntimeInputAxisSource& source) {
    validate_axis_source(source);
    switch (source.kind) {
    case RuntimeInputAxisSourceKind::key_pair: {
        std::string value("keys:");
        value.append(key_name(source.negative_key));
        value.push_back(':');
        value.append(key_name(source.positive_key));
        return value;
    }
    case RuntimeInputAxisSourceKind::gamepad_axis: {
        std::string value("gamepad:");
        value.append(std::to_string(source.gamepad_id));
        value.push_back(':');
        value.append(gamepad_axis_name(source.gamepad_axis));
        value.push_back(':');
        value.append(format_float(source.scale));
        value.push_back(':');
        value.append(format_float(source.deadzone));
        return value;
    }
    }
    throw std::invalid_argument("runtime input action axis source kind is unsupported");
}

[[nodiscard]] RuntimeInputAxisSource parse_axis_source(std::string_view value) {
    const auto kind_end = value.find(':');
    if (kind_end == std::string_view::npos) {
        throw std::invalid_argument("runtime input action axis source is missing kind separator");
    }

    const auto kind = value.substr(0, kind_end);
    const auto payload = value.substr(kind_end + 1U);
    if (kind == "keys") {
        const auto key_separator = payload.find(':');
        if (key_separator == std::string_view::npos || key_separator == 0 || key_separator + 1U >= payload.size() ||
            payload.find(':', key_separator + 1U) != std::string_view::npos) {
            throw std::invalid_argument("runtime input action key axis source is invalid");
        }
        RuntimeInputAxisSource source;
        source.kind = RuntimeInputAxisSourceKind::key_pair;
        source.negative_key = parse_key_name(payload.substr(0, key_separator));
        source.positive_key = parse_key_name(payload.substr(key_separator + 1U));
        validate_axis_source(source);
        return source;
    }
    if (kind == "gamepad") {
        const auto id_separator = payload.find(':');
        const auto axis_separator =
            id_separator == std::string_view::npos ? std::string_view::npos : payload.find(':', id_separator + 1U);
        const auto scale_separator =
            axis_separator == std::string_view::npos ? std::string_view::npos : payload.find(':', axis_separator + 1U);
        if (id_separator == std::string_view::npos || id_separator == 0 || axis_separator == std::string_view::npos ||
            axis_separator == id_separator + 1U || scale_separator == std::string_view::npos ||
            scale_separator == axis_separator + 1U || scale_separator + 1U >= payload.size() ||
            payload.find(':', scale_separator + 1U) != std::string_view::npos) {
            throw std::invalid_argument("runtime input action gamepad axis source is invalid");
        }

        RuntimeInputAxisSource source;
        source.kind = RuntimeInputAxisSourceKind::gamepad_axis;
        source.gamepad_id =
            static_cast<GamepadId>(parse_u32(payload.substr(0, id_separator), "runtime input action gamepad id"));
        source.gamepad_axis =
            parse_gamepad_axis_name(payload.substr(id_separator + 1U, axis_separator - id_separator - 1U));
        source.scale = parse_float(payload.substr(axis_separator + 1U, scale_separator - axis_separator - 1U),
                                   "runtime input action axis scale");
        source.deadzone = parse_float(payload.substr(scale_separator + 1U), "runtime input action axis deadzone");
        validate_axis_source(source);
        return source;
    }
    throw std::invalid_argument("runtime input action axis source kind is unsupported");
}

struct RuntimeInputBindingDocumentKey {
    std::string_view context;
    std::string_view action;
};

[[nodiscard]] RuntimeInputBindingDocumentKey parse_runtime_input_binding_key(std::string_view key,
                                                                             std::string_view prefix) {
    const auto payload = key.substr(prefix.size());
    const auto separator = payload.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("runtime input actions binding key is missing context/action separator");
    }

    const auto context = payload.substr(0, separator);
    const auto action = payload.substr(separator + 1U);
    validate_context_name(context);
    validate_action_name(action);
    return RuntimeInputBindingDocumentKey{.context = context, .action = action};
}

[[nodiscard]] std::string make_runtime_input_binding_identity(std::string_view context, std::string_view action) {
    std::string identity(context);
    identity.push_back('\n');
    identity.append(action);
    return identity;
}

void validate_profile_id(std::string_view profile_id) {
    validate_entry_key(profile_id, "runtime input rebinding profile id");
    if (profile_id.find('.') != std::string_view::npos || profile_id.find(' ') != std::string_view::npos) {
        throw std::invalid_argument("runtime input rebinding profile id contains an unsupported character");
    }
}

[[nodiscard]] std::string make_rebinding_path(std::string_view prefix, std::string_view context,
                                              std::string_view action) {
    std::string path(prefix);
    path.append(context);
    path.push_back('.');
    path.append(action);
    return path;
}

void add_rebinding_diagnostic(std::vector<RuntimeInputRebindingDiagnostic>& diagnostics,
                              RuntimeInputRebindingDiagnosticCode code, std::string path, std::string message) {
    diagnostics.push_back(
        RuntimeInputRebindingDiagnostic{.code = code, .path = std::move(path), .message = std::move(message)});
}

[[nodiscard]] const RuntimeInputRebindingActionOverride*
find_action_override(const RuntimeInputRebindingProfile& profile, std::string_view context, std::string_view action) {
    const auto it = std::ranges::find_if(profile.action_overrides, [context, action](const auto& override_row) {
        return override_row.context == context && override_row.action == action;
    });
    return it == profile.action_overrides.end() ? nullptr : &*it;
}

[[nodiscard]] const RuntimeInputRebindingAxisOverride*
find_axis_override(const RuntimeInputRebindingProfile& profile, std::string_view context, std::string_view action) {
    const auto it = std::ranges::find_if(profile.axis_overrides, [context, action](const auto& override_row) {
        return override_row.context == context && override_row.action == action;
    });
    return it == profile.axis_overrides.end() ? nullptr : &*it;
}

struct RuntimeInputRebindingTriggerAssignment {
    std::string context;
    std::string action;
    RuntimeInputActionTrigger trigger;
    std::string path;
    bool from_override{false};
};

void collect_trigger_assignments(std::vector<RuntimeInputRebindingTriggerAssignment>& assignments,
                                 std::string_view context, std::string_view action,
                                 const std::vector<RuntimeInputActionTrigger>& triggers, const std::string& path,
                                 bool from_override) {
    for (const auto& trigger : triggers) {
        assignments.push_back(RuntimeInputRebindingTriggerAssignment{.context = std::string(context),
                                                                     .action = std::string(action),
                                                                     .trigger = trigger,
                                                                     .path = path,
                                                                     .from_override = from_override});
    }
}

void diagnose_trigger_conflicts(std::vector<RuntimeInputRebindingDiagnostic>& diagnostics,
                                const std::vector<RuntimeInputRebindingTriggerAssignment>& assignments) {
    std::unordered_set<std::string> reported;
    for (std::size_t lhs_index = 0; lhs_index < assignments.size(); ++lhs_index) {
        const auto& lhs = assignments[lhs_index];
        for (std::size_t rhs_index = lhs_index + 1U; rhs_index < assignments.size(); ++rhs_index) {
            const auto& rhs = assignments[rhs_index];
            if (lhs.context != rhs.context || lhs.action == rhs.action || !same_trigger(lhs.trigger, rhs.trigger) ||
                (!lhs.from_override && !rhs.from_override)) {
                continue;
            }

            const auto path = lhs.from_override ? lhs.path : rhs.path;
            const auto conflict_id = make_runtime_input_binding_identity(lhs.context, path);
            if (!reported.insert(conflict_id).second) {
                continue;
            }
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::trigger_conflict, path,
                                     "runtime input rebinding profile maps one trigger to multiple actions in a "
                                     "context");
        }
    }
}

[[nodiscard]] std::vector<RuntimeInputRebindingDiagnostic>
validate_runtime_input_rebinding_profile_impl(const RuntimeInputActionMap* base,
                                              const RuntimeInputRebindingProfile& profile, bool require_base) {
    std::vector<RuntimeInputRebindingDiagnostic> diagnostics;

    try {
        validate_profile_id(profile.profile_id);
    } catch (const std::exception& error) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_profile_id, "profile.id",
                                 error.what());
    }

    std::unordered_set<std::string> seen_action_overrides;
    std::unordered_set<std::string> seen_axis_overrides;
    std::vector<RuntimeInputRebindingTriggerAssignment> trigger_assignments;

    for (const auto& override_row : profile.action_overrides) {
        const auto path = make_rebinding_path("bind.", override_row.context, override_row.action);
        bool names_valid = true;
        try {
            validate_context_name(override_row.context);
        } catch (const std::exception& error) {
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_context, path,
                                     error.what());
            names_valid = false;
        }
        try {
            validate_action_name(override_row.action);
        } catch (const std::exception& error) {
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_action, path,
                                     error.what());
            names_valid = false;
        }
        if (!names_valid) {
            continue;
        }

        const auto identity = make_runtime_input_binding_identity(override_row.context, override_row.action);
        if (!seen_action_overrides.insert(identity).second) {
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::duplicate_override, path,
                                     "runtime input rebinding profile contains duplicate action override");
        }
        if (require_base && base != nullptr && base->find(override_row.context, override_row.action) == nullptr) {
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::missing_base_binding, path,
                                     "runtime input rebinding profile action override is missing from the base map");
        }
        if (override_row.triggers.empty()) {
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::empty_override, path,
                                     "runtime input rebinding profile action override has no triggers");
            continue;
        }

        std::vector<RuntimeInputActionTrigger> seen_triggers;
        std::vector<RuntimeInputActionTrigger> valid_triggers;
        for (const auto& trigger : override_row.triggers) {
            try {
                validate_trigger(trigger);
            } catch (const std::exception& error) {
                add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_trigger, path,
                                         error.what());
                continue;
            }
            if (std::ranges::any_of(seen_triggers,
                                    [&trigger](const auto& seen) { return same_trigger(seen, trigger); })) {
                add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::duplicate_trigger, path,
                                         "runtime input rebinding profile action override contains duplicate trigger");
                continue;
            }
            seen_triggers.push_back(trigger);
            valid_triggers.push_back(trigger);
        }
        collect_trigger_assignments(trigger_assignments, override_row.context, override_row.action, valid_triggers,
                                    path, true);
    }

    for (const auto& override_row : profile.axis_overrides) {
        const auto path = make_rebinding_path("axis.", override_row.context, override_row.action);
        bool names_valid = true;
        try {
            validate_context_name(override_row.context);
        } catch (const std::exception& error) {
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_context, path,
                                     error.what());
            names_valid = false;
        }
        try {
            validate_action_name(override_row.action);
        } catch (const std::exception& error) {
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_action, path,
                                     error.what());
            names_valid = false;
        }
        if (!names_valid) {
            continue;
        }

        const auto identity = make_runtime_input_binding_identity(override_row.context, override_row.action);
        if (!seen_axis_overrides.insert(identity).second) {
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::duplicate_override, path,
                                     "runtime input rebinding profile contains duplicate axis override");
        }
        if (require_base && base != nullptr && base->find_axis(override_row.context, override_row.action) == nullptr) {
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::missing_base_binding, path,
                                     "runtime input rebinding profile axis override is missing from the base map");
        }
        if (override_row.sources.empty()) {
            add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::empty_override, path,
                                     "runtime input rebinding profile axis override has no sources");
            continue;
        }

        std::vector<RuntimeInputAxisSource> seen_sources;
        for (const auto& source : override_row.sources) {
            try {
                validate_axis_source(source);
            } catch (const std::exception& error) {
                add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_axis_source, path,
                                         error.what());
                continue;
            }
            if (std::ranges::any_of(seen_sources,
                                    [&source](const auto& seen) { return same_axis_source(seen, source); })) {
                add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::duplicate_axis_source, path,
                                         "runtime input rebinding profile axis override contains duplicate source");
                continue;
            }
            seen_sources.push_back(source);
        }
    }

    if (require_base && base != nullptr) {
        for (const auto& binding : base->bindings()) {
            const auto* override_row = find_action_override(profile, binding.context, binding.action);
            if (override_row == nullptr) {
                collect_trigger_assignments(trigger_assignments, binding.context, binding.action, binding.triggers,
                                            make_rebinding_path("bind.", binding.context, binding.action), false);
            }
        }
    }

    diagnose_trigger_conflicts(diagnostics, trigger_assignments);
    return diagnostics;
}

[[nodiscard]] std::vector<RuntimeInputActionTrigger> parse_rebinding_triggers(std::string_view value) {
    if (value.empty()) {
        throw std::invalid_argument("runtime input rebinding profile binding has no triggers");
    }

    std::vector<RuntimeInputActionTrigger> triggers;
    std::size_t trigger_start = 0;
    while (trigger_start <= value.size()) {
        const auto trigger_end = value.find(',', trigger_start);
        const auto raw_trigger =
            value.substr(trigger_start, trigger_end == std::string_view::npos ? value.size() - trigger_start
                                                                              : trigger_end - trigger_start);
        if (raw_trigger.empty()) {
            throw std::invalid_argument("runtime input rebinding profile binding contains an empty trigger");
        }
        triggers.push_back(parse_trigger(raw_trigger));
        if (trigger_end == std::string_view::npos) {
            break;
        }
        trigger_start = trigger_end + 1U;
    }
    return triggers;
}

[[nodiscard]] std::vector<RuntimeInputAxisSource> parse_rebinding_axis_sources(std::string_view value) {
    if (value.empty()) {
        throw std::invalid_argument("runtime input rebinding profile axis binding has no sources");
    }

    std::vector<RuntimeInputAxisSource> sources;
    std::size_t source_start = 0;
    while (source_start <= value.size()) {
        const auto source_end = value.find(',', source_start);
        const auto raw_source =
            value.substr(source_start, source_end == std::string_view::npos ? value.size() - source_start
                                                                            : source_end - source_start);
        if (raw_source.empty()) {
            throw std::invalid_argument("runtime input rebinding profile axis binding contains an empty source");
        }
        sources.push_back(parse_axis_source(raw_source));
        if (source_end == std::string_view::npos) {
            break;
        }
        source_start = source_end + 1U;
    }
    return sources;
}

void bind_trigger_to_action_map(RuntimeInputActionMap& actions, std::string_view context, std::string_view action,
                                const RuntimeInputActionTrigger& trigger) {
    switch (trigger.kind) {
    case RuntimeInputActionTriggerKind::key:
        actions.bind_key_in_context(std::string(context), std::string(action), trigger.key);
        return;
    case RuntimeInputActionTriggerKind::pointer:
        actions.bind_pointer_in_context(std::string(context), std::string(action), trigger.pointer_id);
        return;
    case RuntimeInputActionTriggerKind::gamepad_button:
        actions.bind_gamepad_button_in_context(std::string(context), std::string(action), trigger.gamepad_id,
                                               trigger.gamepad_button);
        return;
    }
    throw std::invalid_argument("runtime input action trigger kind is unsupported");
}

void bind_axis_source_to_action_map(RuntimeInputActionMap& actions, std::string_view context, std::string_view action,
                                    const RuntimeInputAxisSource& source) {
    switch (source.kind) {
    case RuntimeInputAxisSourceKind::key_pair:
        actions.bind_key_axis_in_context(std::string(context), std::string(action), source.negative_key,
                                         source.positive_key);
        return;
    case RuntimeInputAxisSourceKind::gamepad_axis:
        actions.bind_gamepad_axis_in_context(std::string(context), std::string(action), source.gamepad_id,
                                             source.gamepad_axis, source.scale, source.deadzone);
        return;
    }
    throw std::invalid_argument("runtime input action axis source kind is unsupported");
}

[[nodiscard]] std::vector<RuntimeInputRebindingDiagnostic>
diagnose_rebinding_capture_request(const RuntimeInputActionMap& base,
                                   const RuntimeInputRebindingCaptureRequest& request) {
    std::vector<RuntimeInputRebindingDiagnostic> diagnostics;
    const auto path = make_rebinding_path("capture.", request.context, request.action);
    bool names_valid = true;

    try {
        validate_context_name(request.context);
    } catch (const std::exception& error) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_context, path, error.what());
        names_valid = false;
    }
    try {
        validate_action_name(request.action);
    } catch (const std::exception& error) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_action, path, error.what());
        names_valid = false;
    }
    if (!names_valid) {
        return diagnostics;
    }

    if (base.find(request.context, request.action) == nullptr) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::missing_base_binding, path,
                                 "runtime input rebinding capture target is missing from the base map");
    }
    return diagnostics;
}

[[nodiscard]] std::vector<RuntimeInputRebindingDiagnostic>
diagnose_rebinding_axis_capture_request(const RuntimeInputActionMap& base,
                                        const RuntimeInputRebindingAxisCaptureRequest& request) {
    std::vector<RuntimeInputRebindingDiagnostic> diagnostics;
    const auto path = make_rebinding_path("capture.axis.", request.context, request.action);
    bool names_valid = true;

    try {
        validate_context_name(request.context);
    } catch (const std::exception& error) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_context, path, error.what());
        names_valid = false;
    }
    try {
        validate_action_name(request.action);
    } catch (const std::exception& error) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_action, path, error.what());
        names_valid = false;
    }
    if (!names_valid) {
        return diagnostics;
    }

    if (base.find_axis(request.context, request.action) == nullptr) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::missing_base_binding, path,
                                 "runtime input rebinding axis capture target is missing from the base axis map");
    }
    return diagnostics;
}

[[nodiscard]] float capture_axis_detection_deadzone(float requested) noexcept {
    constexpr float default_deadzone = 0.25F;
    if (!std::isfinite(requested) || requested <= 0.0F) {
        return default_deadzone;
    }
    return std::clamp(requested, 0.01F, 0.99F);
}

[[nodiscard]] std::pair<float, float> gamepad_axis_binding_defaults(const RuntimeInputAxisBinding* binding) noexcept {
    float scale = 1.0F;
    float deadzone = 0.2F;
    if (binding != nullptr) {
        for (const auto& src : binding->sources) {
            if (src.kind == RuntimeInputAxisSourceKind::gamepad_axis) {
                scale = src.scale;
                deadzone = src.deadzone;
                break;
            }
        }
    }
    return {scale, deadzone};
}

[[nodiscard]] std::pair<float, float> key_pair_binding_defaults(const RuntimeInputAxisBinding* binding) noexcept {
    float scale = 1.0F;
    float deadzone = 0.2F;
    if (binding != nullptr) {
        for (const auto& src : binding->sources) {
            if (src.kind == RuntimeInputAxisSourceKind::key_pair) {
                scale = src.scale;
                deadzone = src.deadzone;
                break;
            }
        }
    }
    return {scale, deadzone};
}

[[nodiscard]] std::optional<RuntimeInputAxisSource>
capture_first_gamepad_axis_source(const RuntimeInputRebindingAxisCaptureRequest& request, float detection_deadzone,
                                  const RuntimeInputAxisBinding* base_axis_binding) {
    if (!request.capture_gamepad_axes || request.state.gamepad == nullptr) {
        return std::nullopt;
    }

    const auto defaults = gamepad_axis_binding_defaults(base_axis_binding);
    const float scale = defaults.first;
    const float deadzone = defaults.second;

    for (const auto& gamepad : request.state.gamepad->gamepads()) {
        if (gamepad.id == 0) {
            continue;
        }
        for (auto axis_index = static_cast<int>(GamepadAxis::unknown) + 1;
             axis_index < static_cast<int>(GamepadAxis::count); ++axis_index) {
            const auto axis = static_cast<GamepadAxis>(axis_index);
            const float value = request.state.gamepad->axis_value(gamepad.id, axis);
            if (std::abs(value) > detection_deadzone) {
                return RuntimeInputAxisSource{.kind = RuntimeInputAxisSourceKind::gamepad_axis,
                                              .negative_key = Key::unknown,
                                              .positive_key = Key::unknown,
                                              .gamepad_id = gamepad.id,
                                              .gamepad_axis = axis,
                                              .scale = scale,
                                              .deadzone = deadzone};
            }
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<RuntimeInputAxisSource>
capture_first_keyboard_key_pair_axis_source(const RuntimeInputRebindingAxisCaptureRequest& request,
                                            const RuntimeInputAxisBinding* base_axis_binding) {
    if (!request.capture_keyboard_key_pair_axes || request.state.keyboard == nullptr) {
        return std::nullopt;
    }

    const auto defaults = key_pair_binding_defaults(base_axis_binding);
    const float scale = defaults.first;
    const float deadzone = defaults.second;

    std::vector<Key> held;
    held.reserve(32);
    for (auto key_index = static_cast<int>(Key::unknown) + 1; key_index < static_cast<int>(Key::count); ++key_index) {
        const auto key = static_cast<Key>(key_index);
        if (request.state.keyboard->key_down(key)) {
            held.push_back(key);
        }
    }
    if (held.size() < 2) {
        return std::nullopt;
    }
    std::ranges::sort(held, [](Key left, Key right) { return static_cast<int>(left) < static_cast<int>(right); });
    const Key negative_key = held.front();
    const Key positive_key = held[1];
    return RuntimeInputAxisSource{.kind = RuntimeInputAxisSourceKind::key_pair,
                                  .negative_key = negative_key,
                                  .positive_key = positive_key,
                                  .gamepad_id = 0,
                                  .gamepad_axis = GamepadAxis::unknown,
                                  .scale = scale,
                                  .deadzone = deadzone};
}

[[nodiscard]] std::vector<RuntimeInputRebindingDiagnostic>
diagnose_rebinding_focus_capture_request(const RuntimeInputRebindingFocusCaptureRequest& request) {
    std::vector<RuntimeInputRebindingDiagnostic> diagnostics;

    if (!request.armed) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::capture_not_armed,
                                 "capture.focus.armed",
                                 "runtime input rebinding focus capture must be armed before input is consumed");
    }

    bool capture_id_valid = true;
    try {
        validate_entry_key(request.capture_id, "runtime input rebinding capture id");
    } catch (const std::exception& error) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_capture_id,
                                 "capture.focus.id", error.what());
        capture_id_valid = false;
    }

    if (capture_id_valid && request.focused_id != request.capture_id) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::invalid_capture_focus,
                                 "capture.focused_id",
                                 "runtime input rebinding focus capture requires focus on the active capture id");
    }

    if (capture_id_valid && !request.modal_layer_id.empty() && request.modal_layer_id != request.capture_id) {
        add_rebinding_diagnostic(diagnostics, RuntimeInputRebindingDiagnosticCode::modal_layer_mismatch,
                                 "capture.modal_layer_id",
                                 "runtime input rebinding focus capture modal layer must match the active capture id");
    }

    return diagnostics;
}

[[nodiscard]] std::optional<RuntimeInputActionTrigger>
capture_first_pressed_trigger(const RuntimeInputRebindingCaptureRequest& request) {
    if (request.capture_keyboard && request.state.keyboard != nullptr) {
        for (auto key_index = static_cast<int>(Key::unknown) + 1; key_index < static_cast<int>(Key::count);
             ++key_index) {
            const auto key = static_cast<Key>(key_index);
            if (request.state.keyboard->key_pressed(key)) {
                return RuntimeInputActionTrigger{.kind = RuntimeInputActionTriggerKind::key, .key = key};
            }
        }
    }

    if (request.capture_pointer && request.state.pointer != nullptr) {
        for (const auto& pointer : request.state.pointer->pointers()) {
            if (pointer.id != 0 && pointer.pressed) {
                return RuntimeInputActionTrigger{
                    .kind = RuntimeInputActionTriggerKind::pointer, .key = Key::unknown, .pointer_id = pointer.id};
            }
        }
    }

    if (request.capture_gamepad_buttons && request.state.gamepad != nullptr) {
        for (const auto& gamepad : request.state.gamepad->gamepads()) {
            if (gamepad.id == 0) {
                continue;
            }
            for (auto button_index = static_cast<int>(GamepadButton::unknown) + 1;
                 button_index < static_cast<int>(GamepadButton::count); ++button_index) {
                const auto button = static_cast<GamepadButton>(button_index);
                if (request.state.gamepad->button_pressed(gamepad.id, button)) {
                    return RuntimeInputActionTrigger{.kind = RuntimeInputActionTriggerKind::gamepad_button,
                                                     .key = Key::unknown,
                                                     .pointer_id = 0,
                                                     .gamepad_id = gamepad.id,
                                                     .gamepad_button = button};
                }
            }
        }
    }

    return std::nullopt;
}

void replace_or_append_rebinding_action_override(RuntimeInputRebindingProfile& profile,
                                                 RuntimeInputRebindingActionOverride override_row) {
    const auto it = std::ranges::find_if(profile.action_overrides, [&override_row](const auto& current) {
        return current.context == override_row.context && current.action == override_row.action;
    });
    if (it == profile.action_overrides.end()) {
        profile.action_overrides.push_back(std::move(override_row));
        return;
    }
    *it = std::move(override_row);
}

void replace_or_append_rebinding_axis_override(RuntimeInputRebindingProfile& profile,
                                               RuntimeInputRebindingAxisOverride override_row) {
    const auto it = std::ranges::find_if(profile.axis_overrides, [&override_row](const auto& current) {
        return current.context == override_row.context && current.action == override_row.action;
    });
    if (it == profile.axis_overrides.end()) {
        profile.axis_overrides.push_back(std::move(override_row));
        return;
    }
    *it = std::move(override_row);
}

[[nodiscard]] RuntimeInputRebindingPresentationToken
make_action_trigger_presentation_token(const RuntimeInputActionTrigger& trigger) {
    validate_trigger(trigger);

    RuntimeInputRebindingPresentationToken token;
    switch (trigger.kind) {
    case RuntimeInputActionTriggerKind::key: {
        const auto name = key_name(trigger.key);
        token.kind = RuntimeInputRebindingPresentationTokenKind::key;
        token.label = "key:" + std::string(name);
        token.glyph_lookup_key = "keyboard.key." + std::string(name);
        token.key = trigger.key;
        return token;
    }
    case RuntimeInputActionTriggerKind::pointer:
        token.kind = RuntimeInputRebindingPresentationTokenKind::pointer;
        token.label = "pointer:" + std::to_string(trigger.pointer_id);
        token.glyph_lookup_key = "pointer." + std::to_string(trigger.pointer_id);
        token.pointer_id = trigger.pointer_id;
        return token;
    case RuntimeInputActionTriggerKind::gamepad_button: {
        const auto button = gamepad_button_name(trigger.gamepad_button);
        token.kind = RuntimeInputRebindingPresentationTokenKind::gamepad_button;
        token.label = "gamepad:" + std::to_string(trigger.gamepad_id) + ":" + std::string(button);
        token.glyph_lookup_key = "gamepad." + std::to_string(trigger.gamepad_id) + ".button." + std::string(button);
        token.gamepad_id = trigger.gamepad_id;
        token.gamepad_button = trigger.gamepad_button;
        return token;
    }
    }
    throw std::invalid_argument("runtime input rebinding presentation trigger kind is unsupported");
}

[[nodiscard]] RuntimeInputRebindingPresentationToken
make_axis_source_presentation_token(const RuntimeInputAxisSource& source) {
    validate_axis_source(source);

    RuntimeInputRebindingPresentationToken token;
    switch (source.kind) {
    case RuntimeInputAxisSourceKind::key_pair: {
        const auto negative = key_name(source.negative_key);
        const auto positive = key_name(source.positive_key);
        token.kind = RuntimeInputRebindingPresentationTokenKind::key_pair;
        token.label = "keys:" + std::string(negative) + "/" + std::string(positive);
        token.glyph_lookup_key = "keyboard.axis." + std::string(negative) + "." + std::string(positive);
        token.negative_key = source.negative_key;
        token.positive_key = source.positive_key;
        return token;
    }
    case RuntimeInputAxisSourceKind::gamepad_axis: {
        const auto axis = gamepad_axis_name(source.gamepad_axis);
        token.kind = RuntimeInputRebindingPresentationTokenKind::gamepad_axis;
        token.label = "gamepad:" + std::to_string(source.gamepad_id) + ":" + std::string(axis) +
                      " scale=" + format_float(source.scale) + " deadzone=" + format_float(source.deadzone);
        token.glyph_lookup_key = "gamepad." + std::to_string(source.gamepad_id) + ".axis." + std::string(axis);
        token.gamepad_id = source.gamepad_id;
        token.gamepad_axis = source.gamepad_axis;
        token.scale = source.scale;
        token.deadzone = source.deadzone;
        return token;
    }
    }
    throw std::invalid_argument("runtime input rebinding presentation axis source kind is unsupported");
}

[[nodiscard]] std::vector<RuntimeInputRebindingPresentationToken>
present_action_trigger_tokens_safely(const std::vector<RuntimeInputActionTrigger>& triggers) {
    std::vector<RuntimeInputRebindingPresentationToken> tokens;
    tokens.reserve(triggers.size());
    for (const auto& trigger : triggers) {
        try {
            tokens.push_back(make_action_trigger_presentation_token(trigger));
        } catch (const std::exception& exception) {
            (void)exception;
            // The presentation model carries validation diagnostics; malformed token rows are omitted.
        }
    }
    return tokens;
}

[[nodiscard]] std::vector<RuntimeInputRebindingPresentationToken>
present_axis_source_tokens_safely(const std::vector<RuntimeInputAxisSource>& sources) {
    std::vector<RuntimeInputRebindingPresentationToken> tokens;
    tokens.reserve(sources.size());
    for (const auto& source : sources) {
        try {
            tokens.push_back(make_axis_source_presentation_token(source));
        } catch (const std::exception& exception) {
            (void)exception;
            // The presentation model carries validation diagnostics; malformed token rows are omitted.
        }
    }
    return tokens;
}

} // namespace

void RuntimeSaveData::set_value(std::string key, std::string value) {
    set_sorted_entry(entries_, std::move(key), std::move(value), "runtime save data");
}

const std::string* RuntimeSaveData::find_value(std::string_view key) const noexcept {
    return find_entry_value(entries_, key);
}

std::string RuntimeSaveData::value_or(std::string_view key, std::string_view fallback) const {
    const auto* value = find_value(key);
    return value == nullptr ? std::string(fallback) : *value;
}

const std::vector<RuntimeKeyValue>& RuntimeSaveData::entries() const noexcept {
    return entries_;
}

bool RuntimeSaveDataLoadResult::succeeded() const noexcept {
    return diagnostic.empty();
}

void RuntimeSettings::set_value(std::string key, std::string value) {
    set_sorted_entry(entries_, std::move(key), std::move(value), "runtime settings");
}

const std::string* RuntimeSettings::find_value(std::string_view key) const noexcept {
    return find_entry_value(entries_, key);
}

std::string RuntimeSettings::value_or(std::string_view key, std::string_view fallback) const {
    const auto* value = find_value(key);
    return value == nullptr ? std::string(fallback) : *value;
}

const std::vector<RuntimeKeyValue>& RuntimeSettings::entries() const noexcept {
    return entries_;
}

bool RuntimeSettingsLoadResult::succeeded() const noexcept {
    return diagnostic.empty();
}

void RuntimeLocalizationCatalog::set_text(std::string key, std::string text) {
    set_sorted_entry(entries_, std::move(key), std::move(text), "runtime localization");
}

const std::string* RuntimeLocalizationCatalog::find_text(std::string_view key) const noexcept {
    return find_entry_value(entries_, key);
}

std::string RuntimeLocalizationCatalog::text_or_key(std::string_view key) const {
    const auto* text = find_text(key);
    return text == nullptr ? std::string(key) : *text;
}

const std::vector<RuntimeLocalizationEntry>& RuntimeLocalizationCatalog::entries() const noexcept {
    return entries_;
}

bool RuntimeLocalizationCatalogLoadResult::succeeded() const noexcept {
    return diagnostic.empty();
}

void RuntimeInputActionMap::bind_key(std::string action, Key key) {
    bind_key_in_context(std::string(runtime_input_default_context), std::move(action), key);
}

void RuntimeInputActionMap::bind_pointer(std::string action, PointerId pointer_id) {
    bind_pointer_in_context(std::string(runtime_input_default_context), std::move(action), pointer_id);
}

void RuntimeInputActionMap::bind_gamepad_button(std::string action, GamepadId gamepad_id, GamepadButton button) {
    bind_gamepad_button_in_context(std::string(runtime_input_default_context), std::move(action), gamepad_id, button);
}

void RuntimeInputActionMap::bind_key_axis(std::string action, Key negative_key, Key positive_key) {
    bind_key_axis_in_context(std::string(runtime_input_default_context), std::move(action), negative_key, positive_key);
}

void RuntimeInputActionMap::bind_gamepad_axis(std::string action, GamepadId gamepad_id, GamepadAxis axis, float scale,
                                              float deadzone) {
    bind_gamepad_axis_in_context(std::string(runtime_input_default_context), std::move(action), gamepad_id, axis, scale,
                                 deadzone);
}

void RuntimeInputActionMap::bind_key_in_context(std::string context, std::string action, Key key) {
    bind_trigger(std::move(context), std::move(action),
                 RuntimeInputActionTrigger{.kind = RuntimeInputActionTriggerKind::key, .key = key});
}

void RuntimeInputActionMap::bind_pointer_in_context(std::string context, std::string action, PointerId pointer_id) {
    bind_trigger(std::move(context), std::move(action),
                 RuntimeInputActionTrigger{
                     .kind = RuntimeInputActionTriggerKind::pointer, .key = Key::unknown, .pointer_id = pointer_id});
}

void RuntimeInputActionMap::bind_gamepad_button_in_context(std::string context, std::string action,
                                                           GamepadId gamepad_id, GamepadButton button) {
    bind_trigger(std::move(context), std::move(action),
                 RuntimeInputActionTrigger{.kind = RuntimeInputActionTriggerKind::gamepad_button,
                                           .key = Key::unknown,
                                           .pointer_id = 0,
                                           .gamepad_id = gamepad_id,
                                           .gamepad_button = button});
}

void RuntimeInputActionMap::bind_key_axis_in_context(std::string context, std::string action, Key negative_key,
                                                     Key positive_key) {
    RuntimeInputAxisSource source;
    source.kind = RuntimeInputAxisSourceKind::key_pair;
    source.negative_key = negative_key;
    source.positive_key = positive_key;
    bind_axis_source(std::move(context), std::move(action), source);
}

void RuntimeInputActionMap::bind_gamepad_axis_in_context(std::string context, std::string action, GamepadId gamepad_id,
                                                         GamepadAxis axis, float scale, float deadzone) {
    RuntimeInputAxisSource source;
    source.kind = RuntimeInputAxisSourceKind::gamepad_axis;
    source.gamepad_id = gamepad_id;
    source.gamepad_axis = axis;
    source.scale = scale;
    source.deadzone = deadzone;
    bind_axis_source(std::move(context), std::move(action), source);
}

void RuntimeInputActionMap::bind_trigger(std::string context, std::string action, RuntimeInputActionTrigger trigger) {
    validate_context_name(context);
    validate_action_name(action);
    validate_trigger(trigger);

    const std::pair<std::string_view, std::string_view> needle{context, action};
    const auto it =
        std::ranges::lower_bound(bindings_, needle, std::ranges::less{}, [](const RuntimeInputActionBinding& binding) {
            return std::pair<std::string_view, std::string_view>{binding.context, binding.action};
        });
    if (it == bindings_.end() || it->context != context || it->action != action) {
        bindings_.insert(it, RuntimeInputActionBinding{
                                 .context = std::move(context), .action = std::move(action), .triggers = {trigger}});
        return;
    }
    if (std::ranges::none_of(it->triggers, [&trigger](const RuntimeInputActionTrigger& bound) {
            return same_trigger(bound, trigger);
        })) {
        it->triggers.push_back(trigger);
        std::ranges::sort(it->triggers, trigger_less);
    }
}

void RuntimeInputActionMap::bind_axis_source(std::string context, std::string action, RuntimeInputAxisSource source) {
    validate_context_name(context);
    validate_action_name(action);
    validate_axis_source(source);

    const std::pair<std::string_view, std::string_view> needle{context, action};
    const auto it = std::ranges::lower_bound(
        axis_bindings_, needle, std::ranges::less{}, [](const RuntimeInputAxisBinding& binding) {
            return std::pair<std::string_view, std::string_view>{binding.context, binding.action};
        });
    if (it == axis_bindings_.end() || it->context != context || it->action != action) {
        axis_bindings_.insert(it, RuntimeInputAxisBinding{
                                      .context = std::move(context), .action = std::move(action), .sources = {source}});
        return;
    }
    if (std::ranges::none_of(
            it->sources, [&source](const RuntimeInputAxisSource& bound) { return same_axis_source(bound, source); })) {
        it->sources.push_back(source);
        std::ranges::sort(it->sources, axis_source_less);
    }
}

const RuntimeInputActionBinding* RuntimeInputActionMap::find(std::string_view action) const noexcept {
    return find(runtime_input_default_context, action);
}

const RuntimeInputActionBinding* RuntimeInputActionMap::find(std::string_view context,
                                                             std::string_view action) const noexcept {
    const std::pair<std::string_view, std::string_view> needle{context, action};
    const auto it =
        std::ranges::lower_bound(bindings_, needle, std::ranges::less{}, [](const RuntimeInputActionBinding& binding) {
            return std::pair<std::string_view, std::string_view>{binding.context, binding.action};
        });
    return it == bindings_.end() || it->context != context || it->action != action ? nullptr : &*it;
}

const RuntimeInputAxisBinding* RuntimeInputActionMap::find_axis(std::string_view action) const noexcept {
    return find_axis(runtime_input_default_context, action);
}

const RuntimeInputAxisBinding* RuntimeInputActionMap::find_axis(std::string_view context,
                                                                std::string_view action) const noexcept {
    const std::pair<std::string_view, std::string_view> needle{context, action};
    const auto it = std::ranges::lower_bound(
        axis_bindings_, needle, std::ranges::less{}, [](const RuntimeInputAxisBinding& binding) {
            return std::pair<std::string_view, std::string_view>{binding.context, binding.action};
        });
    return it == axis_bindings_.end() || it->context != context || it->action != action ? nullptr : &*it;
}

const RuntimeInputActionBinding*
RuntimeInputActionMap::find_active(std::string_view action,
                                   const RuntimeInputContextStack& context_stack) const noexcept {
    if (context_stack.active_contexts.empty()) {
        return find(action);
    }
    for (const auto& context : context_stack.active_contexts) {
        if (const auto* binding = find(context, action); binding != nullptr) {
            return binding;
        }
    }
    return nullptr;
}

const RuntimeInputAxisBinding*
RuntimeInputActionMap::find_active_axis(std::string_view action,
                                        const RuntimeInputContextStack& context_stack) const noexcept {
    if (context_stack.active_contexts.empty()) {
        return find_axis(action);
    }
    for (const auto& context : context_stack.active_contexts) {
        if (const auto* binding = find_axis(context, action); binding != nullptr) {
            return binding;
        }
    }
    return nullptr;
}

bool RuntimeInputActionMap::action_down(std::string_view action, RuntimeInputStateView state) const noexcept {
    const auto* binding = find(action);
    if (binding == nullptr) {
        return false;
    }
    return std::ranges::any_of(
        binding->triggers, [state](const RuntimeInputActionTrigger& trigger) { return trigger_down(trigger, state); });
}

bool RuntimeInputActionMap::action_down(std::string_view action, RuntimeInputStateView state,
                                        const RuntimeInputContextStack& context_stack) const noexcept {
    const auto* binding = find_active(action, context_stack);
    if (binding == nullptr) {
        return false;
    }
    return std::ranges::any_of(
        binding->triggers, [state](const RuntimeInputActionTrigger& trigger) { return trigger_down(trigger, state); });
}

bool RuntimeInputActionMap::action_pressed(std::string_view action, RuntimeInputStateView state) const noexcept {
    const auto* binding = find(action);
    if (binding == nullptr) {
        return false;
    }
    const auto current_down = std::ranges::any_of(
        binding->triggers, [state](const RuntimeInputActionTrigger& trigger) { return trigger_down(trigger, state); });
    const auto previous_down =
        std::ranges::any_of(binding->triggers, [state](const RuntimeInputActionTrigger& trigger) {
            return trigger_previously_down(trigger, state);
        });
    return current_down && !previous_down;
}

bool RuntimeInputActionMap::action_pressed(std::string_view action, RuntimeInputStateView state,
                                           const RuntimeInputContextStack& context_stack) const noexcept {
    const auto* binding = find_active(action, context_stack);
    if (binding == nullptr) {
        return false;
    }
    const auto current_down = std::ranges::any_of(
        binding->triggers, [state](const RuntimeInputActionTrigger& trigger) { return trigger_down(trigger, state); });
    const auto previous_down =
        std::ranges::any_of(binding->triggers, [state](const RuntimeInputActionTrigger& trigger) {
            return trigger_previously_down(trigger, state);
        });
    return current_down && !previous_down;
}

bool RuntimeInputActionMap::action_released(std::string_view action, RuntimeInputStateView state) const noexcept {
    const auto* binding = find(action);
    if (binding == nullptr) {
        return false;
    }
    const auto current_down = std::ranges::any_of(
        binding->triggers, [state](const RuntimeInputActionTrigger& trigger) { return trigger_down(trigger, state); });
    const auto previous_down =
        std::ranges::any_of(binding->triggers, [state](const RuntimeInputActionTrigger& trigger) {
            return trigger_previously_down(trigger, state);
        });
    return !current_down && previous_down;
}

bool RuntimeInputActionMap::action_released(std::string_view action, RuntimeInputStateView state,
                                            const RuntimeInputContextStack& context_stack) const noexcept {
    const auto* binding = find_active(action, context_stack);
    if (binding == nullptr) {
        return false;
    }
    const auto current_down = std::ranges::any_of(
        binding->triggers, [state](const RuntimeInputActionTrigger& trigger) { return trigger_down(trigger, state); });
    const auto previous_down =
        std::ranges::any_of(binding->triggers, [state](const RuntimeInputActionTrigger& trigger) {
            return trigger_previously_down(trigger, state);
        });
    return !current_down && previous_down;
}

float RuntimeInputActionMap::axis_value(std::string_view action, RuntimeInputStateView state) const noexcept {
    const auto* binding = find_axis(action);
    if (binding == nullptr) {
        return 0.0F;
    }

    float selected = 0.0F;
    float selected_magnitude = 0.0F;
    for (const auto& source : binding->sources) {
        const auto value = axis_source_value(source, state);
        const auto magnitude = std::abs(value);
        if (magnitude > selected_magnitude) {
            selected = value;
            selected_magnitude = magnitude;
        }
    }
    return selected;
}

float RuntimeInputActionMap::axis_value(std::string_view action, RuntimeInputStateView state,
                                        const RuntimeInputContextStack& context_stack) const noexcept {
    const auto* binding = find_active_axis(action, context_stack);
    if (binding == nullptr) {
        return 0.0F;
    }

    float selected = 0.0F;
    float selected_magnitude = 0.0F;
    for (const auto& source : binding->sources) {
        const auto value = axis_source_value(source, state);
        const auto magnitude = std::abs(value);
        if (magnitude > selected_magnitude) {
            selected = value;
            selected_magnitude = magnitude;
        }
    }
    return selected;
}

const std::vector<RuntimeInputActionBinding>& RuntimeInputActionMap::bindings() const noexcept {
    return bindings_;
}

const std::vector<RuntimeInputAxisBinding>& RuntimeInputActionMap::axis_bindings() const noexcept {
    return axis_bindings_;
}

bool RuntimeInputActionMapLoadResult::succeeded() const noexcept {
    return diagnostic.empty();
}

bool RuntimeInputRebindingProfileLoadResult::succeeded() const noexcept {
    return diagnostic.empty();
}

bool RuntimeInputRebindingProfileApplyResult::succeeded() const noexcept {
    return diagnostics.empty();
}

bool RuntimeInputRebindingCaptureResult::captured() const noexcept {
    return status == RuntimeInputRebindingCaptureStatus::captured;
}

bool RuntimeInputRebindingCaptureResult::waiting() const noexcept {
    return status == RuntimeInputRebindingCaptureStatus::waiting;
}

bool RuntimeInputRebindingAxisCaptureResult::captured() const noexcept {
    return status == RuntimeInputRebindingCaptureStatus::captured;
}

bool RuntimeInputRebindingAxisCaptureResult::waiting() const noexcept {
    return status == RuntimeInputRebindingCaptureStatus::waiting;
}

bool RuntimeInputRebindingFocusCaptureResult::captured() const noexcept {
    return capture.captured();
}

bool RuntimeInputRebindingFocusCaptureResult::waiting() const noexcept {
    return capture.waiting();
}

bool RuntimeInputRebindingFocusCaptureResult::blocked() const noexcept {
    return capture.status == RuntimeInputRebindingCaptureStatus::blocked;
}

bool RuntimeInputRebindingPresentationModel::ready() const noexcept {
    return ready_for_display && !has_blocking_diagnostics && diagnostics.empty();
}

std::string serialize_runtime_save_data(const RuntimeSaveData& data) {
    return serialize_key_value_document(save_data_format, data);
}

RuntimeSaveDataLoadResult deserialize_runtime_save_data(std::string_view text) {
    try {
        return RuntimeSaveDataLoadResult{
            .data = deserialize_key_value_document<RuntimeSaveData>(text, save_data_format, "runtime save data"),
            .diagnostic = {},
        };
    } catch (const std::exception& error) {
        return RuntimeSaveDataLoadResult{.data = RuntimeSaveData{}, .diagnostic = error.what()};
    }
}

RuntimeSaveDataLoadResult load_runtime_save_data(IFileSystem& filesystem, std::string_view path) {
    validate_session_path(path);
    return deserialize_runtime_save_data(filesystem.read_text(path));
}

void write_runtime_save_data(IFileSystem& filesystem, std::string_view path, const RuntimeSaveData& data) {
    validate_session_path(path);
    filesystem.write_text(path, serialize_runtime_save_data(data));
}

std::string serialize_runtime_settings(const RuntimeSettings& settings) {
    return serialize_key_value_document(settings_format, settings);
}

RuntimeSettingsLoadResult deserialize_runtime_settings(std::string_view text) {
    try {
        return RuntimeSettingsLoadResult{
            .settings = deserialize_key_value_document<RuntimeSettings>(text, settings_format, "runtime settings"),
            .diagnostic = {},
        };
    } catch (const std::exception& error) {
        return RuntimeSettingsLoadResult{.settings = RuntimeSettings{}, .diagnostic = error.what()};
    }
}

RuntimeSettingsLoadResult load_runtime_settings(IFileSystem& filesystem, std::string_view path) {
    validate_session_path(path);
    return deserialize_runtime_settings(filesystem.read_text(path));
}

void write_runtime_settings(IFileSystem& filesystem, std::string_view path, const RuntimeSettings& settings) {
    validate_session_path(path);
    filesystem.write_text(path, serialize_runtime_settings(settings));
}

std::string serialize_runtime_localization_catalog(const RuntimeLocalizationCatalog& catalog) {
    validate_locale(catalog.locale);

    std::string text;
    text.append(make_line("format", localization_format));
    text.append(make_line("locale", catalog.locale));
    for (const auto& entry : catalog.entries()) {
        validate_entry_key(entry.key, "runtime localization");
        validate_entry_value(entry.text, "runtime localization");
        std::string key("text.");
        key.append(entry.key);
        text.append(make_line(key, entry.text));
    }
    return text;
}

RuntimeLocalizationCatalogLoadResult deserialize_runtime_localization_catalog(std::string_view text) {
    try {
        RuntimeLocalizationCatalog catalog;
        bool saw_format = false;
        bool saw_locale = false;
        std::unordered_set<std::string> seen_text;

        std::size_t line_start = 0;
        while (line_start < text.size()) {
            const auto line_end = text.find('\n', line_start);
            const auto raw_line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                             : line_end - line_start);
            line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
            if (raw_line.empty()) {
                continue;
            }
            if (raw_line.find('\r') != std::string_view::npos) {
                throw std::invalid_argument("runtime localization line contains carriage return");
            }
            const auto separator = raw_line.find('=');
            if (separator == std::string_view::npos) {
                throw std::invalid_argument("runtime localization line is missing '='");
            }
            const auto key = raw_line.substr(0, separator);
            const auto value = raw_line.substr(separator + 1U);
            validate_entry_value(value, "runtime localization");

            if (key == "format") {
                if (saw_format) {
                    throw std::invalid_argument("runtime localization format is duplicate");
                }
                saw_format = true;
                if (value != localization_format) {
                    throw std::invalid_argument("runtime localization format is unsupported");
                }
                continue;
            }
            if (key == "locale") {
                if (saw_locale) {
                    throw std::invalid_argument("runtime localization locale is duplicate");
                }
                validate_locale(value);
                catalog.locale = std::string(value);
                saw_locale = true;
                continue;
            }
            if (key.starts_with("text.")) {
                const auto text_key = key.substr(std::string_view("text.").size());
                validate_entry_key(text_key, "runtime localization");
                if (!seen_text.insert(std::string(text_key)).second) {
                    throw std::invalid_argument("runtime localization contains duplicate text key");
                }
                catalog.set_text(std::string(text_key), std::string(value));
                continue;
            }
            throw std::invalid_argument("runtime localization contains an unsupported key");
        }

        if (!saw_format) {
            throw std::invalid_argument("runtime localization format is missing");
        }
        if (!saw_locale) {
            throw std::invalid_argument("runtime localization locale is missing");
        }
        return RuntimeLocalizationCatalogLoadResult{.catalog = std::move(catalog), .diagnostic = {}};
    } catch (const std::exception& error) {
        return RuntimeLocalizationCatalogLoadResult{.catalog = RuntimeLocalizationCatalog{},
                                                    .diagnostic = error.what()};
    }
}

RuntimeLocalizationCatalogLoadResult load_runtime_localization_catalog(IFileSystem& filesystem, std::string_view path) {
    validate_session_path(path);
    return deserialize_runtime_localization_catalog(filesystem.read_text(path));
}

void write_runtime_localization_catalog(IFileSystem& filesystem, std::string_view path,
                                        const RuntimeLocalizationCatalog& catalog) {
    validate_session_path(path);
    filesystem.write_text(path, serialize_runtime_localization_catalog(catalog));
}

std::string serialize_runtime_input_actions(const RuntimeInputActionMap& actions) {
    std::string text;
    text.append(make_line("format", input_actions_format));
    for (const auto& binding : actions.bindings()) {
        validate_context_name(binding.context);
        validate_action_name(binding.action);
        if (binding.triggers.empty()) {
            throw std::invalid_argument("runtime input action binding has no triggers");
        }
        std::string key("bind.");
        key.append(binding.context);
        key.push_back('.');
        key.append(binding.action);

        std::string value;
        for (const auto& trigger : binding.triggers) {
            validate_trigger(trigger);
            if (!value.empty()) {
                value.push_back(',');
            }
            value.append(serialize_trigger(trigger));
        }
        text.append(make_line(key, value));
    }
    for (const auto& binding : actions.axis_bindings()) {
        validate_context_name(binding.context);
        validate_action_name(binding.action);
        if (binding.sources.empty()) {
            throw std::invalid_argument("runtime input action axis binding has no sources");
        }
        std::string key("axis.");
        key.append(binding.context);
        key.push_back('.');
        key.append(binding.action);

        std::string value;
        for (const auto& source : binding.sources) {
            validate_axis_source(source);
            if (!value.empty()) {
                value.push_back(',');
            }
            value.append(serialize_axis_source(source));
        }
        text.append(make_line(key, value));
    }
    return text;
}

RuntimeInputActionMapLoadResult deserialize_runtime_input_actions(std::string_view text) {
    try {
        RuntimeInputActionMap actions;
        bool saw_format = false;
        std::unordered_set<std::string> seen_actions;
        std::unordered_set<std::string> seen_axis_actions;

        std::size_t line_start = 0;
        while (line_start < text.size()) {
            const auto line_end = text.find('\n', line_start);
            const auto raw_line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                             : line_end - line_start);
            line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
            if (raw_line.empty()) {
                continue;
            }
            if (raw_line.find('\r') != std::string_view::npos) {
                throw std::invalid_argument("runtime input actions line contains carriage return");
            }
            const auto separator = raw_line.find('=');
            if (separator == std::string_view::npos) {
                throw std::invalid_argument("runtime input actions line is missing '='");
            }
            const auto key = raw_line.substr(0, separator);
            const auto value = raw_line.substr(separator + 1U);
            validate_entry_value(value, "runtime input actions");

            if (key == "format") {
                if (saw_format) {
                    throw std::invalid_argument("runtime input actions format is duplicate");
                }
                saw_format = true;
                if (value != input_actions_format) {
                    throw std::invalid_argument("runtime input actions format is unsupported");
                }
                continue;
            }
            if (key.starts_with("bind.")) {
                const auto binding_key = parse_runtime_input_binding_key(key, "bind.");
                if (!seen_actions.insert(make_runtime_input_binding_identity(binding_key.context, binding_key.action))
                         .second) {
                    throw std::invalid_argument("runtime input actions contains duplicate action");
                }
                if (value.empty()) {
                    throw std::invalid_argument("runtime input actions binding has no triggers");
                }

                std::vector<RuntimeInputActionTrigger> parsed_triggers;
                std::size_t trigger_start = 0;
                while (trigger_start <= value.size()) {
                    const auto trigger_end = value.find(',', trigger_start);
                    const auto raw_trigger = value.substr(trigger_start, trigger_end == std::string_view::npos
                                                                             ? value.size() - trigger_start
                                                                             : trigger_end - trigger_start);
                    if (raw_trigger.empty()) {
                        throw std::invalid_argument("runtime input actions binding contains an empty trigger");
                    }
                    auto trigger = parse_trigger(raw_trigger);
                    if (std::ranges::any_of(parsed_triggers, [&trigger](const RuntimeInputActionTrigger& parsed) {
                            return same_trigger(parsed, trigger);
                        })) {
                        throw std::invalid_argument("runtime input actions binding contains duplicate trigger");
                    }
                    switch (trigger.kind) {
                    case RuntimeInputActionTriggerKind::key:
                        actions.bind_key_in_context(std::string(binding_key.context), std::string(binding_key.action),
                                                    trigger.key);
                        break;
                    case RuntimeInputActionTriggerKind::pointer:
                        actions.bind_pointer_in_context(std::string(binding_key.context),
                                                        std::string(binding_key.action), trigger.pointer_id);
                        break;
                    case RuntimeInputActionTriggerKind::gamepad_button:
                        actions.bind_gamepad_button_in_context(std::string(binding_key.context),
                                                               std::string(binding_key.action), trigger.gamepad_id,
                                                               trigger.gamepad_button);
                        break;
                    }
                    parsed_triggers.push_back(trigger);
                    if (trigger_end == std::string_view::npos) {
                        break;
                    }
                    trigger_start = trigger_end + 1U;
                }
                continue;
            }
            if (key.starts_with("axis.")) {
                const auto binding_key = parse_runtime_input_binding_key(key, "axis.");
                if (!seen_axis_actions
                         .insert(make_runtime_input_binding_identity(binding_key.context, binding_key.action))
                         .second) {
                    throw std::invalid_argument("runtime input actions contains duplicate axis action");
                }
                if (value.empty()) {
                    throw std::invalid_argument("runtime input actions axis binding has no sources");
                }

                std::vector<RuntimeInputAxisSource> parsed_sources;
                std::size_t source_start = 0;
                while (source_start <= value.size()) {
                    const auto source_end = value.find(',', source_start);
                    const auto raw_source =
                        value.substr(source_start, source_end == std::string_view::npos ? value.size() - source_start
                                                                                        : source_end - source_start);
                    if (raw_source.empty()) {
                        throw std::invalid_argument("runtime input actions axis binding contains an empty source");
                    }
                    auto source = parse_axis_source(raw_source);
                    if (std::ranges::any_of(parsed_sources, [&source](const RuntimeInputAxisSource& parsed) {
                            return same_axis_source(parsed, source);
                        })) {
                        throw std::invalid_argument("runtime input actions axis binding contains duplicate source");
                    }
                    switch (source.kind) {
                    case RuntimeInputAxisSourceKind::key_pair:
                        actions.bind_key_axis_in_context(std::string(binding_key.context),
                                                         std::string(binding_key.action), source.negative_key,
                                                         source.positive_key);
                        break;
                    case RuntimeInputAxisSourceKind::gamepad_axis:
                        actions.bind_gamepad_axis_in_context(std::string(binding_key.context),
                                                             std::string(binding_key.action), source.gamepad_id,
                                                             source.gamepad_axis, source.scale, source.deadzone);
                        break;
                    }
                    parsed_sources.push_back(source);
                    if (source_end == std::string_view::npos) {
                        break;
                    }
                    source_start = source_end + 1U;
                }
                continue;
            }
            throw std::invalid_argument("runtime input actions contains an unsupported key");
        }

        if (!saw_format) {
            throw std::invalid_argument("runtime input actions format is missing");
        }
        return RuntimeInputActionMapLoadResult{.actions = std::move(actions), .diagnostic = {}};
    } catch (const std::exception& error) {
        return RuntimeInputActionMapLoadResult{.actions = RuntimeInputActionMap{}, .diagnostic = error.what()};
    }
}

RuntimeInputActionMapLoadResult load_runtime_input_actions(IFileSystem& filesystem, std::string_view path) {
    validate_session_path(path);
    return deserialize_runtime_input_actions(filesystem.read_text(path));
}

void write_runtime_input_actions(IFileSystem& filesystem, std::string_view path, const RuntimeInputActionMap& actions) {
    validate_session_path(path);
    filesystem.write_text(path, serialize_runtime_input_actions(actions));
}

std::vector<RuntimeInputRebindingDiagnostic>
validate_runtime_input_rebinding_profile(const RuntimeInputActionMap& base,
                                         const RuntimeInputRebindingProfile& profile) {
    return validate_runtime_input_rebinding_profile_impl(&base, profile, true);
}

RuntimeInputRebindingProfileApplyResult
apply_runtime_input_rebinding_profile(const RuntimeInputActionMap& base, const RuntimeInputRebindingProfile& profile) {
    auto diagnostics = validate_runtime_input_rebinding_profile(base, profile);
    if (!diagnostics.empty()) {
        return RuntimeInputRebindingProfileApplyResult{.actions = RuntimeInputActionMap{},
                                                       .diagnostics = std::move(diagnostics),
                                                       .action_overrides_applied = 0U,
                                                       .axis_overrides_applied = 0U};
    }

    RuntimeInputActionMap actions;
    std::size_t action_overrides_applied = 0;
    std::size_t axis_overrides_applied = 0;

    for (const auto& binding : base.bindings()) {
        const auto* override_row = find_action_override(profile, binding.context, binding.action);
        const auto& triggers = override_row == nullptr ? binding.triggers : override_row->triggers;
        for (const auto& trigger : triggers) {
            bind_trigger_to_action_map(actions, binding.context, binding.action, trigger);
        }
        if (override_row != nullptr) {
            ++action_overrides_applied;
        }
    }

    for (const auto& binding : base.axis_bindings()) {
        const auto* override_row = find_axis_override(profile, binding.context, binding.action);
        const auto& sources = override_row == nullptr ? binding.sources : override_row->sources;
        for (const auto& source : sources) {
            bind_axis_source_to_action_map(actions, binding.context, binding.action, source);
        }
        if (override_row != nullptr) {
            ++axis_overrides_applied;
        }
    }

    return RuntimeInputRebindingProfileApplyResult{.actions = std::move(actions),
                                                   .diagnostics = {},
                                                   .action_overrides_applied = action_overrides_applied,
                                                   .axis_overrides_applied = axis_overrides_applied};
}

RuntimeInputRebindingCaptureResult
capture_runtime_input_rebinding_action(const RuntimeInputActionMap& base, const RuntimeInputRebindingProfile& profile,
                                       const RuntimeInputRebindingCaptureRequest& request) {
    RuntimeInputRebindingCaptureResult result;
    result.candidate_profile = profile;

    result.diagnostics = diagnose_rebinding_capture_request(base, request);
    if (!result.diagnostics.empty()) {
        result.status = RuntimeInputRebindingCaptureStatus::blocked;
        return result;
    }

    auto trigger = capture_first_pressed_trigger(request);
    if (!trigger.has_value()) {
        result.status = RuntimeInputRebindingCaptureStatus::waiting;
        return result;
    }

    result.trigger = *trigger;
    result.action_override = RuntimeInputRebindingActionOverride{
        .context = request.context, .action = request.action, .triggers = {*trigger}};
    replace_or_append_rebinding_action_override(result.candidate_profile, result.action_override);

    result.diagnostics = validate_runtime_input_rebinding_profile(base, result.candidate_profile);
    result.status = result.diagnostics.empty() ? RuntimeInputRebindingCaptureStatus::captured
                                               : RuntimeInputRebindingCaptureStatus::blocked;
    return result;
}

RuntimeInputRebindingAxisCaptureResult
capture_runtime_input_rebinding_axis(const RuntimeInputActionMap& base, const RuntimeInputRebindingProfile& profile,
                                     const RuntimeInputRebindingAxisCaptureRequest& request) {
    RuntimeInputRebindingAxisCaptureResult result;
    result.candidate_profile = profile;

    result.diagnostics = diagnose_rebinding_axis_capture_request(base, request);
    if (!result.diagnostics.empty()) {
        result.status = RuntimeInputRebindingCaptureStatus::blocked;
        return result;
    }

    const auto* axis_binding = base.find_axis(request.context, request.action);
    const float detection_deadzone = capture_axis_detection_deadzone(request.capture_deadzone);

    auto source = capture_first_gamepad_axis_source(request, detection_deadzone, axis_binding);
    if (!source.has_value()) {
        source = capture_first_keyboard_key_pair_axis_source(request, axis_binding);
    }
    if (!source.has_value()) {
        result.status = RuntimeInputRebindingCaptureStatus::waiting;
        return result;
    }

    result.source = source;
    result.axis_override = RuntimeInputRebindingAxisOverride{
        .context = std::string(request.context), .action = std::string(request.action), .sources = {*source}};
    replace_or_append_rebinding_axis_override(result.candidate_profile, result.axis_override);

    result.diagnostics = validate_runtime_input_rebinding_profile(base, result.candidate_profile);
    result.status = result.diagnostics.empty() ? RuntimeInputRebindingCaptureStatus::captured
                                               : RuntimeInputRebindingCaptureStatus::blocked;
    return result;
}

RuntimeInputRebindingFocusCaptureResult
capture_runtime_input_rebinding_action_with_focus(const RuntimeInputActionMap& base,
                                                  const RuntimeInputRebindingProfile& profile,
                                                  const RuntimeInputRebindingFocusCaptureRequest& request) {
    RuntimeInputRebindingFocusCaptureResult result;
    result.active_capture_id = request.capture_id;
    result.capture.candidate_profile = profile;

    result.diagnostics = diagnose_rebinding_focus_capture_request(request);
    if (!result.diagnostics.empty()) {
        result.capture.status = RuntimeInputRebindingCaptureStatus::blocked;
        result.capture.diagnostics = result.diagnostics;
        return result;
    }

    result.capture = capture_runtime_input_rebinding_action(base, profile, request.capture);
    result.diagnostics = result.capture.diagnostics;
    if (request.consume_gameplay_input &&
        (result.capture.waiting() || result.capture.captured() || result.capture.trigger.has_value())) {
        result.gameplay_input_consumed = true;
    }
    result.focus_retained = result.capture.waiting();
    return result;
}

RuntimeInputRebindingPresentationToken present_runtime_input_action_trigger(const RuntimeInputActionTrigger& trigger) {
    return make_action_trigger_presentation_token(trigger);
}

RuntimeInputRebindingPresentationToken present_runtime_input_axis_source(const RuntimeInputAxisSource& source) {
    return make_axis_source_presentation_token(source);
}

RuntimeInputRebindingPresentationModel
make_runtime_input_rebinding_presentation(const RuntimeInputActionMap& base,
                                          const RuntimeInputRebindingProfile& profile) {
    RuntimeInputRebindingPresentationModel model;
    model.diagnostics = validate_runtime_input_rebinding_profile(base, profile);
    model.ready_for_display = model.diagnostics.empty();
    model.has_blocking_diagnostics = !model.diagnostics.empty();
    model.has_profile_overrides = !profile.action_overrides.empty() || !profile.axis_overrides.empty();

    for (const auto& binding : base.bindings()) {
        const auto* override_row = find_action_override(profile, binding.context, binding.action);
        RuntimeInputRebindingPresentationRow row;
        row.id = make_rebinding_path("bind.", binding.context, binding.action);
        row.kind = RuntimeInputRebindingPresentationRowKind::action;
        row.context = binding.context;
        row.action = binding.action;
        row.base_tokens = present_action_trigger_tokens_safely(binding.triggers);
        if (override_row != nullptr) {
            row.profile_tokens = present_action_trigger_tokens_safely(override_row->triggers);
        }
        row.overridden = override_row != nullptr;
        row.ready = model.ready_for_display;
        row.diagnostic = row.ready ? (row.overridden ? "profile action override reviewed" : "base action binding")
                                   : "runtime input rebinding profile diagnostics block presentation";
        model.rows.push_back(std::move(row));
    }

    for (const auto& binding : base.axis_bindings()) {
        const auto* override_row = find_axis_override(profile, binding.context, binding.action);
        RuntimeInputRebindingPresentationRow row;
        row.id = make_rebinding_path("axis.", binding.context, binding.action);
        row.kind = RuntimeInputRebindingPresentationRowKind::axis;
        row.context = binding.context;
        row.action = binding.action;
        row.base_tokens = present_axis_source_tokens_safely(binding.sources);
        if (override_row != nullptr) {
            row.profile_tokens = present_axis_source_tokens_safely(override_row->sources);
        }
        row.overridden = override_row != nullptr;
        row.ready = model.ready_for_display;
        row.diagnostic = row.ready ? (row.overridden ? "profile axis override reviewed" : "base axis binding")
                                   : "runtime input rebinding profile diagnostics block presentation";
        model.rows.push_back(std::move(row));
    }

    return model;
}

std::string serialize_runtime_input_rebinding_profile(const RuntimeInputRebindingProfile& profile) {
    const auto diagnostics = validate_runtime_input_rebinding_profile_impl(nullptr, profile, false);
    if (!diagnostics.empty()) {
        throw std::invalid_argument(diagnostics.front().message);
    }

    std::string text;
    text.append(make_line("format", input_rebinding_profile_format));
    text.append(make_line("profile.id", profile.profile_id));

    auto action_overrides = profile.action_overrides;
    std::ranges::sort(action_overrides, [](const auto& lhs, const auto& rhs) {
        return std::pair<std::string_view, std::string_view>{lhs.context, lhs.action} <
               std::pair<std::string_view, std::string_view>{rhs.context, rhs.action};
    });
    for (const auto& override_row : action_overrides) {
        std::string key("bind.");
        key.append(override_row.context);
        key.push_back('.');
        key.append(override_row.action);

        auto triggers = override_row.triggers;
        std::ranges::sort(triggers, trigger_less);

        std::string value;
        for (const auto& trigger : triggers) {
            if (!value.empty()) {
                value.push_back(',');
            }
            value.append(serialize_trigger(trigger));
        }
        text.append(make_line(key, value));
    }

    auto axis_overrides = profile.axis_overrides;
    std::ranges::sort(axis_overrides, [](const auto& lhs, const auto& rhs) {
        return std::pair<std::string_view, std::string_view>{lhs.context, lhs.action} <
               std::pair<std::string_view, std::string_view>{rhs.context, rhs.action};
    });
    for (const auto& override_row : axis_overrides) {
        std::string key("axis.");
        key.append(override_row.context);
        key.push_back('.');
        key.append(override_row.action);

        auto sources = override_row.sources;
        std::ranges::sort(sources, axis_source_less);

        std::string value;
        for (const auto& source : sources) {
            if (!value.empty()) {
                value.push_back(',');
            }
            value.append(serialize_axis_source(source));
        }
        text.append(make_line(key, value));
    }

    return text;
}

RuntimeInputRebindingProfileLoadResult deserialize_runtime_input_rebinding_profile(std::string_view text) {
    try {
        RuntimeInputRebindingProfile profile;
        bool saw_format = false;
        bool saw_profile_id = false;

        std::size_t line_start = 0;
        while (line_start < text.size()) {
            const auto line_end = text.find('\n', line_start);
            const auto raw_line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                             : line_end - line_start);
            line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
            if (raw_line.empty()) {
                continue;
            }
            if (raw_line.find('\r') != std::string_view::npos) {
                throw std::invalid_argument("runtime input rebinding profile line contains carriage return");
            }
            const auto separator = raw_line.find('=');
            if (separator == std::string_view::npos) {
                throw std::invalid_argument("runtime input rebinding profile line is missing '='");
            }

            const auto key = raw_line.substr(0, separator);
            const auto value = raw_line.substr(separator + 1U);
            validate_entry_value(value, "runtime input rebinding profile");

            if (key == "format") {
                if (saw_format) {
                    throw std::invalid_argument("runtime input rebinding profile format is duplicate");
                }
                saw_format = true;
                if (value != input_rebinding_profile_format) {
                    throw std::invalid_argument("runtime input rebinding profile format is unsupported");
                }
                continue;
            }
            if (key == "profile.id") {
                if (saw_profile_id) {
                    throw std::invalid_argument("runtime input rebinding profile id is duplicate");
                }
                profile.profile_id = std::string(value);
                saw_profile_id = true;
                continue;
            }
            if (key.starts_with("bind.")) {
                const auto binding_key = parse_runtime_input_binding_key(key, "bind.");
                profile.action_overrides.push_back(
                    RuntimeInputRebindingActionOverride{.context = std::string(binding_key.context),
                                                        .action = std::string(binding_key.action),
                                                        .triggers = parse_rebinding_triggers(value)});
                continue;
            }
            if (key.starts_with("axis.")) {
                const auto binding_key = parse_runtime_input_binding_key(key, "axis.");
                profile.axis_overrides.push_back(
                    RuntimeInputRebindingAxisOverride{.context = std::string(binding_key.context),
                                                      .action = std::string(binding_key.action),
                                                      .sources = parse_rebinding_axis_sources(value)});
                continue;
            }
            throw std::invalid_argument("runtime input rebinding profile contains an unsupported key");
        }

        if (!saw_format) {
            throw std::invalid_argument("runtime input rebinding profile format is missing");
        }
        if (!saw_profile_id) {
            throw std::invalid_argument("runtime input rebinding profile id is missing");
        }

        const auto diagnostics = validate_runtime_input_rebinding_profile_impl(nullptr, profile, false);
        if (!diagnostics.empty()) {
            throw std::invalid_argument(diagnostics.front().message);
        }

        return RuntimeInputRebindingProfileLoadResult{.profile = std::move(profile), .diagnostic = {}};
    } catch (const std::exception& error) {
        return RuntimeInputRebindingProfileLoadResult{.profile = RuntimeInputRebindingProfile{},
                                                      .diagnostic = error.what()};
    }
}

RuntimeInputRebindingProfileLoadResult load_runtime_input_rebinding_profile(IFileSystem& filesystem,
                                                                            std::string_view path) {
    validate_session_path(path);
    return deserialize_runtime_input_rebinding_profile(filesystem.read_text(path));
}

void write_runtime_input_rebinding_profile(IFileSystem& filesystem, std::string_view path,
                                           const RuntimeInputRebindingProfile& profile) {
    validate_session_path(path);
    filesystem.write_text(path, serialize_runtime_input_rebinding_profile(profile));
}

} // namespace mirakana::runtime
