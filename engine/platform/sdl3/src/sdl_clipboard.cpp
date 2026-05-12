// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/sdl3/sdl_clipboard.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace mirakana {
namespace {

[[nodiscard]] std::runtime_error sdl_clipboard_error(const char* operation) {
    return std::runtime_error(std::string(operation) + " failed: " + SDL_GetError());
}

} // namespace

bool SdlClipboard::has_text() const {
    return SDL_HasClipboardText();
}

std::string SdlClipboard::text() const {
    char* value = SDL_GetClipboardText();
    if (value == nullptr) {
        throw sdl_clipboard_error("SDL_GetClipboardText");
    }

    std::string result{value};
    SDL_free(value);
    return result;
}

void SdlClipboard::set_text(std::string_view text) {
    const std::string value{text};
    if (!SDL_SetClipboardText(value.c_str())) {
        throw sdl_clipboard_error("SDL_SetClipboardText");
    }
}

void SdlClipboard::clear() {
    set_text({});
}

SdlClipboardTextAdapter::SdlClipboardTextAdapter(SdlClipboard& clipboard) noexcept : clipboard_(&clipboard) {}

void SdlClipboardTextAdapter::set_clipboard_text(std::string_view text) {
    clipboard_->set_text(text);
}

bool SdlClipboardTextAdapter::has_clipboard_text() const {
    return clipboard_->has_text();
}

std::string SdlClipboardTextAdapter::clipboard_text() const {
    return clipboard_->text();
}

} // namespace mirakana
