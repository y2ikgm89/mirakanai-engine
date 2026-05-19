// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ai/behavior_tree.hpp"
#include "mirakana/ai/perception.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/audio/audio_mixer.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/navigation/navigation_agent.hpp"
#include "mirakana/navigation/navigation_grid.hpp"
#include "mirakana/navigation/navigation_path_planner.hpp"
#include "mirakana/physics/physics2d.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/renderer/sprite_batch.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/runtime_diagnostics.hpp"
#include "mirakana/runtime/session_services.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"
#include "mirakana/scene/playable_2d.hpp"
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/ui/ui.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct DesktopRuntimeOptions {
    bool smoke{false};
    bool show_help{false};
    bool throttle{true};
    bool require_d3d12_shaders{false};
    bool require_d3d12_renderer{false};
    bool require_vulkan_shaders{false};
    bool require_vulkan_renderer{false};
    bool require_native_2d_sprites{false};
    bool require_sprite_animation{false};
    bool require_tilemap_runtime_ux{false};
    bool require_gameplay_systems{false};
    std::uint32_t max_frames{0};
    std::string video_driver_hint;
    std::string required_config_path;
    std::string required_scene_package_path;
};

constexpr std::string_view kExpectedConfigFormat{"format=GameEngine.Sample2DDesktopRuntimePackage.Config.v1"};
constexpr std::string_view kRuntime2dVertexShaderPath{"shaders/sample_2d_desktop_runtime_package_sprite.vs.dxil"};
constexpr std::string_view kRuntime2dFragmentShaderPath{"shaders/sample_2d_desktop_runtime_package_sprite.ps.dxil"};
constexpr std::string_view kRuntime2dVulkanVertexShaderPath{"shaders/sample_2d_desktop_runtime_package_sprite.vs.spv"};
constexpr std::string_view kRuntime2dVulkanFragmentShaderPath{
    "shaders/sample_2d_desktop_runtime_package_sprite.ps.spv"};
constexpr std::string_view kRuntime2dNativeSpriteOverlayVertexShaderPath{
    "shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil"};
constexpr std::string_view kRuntime2dNativeSpriteOverlayFragmentShaderPath{
    "shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil"};
constexpr std::string_view kRuntime2dVulkanNativeSpriteOverlayVertexShaderPath{
    "shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv"};
constexpr std::string_view kRuntime2dVulkanNativeSpriteOverlayFragmentShaderPath{
    "shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv"};
constexpr mirakana::SceneNodeId kPlayerNode{2};
constexpr mirakana::BehaviorTreeNodeId kGameplay2dRootNode{1};
constexpr mirakana::BehaviorTreeNodeId kGameplay2dHasTargetNode{2};
constexpr mirakana::BehaviorTreeNodeId kGameplay2dNeedsMoveNode{3};
constexpr mirakana::BehaviorTreeNodeId kGameplay2dMoveActionNode{4};
constexpr const char* kGameplay2dHasTargetKey{"sample2d.has_target"};
constexpr const char* kGameplay2dNeedsMoveKey{"sample2d.needs_move"};
constexpr const char* kGameplay2dTargetIdKey{"sample2d.target_id"};
constexpr const char* kGameplay2dTargetDistanceKey{"sample2d.target_distance"};
constexpr const char* kGameplay2dVisibleTargetsKey{"sample2d.visible_targets"};
constexpr const char* kGameplay2dAudibleTargetsKey{"sample2d.audible_targets"};
constexpr const char* kGameplay2dTargetStateKey{"sample2d.target_state"};

enum class Gameplay2DSystemsStatus : std::uint8_t {
    not_started,
    ready,
    diagnostics,
};

[[nodiscard]] std::string_view gameplay_2d_systems_status_name(Gameplay2DSystemsStatus status) noexcept {
    switch (status) {
    case Gameplay2DSystemsStatus::not_started:
        return "not_started";
    case Gameplay2DSystemsStatus::ready:
        return "ready";
    case Gameplay2DSystemsStatus::diagnostics:
        return "diagnostics";
    }
    return "unknown";
}

[[nodiscard]] bool gameplay_2d_near(float value, float expected, float epsilon = 0.001F) noexcept {
    return std::abs(value - expected) <= epsilon;
}

[[nodiscard]] std::string_view
navigation_grid_agent_path_status_name(mirakana::NavigationGridAgentPathStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationGridAgentPathStatus::ready:
        return "ready";
    case mirakana::NavigationGridAgentPathStatus::invalid_request:
        return "invalid_request";
    case mirakana::NavigationGridAgentPathStatus::invalid_mapping:
        return "invalid_mapping";
    case mirakana::NavigationGridAgentPathStatus::unsupported_adjacency:
        return "unsupported_adjacency";
    case mirakana::NavigationGridAgentPathStatus::invalid_endpoint:
        return "invalid_endpoint";
    case mirakana::NavigationGridAgentPathStatus::blocked_endpoint:
        return "blocked_endpoint";
    case mirakana::NavigationGridAgentPathStatus::no_path:
        return "no_path";
    case mirakana::NavigationGridAgentPathStatus::invalid_source_path:
        return "invalid_source_path";
    case mirakana::NavigationGridAgentPathStatus::agent_path_invalid:
        return "agent_path_invalid";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_grid_agent_path_diagnostic_name(mirakana::NavigationGridAgentPathDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::NavigationGridAgentPathDiagnostic::none:
        return "none";
    case mirakana::NavigationGridAgentPathDiagnostic::invalid_mapping:
        return "invalid_mapping";
    case mirakana::NavigationGridAgentPathDiagnostic::unsupported_adjacency:
        return "unsupported_adjacency";
    case mirakana::NavigationGridAgentPathDiagnostic::path_invalid_endpoint:
        return "path_invalid_endpoint";
    case mirakana::NavigationGridAgentPathDiagnostic::path_blocked_endpoint:
        return "path_blocked_endpoint";
    case mirakana::NavigationGridAgentPathDiagnostic::path_not_found:
        return "path_not_found";
    case mirakana::NavigationGridAgentPathDiagnostic::smoothing_invalid_source_path:
        return "smoothing_invalid_source_path";
    case mirakana::NavigationGridAgentPathDiagnostic::smoothing_unsupported_adjacency:
        return "smoothing_unsupported_adjacency";
    case mirakana::NavigationGridAgentPathDiagnostic::point_mapping_failed:
        return "point_mapping_failed";
    case mirakana::NavigationGridAgentPathDiagnostic::agent_path_rejected:
        return "agent_path_rejected";
    }
    return "unknown";
}

[[nodiscard]] std::string_view navigation_agent_status_name(mirakana::NavigationAgentStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationAgentStatus::idle:
        return "idle";
    case mirakana::NavigationAgentStatus::moving:
        return "moving";
    case mirakana::NavigationAgentStatus::reached_destination:
        return "reached_destination";
    case mirakana::NavigationAgentStatus::cancelled:
        return "cancelled";
    case mirakana::NavigationAgentStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view ai_perception_status_name(mirakana::AiPerceptionStatus status) noexcept {
    switch (status) {
    case mirakana::AiPerceptionStatus::ready:
        return "ready";
    case mirakana::AiPerceptionStatus::invalid_agent:
        return "invalid_agent";
    case mirakana::AiPerceptionStatus::invalid_target:
        return "invalid_target";
    case mirakana::AiPerceptionStatus::duplicate_target_id:
        return "duplicate_target_id";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
ai_perception_blackboard_status_name(mirakana::AiPerceptionBlackboardStatus status) noexcept {
    switch (status) {
    case mirakana::AiPerceptionBlackboardStatus::ready:
        return "ready";
    case mirakana::AiPerceptionBlackboardStatus::invalid_snapshot:
        return "invalid_snapshot";
    case mirakana::AiPerceptionBlackboardStatus::invalid_key:
        return "invalid_key";
    case mirakana::AiPerceptionBlackboardStatus::blackboard_write_failed:
        return "blackboard_write_failed";
    }
    return "unknown";
}

[[nodiscard]] std::string_view behavior_tree_status_name(mirakana::BehaviorTreeStatus status) noexcept {
    switch (status) {
    case mirakana::BehaviorTreeStatus::success:
        return "success";
    case mirakana::BehaviorTreeStatus::failure:
        return "failure";
    case mirakana::BehaviorTreeStatus::running:
        return "running";
    case mirakana::BehaviorTreeStatus::invalid_tree:
        return "invalid_tree";
    case mirakana::BehaviorTreeStatus::missing_leaf_result:
        return "missing_leaf_result";
    case mirakana::BehaviorTreeStatus::invalid_leaf_result:
        return "invalid_leaf_result";
    }
    return "unknown";
}

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/scene");
}

[[nodiscard]] mirakana::AssetId packaged_audio_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/jump-audio");
}

[[nodiscard]] mirakana::AssetId packaged_sprite_texture_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/player-sprite");
}

