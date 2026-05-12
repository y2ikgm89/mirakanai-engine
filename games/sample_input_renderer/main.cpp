// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/renderer/renderer.hpp"

#include <iostream>

namespace {

struct Player {
    mirakana::Transform2D transform;
};

class SampleInputRendererGame final : public mirakana::GameApp {
  public:
    SampleInputRendererGame(mirakana::VirtualInput& input, mirakana::NullRenderer& renderer)
        : input_(input), renderer_(renderer) {}

    void on_start(mirakana::EngineContext& context) override {
        player_ = context.registry.create();
        context.registry.emplace<Player>(player_, mirakana::Transform2D{});
        input_.press(mirakana::Key::right);
        renderer_.set_clear_color(mirakana::Color{.r = 0.05F, .g = 0.06F, .b = 0.07F, .a = 1.0F});
    }

    bool on_update(mirakana::EngineContext& context, double /*delta_seconds*/) override {
        renderer_.begin_frame();

        auto* player = context.registry.try_get<Player>(player_);
        if (player == nullptr) {
            return false;
        }

        const auto axis =
            input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
        player->transform.position = player->transform.position + axis;
        renderer_.draw_sprite(mirakana::SpriteCommand{
            .transform = player->transform, .color = mirakana::Color{.r = 0.2F, .g = 0.7F, .b = 1.0F, .a = 1.0F}});
        ++frames_;

        renderer_.end_frame();
        input_.begin_frame();

        return frames_ < 4;
    }

    void on_stop(mirakana::EngineContext& context) override {
        const auto* player = context.registry.try_get<Player>(player_);
        if (player != nullptr) {
            final_x_ = player->transform.position.x;
        }
        context.registry.destroy(player_);
    }

    [[nodiscard]] int frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] float final_x() const noexcept {
        return final_x_;
    }

  private:
    mirakana::VirtualInput& input_;
    mirakana::NullRenderer& renderer_;
    mirakana::Entity player_{};
    int frames_{0};
    float final_x_{0.0F};
};

} // namespace

int main() {
    mirakana::RingBufferLogger logger(16);
    mirakana::Registry registry;
    mirakana::VirtualInput input;
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    mirakana::HeadlessRunner runner(logger, registry);
    SampleInputRendererGame game(input, renderer);

    const auto result = runner.run(game, mirakana::RunConfig{.max_frames = 8, .fixed_delta_seconds = 1.0 / 60.0});
    const auto stats = renderer.stats();

    std::cout << "sample_input_renderer frames=" << result.frames_run << " final_x=" << game.final_x() << '\n';

    return result.status == mirakana::RunStatus::stopped_by_app && result.frames_run == 4 && game.frames() == 4 &&
                   game.final_x() == 4.0F && stats.frames_started == 4 && stats.frames_finished == 4 &&
                   stats.sprites_submitted == 4
               ? 0
               : 1;
}
