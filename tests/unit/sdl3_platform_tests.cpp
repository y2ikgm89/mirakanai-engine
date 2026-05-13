// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/platform/input.hpp"
#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/sdl3/sdl_clipboard.hpp"
#include "mirakana/platform/sdl3/sdl_cursor.hpp"
#include "mirakana/platform/sdl3/sdl_file_dialog.hpp"
#include "mirakana/platform/sdl3/sdl_input.hpp"
#include "mirakana/platform/sdl3/sdl_ui_platform_integration.hpp"
#include "mirakana/platform/sdl3/sdl_window.hpp"
#include "mirakana/ui/ui.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_touch.h>
#include <SDL3/SDL_video.h>

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

[[nodiscard]] mirakana::SdlWindowEvent translate_sdl_event(const SDL_Event& event, const mirakana::SdlWindow& window) {
    return mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, window.extent());
}

template <typename T>
[[nodiscard]] const T& require_optional_value(const std::optional<T>& value, const char* const expression_text) {
    if (!value.has_value()) {
        mirakana::test::throw_require_failed(expression_text);
    }
    return *value;
}

class CapturingSdlImeAdapter final : public mirakana::ui::IImeAdapter {
  public:
    void update_composition(const mirakana::ui::ImeComposition& composition) override {
        published_composition_ = composition;
        ++publish_count_;
    }

    [[nodiscard]] const mirakana::ui::ImeComposition& published_composition() const noexcept {
        return published_composition_;
    }

    [[nodiscard]] int publish_count() const noexcept {
        return publish_count_;
    }

  private:
    mirakana::ui::ImeComposition published_composition_;
    int publish_count_{0};
};

} // namespace

MK_TEST("sdl3 keycodes map to engine virtual keys") {
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_LEFT) == mirakana::Key::left);
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_RIGHT) == mirakana::Key::right);
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_UP) == mirakana::Key::up);
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_DOWN) == mirakana::Key::down);
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_SPACE) == mirakana::Key::space);
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_ESCAPE) == mirakana::Key::escape);
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_BACKSPACE) == mirakana::Key::backspace);
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_DELETE) == mirakana::Key::delete_key);
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_HOME) == mirakana::Key::home);
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_END) == mirakana::Key::end);
}

MK_TEST("unhandled sdl3 keycodes map to unknown") {
    MK_REQUIRE(mirakana::sdl3_key_to_key(SDLK_A) == mirakana::Key::unknown);
}

MK_TEST("sdl3 key rows preserve window id for filtering") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Key Window Filter Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});
    mirakana::VirtualInput input;

    auto* native = reinterpret_cast<SDL_Window*>(window.native_window().value);
    const auto window_id = SDL_GetWindowID(native);

    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = window_id + 1U;
    event.key.key = SDLK_BACKSPACE;

    const auto translated = translate_sdl_event(event, window);
    MK_REQUIRE(translated.window_id == window_id + 1U);
    window.handle_event(translated, &input);
    MK_REQUIRE(!input.key_pressed(mirakana::Key::backspace));

    event.key.windowID = window_id;
    window.handle_event(translate_sdl_event(event, window), &input);
    MK_REQUIRE(input.key_pressed(mirakana::Key::backspace));
}

MK_TEST("sdl3 window keeps repeated key rows out of ordinary virtual input") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Key Repeat Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});
    mirakana::VirtualInput input;

    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_BACKSPACE;
    window.handle_event(translate_sdl_event(event, window), &input);

    MK_REQUIRE(input.key_pressed(mirakana::Key::backspace));
    MK_REQUIRE(input.key_down(mirakana::Key::backspace));

    input.begin_frame();
    event.key.repeat = true;
    window.handle_event(translate_sdl_event(event, window), &input);

    MK_REQUIRE(!input.key_pressed(mirakana::Key::backspace));
    MK_REQUIRE(input.key_down(mirakana::Key::backspace));
}

MK_TEST("sdl3 platform integration maps key rows to text edit commands") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_BACKSPACE;
    event.key.repeat = true;

    auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                            mirakana::WindowExtent{.width = 320, .height = 240});
    auto command =
        mirakana::sdl3_text_edit_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);

    if (!command.has_value()) {
        MK_REQUIRE(false);
        return;
    }
    MK_REQUIRE(translated.kind == mirakana::SdlWindowEventKind::key_pressed);
    MK_REQUIRE(translated.key == mirakana::Key::backspace);
    MK_REQUIRE(translated.repeated);
    MK_REQUIRE(command->target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(command->kind == mirakana::ui::TextEditCommandKind::delete_backward);

    event = SDL_Event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_DELETE;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    command = mirakana::sdl3_text_edit_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(command.has_value());
    MK_REQUIRE(command->kind == mirakana::ui::TextEditCommandKind::delete_forward);

    event = SDL_Event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_HOME;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    command = mirakana::sdl3_text_edit_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(command.has_value());
    MK_REQUIRE(command->kind == mirakana::ui::TextEditCommandKind::move_cursor_to_start);

    event = SDL_Event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_END;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    command = mirakana::sdl3_text_edit_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(command.has_value());
    MK_REQUIRE(command->kind == mirakana::ui::TextEditCommandKind::move_cursor_to_end);

    event = SDL_Event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_LEFT;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    command = mirakana::sdl3_text_edit_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(command.has_value());
    MK_REQUIRE(command->kind == mirakana::ui::TextEditCommandKind::move_cursor_backward);

    event = SDL_Event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_RIGHT;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    command = mirakana::sdl3_text_edit_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(command.has_value());
    MK_REQUIRE(command->kind == mirakana::ui::TextEditCommandKind::move_cursor_forward);
}

