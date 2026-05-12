// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/sdl3/sdl_window.hpp"
#include "mirakana/runtime_host/desktop_runner.hpp"

namespace mirakana {

class SdlDesktopEventPump final : public IDesktopEventPump {
  public:
    explicit SdlDesktopEventPump(SdlWindow& window) noexcept;

    void pump_events(DesktopHostServices& services) override;

  private:
    SdlWindow* window_{nullptr};
};

} // namespace mirakana
