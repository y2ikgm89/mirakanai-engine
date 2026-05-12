// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/sdl3/sdl_desktop_event_pump.hpp"

#include <SDL3/SDL_events.h>

namespace mirakana {

SdlDesktopEventPump::SdlDesktopEventPump(SdlWindow& window) noexcept : window_(&window) {}

void SdlDesktopEventPump::pump_events(DesktopHostServices& services) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        const auto translated = sdl3_translate_window_event(SdlRawEventHandle{&event}, window_->extent());
        window_->handle_event(translated, services.input, services.pointer_input, services.gamepad_input,
                              services.lifecycle);
    }
}

} // namespace mirakana