MK_TEST("sdl3 platform integration ignores non command key rows for text edit commands") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_UP;
    event.key.key = SDLK_BACKSPACE;

    auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                            mirakana::WindowExtent{.width = 320, .height = 240});
    auto command =
        mirakana::sdl3_text_edit_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(!command.has_value());

    event = SDL_Event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_UP;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    command = mirakana::sdl3_text_edit_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(!command.has_value());

    event = SDL_Event{};
    event.type = SDL_EVENT_TEXT_INPUT;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    command = mirakana::sdl3_text_edit_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(!command.has_value());

    event = SDL_Event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_A;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    command = mirakana::sdl3_text_edit_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(!command.has_value());
}

MK_TEST("sdl3 pointer helpers map mouse and normalized touch samples") {
    const auto mouse = mirakana::sdl3_mouse_pointer_sample(12.0F, 24.0F);
    MK_REQUIRE(mouse.id == mirakana::primary_pointer_id);
    MK_REQUIRE(mouse.kind == mirakana::PointerKind::mouse);
    MK_REQUIRE(mouse.position == (mirakana::Vec2{12.0F, 24.0F}));

    const auto touch_id = mirakana::sdl3_touch_pointer_id(42);
    const auto touch =
        mirakana::sdl3_touch_pointer_sample(42, 0.5F, 0.25F, mirakana::WindowExtent{.width = 320, .height = 240});
    MK_REQUIRE(touch.id == touch_id);
    MK_REQUIRE(touch.id != mirakana::primary_pointer_id);
    MK_REQUIRE(touch.kind == mirakana::PointerKind::touch);
    MK_REQUIRE(touch.position == (mirakana::Vec2{160.0F, 60.0F}));
}

MK_TEST("sdl3 gamepad helpers map buttons axes and normalized values") {
    MK_REQUIRE(mirakana::sdl3_gamepad_button_to_button(SDL_GAMEPAD_BUTTON_SOUTH) == mirakana::GamepadButton::south);
    MK_REQUIRE(mirakana::sdl3_gamepad_button_to_button(SDL_GAMEPAD_BUTTON_EAST) == mirakana::GamepadButton::east);
    MK_REQUIRE(mirakana::sdl3_gamepad_button_to_button(SDL_GAMEPAD_BUTTON_DPAD_LEFT) ==
               mirakana::GamepadButton::dpad_left);
    MK_REQUIRE(mirakana::sdl3_gamepad_button_to_button(SDL_GAMEPAD_BUTTON_INVALID) == mirakana::GamepadButton::unknown);

    MK_REQUIRE(mirakana::sdl3_gamepad_axis_to_axis(SDL_GAMEPAD_AXIS_LEFTX) == mirakana::GamepadAxis::left_x);
    MK_REQUIRE(mirakana::sdl3_gamepad_axis_to_axis(SDL_GAMEPAD_AXIS_RIGHTY) == mirakana::GamepadAxis::right_y);
    MK_REQUIRE(mirakana::sdl3_gamepad_axis_to_axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER) ==
               mirakana::GamepadAxis::left_trigger);
    MK_REQUIRE(mirakana::sdl3_gamepad_axis_to_axis(SDL_GAMEPAD_AXIS_INVALID) == mirakana::GamepadAxis::unknown);

    MK_REQUIRE(mirakana::sdl3_gamepad_axis_value(mirakana::GamepadAxis::left_x, 32767) == 1.0F);
    MK_REQUIRE(mirakana::sdl3_gamepad_axis_value(mirakana::GamepadAxis::left_y, -32768) == -1.0F);
    MK_REQUIRE(mirakana::sdl3_gamepad_axis_value(mirakana::GamepadAxis::left_trigger, 32767) == 1.0F);
    MK_REQUIRE(mirakana::sdl3_gamepad_axis_value(mirakana::GamepadAxis::right_trigger, -32768) == 0.0F);
}

MK_TEST("sdl3 window maps mouse events into virtual pointer input") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Pointer Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});
    mirakana::VirtualPointerInput pointer_input;

    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = 0;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 12.0F;
    event.button.y = 24.0F;
    window.handle_event(translate_sdl_event(event, window), nullptr, &pointer_input);

    MK_REQUIRE(pointer_input.pointer_pressed(mirakana::primary_pointer_id));
    MK_REQUIRE(pointer_input.pointer_position(mirakana::primary_pointer_id) == (mirakana::Vec2{12.0F, 24.0F}));

    event = SDL_Event{};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = 0;
    event.motion.x = 16.0F;
    event.motion.y = 20.0F;
    window.handle_event(translate_sdl_event(event, window), nullptr, &pointer_input);

    MK_REQUIRE(pointer_input.pointer_delta(mirakana::primary_pointer_id) == (mirakana::Vec2{4.0F, -4.0F}));

    event = SDL_Event{};
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.windowID = 0;
    event.button.button = SDL_BUTTON_LEFT;
    window.handle_event(translate_sdl_event(event, window), nullptr, &pointer_input);

    MK_REQUIRE(pointer_input.pointer_released(mirakana::primary_pointer_id));
}

