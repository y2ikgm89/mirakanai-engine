// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ai/behavior_tree.hpp"
#include "mirakana/ai/perception.hpp"

#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace {
// Writes an invalid `BehaviorTreeBlackboardValueKind` bit-pattern for negative tests without
// `static_cast` from an out-of-range integer to the enum (clang-analyzer EnumCastOutOfRange).
void corrupt_blackboard_value_kind(mirakana::BehaviorTreeBlackboardValue& value) {
    constexpr auto k_invalid_raw = static_cast<std::underlying_type_t<mirakana::BehaviorTreeBlackboardValueKind>>(999);
    static_assert(sizeof(mirakana::BehaviorTreeBlackboardValueKind) <= sizeof(k_invalid_raw),
                  "unexpected enum object size");
    std::memcpy(std::addressof(value.kind), &k_invalid_raw, sizeof(value.kind));
}
} // namespace

MK_TEST("behavior tree sequence propagates child status deterministically") {
    const mirakana::BehaviorTreeDesc tree{
        .root_id = 1,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{
                    .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2, 3}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{.id = 3, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };

    const auto success = mirakana::evaluate_behavior_tree(
        tree, std::vector<mirakana::BehaviorTreeLeafResult>{
                  mirakana::BehaviorTreeLeafResult{.node_id = 2, .status = mirakana::BehaviorTreeStatus::success},
                  mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::success},
              });
    MK_REQUIRE(success.status == mirakana::BehaviorTreeStatus::success);
    MK_REQUIRE((success.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2, 3}));

    const auto failure = mirakana::evaluate_behavior_tree(
        tree, std::vector<mirakana::BehaviorTreeLeafResult>{
                  mirakana::BehaviorTreeLeafResult{.node_id = 2, .status = mirakana::BehaviorTreeStatus::failure},
                  mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::success},
              });
    MK_REQUIRE(failure.status == mirakana::BehaviorTreeStatus::failure);
    MK_REQUIRE((failure.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2}));

    const auto running = mirakana::evaluate_behavior_tree(
        tree, std::vector<mirakana::BehaviorTreeLeafResult>{
                  mirakana::BehaviorTreeLeafResult{.node_id = 2, .status = mirakana::BehaviorTreeStatus::success},
                  mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::running},
              });
    MK_REQUIRE(running.status == mirakana::BehaviorTreeStatus::running);
    MK_REQUIRE((running.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2, 3}));
}

MK_TEST("behavior tree selector propagates child status deterministically") {
    const mirakana::BehaviorTreeDesc tree{
        .root_id = 1,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{
                    .id = 1, .kind = mirakana::BehaviorTreeNodeKind::selector, .children = {2, 3}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{.id = 3, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };

    const auto success = mirakana::evaluate_behavior_tree(
        tree, std::vector<mirakana::BehaviorTreeLeafResult>{
                  mirakana::BehaviorTreeLeafResult{.node_id = 2, .status = mirakana::BehaviorTreeStatus::success},
                  mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::failure},
              });
    MK_REQUIRE(success.status == mirakana::BehaviorTreeStatus::success);
    MK_REQUIRE((success.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2}));

    const auto running = mirakana::evaluate_behavior_tree(
        tree, std::vector<mirakana::BehaviorTreeLeafResult>{
                  mirakana::BehaviorTreeLeafResult{.node_id = 2, .status = mirakana::BehaviorTreeStatus::failure},
                  mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::running},
              });
    MK_REQUIRE(running.status == mirakana::BehaviorTreeStatus::running);
    MK_REQUIRE((running.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2, 3}));

    const auto failure = mirakana::evaluate_behavior_tree(
        tree, std::vector<mirakana::BehaviorTreeLeafResult>{
                  mirakana::BehaviorTreeLeafResult{.node_id = 2, .status = mirakana::BehaviorTreeStatus::failure},
                  mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::failure},
              });
    MK_REQUIRE(failure.status == mirakana::BehaviorTreeStatus::failure);
    MK_REQUIRE((failure.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2, 3}));
}

