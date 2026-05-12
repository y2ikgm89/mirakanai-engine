// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/desktop_runner.hpp"

#include <optional>
#include <stdexcept>

namespace mirakana {
namespace {

[[nodiscard]] bool reached_frame_limit(std::uint32_t frames, std::uint32_t max_frames) noexcept {
    return max_frames != 0 && frames >= max_frames;
}

[[nodiscard]] bool lifecycle_requests_quit(const VirtualLifecycle* lifecycle) noexcept {
    if (lifecycle == nullptr) {
        return false;
    }

    const auto state = lifecycle->state();
    return state.quit_requested || state.terminating;
}

void begin_frame_inputs(DesktopHostServices& services) {
    if (services.input != nullptr) {
        services.input->begin_frame();
    }
    if (services.pointer_input != nullptr) {
        services.pointer_input->begin_frame();
    }
    if (services.gamepad_input != nullptr) {
        services.gamepad_input->begin_frame();
    }
    if (services.lifecycle != nullptr) {
        services.lifecycle->begin_frame();
    }
}

void resize_renderer_to_window(DesktopHostServices& services) {
    if (services.renderer == nullptr) {
        return;
    }

    const auto window_extent = services.window->extent();
    if (window_extent.width == 0 || window_extent.height == 0) {
        return;
    }

    const auto renderer_extent = services.renderer->backbuffer_extent();
    if (renderer_extent.width == window_extent.width && renderer_extent.height == window_extent.height) {
        return;
    }

    services.renderer->resize(Extent2D{.width = window_extent.width, .height = window_extent.height});
}

void record_renderer_counters(DiagnosticsRecorder& recorder, const IRenderer& renderer, std::uint64_t frame_index) {
    const auto stats = renderer.stats();
    recorder.record_counter(CounterSample{.name = "renderer.frames_started",
                                          .value = static_cast<double>(stats.frames_started),
                                          .frame_index = frame_index});
    recorder.record_counter(CounterSample{.name = "renderer.frames_finished",
                                          .value = static_cast<double>(stats.frames_finished),
                                          .frame_index = frame_index});
    recorder.record_counter(CounterSample{.name = "renderer.sprites_submitted",
                                          .value = static_cast<double>(stats.sprites_submitted),
                                          .frame_index = frame_index});
    recorder.record_counter(CounterSample{.name = "renderer.meshes_submitted",
                                          .value = static_cast<double>(stats.meshes_submitted),
                                          .frame_index = frame_index});
}

void validate_services(const DesktopHostServices& services) {
    if (services.window == nullptr) {
        throw std::invalid_argument("desktop game runner requires a window service");
    }
}

void validate_config(DesktopRunConfig config) {
    if (config.fixed_delta_seconds <= 0.0) {
        throw std::invalid_argument("desktop game runner fixed delta must be positive");
    }
}

} // namespace

DesktopGameRunner::DesktopGameRunner(ILogger& logger, Registry& registry) : logger_(logger), registry_(registry) {}

DesktopRunResult DesktopGameRunner::run(GameApp& app, DesktopHostServices& services, DesktopRunConfig config,
                                        IDesktopEventPump* event_pump) {
    validate_services(services);
    validate_config(config);

    EngineContext context{.logger = logger_, .registry = registry_};
    app.on_start(context);

    std::uint32_t frames = 0;
    DesktopRunStatus status = DesktopRunStatus::completed;
    SteadyProfileClock steady_clock;

    while (!reached_frame_limit(frames, config.max_frames)) {
        const auto frame_index = static_cast<std::uint64_t>(frames);
        const IProfileClock& profile_clock = services.profile_clock != nullptr ? *services.profile_clock : steady_clock;
        std::optional<ScopedProfileZone> frame_profile;
        if (services.diagnostics_recorder != nullptr) {
            frame_profile.emplace(*services.diagnostics_recorder, profile_clock, "runtime_host.frame", frame_index);
        }

        begin_frame_inputs(services);

        if (!services.window->is_open()) {
            status = DesktopRunStatus::window_closed;
            break;
        }

        if (event_pump != nullptr) {
            event_pump->pump_events(services);
        }

        if (!services.window->is_open()) {
            status = DesktopRunStatus::window_closed;
            break;
        }

        if (lifecycle_requests_quit(services.lifecycle)) {
            status = DesktopRunStatus::lifecycle_quit;
            break;
        }

        if (config.resize_renderer_to_window) {
            resize_renderer_to_window(services);
        }

        const bool continue_running = app.on_update(context, config.fixed_delta_seconds);
        if (services.diagnostics_recorder != nullptr && services.renderer != nullptr) {
            record_renderer_counters(*services.diagnostics_recorder, *services.renderer, frame_index);
        }

        if (!continue_running) {
            status = DesktopRunStatus::stopped_by_app;
            ++frames;
            break;
        }

        ++frames;
    }

    app.on_stop(context);
    return DesktopRunResult{.status = status, .frames_run = frames};
}

} // namespace mirakana