MK_TEST("sdl3 window maps touch events into virtual pointer input") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(
        mirakana::WindowDesc{.title = "SDL Touch Test", .extent = mirakana::WindowExtent{.width = 400, .height = 200}});
    mirakana::VirtualPointerInput pointer_input;
    const auto pointer_id = mirakana::sdl3_touch_pointer_id(7);

    SDL_Event event{};
    event.type = SDL_EVENT_FINGER_DOWN;
    event.tfinger.windowID = 0;
    event.tfinger.fingerID = 7;
    event.tfinger.x = 0.25F;
    event.tfinger.y = 0.5F;
    window.handle_event(translate_sdl_event(event, window), nullptr, &pointer_input);

    MK_REQUIRE(pointer_input.pointer_pressed(pointer_id));
    MK_REQUIRE(pointer_input.pointer_position(pointer_id) == (mirakana::Vec2{100.0F, 100.0F}));

    event = SDL_Event{};
    event.type = SDL_EVENT_FINGER_UP;
    event.tfinger.windowID = 0;
    event.tfinger.fingerID = 7;
    window.handle_event(translate_sdl_event(event, window), nullptr, &pointer_input);

    MK_REQUIRE(pointer_input.pointer_released(pointer_id));
}

MK_TEST("sdl3 window maps touch cancel events into virtual pointer cancel") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Touch Cancel Test",
                                                    .extent = mirakana::WindowExtent{.width = 400, .height = 200}});
    mirakana::VirtualPointerInput pointer_input;
    const auto pointer_id = mirakana::sdl3_touch_pointer_id(9);

    SDL_Event event{};
    event.type = SDL_EVENT_FINGER_DOWN;
    event.tfinger.windowID = 0;
    event.tfinger.fingerID = 9;
    event.tfinger.x = 0.5F;
    event.tfinger.y = 0.25F;
    window.handle_event(translate_sdl_event(event, window), nullptr, &pointer_input);
    MK_REQUIRE(pointer_input.pointer_down(pointer_id));

    event = SDL_Event{};
    event.type = SDL_EVENT_FINGER_CANCELED;
    event.tfinger.windowID = 0;
    event.tfinger.fingerID = 9;
    window.handle_event(translate_sdl_event(event, window), nullptr, &pointer_input);

    MK_REQUIRE(!pointer_input.pointer_down(pointer_id));
    MK_REQUIRE(pointer_input.pointer_canceled(pointer_id));
    MK_REQUIRE(!pointer_input.pointer_released(pointer_id));
}

MK_TEST("sdl3 window ignores touch generated virtual mouse rows") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Touch Mouse Filter Test",
                                                    .extent = mirakana::WindowExtent{.width = 400, .height = 200}});
    mirakana::VirtualPointerInput pointer_input;

    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = 0;
    event.button.which = SDL_TOUCH_MOUSEID;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 20.0F;
    event.button.y = 40.0F;
    window.handle_event(translate_sdl_event(event, window), nullptr, &pointer_input);

    MK_REQUIRE(!pointer_input.pointer_down(mirakana::primary_pointer_id));
    MK_REQUIRE(pointer_input.pointers().empty());

    event = SDL_Event{};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = 0;
    event.motion.which = SDL_TOUCH_MOUSEID;
    event.motion.x = 30.0F;
    event.motion.y = 60.0F;
    window.handle_event(translate_sdl_event(event, window), nullptr, &pointer_input);

    MK_REQUIRE(pointer_input.pointers().empty());
}

MK_TEST("sdl3 window maps gamepad events into virtual gamepad input") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Gamepad Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});
    mirakana::VirtualGamepadInput gamepad_input;

    SDL_Event event{};
    event.type = SDL_EVENT_GAMEPAD_ADDED;
    event.gdevice.which = 7;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, &gamepad_input);
    MK_REQUIRE(gamepad_input.gamepad_connected(7));

    event = SDL_Event{};
    event.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    event.gbutton.which = 7;
    event.gbutton.button = SDL_GAMEPAD_BUTTON_SOUTH;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, &gamepad_input);
    MK_REQUIRE(gamepad_input.button_pressed(7, mirakana::GamepadButton::south));
    MK_REQUIRE(gamepad_input.button_down(7, mirakana::GamepadButton::south));

    event = SDL_Event{};
    event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
    event.gaxis.which = 7;
    event.gaxis.axis = SDL_GAMEPAD_AXIS_LEFTX;
    event.gaxis.value = 32767;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, &gamepad_input);
    MK_REQUIRE(gamepad_input.axis_value(7, mirakana::GamepadAxis::left_x) == 1.0F);

    event = SDL_Event{};
    event.type = SDL_EVENT_GAMEPAD_BUTTON_UP;
    event.gbutton.which = 7;
    event.gbutton.button = SDL_GAMEPAD_BUTTON_SOUTH;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, &gamepad_input);
    MK_REQUIRE(gamepad_input.button_released(7, mirakana::GamepadButton::south));

    event = SDL_Event{};
    event.type = SDL_EVENT_GAMEPAD_REMOVED;
    event.gdevice.which = 7;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, &gamepad_input);
    MK_REQUIRE(!gamepad_input.gamepad_connected(7));
}

