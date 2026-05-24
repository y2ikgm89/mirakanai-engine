// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ai/behavior_tree.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

using AiPerceptionEntityId = std::uint32_t;

struct AiPerceptionPoint2 {
    float x{0.0F};
    float y{0.0F};
};

enum class AiPerceptionStatus : std::uint8_t {
    ready,
    invalid_agent,
    invalid_target,
    duplicate_target_id,
};

enum class AiPerceptionBlackboardStatus : std::uint8_t {
    ready,
    invalid_snapshot,
    invalid_key,
    blackboard_write_failed,
};

enum class AiPerceptionReadinessStatus : std::uint8_t {
    ready,
    diagnostics,
    invalid_snapshot,
};

enum class AiPerceptionDiagnostic : std::uint8_t {
    none,
    invalid_agent_id,
    invalid_agent_position,
    invalid_agent_forward,
    invalid_sight_range,
    invalid_hearing_range,
    invalid_field_of_view,
    invalid_target_id,
    invalid_target_position,
    invalid_target_radius,
    invalid_target_signal_range,
    duplicate_target_id,
    calculation_overflow,
    snapshot_not_ready,
    invalid_blackboard_key,
    blackboard_set_failed,
};

enum class AiPerceptionReadinessDiagnostic : std::uint8_t {
    none,
    invalid_snapshot,
    blackboard_projection_failed,
    unstable_primary_target,
    insufficient_targets,
    missing_primary_target,
    missing_visible_target,
    missing_audible_target,
    target_budget_exceeded,
};

struct AiPerceptionAgent2D {
    AiPerceptionEntityId id{};
    AiPerceptionPoint2 position{};
    AiPerceptionPoint2 forward{.x = 1.0F, .y = 0.0F};
    float sight_range{0.0F};
    float field_of_view_radians{6.28318530718F};
    float hearing_range{0.0F};
};

struct AiPerceptionTarget2D {
    AiPerceptionEntityId id{};
    AiPerceptionPoint2 position{};
    float radius{0.0F};
    bool sight_enabled{true};
    bool hearing_enabled{false};
    float sound_radius{0.0F};
};

struct AiPerceivedTarget2D {
    AiPerceptionEntityId id{};
    AiPerceptionPoint2 position{};
    float distance{0.0F};
    bool visible{false};
    bool audible{false};
};

struct AiPerceptionRequest2D {
    AiPerceptionAgent2D agent{};
    std::span<const AiPerceptionTarget2D> targets;
};

struct AiPerceptionSnapshot2D {
    AiPerceptionStatus status{AiPerceptionStatus::ready};
    AiPerceptionDiagnostic diagnostic{AiPerceptionDiagnostic::none};
    AiPerceptionEntityId diagnostic_target_id{};
    std::vector<AiPerceivedTarget2D> targets;
    bool has_primary_target{false};
    AiPerceivedTarget2D primary_target{};
    std::size_t visible_count{0U};
    std::size_t audible_count{0U};
};

struct AiPerceptionBlackboardKeys {
    std::string has_target_key{"perception.has_target"};
    std::string target_id_key{"perception.target_id"};
    std::string target_distance_key{"perception.target_distance"};
    std::string visible_count_key{"perception.visible_count"};
    std::string audible_count_key{"perception.audible_count"};
    std::string target_state_key{"perception.target_state"};
};

struct AiPerceptionBlackboardResult {
    AiPerceptionBlackboardStatus status{AiPerceptionBlackboardStatus::ready};
    AiPerceptionDiagnostic diagnostic{AiPerceptionDiagnostic::none};
};

struct AiPerceptionReadinessConfig {
    bool require_stable_primary_target{true};
    bool require_blackboard_projection{true};
    bool require_primary_target{true};
    bool require_visible_target{false};
    bool require_audible_target{false};
    std::size_t min_targets{1};
    std::size_t max_targets{std::numeric_limits<std::size_t>::max()};
};

struct AiPerceptionReadinessReport {
    AiPerceptionReadinessStatus status{AiPerceptionReadinessStatus::invalid_snapshot};
    AiPerceptionReadinessDiagnostic diagnostic{AiPerceptionReadinessDiagnostic::none};
    std::vector<AiPerceptionReadinessDiagnostic> diagnostics;
    AiPerceptionStatus snapshot_status{AiPerceptionStatus::invalid_agent};
    AiPerceptionDiagnostic snapshot_diagnostic{AiPerceptionDiagnostic::none};
    AiPerceptionBlackboardStatus blackboard_status{AiPerceptionBlackboardStatus::invalid_snapshot};
    AiPerceptionDiagnostic blackboard_diagnostic{AiPerceptionDiagnostic::none};
    bool stable_primary_target_ready{false};
    bool blackboard_projection_ready{false};
    std::size_t target_count{0U};
    std::size_t visible_count{0U};
    std::size_t audible_count{0U};
    bool has_primary_target{false};
    AiPerceptionEntityId primary_target_id{};
    float primary_target_distance{0.0F};
    bool primary_target_visible{false};
    bool primary_target_audible{false};
};

[[nodiscard]] AiPerceptionSnapshot2D build_ai_perception_snapshot_2d(AiPerceptionRequest2D request);

[[nodiscard]] AiPerceptionBlackboardResult write_ai_perception_blackboard(const AiPerceptionSnapshot2D& snapshot,
                                                                          const AiPerceptionBlackboardKeys& keys,
                                                                          BehaviorTreeBlackboard& blackboard);

[[nodiscard]] AiPerceptionReadinessReport
evaluate_ai_perception_readiness_2d(AiPerceptionRequest2D request, const AiPerceptionBlackboardKeys& keys = {},
                                    const AiPerceptionReadinessConfig& config = {});

} // namespace mirakana
