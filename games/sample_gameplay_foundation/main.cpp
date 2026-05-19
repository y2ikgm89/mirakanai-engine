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
#include "mirakana/navigation/local_avoidance.hpp"
#include "mirakana/navigation/navigation_agent.hpp"
#include "mirakana/navigation/navigation_grid.hpp"
#include "mirakana/navigation/navigation_navmesh.hpp"
#include "mirakana/navigation/navigation_path_planner.hpp"
#include "mirakana/physics/physics3d.hpp"
#include "mirakana/runtime_scene/runtime_scene.hpp"

#include <array>
#include <cmath>
#include <cstdint>
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
enum class GameplayRuntimeStep : std::uint8_t {
    physics_apply_force,
    physics_step,
    physics_resolve_contacts,
    animation_update,
    navigation_replan,
    navigation_ai_update,
    count,
};

constexpr std::size_t k_gameplay_runtime_step_count{static_cast<std::size_t>(GameplayRuntimeStep::count)};

[[nodiscard]] constexpr std::string_view gameplay_runtime_step_name(GameplayRuntimeStep step) noexcept {
    switch (step) {
    case GameplayRuntimeStep::physics_apply_force:
        return "physics.apply_force";
    case GameplayRuntimeStep::physics_step:
        return "physics.step";
    case GameplayRuntimeStep::physics_resolve_contacts:
        return "physics.resolve_contacts";
    case GameplayRuntimeStep::animation_update:
        return "animation.update";
    case GameplayRuntimeStep::navigation_replan:
        return "navigation.replan";
    case GameplayRuntimeStep::navigation_ai_update:
        return "navigation_and_ai.update";
    case GameplayRuntimeStep::count:
        return "unknown";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view
navigation_grid_replan_status_name(mirakana::NavigationGridReplanStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationGridReplanStatus::reused_existing_path:
        return "reused_existing_path";
    case mirakana::NavigationGridReplanStatus::replanned:
        return "replanned";
    case mirakana::NavigationGridReplanStatus::unsupported_adjacency:
        return "unsupported_adjacency";
    case mirakana::NavigationGridReplanStatus::invalid_endpoint:
        return "invalid_endpoint";
    case mirakana::NavigationGridReplanStatus::blocked_endpoint:
        return "blocked_endpoint";
    case mirakana::NavigationGridReplanStatus::no_path:
        return "no_path";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view
navigation_navmesh_path_status_name(mirakana::NavigationNavmeshPathStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationNavmeshPathStatus::success:
        return "success";
    case mirakana::NavigationNavmeshPathStatus::invalid_request:
        return "invalid_request";
    case mirakana::NavigationNavmeshPathStatus::invalid_navmesh:
        return "invalid_navmesh";
    case mirakana::NavigationNavmeshPathStatus::invalid_endpoint:
        return "invalid_endpoint";
    case mirakana::NavigationNavmeshPathStatus::blocked_endpoint:
        return "blocked_endpoint";
    case mirakana::NavigationNavmeshPathStatus::no_path:
        return "no_path";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view
navigation_navmesh_path_diagnostic_name(mirakana::NavigationNavmeshPathDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::NavigationNavmeshPathDiagnostic::none:
        return "none";
    case mirakana::NavigationNavmeshPathDiagnostic::empty_navmesh:
        return "empty_navmesh";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_polygon_id:
        return "invalid_polygon_id";
    case mirakana::NavigationNavmeshPathDiagnostic::duplicate_polygon_id:
        return "duplicate_polygon_id";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_scene_ref:
        return "invalid_scene_ref";
    case mirakana::NavigationNavmeshPathDiagnostic::duplicate_scene_ref:
        return "duplicate_scene_ref";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_polygon_center:
        return "invalid_polygon_center";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_traversal_cost:
        return "invalid_traversal_cost";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_portal_endpoint:
        return "invalid_portal_endpoint";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_portal_cost:
        return "invalid_portal_cost";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_obstacle:
        return "invalid_obstacle";
    case mirakana::NavigationNavmeshPathDiagnostic::duplicate_obstacle_id:
        return "duplicate_obstacle_id";
    case mirakana::NavigationNavmeshPathDiagnostic::blocked_start:
        return "blocked_start";
    case mirakana::NavigationNavmeshPathDiagnostic::blocked_goal:
        return "blocked_goal";
    case mirakana::NavigationNavmeshPathDiagnostic::cost_overflow:
        return "cost_overflow";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view
navigation_local_avoidance_status_name(mirakana::NavigationLocalAvoidanceStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationLocalAvoidanceStatus::success:
        return "success";
    case mirakana::NavigationLocalAvoidanceStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view
navigation_local_avoidance_diagnostic_name(mirakana::NavigationLocalAvoidanceDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::NavigationLocalAvoidanceDiagnostic::none:
        return "none";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_agent_id:
        return "invalid_agent_id";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_position:
        return "invalid_position";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_desired_velocity:
        return "invalid_desired_velocity";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_radius:
        return "invalid_radius";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_max_speed:
        return "invalid_max_speed";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_id:
        return "invalid_neighbor_id";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_position:
        return "invalid_neighbor_position";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_velocity:
        return "invalid_neighbor_velocity";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_radius:
        return "invalid_neighbor_radius";
    case mirakana::NavigationLocalAvoidanceDiagnostic::duplicate_neighbor_id:
        return "duplicate_neighbor_id";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_separation_weight:
        return "invalid_separation_weight";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_prediction_time_seconds:
        return "invalid_prediction_time_seconds";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_epsilon:
        return "invalid_epsilon";
    case mirakana::NavigationLocalAvoidanceDiagnostic::calculation_overflow:
        return "calculation_overflow";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view
physics_character_dynamic_policy_status_name(mirakana::PhysicsCharacterDynamicPolicy3DStatus status) noexcept {
    switch (status) {
    case mirakana::PhysicsCharacterDynamicPolicy3DStatus::moved:
        return "moved";
    case mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained:
        return "constrained";
    case mirakana::PhysicsCharacterDynamicPolicy3DStatus::stepped:
        return "stepped";
    case mirakana::PhysicsCharacterDynamicPolicy3DStatus::initial_overlap:
        return "initial_overlap";
    case mirakana::PhysicsCharacterDynamicPolicy3DStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view physics_character_dynamic_policy_diagnostic_name(
    mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::none:
        return "none";
    case mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::invalid_request:
        return "invalid_request";
    case mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::initial_overlap:
        return "initial_overlap";
    case mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::step_blocked:
        return "step_blocked";
    }
    return "unknown";
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

[[nodiscard]] constexpr mirakana::NavigationGridPointMapping navigation_mapping() noexcept {
    return mirakana::NavigationGridPointMapping{
        .origin = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .cell_size = 1.0F,
        .use_cell_centers = true,
    };
}

[[nodiscard]] constexpr std::size_t expected_navigation_goal_count() noexcept {
    return 2U;
}

[[nodiscard]] mirakana::NavigationGrid make_navigation_grid() {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 4, .height = 4});
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 1}, false);
    return grid;
}

[[nodiscard]] mirakana::NavigationGridCoord to_navigation_grid_coord(mirakana::NavigationPoint2 point) noexcept;

[[nodiscard]] std::vector<mirakana::NavigationGridCoord>
to_navigation_grid_path(std::span<const mirakana::NavigationPoint2> path) {
    std::vector<mirakana::NavigationGridCoord> grid_path;
    grid_path.reserve(path.size());
    for (const auto point : path) {
        grid_path.push_back(to_navigation_grid_coord(point));
    }
    return grid_path;
}

[[nodiscard]] mirakana::NavigationGridCoord to_navigation_grid_coord(mirakana::NavigationPoint2 point) noexcept {
    const auto mapping = navigation_mapping();
    const float inverse_offset = mapping.use_cell_centers ? 0.5F : 0.0F;
    return mirakana::NavigationGridCoord{
        .x = static_cast<int>(std::lround((point.x - mapping.origin.x) / mapping.cell_size - inverse_offset)),
        .y = static_cast<int>(std::lround((point.y - mapping.origin.y) / mapping.cell_size - inverse_offset)),
    };
}

[[nodiscard]] mirakana::NavigationPoint2 to_navigation_grid_cell_center(mirakana::NavigationGridCoord coord) noexcept {
    const auto mapping = navigation_mapping();
    return mirakana::NavigationPoint2{
        .x = mapping.origin.x +
             (static_cast<float>(coord.x) + (mapping.use_cell_centers ? 0.5F : 0.0F)) * mapping.cell_size,
        .y = mapping.origin.y +
             (static_cast<float>(coord.y) + (mapping.use_cell_centers ? 0.5F : 0.0F)) * mapping.cell_size,
    };
}

struct ActorTag {};

class SampleGameplayFoundationGame final : public mirakana::GameApp {
  public:
    SampleGameplayFoundationGame()
        : navigation_grid_(make_navigation_grid()),
          physics_(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}}),
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
        build_navigation_navmesh_probe();
        build_physics_movement_policy_probe();
        render_audio_stream_probe();
        build_runtime_scene_gameplay_interaction_probe();

        (void)animation_.trigger("move");
        context.logger.write(mirakana::LogRecord{
            .level = mirakana::LogLevel::info, .category = "sample", .message = "gameplay foundation started"});
    }

    bool on_update(mirakana::EngineContext& /*context*/, double /*delta_seconds*/) override {
        std::array<GameplayRuntimeStep, k_gameplay_runtime_step_count> step_trace{};
        std::size_t step_count{0U};

        const auto record_step = [&](const GameplayRuntimeStep step) {
            if (step_count < step_trace.size()) {
                step_trace[step_count] = step;
                ++step_count;
            }
        };

        record_step(GameplayRuntimeStep::physics_apply_force);
        physics_.apply_force(actor_body_, mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F});
        record_step(GameplayRuntimeStep::physics_step);
        physics_.step(0.25F);
        record_step(GameplayRuntimeStep::physics_resolve_contacts);
        physics_.resolve_contacts();
        record_step(GameplayRuntimeStep::animation_update);
        animation_.update(0.25F);
        record_step(GameplayRuntimeStep::navigation_replan);
        update_navigation_replan_layer();
        record_step(GameplayRuntimeStep::navigation_ai_update);
        animation_.update(0.25F);
        update_ai_navigation_composition();

        static constexpr std::array expected_steps{
            GameplayRuntimeStep::physics_apply_force,      GameplayRuntimeStep::physics_step,
            GameplayRuntimeStep::physics_resolve_contacts, GameplayRuntimeStep::animation_update,
            GameplayRuntimeStep::navigation_replan,        GameplayRuntimeStep::navigation_ai_update,
        };
        bool step_order_ok{true};
        if (step_count != expected_steps.size()) {
            step_order_ok = false;
        } else {
            for (std::size_t i = 0U; i < expected_steps.size(); ++i) {
                if (step_trace[i] != expected_steps[i]) {
                    step_order_ok = false;
                    break;
                }
            }
        }
        if (!step_order_ok) {
            ++tick_order_violations_;
        }

        last_tick_step_count_ = step_count;
        last_tick_steps_ = step_trace;

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

    [[nodiscard]] bool gameplay_tick_order_stable() const noexcept {
        return tick_order_violations_ == 0U;
    }

    [[nodiscard]] std::size_t gameplay_tick_order_violations() const noexcept {
        return tick_order_violations_;
    }

    [[nodiscard]] std::size_t last_tick_step_count() const noexcept {
        return last_tick_step_count_;
    }

    [[nodiscard]] std::size_t navigation_replan_attempt_count() const noexcept {
        return navigation_replan_attempt_count_;
    }

    [[nodiscard]] std::size_t navigation_replan_replanned_count() const noexcept {
        return navigation_replan_replanned_count_;
    }

    [[nodiscard]] std::size_t navigation_replan_reused_count() const noexcept {
        return navigation_replan_reused_count_;
    }

    [[nodiscard]] std::size_t local_avoidance_applied_neighbor_count() const noexcept {
        return local_avoidance_applied_neighbor_count_;
    }

    [[nodiscard]] std::size_t navigation_dynamic_obstacle_count() const noexcept {
        return navigation_dynamic_obstacle_count_;
    }

    [[nodiscard]] std::uint32_t navigation_replan_last_total_cost() const noexcept {
        return navigation_replan_last_total_cost_;
    }

    [[nodiscard]] std::size_t local_avoidance_step_count() const noexcept {
        return local_avoidance_step_count_;
    }

    [[nodiscard]] std::size_t navigation_replan_last_path_point_count() const noexcept {
        return navigation_replan_last_path_point_count_;
    }

    [[nodiscard]] mirakana::NavigationGridReplanStatus navigation_replan_last_status() const noexcept {
        return navigation_replan_last_status_;
    }

    [[nodiscard]] mirakana::NavigationLocalAvoidanceStatus local_avoidance_last_status() const noexcept {
        return local_avoidance_last_status_;
    }

    [[nodiscard]] mirakana::NavigationLocalAvoidanceDiagnostic local_avoidance_last_diagnostic() const noexcept {
        return local_avoidance_last_diagnostic_;
    }

    [[nodiscard]] mirakana::NavigationNavmeshPathStatus navigation_navmesh_status() const noexcept {
        return navigation_navmesh_plan_.status;
    }

    [[nodiscard]] mirakana::NavigationNavmeshPathDiagnostic navigation_navmesh_diagnostic() const noexcept {
        return navigation_navmesh_plan_.diagnostic;
    }

    [[nodiscard]] std::size_t navigation_navmesh_polygon_count() const noexcept {
        return navigation_navmesh_plan_.polygon_path.size();
    }

    [[nodiscard]] std::size_t navigation_navmesh_dynamic_obstacle_count() const noexcept {
        return navigation_navmesh_plan_.dynamic_obstacle_count;
    }

    [[nodiscard]] std::uint32_t navigation_navmesh_total_cost() const noexcept {
        return navigation_navmesh_plan_.total_cost;
    }

    [[nodiscard]] mirakana::PhysicsCharacterDynamicPolicy3DStatus physics_policy_status() const noexcept {
        return physics_policy_result_.status;
    }

    [[nodiscard]] mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic physics_policy_diagnostic() const noexcept {
        return physics_policy_result_.diagnostic;
    }

    [[nodiscard]] std::size_t physics_policy_row_count() const noexcept {
        return physics_policy_result_.rows.size();
    }

    [[nodiscard]] std::size_t physics_policy_dynamic_push_count() const noexcept {
        return physics_policy_dynamic_push_count_;
    }

    [[nodiscard]] std::size_t physics_policy_solid_contact_count() const noexcept {
        return physics_policy_solid_contact_count_;
    }

    [[nodiscard]] std::size_t physics_policy_trigger_overlap_count() const noexcept {
        return physics_policy_trigger_overlap_count_;
    }

    [[nodiscard]] bool runtime_scene_gameplay_interactions_ready() const noexcept {
        return runtime_scene_gameplay_interaction_plan_.succeeded() &&
               runtime_scene_gameplay_interaction_plan_.rows.size() == 4U &&
               runtime_scene_gameplay_interaction_plan_.final_session_state ==
                   mirakana::runtime_scene::RuntimeSceneGameplaySessionState::won;
    }

    [[nodiscard]] std::size_t runtime_scene_gameplay_interaction_count() const noexcept {
        return runtime_scene_gameplay_interaction_plan_.rows.size();
    }

    [[nodiscard]] mirakana::runtime_scene::RuntimeSceneGameplaySessionState
    runtime_scene_gameplay_final_state() const noexcept {
        return runtime_scene_gameplay_interaction_plan_.final_session_state;
    }

    [[nodiscard]] std::string gameplay_tick_order_trace() const {
        std::string trace;
        for (std::size_t i = 0U; i < last_tick_step_count_; ++i) {
            if (i != 0U) {
                trace += " > ";
            }
            trace += gameplay_runtime_step_name(last_tick_steps_[i]);
        }
        return trace;
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
        const auto plan = mirakana::plan_navigation_grid_agent_path(
            navigation_grid_, mirakana::NavigationGridAgentPathRequest{
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

    void build_navigation_navmesh_probe() {
        const std::array polygons{
            mirakana::NavigationNavmeshPolygon{.id = 1U,
                                               .scene_ref = "gameplay.start",
                                               .center = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                                               .traversal_cost = 1U},
            mirakana::NavigationNavmeshPolygon{.id = 2U,
                                               .scene_ref = "gameplay.direct",
                                               .center = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                                               .traversal_cost = 1U},
            mirakana::NavigationNavmeshPolygon{.id = 3U,
                                               .scene_ref = "gameplay.goal",
                                               .center = mirakana::NavigationPoint2{.x = 2.0F, .y = 0.0F},
                                               .traversal_cost = 1U},
            mirakana::NavigationNavmeshPolygon{.id = 4U,
                                               .scene_ref = "gameplay.detour",
                                               .center = mirakana::NavigationPoint2{.x = 1.0F, .y = 1.0F},
                                               .traversal_cost = 1U},
        };
        const std::array portals{
            mirakana::NavigationNavmeshPortal{.from = 1U, .to = 2U, .cost = 1U, .bidirectional = true},
            mirakana::NavigationNavmeshPortal{.from = 2U, .to = 3U, .cost = 1U, .bidirectional = true},
            mirakana::NavigationNavmeshPortal{.from = 1U, .to = 4U, .cost = 1U, .bidirectional = true},
            mirakana::NavigationNavmeshPortal{.from = 4U, .to = 3U, .cost = 1U, .bidirectional = true},
        };
        const std::array obstacles{
            mirakana::NavigationNavmeshDynamicObstacle{
                .id = 1U, .blocked_polygon = 2U, .scene_ref = "gameplay.dynamic_obstacle", .enabled = true},
        };
        navigation_navmesh_plan_ = mirakana::plan_navigation_navmesh_path(mirakana::NavigationNavmeshPathRequest{
            .polygons = std::span<const mirakana::NavigationNavmeshPolygon>{polygons},
            .portals = std::span<const mirakana::NavigationNavmeshPortal>{portals},
            .dynamic_obstacles = std::span<const mirakana::NavigationNavmeshDynamicObstacle>{obstacles},
            .start = 1U,
            .goal = 3U,
        });
    }

    void build_physics_movement_policy_probe() {
        mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
        constexpr std::uint32_t character_layer = 1U << 0U;
        constexpr std::uint32_t trigger_layer = 1U << 1U;
        constexpr std::uint32_t dynamic_layer = 1U << 2U;
        constexpr std::uint32_t solid_layer = 1U << 3U;
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.8F, .y = 1.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 1.0F, .z = 1.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .collision_layer = trigger_layer,
            .collision_mask = character_layer,
            .trigger = true,
        });
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 2.5F, .y = 1.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.5F, .z = 0.5F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .collision_layer = dynamic_layer,
            .collision_mask = character_layer,
        });
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 0.25F, .y = 1.0F, .z = 1.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .collision_layer = solid_layer,
            .collision_mask = character_layer,
        });

        mirakana::PhysicsCharacterDynamicPolicy3DDesc request;
        request.position = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F};
        request.displacement = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F};
        request.radius = 0.5F;
        request.half_height = 0.5F;
        request.character_layer = character_layer;
        request.collision_mask = trigger_layer | dynamic_layer | solid_layer;
        request.include_triggers = true;
        request.skin_width = 0.05F;
        request.dynamic_push_distance = 0.25F;
        physics_policy_result_ = mirakana::evaluate_physics_character_dynamic_policy_3d(world, request);
        physics_policy_dynamic_push_count_ = 0U;
        physics_policy_solid_contact_count_ = 0U;
        physics_policy_trigger_overlap_count_ = 0U;
        for (const auto& row : physics_policy_result_.rows) {
            if (row.kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::dynamic_push) {
                ++physics_policy_dynamic_push_count_;
            } else if (row.kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::solid_contact) {
                ++physics_policy_solid_contact_count_;
            } else if (row.kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::trigger_overlap) {
                ++physics_policy_trigger_overlap_count_;
            }
        }
    }

    void seed_navigation_dynamic_obstacle() {
        if (navigation_dynamic_obstacle_injected_) {
            return;
        }

        if (navigation_agent_.path.size() < 3U) {
            return;
        }

        const mirakana::NavigationGridCoord obstacle_coord = to_navigation_grid_coord(navigation_agent_.path[1U]);
        if (!navigation_grid_.in_bounds(obstacle_coord) || !navigation_grid_.cell(obstacle_coord).walkable) {
            return;
        }

        navigation_grid_.set_walkable(obstacle_coord, false);
        navigation_dynamic_obstacle_injected_ = true;
        ++navigation_dynamic_obstacle_count_;
    }

    void update_navigation_replan_layer() {
        if (navigation_plan_status_ != mirakana::NavigationGridAgentPathStatus::ready ||
            navigation_agent_.path.empty()) {
            ++navigation_replan_attempt_count_;
            navigation_replan_last_status_ = mirakana::NavigationGridReplanStatus::invalid_endpoint;
            return;
        }

        seed_navigation_dynamic_obstacle();

        const auto current_coord = to_navigation_grid_coord(navigation_agent_.position);
        const auto goal_coord = to_navigation_grid_coord(navigation_agent_.path.back());
        const auto remaining_grid_path =
            to_navigation_grid_path(std::span<const mirakana::NavigationPoint2>{navigation_agent_.path});
        const auto replan = mirakana::replan_navigation_grid_path(
            navigation_grid_, mirakana::NavigationGridReplanRequest{
                                  .current = current_coord,
                                  .goal = goal_coord,
                                  .remaining_path = std::span<const mirakana::NavigationGridCoord>{remaining_grid_path},
                              });

        ++navigation_replan_attempt_count_;
        navigation_replan_last_status_ = replan.status;
        navigation_replan_last_total_cost_ = replan.total_cost;
        navigation_replan_last_path_point_count_ = replan.path.size();

        if (replan.status == mirakana::NavigationGridReplanStatus::reused_existing_path) {
            ++navigation_replan_reused_count_;
            return;
        }
        if (replan.status == mirakana::NavigationGridReplanStatus::replanned) {
            const auto replanned_path_points = mirakana::build_navigation_point_path(
                std::span<const mirakana::NavigationGridCoord>{replan.path}, navigation_mapping());
            navigation_agent_ = mirakana::replace_navigation_agent_path(navigation_agent_, replanned_path_points);
            ++navigation_replan_replanned_count_;
            return;
        }
        if (replan.status == mirakana::NavigationGridReplanStatus::no_path) {
            navigation_agent_ = mirakana::cancel_navigation_agent_move(navigation_agent_);
        }
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

    void build_runtime_scene_gameplay_interaction_probe() {
        const std::vector<mirakana::runtime_scene::RuntimeSceneGameplayBindingRow> bindings{
            {
                .binding_id = "actor.player",
                .gameplay_system_id = "sample_gameplay",
                .slot_id = "actor",
                .node_name = "Player",
                .node = mirakana::SceneNodeId{1U},
                .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::mesh_renderer,
            },
            {
                .binding_id = "hazard.trigger",
                .gameplay_system_id = "sample_gameplay",
                .slot_id = "hazard",
                .node_name = "Hazard",
                .node = mirakana::SceneNodeId{2U},
                .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::any_renderable,
            },
            {
                .binding_id = "goal.exit",
                .gameplay_system_id = "sample_gameplay",
                .slot_id = "goal",
                .node_name = "Exit",
                .node = mirakana::SceneNodeId{3U},
                .required_component = mirakana::runtime_scene::RuntimeSceneGameplayBindingComponentKind::any_renderable,
            },
        };
        const std::vector<mirakana::runtime_scene::RuntimeSceneGameplayInteractionSourceRow> interactions{
            {
                .action_id = "hazard.enter",
                .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::trigger,
                .source_binding_id = "actor.player",
                .target_binding_id = "hazard.trigger",
            },
            {
                .action_id = "hazard.damage",
                .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::damage,
                .source_binding_id = "hazard.trigger",
                .target_binding_id = "actor.player",
                .amount = 1,
            },
            {
                .action_id = "objective.escape",
                .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::objective_progress,
                .source_binding_id = "actor.player",
                .objective_id = "escape",
                .amount = 1,
            },
            {
                .action_id = "goal.win",
                .kind = mirakana::runtime_scene::RuntimeSceneGameplayInteractionKind::win,
                .source_binding_id = "goal.exit",
            },
        };
        runtime_scene_gameplay_interaction_plan_ = mirakana::runtime_scene::plan_runtime_scene_gameplay_interactions(
            bindings, interactions,
            mirakana::runtime_scene::RuntimeSceneGameplayInteractionPlanRequest{
                .session_state = mirakana::runtime_scene::RuntimeSceneGameplaySessionState::running,
            });
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

        update_navigation_local_avoidance_probe();
        const auto update = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
            .state = navigation_agent_,
            .config =
                mirakana::NavigationAgentConfig{.max_speed = 8.0F, .slowing_radius = 1.0F, .arrival_radius = 0.001F},
            .delta_seconds = 1.0F,
        });
        navigation_agent_ = update.state;
    }

    void update_navigation_local_avoidance_probe() {
        const std::array neighbors{
            mirakana::NavigationLocalAvoidanceNeighborDesc{
                .id = 200U,
                .position = mirakana::NavigationPoint2{.x = navigation_agent_.position.x + 0.25F,
                                                       .y = navigation_agent_.position.y},
                .velocity = mirakana::NavigationPoint2{.x = -0.25F, .y = 0.0F},
                .radius = 0.5F,
            },
        };
        const auto avoidance = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
            .agent =
                mirakana::NavigationLocalAvoidanceAgentDesc{
                    .id = 100U,
                    .position = navigation_agent_.position,
                    .desired_velocity = mirakana::NavigationPoint2{.x = 2.0F, .y = 0.0F},
                    .radius = 0.5F,
                    .max_speed = 2.0F,
                },
            .neighbors = std::span<const mirakana::NavigationLocalAvoidanceNeighborDesc>{neighbors},
            .separation_weight = 1.0F,
            .prediction_time_seconds = 1.0F,
        });
        local_avoidance_last_status_ = avoidance.status;
        local_avoidance_last_diagnostic_ = avoidance.diagnostic;
        if (avoidance.status == mirakana::NavigationLocalAvoidanceStatus::success) {
            ++local_avoidance_step_count_;
            local_avoidance_applied_neighbor_count_ += avoidance.applied_neighbor_count;
        }
    }

    mirakana::NavigationGrid navigation_grid_;
    mirakana::PhysicsWorld3D physics_;
    mirakana::AnimationStateMachine animation_;
    mirakana::PhysicsAuthoredCollisionScene3DBuildResult authored_collision_;
    mirakana::PhysicsCharacterController3DResult controller_result_;
    mirakana::PhysicsCharacterDynamicPolicy3DResult physics_policy_result_;
    mirakana::NavigationNavmeshPathResult navigation_navmesh_plan_;
    mirakana::NavigationAgentState navigation_agent_;
    mirakana::BehaviorTreeTickResult last_tree_result_;
    mirakana::runtime_scene::RuntimeSceneGameplayInteractionPlan runtime_scene_gameplay_interaction_plan_;
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
    std::size_t navigation_replan_attempt_count_{0U};
    std::size_t navigation_replan_reused_count_{0U};
    std::size_t navigation_replan_replanned_count_{0U};
    std::size_t local_avoidance_applied_neighbor_count_{0U};
    std::size_t navigation_dynamic_obstacle_count_{0U};
    std::uint32_t navigation_replan_last_total_cost_{0U};
    std::size_t local_avoidance_step_count_{0U};
    std::size_t navigation_replan_last_path_point_count_{0U};
    std::size_t physics_policy_dynamic_push_count_{0U};
    std::size_t physics_policy_solid_contact_count_{0U};
    std::size_t physics_policy_trigger_overlap_count_{0U};
    std::size_t last_perception_target_count_{0U};
    std::uint32_t audio_stream_frames_{0U};
    std::size_t audio_stream_sample_count_{0U};
    std::size_t tick_order_violations_{0U};
    std::size_t last_tick_step_count_{0U};
    std::array<GameplayRuntimeStep, k_gameplay_runtime_step_count> last_tick_steps_{};
    mirakana::NavigationGridReplanStatus navigation_replan_last_status_{
        mirakana::NavigationGridReplanStatus::invalid_endpoint};
    mirakana::NavigationLocalAvoidanceStatus local_avoidance_last_status_{
        mirakana::NavigationLocalAvoidanceStatus::invalid_request};
    mirakana::NavigationLocalAvoidanceDiagnostic local_avoidance_last_diagnostic_{
        mirakana::NavigationLocalAvoidanceDiagnostic::none};
    bool navigation_dynamic_obstacle_injected_{false};
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
              << " gameplay_tick_steps=" << game.last_tick_step_count()
              << " gameplay_tick_order_ok=" << (game.gameplay_tick_order_stable() ? 1 : 0)
              << " gameplay_tick_order_violations=" << game.gameplay_tick_order_violations()
              << " gameplay_tick_order_trace=" << game.gameplay_tick_order_trace()
              << " navigation_navmesh_status=" << navigation_navmesh_path_status_name(game.navigation_navmesh_status())
              << " navigation_navmesh_diagnostic="
              << navigation_navmesh_path_diagnostic_name(game.navigation_navmesh_diagnostic())
              << " navigation_navmesh_polygons=" << game.navigation_navmesh_polygon_count()
              << " navigation_navmesh_dynamic_obstacles=" << game.navigation_navmesh_dynamic_obstacle_count()
              << " navigation_navmesh_total_cost=" << game.navigation_navmesh_total_cost()
              << " navigation_dynamic_obstacles=" << game.navigation_dynamic_obstacle_count()
              << " navigation_replan_attempts=" << game.navigation_replan_attempt_count()
              << " navigation_replan_replanned=" << game.navigation_replan_replanned_count()
              << " navigation_replan_reused=" << game.navigation_replan_reused_count()
              << " navigation_replan_last_status="
              << navigation_grid_replan_status_name(game.navigation_replan_last_status())
              << " navigation_replan_last_path_points=" << game.navigation_replan_last_path_point_count()
              << " local_avoidance_status="
              << navigation_local_avoidance_status_name(game.local_avoidance_last_status())
              << " local_avoidance_diagnostic="
              << navigation_local_avoidance_diagnostic_name(game.local_avoidance_last_diagnostic())
              << " local_avoidance_steps=" << game.local_avoidance_step_count()
              << " local_avoidance_applied_neighbors=" << game.local_avoidance_applied_neighbor_count()
              << " physics_policy_status=" << physics_character_dynamic_policy_status_name(game.physics_policy_status())
              << " physics_policy_diagnostic="
              << physics_character_dynamic_policy_diagnostic_name(game.physics_policy_diagnostic())
              << " physics_policy_rows=" << game.physics_policy_row_count()
              << " physics_policy_dynamic_pushes=" << game.physics_policy_dynamic_push_count()
              << " physics_policy_solid_contacts=" << game.physics_policy_solid_contact_count()
              << " physics_policy_trigger_overlaps=" << game.physics_policy_trigger_overlap_count()
              << " runtime_scene_gameplay_interactions=" << game.runtime_scene_gameplay_interaction_count()
              << " runtime_scene_gameplay_final_state="
              << runtime_scene_gameplay_session_state_name(game.runtime_scene_gameplay_final_state()) << '\n';

    return result.status == mirakana::RunStatus::stopped_by_app && result.frames_run == 4 && game.frames() == 4 &&
                   game.initialized() && registry.living_count() == 0 && std::abs(position.x - 1.25F) < 0.0001F &&
                   std::abs(position.y - 2.0F) < 0.0001F && std::abs(velocity.y) < 0.0001F &&
                   animation.to_state == "walk" && !animation.blending && game.authored_collision_body_count() == 2U &&
                   game.controller_grounded() && game.navigation_path_point_count() == 3U &&
                   navigation.status == mirakana::NavigationAgentStatus::reached_destination &&
                   behavior.status == mirakana::BehaviorTreeStatus::success && behavior.visited_nodes.size() == 4U &&
                   game.perception_target_count() == 1U && game.audio_stream_frames() == 2U &&
                   game.audio_stream_sample_count() == 2U && game.gameplay_tick_order_stable() &&
                   game.navigation_navmesh_status() == mirakana::NavigationNavmeshPathStatus::success &&
                   game.navigation_navmesh_polygon_count() == 3U &&
                   game.navigation_navmesh_dynamic_obstacle_count() == 1U &&
                   game.navigation_dynamic_obstacle_count() == 1U && game.navigation_replan_replanned_count() > 0U &&
                   game.local_avoidance_last_status() == mirakana::NavigationLocalAvoidanceStatus::success &&
                   game.local_avoidance_step_count() > 0U && game.local_avoidance_applied_neighbor_count() > 0U &&
                   game.physics_policy_status() == mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained &&
                   game.physics_policy_diagnostic() == mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::none &&
                   game.physics_policy_row_count() == 3U && game.physics_policy_dynamic_push_count() == 1U &&
                   game.physics_policy_solid_contact_count() == 1U &&
                   game.physics_policy_trigger_overlap_count() == 1U && game.runtime_scene_gameplay_interactions_ready()
               ? 0
               : 1;
}