MK_TEST("sdl3 runtime enumerates display info") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});

    const auto displays = mirakana::sdl3_displays();
    MK_REQUIRE(!displays.empty());

    bool found_primary = false;
    for (const auto& display : displays) {
        MK_REQUIRE(mirakana::is_valid_display_info(display));
        found_primary = found_primary || display.primary;
    }
    MK_REQUIRE(found_primary);
}

MK_TEST("sdl3 runtime selects display through first party monitor policy") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});

    const auto selected = mirakana::sdl3_select_display(
        mirakana::DisplaySelectionRequest{.policy = mirakana::DisplaySelectionPolicy::primary});

    MK_REQUIRE(selected.has_value());
    MK_REQUIRE(mirakana::is_valid_display_info(*selected));
}

MK_TEST("sdl3 window reports display scale density and safe area") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Display Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});

    const auto state = window.display_state();

    MK_REQUIRE(mirakana::is_valid_window_display_state(state));
    MK_REQUIRE(state.safe_area.width > 0);
    MK_REQUIRE(state.safe_area.height > 0);
}

MK_TEST("sdl3 window tracks position and applies placement") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Placement Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240},
                                                    .position = mirakana::WindowPosition{.x = 20, .y = 30}});

    window.move(mirakana::WindowPosition{.x = 40, .y = 50});
    MK_REQUIRE(window.position().x == 40);
    MK_REQUIRE(window.position().y == 50);

    window.apply_placement(mirakana::WindowPlacement{.position = mirakana::WindowPosition{.x = 60, .y = 70},
                                                     .extent = mirakana::WindowExtent{.width = 400, .height = 300},
                                                     .display_id = 1});
    MK_REQUIRE(window.position().x == 60);
    MK_REQUIRE(window.position().y == 70);
    MK_REQUIRE(window.extent().width == 400);
    MK_REQUIRE(window.extent().height == 300);
}

MK_TEST("sdl3 platform integration adapter begins text input with a window text area") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Text Input Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});
    mirakana::SdlPlatformIntegrationAdapter adapter(window);

    const mirakana::ui::PlatformTextInputRequest request{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text_bounds = mirakana::ui::Rect{.x = 4.0F, .y = 8.0F, .width = 160.0F, .height = 24.0F},
    };

    const auto result = mirakana::ui::begin_platform_text_input(adapter, request);

    MK_REQUIRE(result.succeeded());
    auto* native = reinterpret_cast<SDL_Window*>(window.native_window().value);
    MK_REQUIRE(SDL_TextInputActive(native));

    SDL_Rect area{};
    int cursor = -1;
    MK_REQUIRE(SDL_GetTextInputArea(native, &area, &cursor));
    MK_REQUIRE(area.x == 4);
    MK_REQUIRE(area.y == 8);
    MK_REQUIRE(area.w == 160);
    MK_REQUIRE(area.h == 24);
    MK_REQUIRE(cursor == 0);

    const auto end_result = mirakana::ui::end_platform_text_input(adapter, request.target);
    MK_REQUIRE(end_result.succeeded());
    MK_REQUIRE(!SDL_TextInputActive(native));
}

MK_TEST("sdl3 platform integration adapter rejects invalid direct requests before calling SDL") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Text Input Invalid Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});
    mirakana::SdlPlatformIntegrationAdapter adapter(window);

    bool threw = false;
    try {
        adapter.begin_text_input(mirakana::ui::PlatformTextInputRequest{
            .target = mirakana::ui::ElementId{},
            .text_bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 10.0F, .height = 10.0F},
        });
    } catch (const std::invalid_argument& error) {
        threw = std::string{error.what()} == "platform text input target must not be empty";
    }

    MK_REQUIRE(threw);
    auto* native = reinterpret_cast<SDL_Window*>(window.native_window().value);
    MK_REQUIRE(!SDL_TextInputActive(native));
}

MK_TEST("sdl3 window translates text input events into copied first party text") {
    std::array<char, 13> text{"chat message"};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.windowID = 42;
    event.text.text = text.data();

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    text[0] = 'C';

    MK_REQUIRE(translated.kind == mirakana::SdlWindowEventKind::text_input);
    MK_REQUIRE(translated.window_id == 42);
    MK_REQUIRE(translated.text == "chat message");
}

MK_TEST("sdl3 window translates text editing events into copied composition rows") {
    std::array<char, 4> text{"\xE3\x81\x82"};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_EDITING;
    event.edit.windowID = 43;
    event.edit.text = text.data();
    event.edit.start = 1;
    event.edit.length = 2;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    text[0] = '\0';

    MK_REQUIRE(translated.kind == mirakana::SdlWindowEventKind::text_editing);
    MK_REQUIRE(translated.window_id == 43);
    MK_REQUIRE(translated.text == "\xE3\x81\x82");
    MK_REQUIRE(translated.text_edit_start == 1);
    MK_REQUIRE(translated.text_edit_length == 2);
}

MK_TEST("sdl3 platform integration builds ime composition from text editing events") {
    std::array<char, 7> text{"\xE3\x81\x82\xE3\x81\x84"};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_EDITING;
    event.edit.windowID = 46;
    event.edit.text = text.data();
    event.edit.start = 1;
    event.edit.length = 0;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});

    const auto composition =
        mirakana::sdl3_ime_composition_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);

    if (!composition.has_value()) {
        MK_REQUIRE(false);
        return;
    }
    const auto& published = composition.value();
    MK_REQUIRE(published.target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(published.composition_text == "\xE3\x81\x82\xE3\x81\x84");
    MK_REQUIRE(published.cursor_index == 3U);
}