MK_TEST("behavior tree replay returns stable status diagnostics and trace") {
    const mirakana::BehaviorTreeDesc tree{
        .root_id = 1,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{
                    .id = 1, .kind = mirakana::BehaviorTreeNodeKind::selector, .children = {2, 3}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{.id = 3, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };
    const std::vector<mirakana::BehaviorTreeLeafResult> leaf_results{
        mirakana::BehaviorTreeLeafResult{.node_id = 2, .status = mirakana::BehaviorTreeStatus::failure},
        mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::running},
    };

    const auto first = mirakana::evaluate_behavior_tree(tree, leaf_results);
    const auto second = mirakana::evaluate_behavior_tree(tree, leaf_results);

    MK_REQUIRE(first.status == second.status);
    MK_REQUIRE(first.diagnostic.code == second.diagnostic.code);
    MK_REQUIRE(first.diagnostic.node_id == second.diagnostic.node_id);
    MK_REQUIRE(first.diagnostic.referenced_node_id == second.diagnostic.referenced_node_id);
    MK_REQUIRE(first.visited_nodes == second.visited_nodes);
}

MK_TEST("behavior tree rejects invalid descriptions deterministically") {
    const auto missing_root = mirakana::evaluate_behavior_tree(
        mirakana::BehaviorTreeDesc{.root_id = 99, .nodes = std::vector<mirakana::BehaviorTreeNodeDesc>{}}, {});
    MK_REQUIRE(missing_root.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(missing_root.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::missing_root);

    const auto duplicate = mirakana::evaluate_behavior_tree(
        mirakana::BehaviorTreeDesc{
            .root_id = 1,
            .nodes =
                std::vector<mirakana::BehaviorTreeNodeDesc>{
                    mirakana::BehaviorTreeNodeDesc{
                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
                    mirakana::BehaviorTreeNodeDesc{
                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                },
        },
        {});
    MK_REQUIRE(duplicate.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(duplicate.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::duplicate_node_id);

    const auto missing_child = mirakana::evaluate_behavior_tree(
        mirakana::BehaviorTreeDesc{
            .root_id = 1,
            .nodes =
                std::vector<mirakana::BehaviorTreeNodeDesc>{
                    mirakana::BehaviorTreeNodeDesc{
                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2}},
                },
        },
        {});
    MK_REQUIRE(missing_child.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(missing_child.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::missing_child);

    const auto empty_composite = mirakana::evaluate_behavior_tree(
        mirakana::BehaviorTreeDesc{
            .root_id = 1,
            .nodes =
                std::vector<mirakana::BehaviorTreeNodeDesc>{
                    mirakana::BehaviorTreeNodeDesc{
                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::selector, .children = {}},
                },
        },
        {});
    MK_REQUIRE(empty_composite.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(empty_composite.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::empty_composite);

    const auto leaf_child = mirakana::evaluate_behavior_tree(
        mirakana::BehaviorTreeDesc{
            .root_id = 1,
            .nodes =
                std::vector<mirakana::BehaviorTreeNodeDesc>{
                    mirakana::BehaviorTreeNodeDesc{
                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {2}},
                    mirakana::BehaviorTreeNodeDesc{
                        .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                },
        },
        {mirakana::BehaviorTreeLeafResult{.node_id = 1, .status = mirakana::BehaviorTreeStatus::success}});
    MK_REQUIRE(leaf_child.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(leaf_child.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::leaf_has_children);
    MK_REQUIRE(leaf_child.diagnostic.node_id == 1);
    MK_REQUIRE(leaf_child.diagnostic.referenced_node_id == 2);
}

MK_TEST("behavior tree rejects missing invalid and cyclic leaves") {
    const mirakana::BehaviorTreeDesc leaf_tree{
        .root_id = 7,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{.id = 7, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };

    const auto missing_leaf = mirakana::evaluate_behavior_tree(leaf_tree, {});
    MK_REQUIRE(missing_leaf.status == mirakana::BehaviorTreeStatus::missing_leaf_result);
    MK_REQUIRE(missing_leaf.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::missing_leaf_result);

    const auto invalid_leaf = mirakana::evaluate_behavior_tree(
        leaf_tree,
        std::vector<mirakana::BehaviorTreeLeafResult>{
            mirakana::BehaviorTreeLeafResult{.node_id = 7, .status = mirakana::BehaviorTreeStatus::invalid_tree},
        });
    MK_REQUIRE(invalid_leaf.status == mirakana::BehaviorTreeStatus::invalid_leaf_result);
    MK_REQUIRE(invalid_leaf.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::invalid_leaf_result);

    const auto duplicate_leaf_result = mirakana::evaluate_behavior_tree(
        leaf_tree, std::vector<mirakana::BehaviorTreeLeafResult>{
                       mirakana::BehaviorTreeLeafResult{.node_id = 7, .status = mirakana::BehaviorTreeStatus::success},
                       mirakana::BehaviorTreeLeafResult{.node_id = 7, .status = mirakana::BehaviorTreeStatus::failure},
                   });
    MK_REQUIRE(duplicate_leaf_result.status == mirakana::BehaviorTreeStatus::invalid_leaf_result);
    MK_REQUIRE(duplicate_leaf_result.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::duplicate_leaf_result);
    MK_REQUIRE(duplicate_leaf_result.diagnostic.node_id == 7);

    const auto cycle = mirakana::evaluate_behavior_tree(
        mirakana::BehaviorTreeDesc{
            .root_id = 1,
            .nodes =
                std::vector<mirakana::BehaviorTreeNodeDesc>{
                    mirakana::BehaviorTreeNodeDesc{
                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2}},
                    mirakana::BehaviorTreeNodeDesc{
                        .id = 2, .kind = mirakana::BehaviorTreeNodeKind::selector, .children = {1}},
                },
        },
        {});
    MK_REQUIRE(cycle.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(cycle.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::cycle_detected);

    const auto max_depth = mirakana::evaluate_behavior_tree(
        mirakana::BehaviorTreeDesc{
            .root_id = 1,
            .nodes =
                std::vector<mirakana::BehaviorTreeNodeDesc>{
                    mirakana::BehaviorTreeNodeDesc{
                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2}},
                    mirakana::BehaviorTreeNodeDesc{
                        .id = 2, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
                },
            .max_depth = 1,
        },
        {});
    MK_REQUIRE(max_depth.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(max_depth.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::max_depth_exceeded);
}

MK_TEST("behavior tree blackboard stores typed values deterministically") {
    mirakana::BehaviorTreeBlackboard blackboard;

    MK_REQUIRE(blackboard.set("has_path", mirakana::make_behavior_tree_blackboard_bool(true)));
    MK_REQUIRE(blackboard.set("remaining_steps", mirakana::make_behavior_tree_blackboard_integer(3)));
    MK_REQUIRE(blackboard.set("distance", mirakana::make_behavior_tree_blackboard_double(2.5)));
    MK_REQUIRE(blackboard.set("mode", mirakana::make_behavior_tree_blackboard_string("moving")));
    MK_REQUIRE(blackboard.set("remaining_steps", mirakana::make_behavior_tree_blackboard_integer(1)));
    MK_REQUIRE(!blackboard.set("", mirakana::make_behavior_tree_blackboard_bool(false)));
    MK_REQUIRE(!blackboard.set(
        "bad_double", mirakana::make_behavior_tree_blackboard_double(std::numeric_limits<double>::infinity())));

    const auto entries = blackboard.entries();
    MK_REQUIRE(entries.size() == 4);
    MK_REQUIRE(entries[0].key == "has_path");
    MK_REQUIRE(entries[1].key == "remaining_steps");
    MK_REQUIRE(entries[2].key == "distance");
    MK_REQUIRE(entries[3].key == "mode");

    const auto* const remaining = blackboard.find("remaining_steps");
    MK_REQUIRE(remaining != nullptr);
    MK_REQUIRE(remaining->kind == mirakana::BehaviorTreeBlackboardValueKind::signed_integer);
    MK_REQUIRE(remaining->integer_value == 1);

    const auto* const mode = blackboard.find("mode");
    MK_REQUIRE(mode != nullptr);
    MK_REQUIRE(mode->kind == mirakana::BehaviorTreeBlackboardValueKind::string);
    MK_REQUIRE(mode->string_value == "moving");
}

MK_TEST("behavior tree blackboard conditions drive condition leaves") {
    const mirakana::BehaviorTreeDesc tree{
        .root_id = 1,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{
                    .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2, 3, 4, 5, 6}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 3, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 4, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 5, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{.id = 6, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };

    mirakana::BehaviorTreeBlackboard blackboard;
    MK_REQUIRE(blackboard.set("has_path", mirakana::make_behavior_tree_blackboard_bool(true)));
    MK_REQUIRE(blackboard.set("remaining_steps", mirakana::make_behavior_tree_blackboard_integer(3)));
    MK_REQUIRE(blackboard.set("distance", mirakana::make_behavior_tree_blackboard_double(4.0)));
    MK_REQUIRE(blackboard.set("mode", mirakana::make_behavior_tree_blackboard_string("moving")));

    const std::vector<mirakana::BehaviorTreeBlackboardCondition> conditions{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 3,
                                                  .key = "remaining_steps",
                                                  .comparison =
                                                      mirakana::BehaviorTreeBlackboardComparison::less_or_equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_integer(3)},
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 4,
                                                  .key = "distance",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::less,
                                                  .expected = mirakana::make_behavior_tree_blackboard_double(5.0)},
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 5,
                                                  .key = "mode",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::not_equal,
                                                  .expected =
                                                      mirakana::make_behavior_tree_blackboard_string("blocked")},
    };
    const std::vector<mirakana::BehaviorTreeLeafResult> actions{
        mirakana::BehaviorTreeLeafResult{.node_id = 6, .status = mirakana::BehaviorTreeStatus::success},
    };

    const auto first = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{actions},
                  .blackboard_entries = blackboard.entries(),
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{conditions},
              });
    const auto second = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{actions},
                  .blackboard_entries = blackboard.entries(),
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{conditions},
              });

    MK_REQUIRE(first.status == mirakana::BehaviorTreeStatus::success);
    MK_REQUIRE((first.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2, 3, 4, 5, 6}));
    MK_REQUIRE(first.status == second.status);
    MK_REQUIRE(first.visited_nodes == second.visited_nodes);
    MK_REQUIRE(first.diagnostic.code == second.diagnostic.code);
}

MK_TEST("behavior tree blackboard condition failure stops sequence without diagnostics") {
    const mirakana::BehaviorTreeDesc tree{
        .root_id = 1,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{
                    .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2, 3}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{.id = 3, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };

    mirakana::BehaviorTreeBlackboard blackboard;
    MK_REQUIRE(blackboard.set("has_path", mirakana::make_behavior_tree_blackboard_bool(false)));
    const std::vector<mirakana::BehaviorTreeBlackboardCondition> conditions{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
    };
    const std::vector<mirakana::BehaviorTreeLeafResult> actions{
        mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::success},
    };

    const auto result = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{actions},
                  .blackboard_entries = blackboard.entries(),
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{conditions},
              });

    MK_REQUIRE(result.status == mirakana::BehaviorTreeStatus::failure);
    MK_REQUIRE((result.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2}));
    MK_REQUIRE(result.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::none);
}

