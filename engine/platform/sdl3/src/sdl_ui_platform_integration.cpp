// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/sdl3/sdl_ui_platform_integration.hpp"

#include "mirakana/platform/sdl3/sdl_window.hpp"
#include "mirakana/ui/ui.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_video.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] SDL_Window* native_window(SdlWindow& window) noexcept {
    return reinterpret_cast<SDL_Window*>(window.native_window().value);
}

[[nodiscard]] std::runtime_error sdl_text_input_error(const char* operation) {
    return std::runtime_error(std::string(operation) + " failed: " + SDL_GetError());
}

[[nodiscard]] std::invalid_argument
invalid_platform_text_input_request(const std::vector<ui::AdapterPayloadDiagnostic>& diagnostics) {
    return std::invalid_argument(diagnostics.empty() ? "platform text input request is invalid"
                                                     : diagnostics.front().message);
}

[[nodiscard]] bool fits_sdl_coordinate(float value) noexcept {
    const auto coordinate = static_cast<double>(value);
    return coordinate >= static_cast<double>(std::numeric_limits<int>::min()) &&
           coordinate <= static_cast<double>(std::numeric_limits<int>::max());
}

[[nodiscard]] SDL_Rect sdl_text_input_area(ui::Rect bounds) {
    if (!fits_sdl_coordinate(bounds.x) || !fits_sdl_coordinate(bounds.y) || !fits_sdl_coordinate(bounds.width) ||
        !fits_sdl_coordinate(bounds.height)) {
        throw std::invalid_argument("platform text input bounds must fit SDL coordinates");
    }

    return SDL_Rect{
        static_cast<int>(bounds.x),
        static_cast<int>(bounds.y),
        static_cast<int>(bounds.width),
        static_cast<int>(bounds.height),
    };
}

[[nodiscard]] bool is_utf8_continuation_byte(char value) noexcept {
    return (static_cast<unsigned char>(value) & 0xC0U) == 0x80U;
}