MK_TEST("sdl3 platform integration publishes text editing composition through ge ui adapter") {
    std::array<char, 4> text{"abc"};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_EDITING;
    event.edit.windowID = 47;
    event.edit.text = text.data();
    event.edit.start = 2;
    event.edit.length = 0;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    CapturingSdlImeAdapter adapter;

    const auto result =
        mirakana::publish_sdl3_ime_composition_event(adapter, mirakana::ui::ElementId{"chat.input"}, translated);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(adapter.publish_count() == 1);
    MK_REQUIRE(adapter.published_composition().target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(adapter.published_composition().composition_text == "abc");
    MK_REQUIRE(adapter.published_composition().cursor_index == 2U);
}

MK_TEST("sdl3 platform integration ignores non editing events for ime composition") {
    std::array<char, 7> text{"submit"};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.windowID = 48;
    event.text.text = text.data();

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    CapturingSdlImeAdapter adapter;

    const auto composition =
        mirakana::sdl3_ime_composition_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    const auto result =
        mirakana::publish_sdl3_ime_composition_event(adapter, mirakana::ui::ElementId{"chat.input"}, translated);

    MK_REQUIRE(!composition.has_value());
    MK_REQUIRE(!result.published);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(adapter.publish_count() == 0);
}

MK_TEST("sdl3 platform integration reports invalid ime target without adapter dispatch") {
    std::array<char, 4> text{"abc"};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_EDITING;
    event.edit.windowID = 49;
    event.edit.text = text.data();
    event.edit.start = 0;
    event.edit.length = 0;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    CapturingSdlImeAdapter adapter;

    const auto result = mirakana::publish_sdl3_ime_composition_event(adapter, mirakana::ui::ElementId{}, translated);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.published);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_ime_target);
    MK_REQUIRE(adapter.publish_count() == 0);
}

MK_TEST("sdl3 platform integration publishes empty composition clear rows") {
    std::array<char, 1> text{'\0'};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_EDITING;
    event.edit.windowID = 50;
    event.edit.text = text.data();
    event.edit.start = 0;
    event.edit.length = 0;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    CapturingSdlImeAdapter adapter;

    const auto result =
        mirakana::publish_sdl3_ime_composition_event(adapter, mirakana::ui::ElementId{"chat.input"}, translated);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(adapter.publish_count() == 1);
    MK_REQUIRE(adapter.published_composition().composition_text.empty());
    MK_REQUIRE(adapter.published_composition().cursor_index == 0U);
}

MK_TEST("sdl3 platform integration clamps unset and oversized ime cursor rows") {
    std::array<char, 7> text{"\xE3\x81\x82\xE3\x81\x84"};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_EDITING;
    event.edit.windowID = 51;
    event.edit.text = text.data();
    event.edit.start = -1;
    event.edit.length = -1;

    auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                            mirakana::WindowExtent{.width = 320, .height = 240});
    auto composition =
        mirakana::sdl3_ime_composition_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    if (!composition.has_value()) {
        MK_REQUIRE(false);
        return;
    }
    MK_REQUIRE(composition.value().cursor_index == 0U);

    event.edit.start = 99;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    composition = mirakana::sdl3_ime_composition_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    if (!composition.has_value()) {
        MK_REQUIRE(false);
        return;
    }
    const auto& clamped = composition.value();
    MK_REQUIRE(clamped.cursor_index == clamped.composition_text.size());
}

MK_TEST("sdl3 platform integration applies committed text input rows through ge ui text edit state") {
    std::array<char, 4> text{"llo"};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.windowID = 52;
    event.text.text = text.data();

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "he",
        .cursor_byte_offset = 2,
        .selection_byte_length = 0,
    };

    const auto committed =
        mirakana::sdl3_committed_text_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    if (!committed.has_value()) {
        MK_REQUIRE(false);
        return;
    }
    MK_REQUIRE(committed->target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(committed->text == "llo");

    const auto result =
        mirakana::apply_sdl3_committed_text_event(state, mirakana::ui::ElementId{"chat.input"}, translated);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.committed);
    MK_REQUIRE(result.state.text == "hello");
    MK_REQUIRE(result.state.cursor_byte_offset == 5U);
}

MK_TEST("sdl3 platform integration applies text edit command key rows through ge ui state") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_DELETE;
    event.key.repeat = true;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 1,
        .selection_byte_length = 0,
    };

    const auto result =
        mirakana::apply_sdl3_text_edit_command_event(state, mirakana::ui::ElementId{"chat.input"}, translated);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.state.text == "ab");
    MK_REQUIRE(result.state.cursor_byte_offset == 1U);
    MK_REQUIRE(result.state.selection_byte_length == 0U);
}

MK_TEST("sdl3 platform integration leaves state unchanged for ignored text edit command rows") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_UP;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello",
        .cursor_byte_offset = 5,
        .selection_byte_length = 0,
    };

    const auto result =
        mirakana::apply_sdl3_text_edit_command_event(state, mirakana::ui::ElementId{"chat.input"}, translated);

    MK_REQUIRE(!result.applied);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.state.text == "hello");
    MK_REQUIRE(result.state.cursor_byte_offset == 5U);
    MK_REQUIRE(result.state.selection_byte_length == 0U);
}