[[nodiscard]] mirakana::AssetId packaged_sprite_animation_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/player-sprite-animation");
}

[[nodiscard]] mirakana::AssetId packaged_tilemap_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/tilemap");
}

class Gameplay2DSystemsProbe final {
  public:
    Gameplay2DSystemsProbe() : physics_(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}}) {}

    void start() {
        constexpr std::uint32_t character_layer = 1U << 0U;
        constexpr std::uint32_t solid_layer = 1U << 1U;
        constexpr std::uint32_t trigger_layer = 1U << 2U;

        actor_body_ = physics_.create_body(mirakana::PhysicsBody2DDesc{
            .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape2DKind::aabb,
            .radius = 0.5F,
            .collision_layer = character_layer,
            .collision_mask = solid_layer | trigger_layer,
        });
        solid_body_ = physics_.create_body(mirakana::PhysicsBody2DDesc{
            .position = mirakana::Vec2{.x = 0.75F, .y = 0.0F},
            .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape2DKind::aabb,
            .radius = 0.5F,
            .collision_layer = solid_layer,
            .collision_mask = character_layer,
        });
        trigger_body_ = physics_.create_body(mirakana::PhysicsBody2DDesc{
            .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec2{.x = 0.6F, .y = 0.6F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape2DKind::aabb,
            .radius = 0.5F,
            .collision_layer = trigger_layer,
            .collision_mask = character_layer,
            .trigger = true,
        });

        mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 2, .height = 1});
        const auto plan =
            mirakana::plan_navigation_grid_agent_path(grid, mirakana::NavigationGridAgentPathRequest{
                                                                .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                                .goal = mirakana::NavigationGridCoord{.x = 1, .y = 0},
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
        navigation_plan_diagnostic_ = plan.diagnostic;
        navigation_path_point_count_ = plan.planned_grid_point_count;
        navigation_agent_ = plan.agent_state;
        if (!navigation_agent_.path.empty()) {
            navigation_goal_ = navigation_agent_.path.back();
        }

        started_ = true;
    }

    void tick() {
        if (!started_) {
            return;
        }

        physics_.apply_force(actor_body_, mirakana::Vec2{.x = 4.0F, .y = 0.0F});
        physics_.step(0.25F);
        const auto contacts = physics_.contacts();
        const auto overlaps = physics_.trigger_overlaps();
        physics_contact_count_ += contacts.size();
        physics_trigger_overlap_count_ += overlaps.size();
        physics_.resolve_contacts();
        ++physics_ticks_;

        update_ai_navigation_composition();
        ++ticks_;
    }

    [[nodiscard]] bool passed(std::uint32_t expected_ticks) const {
        return started_ && ticks_ == expected_ticks && physics_ticks_ == expected_ticks &&
               physics_.bodies().size() == 3U && actor_body_ != mirakana::null_physics_body_2d &&
               solid_body_ != mirakana::null_physics_body_2d && trigger_body_ != mirakana::null_physics_body_2d &&
               physics_contact_count_ > 0U && physics_trigger_overlap_count_ > 0U &&
               navigation_plan_status_ == mirakana::NavigationGridAgentPathStatus::ready &&
               navigation_plan_diagnostic_ == mirakana::NavigationGridAgentPathDiagnostic::none &&
               navigation_path_point_count_ == 2U &&
               navigation_agent_.status == mirakana::NavigationAgentStatus::reached_destination &&
               gameplay_2d_near(navigation_goal_.x, 1.5F) && gameplay_2d_near(navigation_goal_.y, 0.5F) &&
               gameplay_2d_near(navigation_agent_.position.x, 1.5F) &&
               gameplay_2d_near(navigation_agent_.position.y, 0.5F) &&
               last_perception_status_ == mirakana::AiPerceptionStatus::ready && last_perception_has_primary_target_ &&
               last_perception_target_count_ == 1U && last_perception_visible_count_ == 1U &&
               last_blackboard_status_ == mirakana::AiPerceptionBlackboardStatus::ready && blackboard_has_target_ &&
               blackboard_needs_move_ && last_tree_result_.status == mirakana::BehaviorTreeStatus::success &&
               last_tree_result_.visited_nodes.size() == 4U;
    }

    [[nodiscard]] Gameplay2DSystemsStatus status(std::uint32_t expected_ticks) const {
        if (!started_) {
            return Gameplay2DSystemsStatus::not_started;
        }
        return passed(expected_ticks) ? Gameplay2DSystemsStatus::ready : Gameplay2DSystemsStatus::diagnostics;
    }

    [[nodiscard]] std::uint32_t ticks() const noexcept {
        return ticks_;
    }

    [[nodiscard]] std::uint32_t physics_ticks() const noexcept {
        return physics_ticks_;
    }

    [[nodiscard]] std::size_t physics_body_count() const noexcept {
        return physics_.bodies().size();
    }

    [[nodiscard]] std::size_t physics_contact_count() const noexcept {
        return physics_contact_count_;
    }

    [[nodiscard]] std::size_t physics_trigger_overlap_count() const noexcept {
        return physics_trigger_overlap_count_;
    }

    [[nodiscard]] std::size_t navigation_path_point_count() const noexcept {
        return navigation_path_point_count_;
    }

    [[nodiscard]] bool navigation_reached_destination() const noexcept {
        return navigation_agent_.status == mirakana::NavigationAgentStatus::reached_destination;
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathStatus navigation_plan_status() const noexcept {
        return navigation_plan_status_;
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathDiagnostic navigation_plan_diagnostic() const noexcept {
        return navigation_plan_diagnostic_;
    }

    [[nodiscard]] mirakana::NavigationAgentStatus navigation_agent_status() const noexcept {
        return navigation_agent_.status;
    }

    [[nodiscard]] mirakana::AiPerceptionStatus perception_status() const noexcept {
        return last_perception_status_;
    }

    [[nodiscard]] std::size_t perception_target_count() const noexcept {
        return last_perception_target_count_;
    }

    [[nodiscard]] bool perception_has_primary_target() const noexcept {
        return last_perception_has_primary_target_;
    }

    [[nodiscard]] std::size_t perception_visible_count() const noexcept {
        return last_perception_visible_count_;
    }

    [[nodiscard]] mirakana::AiPerceptionBlackboardStatus blackboard_status() const noexcept {
        return last_blackboard_status_;
    }

    [[nodiscard]] bool blackboard_has_target() const noexcept {
        return blackboard_has_target_;
    }

    [[nodiscard]] bool blackboard_needs_move() const noexcept {
        return blackboard_needs_move_;
    }

    [[nodiscard]] mirakana::BehaviorTreeStatus behavior_status() const noexcept {
        return last_tree_result_.status;
    }

    [[nodiscard]] std::size_t behavior_visited_node_count() const noexcept {
        return last_tree_result_.visited_nodes.size();
    }

  private:
    void update_ai_navigation_composition() {
        if (navigation_plan_status_ != mirakana::NavigationGridAgentPathStatus::ready ||
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
                                                   .sight_range = 4.0F,
                                                   .field_of_view_radians = 6.28318530718F,
                                                   .hearing_range = 0.0F},
            .targets = std::span<const mirakana::AiPerceptionTarget2D>{route_targets},
        });
        last_perception_status_ = perception.status;
        last_perception_target_count_ = perception.targets.size();
        last_perception_has_primary_target_ = perception.has_primary_target;
        last_perception_visible_count_ = perception.visible_count;

        mirakana::BehaviorTreeBlackboard blackboard;
        const auto blackboard_result =
            mirakana::write_ai_perception_blackboard(perception,
                                                     mirakana::AiPerceptionBlackboardKeys{
                                                         .has_target_key = kGameplay2dHasTargetKey,
                                                         .target_id_key = kGameplay2dTargetIdKey,
                                                         .target_distance_key = kGameplay2dTargetDistanceKey,
                                                         .visible_count_key = kGameplay2dVisibleTargetsKey,
                                                         .audible_count_key = kGameplay2dAudibleTargetsKey,
                                                         .target_state_key = kGameplay2dTargetStateKey,
                                                     },
                                                     blackboard);
        last_blackboard_status_ = blackboard_result.status;
        if (blackboard_result.status != mirakana::AiPerceptionBlackboardStatus::ready ||
            !blackboard.set(kGameplay2dNeedsMoveKey, mirakana::make_behavior_tree_blackboard_bool(true))) {
            return;
        }

        if (const auto* value = blackboard.find(kGameplay2dHasTargetKey);
            value != nullptr && value->kind == mirakana::BehaviorTreeBlackboardValueKind::boolean) {
            blackboard_has_target_ = value->bool_value;
        }
        if (const auto* value = blackboard.find(kGameplay2dNeedsMoveKey);
            value != nullptr && value->kind == mirakana::BehaviorTreeBlackboardValueKind::boolean) {
            blackboard_needs_move_ = value->bool_value;
        }

        const std::vector<mirakana::BehaviorTreeBlackboardCondition> conditions{
            mirakana::BehaviorTreeBlackboardCondition{.node_id = kGameplay2dHasTargetNode,
                                                      .key = kGameplay2dHasTargetKey,
                                                      .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                      .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
            mirakana::BehaviorTreeBlackboardCondition{.node_id = kGameplay2dNeedsMoveNode,
                                                      .key = kGameplay2dNeedsMoveKey,
                                                      .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                      .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
        };
        const auto supporting_systems_ready =
            physics_.bodies().size() == 3U && physics_contact_count_ > 0U && physics_trigger_overlap_count_ > 0U;
        const std::vector<mirakana::BehaviorTreeLeafResult> leaf_results{
            mirakana::BehaviorTreeLeafResult{.node_id = kGameplay2dMoveActionNode,
                                             .status = supporting_systems_ready
                                                           ? mirakana::BehaviorTreeStatus::success
                                                           : mirakana::BehaviorTreeStatus::failure},
        };

        last_tree_result_ = mirakana::evaluate_behavior_tree(
            mirakana::BehaviorTreeDesc{
                .root_id = kGameplay2dRootNode,
                .nodes =
                    {
                        mirakana::BehaviorTreeNodeDesc{.id = kGameplay2dRootNode,
                                                       .kind = mirakana::BehaviorTreeNodeKind::sequence,
                                                       .children = {kGameplay2dHasTargetNode, kGameplay2dNeedsMoveNode,
                                                                    kGameplay2dMoveActionNode}},
                        mirakana::BehaviorTreeNodeDesc{.id = kGameplay2dHasTargetNode,
                                                       .kind = mirakana::BehaviorTreeNodeKind::condition,
                                                       .children = {}},
                        mirakana::BehaviorTreeNodeDesc{.id = kGameplay2dNeedsMoveNode,
                                                       .kind = mirakana::BehaviorTreeNodeKind::condition,
                                                       .children = {}},
                        mirakana::BehaviorTreeNodeDesc{.id = kGameplay2dMoveActionNode,
                                                       .kind = mirakana::BehaviorTreeNodeKind::action,
                                                       .children = {}},
                    },
            },
            mirakana::BehaviorTreeEvaluationContext{
                .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{leaf_results},
                .blackboard_entries = blackboard.entries(),
                .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{conditions},
            });

        if (last_tree_result_.status != mirakana::BehaviorTreeStatus::success ||
            navigation_agent_.status == mirakana::NavigationAgentStatus::reached_destination) {
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

    mirakana::PhysicsWorld2D physics_;
    mirakana::NavigationAgentState navigation_agent_;
    mirakana::BehaviorTreeTickResult last_tree_result_;
    mirakana::PhysicsBody2DId actor_body_{mirakana::null_physics_body_2d};
    mirakana::PhysicsBody2DId solid_body_{mirakana::null_physics_body_2d};
    mirakana::PhysicsBody2DId trigger_body_{mirakana::null_physics_body_2d};
    mirakana::NavigationGridAgentPathStatus navigation_plan_status_{
        mirakana::NavigationGridAgentPathStatus::invalid_request};
    mirakana::NavigationGridAgentPathDiagnostic navigation_plan_diagnostic_{
        mirakana::NavigationGridAgentPathDiagnostic::none};
    mirakana::NavigationPoint2 navigation_goal_{};
    mirakana::AiPerceptionStatus last_perception_status_{mirakana::AiPerceptionStatus::invalid_agent};
    mirakana::AiPerceptionBlackboardStatus last_blackboard_status_{
        mirakana::AiPerceptionBlackboardStatus::invalid_snapshot};
    std::size_t physics_contact_count_{0U};
    std::size_t physics_trigger_overlap_count_{0U};
    std::size_t navigation_path_point_count_{0U};
    std::size_t last_perception_target_count_{0U};
    std::size_t last_perception_visible_count_{0U};
    std::uint32_t ticks_{0U};
    std::uint32_t physics_ticks_{0U};
    bool last_perception_has_primary_target_{false};
    bool blackboard_has_target_{false};
    bool blackboard_needs_move_{false};
    bool started_{false};
};

class Sample2DDesktopRuntimePackageGame final : public mirakana::GameApp {
  public:
    Sample2DDesktopRuntimePackageGame(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle,
                                      mirakana::RuntimeSceneRenderInstance scene,
                                      mirakana::AudioClipSampleData audio_samples,
                                      mirakana::runtime::RuntimeSpriteAnimationPayload sprite_animation,
                                      mirakana::runtime::RuntimeTilemapPayload tilemap)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)),
          audio_samples_(std::move(audio_samples)), sprite_animation_(std::move(sprite_animation)),
          tilemap_(std::move(tilemap)) {}

    void on_start(mirakana::EngineContext&) override {
        actions_.bind_key_axis("move_x", mirakana::Key::left, mirakana::Key::right);
        actions_.bind_key("jump", mirakana::Key::space);
        input_.press(mirakana::Key::right);
        input_.press(mirakana::Key::space);
        gameplay_systems_.start();

        if (scene_.scene.has_value()) {
            const auto validation = mirakana::validate_playable_2d_scene(*scene_.scene);
            validation_ok_ = validation.succeeded();
            package_scene_sprites_ = validation.visible_sprite_count;
        }

        const auto tilemap_sample = mirakana::runtime::sample_runtime_tilemap_visible_cells(tilemap_);
        tilemap_runtime_ok_ = tilemap_sample.succeeded;
        tilemap_layers_ = tilemap_sample.layer_count;
        tilemap_visible_layers_ = tilemap_sample.visible_layer_count;
        tilemap_tiles_ = tilemap_sample.tile_count;
        tilemap_non_empty_cells_ = tilemap_sample.non_empty_cell_count;
        tilemap_sampled_cells_ = tilemap_sample.sampled_cell_count;
        tilemap_diagnostics_ = tilemap_sample.diagnostic_count;

        ui_ok_ = build_hud();
        theme_.add(mirakana::UiThemeColor{.token = "hud.panel",
                                          .color = mirakana::Color{.r = 0.06F, .g = 0.08F, .b = 0.09F, .a = 1.0F}});

        audio_clip_registered_ = mixer_.register_clip(mirakana::AudioClipDesc{
            .clip = audio_samples_.clip,
            .sample_rate = audio_samples_.format.sample_rate,
            .channel_count = audio_samples_.format.channel_count,
            .frame_count = audio_samples_.frame_count,
            .sample_format = mirakana::AudioSampleFormat::float32,
            .streaming = false,
            .buffered_frame_count = audio_samples_.frame_count,
        });

        renderer_.set_clear_color(mirakana::Color{.r = 0.02F, .g = 0.03F, .b = 0.04F, .a = 1.0F});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        if (!scene_.scene.has_value()) {
            return false;
        }
        gameplay_systems_.tick();

        const mirakana::runtime::RuntimeInputStateView input_state{
            .keyboard = &input_,
            .pointer = nullptr,
            .gamepad = nullptr,
        };

        auto* player = scene_.scene->find_node(kPlayerNode);
        if (player == nullptr) {
            return false;
        }

        player->transform.position.x += actions_.axis_value("move_x", input_state);
        if (jump_voice_ == mirakana::null_audio_voice && actions_.action_down("jump", input_state)) {
            jump_voice_ = mixer_.play(
                mirakana::AudioVoiceDesc{.clip = audio_samples_.clip, .bus = "master", .gain = 1.0F, .looping = false});
        }

        const auto animation_result = mirakana::sample_and_apply_runtime_scene_render_sprite_animation(
            scene_, sprite_animation_, static_cast<float>(frames_) * 0.25F);
        sprite_animation_ok_ = sprite_animation_ok_ && animation_result.succeeded;
        sprite_animation_frames_sampled_ += animation_result.sampled_frame_count;
        sprite_animation_frames_applied_ += animation_result.applied_frame_count;
        sprite_animation_selected_frame_sum_ += animation_result.selected_frame_index;
        if (!animation_result.succeeded) {
            ++sprite_animation_diagnostics_;
        }

        const auto validation = mirakana::validate_playable_2d_scene(*scene_.scene);
        validation_ok_ = validation_ok_ && validation.succeeded();

        renderer_.begin_frame();
        const auto packet = mirakana::build_scene_render_packet(*scene_.scene);
        const auto batch_plan = mirakana::plan_scene_sprite_batches(packet);
        const auto scene_submit = mirakana::submit_scene_render_packet(
            renderer_, packet, mirakana::SceneRenderSubmitDesc{.material_palette = &scene_.material_palette});
        sprite_batch_plan_ok_ = sprite_batch_plan_ok_ && batch_plan.succeeded();
        sprite_batch_plan_sprites_ += batch_plan.sprite_count;
        sprite_batch_plan_textured_sprites_ += batch_plan.textured_sprite_count;
        sprite_batch_plan_draws_ += batch_plan.draw_count;
        sprite_batch_plan_texture_binds_ += batch_plan.texture_bind_count;
        sprite_batch_plan_diagnostics_ += batch_plan.diagnostics.size();
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
                .format = audio_samples_.format,
                .frame_count = 4,
                .device_frame = static_cast<std::uint64_t>(frames_) * 4U,
                .underrun_warning_threshold_frames = 4,
            },
            std::span<const mirakana::AudioClipSampleData>{&audio_samples_, 1});
        audio_commands_ += audio.plan.commands.size();
        audio_underruns_ += audio.plan.underruns.size();

        ++frames_;
        input_.begin_frame();

        if (throttle_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        return !input_.key_down(mirakana::Key::escape);
    }

    void on_stop(mirakana::EngineContext&) override {
        if (scene_.scene.has_value()) {
            if (const auto* player = scene_.scene->find_node(kPlayerNode); player != nullptr) {
                final_x_ = player->transform.position.x;
            }
        }
    }

    [[nodiscard]] bool passed(std::uint32_t expected_frames) const noexcept {
        return ui_ok_ && ui_text_updates_ok_ && validation_ok_ && audio_clip_registered_ &&
               jump_voice_ != mirakana::null_audio_voice && frames_ == expected_frames &&
               final_x_ == static_cast<float>(expected_frames) && primary_camera_seen_ &&
               scene_sprites_submitted_ == expected_frames && hud_boxes_submitted_ == expected_frames &&
               sprite_batch_plan_ok_ && sprite_batch_plan_sprites_ == expected_frames &&
               sprite_batch_plan_textured_sprites_ == expected_frames && sprite_batch_plan_draws_ == expected_frames &&
               sprite_batch_plan_texture_binds_ == expected_frames && sprite_batch_plan_diagnostics_ == 0 &&
               sprite_animation_ok_ && sprite_animation_frames_sampled_ == expected_frames &&
               sprite_animation_frames_applied_ == expected_frames && sprite_animation_diagnostics_ == 0 &&
               sprite_animation_selected_frame_sum_ > 0 && audio_commands_ == 1 && audio_underruns_ == 0 &&
               package_scene_sprites_ == 1 && tilemap_runtime_ok_ && tilemap_layers_ == 1 &&
               tilemap_visible_layers_ == 1 && tilemap_tiles_ == 2 && tilemap_non_empty_cells_ == 3 &&
               tilemap_sampled_cells_ == 3 && tilemap_diagnostics_ == 0 && gameplay_systems_.passed(expected_frames);
    }

    [[nodiscard]] std::uint32_t frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] float final_x() const noexcept {
        return final_x_;
    }

    [[nodiscard]] std::size_t scene_sprites_submitted() const noexcept {
        return scene_sprites_submitted_;
    }

    [[nodiscard]] std::size_t hud_boxes_submitted() const noexcept {
        return hud_boxes_submitted_;
    }

    [[nodiscard]] std::size_t audio_commands() const noexcept {
        return audio_commands_;
    }

    [[nodiscard]] std::size_t audio_underruns() const noexcept {
        return audio_underruns_;
    }

    [[nodiscard]] std::size_t package_scene_sprites() const noexcept {
        return package_scene_sprites_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_sprites() const noexcept {
        return sprite_batch_plan_sprites_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_textured_sprites() const noexcept {
        return sprite_batch_plan_textured_sprites_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_draws() const noexcept {
        return sprite_batch_plan_draws_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_texture_binds() const noexcept {
        return sprite_batch_plan_texture_binds_;
    }

    [[nodiscard]] std::size_t sprite_batch_plan_diagnostics() const noexcept {
        return sprite_batch_plan_diagnostics_;
    }

    [[nodiscard]] std::uint64_t sprite_animation_frames_sampled() const noexcept {
        return sprite_animation_frames_sampled_;
    }

    [[nodiscard]] std::uint64_t sprite_animation_frames_applied() const noexcept {
        return sprite_animation_frames_applied_;
    }

    [[nodiscard]] std::uint64_t sprite_animation_selected_frame_sum() const noexcept {
        return sprite_animation_selected_frame_sum_;
    }

    [[nodiscard]] std::size_t sprite_animation_diagnostics() const noexcept {
        return sprite_animation_diagnostics_;
    }

    [[nodiscard]] std::size_t tilemap_layers() const noexcept {
        return tilemap_layers_;
    }

    [[nodiscard]] std::size_t tilemap_visible_layers() const noexcept {
        return tilemap_visible_layers_;
    }

    [[nodiscard]] std::size_t tilemap_tiles() const noexcept {
        return tilemap_tiles_;
    }

    [[nodiscard]] std::size_t tilemap_non_empty_cells() const noexcept {
        return tilemap_non_empty_cells_;
    }

    [[nodiscard]] std::size_t tilemap_sampled_cells() const noexcept {
        return tilemap_sampled_cells_;
    }

    [[nodiscard]] std::size_t tilemap_diagnostics() const noexcept {
        return tilemap_diagnostics_;
    }

    [[nodiscard]] bool gameplay_systems_passed(std::uint32_t expected_frames) const {
        return gameplay_systems_.passed(expected_frames);
    }

    [[nodiscard]] Gameplay2DSystemsStatus gameplay_systems_status(std::uint32_t expected_frames) const {
        return gameplay_systems_.status(expected_frames);
    }

    [[nodiscard]] std::uint32_t gameplay_systems_ticks() const noexcept {
        return gameplay_systems_.ticks();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_physics_ticks() const noexcept {
        return gameplay_systems_.physics_ticks();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_bodies() const noexcept {
        return gameplay_systems_.physics_body_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_contacts() const noexcept {
        return gameplay_systems_.physics_contact_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_trigger_overlaps() const noexcept {
        return gameplay_systems_.physics_trigger_overlap_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_path_points() const noexcept {
        return gameplay_systems_.navigation_path_point_count();
    }

    [[nodiscard]] bool gameplay_systems_navigation_reached_destination() const noexcept {
        return gameplay_systems_.navigation_reached_destination();
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathStatus gameplay_systems_navigation_plan_status() const noexcept {
        return gameplay_systems_.navigation_plan_status();
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathDiagnostic
    gameplay_systems_navigation_plan_diagnostic() const noexcept {
        return gameplay_systems_.navigation_plan_diagnostic();
    }

    [[nodiscard]] mirakana::NavigationAgentStatus gameplay_systems_navigation_agent_status() const noexcept {
        return gameplay_systems_.navigation_agent_status();
    }

    [[nodiscard]] mirakana::AiPerceptionStatus gameplay_systems_perception_status() const noexcept {
        return gameplay_systems_.perception_status();
    }

    [[nodiscard]] std::size_t gameplay_systems_perception_targets() const noexcept {
        return gameplay_systems_.perception_target_count();
    }

    [[nodiscard]] bool gameplay_systems_perception_has_primary_target() const noexcept {
        return gameplay_systems_.perception_has_primary_target();
    }

    [[nodiscard]] std::size_t gameplay_systems_perception_visible_count() const noexcept {
        return gameplay_systems_.perception_visible_count();
    }

    [[nodiscard]] mirakana::AiPerceptionBlackboardStatus gameplay_systems_blackboard_status() const noexcept {
        return gameplay_systems_.blackboard_status();
    }

    [[nodiscard]] bool gameplay_systems_blackboard_has_target() const noexcept {
        return gameplay_systems_.blackboard_has_target();
    }

    [[nodiscard]] bool gameplay_systems_blackboard_needs_move() const noexcept {
        return gameplay_systems_.blackboard_needs_move();
    }

    [[nodiscard]] mirakana::BehaviorTreeStatus gameplay_systems_behavior_status() const noexcept {
        return gameplay_systems_.behavior_status();
    }

    [[nodiscard]] std::size_t gameplay_systems_behavior_nodes() const noexcept {
        return gameplay_systems_.behavior_visited_node_count();
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
        const auto score = std::string{"Score "} + std::to_string((frames_ + 1U) * 100U);
        ui_text_updates_ok_ =
            ui_text_updates_ok_ &&
            hud_.set_text(mirakana::ui::ElementId{"hud.score"},
                          mirakana::ui::TextContent{
                              .label = score, .localization_key = "hud.score", .font_family = "engine-default"});
    }

    mirakana::VirtualInput& input_;
    mirakana::IRenderer& renderer_;
    mirakana::runtime::RuntimeInputActionMap actions_;
    mirakana::RuntimeSceneRenderInstance scene_;
    mirakana::ui::UiDocument hud_;
    mirakana::UiRendererTheme theme_;
    mirakana::AudioMixer mixer_;
    Gameplay2DSystemsProbe gameplay_systems_;
    mirakana::AudioClipSampleData audio_samples_;
    mirakana::runtime::RuntimeSpriteAnimationPayload sprite_animation_;
    mirakana::runtime::RuntimeTilemapPayload tilemap_;
    mirakana::AudioVoiceId jump_voice_;
    std::size_t scene_sprites_submitted_{0};
    std::size_t hud_boxes_submitted_{0};
    std::size_t audio_commands_{0};
    std::size_t audio_underruns_{0};
    std::size_t package_scene_sprites_{0};
    std::uint64_t sprite_batch_plan_sprites_{0};
    std::uint64_t sprite_batch_plan_textured_sprites_{0};
    std::uint64_t sprite_batch_plan_draws_{0};
    std::uint64_t sprite_batch_plan_texture_binds_{0};
    std::size_t sprite_batch_plan_diagnostics_{0};
    std::uint64_t sprite_animation_frames_sampled_{0};
    std::uint64_t sprite_animation_frames_applied_{0};
    std::uint64_t sprite_animation_selected_frame_sum_{0};
    std::size_t sprite_animation_diagnostics_{0};
    std::size_t tilemap_layers_{0};
    std::size_t tilemap_visible_layers_{0};
    std::size_t tilemap_tiles_{0};
    std::size_t tilemap_non_empty_cells_{0};
    std::size_t tilemap_sampled_cells_{0};
    std::size_t tilemap_diagnostics_{0};
    std::uint32_t frames_{0};
    float final_x_{0.0F};
    bool ui_ok_{false};
    bool ui_text_updates_ok_{true};
    bool validation_ok_{false};
    bool audio_clip_registered_{false};
    bool primary_camera_seen_{false};
    bool sprite_batch_plan_ok_{true};
    bool sprite_animation_ok_{true};
    bool tilemap_runtime_ok_{false};
    bool throttle_{true};
};

[[nodiscard]] bool parse_positive_uint32(std::string_view text, std::uint32_t& value) noexcept {
    std::uint32_t parsed{};
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end || parsed == 0) {
        return false;
    }
    value = parsed;
    return true;
}

void print_usage() {
    std::cout << "sample_2d_desktop_runtime_package [--smoke] [--max-frames N] [--video-driver NAME] "
                 "[--require-config PATH] --require-scene-package PATH "
                 "[--require-d3d12-shaders] [--require-d3d12-renderer] "
                 "[--require-vulkan-shaders] [--require-vulkan-renderer] [--require-native-2d-sprites] "
                 "[--require-sprite-animation] [--require-tilemap-runtime-ux] [--require-gameplay-systems]\n";
}

[[nodiscard]] bool parse_args(int argc, char** argv, DesktopRuntimeOptions& options) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view arg{argv[index]};
        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
            return true;
        }
        if (arg == "--smoke") {
            options.smoke = true;
            continue;
        }
        if (arg == "--require-d3d12-shaders") {
            options.require_d3d12_shaders = true;
            continue;
        }
        if (arg == "--require-d3d12-renderer") {
            options.require_d3d12_renderer = true;
            options.require_d3d12_shaders = true;
            continue;
        }
        if (arg == "--require-vulkan-shaders") {
            options.require_vulkan_shaders = true;
            continue;
        }
        if (arg == "--require-vulkan-renderer") {
            options.require_vulkan_renderer = true;
            options.require_vulkan_shaders = true;
            continue;
        }
        if (arg == "--require-native-2d-sprites") {
            options.require_native_2d_sprites = true;
            continue;
        }
        if (arg == "--require-sprite-animation") {
            options.require_sprite_animation = true;
            continue;
        }
        if (arg == "--require-tilemap-runtime-ux") {
            options.require_tilemap_runtime_ux = true;
            continue;
        }
        if (arg == "--require-gameplay-systems") {
            options.require_gameplay_systems = true;
            continue;
        }
        if (arg == "--max-frames") {
            if (index + 1 >= argc || !parse_positive_uint32(argv[index + 1], options.max_frames)) {
                std::cerr << "--max-frames requires a positive integer\n";
                return false;
            }
            ++index;
            continue;
        }
        if (arg == "--video-driver") {
            if (index + 1 >= argc) {
                std::cerr << "--video-driver requires a driver name\n";
                return false;
            }
            options.video_driver_hint = argv[index + 1];
            ++index;
            continue;
        }
        if (arg == "--require-config") {
            if (index + 1 >= argc) {
                std::cerr << "--require-config requires a relative path\n";
                return false;
            }
            options.required_config_path = argv[index + 1];
            ++index;
            continue;
        }
        if (arg == "--require-scene-package") {
            if (index + 1 >= argc) {
                std::cerr << "--require-scene-package requires a relative path\n";
                return false;
            }
            options.required_scene_package_path = argv[index + 1];
            ++index;
            continue;
        }

        std::cerr << "unknown argument: " << arg << '\n';
        return false;
    }

    if (options.smoke) {
        if (options.max_frames == 0) {
            options.max_frames = 3;
        }
        if (options.video_driver_hint.empty()) {
            options.video_driver_hint = "dummy";
        }
        options.throttle = false;
    }
    if (options.require_native_2d_sprites) {
        if (options.require_vulkan_renderer) {
            options.require_vulkan_shaders = true;
        } else {
            options.require_d3d12_renderer = true;
            options.require_d3d12_shaders = true;
        }
    }
    return true;
}

[[nodiscard]] std::filesystem::path executable_directory(const char* executable_path) {
    try {
        if (executable_path != nullptr && !std::string_view{executable_path}.empty()) {
            const auto absolute_path = std::filesystem::absolute(std::filesystem::path{executable_path});
            if (absolute_path.has_parent_path()) {
                return absolute_path.parent_path();
            }
        }
        return std::filesystem::current_path();
    } catch (const std::exception&) {
        return std::filesystem::path{"."};
    }
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntime2dVertexShaderPath},
        .fragment_path = std::string{kRuntime2dFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_native_sprite_overlay_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntime2dNativeSpriteOverlayVertexShaderPath},
        .fragment_path = std::string{kRuntime2dNativeSpriteOverlayFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_vulkan_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntime2dVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntime2dVulkanFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_native_sprite_overlay_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntime2dVulkanNativeSpriteOverlayVertexShaderPath},
        .fragment_path = std::string{kRuntime2dVulkanNativeSpriteOverlayFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::SdlDesktopPresentationShaderBytecode
to_presentation_shader_bytecode(const mirakana::DesktopShaderBytecodeBlob& bytecode) noexcept {
    return mirakana::SdlDesktopPresentationShaderBytecode{
        .entry_point = bytecode.entry_point,
        .bytecode = std::span<const std::uint8_t>{bytecode.bytecode.data(), bytecode.bytecode.size()},
    };
}

[[nodiscard]] bool verify_required_config(const char* executable_path, std::string_view config_path) {
    if (config_path.empty()) {
        return true;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        if (!filesystem.exists(config_path)) {
            std::cerr << "required config was not found: " << config_path << '\n';
            return false;
        }

        const auto config_text = filesystem.read_text(config_path);
        if (config_text.empty()) {
            std::cerr << "required config is empty: " << config_path << '\n';
            return false;
        }
        if (!config_text.starts_with(kExpectedConfigFormat)) {
            std::cerr << "required config has unexpected format: " << config_path << '\n';
            return false;
        }
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required config '" << config_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
}

void print_package_failures(const std::vector<mirakana::runtime::RuntimeAssetPackageLoadFailure>& failures) {
    for (const auto& failure : failures) {
        std::cerr << "runtime package failure asset=" << failure.asset.value << " path=" << failure.path << ": "
                  << failure.diagnostic << '\n';
    }
}

void print_scene_failures(const std::vector<mirakana::RuntimeSceneRenderLoadFailure>& failures) {
    for (const auto& failure : failures) {
        std::cerr << "runtime scene failure asset=" << failure.asset.value << ": " << failure.diagnostic << '\n';
    }
}

[[nodiscard]] std::optional<mirakana::AudioClipSampleData>
make_audio_samples(const mirakana::runtime::RuntimeAudioPayload& payload) {
    if (payload.sample_format != mirakana::AudioSourceSampleFormat::float32 || payload.channel_count == 0 ||
        payload.samples.size() != payload.source_bytes || payload.samples.size() % sizeof(float) != 0) {
        return std::nullopt;
    }

    std::vector<float> samples(payload.samples.size() / sizeof(float));
    if (!samples.empty()) {
        std::memcpy(samples.data(), payload.samples.data(), payload.samples.size());
    }

    return mirakana::AudioClipSampleData{
        .clip = payload.asset,
        .format = mirakana::AudioDeviceFormat{.sample_rate = payload.sample_rate,
                                              .channel_count = payload.channel_count,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .frame_count = payload.frame_count,
        .interleaved_float_samples = std::move(samples),
    };
}

[[nodiscard]] bool
load_required_2d_package(const char* executable_path, std::string_view package_path,
                         std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package,
                         std::optional<mirakana::RuntimeSceneRenderInstance>& scene,
                         std::optional<mirakana::AudioClipSampleData>& audio_samples,
                         std::optional<mirakana::runtime::RuntimeSpriteAnimationPayload>& sprite_animation,
                         std::optional<mirakana::runtime::RuntimeTilemapPayload>& tilemap) {
    if (package_path.empty()) {
        std::cerr << "--require-scene-package is required for the 2D desktop runtime package proof\n";
        return false;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        if (!filesystem.exists(package_path)) {
            std::cerr << "required 2D package was not found: " << package_path << '\n';
            return false;
        }

        auto package_result =
            mirakana::runtime::load_runtime_asset_package(filesystem, mirakana::runtime::RuntimeAssetPackageDesc{
                                                                          .index_path = std::string{package_path},
                                                                          .content_root = {},
                                                                      });
        if (!package_result.succeeded()) {
            print_package_failures(package_result.failures);
            return false;
        }
        const auto diagnostics = mirakana::runtime::inspect_runtime_asset_package(package_result.package);
        if (!diagnostics.succeeded()) {
            for (const auto& diagnostic : diagnostics.diagnostics) {
                std::cerr << "runtime package diagnostic asset=" << diagnostic.asset.value
                          << " path=" << diagnostic.path << ": " << diagnostic.message << '\n';
            }
            return false;
        }

        auto instance =
            mirakana::instantiate_runtime_scene_render_data(package_result.package, packaged_scene_asset_id());
        if (!instance.succeeded()) {
            print_scene_failures(instance.failures);
            return false;
        }
        if (!instance.scene.has_value()) {
            std::cerr << "runtime 2D package did not produce a scene: " << package_path << '\n';
            return false;
        }
        const auto validation = mirakana::validate_playable_2d_scene(*instance.scene);
        if (!validation.succeeded() || instance.render_packet.sprites.empty()) {
            std::cerr << "runtime 2D package did not produce a playable orthographic sprite scene: " << package_path
                      << '\n';
            return false;
        }

        const auto* audio_record = package_result.package.find(packaged_audio_asset_id());
        if (audio_record == nullptr) {
            std::cerr << "runtime 2D package is missing the jump audio payload\n";
            return false;
        }
        const auto audio_payload = mirakana::runtime::runtime_audio_payload(*audio_record);
        if (!audio_payload.succeeded()) {
            std::cerr << "runtime 2D package audio payload failed: " << audio_payload.diagnostic << '\n';
            return false;
        }
        auto samples = make_audio_samples(audio_payload.payload);
        if (!samples.has_value()) {
            std::cerr << "runtime 2D package audio payload is not a supported float32 sample clip\n";
            return false;
        }

        const auto* animation_record = package_result.package.find(packaged_sprite_animation_asset_id());
        if (animation_record == nullptr) {
            std::cerr << "runtime 2D package is missing the player sprite animation payload\n";
            return false;
        }
        const auto animation_payload = mirakana::runtime::runtime_sprite_animation_payload(*animation_record);
        if (!animation_payload.succeeded()) {
            std::cerr << "runtime 2D package sprite animation payload failed: " << animation_payload.diagnostic << '\n';
            return false;
        }

        const auto* tilemap_record = package_result.package.find(packaged_tilemap_asset_id());
        if (tilemap_record == nullptr) {
            std::cerr << "runtime 2D package is missing the tilemap payload\n";
            return false;
        }
        const auto tilemap_payload = mirakana::runtime::runtime_tilemap_payload(*tilemap_record);
        if (!tilemap_payload.succeeded()) {
            std::cerr << "runtime 2D package tilemap payload failed: " << tilemap_payload.diagnostic << '\n';
            return false;
        }

        scene = std::move(instance);
        audio_samples = std::move(samples);
        sprite_animation = animation_payload.payload;
        tilemap = tilemap_payload.payload;
        runtime_package = std::move(package_result.package);
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required 2D package '" << package_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] std::string_view status_name(mirakana::DesktopRunStatus status) noexcept {
    switch (status) {
    case mirakana::DesktopRunStatus::completed:
        return "completed";
    case mirakana::DesktopRunStatus::stopped_by_app:
        return "stopped_by_app";
    case mirakana::DesktopRunStatus::window_closed:
        return "window_closed";
    case mirakana::DesktopRunStatus::lifecycle_quit:
        return "lifecycle_quit";
    }
    return "unknown";
}

void print_presentation_report(std::string_view prefix, const mirakana::SdlDesktopGameHost& host) {
    const auto report = host.presentation_report();
    std::cout << prefix << " presentation_report=requested="
              << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
              << " selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " fallback=" << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " diagnostics=" << report.diagnostics_count << " backend_reports=" << report.backend_reports_count
              << " scene_gpu_status="
              << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " native_2d_sprites_status="
              << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
              << " native_2d_sprites_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
              << " native_2d_texture_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
              << " renderer_frames_finished=" << report.renderer_stats.frames_finished << '\n';
    for (const auto& backend_report : host.presentation_backend_reports()) {
        std::cout << prefix << " presentation_backend_report="
                  << mirakana::sdl_desktop_presentation_backend_name(backend_report.backend) << ":"
                  << mirakana::sdl_desktop_presentation_backend_report_status_name(backend_report.status) << ":"
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(backend_report.fallback_reason) << ": "
                  << backend_report.message << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    DesktopRuntimeOptions options;
    if (!parse_args(argc, argv, options)) {
        print_usage();
        return 2;
    }
    if (options.show_help) {
        print_usage();
        return 0;
    }
    if (!verify_required_config(argc > 0 ? argv[0] : nullptr, options.required_config_path)) {
        return 4;
    }

    std::optional<mirakana::runtime::RuntimeAssetPackage> runtime_package;
    std::optional<mirakana::RuntimeSceneRenderInstance> packaged_scene;
    std::optional<mirakana::AudioClipSampleData> audio_samples;
    std::optional<mirakana::runtime::RuntimeSpriteAnimationPayload> sprite_animation;
    std::optional<mirakana::runtime::RuntimeTilemapPayload> tilemap;
    if (!load_required_2d_package(argc > 0 ? argv[0] : nullptr, options.required_scene_package_path, runtime_package,
                                  packaged_scene, audio_samples, sprite_animation, tilemap)) {
        return 4;
    }

    auto shader_bytecode = load_packaged_d3d12_shaders(argc > 0 ? argv[0] : nullptr);
    if (!shader_bytecode.ready()) {
        std::cout << "sample_2d_desktop_runtime_package shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(shader_bytecode.status) << ": "
                  << shader_bytecode.diagnostic << '\n';
        if (options.require_d3d12_shaders) {
            return 5;
        }
    }
    auto native_sprite_overlay_shader_bytecode =
        load_packaged_d3d12_native_sprite_overlay_shaders(argc > 0 ? argv[0] : nullptr);
    if (!native_sprite_overlay_shader_bytecode.ready() && options.require_native_2d_sprites &&
        !options.require_vulkan_renderer) {
        std::cout << "sample_2d_desktop_runtime_package native_2d_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(native_sprite_overlay_shader_bytecode.status)
                  << ": " << native_sprite_overlay_shader_bytecode.diagnostic << '\n';
        return 9;
    }

    auto vulkan_shader_bytecode = load_packaged_vulkan_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shader_bytecode.ready() && options.require_vulkan_shaders) {
        std::cout << "sample_2d_desktop_runtime_package vulkan_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shader_bytecode.status) << ": "
                  << vulkan_shader_bytecode.diagnostic << '\n';
        return 7;
    }
    auto vulkan_native_sprite_overlay_shader_bytecode =
        load_packaged_vulkan_native_sprite_overlay_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_native_sprite_overlay_shader_bytecode.ready() && options.require_native_2d_sprites &&
        options.require_vulkan_renderer) {
        std::cout << "sample_2d_desktop_runtime_package vulkan_native_2d_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(
                         vulkan_native_sprite_overlay_shader_bytecode.status)
                  << ": " << vulkan_native_sprite_overlay_shader_bytecode.diagnostic << '\n';
        return 9;
    }

    std::optional<mirakana::SdlDesktopPresentationD3d12RendererDesc> d3d12_renderer;
    if (shader_bytecode.ready()) {
        d3d12_renderer.emplace(mirakana::SdlDesktopPresentationD3d12RendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(shader_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(shader_bytecode.fragment_shader),
            .native_sprite_overlay_vertex_shader =
                native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(native_sprite_overlay_shader_bytecode.vertex_shader)
                    : mirakana::SdlDesktopPresentationShaderBytecode{},
            .native_sprite_overlay_fragment_shader =
                native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(native_sprite_overlay_shader_bytecode.fragment_shader)
                    : mirakana::SdlDesktopPresentationShaderBytecode{},
            .native_sprite_overlay_package = runtime_package.has_value() ? &*runtime_package : nullptr,
            .native_sprite_overlay_atlas_asset = packaged_sprite_texture_asset_id(),
            .enable_native_sprite_overlay = options.require_native_2d_sprites && !options.require_vulkan_renderer,
            .enable_native_sprite_overlay_textures =
                options.require_native_2d_sprites && !options.require_vulkan_renderer,
        });
    }

    std::optional<mirakana::SdlDesktopPresentationVulkanRendererDesc> vulkan_renderer;
    if (vulkan_shader_bytecode.ready()) {
        vulkan_renderer.emplace(mirakana::SdlDesktopPresentationVulkanRendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(vulkan_shader_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(vulkan_shader_bytecode.fragment_shader),
            .native_sprite_overlay_vertex_shader =
                vulkan_native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(vulkan_native_sprite_overlay_shader_bytecode.vertex_shader)
                    : mirakana::SdlDesktopPresentationShaderBytecode{},
            .native_sprite_overlay_fragment_shader =
                vulkan_native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(vulkan_native_sprite_overlay_shader_bytecode.fragment_shader)
                    : mirakana::SdlDesktopPresentationShaderBytecode{},
            .native_sprite_overlay_package = runtime_package.has_value() ? &*runtime_package : nullptr,
            .native_sprite_overlay_atlas_asset = packaged_sprite_texture_asset_id(),
            .enable_native_sprite_overlay = options.require_native_2d_sprites && options.require_vulkan_renderer,
            .enable_native_sprite_overlay_textures =
                options.require_native_2d_sprites && options.require_vulkan_renderer,
        });
    }

    mirakana::SdlDesktopGameHostDesc host_desc{
        .title = "Sample 2D Desktop Runtime Package",
        .extent = mirakana::WindowExtent{.width = 960, .height = 540},
        .video_driver_hint = options.video_driver_hint,
        .prefer_vulkan = options.require_vulkan_renderer,
    };
    if (d3d12_renderer.has_value()) {
        host_desc.d3d12_renderer = &*d3d12_renderer;
    }
    if (vulkan_renderer.has_value()) {
        host_desc.vulkan_renderer = &*vulkan_renderer;
    }

    mirakana::SdlDesktopGameHost host(host_desc);
    if (options.require_d3d12_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::d3d12) {
        std::cout << "sample_2d_desktop_runtime_package required_d3d12_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_2d_desktop_runtime_package", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_2d_desktop_runtime_package presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        return 6;
    }
    if (options.require_vulkan_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::vulkan) {
        std::cout << "sample_2d_desktop_runtime_package required_vulkan_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_2d_desktop_runtime_package", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_2d_desktop_runtime_package presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        return 8;
    }

    Sample2DDesktopRuntimePackageGame game(host.input(), host.renderer(), options.throttle, std::move(*packaged_scene),
                                           std::move(*audio_samples), std::move(*sprite_animation),
                                           std::move(*tilemap));
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();

    const auto package_records = runtime_package.has_value() ? runtime_package->records().size() : 0U;
    std::cout
        << "sample_2d_desktop_runtime_package status=" << status_name(result.status)
        << " renderer=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_requested=" << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
        << " presentation_selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_fallback=" << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
        << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
        << " presentation_backend_reports=" << report.backend_reports_count
        << " presentation_diagnostics=" << report.diagnostics_count << " scene_gpu_status="
        << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
        << " native_2d_sprites_requested=" << (report.native_ui_overlay_requested ? 1 : 0)
        << " native_2d_sprites_status="
        << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
        << " native_2d_sprites_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
        << " native_2d_sprites_submitted=" << report.native_ui_overlay_sprites_submitted
        << " native_2d_textured_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
        << " native_2d_texture_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
        << " native_2d_texture_binds=" << report.native_ui_texture_overlay_texture_binds
        << " native_2d_draws=" << report.native_ui_overlay_draws
        << " native_2d_textured_draws=" << report.native_ui_texture_overlay_draws
        << " native_2d_sprite_batches_executed=" << report.renderer_stats.native_sprite_batches_executed
        << " native_2d_sprite_batch_sprites_executed=" << report.renderer_stats.native_sprite_batch_sprites_executed
        << " native_2d_sprite_batch_textured_sprites_executed="
        << report.renderer_stats.native_sprite_batch_textured_sprites_executed
        << " native_2d_sprite_batch_texture_binds=" << report.renderer_stats.native_sprite_batch_texture_binds
        << " frames=" << result.frames_run << " game_frames=" << game.frames() << " final_x=" << game.final_x()
        << " scene_sprites=" << game.scene_sprites_submitted()
        << " sprite_batch_plan_sprites=" << game.sprite_batch_plan_sprites()
        << " sprite_batch_plan_textured_sprites=" << game.sprite_batch_plan_textured_sprites()
        << " sprite_batch_plan_draws=" << game.sprite_batch_plan_draws()
        << " sprite_batch_plan_texture_binds=" << game.sprite_batch_plan_texture_binds()
        << " sprite_batch_plan_diagnostics=" << game.sprite_batch_plan_diagnostics()
        << " sprite_animation_frames_sampled=" << game.sprite_animation_frames_sampled()
        << " sprite_animation_frames_applied=" << game.sprite_animation_frames_applied()
        << " sprite_animation_selected_frame_sum=" << game.sprite_animation_selected_frame_sum()
        << " sprite_animation_diagnostics=" << game.sprite_animation_diagnostics()
        << " tilemap_layers=" << game.tilemap_layers() << " tilemap_visible_layers=" << game.tilemap_visible_layers()
        << " tilemap_tiles=" << game.tilemap_tiles() << " tilemap_non_empty_cells=" << game.tilemap_non_empty_cells()
        << " tilemap_cells_sampled=" << game.tilemap_sampled_cells()
        << " tilemap_diagnostics=" << game.tilemap_diagnostics() << " gameplay_systems_status="
        << gameplay_2d_systems_status_name(game.gameplay_systems_status(options.max_frames))
        << " gameplay_systems_ready=" << (game.gameplay_systems_passed(options.max_frames) ? 1 : 0)
        << " gameplay_systems_ticks=" << game.gameplay_systems_ticks()
        << " gameplay_systems_physics_ticks=" << game.gameplay_systems_physics_ticks()
        << " gameplay_systems_physics_bodies=" << game.gameplay_systems_physics_bodies()
        << " gameplay_systems_physics_contacts=" << game.gameplay_systems_physics_contacts()
        << " gameplay_systems_physics_trigger_overlaps=" << game.gameplay_systems_physics_trigger_overlaps()
        << " gameplay_systems_navigation_path_points=" << game.gameplay_systems_navigation_path_points()
        << " gameplay_systems_navigation_reached=" << (game.gameplay_systems_navigation_reached_destination() ? 1 : 0)
        << " gameplay_systems_navigation_plan_status="
        << navigation_grid_agent_path_status_name(game.gameplay_systems_navigation_plan_status())
        << " gameplay_systems_navigation_plan_diagnostic="
        << navigation_grid_agent_path_diagnostic_name(game.gameplay_systems_navigation_plan_diagnostic())
        << " gameplay_systems_navigation_agent_status="
        << navigation_agent_status_name(game.gameplay_systems_navigation_agent_status())
        << " gameplay_systems_perception_status="
        << ai_perception_status_name(game.gameplay_systems_perception_status())
        << " gameplay_systems_perception_targets=" << game.gameplay_systems_perception_targets()
        << " gameplay_systems_perception_has_primary_target="
        << (game.gameplay_systems_perception_has_primary_target() ? 1 : 0)
        << " gameplay_systems_perception_visible_count=" << game.gameplay_systems_perception_visible_count()
        << " gameplay_systems_blackboard_status="
        << ai_perception_blackboard_status_name(game.gameplay_systems_blackboard_status())
        << " gameplay_systems_blackboard_has_target=" << (game.gameplay_systems_blackboard_has_target() ? 1 : 0)
        << " gameplay_systems_blackboard_needs_move=" << (game.gameplay_systems_blackboard_needs_move() ? 1 : 0)
        << " gameplay_systems_behavior_status=" << behavior_tree_status_name(game.gameplay_systems_behavior_status())
        << " gameplay_systems_behavior_nodes=" << game.gameplay_systems_behavior_nodes()
        << " hud_boxes=" << game.hud_boxes_submitted() << " audio_commands=" << game.audio_commands()
        << " audio_underruns=" << game.audio_underruns() << " package_records=" << package_records
        << " package_scene_sprites=" << game.package_scene_sprites() << '\n';
    print_presentation_report("sample_2d_desktop_runtime_package", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "sample_2d_desktop_runtime_package presentation_diagnostic="
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                  << diagnostic.message << '\n';
    }

    if (options.require_native_2d_sprites &&
        (!report.native_ui_overlay_requested || !report.native_ui_overlay_ready ||
         !report.native_ui_texture_overlay_atlas_ready || report.native_ui_overlay_sprites_submitted == 0 ||
         report.native_ui_texture_overlay_sprites_submitted == 0 ||
         report.native_ui_texture_overlay_texture_binds == 0 || report.native_ui_overlay_draws == 0 ||
         report.native_ui_texture_overlay_draws == 0 || report.renderer_stats.native_sprite_batches_executed == 0 ||
         report.renderer_stats.native_sprite_batch_sprites_executed == 0 ||
         report.renderer_stats.native_sprite_batch_textured_sprites_executed == 0 ||
         report.renderer_stats.native_sprite_batch_texture_binds == 0)) {
        std::cout << "sample_2d_desktop_runtime_package required_native_2d_sprites_unavailable renderer="
                  << host.presentation_backend_name() << " native_2d_sprites_status="
                  << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
                  << " native_2d_sprites_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
                  << " native_2d_textured_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
                  << " native_2d_texture_binds=" << report.native_ui_texture_overlay_texture_binds
                  << " native_2d_sprite_batches_executed=" << report.renderer_stats.native_sprite_batches_executed
                  << '\n';
        return 9;
    }

    if (options.require_sprite_animation &&
        (game.sprite_animation_frames_sampled() == 0 || game.sprite_animation_frames_applied() == 0 ||
         game.sprite_animation_selected_frame_sum() == 0 || game.sprite_animation_diagnostics() != 0)) {
        std::cout << "sample_2d_desktop_runtime_package required_sprite_animation_unavailable"
                  << " sprite_animation_frames_sampled=" << game.sprite_animation_frames_sampled()
                  << " sprite_animation_frames_applied=" << game.sprite_animation_frames_applied()
                  << " sprite_animation_diagnostics=" << game.sprite_animation_diagnostics() << '\n';
        return 10;
    }

    if (options.require_tilemap_runtime_ux &&
        (game.tilemap_layers() == 0 || game.tilemap_visible_layers() == 0 || game.tilemap_tiles() == 0 ||
         game.tilemap_non_empty_cells() == 0 || game.tilemap_sampled_cells() == 0 || game.tilemap_diagnostics() != 0)) {
        std::cout << "sample_2d_desktop_runtime_package required_tilemap_runtime_ux_unavailable"
                  << " tilemap_layers=" << game.tilemap_layers()
                  << " tilemap_visible_layers=" << game.tilemap_visible_layers()
                  << " tilemap_cells_sampled=" << game.tilemap_sampled_cells()
                  << " tilemap_diagnostics=" << game.tilemap_diagnostics() << '\n';
        return 11;
    }

    if (options.require_gameplay_systems && !game.gameplay_systems_passed(options.max_frames)) {
        std::cout << "sample_2d_desktop_runtime_package required_gameplay_systems_unavailable"
                  << " gameplay_systems_status="
                  << gameplay_2d_systems_status_name(game.gameplay_systems_status(options.max_frames))
                  << " gameplay_systems_physics_contacts=" << game.gameplay_systems_physics_contacts()
                  << " gameplay_systems_navigation_plan_status="
                  << navigation_grid_agent_path_status_name(game.gameplay_systems_navigation_plan_status())
                  << " gameplay_systems_behavior_status="
                  << behavior_tree_status_name(game.gameplay_systems_behavior_status()) << '\n';
        return 12;
    }

    if (options.smoke &&
        (result.status != mirakana::DesktopRunStatus::completed || result.frames_run != options.max_frames ||
         !game.passed(options.max_frames) || package_records != 6U)) {
        return 3;
    }
    return 0;
}
