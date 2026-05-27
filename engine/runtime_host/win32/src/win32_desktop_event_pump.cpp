// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/win32/win32_desktop_event_pump.hpp"

namespace mirakana {

Win32DesktopEventPump::Win32DesktopEventPump(win32::Win32Window& window) noexcept : window_(&window) {}

void Win32DesktopEventPump::pump_events(DesktopHostServices& services) {
    if (window_ == nullptr) {
        return;
    }

    for (const auto& event : pump_.poll()) {
        window_->handle_event(event, services.lifecycle);
    }
}

} // namespace mirakana