MK_TEST("sdl3 platform integration forwards text edit command diagnostics") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_DELETE;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello",
        .cursor_byte_offset = 0,
        .selection_byte_length = 0,
    };

    const auto result =
        mirakana::apply_sdl3_text_edit_command_event(state, mirakana::ui::ElementId{"other.input"}, translated);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.applied);
    MK_REQUIRE(result.state.text == state.text);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::mismatched_text_edit_command_target);
}

MK_TEST("sdl3 key rows copy raw keycode and shortcut modifiers") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_C;
    event.key.mod = SDL_KMOD_CTRL;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});

    MK_REQUIRE(translated.kind == mirakana::SdlWindowEventKind::key_pressed);
    MK_REQUIRE(translated.key == mirakana::Key::unknown);
    MK_REQUIRE(translated.sdl_keycode == SDLK_C);
    MK_REQUIRE((translated.key_modifiers & SDL_KMOD_CTRL) != 0U);
}

MK_TEST("sdl3 platform integration maps shortcut rows to text edit clipboard commands") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_C;
    event.key.mod = SDL_KMOD_CTRL;

    auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                            mirakana::WindowExtent{.width = 320, .height = 240});
    auto command =
        mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);

    const auto& copy_command = require_optional_value(command, "command.has_value()");
    MK_REQUIRE(copy_command.target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(copy_command.kind == mirakana::ui::TextEditClipboardCommandKind::copy_selection);

    event.key.key = SDLK_X;
    event.key.mod = SDL_KMOD_GUI;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    command =
        mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    const auto& cut_command = require_optional_value(command, "command.has_value()");
    MK_REQUIRE(cut_command.kind == mirakana::ui::TextEditClipboardCommandKind::cut_selection);

    event.key.key = SDLK_V;
    event.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    command =
        mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    const auto& paste_command = require_optional_value(command, "command.has_value()");
    MK_REQUIRE(paste_command.kind == mirakana::ui::TextEditClipboardCommandKind::paste_text);
}

MK_TEST("sdl3 platform integration ignores non clipboard shortcut rows") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_UP;
    event.key.key = SDLK_C;
    event.key.mod = SDL_KMOD_CTRL;

    auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                            mirakana::WindowExtent{.width = 320, .height = 240});
    MK_REQUIRE(
        !mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated)
             .has_value());

    event = SDL_Event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_C;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    MK_REQUIRE(
        !mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated)
             .has_value());

    event.key.mod = SDL_KMOD_CTRL | SDL_KMOD_ALT;
    translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                       mirakana::WindowExtent{.width = 320, .height = 240});
    MK_REQUIRE(
        !mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated)
             .has_value());

    const mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlClipboard clipboard;
    mirakana::SdlClipboardTextAdapter adapter{clipboard};
    clipboard.set_text("unchanged");

    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello",
        .cursor_byte_offset = 5,
        .selection_byte_length = 0,
    };
    const auto ignored = mirakana::apply_sdl3_text_edit_clipboard_command_event(
        adapter, state, mirakana::ui::ElementId{"chat.input"}, translated);

    MK_REQUIRE(!ignored.applied);
    MK_REQUIRE(ignored.diagnostics.empty());
    MK_REQUIRE(ignored.state.text == "hello");
    MK_REQUIRE(ignored.state.cursor_byte_offset == 5U);
    MK_REQUIRE(clipboard.text() == "unchanged");
}

MK_TEST("sdl3 platform integration applies clipboard shortcuts through ge ui state") {
    const mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlClipboard clipboard;
    mirakana::SdlClipboardTextAdapter adapter{clipboard};
    clipboard.clear();

    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_X;
    event.key.mod = SDL_KMOD_CTRL;

    const auto cut_event = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                 mirakana::WindowExtent{.width = 320, .height = 240});
    const mirakana::ui::TextEditState cut_state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello world",
        .cursor_byte_offset = 6,
        .selection_byte_length = 5,
    };

    const auto cut = mirakana::apply_sdl3_text_edit_clipboard_command_event(
        adapter, cut_state, mirakana::ui::ElementId{"chat.input"}, cut_event);

    MK_REQUIRE(cut.succeeded());
    MK_REQUIRE(cut.applied);
    MK_REQUIRE(cut.state.text == "hello ");
    MK_REQUIRE(clipboard.text() == "world");

    event.key.key = SDLK_V;
    const auto paste_event = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                   mirakana::WindowExtent{.width = 320, .height = 240});
    const mirakana::ui::TextEditState paste_state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "say ",
        .cursor_byte_offset = 4,
        .selection_byte_length = 0,
    };

    const auto paste = mirakana::apply_sdl3_text_edit_clipboard_command_event(
        adapter, paste_state, mirakana::ui::ElementId{"chat.input"}, paste_event);

    MK_REQUIRE(paste.succeeded());
    MK_REQUIRE(paste.applied);
    MK_REQUIRE(paste.state.text == "say world");
    MK_REQUIRE(paste.state.cursor_byte_offset == 9U);
}

MK_TEST("sdl3 platform integration ignores non committed text rows for text edit apply") {
    std::array<char, 4> text{"ime"};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_EDITING;
    event.edit.windowID = 53;
    event.edit.text = text.data();
    event.edit.start = 0;
    event.edit.length = 0;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello",
        .cursor_byte_offset = 5,
        .selection_byte_length = 0,
    };

    const auto committed =
        mirakana::sdl3_committed_text_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    const auto result =
        mirakana::apply_sdl3_committed_text_event(state, mirakana::ui::ElementId{"chat.input"}, translated);

    MK_REQUIRE(!committed.has_value());
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.state.text == "hello");
    MK_REQUIRE(result.state.cursor_byte_offset == 5U);
}

