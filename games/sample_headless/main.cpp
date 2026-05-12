// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/application.hpp"
#include "mirakana/core/entity.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"

#include <iostream>

namespace {

struct Lifetime {
    int frames_remaining;
};

class SampleHeadlessGame final : public mirakana::GameApp {
  public:
    void on_start(mirakana::EngineContext& context) override {
        player_ = context.registry.create();
        context.registry.emplace<Lifetime>(player_, 3);
        context.logger.write(
            mirakana::LogRecord{.level = mirakana::LogLevel::info, .category = "sample", .message = "game started"});
    }

    bool on_update(mirakana::EngineContext& context, double /*delta_seconds*/) override {
        auto* lifetime = context.registry.try_get<Lifetime>(player_);
        if (lifetime == nullptr) {
            return false;
        }

        --lifetime->frames_remaining;
        ++frames_;
        return lifetime->frames_remaining > 0;
    }

    void on_stop(mirakana::EngineContext& context) override {
        context.registry.destroy(player_);
        context.logger.write(
            mirakana::LogRecord{.level = mirakana::LogLevel::info, .category = "sample", .message = "game stopped"});
    }

    [[nodiscard]] int frames() const noexcept {
        return frames_;
    }

  private:
    mirakana::Entity player_{};
    int frames_{0};
};

} // namespace

int main() {
    mirakana::RingBufferLogger logger(16);
    mirakana::Registry registry;
    mirakana::HeadlessRunner runner(logger, registry);
    SampleHeadlessGame game;

    const auto result = runner.run(game, mirakana::RunConfig{.max_frames = 16, .fixed_delta_seconds = 1.0 / 60.0});
    std::cout << "sample_headless frames=" << result.frames_run << '\n';

    return result.status == mirakana::RunStatus::stopped_by_app && result.frames_run == 3 && game.frames() == 3 &&
                   registry.living_count() == 0
               ? 0
               : 1;
}
