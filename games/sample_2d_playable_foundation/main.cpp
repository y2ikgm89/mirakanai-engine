// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/audio/audio_mixer.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/session_services.hpp"
#include "mirakana/scene/playable_2d.hpp"
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/ui/ui.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

class Sample2DPlayableFoundationGame final : public mirakana::GameApp {
  public:
    Sample2DPlayableFoundationGame(mirakana::VirtualInput& input, mirakana::NullRenderer& renderer)
        : input_(input), renderer_(renderer) {}

    void on_start(mirakana::EngineContext& context) override {
        context.logger.write(mirakana::LogRecord{
            .level = mirakana::LogLevel::info, .category = "sample", .message = "2d playable foundation started"});

        actions_.bind_key_axis("move_x", mirakana::Key::left, mirakana::Key::right);
        actions_.bind_key("jump", mirakana::Key::space);
        input_.press(mirakana::Key::right);
        input_.press(mirakana::Key::space);

        player_sprite_ = asset_id_from_game_asset_key("sprites/player");
        player_material_ = asset_id_from_game_asset_key("materials/player_sprite");
        jump_cue_ = asset_id_from_game_asset_key("audio/jump");

        scene_ = mirakana::Scene("Sample 2D Playable Foundation");
        const auto camera = scene_.create_node("Main Camera");
        player_ = scene_.create_node("Player");

        mirakana::SceneNodeComponents camera_components;
        camera_components.camera = mirakana::CameraComponent{
            .projection = mirakana::CameraProjectionMode::orthographic,
            .vertical_fov_radians = 1.04719758F,
            .orthographic_height = 12.0F,
            .near_plane = 0.1F,
            .far_plane = 100.0F,
            .primary = true,
        };
        scene_.set_components(camera, camera_components);

        mirakana::SceneNodeComponents player_components;
        player_components.sprite_renderer = mirakana::SpriteRendererComponent{
            .sprite = player_sprite_,
            .material = player_material_,
            .size = mirakana::Vec2{.x = 1.5F, .y = 2.0F},
            .tint = {0.2F, 0.7F, 1.0F, 1.0F},
            .visible = true,
        };
        scene_.set_components(player_, player_components);

        ui_ok_ = build_hud();
        theme_.add(mirakana::UiThemeColor{.token = "hud.panel",
                                          .color = mirakana::Color{.r = 0.06F, .g = 0.08F, .b = 0.09F, .a = 1.0F}});

        audio_clip_registered_ =
            mixer_.register_clip(mirakana::AudioClipDesc{.clip = jump_cue_,
                                                         .sample_rate = 48000,
                                                         .channel_count = 1,
                                                         .frame_count = 4,
                                                         .sample_format = mirakana::AudioSampleFormat::float32,
                                                         .streaming = false,
                                                         .buffered_frame_count = 4});
        audio_samples_.push_back(mirakana::AudioClipSampleData{
            .clip = jump_cue_,
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 4,
            .interleaved_float_samples = {0.25F, 0.5F, 0.25F, 0.0F},
        });

        renderer_.set_clear_color(mirakana::Color{.r = 0.02F, .g = 0.03F, .b = 0.04F, .a = 1.0F});
    }

    bool on_update(mirakana::EngineContext& /*context*/, double /*delta_seconds*/) override {
        const mirakana::runtime::RuntimeInputStateView input_state{
            .keyboard = &input_,
            .pointer = nullptr,
            .gamepad = nullptr,
        };

        auto* player = scene_.find_node(player_);
        if (player == nullptr) {
            return false;
        }

        player->transform.position.x += actions_.axis_value("move_x", input_state);
        if (actions_.action_pressed("jump", input_state)) {
            jump_voice_ = mixer_.play(
                mirakana::AudioVoiceDesc{.clip = jump_cue_, .bus = "master", .gain = 1.0F, .looping = false});
        }

        const auto validation = mirakana::validate_playable_2d_scene(scene_);
        validation_ok_ = validation_ok_ && validation.succeeded();

        renderer_.begin_frame();
        const auto packet = mirakana::build_scene_render_packet(scene_);
        const auto scene_submit = mirakana::submit_scene_render_packet(renderer_, packet);
        scene_sprites_submitted_ += scene_submit.sprites_submitted;
        primary_camera_seen_ = primary_camera_seen_ || scene_submit.has_primary_camera;

        update_hud_text();
        const auto layout =
            mirakana::ui::solve_layout(hud_, mirakana::ui::ElementId{"hud.root"},
                                       mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 320.0F, .height = 180.0F});
        const auto submission = mirakana::ui::build_renderer_submission(hud_, layout);
        const auto hud_submit = mirakana::submit_ui_renderer_submission(renderer_, submission,
                                                                        mirakana::UiRenderSubmitDesc{.theme = &theme_});
        hud_boxes_submitted_ += hud_submit.boxes_submitted;
        renderer_.end_frame();

