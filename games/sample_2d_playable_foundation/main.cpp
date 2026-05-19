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
#include "mirakana/runtime_scene/runtime_scene.hpp"
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

[[nodiscard]] constexpr std::string_view
runtime_scene_gameplay_session_state_name(mirakana::runtime_scene::RuntimeSceneGameplaySessionState state) noexcept {
    switch (state) {
    case mirakana::runtime_scene::RuntimeSceneGameplaySessionState::running:
        return "running";
    case mirakana::runtime_scene::RuntimeSceneGameplaySessionState::won:
        return "won";
    case mirakana::runtime_scene::RuntimeSceneGameplaySessionState::lost:
        return "lost";
    }
    return "unknown";
}

class Sample2DPlayableFoundationGame final : public mirakana::GameApp {
  public:
    Sample2DPlayableFoundationGame(mirakana::VirtualInput& input, mirakana::NullRenderer& renderer)
        : input_(input), renderer_(renderer) {}

    void on_start(mirakana::EngineContext& context) override {
        context.logger.write(mirakana::LogRecord{
            .level = mirakana::LogLevel::info, .category = "sample", .message = "2d playable foundation started"});

        actions_.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);
        actions_.bind_key_in_context("gameplay", "jump", mirakana::Key::space);
        actions_.bind_key_in_context("hud", "toggle_debug", mirakana::Key::escape);
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

        build_gameplay_interaction_plan(camera);

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

        input_context_plan_ =
            mirakana::runtime::plan_runtime_input_context_stack(mirakana::runtime::RuntimeInputContextStackRequest{
                .layers =
                    {
                        mirakana::runtime::RuntimeInputContextLayerDesc{
                            .context = "hud",
                            .kind = mirakana::runtime::RuntimeInputContextLayerKind::overlay,
                            .active = true,
                            .blocks_lower_priority = false,
                            .consumes_gameplay_input = false,
                        },
                        mirakana::runtime::RuntimeInputContextLayerDesc{
                            .context = "gameplay",
                            .kind = mirakana::runtime::RuntimeInputContextLayerKind::gameplay,
                            .active = true,
                            .blocks_lower_priority = false,
                            .consumes_gameplay_input = false,
                        },
                    },
            });
        input_context_plan_ok_ = input_context_plan_ok_ && input_context_plan_.succeeded();
        input_contexts_planned_ += input_context_plan_.stack.active_contexts.size();
        gameplay_input_available_seen_ = gameplay_input_available_seen_ || input_context_plan_.gameplay_input_available;
        hud_overlay_seen_ = hud_overlay_seen_ || input_context_plan_.ui_context_active;

        player->transform.position.x += actions_.axis_value("move_x", input_state, input_context_plan_.stack);
        if (actions_.action_pressed("jump", input_state, input_context_plan_.stack)) {
            trigger_jump_audio();
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

        plan_debug_overlay();

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
               stats.sprites_submitted == 6 && input_context_plan_ok_ && input_contexts_planned_ == 6U &&
               gameplay_input_available_seen_ && hud_overlay_seen_ && gameplay_audio_plan_ok_ &&
               gameplay_audio_buses_planned_ == 1U && gameplay_audio_commands_planned_ == 1U &&
               debug_overlay_plan_ok_ && debug_overlay_rows_planned_ == 12U && gameplay_interaction_plan_.succeeded() &&
               gameplay_interaction_plan_.rows.size() == 3U &&
               gameplay_interaction_plan_.final_session_state ==
                   mirakana::runtime_scene::RuntimeSceneGameplaySessionState::won;
    }

