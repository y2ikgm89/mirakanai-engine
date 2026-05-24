// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ai/perception.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {
namespace {

constexpr float k_two_pi = 6.28318530718F;
constexpr float k_distance_epsilon = 0.000001F;

[[nodiscard]] bool is_finite_point(const AiPerceptionPoint2 point) noexcept {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

[[nodiscard]] bool is_non_negative_finite(const float value) noexcept {
    return std::isfinite(value) && value >= 0.0F;
}

[[nodiscard]] float length_squared(const AiPerceptionPoint2 point) noexcept {
    return (point.x * point.x) + (point.y * point.y);
}

[[nodiscard]] AiPerceptionSnapshot2D make_agent_error(const AiPerceptionDiagnostic diagnostic) {
    return AiPerceptionSnapshot2D{
        .status = AiPerceptionStatus::invalid_agent,
        .diagnostic = diagnostic,
        .diagnostic_target_id = {},
        .targets = {},
        .has_primary_target = false,
        .primary_target = {},
        .visible_count = 0U,
        .audible_count = 0U,
    };
}

[[nodiscard]] AiPerceptionSnapshot2D make_target_error(const AiPerceptionDiagnostic diagnostic,
                                                       const AiPerceptionEntityId target_id) {
    return AiPerceptionSnapshot2D{
        .status = diagnostic == AiPerceptionDiagnostic::duplicate_target_id ? AiPerceptionStatus::duplicate_target_id
                                                                            : AiPerceptionStatus::invalid_target,
        .diagnostic = diagnostic,
        .diagnostic_target_id = target_id,
        .targets = {},
        .has_primary_target = false,
        .primary_target = {},
        .visible_count = 0U,
        .audible_count = 0U,
    };
}

[[nodiscard]] bool validate_agent(const AiPerceptionAgent2D& agent, AiPerceptionDiagnostic& diagnostic) noexcept {
    if (agent.id == 0U) {
        diagnostic = AiPerceptionDiagnostic::invalid_agent_id;
        return false;
    }
    if (!is_finite_point(agent.position)) {
        diagnostic = AiPerceptionDiagnostic::invalid_agent_position;
        return false;
    }
    if (!is_finite_point(agent.forward) || length_squared(agent.forward) <= k_distance_epsilon) {
        diagnostic = AiPerceptionDiagnostic::invalid_agent_forward;
        return false;
    }
    if (!is_non_negative_finite(agent.sight_range)) {
        diagnostic = AiPerceptionDiagnostic::invalid_sight_range;
        return false;
    }
    if (!is_non_negative_finite(agent.hearing_range)) {
        diagnostic = AiPerceptionDiagnostic::invalid_hearing_range;
        return false;
    }
    if (!std::isfinite(agent.field_of_view_radians) || agent.field_of_view_radians <= 0.0F ||
        agent.field_of_view_radians > k_two_pi) {
        diagnostic = AiPerceptionDiagnostic::invalid_field_of_view;
        return false;
    }

    diagnostic = AiPerceptionDiagnostic::none;
    return true;
}

[[nodiscard]] bool validate_target(const AiPerceptionTarget2D& target, AiPerceptionDiagnostic& diagnostic) noexcept {
    if (target.id == 0U) {
        diagnostic = AiPerceptionDiagnostic::invalid_target_id;
        return false;
    }
    if (!is_finite_point(target.position)) {
        diagnostic = AiPerceptionDiagnostic::invalid_target_position;
        return false;
    }
    if (!is_non_negative_finite(target.radius)) {
        diagnostic = AiPerceptionDiagnostic::invalid_target_radius;
        return false;
    }
    if (!is_non_negative_finite(target.sound_radius)) {
        diagnostic = AiPerceptionDiagnostic::invalid_target_signal_range;
        return false;
    }

    diagnostic = AiPerceptionDiagnostic::none;
    return true;
}

[[nodiscard]] bool has_duplicate_target_id(const std::span<const AiPerceptionTarget2D> targets,
                                           AiPerceptionEntityId& duplicate_id) noexcept {
    for (auto first = targets.begin(); first != targets.end(); ++first) {
        for (auto second = std::next(first); second != targets.end(); ++second) {
            if (first->id == second->id) {
                duplicate_id = first->id;
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] bool target_in_field_of_view(const AiPerceptionAgent2D& agent, const AiPerceptionTarget2D& target,
                                           const float center_distance) noexcept {
    if (agent.field_of_view_radians >= k_two_pi) {
        return true;
    }
    if (center_distance <= k_distance_epsilon) {
        return true;
    }

    const AiPerceptionPoint2 to_target{
        .x = target.position.x - agent.position.x,
        .y = target.position.y - agent.position.y,
    };
    const auto forward_length = std::sqrt(length_squared(agent.forward));
    const auto target_length = std::sqrt(length_squared(to_target));
    if (forward_length <= k_distance_epsilon || target_length <= k_distance_epsilon) {
        return true;
    }

    const auto dot = ((agent.forward.x / forward_length) * (to_target.x / target_length)) +
                     ((agent.forward.y / forward_length) * (to_target.y / target_length));
    const auto clamped_dot = std::clamp(dot, -1.0F, 1.0F);
    const auto threshold = std::cos(agent.field_of_view_radians * 0.5F);
    return clamped_dot >= threshold;
}

[[nodiscard]] bool perceived_target_less(const AiPerceivedTarget2D& lhs, const AiPerceivedTarget2D& rhs) noexcept {
    const auto lhs_rank = lhs.visible ? 0 : (lhs.audible ? 1 : 2);
    const auto rhs_rank = rhs.visible ? 0 : (rhs.audible ? 1 : 2);
    if (lhs_rank != rhs_rank) {
        return lhs_rank < rhs_rank;
    }
    if (lhs.distance != rhs.distance) {
        return lhs.distance < rhs.distance;
    }
    return lhs.id < rhs.id;
}

[[nodiscard]] bool is_valid_key(const std::string_view key) noexcept {
    return !key.empty();
}

[[nodiscard]] bool has_unique_keys(const AiPerceptionBlackboardKeys& keys) noexcept {
    const std::array<std::string_view, 6> key_views{
        keys.has_target_key,    keys.target_id_key,     keys.target_distance_key,
        keys.visible_count_key, keys.audible_count_key, keys.target_state_key,
    };

    for (auto first = key_views.begin(); first != key_views.end(); ++first) {
        for (auto second = std::next(first); second != key_views.end(); ++second) {
            if (*first == *second) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] AiPerceptionBlackboardResult make_blackboard_result(const AiPerceptionBlackboardStatus status,
                                                                  const AiPerceptionDiagnostic diagnostic) noexcept {
    return AiPerceptionBlackboardResult{.status = status, .diagnostic = diagnostic};
}

void append_readiness_diagnostic(AiPerceptionReadinessReport& report,
                                 const AiPerceptionReadinessDiagnostic diagnostic) {
    if (diagnostic == AiPerceptionReadinessDiagnostic::none) {
        return;
    }
    if (report.diagnostics.empty()) {
        report.diagnostic = diagnostic;
    }
    report.diagnostics.push_back(diagnostic);
}

[[nodiscard]] bool point_matches(const AiPerceptionPoint2 lhs, const AiPerceptionPoint2 rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

[[nodiscard]] bool perceived_target_matches(const AiPerceivedTarget2D& lhs, const AiPerceivedTarget2D& rhs) noexcept {
    return lhs.id == rhs.id && point_matches(lhs.position, rhs.position) && lhs.distance == rhs.distance &&
           lhs.visible == rhs.visible && lhs.audible == rhs.audible;
}

[[nodiscard]] bool snapshot_matches(const AiPerceptionSnapshot2D& lhs, const AiPerceptionSnapshot2D& rhs) noexcept {
    if (lhs.status != rhs.status || lhs.diagnostic != rhs.diagnostic ||
        lhs.diagnostic_target_id != rhs.diagnostic_target_id || lhs.has_primary_target != rhs.has_primary_target ||
        lhs.visible_count != rhs.visible_count || lhs.audible_count != rhs.audible_count ||
        lhs.targets.size() != rhs.targets.size()) {
        return false;
    }
    if (lhs.has_primary_target && !perceived_target_matches(lhs.primary_target, rhs.primary_target)) {
        return false;
    }
    for (std::size_t index = 0U; index < lhs.targets.size(); ++index) {
        if (!perceived_target_matches(lhs.targets[index], rhs.targets[index])) {
            return false;
        }
    }
    return true;
}

} // namespace

AiPerceptionSnapshot2D build_ai_perception_snapshot_2d(const AiPerceptionRequest2D request) {
    AiPerceptionDiagnostic diagnostic{AiPerceptionDiagnostic::none};
    if (!validate_agent(request.agent, diagnostic)) {
        return make_agent_error(diagnostic);
    }

    for (const auto& target : request.targets) {
        if (!validate_target(target, diagnostic)) {
            return make_target_error(diagnostic, target.id);
        }
    }

    AiPerceptionEntityId duplicate_id{};
    if (has_duplicate_target_id(request.targets, duplicate_id)) {
        return make_target_error(AiPerceptionDiagnostic::duplicate_target_id, duplicate_id);
    }

    std::vector<AiPerceivedTarget2D> perceived_targets;
    perceived_targets.reserve(request.targets.size());
    std::size_t visible_count = 0U;
    std::size_t audible_count = 0U;

    for (const auto& target : request.targets) {
        const auto dx = target.position.x - request.agent.position.x;
        const auto dy = target.position.y - request.agent.position.y;
        const auto distance_squared = (dx * dx) + (dy * dy);
        if (!std::isfinite(distance_squared)) {
            return make_target_error(AiPerceptionDiagnostic::calculation_overflow, target.id);
        }

        const auto center_distance = std::sqrt(distance_squared);
        if (!std::isfinite(center_distance)) {
            return make_target_error(AiPerceptionDiagnostic::calculation_overflow, target.id);
        }
        const auto effective_distance = std::max(0.0F, center_distance - target.radius);
        const auto hearing_extent = request.agent.hearing_range + target.sound_radius;
        if (!std::isfinite(hearing_extent)) {
            return make_target_error(AiPerceptionDiagnostic::calculation_overflow, target.id);
        }

        const auto visible = target.sight_enabled && effective_distance <= request.agent.sight_range &&
                             target_in_field_of_view(request.agent, target, center_distance);
        const auto audible = target.hearing_enabled && effective_distance <= hearing_extent;
        if (!visible && !audible) {
            continue;
        }

        visible_count += visible ? 1U : 0U;
        audible_count += audible ? 1U : 0U;
        perceived_targets.push_back(AiPerceivedTarget2D{
            .id = target.id,
            .position = target.position,
            .distance = effective_distance,
            .visible = visible,
            .audible = audible,
        });
    }

    std::ranges::sort(perceived_targets, perceived_target_less);

    const auto has_primary = !perceived_targets.empty();
    const auto primary_target = has_primary ? perceived_targets.front() : AiPerceivedTarget2D{};
    return AiPerceptionSnapshot2D{
        .status = AiPerceptionStatus::ready,
        .diagnostic = AiPerceptionDiagnostic::none,
        .diagnostic_target_id = {},
        .targets = std::move(perceived_targets),
        .has_primary_target = has_primary,
        .primary_target = primary_target,
        .visible_count = visible_count,
        .audible_count = audible_count,
    };
}

AiPerceptionBlackboardResult write_ai_perception_blackboard(const AiPerceptionSnapshot2D& snapshot,
                                                            const AiPerceptionBlackboardKeys& keys,
                                                            BehaviorTreeBlackboard& blackboard) {
    if (snapshot.status != AiPerceptionStatus::ready || snapshot.diagnostic != AiPerceptionDiagnostic::none) {
        return make_blackboard_result(AiPerceptionBlackboardStatus::invalid_snapshot,
                                      AiPerceptionDiagnostic::snapshot_not_ready);
    }

    if (!is_valid_key(keys.has_target_key) || !is_valid_key(keys.target_id_key) ||
        !is_valid_key(keys.target_distance_key) || !is_valid_key(keys.visible_count_key) ||
        !is_valid_key(keys.audible_count_key) || !is_valid_key(keys.target_state_key) || !has_unique_keys(keys)) {
        return make_blackboard_result(AiPerceptionBlackboardStatus::invalid_key,
                                      AiPerceptionDiagnostic::invalid_blackboard_key);
    }

    const auto target_id = snapshot.has_primary_target ? static_cast<std::int64_t>(snapshot.primary_target.id) : 0;
    const auto target_distance =
        snapshot.has_primary_target ? static_cast<double>(snapshot.primary_target.distance) : 0.0;
    const auto target_state = [&]() -> std::string {
        if (!snapshot.has_primary_target) {
            return "none";
        }
        return snapshot.primary_target.visible ? "visible" : "audible";
    }();

    const auto written =
        blackboard.set(keys.has_target_key, make_behavior_tree_blackboard_bool(snapshot.has_primary_target)) &&
        blackboard.set(keys.target_id_key, make_behavior_tree_blackboard_integer(target_id)) &&
        blackboard.set(keys.target_distance_key, make_behavior_tree_blackboard_double(target_distance)) &&
        blackboard.set(keys.visible_count_key,
                       make_behavior_tree_blackboard_integer(static_cast<std::int64_t>(snapshot.visible_count))) &&
        blackboard.set(keys.audible_count_key,
                       make_behavior_tree_blackboard_integer(static_cast<std::int64_t>(snapshot.audible_count))) &&
        blackboard.set(keys.target_state_key, make_behavior_tree_blackboard_string(target_state));

    if (!written) {
        return make_blackboard_result(AiPerceptionBlackboardStatus::blackboard_write_failed,
                                      AiPerceptionDiagnostic::blackboard_set_failed);
    }

    return make_blackboard_result(AiPerceptionBlackboardStatus::ready, AiPerceptionDiagnostic::none);
}

AiPerceptionReadinessReport evaluate_ai_perception_readiness_2d(const AiPerceptionRequest2D request,
                                                                const AiPerceptionBlackboardKeys& keys,
                                                                const AiPerceptionReadinessConfig& config) {
    const auto first_snapshot = build_ai_perception_snapshot_2d(request);
    const auto second_snapshot = build_ai_perception_snapshot_2d(request);

    AiPerceptionReadinessReport report{
        .status = AiPerceptionReadinessStatus::diagnostics,
        .diagnostic = AiPerceptionReadinessDiagnostic::none,
        .diagnostics = {},
        .snapshot_status = first_snapshot.status,
        .snapshot_diagnostic = first_snapshot.diagnostic,
        .blackboard_status = AiPerceptionBlackboardStatus::invalid_snapshot,
        .blackboard_diagnostic = AiPerceptionDiagnostic::none,
        .stable_primary_target_ready = snapshot_matches(first_snapshot, second_snapshot),
        .blackboard_projection_ready = false,
        .target_count = first_snapshot.targets.size(),
        .visible_count = first_snapshot.visible_count,
        .audible_count = first_snapshot.audible_count,
        .has_primary_target = first_snapshot.has_primary_target,
        .primary_target_id = first_snapshot.has_primary_target ? first_snapshot.primary_target.id : 0U,
        .primary_target_distance = first_snapshot.has_primary_target ? first_snapshot.primary_target.distance : 0.0F,
        .primary_target_visible = first_snapshot.has_primary_target && first_snapshot.primary_target.visible,
        .primary_target_audible = first_snapshot.has_primary_target && first_snapshot.primary_target.audible,
    };

    if (first_snapshot.status != AiPerceptionStatus::ready ||
        first_snapshot.diagnostic != AiPerceptionDiagnostic::none) {
        append_readiness_diagnostic(report, AiPerceptionReadinessDiagnostic::invalid_snapshot);
        report.status = AiPerceptionReadinessStatus::invalid_snapshot;
        return report;
    }

    BehaviorTreeBlackboard blackboard;
    const auto blackboard_result = write_ai_perception_blackboard(first_snapshot, keys, blackboard);
    report.blackboard_status = blackboard_result.status;
    report.blackboard_diagnostic = blackboard_result.diagnostic;
    report.blackboard_projection_ready = blackboard_result.status == AiPerceptionBlackboardStatus::ready &&
                                         blackboard_result.diagnostic == AiPerceptionDiagnostic::none;

    if (config.require_blackboard_projection && !report.blackboard_projection_ready) {
        append_readiness_diagnostic(report, AiPerceptionReadinessDiagnostic::blackboard_projection_failed);
    }
    if (config.require_stable_primary_target && !report.stable_primary_target_ready) {
        append_readiness_diagnostic(report, AiPerceptionReadinessDiagnostic::unstable_primary_target);
    }
    if (report.target_count < config.min_targets) {
        append_readiness_diagnostic(report, AiPerceptionReadinessDiagnostic::insufficient_targets);
    }
    if (config.require_primary_target && !report.has_primary_target) {
        append_readiness_diagnostic(report, AiPerceptionReadinessDiagnostic::missing_primary_target);
    }
    if (config.require_visible_target && report.visible_count == 0U) {
        append_readiness_diagnostic(report, AiPerceptionReadinessDiagnostic::missing_visible_target);
    }
    if (config.require_audible_target && report.audible_count == 0U) {
        append_readiness_diagnostic(report, AiPerceptionReadinessDiagnostic::missing_audible_target);
    }
    if (report.target_count > config.max_targets) {
        append_readiness_diagnostic(report, AiPerceptionReadinessDiagnostic::target_budget_exceeded);
    }

    report.status =
        report.diagnostics.empty() ? AiPerceptionReadinessStatus::ready : AiPerceptionReadinessStatus::diagnostics;
    return report;
}

} // namespace mirakana
