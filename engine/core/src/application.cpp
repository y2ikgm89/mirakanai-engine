// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/application.hpp"

namespace mirakana {

void GameApp::on_start(EngineContext& /*unused*/) {}

void GameApp::on_stop(EngineContext& /*unused*/) {}

HeadlessRunner::HeadlessRunner(ILogger& logger, Registry& registry) : logger_(logger), registry_(registry) {}

RunResult HeadlessRunner::run(GameApp& app, RunConfig config) {
    EngineContext context{.logger = logger_, .registry = registry_};
    app.on_start(context);

    std::uint32_t frames = 0;
    RunStatus status = RunStatus::completed;

    for (; frames < config.max_frames; ++frames) {
        if (!app.on_update(context, config.fixed_delta_seconds)) {
            status = RunStatus::stopped_by_app;
            ++frames;
            break;
        }
    }

    app.on_stop(context);
    return RunResult{.status = status, .frames_run = frames};
}

} // namespace mirakana
