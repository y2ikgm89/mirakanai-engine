// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

using BehaviorTreeNodeId = std::uint32_t;

enum class BehaviorTreeNodeKind : std::uint8_t {
    sequence,
    selector,
    action,
    condition,
};

enum class BehaviorTreeStatus : std::uint8_t {
    success,
    failure,
    running,
    invalid_tree,
    missing_leaf_result,
    invalid_leaf_result,
};

enum class BehaviorTreeDiagnosticCode : std::uint8_t {
    none,
    missing_root,
    duplicate_node_id,
    missing_child,
    empty_composite,
    missing_leaf_result,
    invalid_leaf_result,
    duplicate_leaf_result,
    cycle_detected,
    max_depth_exceeded,
    leaf_has_children,
    invalid_node_kind,
    duplicate_blackboard_key,
    invalid_blackboard_key,
    invalid_blackboard_value,
    duplicate_blackboard_condition,
    invalid_blackboard_condition,
    missing_blackboard_key,
    blackboard_type_mismatch,
    invalid_blackboard_operator,
};

struct BehaviorTreeNodeDesc {
    BehaviorTreeNodeId id{};
    BehaviorTreeNodeKind kind{BehaviorTreeNodeKind::action};
    std::vector<BehaviorTreeNodeId> children;
};

struct BehaviorTreeDesc {
    BehaviorTreeNodeId root_id{};
    std::vector<BehaviorTreeNodeDesc> nodes;
    std::size_t max_depth{64};
};

struct BehaviorTreeLeafResult {
    BehaviorTreeNodeId node_id{};
    BehaviorTreeStatus status{BehaviorTreeStatus::failure};
};

enum class BehaviorTreeBlackboardValueKind : std::uint8_t {
    boolean,
    signed_integer,
    finite_double,
    string,
};

struct BehaviorTreeBlackboardValue {
    BehaviorTreeBlackboardValueKind kind{BehaviorTreeBlackboardValueKind::boolean};
    bool bool_value{};
    std::int64_t integer_value{};
    double double_value{};
    std::string string_value;
};

[[nodiscard]] BehaviorTreeBlackboardValue make_behavior_tree_blackboard_bool(bool value);
[[nodiscard]] BehaviorTreeBlackboardValue make_behavior_tree_blackboard_integer(std::int64_t value);
[[nodiscard]] BehaviorTreeBlackboardValue make_behavior_tree_blackboard_double(double value);
[[nodiscard]] BehaviorTreeBlackboardValue make_behavior_tree_blackboard_string(std::string value);
[[nodiscard]] BehaviorTreeBlackboardValue make_behavior_tree_blackboard_string(std::string_view value);
[[nodiscard]] BehaviorTreeBlackboardValue make_behavior_tree_blackboard_string(const char* value);

struct BehaviorTreeBlackboardEntry {
    std::string key;
    BehaviorTreeBlackboardValue value;
};

class BehaviorTreeBlackboard {
  public:
    [[nodiscard]] bool set(std::string key, BehaviorTreeBlackboardValue value);
    [[nodiscard]] const BehaviorTreeBlackboardValue* find(std::string_view key) const noexcept;
    [[nodiscard]] std::span<const BehaviorTreeBlackboardEntry> entries() const noexcept;

  private:
    std::vector<BehaviorTreeBlackboardEntry> entries_;
};

enum class BehaviorTreeBlackboardComparison : std::uint8_t {
    equal,
    not_equal,
    less,
    less_or_equal,
    greater,
    greater_or_equal,
};

struct BehaviorTreeBlackboardCondition {
    BehaviorTreeNodeId node_id{};
    std::string key;
    BehaviorTreeBlackboardComparison comparison{BehaviorTreeBlackboardComparison::equal};
    BehaviorTreeBlackboardValue expected;
};

struct BehaviorTreeEvaluationContext {
    std::span<const BehaviorTreeLeafResult> leaf_results;
    std::span<const BehaviorTreeBlackboardEntry> blackboard_entries;
    std::span<const BehaviorTreeBlackboardCondition> blackboard_conditions;
};

struct BehaviorTreeDiagnostic {
    BehaviorTreeDiagnosticCode code{BehaviorTreeDiagnosticCode::none};
    BehaviorTreeNodeId node_id{};
    BehaviorTreeNodeId referenced_node_id{};
};

struct BehaviorTreeTickResult {
    BehaviorTreeStatus status{BehaviorTreeStatus::invalid_tree};
    std::vector<BehaviorTreeNodeId> visited_nodes;
    BehaviorTreeDiagnostic diagnostic;
};

[[nodiscard]] BehaviorTreeTickResult evaluate_behavior_tree(const BehaviorTreeDesc& tree,
                                                            BehaviorTreeEvaluationContext context);

[[nodiscard]] BehaviorTreeTickResult evaluate_behavior_tree(const BehaviorTreeDesc& tree,
                                                            std::span<const BehaviorTreeLeafResult> leaf_results);

[[nodiscard]] BehaviorTreeTickResult evaluate_behavior_tree(const BehaviorTreeDesc& tree,
                                                            const std::vector<BehaviorTreeLeafResult>& leaf_results);

[[nodiscard]] BehaviorTreeTickResult evaluate_behavior_tree(const BehaviorTreeDesc& tree,
                                                            std::initializer_list<BehaviorTreeLeafResult> leaf_results);

} // namespace mirakana
