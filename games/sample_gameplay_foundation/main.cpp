// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ai/behavior_tree.hpp"
#include "mirakana/ai/perception.hpp"
#include "mirakana/animation/state_machine.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/audio/audio_mixer.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"
#include "mirakana/navigation/navigation_agent.hpp"
#include "mirakana/navigation/navigation_grid.hpp"
#include "mirakana/navigation/navigation_path_planner.hpp"
#include "mirakana/physics/physics3d.hpp"

#include <cmath>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

constexpr mirakana::BehaviorTreeNodeId k_root_node{1};
constexpr mirakana::BehaviorTreeNodeId k_has_target_node{2};
constexpr mirakana::BehaviorTreeNodeId k_needs_move_node{3};
constexpr mirakana::BehaviorTreeNodeId k_move_action_node{4};
constexpr const char* k_has_target_key{"composition.has_target"};
constexpr const char* k_needs_move_key{"composition.needs_move"};
constexpr const char* k_target_id_key{"composition.target_id"};
constexpr const char* k_target_distance_key{"composition.target_distance"};
constexpr const char* k_visible_targets_key{"composition.visible_targets"};
constexpr const char* k_audible_targets_key{"composition.audible_targets"};
constexpr const char* k_target_state_key{"composition.target_state"};

struct ActorTag {};

