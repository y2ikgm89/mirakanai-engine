// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/world_entity_model.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeRpgSystemsStatus : std::uint8_t {
    ready = 0,
    no_rows,
    invalid_request,
};

enum class RuntimeRpgCombatSide : std::uint8_t {
    party = 0,
    enemy,
};

enum class RuntimeRpgCombatActionKind : std::uint8_t {
    act = 0,
};

enum class RuntimeRpgSkillStatus : std::uint8_t {
    unlocked = 0,
    blocked_prerequisite,
    blocked_progression,
    invalid,
};

enum class RuntimeRpgEquipmentStatus : std::uint8_t {
    equipped = 0,
    blocked_slot,
    blocked_requirement,
    invalid,
};

enum class RuntimeRpgRewardKind : std::uint8_t {
    experience = 0,
    currency,
    item,
    flag,
};

enum class RuntimeRpgSaveValidationStatus : std::uint8_t {
    accepted = 0,
    repairable,
    rejected,
};

enum class RuntimeRpgDiagnosticCode : std::uint8_t {
    missing_system_id = 0,
    missing_world_id,
    unsupported_backend_reference,
    missing_actor_id,
    duplicate_actor_id,
    invalid_stat_row,
    duplicate_stat_row,
    unknown_stat_actor,
    invalid_progression_row,
    duplicate_progression_row,
    unknown_progression_actor,
    invalid_skill_row,
    duplicate_skill_id,
    unknown_skill_actor,
    unknown_skill_stat,
    invalid_equipment_row,
    duplicate_equipment_slot,
    unknown_equipment_actor,
    unknown_equipment_stat,
    invalid_combat_participant,
    duplicate_combat_turn_actor,
    invalid_combat_rounds,
    invalid_reward_row,
    duplicate_reward_id,
    unknown_reward_actor,
    invalid_reward_quantity,
    invalid_save_row,
    duplicate_save_key,
    unknown_save_actor,
    save_state_mismatch,
    row_budget_exceeded,
};

struct RuntimeRpgStatRow {
    RuntimeWorldEntityId entity_id;
    std::string stat_id;
    std::int32_t current_value{0};
    std::int32_t max_value{0};
    std::uint32_t source_index{0U};
};

struct RuntimeRpgProgressionRow {
    RuntimeWorldEntityId entity_id;
    std::string track_id;
    std::uint32_t level{1U};
    std::uint64_t experience{0U};
    std::uint32_t skill_points{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeRpgSkillRow {
    RuntimeWorldEntityId entity_id;
    std::string skill_id;
    std::uint32_t required_level{1U};
    std::string required_stat_id;
    std::int32_t required_stat_value{0};
    RuntimeRpgSkillStatus status{RuntimeRpgSkillStatus::invalid};
    std::uint32_t source_index{0U};
};

struct RuntimeRpgEquipmentRow {
    RuntimeWorldEntityId entity_id;
    std::string slot_id;
    std::string item_id;
    std::uint32_t required_level{1U};
    std::string required_stat_id;
    std::int32_t required_stat_value{0};
    RuntimeRpgEquipmentStatus status{RuntimeRpgEquipmentStatus::invalid};
    std::uint32_t source_index{0U};
};

struct RuntimeRpgCombatLoopRequest {
    std::string encounter_id;
    std::string initiative_stat_id;
    std::uint32_t max_rounds{0U};
};

struct RuntimeRpgCombatLoopRow {
    std::string encounter_id;
    std::uint32_t round_index{0U};
    std::uint32_t turn_index{0U};
    RuntimeWorldEntityId entity_id;
    RuntimeRpgCombatSide side{RuntimeRpgCombatSide::party};
    std::int32_t initiative_value{0};
    RuntimeRpgCombatActionKind action_kind{RuntimeRpgCombatActionKind::act};
    std::uint32_t source_index{0U};
};

struct RuntimeRpgRewardRow {
    std::string reward_id;
    RuntimeWorldEntityId entity_id;
    RuntimeRpgRewardKind kind{RuntimeRpgRewardKind::experience};
    std::string item_id;
    std::uint32_t quantity{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeRpgSaveValidationRow {
    RuntimeWorldEntityId entity_id;
    std::string domain;
    std::string key;
    std::uint32_t expected_schema_version{1U};
    std::uint32_t observed_schema_version{1U};
    RuntimeRpgSaveValidationStatus status{RuntimeRpgSaveValidationStatus::rejected};
    std::uint32_t source_index{0U};
};

struct RuntimeRpgSystemsRequest {
    std::string system_id;
    std::string world_id;
    std::uint64_t world_tick{0U};
    std::vector<RuntimeWorldEntityId> party_entity_ids;
    std::vector<RuntimeWorldEntityId> enemy_entity_ids;
    std::vector<RuntimeRpgStatRow> stat_rows;
    std::vector<RuntimeRpgProgressionRow> progression_rows;
    std::vector<RuntimeRpgSkillRow> skill_rows;
    std::vector<RuntimeRpgEquipmentRow> equipment_rows;
    RuntimeRpgCombatLoopRequest combat_request;
    std::vector<RuntimeRpgRewardRow> reward_rows;
    std::vector<RuntimeRpgSaveValidationRow> save_validation_rows;
    std::size_t row_budget{512U};
    std::uint64_t seed{0U};
};

struct RuntimeRpgDiagnostic {
    RuntimeRpgDiagnosticCode code{RuntimeRpgDiagnosticCode::missing_system_id};
    std::string system_id;
    std::string world_id;
    RuntimeWorldEntityId entity_id;
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeRpgSystemsPlan {
    RuntimeRpgSystemsStatus status{RuntimeRpgSystemsStatus::invalid_request};
    std::vector<RuntimeRpgDiagnostic> diagnostics;
    std::vector<RuntimeRpgStatRow> stat_rows;
    std::vector<RuntimeRpgProgressionRow> progression_rows;
    std::vector<RuntimeRpgSkillRow> skill_rows;
    std::vector<RuntimeRpgEquipmentRow> equipment_rows;
    std::vector<RuntimeRpgCombatLoopRow> combat_loop_rows;
    std::vector<RuntimeRpgRewardRow> reward_rows;
    std::vector<RuntimeRpgSaveValidationRow> save_validation_rows;
    std::size_t party_member_count{0U};
    std::size_t enemy_member_count{0U};
    std::size_t stat_count{0U};
    std::size_t progression_count{0U};
    std::size_t unlocked_skill_count{0U};
    std::size_t blocked_skill_count{0U};
    std::size_t equipped_item_count{0U};
    std::size_t blocked_equipment_count{0U};
    std::size_t combat_turn_count{0U};
    std::size_t combat_round_count{0U};
    std::size_t reward_count{0U};
    std::size_t save_validation_count{0U};
    std::size_t repairable_save_validation_count{0U};
    std::uint64_t replay_hash{0U};
    bool invoked_combat_execution{false};
    bool invoked_reward_application{false};
    bool invoked_save_io{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews RPG-oriented runtime value rows and emits deterministic planning/package-evidence rows.
/// This value-only planner does not run combat, mutate scenes, roll loot, write saves, stream packages, create
/// threads, call renderer/platform/editor APIs, or own inventory/quest/procedural-generation semantics.
[[nodiscard]] RuntimeRpgSystemsPlan plan_runtime_rpg_systems(const RuntimeRpgSystemsRequest& request);

} // namespace mirakana::runtime