MK_TEST("behavior tree selector preserves blackboard diagnostics from failed children") {
    const mirakana::BehaviorTreeDesc tree{
        .root_id = 1,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{
                    .id = 1, .kind = mirakana::BehaviorTreeNodeKind::selector, .children = {2, 3}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{.id = 3, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };
    const std::vector<mirakana::BehaviorTreeBlackboardCondition> conditions{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
    };
    const std::vector<mirakana::BehaviorTreeLeafResult> actions{
        mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::success},
    };

    const auto result = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{actions},
                  .blackboard_entries = {},
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{conditions},
              });

    MK_REQUIRE(result.status == mirakana::BehaviorTreeStatus::failure);
    MK_REQUIRE(result.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::missing_blackboard_key);
    MK_REQUIRE(result.diagnostic.node_id == 2);
    MK_REQUIRE((result.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2}));
}

MK_TEST("behavior tree evaluation context conditions fall back to leaf results without blackboard binding") {
    const mirakana::BehaviorTreeDesc tree{
        .root_id = 1,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{
                    .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2, 3}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{.id = 3, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };
    mirakana::BehaviorTreeBlackboard blackboard;
    MK_REQUIRE(blackboard.set("unused", mirakana::make_behavior_tree_blackboard_bool(true)));
    const std::vector<mirakana::BehaviorTreeLeafResult> leaves{
        mirakana::BehaviorTreeLeafResult{.node_id = 2, .status = mirakana::BehaviorTreeStatus::success},
        mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::success},
    };

    const auto result = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{leaves},
                  .blackboard_entries = blackboard.entries(),
                  .blackboard_conditions = {},
              });

    MK_REQUIRE(result.status == mirakana::BehaviorTreeStatus::success);
    MK_REQUIRE((result.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2, 3}));
    MK_REQUIRE(result.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::none);
}

MK_TEST("behavior tree blackboard conditions report missing key and type mismatch") {
    const mirakana::BehaviorTreeDesc tree{
        .root_id = 2,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{
                    .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
            },
    };

    const std::vector<mirakana::BehaviorTreeBlackboardCondition> missing_conditions{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
    };
    const auto missing = mirakana::evaluate_behavior_tree(
        tree,
        mirakana::BehaviorTreeEvaluationContext{
            .leaf_results = {},
            .blackboard_entries = {},
            .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{missing_conditions},
        });
    MK_REQUIRE(missing.status == mirakana::BehaviorTreeStatus::failure);
    MK_REQUIRE(missing.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::missing_blackboard_key);
    MK_REQUIRE(missing.diagnostic.node_id == 2);

    mirakana::BehaviorTreeBlackboard blackboard;
    MK_REQUIRE(blackboard.set("has_path", mirakana::make_behavior_tree_blackboard_bool(true)));
    const std::vector<mirakana::BehaviorTreeBlackboardCondition> mismatch_conditions{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_integer(1)},
    };
    const auto mismatch = mirakana::evaluate_behavior_tree(
        tree,
        mirakana::BehaviorTreeEvaluationContext{
            .leaf_results = {},
            .blackboard_entries = blackboard.entries(),
            .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{mismatch_conditions},
        });
    MK_REQUIRE(mismatch.status == mirakana::BehaviorTreeStatus::failure);
    MK_REQUIRE(mismatch.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::blackboard_type_mismatch);
    MK_REQUIRE(mismatch.diagnostic.node_id == 2);
}

MK_TEST("behavior tree rejects invalid blackboard descriptors deterministically") {
    const mirakana::BehaviorTreeDesc tree{
        .root_id = 1,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{
                    .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2, 3}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{.id = 3, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };
    const std::vector<mirakana::BehaviorTreeLeafResult> actions{
        mirakana::BehaviorTreeLeafResult{.node_id = 3, .status = mirakana::BehaviorTreeStatus::success},
    };

    const std::vector<mirakana::BehaviorTreeBlackboardEntry> duplicate_entries{
        mirakana::BehaviorTreeBlackboardEntry{.key = "has_path",
                                              .value = mirakana::make_behavior_tree_blackboard_bool(true)},
        mirakana::BehaviorTreeBlackboardEntry{.key = "has_path",
                                              .value = mirakana::make_behavior_tree_blackboard_bool(false)},
    };
    const std::vector<mirakana::BehaviorTreeBlackboardCondition> valid_condition{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
    };
    const std::span<const mirakana::BehaviorTreeBlackboardEntry> first_entry{duplicate_entries.data(), 1};
    const auto duplicate_key = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{actions},
                  .blackboard_entries = std::span<const mirakana::BehaviorTreeBlackboardEntry>{duplicate_entries},
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{valid_condition},
              });
    MK_REQUIRE(duplicate_key.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(duplicate_key.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::duplicate_blackboard_key);

    const std::vector<mirakana::BehaviorTreeBlackboardCondition> duplicate_conditions{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
    };
    const auto duplicate_condition = mirakana::evaluate_behavior_tree(
        tree,
        mirakana::BehaviorTreeEvaluationContext{
            .leaf_results = {},
            .blackboard_entries = first_entry,
            .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{duplicate_conditions},
        });
    MK_REQUIRE(duplicate_condition.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(duplicate_condition.diagnostic.code ==
               mirakana::BehaviorTreeDiagnosticCode::duplicate_blackboard_condition);

    const std::vector<mirakana::BehaviorTreeBlackboardCondition> action_binding{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 3,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
    };
    const auto invalid_binding = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = {},
                  .blackboard_entries = first_entry,
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{action_binding},
              });
    MK_REQUIRE(invalid_binding.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(invalid_binding.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::invalid_blackboard_condition);

    const std::vector<mirakana::BehaviorTreeBlackboardCondition> invalid_operator{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::less,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
    };
    const auto bad_operator = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = {},
                  .blackboard_entries = first_entry,
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{invalid_operator},
              });
    MK_REQUIRE(bad_operator.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(bad_operator.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::invalid_blackboard_operator);

    const std::vector<mirakana::BehaviorTreeBlackboardCondition> invalid_value{
        mirakana::BehaviorTreeBlackboardCondition{
            .node_id = 2,
            .key = "distance",
            .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
            .expected = mirakana::make_behavior_tree_blackboard_double(std::numeric_limits<double>::infinity())},
    };
    const auto bad_value = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = {},
                  .blackboard_entries = first_entry,
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{invalid_value},
              });
    MK_REQUIRE(bad_value.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(bad_value.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::invalid_blackboard_value);

    const std::vector<mirakana::BehaviorTreeBlackboardEntry> invalid_key_entry{
        mirakana::BehaviorTreeBlackboardEntry{.key = "", .value = mirakana::make_behavior_tree_blackboard_bool(true)},
    };
    const auto bad_entry_key = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = {},
                  .blackboard_entries = std::span<const mirakana::BehaviorTreeBlackboardEntry>{invalid_key_entry},
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{valid_condition},
              });
    MK_REQUIRE(bad_entry_key.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(bad_entry_key.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::invalid_blackboard_key);

    const std::vector<mirakana::BehaviorTreeBlackboardCondition> invalid_key_condition{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
    };
    const auto bad_condition_key = mirakana::evaluate_behavior_tree(
        tree,
        mirakana::BehaviorTreeEvaluationContext{
            .leaf_results = {},
            .blackboard_entries = first_entry,
            .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{invalid_key_condition},
        });
    MK_REQUIRE(bad_condition_key.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(bad_condition_key.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::invalid_blackboard_condition);

    auto unknown_value = mirakana::make_behavior_tree_blackboard_bool(true);
    corrupt_blackboard_value_kind(unknown_value);
    const std::vector<mirakana::BehaviorTreeBlackboardEntry> invalid_kind_entry{
        mirakana::BehaviorTreeBlackboardEntry{.key = "has_path", .value = unknown_value},
    };
    const auto bad_entry_kind = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = {},
                  .blackboard_entries = std::span<const mirakana::BehaviorTreeBlackboardEntry>{invalid_kind_entry},
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{valid_condition},
              });
    MK_REQUIRE(bad_entry_kind.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(bad_entry_kind.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::invalid_blackboard_value);

    const std::vector<mirakana::BehaviorTreeBlackboardCondition> invalid_kind_condition{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "has_path",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = unknown_value},
    };
    const auto bad_condition_kind = mirakana::evaluate_behavior_tree(
        tree,
        mirakana::BehaviorTreeEvaluationContext{
            .leaf_results = {},
            .blackboard_entries = first_entry,
            .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{invalid_kind_condition},
        });
    MK_REQUIRE(bad_condition_kind.status == mirakana::BehaviorTreeStatus::invalid_tree);
    MK_REQUIRE(bad_condition_kind.diagnostic.code == mirakana::BehaviorTreeDiagnosticCode::invalid_blackboard_value);
}