MK_TEST("sdl3 platform integration reports invalid committed text target through ge ui plan") {
    std::array<char, 2> text{"x"};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.windowID = 54;
    event.text.text = text.data();

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello",
        .cursor_byte_offset = 5,
        .selection_byte_length = 0,
    };

    const auto result = mirakana::apply_sdl3_committed_text_event(state, mirakana::ui::ElementId{}, translated);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(result.state.text == "hello");
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::mismatched_committed_text_target);
}

MK_TEST("sdl3 window translates text editing candidate events into copied candidate rows") {
    std::array<char, 4> first{"\xE3\x81\x82"};
    std::array<char, 4> second{"\xE3\x81\x84"};
    std::array<const char*, 2> candidates{first.data(), second.data()};
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_EDITING_CANDIDATES;
    event.edit_candidates.windowID = 44;
    event.edit_candidates.candidates = candidates.data();
    event.edit_candidates.num_candidates = static_cast<std::int32_t>(candidates.size());
    event.edit_candidates.selected_candidate = 1;
    event.edit_candidates.horizontal = true;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});
    first[0] = '\0';
    second[0] = '\0';

    MK_REQUIRE(translated.kind == mirakana::SdlWindowEventKind::text_editing_candidates);
    MK_REQUIRE(translated.window_id == 44);
    MK_REQUIRE(translated.text_editing_candidates.size() == 2);
    MK_REQUIRE(translated.text_editing_candidates[0] == "\xE3\x81\x82");
    MK_REQUIRE(translated.text_editing_candidates[1] == "\xE3\x81\x84");
    MK_REQUIRE(translated.selected_text_editing_candidate == 1);
    MK_REQUIRE(translated.text_editing_candidates_horizontal);
}

MK_TEST("sdl3 window translates null text editing candidate lists as empty rows") {
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_EDITING_CANDIDATES;
    event.edit_candidates.windowID = 45;
    event.edit_candidates.candidates = nullptr;
    event.edit_candidates.num_candidates = 0;
    event.edit_candidates.selected_candidate = -1;
    event.edit_candidates.horizontal = false;

    const auto translated = mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event},
                                                                  mirakana::WindowExtent{.width = 320, .height = 240});

    MK_REQUIRE(translated.kind == mirakana::SdlWindowEventKind::text_editing_candidates);
    MK_REQUIRE(translated.window_id == 45);
    MK_REQUIRE(translated.text_editing_candidates.empty());
    MK_REQUIRE(translated.selected_text_editing_candidate == -1);
    MK_REQUIRE(!translated.text_editing_candidates_horizontal);
}

MK_TEST("sdl3 clipboard reads writes and clears text") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlClipboard clipboard;

    clipboard.clear();
    MK_REQUIRE(clipboard.text().empty());

    clipboard.set_text("sdl clipboard");
    MK_REQUIRE(clipboard.has_text());
    MK_REQUIRE(clipboard.text() == "sdl clipboard");

    clipboard.clear();
    MK_REQUIRE(!clipboard.has_text());
    MK_REQUIRE(clipboard.text().empty());
}

MK_TEST("sdl3 clipboard text adapter dispatches ge ui clipboard requests") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlClipboard clipboard;
    mirakana::SdlClipboardTextAdapter adapter{clipboard};

    clipboard.clear();

    const mirakana::ui::ClipboardTextWriteRequest write_request{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "sdl adapter clipboard",
    };
    const auto write = mirakana::ui::write_clipboard_text(adapter, write_request);
    MK_REQUIRE(write.succeeded());
    MK_REQUIRE(clipboard.has_text());
    MK_REQUIRE(clipboard.text() == "sdl adapter clipboard");

    const auto read = mirakana::ui::read_clipboard_text(
        adapter, mirakana::ui::ClipboardTextReadRequest{.target = write_request.target});
    MK_REQUIRE(read.succeeded());
    MK_REQUIRE(read.read);
    MK_REQUIRE(read.has_text);
    MK_REQUIRE(read.text == "sdl adapter clipboard");

    const auto clear = mirakana::ui::write_clipboard_text(
        adapter, mirakana::ui::ClipboardTextWriteRequest{.target = write_request.target, .text = ""});
    MK_REQUIRE(clear.succeeded());
    MK_REQUIRE(!clipboard.has_text());

    const auto empty = mirakana::ui::read_clipboard_text(
        adapter, mirakana::ui::ClipboardTextReadRequest{.target = write_request.target});
    MK_REQUIRE(empty.succeeded());
    MK_REQUIRE(empty.read);
    MK_REQUIRE(!empty.has_text);
    MK_REQUIRE(empty.text.empty());
}

