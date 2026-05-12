// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/sdl3/sdl_cursor.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace mirakana {
namespace {

[[nodiscard]] std::runtime_error sdl_cursor_error(const char* operation) {
    return std::runtime_error(std::string(operation) + " failed: " + SDL_GetError());
}

void set_relative_mode(SDL_Window* window, bool enabled) {
    if (!SDL_SetWindowRelativeMouseMode(window, enabled)) {
        throw sdl_cursor_error("SDL_SetWindowRelativeMouseMode");
    }
}

void set_mouse_grab(SDL_Window* window, bool enabled) {
    if (!SDL_SetWindowMouseGrab(window, enabled)) {
        throw sdl_cursor_error("SDL_SetWindowMouseGrab");
    }
}

void set_cursor_visible(bool visible) {
    const bool ok = visible ? SDL_ShowCursor() : SDL_HideCursor();
    if (!ok) {
        throw sdl_cursor_error(visible ? "SDL_ShowCursor" : "SDL_HideCursor");
    }
}

[[nodiscard]] SDL_Window* native_window(SdlWindow& window) noexcept {
    return reinterpret_cast<SDL_Window*>(window.native_window().value);
}

} // namespace

SdlCursor::SdlCursor(SdlWindow& window) noexcept : window_(&window) {}

CursorState SdlCursor::state() const {
    SDL_Window* window = native_window(*window_);
    const bool relative = SDL_GetWindowRelativeMouseMode(window);
    if (relative) {
        return make_cursor_state(CursorMode::relative);
    }

    const bool grabbed = SDL_GetWindowMouseGrab(window);
    if (grabbed) {
        return make_cursor_state(CursorMode::confined);
    }

    return make_cursor_state(SDL_CursorVisible() ? CursorMode::normal : CursorMode::hidden);
}

void SdlCursor::set_mode(CursorMode mode) {
    SDL_Window* window = native_window(*window_);

    if (mode != CursorMode::relative && SDL_GetWindowRelativeMouseMode(window)) {
        set_relative_mode(window, false);
    }

    switch (mode) {
    case CursorMode::normal:
        set_mouse_grab(window, false);
        set_cursor_visible(true);
        break;
    case CursorMode::hidden:
        set_mouse_grab(window, false);
        set_cursor_visible(false);
        break;
    case CursorMode::confined:
        set_mouse_grab(window, true);
        set_cursor_visible(true);
        break;
    case CursorMode::relative:
        set_cursor_visible(false);
        set_mouse_grab(window, true);
        set_relative_mode(window, true);
        break;
    }
}

} // namespace mirakana
