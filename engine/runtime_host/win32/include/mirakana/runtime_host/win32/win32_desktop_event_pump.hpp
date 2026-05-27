// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/win32/win32_event_pump.hpp"
#include "mirakana/platform/win32/win32_window.hpp"
#include "mirakana/runtime_host/desktop_runner.hpp"

namespace mirakana {

class Win32DesktopEventPump final : public IDesktopEventPump {
  public:
    explicit Win32DesktopEventPump(win32::Win32Window& window) noexcept;

    void pump_events(DesktopHostServices& services) override;

  private:
    win32::Win32Window* window_{nullptr};
    win32::Win32EventPump pump_;
};

} // namespace mirakana