MK_TEST("sdl3 clipboard text adapter composes with ge ui text edit clipboard commands") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlClipboard clipboard;
    mirakana::SdlClipboardTextAdapter adapter{clipboard};
    clipboard.clear();

    const mirakana::ui::ElementId target{"chat.input"};
    const mirakana::ui::TextEditState cut_state{
        .target = target,
        .text = "hello world",
        .cursor_byte_offset = 6,
        .selection_byte_length = 5,
    };
    const auto cut = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, cut_state,
        mirakana::ui::TextEditClipboardCommand{.target = target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::cut_selection});

    MK_REQUIRE(cut.succeeded());
    MK_REQUIRE(cut.state.text == "hello ");
    MK_REQUIRE(cut.state.cursor_byte_offset == 6U);
    MK_REQUIRE(cut.state.selection_byte_length == 0U);
    MK_REQUIRE(clipboard.has_text());
    MK_REQUIRE(clipboard.text() == "world");

    const mirakana::ui::TextEditState paste_state{
        .target = target,
        .text = "say ",
        .cursor_byte_offset = 4,
        .selection_byte_length = 0,
    };
    const auto paste = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, paste_state,
        mirakana::ui::TextEditClipboardCommand{.target = target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::paste_text});

    MK_REQUIRE(paste.succeeded());
    MK_REQUIRE(paste.state.text == "say world");
    MK_REQUIRE(paste.state.cursor_byte_offset == 9U);
    MK_REQUIRE(paste.state.selection_byte_length == 0U);
}

MK_TEST("sdl3 cursor applies normal hidden confined and relative modes") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Cursor Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});
    mirakana::SdlCursor cursor(window);

    cursor.set_mode(mirakana::CursorMode::normal);
    MK_REQUIRE(cursor.state().mode == mirakana::CursorMode::normal);
    MK_REQUIRE(cursor.state().visible);
    MK_REQUIRE(!cursor.state().grabbed);
    MK_REQUIRE(!cursor.state().relative);

    cursor.set_mode(mirakana::CursorMode::hidden);
    MK_REQUIRE(cursor.state().mode == mirakana::CursorMode::hidden);
    MK_REQUIRE(!cursor.state().visible);
    MK_REQUIRE(!cursor.state().grabbed);
    MK_REQUIRE(!cursor.state().relative);

    cursor.set_mode(mirakana::CursorMode::confined);
    MK_REQUIRE(cursor.state().mode == mirakana::CursorMode::confined);
    MK_REQUIRE(cursor.state().visible);
    MK_REQUIRE(cursor.state().grabbed);
    MK_REQUIRE(!cursor.state().relative);

    cursor.set_mode(mirakana::CursorMode::relative);
    MK_REQUIRE(cursor.state().mode == mirakana::CursorMode::relative);
    MK_REQUIRE(!cursor.state().visible);
    MK_REQUIRE(cursor.state().grabbed);
    MK_REQUIRE(cursor.state().relative);

    cursor.set_mode(mirakana::CursorMode::normal);
}

MK_TEST("sdl3 window maps application lifecycle events") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Lifecycle Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});
    mirakana::VirtualLifecycle lifecycle;

    SDL_Event event{};
    event.type = SDL_EVENT_LOW_MEMORY;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, nullptr, &lifecycle);
    MK_REQUIRE(lifecycle.state().low_memory);
    MK_REQUIRE(lifecycle.events()[0].kind == mirakana::LifecycleEventKind::low_memory);

    lifecycle.begin_frame();
    event = SDL_Event{};
    event.type = SDL_EVENT_WILL_ENTER_BACKGROUND;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, nullptr, &lifecycle);
    event = SDL_Event{};
    event.type = SDL_EVENT_DID_ENTER_BACKGROUND;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, nullptr, &lifecycle);
    MK_REQUIRE(lifecycle.state().backgrounded);
    MK_REQUIRE(!lifecycle.state().interactive);

    lifecycle.begin_frame();
    event = SDL_Event{};
    event.type = SDL_EVENT_WILL_ENTER_FOREGROUND;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, nullptr, &lifecycle);
    event = SDL_Event{};
    event.type = SDL_EVENT_DID_ENTER_FOREGROUND;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, nullptr, &lifecycle);
    MK_REQUIRE(!lifecycle.state().backgrounded);
    MK_REQUIRE(lifecycle.state().interactive);
}

MK_TEST("sdl3 quit and terminating events request lifecycle shutdown") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Shutdown Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});
    mirakana::VirtualLifecycle lifecycle;

    SDL_Event event{};
    event.type = SDL_EVENT_QUIT;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, nullptr, &lifecycle);
    MK_REQUIRE(lifecycle.state().quit_requested);
    MK_REQUIRE(!window.is_open());

    event = SDL_Event{};
    event.type = SDL_EVENT_TERMINATING;
    window.handle_event(translate_sdl_event(event, window), nullptr, nullptr, nullptr, &lifecycle);
    MK_REQUIRE(lifecycle.state().terminating);
}

MK_TEST("sdl3 window wraps runtime window state") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(
        mirakana::WindowDesc{.title = "SDL Test", .extent = mirakana::WindowExtent{.width = 320, .height = 240}});

    MK_REQUIRE(window.native_window().value != 0);
    MK_REQUIRE(window.title() == "SDL Test");
    MK_REQUIRE(window.extent().width == 320);
    MK_REQUIRE(window.extent().height == 240);
    MK_REQUIRE(window.is_open());

    window.resize(mirakana::WindowExtent{.width = 640, .height = 480});
    MK_REQUIRE(window.extent().width == 640);
    MK_REQUIRE(window.extent().height == 480);

    window.request_close();
    MK_REQUIRE(!window.is_open());
}

int main() {
    return mirakana::test::run_all();
}