class SampleGameplayFoundationGame final : public mirakana::GameApp {
  public:
    SampleGameplayFoundationGame()
        : physics_(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}}),
          animation_(mirakana::AnimationStateMachineDesc{
              .states =
                  {
                      mirakana::AnimationStateDesc{
                          .name = "idle",
                          .clip =
                              mirakana::AnimationClipDesc{.name = "idle", .duration_seconds = 1.0F, .looping = true}},
                      mirakana::AnimationStateDesc{
                          .name = "walk",
                          .clip =
                              mirakana::AnimationClipDesc{.name = "walk", .duration_seconds = 1.0F, .looping = true}},
                  },
              .initial_state = "idle",
              .transitions =
                  {
                      mirakana::AnimationTransitionDesc{
                          .from_state = "idle", .to_state = "walk", .trigger = "move", .blend_seconds = 0.5F},
                  },
          }) {}

    void on_start(mirakana::EngineContext& context) override {
        actor_entity_ = context.registry.create();
        context.registry.emplace<ActorTag>(actor_entity_);

        floor_body_ = physics_.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 5.0F, .y = 1.0F, .z = 5.0F},
        });
        actor_body_ = physics_.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 1.5F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.5F, .y = 1.0F, .z = 0.5F},
        });

        build_authored_collision_probe();
        build_navigation_agent();
        render_audio_stream_probe();

        (void)animation_.trigger("move");
        context.logger.write(mirakana::LogRecord{
            .level = mirakana::LogLevel::info, .category = "sample", .message = "gameplay foundation started"});
    }

    bool on_update(mirakana::EngineContext& /*context*/, double /*delta_seconds*/) override {
        physics_.apply_force(actor_body_, mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F});
        physics_.step(0.25F);
        physics_.resolve_contacts();
        animation_.update(0.25F);
        update_ai_navigation_composition();
        ++frames_;
        return frames_ < 4;
    }

    void on_stop(mirakana::EngineContext& context) override {
        const auto* actor = physics_.find_body(actor_body_);
        if (actor != nullptr) {
            final_actor_position_ = actor->position;
            final_actor_velocity_ = actor->velocity;
        }
        final_animation_sample_ = animation_.sample();
        context.registry.destroy(actor_entity_);
        context.logger.write(mirakana::LogRecord{
            .level = mirakana::LogLevel::info, .category = "sample", .message = "gameplay foundation stopped"});
    }

    [[nodiscard]] int frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] mirakana::Vec3 final_actor_position() const noexcept {
        return final_actor_position_;
    }

    [[nodiscard]] mirakana::Vec3 final_actor_velocity() const noexcept {
        return final_actor_velocity_;
    }

    [[nodiscard]] const mirakana::AnimationStateMachineSample& final_animation_sample() const noexcept {
        return final_animation_sample_;
    }

    [[nodiscard]] bool initialized() const noexcept {
        return floor_body_ != mirakana::null_physics_body_3d && actor_body_ != mirakana::null_physics_body_3d;
    }

    [[nodiscard]] std::size_t authored_collision_body_count() const noexcept {
        return authored_collision_.bodies.size();
    }

    [[nodiscard]] bool controller_grounded() const noexcept {
        return controller_result_.grounded;
    }

    [[nodiscard]] mirakana::NavigationAgentState navigation_agent() const {
        return navigation_agent_;
    }

    [[nodiscard]] std::size_t navigation_path_point_count() const noexcept {
        return navigation_path_point_count_;
    }

    [[nodiscard]] mirakana::BehaviorTreeTickResult last_tree_result() const {
        return last_tree_result_;
    }

    [[nodiscard]] std::size_t perception_target_count() const noexcept {
        return last_perception_target_count_;
    }

    [[nodiscard]] std::uint32_t audio_stream_frames() const noexcept {
        return audio_stream_frames_;
    }

    [[nodiscard]] std::size_t audio_stream_sample_count() const noexcept {
        return audio_stream_sample_count_;
    }

  private:
    void build_authored_collision_probe() {
        mirakana::PhysicsAuthoredCollisionScene3DDesc scene;
        scene.world_config = mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
        scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
            .name = "floor",
            .body =
                mirakana::PhysicsBody3DDesc{
                    .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                    .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                    .mass = 0.0F,
                    .linear_damping = 0.0F,
                    .dynamic = false,
                    .half_extents = mirakana::Vec3{.x = 5.0F, .y = 0.5F, .z = 5.0F},
                },
        });
        scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
            .name = "wall",
            .body =
                mirakana::PhysicsBody3DDesc{
                    .position = mirakana::Vec3{.x = 3.0F, .y = 1.0F, .z = 0.0F},
                    .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                    .mass = 0.0F,
                    .linear_damping = 0.0F,
                    .dynamic = false,
                    .half_extents = mirakana::Vec3{.x = 0.5F, .y = 1.0F, .z = 2.0F},
                },
        });

        authored_collision_ = mirakana::build_physics_world_3d_from_authored_collision_scene(scene);
        if (authored_collision_.status != mirakana::PhysicsAuthoredCollision3DBuildStatus::success) {
            return;
        }

        mirakana::PhysicsCharacterController3DDesc request;
        request.position = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F};
        request.displacement = mirakana::Vec3{.x = 0.0F, .y = -3.0F, .z = 0.0F};
        request.radius = 0.5F;
        request.half_height = 0.5F;
        request.skin_width = 0.05F;
        controller_result_ = mirakana::move_physics_character_controller_3d(authored_collision_.world, request);
    }

    void build_navigation_agent() {
        mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 4, .height = 4});
        grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 0}, false);
        grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 1}, false);
        grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 2}, false);

        const auto plan =
            mirakana::plan_navigation_grid_agent_path(grid, mirakana::NavigationGridAgentPathRequest{
                                                                .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                                .goal = mirakana::NavigationGridCoord{.x = 3, .y = 3},
                                                                .mapping =
                                                                    mirakana::NavigationGridPointMapping{
                                                                        .origin = mirakana::NavigationPoint2{},
                                                                        .cell_size = 1.0F,
                                                                        .use_cell_centers = true,
                                                                    },
                                                                .adjacency = mirakana::NavigationAdjacency::cardinal4,
                                                                .smooth_path = true,
                                                            });
        navigation_plan_status_ = plan.status;
        navigation_path_point_count_ = plan.planned_grid_point_count;
        navigation_agent_ = plan.agent_state;
    }

    void render_audio_stream_probe() {
        mirakana::AudioMixer mixer;
        const auto clip = asset_id_from_game_asset_key("audio/gameplay-foundation-stream");
        mixer.add_clip(mirakana::AudioClipDesc{.clip = clip,
                                               .sample_rate = 48000,
                                               .channel_count = 1,
                                               .frame_count = 4,
                                               .sample_format = mirakana::AudioSampleFormat::float32,
                                               .streaming = false,
                                               .buffered_frame_count = 4});
        (void)mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "master", .gain = 1.0F, .looping = false});

        const std::vector<mirakana::AudioClipSampleData> samples{
            mirakana::AudioClipSampleData{
                .clip = clip,
                .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                      .channel_count = 1,
                                                      .sample_format = mirakana::AudioSampleFormat::float32},
                .frame_count = 4,
                .interleaved_float_samples = {0.1F, 0.2F, 0.3F, 0.4F},
            },
        };
        const auto output = mirakana::render_audio_device_stream_interleaved_float(
            mixer,
            mirakana::AudioDeviceStreamRequest{
                .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                      .channel_count = 1,
                                                      .sample_format = mirakana::AudioSampleFormat::float32},
                .device_frame = 0,
                .queued_frames = 2,
                .target_queued_frames = 4,
                .max_render_frames = 2,
            },
            samples);
        audio_stream_status_ = output.plan.status;
        audio_stream_frames_ = output.buffer.frame_count;
        audio_stream_sample_count_ = output.buffer.interleaved_float_samples.size();
    }

    void update_ai_navigation_composition() {
        if (navigation_agent_.status != mirakana::NavigationAgentStatus::moving ||
            navigation_plan_status_ != mirakana::NavigationGridAgentPathStatus::ready ||
            navigation_agent_.path.empty()) {
            return;
        }

        const std::vector<mirakana::AiPerceptionTarget2D> route_targets{
            mirakana::AiPerceptionTarget2D{
                .id = 1U,
                .position = mirakana::AiPerceptionPoint2{.x = navigation_agent_.path.back().x,
                                                         .y = navigation_agent_.path.back().y},
                .radius = 0.0F,
                .sight_enabled = true,
                .hearing_enabled = false,
                .sound_radius = 0.0F,
            },
        };
        const auto perception = mirakana::build_ai_perception_snapshot_2d(mirakana::AiPerceptionRequest2D{
            .agent = mirakana::AiPerceptionAgent2D{.id = 100U,
                                                   .position =
                                                       mirakana::AiPerceptionPoint2{.x = navigation_agent_.position.x,
                                                                                    .y = navigation_agent_.position.y},
                                                   .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                                   .sight_range = 16.0F,
                                                   .field_of_view_radians = 6.28318530718F,
                                                   .hearing_range = 0.0F},
            .targets = std::span<const mirakana::AiPerceptionTarget2D>{route_targets},
        });
        last_perception_target_count_ = perception.targets.size();

        mirakana::BehaviorTreeBlackboard blackboard;
        const auto blackboard_result = mirakana::write_ai_perception_blackboard(
            perception,
            mirakana::AiPerceptionBlackboardKeys{.has_target_key = k_has_target_key,
                                                 .target_id_key = k_target_id_key,
                                                 .target_distance_key = k_target_distance_key,
                                                 .visible_count_key = k_visible_targets_key,
                                                 .audible_count_key = k_audible_targets_key,
                                                 .target_state_key = k_target_state_key},
            blackboard);
        if (blackboard_result.status != mirakana::AiPerceptionBlackboardStatus::ready ||
            !blackboard.set(k_needs_move_key, mirakana::make_behavior_tree_blackboard_bool(true))) {
            return;
        }

        const std::vector<mirakana::BehaviorTreeBlackboardCondition> conditions{
            mirakana::BehaviorTreeBlackboardCondition{.node_id = k_has_target_node,
                                                      .key = k_has_target_key,
                                                      .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                      .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
            mirakana::BehaviorTreeBlackboardCondition{.node_id = k_needs_move_node,
                                                      .key = k_needs_move_key,
                                                      .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                      .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
        };
        const auto supporting_systems_ready =
            authored_collision_.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::success &&
            controller_result_.grounded && audio_stream_status_ == mirakana::AudioDeviceStreamStatus::ready;
        const std::vector<mirakana::BehaviorTreeLeafResult> leaf_results{
            mirakana::BehaviorTreeLeafResult{.node_id = k_move_action_node,
                                             .status = supporting_systems_ready
                                                           ? mirakana::BehaviorTreeStatus::success
                                                           : mirakana::BehaviorTreeStatus::failure},
        };

        last_tree_result_ = mirakana::evaluate_behavior_tree(
            mirakana::BehaviorTreeDesc{
                .root_id = k_root_node,
                .nodes =
                    {
                        mirakana::BehaviorTreeNodeDesc{
                            .id = k_root_node,
                            .kind = mirakana::BehaviorTreeNodeKind::sequence,
                            .children = {k_has_target_node, k_needs_move_node, k_move_action_node}},
                        mirakana::BehaviorTreeNodeDesc{
                            .id = k_has_target_node, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                        mirakana::BehaviorTreeNodeDesc{
                            .id = k_needs_move_node, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                        mirakana::BehaviorTreeNodeDesc{
                            .id = k_move_action_node, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
                    },
            },
            mirakana::BehaviorTreeEvaluationContext{
                .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{leaf_results},
                .blackboard_entries = blackboard.entries(),
                .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{conditions},
            });

        if (last_tree_result_.status != mirakana::BehaviorTreeStatus::success) {
            return;
        }

        const auto update = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
            .state = navigation_agent_,
            .config =
                mirakana::NavigationAgentConfig{.max_speed = 8.0F, .slowing_radius = 1.0F, .arrival_radius = 0.001F},
            .delta_seconds = 1.0F,
        });
        navigation_agent_ = update.state;
    }

    mirakana::PhysicsWorld3D physics_;
    mirakana::AnimationStateMachine animation_;
    mirakana::PhysicsAuthoredCollisionScene3DBuildResult authored_collision_;
    mirakana::PhysicsCharacterController3DResult controller_result_;
    mirakana::NavigationAgentState navigation_agent_;
    mirakana::BehaviorTreeTickResult last_tree_result_;
    mirakana::Entity actor_entity_{};
    mirakana::PhysicsBody3DId floor_body_{};
    mirakana::PhysicsBody3DId actor_body_{};
    mirakana::Vec3 final_actor_position_{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    mirakana::Vec3 final_actor_velocity_{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    mirakana::AnimationStateMachineSample final_animation_sample_;
    mirakana::NavigationGridAgentPathStatus navigation_plan_status_{
        mirakana::NavigationGridAgentPathStatus::invalid_request};
    mirakana::AudioDeviceStreamStatus audio_stream_status_{mirakana::AudioDeviceStreamStatus::invalid_request};
    std::size_t navigation_path_point_count_{0U};
    std::size_t last_perception_target_count_{0U};
    std::uint32_t audio_stream_frames_{0U};
    std::size_t audio_stream_sample_count_{0U};
    int frames_{0};
};

} // namespace

int main() {
    mirakana::RingBufferLogger logger(16);
    mirakana::Registry registry;
    mirakana::HeadlessRunner runner(logger, registry);
    SampleGameplayFoundationGame game;

    const auto result = runner.run(game, mirakana::RunConfig{.max_frames = 8, .fixed_delta_seconds = 1.0 / 60.0});
    const auto position = game.final_actor_position();
    const auto velocity = game.final_actor_velocity();
    const auto animation = game.final_animation_sample();
    const auto navigation = game.navigation_agent();
    const auto behavior = game.last_tree_result();

    std::cout << "sample_gameplay_foundation frames=" << result.frames_run << " x=" << position.x << " y=" << position.y
              << " state=" << animation.to_state
              << " authored_collision_bodies=" << game.authored_collision_body_count()
              << " controller_grounded=" << game.controller_grounded()
              << " nav_points=" << game.navigation_path_point_count()
              << " behavior_nodes=" << behavior.visited_nodes.size() << " audio_frames=" << game.audio_stream_frames()
              << '\n';

    return result.status == mirakana::RunStatus::stopped_by_app && result.frames_run == 4 && game.frames() == 4 &&
                   game.initialized() && registry.living_count() == 0 && std::abs(position.x - 1.25F) < 0.0001F &&
                   std::abs(position.y - 2.0F) < 0.0001F && std::abs(velocity.y) < 0.0001F &&
                   animation.to_state == "walk" && !animation.blending && game.authored_collision_body_count() == 2U &&
                   game.controller_grounded() && game.navigation_path_point_count() == 3U &&
                   navigation.status == mirakana::NavigationAgentStatus::reached_destination &&
                   behavior.status == mirakana::BehaviorTreeStatus::success && behavior.visited_nodes.size() == 4U &&
                   game.perception_target_count() == 1U && game.audio_stream_frames() == 2U &&
                   game.audio_stream_sample_count() == 2U
               ? 0
               : 1;
}
