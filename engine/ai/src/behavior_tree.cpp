// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ai/behavior_tree.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool is_runtime_leaf_status(const BehaviorTreeStatus status) noexcept {
    return status == BehaviorTreeStatus::success || status == BehaviorTreeStatus::failure ||
           status == BehaviorTreeStatus::running;
}

[[nodiscard]] bool is_valid_blackboard_key(const std::string_view key) noexcept {
    return !key.empty();
}

[[nodiscard]] bool is_known_blackboard_value_kind(const BehaviorTreeBlackboardValueKind kind) noexcept {
    switch (kind) {
    case BehaviorTreeBlackboardValueKind::boolean:
    case BehaviorTreeBlackboardValueKind::signed_integer:
    case BehaviorTreeBlackboardValueKind::finite_double:
    case BehaviorTreeBlackboardValueKind::string:
        return true;
    }
    return false;
}

[[nodiscard]] bool is_valid_blackboard_value(const BehaviorTreeBlackboardValue& value) noexcept {
    if (!is_known_blackboard_value_kind(value.kind)) {
        return false;
    }
    return value.kind != BehaviorTreeBlackboardValueKind::finite_double || std::isfinite(value.double_value);
}

[[nodiscard]] bool is_ordered_blackboard_value(const BehaviorTreeBlackboardValueKind kind) noexcept {
    return kind == BehaviorTreeBlackboardValueKind::signed_integer ||
           kind == BehaviorTreeBlackboardValueKind::finite_double;
}

[[nodiscard]] bool is_known_blackboard_comparison(const BehaviorTreeBlackboardComparison comparison) noexcept {
    switch (comparison) {
    case BehaviorTreeBlackboardComparison::equal:
    case BehaviorTreeBlackboardComparison::not_equal:
    case BehaviorTreeBlackboardComparison::less:
    case BehaviorTreeBlackboardComparison::less_or_equal:
    case BehaviorTreeBlackboardComparison::greater:
    case BehaviorTreeBlackboardComparison::greater_or_equal:
        return true;
    }
    return false;
}

[[nodiscard]] bool is_valid_blackboard_comparison(const BehaviorTreeBlackboardComparison comparison,
                                                  const BehaviorTreeBlackboardValueKind kind) noexcept {
    if (!is_known_blackboard_comparison(comparison)) {
        return false;
    }
    if (comparison == BehaviorTreeBlackboardComparison::equal ||
        comparison == BehaviorTreeBlackboardComparison::not_equal) {
        return true;
    }
    return is_ordered_blackboard_value(kind);
}

[[nodiscard]] const BehaviorTreeNodeDesc* find_node(const BehaviorTreeDesc& tree,
                                                    const BehaviorTreeNodeId id) noexcept {
    const auto match =
        std::ranges::find_if(tree.nodes, [id](const BehaviorTreeNodeDesc& node) { return node.id == id; });
    if (match == tree.nodes.end()) {
        return nullptr;
    }
    return &(*match);
}