MK_TEST("ai perception snapshot selects visible primary target and writes blackboard facts") {
    const std::vector<mirakana::AiPerceptionTarget2D> targets{
        mirakana::AiPerceptionTarget2D{.id = 4,
                                       .position = mirakana::AiPerceptionPoint2{.x = 5.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = true,
                                       .hearing_enabled = false,
                                       .sound_radius = 0.0F},
        mirakana::AiPerceptionTarget2D{.id = 2,
                                       .position = mirakana::AiPerceptionPoint2{.x = 3.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = true,
                                       .hearing_enabled = false,
                                       .sound_radius = 0.0F},
        mirakana::AiPerceptionTarget2D{.id = 3,
                                       .position = mirakana::AiPerceptionPoint2{.x = 2.0F, .y = 3.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = true,
                                       .hearing_enabled = false,
                                       .sound_radius = 0.0F},
    };

    const auto snapshot = mirakana::build_ai_perception_snapshot_2d(mirakana::AiPerceptionRequest2D{
        .agent = mirakana::AiPerceptionAgent2D{.id = 1,
                                               .position = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                               .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                               .sight_range = 10.0F,
                                               .field_of_view_radians = 1.57079632679F,
                                               .hearing_range = 0.0F},
        .targets = std::span<const mirakana::AiPerceptionTarget2D>{targets},
    });

    MK_REQUIRE(snapshot.status == mirakana::AiPerceptionStatus::ready);
    MK_REQUIRE(snapshot.diagnostic == mirakana::AiPerceptionDiagnostic::none);
    MK_REQUIRE(snapshot.visible_count == 2U);
    MK_REQUIRE(snapshot.audible_count == 0U);
    MK_REQUIRE(snapshot.has_primary_target);
    MK_REQUIRE(snapshot.primary_target.id == 2U);
    MK_REQUIRE(snapshot.primary_target.visible);
    MK_REQUIRE(!snapshot.primary_target.audible);
    MK_REQUIRE(snapshot.primary_target.distance == 3.0F);

    mirakana::BehaviorTreeBlackboard blackboard;
    const auto projection = mirakana::write_ai_perception_blackboard(
        snapshot,
        mirakana::AiPerceptionBlackboardKeys{.has_target_key = "perception.has_target",
                                             .target_id_key = "perception.target_id",
                                             .target_distance_key = "perception.target_distance",
                                             .visible_count_key = "perception.visible_count",
                                             .audible_count_key = "perception.audible_count",
                                             .target_state_key = "perception.target_state"},
        blackboard);

    MK_REQUIRE(projection.status == mirakana::AiPerceptionBlackboardStatus::ready);
    MK_REQUIRE(projection.diagnostic == mirakana::AiPerceptionDiagnostic::none);

    const auto tree = mirakana::BehaviorTreeDesc{
        .root_id = 1,
        .nodes =
            std::vector<mirakana::BehaviorTreeNodeDesc>{
                mirakana::BehaviorTreeNodeDesc{
                    .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2, 3, 4}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = 3, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{.id = 4, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };
    const std::vector<mirakana::BehaviorTreeBlackboardCondition> conditions{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 2,
                                                  .key = "perception.has_target",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
        mirakana::BehaviorTreeBlackboardCondition{.node_id = 3,
                                                  .key = "perception.target_state",
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected =
                                                      mirakana::make_behavior_tree_blackboard_string("visible")},
    };
    const std::vector<mirakana::BehaviorTreeLeafResult> actions{
        mirakana::BehaviorTreeLeafResult{.node_id = 4, .status = mirakana::BehaviorTreeStatus::success},
    };

    const auto result = mirakana::evaluate_behavior_tree(
        tree, mirakana::BehaviorTreeEvaluationContext{
                  .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{actions},
                  .blackboard_entries = blackboard.entries(),
                  .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{conditions}});

    MK_REQUIRE(result.status == mirakana::BehaviorTreeStatus::success);
    MK_REQUIRE((result.visited_nodes == std::vector<mirakana::BehaviorTreeNodeId>{1, 2, 3, 4}));
}

MK_TEST("ai perception snapshot uses audible fallback when no visible target exists") {
    const std::vector<mirakana::AiPerceptionTarget2D> targets{
        mirakana::AiPerceptionTarget2D{.id = 7,
                                       .position = mirakana::AiPerceptionPoint2{.x = 6.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = false,
                                       .hearing_enabled = true,
                                       .sound_radius = 2.0F},
    };

    const auto snapshot = mirakana::build_ai_perception_snapshot_2d(mirakana::AiPerceptionRequest2D{
        .agent = mirakana::AiPerceptionAgent2D{.id = 1,
                                               .position = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                               .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                               .sight_range = 3.0F,
                                               .field_of_view_radians = 1.57079632679F,
                                               .hearing_range = 4.0F},
        .targets = std::span<const mirakana::AiPerceptionTarget2D>{targets},
    });

    MK_REQUIRE(snapshot.status == mirakana::AiPerceptionStatus::ready);
    MK_REQUIRE(snapshot.visible_count == 0U);
    MK_REQUIRE(snapshot.audible_count == 1U);
    MK_REQUIRE(snapshot.has_primary_target);
    MK_REQUIRE(snapshot.primary_target.id == 7U);
    MK_REQUIRE(!snapshot.primary_target.visible);
    MK_REQUIRE(snapshot.primary_target.audible);

    mirakana::BehaviorTreeBlackboard blackboard;
    const auto projection =
        mirakana::write_ai_perception_blackboard(snapshot, mirakana::AiPerceptionBlackboardKeys{}, blackboard);
    MK_REQUIRE(projection.status == mirakana::AiPerceptionBlackboardStatus::ready);

    const auto* const state = blackboard.find("perception.target_state");
    MK_REQUIRE(state != nullptr);
    MK_REQUIRE(state->kind == mirakana::BehaviorTreeBlackboardValueKind::string);
    MK_REQUIRE(state->string_value == "audible");
}

MK_TEST("ai perception snapshot chooses stable primary target by sense distance and id") {
    const std::vector<mirakana::AiPerceptionTarget2D> targets{
        mirakana::AiPerceptionTarget2D{.id = 9,
                                       .position = mirakana::AiPerceptionPoint2{.x = 4.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = true,
                                       .hearing_enabled = true,
                                       .sound_radius = 0.0F},
        mirakana::AiPerceptionTarget2D{.id = 8,
                                       .position = mirakana::AiPerceptionPoint2{.x = 4.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = true,
                                       .hearing_enabled = true,
                                       .sound_radius = 0.0F},
    };

    const auto snapshot = mirakana::build_ai_perception_snapshot_2d(mirakana::AiPerceptionRequest2D{
        .agent = mirakana::AiPerceptionAgent2D{.id = 1,
                                               .position = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                               .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                               .sight_range = 5.0F,
                                               .field_of_view_radians = 6.28318530718F,
                                               .hearing_range = 5.0F},
        .targets = std::span<const mirakana::AiPerceptionTarget2D>{targets},
    });

    MK_REQUIRE(snapshot.status == mirakana::AiPerceptionStatus::ready);
    MK_REQUIRE(snapshot.targets.size() == 2U);
    MK_REQUIRE(snapshot.has_primary_target);
    MK_REQUIRE(snapshot.primary_target.id == 8U);
    MK_REQUIRE(snapshot.targets[0].id == 8U);
    MK_REQUIRE(snapshot.targets[1].id == 9U);
}

MK_TEST("ai perception snapshot rejects invalid agent target and duplicate ids") {
    const std::vector<mirakana::AiPerceptionTarget2D> valid_targets{
        mirakana::AiPerceptionTarget2D{.id = 2,
                                       .position = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = true,
                                       .hearing_enabled = false,
                                       .sound_radius = 0.0F},
    };

    const auto invalid_agent = mirakana::build_ai_perception_snapshot_2d(mirakana::AiPerceptionRequest2D{
        .agent = mirakana::AiPerceptionAgent2D{.id = 1,
                                               .position = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                               .forward = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                               .sight_range = 5.0F,
                                               .field_of_view_radians = 1.0F,
                                               .hearing_range = 0.0F},
        .targets = std::span<const mirakana::AiPerceptionTarget2D>{valid_targets},
    });
    MK_REQUIRE(invalid_agent.status == mirakana::AiPerceptionStatus::invalid_agent);
    MK_REQUIRE(invalid_agent.diagnostic == mirakana::AiPerceptionDiagnostic::invalid_agent_forward);

    const std::vector<mirakana::AiPerceptionTarget2D> invalid_targets{
        mirakana::AiPerceptionTarget2D{
            .id = 2,
            .position = mirakana::AiPerceptionPoint2{.x = std::numeric_limits<float>::infinity(), .y = 0.0F},
            .radius = 0.0F,
            .sight_enabled = true,
            .hearing_enabled = false,
            .sound_radius = 0.0F},
    };
    const auto invalid_target = mirakana::build_ai_perception_snapshot_2d(mirakana::AiPerceptionRequest2D{
        .agent = mirakana::AiPerceptionAgent2D{.id = 1,
                                               .position = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                               .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                               .sight_range = 5.0F,
                                               .field_of_view_radians = 1.0F,
                                               .hearing_range = 0.0F},
        .targets = std::span<const mirakana::AiPerceptionTarget2D>{invalid_targets},
    });
    MK_REQUIRE(invalid_target.status == mirakana::AiPerceptionStatus::invalid_target);
    MK_REQUIRE(invalid_target.diagnostic == mirakana::AiPerceptionDiagnostic::invalid_target_position);
    MK_REQUIRE(invalid_target.diagnostic_target_id == 2U);

    const std::vector<mirakana::AiPerceptionTarget2D> duplicate_targets{
        mirakana::AiPerceptionTarget2D{.id = 2,
                                       .position = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = true,
                                       .hearing_enabled = false,
                                       .sound_radius = 0.0F},
        mirakana::AiPerceptionTarget2D{.id = 2,
                                       .position = mirakana::AiPerceptionPoint2{.x = 2.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = true,
                                       .hearing_enabled = false,
                                       .sound_radius = 0.0F},
    };
    const auto duplicate = mirakana::build_ai_perception_snapshot_2d(mirakana::AiPerceptionRequest2D{
        .agent = mirakana::AiPerceptionAgent2D{.id = 1,
                                               .position = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                               .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                               .sight_range = 5.0F,
                                               .field_of_view_radians = 1.0F,
                                               .hearing_range = 0.0F},
        .targets = std::span<const mirakana::AiPerceptionTarget2D>{duplicate_targets},
    });
    MK_REQUIRE(duplicate.status == mirakana::AiPerceptionStatus::duplicate_target_id);
    MK_REQUIRE(duplicate.diagnostic == mirakana::AiPerceptionDiagnostic::duplicate_target_id);
    MK_REQUIRE(duplicate.diagnostic_target_id == 2U);
}

MK_TEST("ai perception blackboard projection rejects invalid snapshots and keys") {
    mirakana::BehaviorTreeBlackboard blackboard;
    const auto invalid_snapshot = mirakana::write_ai_perception_blackboard(
        mirakana::AiPerceptionSnapshot2D{.status = mirakana::AiPerceptionStatus::invalid_agent,
                                         .diagnostic = mirakana::AiPerceptionDiagnostic::invalid_agent_forward},
        mirakana::AiPerceptionBlackboardKeys{}, blackboard);
    MK_REQUIRE(invalid_snapshot.status == mirakana::AiPerceptionBlackboardStatus::invalid_snapshot);
    MK_REQUIRE(invalid_snapshot.diagnostic == mirakana::AiPerceptionDiagnostic::snapshot_not_ready);

    const auto empty_key = mirakana::write_ai_perception_blackboard(
        mirakana::AiPerceptionSnapshot2D{.status = mirakana::AiPerceptionStatus::ready,
                                         .diagnostic = mirakana::AiPerceptionDiagnostic::none,
                                         .diagnostic_target_id = {},
                                         .targets = {},
                                         .has_primary_target = false,
                                         .primary_target = {},
                                         .visible_count = 0U,
                                         .audible_count = 0U},
        mirakana::AiPerceptionBlackboardKeys{.has_target_key = "",
                                             .target_id_key = "perception.target_id",
                                             .target_distance_key = "perception.target_distance",
                                             .visible_count_key = "perception.visible_count",
                                             .audible_count_key = "perception.audible_count",
                                             .target_state_key = "perception.target_state"},
        blackboard);
    MK_REQUIRE(empty_key.status == mirakana::AiPerceptionBlackboardStatus::invalid_key);
    MK_REQUIRE(empty_key.diagnostic == mirakana::AiPerceptionDiagnostic::invalid_blackboard_key);

    const auto duplicate_key = mirakana::write_ai_perception_blackboard(
        mirakana::AiPerceptionSnapshot2D{.status = mirakana::AiPerceptionStatus::ready,
                                         .diagnostic = mirakana::AiPerceptionDiagnostic::none,
                                         .diagnostic_target_id = {},
                                         .targets = {},
                                         .has_primary_target = false,
                                         .primary_target = {},
                                         .visible_count = 0U,
                                         .audible_count = 0U},
        mirakana::AiPerceptionBlackboardKeys{.has_target_key = "perception.has_target",
                                             .target_id_key = "perception.has_target",
                                             .target_distance_key = "perception.target_distance",
                                             .visible_count_key = "perception.visible_count",
                                             .audible_count_key = "perception.audible_count",
                                             .target_state_key = "perception.target_state"},
        blackboard);
    MK_REQUIRE(duplicate_key.status == mirakana::AiPerceptionBlackboardStatus::invalid_key);
    MK_REQUIRE(duplicate_key.diagnostic == mirakana::AiPerceptionDiagnostic::invalid_blackboard_key);
}

MK_TEST("ai perception readiness summarizes sight hearing stable primary and blackboard projection") {
    const std::vector<mirakana::AiPerceptionTarget2D> targets{
        mirakana::AiPerceptionTarget2D{.id = 10,
                                       .position = mirakana::AiPerceptionPoint2{.x = 5.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = true,
                                       .hearing_enabled = false,
                                       .sound_radius = 0.0F},
        mirakana::AiPerceptionTarget2D{.id = 11,
                                       .position = mirakana::AiPerceptionPoint2{.x = 7.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = false,
                                       .hearing_enabled = true,
                                       .sound_radius = 2.0F},
    };

    const auto report = mirakana::evaluate_ai_perception_readiness_2d(
        mirakana::AiPerceptionRequest2D{
            .agent = mirakana::AiPerceptionAgent2D{.id = 1,
                                                   .position = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                                   .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                                   .sight_range = 6.0F,
                                                   .field_of_view_radians = 1.57079632679F,
                                                   .hearing_range = 5.0F},
            .targets = std::span<const mirakana::AiPerceptionTarget2D>{targets},
        },
        mirakana::AiPerceptionBlackboardKeys{},
        mirakana::AiPerceptionReadinessConfig{
            .require_visible_target = true,
            .require_audible_target = true,
            .min_targets = 2,
            .max_targets = 2,
        });

    MK_REQUIRE(report.status == mirakana::AiPerceptionReadinessStatus::ready);
    MK_REQUIRE(report.diagnostic == mirakana::AiPerceptionReadinessDiagnostic::none);
    MK_REQUIRE(report.snapshot_status == mirakana::AiPerceptionStatus::ready);
    MK_REQUIRE(report.blackboard_status == mirakana::AiPerceptionBlackboardStatus::ready);
    MK_REQUIRE(report.stable_primary_target_ready);
    MK_REQUIRE(report.blackboard_projection_ready);
    MK_REQUIRE(report.target_count == 2U);
    MK_REQUIRE(report.visible_count == 1U);
    MK_REQUIRE(report.audible_count == 1U);
    MK_REQUIRE(report.has_primary_target);
    MK_REQUIRE(report.primary_target_id == 10U);
    MK_REQUIRE(report.primary_target_visible);
    MK_REQUIRE(!report.primary_target_audible);
    MK_REQUIRE(report.diagnostics.empty());
}

MK_TEST("ai perception readiness reports invalid snapshots projection failures and evidence gaps") {
    const std::vector<mirakana::AiPerceptionTarget2D> hidden_targets{
        mirakana::AiPerceptionTarget2D{.id = 12,
                                       .position = mirakana::AiPerceptionPoint2{.x = 20.0F, .y = 0.0F},
                                       .radius = 0.0F,
                                       .sight_enabled = true,
                                       .hearing_enabled = true,
                                       .sound_radius = 0.0F},
    };
    const auto missing_evidence = mirakana::evaluate_ai_perception_readiness_2d(
        mirakana::AiPerceptionRequest2D{
            .agent = mirakana::AiPerceptionAgent2D{.id = 1,
                                                   .position = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                                   .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                                   .sight_range = 1.0F,
                                                   .field_of_view_radians = 1.57079632679F,
                                                   .hearing_range = 1.0F},
            .targets = std::span<const mirakana::AiPerceptionTarget2D>{hidden_targets},
        },
        mirakana::AiPerceptionBlackboardKeys{},
        mirakana::AiPerceptionReadinessConfig{.require_visible_target = true, .require_audible_target = true});

    MK_REQUIRE(missing_evidence.status == mirakana::AiPerceptionReadinessStatus::diagnostics);
    MK_REQUIRE(missing_evidence.diagnostic == mirakana::AiPerceptionReadinessDiagnostic::insufficient_targets);
    MK_REQUIRE(
        (missing_evidence.diagnostics == std::vector<mirakana::AiPerceptionReadinessDiagnostic>{
                                             mirakana::AiPerceptionReadinessDiagnostic::insufficient_targets,
                                             mirakana::AiPerceptionReadinessDiagnostic::missing_primary_target,
                                             mirakana::AiPerceptionReadinessDiagnostic::missing_visible_target,
                                             mirakana::AiPerceptionReadinessDiagnostic::missing_audible_target}));

    const auto invalid_snapshot = mirakana::evaluate_ai_perception_readiness_2d(mirakana::AiPerceptionRequest2D{
        .agent = mirakana::AiPerceptionAgent2D{.id = 0,
                                               .position = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                               .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                               .sight_range = 1.0F,
                                               .field_of_view_radians = 1.57079632679F,
                                               .hearing_range = 1.0F},
        .targets = std::span<const mirakana::AiPerceptionTarget2D>{hidden_targets},
    });
    MK_REQUIRE(invalid_snapshot.status == mirakana::AiPerceptionReadinessStatus::invalid_snapshot);
    MK_REQUIRE(invalid_snapshot.diagnostic == mirakana::AiPerceptionReadinessDiagnostic::invalid_snapshot);
    MK_REQUIRE(invalid_snapshot.snapshot_diagnostic == mirakana::AiPerceptionDiagnostic::invalid_agent_id);

    const auto bad_projection = mirakana::evaluate_ai_perception_readiness_2d(
        mirakana::AiPerceptionRequest2D{
            .agent = mirakana::AiPerceptionAgent2D{.id = 1,
                                                   .position = mirakana::AiPerceptionPoint2{.x = 0.0F, .y = 0.0F},
                                                   .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                                   .sight_range = 25.0F,
                                                   .field_of_view_radians = 6.28318530718F,
                                                   .hearing_range = 25.0F},
            .targets = std::span<const mirakana::AiPerceptionTarget2D>{hidden_targets},
        },
        mirakana::AiPerceptionBlackboardKeys{.has_target_key = "perception.has_target",
                                             .target_id_key = "perception.has_target",
                                             .target_distance_key = "perception.target_distance",
                                             .visible_count_key = "perception.visible_count",
                                             .audible_count_key = "perception.audible_count",
                                             .target_state_key = "perception.target_state"});
    MK_REQUIRE(bad_projection.status == mirakana::AiPerceptionReadinessStatus::diagnostics);
    MK_REQUIRE(bad_projection.diagnostic == mirakana::AiPerceptionReadinessDiagnostic::blackboard_projection_failed);
    MK_REQUIRE(!bad_projection.blackboard_projection_ready);
    MK_REQUIRE(bad_projection.blackboard_status == mirakana::AiPerceptionBlackboardStatus::invalid_key);
}

MK_TEST("behavior authoring document validates supported actions blackboard keys and trace order") {
    const mirakana::BehaviorAuthoringDocument document{
        .behaviors =
            std::vector<mirakana::BehaviorAuthoringBehaviorDesc>{
                mirakana::BehaviorAuthoringBehaviorDesc{
                    .id = "enemy_patrol",
                    .tree =
                        mirakana::BehaviorTreeDesc{
                            .root_id = 1,
                            .nodes =
                                std::vector<mirakana::BehaviorTreeNodeDesc>{
                                    mirakana::BehaviorTreeNodeDesc{
                                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2, 3}},
                                    mirakana::BehaviorTreeNodeDesc{
                                        .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                                    mirakana::BehaviorTreeNodeDesc{
                                        .id = 3, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
                                },
                        },
                    .blackboard_conditions =
                        std::vector<mirakana::BehaviorTreeBlackboardCondition>{
                            mirakana::BehaviorTreeBlackboardCondition{
                                .node_id = 2,
                                .key = "perception.has_target",
                                .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
                        },
                    .actions =
                        std::vector<mirakana::BehaviorAuthoringActionBinding>{
                            mirakana::BehaviorAuthoringActionBinding{.node_id = 3, .action_id = "move_to_target"},
                        },
                },
            },
    };
    const std::vector<std::string> blackboard_keys{"perception.has_target"};
    const std::vector<std::string> action_ids{"move_to_target"};

    const auto first = mirakana::validate_behavior_authoring_document(
        document, mirakana::BehaviorAuthoringValidationContext{
                      .blackboard_keys = std::span<const std::string>{blackboard_keys},
                      .supported_actions = std::span<const std::string>{action_ids},
                  });
    const auto second = mirakana::validate_behavior_authoring_document(
        document, mirakana::BehaviorAuthoringValidationContext{
                      .blackboard_keys = std::span<const std::string>{blackboard_keys},
                      .supported_actions = std::span<const std::string>{action_ids},
                  });

    MK_REQUIRE(first.succeeded);
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.trace == second.trace);
    MK_REQUIRE(first.trace.size() == 3U);
    MK_REQUIRE(first.trace[0].behavior_id == "enemy_patrol");
    MK_REQUIRE(first.trace[0].node_id == 1U);
    MK_REQUIRE(first.trace[1].node_id == 2U);
    MK_REQUIRE(first.trace[2].node_id == 3U);
}

MK_TEST("behavior authoring readiness summarizes validation trace and authoring budgets") {
    const mirakana::BehaviorAuthoringDocument document{
        .behaviors =
            std::vector<mirakana::BehaviorAuthoringBehaviorDesc>{
                mirakana::BehaviorAuthoringBehaviorDesc{
                    .id = "enemy_patrol",
                    .tree =
                        mirakana::BehaviorTreeDesc{
                            .root_id = 1,
                            .nodes =
                                std::vector<mirakana::BehaviorTreeNodeDesc>{
                                    mirakana::BehaviorTreeNodeDesc{
                                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2, 3}},
                                    mirakana::BehaviorTreeNodeDesc{
                                        .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                                    mirakana::BehaviorTreeNodeDesc{
                                        .id = 3, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
                                },
                        },
                    .blackboard_conditions =
                        std::vector<mirakana::BehaviorTreeBlackboardCondition>{
                            mirakana::BehaviorTreeBlackboardCondition{
                                .node_id = 2,
                                .key = "perception.has_target",
                                .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
                        },
                    .actions =
                        std::vector<mirakana::BehaviorAuthoringActionBinding>{
                            mirakana::BehaviorAuthoringActionBinding{.node_id = 3, .action_id = "move_to_target"},
                        },
                },
            },
    };
    const std::vector<std::string> blackboard_keys{"perception.has_target"};
    const std::vector<std::string> action_ids{"move_to_target"};

    const auto report = mirakana::evaluate_behavior_authoring_readiness(
        document,
        mirakana::BehaviorAuthoringValidationContext{
            .blackboard_keys = std::span<const std::string>{blackboard_keys},
            .supported_actions = std::span<const std::string>{action_ids},
        },
        mirakana::BehaviorAuthoringReadinessConfig{
            .min_behaviors = 1,
            .min_trace_nodes = 3,
            .min_action_bindings = 1,
            .min_blackboard_conditions = 1,
            .max_validation_diagnostics = 0,
            .max_behaviors = 2,
            .max_trace_nodes = 4,
        });

    MK_REQUIRE(report.status == mirakana::BehaviorAuthoringReadinessStatus::ready);
    MK_REQUIRE(report.primary_diagnostic == mirakana::BehaviorAuthoringReadinessDiagnostic::none);
    MK_REQUIRE(report.diagnostics.empty());
    MK_REQUIRE(report.validation_succeeded);
    MK_REQUIRE(report.deterministic_trace_ready);
    MK_REQUIRE(report.behavior_count == 1U);
    MK_REQUIRE(report.validation_diagnostic_count == 0U);
    MK_REQUIRE(report.trace_node_count == 3U);
    MK_REQUIRE(report.action_binding_count == 1U);
    MK_REQUIRE(report.blackboard_condition_count == 1U);
}

MK_TEST("behavior authoring readiness reports validation and evidence gaps deterministically") {
    const mirakana::BehaviorAuthoringDocument document{
        .behaviors =
            std::vector<mirakana::BehaviorAuthoringBehaviorDesc>{
                mirakana::BehaviorAuthoringBehaviorDesc{
                    .id = "enemy_chase",
                    .tree =
                        mirakana::BehaviorTreeDesc{
                            .root_id = 1,
                            .nodes =
                                std::vector<mirakana::BehaviorTreeNodeDesc>{
                                    mirakana::BehaviorTreeNodeDesc{
                                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
                                },
                        },
                    .blackboard_conditions = {},
                    .actions =
                        std::vector<mirakana::BehaviorAuthoringActionBinding>{
                            mirakana::BehaviorAuthoringActionBinding{.node_id = 1, .action_id = "teleport_to_target"},
                        },
                },
            },
    };
    const std::vector<std::string> action_ids{"move_to_target"};

    const auto first = mirakana::evaluate_behavior_authoring_readiness(
        document,
        mirakana::BehaviorAuthoringValidationContext{
            .blackboard_keys = {},
            .supported_actions = std::span<const std::string>{action_ids},
        },
        mirakana::BehaviorAuthoringReadinessConfig{
            .min_behaviors = 1,
            .min_trace_nodes = 2,
            .min_action_bindings = 1,
            .min_blackboard_conditions = 1,
            .max_validation_diagnostics = 0,
            .max_behaviors = 1,
            .max_trace_nodes = 1,
        });
    const auto second = mirakana::evaluate_behavior_authoring_readiness(
        document,
        mirakana::BehaviorAuthoringValidationContext{
            .blackboard_keys = {},
            .supported_actions = std::span<const std::string>{action_ids},
        },
        mirakana::BehaviorAuthoringReadinessConfig{
            .min_behaviors = 1,
            .min_trace_nodes = 2,
            .min_action_bindings = 1,
            .min_blackboard_conditions = 1,
            .max_validation_diagnostics = 0,
            .max_behaviors = 1,
            .max_trace_nodes = 1,
        });

    MK_REQUIRE(first.status == mirakana::BehaviorAuthoringReadinessStatus::diagnostics);
    MK_REQUIRE(first.primary_diagnostic == mirakana::BehaviorAuthoringReadinessDiagnostic::validation_diagnostics);
    MK_REQUIRE(first.diagnostics == second.diagnostics);
    MK_REQUIRE(first.diagnostics.size() == 3U);
    MK_REQUIRE(first.diagnostics[0] == mirakana::BehaviorAuthoringReadinessDiagnostic::validation_diagnostics);
    MK_REQUIRE(first.diagnostics[1] == mirakana::BehaviorAuthoringReadinessDiagnostic::insufficient_trace_nodes);
    MK_REQUIRE(first.diagnostics[2] ==
               mirakana::BehaviorAuthoringReadinessDiagnostic::insufficient_blackboard_conditions);
    MK_REQUIRE(!first.validation_succeeded);
    MK_REQUIRE(first.deterministic_trace_ready);
    MK_REQUIRE(first.behavior_count == 1U);
    MK_REQUIRE(first.validation_diagnostic_count == 1U);
    MK_REQUIRE(first.trace_node_count == 1U);
    MK_REQUIRE(first.action_binding_count == 1U);
    MK_REQUIRE(first.blackboard_condition_count == 0U);
}

MK_TEST("behavior authoring document reports invalid node keys and unsupported actions deterministically") {
    const mirakana::BehaviorAuthoringDocument document{
        .behaviors =
            std::vector<mirakana::BehaviorAuthoringBehaviorDesc>{
                mirakana::BehaviorAuthoringBehaviorDesc{
                    .id = "enemy_chase",
                    .tree =
                        mirakana::BehaviorTreeDesc{
                            .root_id = 1,
                            .nodes =
                                std::vector<mirakana::BehaviorTreeNodeDesc>{
                                    mirakana::BehaviorTreeNodeDesc{
                                        .id = 1, .kind = mirakana::BehaviorTreeNodeKind::sequence, .children = {2, 3}},
                                    mirakana::BehaviorTreeNodeDesc{
                                        .id = 2, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                                    mirakana::BehaviorTreeNodeDesc{
                                        .id = 3, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
                                },
                        },
                    .blackboard_conditions =
                        std::vector<mirakana::BehaviorTreeBlackboardCondition>{
                            mirakana::BehaviorTreeBlackboardCondition{
                                .node_id = 42,
                                .key = "perception.lost_target",
                                .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
                        },
                    .actions =
                        std::vector<mirakana::BehaviorAuthoringActionBinding>{
                            mirakana::BehaviorAuthoringActionBinding{.node_id = 3, .action_id = "teleport_to_target"},
                        },
                },
            },
    };
    const std::vector<std::string> blackboard_keys{"perception.has_target"};
    const std::vector<std::string> action_ids{"move_to_target"};
    const auto result = mirakana::validate_behavior_authoring_document(
        document, mirakana::BehaviorAuthoringValidationContext{
                      .blackboard_keys = std::span<const std::string>{blackboard_keys},
                      .supported_actions = std::span<const std::string>{action_ids},
                  });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(result.diagnostics.size() == 3U);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::BehaviorAuthoringDiagnosticCode::invalid_node_id);
    MK_REQUIRE(result.diagnostics[0].node_id == 42U);
    MK_REQUIRE(result.diagnostics[1].code == mirakana::BehaviorAuthoringDiagnosticCode::missing_blackboard_key);
    MK_REQUIRE(result.diagnostics[1].key == "perception.lost_target");
    MK_REQUIRE(result.diagnostics[2].code == mirakana::BehaviorAuthoringDiagnosticCode::unsupported_action);
    MK_REQUIRE(result.diagnostics[2].node_id == 3U);
    MK_REQUIRE(result.diagnostics[2].action_id == "teleport_to_target");
}

MK_TEST("behavior authoring document rejects duplicate behavior ids deterministically") {
    const auto behavior = mirakana::BehaviorAuthoringBehaviorDesc{
        .id = "shared_enemy_behavior",
        .tree =
            mirakana::BehaviorTreeDesc{
                .root_id = 1,
                .nodes =
                    std::vector<mirakana::BehaviorTreeNodeDesc>{
                        mirakana::BehaviorTreeNodeDesc{
                            .id = 1, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
                    },
            },
        .blackboard_conditions = {},
        .actions =
            std::vector<mirakana::BehaviorAuthoringActionBinding>{
                mirakana::BehaviorAuthoringActionBinding{.node_id = 1, .action_id = "idle"},
            },
    };
    const mirakana::BehaviorAuthoringDocument document{
        .behaviors =
            std::vector<mirakana::BehaviorAuthoringBehaviorDesc>{
                behavior,
                behavior,
            },
    };
    const std::vector<std::string> action_ids{"idle"};

    const auto first = mirakana::validate_behavior_authoring_document(
        document, mirakana::BehaviorAuthoringValidationContext{
                      .blackboard_keys = {},
                      .supported_actions = std::span<const std::string>{action_ids},
                  });
    const auto second = mirakana::validate_behavior_authoring_document(
        document, mirakana::BehaviorAuthoringValidationContext{
                      .blackboard_keys = {},
                      .supported_actions = std::span<const std::string>{action_ids},
                  });

    MK_REQUIRE(!first.succeeded);
    MK_REQUIRE(first.diagnostics == second.diagnostics);
    MK_REQUIRE(first.diagnostics.size() == 1U);
    MK_REQUIRE(first.diagnostics[0].code == mirakana::BehaviorAuthoringDiagnosticCode::duplicate_behavior_id);
    MK_REQUIRE(first.diagnostics[0].behavior_id == "shared_enemy_behavior");
}

int main() {
    return mirakana::test::run_all();
}