        const auto audio = mixer_.render_interleaved_float(
            mirakana::AudioRenderRequest{
                .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                      .channel_count = 1,
                                                      .sample_format = mirakana::AudioSampleFormat::float32},
                .frame_count = 4,
                .device_frame = static_cast<std::uint64_t>(frames_) * 4U,
                .underrun_warning_threshold_frames = 4,
            },
            audio_samples_);
        audio_commands_ += audio.plan.commands.size();
        audio_underruns_ += audio.plan.underruns.size();

        ++frames_;
        input_.begin_frame();
        return frames_ < 3;
    }

    void on_stop(mirakana::EngineContext& /*context*/) override {
        if (const auto* player = scene_.find_node(player_); player != nullptr) {
            final_x_ = player->transform.position.x;
        }
    }

    [[nodiscard]] bool passed() const noexcept {
        const auto stats = renderer_.stats();
        return ui_ok_ && ui_text_updates_ok_ && validation_ok_ && audio_clip_registered_ &&
               jump_voice_ != mirakana::null_audio_voice && frames_ == 3 && final_x_ == 3.0F && primary_camera_seen_ &&
               scene_sprites_submitted_ == 3 && hud_boxes_submitted_ == 3 && audio_commands_ == 1 &&
               audio_underruns_ == 0 && stats.frames_started == 3 && stats.frames_finished == 3 &&
               stats.sprites_submitted == 6;
    }

    [[nodiscard]] int frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] float final_x() const noexcept {
        return final_x_;
    }

  private:
    [[nodiscard]] bool build_hud() {
        mirakana::ui::ElementDesc root;
        root.id = mirakana::ui::ElementId{"hud.root"};
        root.role = mirakana::ui::SemanticRole::root;
        root.style.layout = mirakana::ui::LayoutMode::column;
        root.style.padding = mirakana::ui::EdgeInsets{.top = 8.0F, .right = 8.0F, .bottom = 8.0F, .left = 8.0F};
        if (!hud_.try_add_element(root)) {
            return false;
        }

        mirakana::ui::ElementDesc score;
        score.id = mirakana::ui::ElementId{"hud.score"};
        score.parent = root.id;
        score.role = mirakana::ui::SemanticRole::label;
        score.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 96.0F, .height = 24.0F};
        score.text = mirakana::ui::TextContent{
            .label = "Score 0", .localization_key = "hud.score", .font_family = "engine-default"};
        score.style.background_token = "hud.panel";
        score.accessibility_label = "Score";
        return hud_.try_add_element(score);
    }

    void update_hud_text() {
        const auto score = std::string{"Score "} + std::to_string((frames_ + 1) * 100);
        ui_text_updates_ok_ =
            ui_text_updates_ok_ &&
            hud_.set_text(mirakana::ui::ElementId{"hud.score"},
                          mirakana::ui::TextContent{
                              .label = score, .localization_key = "hud.score", .font_family = "engine-default"});
    }

    mirakana::VirtualInput& input_;
    mirakana::NullRenderer& renderer_;
    mirakana::runtime::RuntimeInputActionMap actions_;
    mirakana::Scene scene_{"Sample 2D Playable Foundation"};
    mirakana::SceneNodeId player_;
    mirakana::ui::UiDocument hud_;
    mirakana::UiRendererTheme theme_;
    mirakana::AudioMixer mixer_;
    std::vector<mirakana::AudioClipSampleData> audio_samples_;
    mirakana::AssetId player_sprite_;
    mirakana::AssetId player_material_;
    mirakana::AssetId jump_cue_;
    mirakana::AudioVoiceId jump_voice_;
    std::size_t scene_sprites_submitted_{0};
    std::size_t hud_boxes_submitted_{0};
    std::size_t audio_commands_{0};
    std::size_t audio_underruns_{0};
    int frames_{0};
    float final_x_{0.0F};
    bool ui_ok_{false};
    bool ui_text_updates_ok_{true};
    bool validation_ok_{true};
    bool audio_clip_registered_{false};
    bool primary_camera_seen_{false};
};

} // namespace

int main() {
    mirakana::RingBufferLogger logger(16);
    mirakana::Registry registry;
    mirakana::VirtualInput input;
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    mirakana::HeadlessRunner runner(logger, registry);
    Sample2DPlayableFoundationGame game(input, renderer);

    const auto result = runner.run(game, mirakana::RunConfig{.max_frames = 8, .fixed_delta_seconds = 1.0 / 60.0});
    std::cout << "sample_2d_playable_foundation frames=" << result.frames_run << " final_x=" << game.final_x() << '\n';

    return result.status == mirakana::RunStatus::stopped_by_app && result.frames_run == 3 && game.passed() ? 0 : 1;
}