[[nodiscard]] bool has_duplicate_node_ids(const BehaviorTreeDesc& tree, BehaviorTreeNodeId& duplicate_id) noexcept {
    for (auto first = tree.nodes.begin(); first != tree.nodes.end(); ++first) {
        for (auto second = std::next(first); second != tree.nodes.end(); ++second) {
            if (first->id == second->id) {
                duplicate_id = first->id;
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] bool has_duplicate_blackboard_keys(const std::span<const BehaviorTreeBlackboardEntry> entries,
                                                 std::string_view& duplicate_key) noexcept {
    for (auto first = entries.begin(); first != entries.end(); ++first) {
        for (auto second = std::next(first); second != entries.end(); ++second) {
            if (first->key == second->key) {
                duplicate_key = first->key;
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] bool
has_duplicate_blackboard_conditions(const std::span<const BehaviorTreeBlackboardCondition> conditions,
                                    BehaviorTreeNodeId& duplicate_node_id) noexcept {
    for (auto first = conditions.begin(); first != conditions.end(); ++first) {
        for (auto second = std::next(first); second != conditions.end(); ++second) {
            if (first->node_id == second->node_id) {
                duplicate_node_id = first->node_id;
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] constexpr BehaviorTreeDiagnostic behavior_tree_diagnostic_none() noexcept {
    return BehaviorTreeDiagnostic{.code = BehaviorTreeDiagnosticCode::none, .node_id = {}, .referenced_node_id = {}};
}

[[nodiscard]] BehaviorTreeTickResult make_invalid_result(const BehaviorTreeDiagnosticCode code,
                                                         const BehaviorTreeNodeId node_id,
                                                         const BehaviorTreeNodeId referenced_node_id,
                                                         const std::vector<BehaviorTreeNodeId>& visited_nodes) {
    return BehaviorTreeTickResult{.status = BehaviorTreeStatus::invalid_tree,
                                  .visited_nodes = visited_nodes,
                                  .diagnostic = BehaviorTreeDiagnostic{
                                      .code = code, .node_id = node_id, .referenced_node_id = referenced_node_id}};
}

[[nodiscard]] BehaviorTreeTickResult make_leaf_result_error(const BehaviorTreeStatus status,
                                                            const BehaviorTreeDiagnosticCode code,
                                                            const BehaviorTreeNodeId node_id,
                                                            const std::vector<BehaviorTreeNodeId>& visited_nodes) {
    return BehaviorTreeTickResult{
        .status = status,
        .visited_nodes = visited_nodes,
        .diagnostic = BehaviorTreeDiagnostic{.code = code, .node_id = node_id, .referenced_node_id = {}}};
}

[[nodiscard]] BehaviorTreeTickResult make_condition_failure(const BehaviorTreeDiagnosticCode code,
                                                            const BehaviorTreeNodeId node_id,
                                                            const std::vector<BehaviorTreeNodeId>& visited_nodes) {
    return BehaviorTreeTickResult{
        .status = BehaviorTreeStatus::failure,
        .visited_nodes = visited_nodes,
        .diagnostic = BehaviorTreeDiagnostic{.code = code, .node_id = node_id, .referenced_node_id = {}}};
}

[[nodiscard]] const BehaviorTreeLeafResult* find_leaf_result(const std::span<const BehaviorTreeLeafResult> leaf_results,
                                                             const BehaviorTreeNodeId node_id) noexcept {
    const auto match = std::ranges::find_if(
        leaf_results, [node_id](const BehaviorTreeLeafResult& result) { return result.node_id == node_id; });
    if (match == leaf_results.end()) {
        return nullptr;
    }
    return &(*match);
}

[[nodiscard]] bool has_duplicate_leaf_result(const std::span<const BehaviorTreeLeafResult> leaf_results,
                                             const BehaviorTreeNodeId node_id) noexcept {
    bool seen = false;
    for (const auto& result : leaf_results) {
        if (result.node_id != node_id) {
            continue;
        }
        if (seen) {
            return true;
        }
        seen = true;
    }
    return false;
}

[[nodiscard]] const BehaviorTreeBlackboardEntry*
find_blackboard_entry(const std::span<const BehaviorTreeBlackboardEntry> entries, const std::string_view key) noexcept {
    const auto match =
        std::ranges::find_if(entries, [key](const BehaviorTreeBlackboardEntry& entry) { return entry.key == key; });
    if (match == entries.end()) {
        return nullptr;
    }
    return &(*match);
}

[[nodiscard]] const BehaviorTreeBlackboardCondition*
find_blackboard_condition(const std::span<const BehaviorTreeBlackboardCondition> conditions,
                          const BehaviorTreeNodeId node_id) noexcept {
    const auto match = std::ranges::find_if(conditions, [node_id](const BehaviorTreeBlackboardCondition& condition) {
        return condition.node_id == node_id;
    });
    if (match == conditions.end()) {
        return nullptr;
    }
    return &(*match);
}

[[nodiscard]] bool compare_blackboard_values(const BehaviorTreeBlackboardValue& actual,
                                             const BehaviorTreeBlackboardComparison comparison,
                                             const BehaviorTreeBlackboardValue& expected) noexcept {
    if (actual.kind != expected.kind) {
        return false;
    }

    switch (actual.kind) {
    case BehaviorTreeBlackboardValueKind::boolean:
        if (comparison == BehaviorTreeBlackboardComparison::equal) {
            return actual.bool_value == expected.bool_value;
        }
        if (comparison == BehaviorTreeBlackboardComparison::not_equal) {
            return actual.bool_value != expected.bool_value;
        }
        return false;
    case BehaviorTreeBlackboardValueKind::signed_integer:
        switch (comparison) {
        case BehaviorTreeBlackboardComparison::equal:
            return actual.integer_value == expected.integer_value;
        case BehaviorTreeBlackboardComparison::not_equal:
            return actual.integer_value != expected.integer_value;
        case BehaviorTreeBlackboardComparison::less:
            return actual.integer_value < expected.integer_value;
        case BehaviorTreeBlackboardComparison::less_or_equal:
            return actual.integer_value <= expected.integer_value;
        case BehaviorTreeBlackboardComparison::greater:
            return actual.integer_value > expected.integer_value;
        case BehaviorTreeBlackboardComparison::greater_or_equal:
            return actual.integer_value >= expected.integer_value;
        }
        return false;
    case BehaviorTreeBlackboardValueKind::finite_double:
        switch (comparison) {
        case BehaviorTreeBlackboardComparison::equal:
            return actual.double_value == expected.double_value;
        case BehaviorTreeBlackboardComparison::not_equal:
            return actual.double_value != expected.double_value;
        case BehaviorTreeBlackboardComparison::less:
            return actual.double_value < expected.double_value;
        case BehaviorTreeBlackboardComparison::less_or_equal:
            return actual.double_value <= expected.double_value;
        case BehaviorTreeBlackboardComparison::greater:
            return actual.double_value > expected.double_value;
        case BehaviorTreeBlackboardComparison::greater_or_equal:
            return actual.double_value >= expected.double_value;
        }
        return false;
    case BehaviorTreeBlackboardValueKind::string:
        if (comparison == BehaviorTreeBlackboardComparison::equal) {
            return actual.string_value == expected.string_value;
        }
        if (comparison == BehaviorTreeBlackboardComparison::not_equal) {
            return actual.string_value != expected.string_value;
        }
        return false;
    }

    return false;
}

class BehaviorTreeEvaluator {
  public:
    BehaviorTreeEvaluator(const BehaviorTreeDesc& tree, BehaviorTreeEvaluationContext context)
        : tree_(std::addressof(tree)), context_(context) {}

    [[nodiscard]] BehaviorTreeTickResult evaluate() {
        BehaviorTreeNodeId duplicate_id{};
        if (has_duplicate_node_ids(*tree_, duplicate_id)) {
            return make_invalid_result(BehaviorTreeDiagnosticCode::duplicate_node_id, duplicate_id, {}, visited_nodes_);
        }

        const auto* const root = find_node(*tree_, tree_->root_id);
        if (root == nullptr) {
            return make_invalid_result(BehaviorTreeDiagnosticCode::missing_root, tree_->root_id, {}, visited_nodes_);
        }

        auto context_validation = validate_context();
        if (context_validation.diagnostic.code != BehaviorTreeDiagnosticCode::none) {
            return context_validation;
        }

        return evaluate_node(*root);
    }

  private:
    [[nodiscard]] BehaviorTreeTickResult validate_context() const {
        for (const auto& entry : context_.blackboard_entries) {
            if (!is_valid_blackboard_key(entry.key)) {
                return make_invalid_result(BehaviorTreeDiagnosticCode::invalid_blackboard_key, {}, {}, visited_nodes_);
            }
            if (!is_valid_blackboard_value(entry.value)) {
                return make_invalid_result(BehaviorTreeDiagnosticCode::invalid_blackboard_value, {}, {},
                                           visited_nodes_);
            }
        }

        std::string_view duplicate_key;
        if (has_duplicate_blackboard_keys(context_.blackboard_entries, duplicate_key)) {
            return make_invalid_result(BehaviorTreeDiagnosticCode::duplicate_blackboard_key, {}, {}, visited_nodes_);
        }

        BehaviorTreeNodeId duplicate_condition_id{};
        if (has_duplicate_blackboard_conditions(context_.blackboard_conditions, duplicate_condition_id)) {
            return make_invalid_result(BehaviorTreeDiagnosticCode::duplicate_blackboard_condition,
                                       duplicate_condition_id, {}, visited_nodes_);
        }

        for (const auto& condition : context_.blackboard_conditions) {
            if (!is_valid_blackboard_key(condition.key)) {
                return make_invalid_result(BehaviorTreeDiagnosticCode::invalid_blackboard_condition, condition.node_id,
                                           {}, visited_nodes_);
            }
            if (!is_valid_blackboard_value(condition.expected)) {
                return make_invalid_result(BehaviorTreeDiagnosticCode::invalid_blackboard_value, condition.node_id, {},
                                           visited_nodes_);
            }
            if (!is_valid_blackboard_comparison(condition.comparison, condition.expected.kind)) {
                return make_invalid_result(BehaviorTreeDiagnosticCode::invalid_blackboard_operator, condition.node_id,
                                           {}, visited_nodes_);
            }

            const auto* const node = find_node(*tree_, condition.node_id);
            if (node == nullptr || node->kind != BehaviorTreeNodeKind::condition) {
                return make_invalid_result(BehaviorTreeDiagnosticCode::invalid_blackboard_condition, condition.node_id,
                                           {}, visited_nodes_);
            }
        }

        return BehaviorTreeTickResult{.status = BehaviorTreeStatus::success,
                                      .visited_nodes = visited_nodes_,
                                      .diagnostic = behavior_tree_diagnostic_none()};
    }

    // Behavior-tree evaluation is intentionally recursive. Depth is bounded by `BehaviorTreeDesc::max_depth` and
    // `active_stack_` detects cycles; an iterative rewrite would duplicate substantial policy surface without benefit.
    // NOLINTBEGIN(misc-no-recursion)
    [[nodiscard]] BehaviorTreeTickResult evaluate_node(const BehaviorTreeNodeDesc& node) {
        if (std::ranges::find(active_stack_, node.id) != active_stack_.end()) {
            return make_invalid_result(BehaviorTreeDiagnosticCode::cycle_detected, node.id, node.id, visited_nodes_);
        }
        if (tree_->max_depth == 0 || active_stack_.size() >= tree_->max_depth) {
            return make_invalid_result(BehaviorTreeDiagnosticCode::max_depth_exceeded, node.id, {}, visited_nodes_);
        }

        visited_nodes_.push_back(node.id);
        active_stack_.push_back(node.id);

        auto result = [&]() -> BehaviorTreeTickResult {
            switch (node.kind) {
            case BehaviorTreeNodeKind::sequence:
                return evaluate_sequence(node);
            case BehaviorTreeNodeKind::selector:
                return evaluate_selector(node);
            case BehaviorTreeNodeKind::action:
            case BehaviorTreeNodeKind::condition:
                return evaluate_leaf(node);
            }

            return make_invalid_result(BehaviorTreeDiagnosticCode::invalid_node_kind, node.id, {}, visited_nodes_);
        }();

        if (!active_stack_.empty() && active_stack_.back() == node.id) {
            active_stack_.pop_back();
        }
        return result;
    }

    [[nodiscard]] BehaviorTreeTickResult evaluate_sequence(const BehaviorTreeNodeDesc& node) {
        if (node.children.empty()) {
            return make_invalid_result(BehaviorTreeDiagnosticCode::empty_composite, node.id, {}, visited_nodes_);
        }

        for (const auto child_id : node.children) {
            const auto* const child = find_node(*tree_, child_id);
            if (child == nullptr) {
                return make_invalid_result(BehaviorTreeDiagnosticCode::missing_child, node.id, child_id,
                                           visited_nodes_);
            }
            auto child_result = evaluate_node(*child);
            if (child_result.status != BehaviorTreeStatus::success) {
                return child_result;
            }
        }

        return BehaviorTreeTickResult{.status = BehaviorTreeStatus::success,
                                      .visited_nodes = visited_nodes_,
                                      .diagnostic = behavior_tree_diagnostic_none()};
    }

    [[nodiscard]] BehaviorTreeTickResult evaluate_selector(const BehaviorTreeNodeDesc& node) {
        if (node.children.empty()) {
            return make_invalid_result(BehaviorTreeDiagnosticCode::empty_composite, node.id, {}, visited_nodes_);
        }

        for (const auto child_id : node.children) {
            const auto* const child = find_node(*tree_, child_id);
            if (child == nullptr) {
                return make_invalid_result(BehaviorTreeDiagnosticCode::missing_child, node.id, child_id,
                                           visited_nodes_);
            }
            auto child_result = evaluate_node(*child);
            if (child_result.status == BehaviorTreeStatus::success ||
                child_result.status == BehaviorTreeStatus::running) {
                return child_result;
            }
            if (child_result.diagnostic.code != BehaviorTreeDiagnosticCode::none) {
                return child_result;
            }
            if (child_result.status != BehaviorTreeStatus::failure) {
                return child_result;
            }
        }

        return BehaviorTreeTickResult{.status = BehaviorTreeStatus::failure,
                                      .visited_nodes = visited_nodes_,
                                      .diagnostic = behavior_tree_diagnostic_none()};
    }
    // NOLINTEND(misc-no-recursion)

    [[nodiscard]] BehaviorTreeTickResult evaluate_leaf(const BehaviorTreeNodeDesc& node) {
        if (!node.children.empty()) {
            return make_invalid_result(BehaviorTreeDiagnosticCode::leaf_has_children, node.id, node.children.front(),
                                       visited_nodes_);
        }
        if (node.kind == BehaviorTreeNodeKind::condition) {
            const auto* const blackboard_condition = find_blackboard_condition(context_.blackboard_conditions, node.id);
            if (blackboard_condition != nullptr) {
                return evaluate_blackboard_condition(node.id, *blackboard_condition);
            }
        }

        if (has_duplicate_leaf_result(context_.leaf_results, node.id)) {
            return make_leaf_result_error(BehaviorTreeStatus::invalid_leaf_result,
                                          BehaviorTreeDiagnosticCode::duplicate_leaf_result, node.id, visited_nodes_);
        }
        const auto* const leaf_result = find_leaf_result(context_.leaf_results, node.id);
        if (leaf_result == nullptr) {
            return make_leaf_result_error(BehaviorTreeStatus::missing_leaf_result,
                                          BehaviorTreeDiagnosticCode::missing_leaf_result, node.id, visited_nodes_);
        }
        if (!is_runtime_leaf_status(leaf_result->status)) {
            return make_leaf_result_error(BehaviorTreeStatus::invalid_leaf_result,
                                          BehaviorTreeDiagnosticCode::invalid_leaf_result, node.id, visited_nodes_);
        }

        return BehaviorTreeTickResult{.status = leaf_result->status,
                                      .visited_nodes = visited_nodes_,
                                      .diagnostic = behavior_tree_diagnostic_none()};
    }

    [[nodiscard]] BehaviorTreeTickResult
    evaluate_blackboard_condition(const BehaviorTreeNodeId node_id, const BehaviorTreeBlackboardCondition& condition) {
        const auto* const entry = find_blackboard_entry(context_.blackboard_entries, condition.key);
        if (entry == nullptr) {
            return make_condition_failure(BehaviorTreeDiagnosticCode::missing_blackboard_key, node_id, visited_nodes_);
        }
        if (entry->value.kind != condition.expected.kind) {
            return make_condition_failure(BehaviorTreeDiagnosticCode::blackboard_type_mismatch, node_id,
                                          visited_nodes_);
        }

        return BehaviorTreeTickResult{
            .status = compare_blackboard_values(entry->value, condition.comparison, condition.expected)
                          ? BehaviorTreeStatus::success
                          : BehaviorTreeStatus::failure,
            .visited_nodes = visited_nodes_,
            .diagnostic = behavior_tree_diagnostic_none(),
        };
    }

    const BehaviorTreeDesc* tree_;
    BehaviorTreeEvaluationContext context_;
    std::vector<BehaviorTreeNodeId> visited_nodes_;
    std::vector<BehaviorTreeNodeId> active_stack_;
};

} // namespace

BehaviorTreeBlackboardValue make_behavior_tree_blackboard_bool(const bool value) {
    return BehaviorTreeBlackboardValue{.kind = BehaviorTreeBlackboardValueKind::boolean,
                                       .bool_value = value,
                                       .integer_value = {},
                                       .double_value = {},
                                       .string_value = {}};
}

BehaviorTreeBlackboardValue make_behavior_tree_blackboard_integer(const std::int64_t value) {
    return BehaviorTreeBlackboardValue{.kind = BehaviorTreeBlackboardValueKind::signed_integer,
                                       .bool_value = {},
                                       .integer_value = value,
                                       .double_value = {},
                                       .string_value = {}};
}

BehaviorTreeBlackboardValue make_behavior_tree_blackboard_double(const double value) {
    return BehaviorTreeBlackboardValue{.kind = BehaviorTreeBlackboardValueKind::finite_double,
                                       .bool_value = {},
                                       .integer_value = {},
                                       .double_value = value,
                                       .string_value = {}};
}

BehaviorTreeBlackboardValue make_behavior_tree_blackboard_string(std::string value) {
    return BehaviorTreeBlackboardValue{.kind = BehaviorTreeBlackboardValueKind::string,
                                       .bool_value = {},
                                       .integer_value = {},
                                       .double_value = {},
                                       .string_value = std::move(value)};
}

BehaviorTreeBlackboardValue make_behavior_tree_blackboard_string(const std::string_view value) {
    return make_behavior_tree_blackboard_string(std::string{value});
}

BehaviorTreeBlackboardValue make_behavior_tree_blackboard_string(const char* const value) {
    return make_behavior_tree_blackboard_string(value == nullptr ? std::string{} : std::string{value});
}

bool BehaviorTreeBlackboard::set(std::string key, BehaviorTreeBlackboardValue value) {
    if (!is_valid_blackboard_key(key) || !is_valid_blackboard_value(value)) {
        return false;
    }

    const auto match =
        std::ranges::find_if(entries_, [&key](const BehaviorTreeBlackboardEntry& entry) { return entry.key == key; });
    if (match != entries_.end()) {
        match->value = std::move(value);
        return true;
    }

    entries_.push_back(BehaviorTreeBlackboardEntry{.key = std::move(key), .value = std::move(value)});
    return true;
}

const BehaviorTreeBlackboardValue* BehaviorTreeBlackboard::find(const std::string_view key) const noexcept {
    const auto match =
        std::ranges::find_if(entries_, [key](const BehaviorTreeBlackboardEntry& entry) { return entry.key == key; });
    if (match == entries_.end()) {
        return nullptr;
    }
    return &match->value;
}

std::span<const BehaviorTreeBlackboardEntry> BehaviorTreeBlackboard::entries() const noexcept {
    return entries_;
}

BehaviorTreeTickResult evaluate_behavior_tree(const BehaviorTreeDesc& tree,
                                              const BehaviorTreeEvaluationContext context) {
    return BehaviorTreeEvaluator{tree, context}.evaluate();
}

BehaviorTreeTickResult evaluate_behavior_tree(const BehaviorTreeDesc& tree,
                                              const std::span<const BehaviorTreeLeafResult> leaf_results) {
    return evaluate_behavior_tree(tree, BehaviorTreeEvaluationContext{.leaf_results = leaf_results,
                                                                      .blackboard_entries = {},
                                                                      .blackboard_conditions = {}});
}

BehaviorTreeTickResult evaluate_behavior_tree(const BehaviorTreeDesc& tree,
                                              const std::vector<BehaviorTreeLeafResult>& leaf_results) {
    return evaluate_behavior_tree(tree, std::span<const BehaviorTreeLeafResult>{leaf_results});
}

BehaviorTreeTickResult evaluate_behavior_tree(const BehaviorTreeDesc& tree,
                                              const std::initializer_list<BehaviorTreeLeafResult> leaf_results) {
    return evaluate_behavior_tree(tree,
                                  std::span<const BehaviorTreeLeafResult>{leaf_results.begin(), leaf_results.size()});
}

} // namespace mirakana
