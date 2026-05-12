// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/version.hpp"

#include <iostream>

class SandboxApp final : public mirakana::GameApp {
  public:
    bool on_update(mirakana::EngineContext& /*context*/, double /*delta_seconds*/) override {
        ++frames_;
        return frames_ < 1;
    }

  private:
    int frames_{0};
};

int main() {
    constexpr auto version = mirakana::engine_version();
    std::cout << version.name << " " << version.major << "." << version.minor << "." << version.patch << '\n';

    mirakana::RingBufferLogger logger(16);
    mirakana::Registry registry;
    mirakana::HeadlessRunner runner(logger, registry);
    SandboxApp app;

    const auto result = runner.run(app, mirakana::RunConfig{.max_frames = 1, .fixed_delta_seconds = 1.0 / 60.0});
    return result.frames_run == 1 ? 0 : 1;
}