[[nodiscard]] std::size_t utf8_character_index_to_byte_offset(std::string_view text,
                                                              std::int32_t character_index) noexcept {
    if (character_index <= 0) {
        return 0;
    }

    std::size_t character = 0;
    for (std::size_t byte = 0; byte < text.size(); ++byte) {
        if (is_utf8_continuation_byte(text[byte])) {
            continue;
        }
        if (character == static_cast<std::size_t>(character_index)) {
            return byte;
        }
        ++character;
    }

    return text.size();
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

[[nodiscard]] bool has_sdl3_text_edit_shortcut_modifier(std::uint16_t modifiers) noexcept {
    const auto mods = static_cast<SDL_Keymod>(modifiers);
    const bool shortcut = (mods & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
    const bool alt = (mods & SDL_KMOD_ALT) != 0;
    return shortcut && !alt;
}

[[nodiscard]] std::optional<ui::TextEditClipboardCommandKind>
text_edit_clipboard_command_kind_from_sdl_keycode(std::int32_t keycode) noexcept {
    switch (keycode) {
    case SDLK_C:
        return ui::TextEditClipboardCommandKind::copy_selection;
    case SDLK_X:
        return ui::TextEditClipboardCommandKind::cut_selection;
    case SDLK_V:
        return ui::TextEditClipboardCommandKind::paste_text;
    default:
        return std::nullopt;
    }
}

} // namespace

std::optional<ui::ImeComposition> sdl3_ime_composition_from_window_event(const ui::ElementId& target,
                                                                         const SdlWindowEvent& event) {
    if (event.kind != SdlWindowEventKind::text_editing) {
        return std::nullopt;
    }

    ui::ImeComposition composition;
    composition.target = target;
    composition.composition_text = event.text;
    composition.cursor_index = utf8_character_index_to_byte_offset(composition.composition_text, event.text_edit_start);
    return composition;
}

ui::ImeCompositionPublishResult
publish_sdl3_ime_composition_event(ui::IImeAdapter& adapter, const ui::ElementId& target, const SdlWindowEvent& event) {
    const auto composition = sdl3_ime_composition_from_window_event(target, event);
    if (!composition.has_value()) {
        return {};
    }

    return ui::publish_ime_composition(adapter, *composition);
}

std::optional<ui::CommittedTextInput> sdl3_committed_text_from_window_event(const ui::ElementId& target,
                                                                            const SdlWindowEvent& event) {
    if (event.kind != SdlWindowEventKind::text_input) {
        return std::nullopt;
    }

    return ui::CommittedTextInput{
        .target = target,
        .text = event.text,
    };
}

ui::TextEditCommitResult apply_sdl3_committed_text_event(const ui::TextEditState& state, const ui::ElementId& target,
                                                         const SdlWindowEvent& event) {
    const auto committed = sdl3_committed_text_from_window_event(target, event);
    if (!committed.has_value()) {
        return ui::TextEditCommitResult{
            .committed = false,
            .state = state,
            .diagnostics = {},
        };
    }

    return ui::apply_committed_text_input(state, *committed);
}

std::optional<ui::TextEditCommand> sdl3_text_edit_command_from_window_event(const ui::ElementId& target,
                                                                            const SdlWindowEvent& event) {
    if (event.kind != SdlWindowEventKind::key_pressed) {
        return std::nullopt;
    }

    const auto command_kind = text_edit_command_kind_from_key(event.key);
    if (!command_kind.has_value()) {
        return std::nullopt;
    }

    return ui::TextEditCommand{
        .target = target,
        .kind = *command_kind,
    };
}

ui::TextEditCommandResult apply_sdl3_text_edit_command_event(const ui::TextEditState& state,
                                                             const ui::ElementId& target, const SdlWindowEvent& event) {
    const auto command = sdl3_text_edit_command_from_window_event(target, event);
    if (!command.has_value()) {
        return ui::TextEditCommandResult{
            .applied = false,
            .state = state,
            .diagnostics = {},
        };
    }

    return ui::apply_text_edit_command(state, *command);
}

std::optional<ui::TextEditClipboardCommand>
sdl3_text_edit_clipboard_command_from_window_event(const ui::ElementId& target, const SdlWindowEvent& event) {
    if (event.kind != SdlWindowEventKind::key_pressed) {
        return std::nullopt;
    }

    if (!has_sdl3_text_edit_shortcut_modifier(event.key_modifiers)) {
        return std::nullopt;
    }

    const auto command_kind = text_edit_clipboard_command_kind_from_sdl_keycode(event.sdl_keycode);
    if (!command_kind.has_value()) {
        return std::nullopt;
    }

    return ui::TextEditClipboardCommand{
        .target = target,
        .kind = *command_kind,
    };
}

ui::TextEditClipboardCommandResult apply_sdl3_text_edit_clipboard_command_event(ui::IClipboardTextAdapter& adapter,
                                                                                const ui::TextEditState& state,
                                                                                const ui::ElementId& target,
                                                                                const SdlWindowEvent& event) {
    const auto command = sdl3_text_edit_clipboard_command_from_window_event(target, event);
    if (!command.has_value()) {
        return ui::TextEditClipboardCommandResult{
            .applied = false,
            .state = state,
            .diagnostics = {},
        };
    }

    return ui::apply_text_edit_clipboard_command(adapter, state, *command);
}

SdlPlatformIntegrationAdapter::SdlPlatformIntegrationAdapter(SdlWindow& window) noexcept : window_(&window) {}

void SdlPlatformIntegrationAdapter::begin_text_input(const ui::PlatformTextInputRequest& request) {
    const auto plan = ui::plan_platform_text_input_session(request);
    if (!plan.ready()) {
        throw invalid_platform_text_input_request(plan.diagnostics);
    }

    SDL_Window* window = native_window(*window_);
    const SDL_Rect area = sdl_text_input_area(plan.request.text_bounds);
    if (!SDL_SetTextInputArea(window, &area, 0)) {
        throw sdl_text_input_error("SDL_SetTextInputArea");
    }
    if (!SDL_StartTextInput(window)) {
        static_cast<void>(SDL_SetTextInputArea(window, nullptr, 0));
        throw sdl_text_input_error("SDL_StartTextInput");
    }
}

void SdlPlatformIntegrationAdapter::end_text_input(const ui::ElementId& target) {
    const auto plan = ui::plan_platform_text_input_end(target);
    if (!plan.ready()) {
        throw invalid_platform_text_input_request(plan.diagnostics);
    }

    SDL_Window* window = native_window(*window_);
    if (!SDL_StopTextInput(window)) {
        throw sdl_text_input_error("SDL_StopTextInput");
    }
    if (!SDL_SetTextInputArea(window, nullptr, 0)) {
        throw sdl_text_input_error("SDL_SetTextInputArea");
    }
}

} // namespace mirakana
