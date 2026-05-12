// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ai/behavior_tree.hpp"
#include "mirakana/ai/perception.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/navigation/navigation_agent.hpp"
#include "mirakana/navigation/navigation_grid.hpp"
#include "mirakana/navigation/navigation_path_planner.hpp"

#include <cmath>
#include <iostream>
#include <span>
#include <vector>

namespace {

constexpr mirakana::BehaviorTreeNodeId k_root_node{1};
constexpr mirakana::BehaviorTreeNodeId k_has_path_node{2};
constexpr mirakana::BehaviorTreeNodeId k_needs_move_node{3};
constexpr mirakana::BehaviorTreeNodeId k_move_action_node{4};
constexpr const char* k_has_path_key{"has_path"};
constexpr const char* k_needs_move_key{"needs_move"};
constexpr const char* k_target_id_key{"target_id"};
constexpr const char* k_target_distance_key{"target_distance"};
constexpr const char* k_visible_targets_key{"visible_targets"};
constexpr const char* k_audible_targets_key{"audible_targets"};
constexpr const char* k_target_state_key{"target_state"};

[[nodiscard]] bool nearly_equal(float lhs, float rhs) noexcept {
    return std::abs(lhs - rhs) < 0.0001F;
}

[[nodiscard]] const char* status_name(mirakana::NavigationAgentStatus status) noexcept {
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

class SampleAiNavigationGame final : public mirakana::GameApp {
  public:
    void on_start(mirakana::EngineContext& context) override {
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
        path_plan_status_ = plan.status;
        path_status_ = plan.path_result.status;
        smoothing_status_ = plan.smoothing_result.status;
        point_path_status_ = plan.point_path_result.status;
        smoothed_path_count_ = plan.planned_grid_point_count;
        agent_ = plan.agent_state;

        context.logger.write(mirakana::LogRecord{
            .level = mirakana::LogLevel::info, .category = "sample", .message = "AI navigation sample started"});
    }

    bool on_update(mirakana::EngineContext& /*context*/, double delta_seconds) override {
        mirakana::BehaviorTreeBlackboard blackboard;
        const bool has_path = path_status_ == mirakana::NavigationPathStatus::success &&
                              path_plan_status_ == mirakana::NavigationGridAgentPathStatus::ready &&
                              smoothing_status_ == mirakana::NavigationGridPathSmoothingStatus::success &&
                              point_path_status_ == mirakana::NavigationPointPathBuildStatus::success;
        const bool needs_move = agent_.status == mirakana::NavigationAgentStatus::moving;
        const std::vector<mirakana::AiPerceptionTarget2D> route_targets =
            has_path && !agent_.path.empty()
                ? std::vector<mirakana::AiPerceptionTarget2D>{mirakana::AiPerceptionTarget2D{
                      .id = 1U,
                      .position = mirakana::AiPerceptionPoint2{.x = agent_.path.back().x, .y = agent_.path.back().y},
                      .radius = 0.0F,
                      .sight_enabled = true,
                      .hearing_enabled = false,
                      .sound_radius = 0.0F,
                  }}
                : std::vector<mirakana::AiPerceptionTarget2D>{};
        const auto perception = mirakana::build_ai_perception_snapshot_2d(mirakana::AiPerceptionRequest2D{
            .agent = mirakana::AiPerceptionAgent2D{.id = 100U,
                                                   .position = mirakana::AiPerceptionPoint2{.x = agent_.position.x,
                                                                                            .y = agent_.position.y},
                                                   .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                                   .sight_range = 16.0F,
                                                   .field_of_view_radians = 6.28318530718F,
                                                   .hearing_range = 0.0F},
            .targets = std::span<const mirakana::AiPerceptionTarget2D>{route_targets},
        });
        const auto perception_blackboard = mirakana::write_ai_perception_blackboard(
            perception,
            mirakana::AiPerceptionBlackboardKeys{.has_target_key = k_has_path_key,
                                                 .target_id_key = k_target_id_key,
                                                 .target_distance_key = k_target_distance_key,
                                                 .visible_count_key = k_visible_targets_key,
                                                 .audible_count_key = k_audible_targets_key,
                                                 .target_state_key = k_target_state_key},
            blackboard);
        const bool needs_move_set =
            blackboard.set(k_needs_move_key, mirakana::make_behavior_tree_blackboard_bool(needs_move));
        if (perception_blackboard.status != mirakana::AiPerceptionBlackboardStatus::ready || !needs_move_set) {
            return false;
        }
        last_perception_target_count_ = perception.targets.size();

        const auto conditions = std::vector<mirakana::BehaviorTreeBlackboardCondition>{
            mirakana::BehaviorTreeBlackboardCondition{.node_id = k_has_path_node,
                                                      .key = k_has_path_key,
                                                      .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                      .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
            mirakana::BehaviorTreeBlackboardCondition{.node_id = k_needs_move_node,
                                                      .key = k_needs_move_key,
                                                      .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                      .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
        };
        last_blackboard_condition_count_ = conditions.size();

        const auto leaf_results = std::vector<mirakana::BehaviorTreeLeafResult>{
            mirakana::BehaviorTreeLeafResult{.node_id = k_move_action_node,
                                             .status = mirakana::BehaviorTreeStatus::success},
        };

        last_tree_result_ = mirakana::evaluate_behavior_tree(
            mirakana::BehaviorTreeDesc{
                .root_id = k_root_node,
                .nodes =
                    {
                        mirakana::BehaviorTreeNodeDesc{
                            .id = k_root_node,
                            .kind = mirakana::BehaviorTreeNodeKind::sequence,
                            .children = {k_has_path_node, k_needs_move_node, k_move_action_node}},
                        mirakana::BehaviorTreeNodeDesc{
                            .id = k_has_path_node, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
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

        if (last_tree_result_.status == mirakana::BehaviorTreeStatus::success) {
            const auto update = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
                .state = agent_,
                .config = mirakana::NavigationAgentConfig{.max_speed = 2.0F,
                                                          .slowing_radius = 1.0F,
                                                          .arrival_radius = 0.001F},
                .delta_seconds = static_cast<float>(delta_seconds),
            });
            agent_ = update.state;
            last_step_distance_ = update.step_distance;
        }

        ++frames_;
        return frames_ < 8 && agent_.status != mirakana::NavigationAgentStatus::reached_destination;
    }

    void on_stop(mirakana::EngineContext& context) override {
        context.logger.write(mirakana::LogRecord{
            .level = mirakana::LogLevel::info, .category = "sample", .message = "AI navigation sample stopped"});
    }

    [[nodiscard]] int frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] mirakana::NavigationAgentState agent() const {
        return agent_;
    }

    [[nodiscard]] mirakana::BehaviorTreeTickResult last_tree_result() const {
        return last_tree_result_;
    }

    [[nodiscard]] float last_step_distance() const noexcept {
        return last_step_distance_;
    }

    [[nodiscard]] std::size_t smoothed_path_count() const noexcept {
        return smoothed_path_count_;
    }

    [[nodiscard]] std::size_t blackboard_condition_count() const noexcept {
        return last_blackboard_condition_count_;
    }

    [[nodiscard]] std::size_t perception_target_count() const noexcept {
        return last_perception_target_count_;
    }

  private:
    mirakana::NavigationAgentState agent_;
    mirakana::BehaviorTreeTickResult last_tree_result_;
    mirakana::NavigationGridAgentPathStatus path_plan_status_{mirakana::NavigationGridAgentPathStatus::invalid_request};
    mirakana::NavigationPathStatus path_status_{mirakana::NavigationPathStatus::no_path};
    mirakana::NavigationGridPathSmoothingStatus smoothing_status_{
        mirakana::NavigationGridPathSmoothingStatus::invalid_source_path};
    mirakana::NavigationPointPathBuildStatus point_path_status_{
        mirakana::NavigationPointPathBuildStatus::invalid_mapping};
    float last_step_distance_{0.0F};
    std::size_t smoothed_path_count_{0U};
    std::size_t last_blackboard_condition_count_{0U};
    std::size_t last_perception_target_count_{0U};
    int frames_{0};
};

} // namespace

int main() {
    mirakana::RingBufferLogger logger(16);
    mirakana::Registry registry;
    mirakana::HeadlessRunner runner(logger, registry);
    SampleAiNavigationGame game;

    const auto result = runner.run(game, mirakana::RunConfig{.max_frames = 16, .fixed_delta_seconds = 0.5});
    const auto agent = game.agent();
    const auto tree = game.last_tree_result();

    std::cout << "sample_ai_navigation frames=" << result.frames_run << " status=" << status_name(agent.status)
              << " x=" << agent.position.x << " y=" << agent.position.y << " visited=" << tree.visited_nodes.size()
              << " smoothed_points=" << game.smoothed_path_count()
              << " blackboard_conditions=" << game.blackboard_condition_count()
              << " perception_targets=" << game.perception_target_count() << '\n';

    return result.status == mirakana::RunStatus::stopped_by_app && result.frames_run == 6 && game.frames() == 6 &&
                   agent.status == mirakana::NavigationAgentStatus::reached_destination &&
                   nearly_equal(agent.position.x, 3.5F) && nearly_equal(agent.position.y, 3.5F) &&
                   nearly_equal(game.last_step_distance(), 1.0F) &&
                   tree.status == mirakana::BehaviorTreeStatus::success && tree.visited_nodes.size() == 4 &&
                   game.smoothed_path_count() == 3U && game.blackboard_condition_count() == 2U &&
                   game.perception_target_count() == 1U && registry.living_count() == 0
               ? 0
               : 1;
}