    [[nodiscard]] int frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] float final_x() const noexcept {
        return final_x_;
    }

    [[nodiscard]] std::size_t gameplay_interaction_count() const noexcept {
        return gameplay_interaction_plan_.rows.size();
    }

    [[nodiscard]] std::size_t input_context_count() const noexcept {
        return input_contexts_planned_;
    }

    [[nodiscard]] std::size_t gameplay_audio_count() const noexcept {
        return gameplay_audio_commands_planned_;
    }

    [[nodiscard]] std::size_t debug_overlay_row_count() const noexcept {
        return debug_overlay_rows_planned_;
    }

  private:
    void trigger_jump_audio() {
        gameplay_audio_plan_ = mirakana::plan_gameplay_audio_mix(mirakana::AudioGameplayMixRequest{
            .buses =
                {
                    mirakana::AudioGameplayBusMixDesc{.name = "sfx", .gain = 1.0F},
                },
            .cues =
                {
                    mirakana::AudioGameplayCueDesc{
                        .id = "jump",
                        .kind = mirakana::AudioGameplayCueKind::sfx,
                        .clip = jump_cue_,
                        .bus = "sfx",
                        .gain = 1.0F,
                    },
                },
            .triggers =
                {
                    mirakana::AudioGameplayCueTrigger{
                        .cue_id = "jump", .start_frame = static_cast<std::uint64_t>(frames_) * 4U, .gain_scale = 1.0F},
                },
        });
        gameplay_audio_plan_ok_ = gameplay_audio_plan_ok_ && gameplay_audio_plan_.succeeded();
        if (!gameplay_audio_plan_.succeeded()) {
            return;
        }

        for (const auto& bus : gameplay_audio_plan_.buses) {
            if (bus.name == "master") {
                mixer_.set_bus_gain(bus.name, bus.gain);
                mixer_.set_bus_muted(bus.name, bus.muted);
            } else if (!mixer_.try_add_bus(bus)) {
                gameplay_audio_plan_ok_ = false;
                return;
            }
            ++gameplay_audio_buses_planned_;
        }
        for (const auto& command : gameplay_audio_plan_.commands) {
            jump_voice_ = mixer_.play(command.voice);
            ++gameplay_audio_commands_planned_;
        }
    }

    void build_gameplay_interaction_plan(mirakana::SceneNodeId camera) {
        const std::vector<mirakana::runtime_scene::RuntimeSceneGameplayBindingRow> bindings{
            {
                .binding_id = "player.actor",
                .gameplay_system_id = "sample_2d",
                .slot_id = "actor",
                .node_name = "Player",
                .node = player_,
                .required_component =
                    mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::sprite_renderer,
            },
            {
                .binding_id = "camera.primary",
                .gameplay_system_id = "sample_2d",
                .slot_id = "trigger_target",
                .node_name = "Main Camera",
                .node = camera,
                .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::camera,
            },
        };
        const std::vector<mirakana::runtime_scene::RuntimeSceneGameplayInteractionSourceRow> interactions{
            {
                .action_id = "movement.camera_trigger",
                .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::trigger,
                .source_binding_id = "player.actor",
                .target_binding_id = "camera.primary",
            },
            {
                .action_id = "score.progress",
                .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::objective_progress,
                .source_binding_id = "player.actor",
                .objective_id = "score",
                .amount = 100,
            },
            {
                .action_id = "level.win",
                .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::win,
                .source_binding_id = "player.actor",
            },
        };
        gameplay_interaction_plan_ = mirakana::runtime_scene::plan_runtime_scene_gameplay_interactions(
            bindings, interactions,
            mirakana::runtime_scene::RuntimeSceneGameplayInteractionPlanRequest{
                .session_state = mirakana::runtime_scene::RuntimeSceneGameplaySessionState::running,
            });
    }

    void plan_debug_overlay() {
        debug_overlay_plan_ = mirakana::ui::plan_runtime_gameplay_debug_overlay({
            mirakana::ui::RuntimeGameplayDebugOverlayRowDesc{
                .id = "gameplay.interactions",
                .category = mirakana::ui::RuntimeGameplayDebugOverlayCategory::gameplay,
                .kind = mirakana::ui::RuntimeGameplayDebugOverlayRowKind::counter,
                .label = "Gameplay interactions",
                .value = std::to_string(gameplay_interaction_plan_.rows.size()),
            },
            mirakana::ui::RuntimeGameplayDebugOverlayRowDesc{
                .id = "input.contexts",
                .category = mirakana::ui::RuntimeGameplayDebugOverlayCategory::input,
                .kind = mirakana::ui::RuntimeGameplayDebugOverlayRowKind::counter,
                .label = "Input contexts",
                .value = std::to_string(input_context_plan_.stack.active_contexts.size()),
            },
            mirakana::ui::RuntimeGameplayDebugOverlayRowDesc{
                .id = "audio.cues",
                .category = mirakana::ui::RuntimeGameplayDebugOverlayCategory::audio,
                .kind = mirakana::ui::RuntimeGameplayDebugOverlayRowKind::counter,
                .label = "Audio cues",
                .value = std::to_string(gameplay_audio_commands_planned_),
            },
            mirakana::ui::RuntimeGameplayDebugOverlayRowDesc{
                .id = "session.state",
                .category = mirakana::ui::RuntimeGameplayDebugOverlayCategory::session,
                .kind = mirakana::ui::RuntimeGameplayDebugOverlayRowKind::status,
                .label = "Session",
                .value = std::string{runtime_scene_gameplay_session_state_name(
                    gameplay_interaction_plan_.final_session_state)},
            },
        });
        debug_overlay_plan_ok_ = debug_overlay_plan_ok_ && debug_overlay_plan_.succeeded();
        if (debug_overlay_plan_.succeeded()) {
            debug_overlay_rows_planned_ += debug_overlay_plan_.rows.size();
        }
    }

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
    mirakana::runtime::RuntimeInputContextStackPlan input_context_plan_;
    mirakana::Scene scene_{"Sample 2D Playable Foundation"};
    mirakana::SceneNodeId player_;
    mirakana::runtime_scene::RuntimeSceneGameplayInteractionPlan gameplay_interaction_plan_;
    mirakana::ui::RuntimeGameplayDebugOverlayPlan debug_overlay_plan_;
    mirakana::ui::UiDocument hud_;
    mirakana::UiRendererTheme theme_;
    mirakana::AudioMixer mixer_;
    mirakana::AudioGameplayMixPlan gameplay_audio_plan_;
    std::vector<mirakana::AudioClipSampleData> audio_samples_;
    mirakana::AssetId player_sprite_;
    mirakana::AssetId player_material_;
    mirakana::AssetId jump_cue_;
    mirakana::AudioVoiceId jump_voice_;
    std::size_t scene_sprites_submitted_{0};
    std::size_t hud_boxes_submitted_{0};
    std::size_t input_contexts_planned_{0};
    std::size_t gameplay_audio_buses_planned_{0};
    std::size_t gameplay_audio_commands_planned_{0};
    std::size_t debug_overlay_rows_planned_{0};
    std::size_t audio_commands_{0};
    std::size_t audio_underruns_{0};
    int frames_{0};
    float final_x_{0.0F};
    bool ui_ok_{false};
    bool ui_text_updates_ok_{true};
    bool validation_ok_{true};
    bool audio_clip_registered_{false};
    bool gameplay_audio_plan_ok_{true};
    bool primary_camera_seen_{false};
    bool input_context_plan_ok_{true};
    bool gameplay_input_available_seen_{false};
    bool hud_overlay_seen_{false};
    bool debug_overlay_plan_ok_{true};
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
    std::cout << "sample_2d_playable_foundation frames=" << result.frames_run << " final_x=" << game.final_x()
              << " gameplay_interactions=" << game.gameplay_interaction_count()
              << " input_contexts=" << game.input_context_count() << " audio_cues=" << game.gameplay_audio_count()
              << " debug_overlay_rows=" << game.debug_overlay_row_count() << '\n';

    return result.status == mirakana::RunStatus::stopped_by_app && result.frames_run == 3 && game.passed() ? 0 : 1;
}
