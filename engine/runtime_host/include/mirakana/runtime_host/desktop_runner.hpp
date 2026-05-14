// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/application.hpp"
#include "mirakana/core/diagnostics.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/window.hpp"
#include "mirakana/renderer/renderer.hpp"

#include <cstdint>

namespace mirakana {

struct DesktopHostServices {
    IWindow* window{nullptr};
    IRenderer* renderer{nullptr};
    VirtualInput* input{nullptr};
    VirtualPointerInput* pointer_input{nullptr};
    VirtualGamepadInput* gamepad_input{nullptr};
    VirtualLifecycle* lifecycle{nullptr};
    DiagnosticsRecorder* diagnostics_recorder{nullptr};
    const IProfileClock* profile_clock{nullptr};
};

struct DesktopRunConfig {
    std::uint32_t max_frames{0};
    double fixed_delta_seconds{1.0 / 60.0};
    bool resize_renderer_to_window{true};
};

enum class DesktopRunStatus : std::uint8_t {
    completed,
    stopped_by_app,
    window_closed,
    lifecycle_quit,
};

struct DesktopRunResult {
    DesktopRunStatus status{DesktopRunStatus::completed};
    std::uint32_t frames_run{0};
};

class IDesktopEventPump {
  public:
    virtual ~IDesktopEventPump() = default;

    virtual void pump_events(DesktopHostServices& services) = 0;
};

class DesktopGameRunner {
  public:
    DesktopGameRunner(ILogger& logger, Registry& registry);

    [[nodiscard]] DesktopRunResult run(GameApp& app, DesktopHostServices& services, DesktopRunConfig config,
                                       IDesktopEventPump* event_pump = nullptr);

  private:
    ILogger& logger_;
    Registry& registry_;
};

} // namespace mirakana
