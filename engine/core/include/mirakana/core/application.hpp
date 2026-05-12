// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"

#include <cstdint>

namespace mirakana {

struct EngineContext {
    ILogger& logger;
    Registry& registry;
};

class GameApp {
  public:
    virtual ~GameApp() = default;

    virtual void on_start(EngineContext& context);
    virtual bool on_update(EngineContext& context, double delta_seconds) = 0;
    virtual void on_stop(EngineContext& context);
};

struct RunConfig {
    std::uint32_t max_frames{1};
    double fixed_delta_seconds{1.0 / 60.0};
};

enum class RunStatus { completed, stopped_by_app };

struct RunResult {
    RunStatus status;
    std::uint32_t frames_run;
};

class HeadlessRunner {
  public:
    HeadlessRunner(ILogger& logger, Registry& registry);

    [[nodiscard]] RunResult run(GameApp& app, RunConfig config);

  private:
    ILogger& logger_;
    Registry& registry_;
};

} // namespace mirakana
