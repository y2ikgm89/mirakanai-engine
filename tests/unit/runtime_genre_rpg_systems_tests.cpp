// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/genre_rpg_systems.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace {

using mirakana::runtime::RuntimeRpgCombatActionKind;
using mirakana::runtime::RuntimeRpgCombatSide;
using mirakana::runtime::RuntimeRpgEquipmentStatus;
using mirakana::runtime::RuntimeRpgRewardKind;
using mirakana::runtime::RuntimeRpgSaveValidationStatus;
using mirakana::runtime::RuntimeRpgSkillStatus;
using mirakana::runtime::RuntimeWorldEntityId;

[[nodiscard]] RuntimeWorldEntityId entity(std::string value) {
    return RuntimeWorldEntityId{.value = std::move(value)};
}

[[nodiscard]] mirakana::runtime::RuntimeRpgStatRow make_stat(std::string entity_id, std::string stat_id,
                                                             std::int32_t current_value, std::int32_t max_value,
                                                             std::uint32_t source_index) {
    return mirakana::runtime::RuntimeRpgStatRow{
        .entity_id = entity(std::move(entity_id)),
        .stat_id = std::move(stat_id),
        .current_value = current_value,
        .max_value = max_value,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeRpgProgressionRow
make_progression(std::string entity_id, std::string track_id, std::uint32_t level, std::uint64_t experience,
                 std::uint32_t skill_points, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeRpgProgressionRow{
        .entity_id = entity(std::move(entity_id)),
        .track_id = std::move(track_id),
        .level = level,
        .experience = experience,
        .skill_points = skill_points,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeRpgSkillRow
make_skill(std::string entity_id, std::string skill_id, std::uint32_t required_level, std::string required_stat_id,
           std::int32_t required_stat_value, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeRpgSkillRow{
        .entity_id = entity(std::move(entity_id)),
        .skill_id = std::move(skill_id),
        .required_level = required_level,
        .required_stat_id = std::move(required_stat_id),
        .required_stat_value = required_stat_value,
        .status = RuntimeRpgSkillStatus::invalid,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeRpgEquipmentRow
make_equipment(std::string entity_id, std::string slot_id, std::string item_id, std::uint32_t required_level,
               std::string required_stat_id, std::int32_t required_stat_value, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeRpgEquipmentRow{
        .entity_id = entity(std::move(entity_id)),
        .slot_id = std::move(slot_id),
        .item_id = std::move(item_id),
        .required_level = required_level,
        .required_stat_id = std::move(required_stat_id),
        .required_stat_value = required_stat_value,
        .status = RuntimeRpgEquipmentStatus::invalid,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeRpgRewardRow make_reward(std::string reward_id, std::string entity_id,
                                                                 RuntimeRpgRewardKind kind, std::string item_id,
                                                                 std::uint32_t quantity, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeRpgRewardRow{
        .reward_id = std::move(reward_id),
        .entity_id = entity(std::move(entity_id)),
        .kind = kind,
        .item_id = std::move(item_id),
        .quantity = quantity,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeRpgSaveValidationRow
make_save(std::string entity_id, std::string domain, std::string key, std::uint32_t expected_schema_version,
          std::uint32_t observed_schema_version, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeRpgSaveValidationRow{
        .entity_id = entity(std::move(entity_id)),
        .domain = std::move(domain),
        .key = std::move(key),
        .expected_schema_version = expected_schema_version,
        .observed_schema_version = observed_schema_version,
        .status = RuntimeRpgSaveValidationStatus::rejected,
        .source_index = source_index,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeRpgSystemsPlan& plan,
                                           mirakana::runtime::RuntimeRpgDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] mirakana::runtime::RuntimeRpgSystemsRequest make_hash_sensitive_request() {
    return mirakana::runtime::RuntimeRpgSystemsRequest{
        .system_id = "rpg.hash",
        .world_id = "world.hash",
        .world_tick = 11U,
        .party_entity_ids = {entity("actor.0")},
        .enemy_entity_ids = {},
        .stat_rows =
            {
                make_stat("actor.0", "focus", 9, 10, 1U),
                make_stat("actor.0", "initiative", 5, 5, 2U),
            },
        .progression_rows = {make_progression("actor.0", "track.core", 5U, 100U, 3U, 1U)},
        .skill_rows = {make_skill("actor.0", "skill.primary", 2U, "focus", 6, 1U)},
        .equipment_rows = {make_equipment("actor.0", "slot.primary", "item.primary", 2U, "focus", 5, 1U)},
        .combat_request =
            mirakana::runtime::RuntimeRpgCombatLoopRequest{
                .encounter_id = "encounter.hash",
                .initiative_stat_id = "initiative",
                .max_rounds = 1U,
            },
        .reward_rows = {make_reward("reward.item", "actor.0", RuntimeRpgRewardKind::item, "item.resource", 1U, 1U)},
        .save_validation_rows = {make_save("actor.0", "profile", "state.actor.0", 3U, 2U, 1U)},
    };
}

} // namespace

MK_TEST("runtime RPG systems normalizes stats progression skills equipment combat rewards and saves") {
    using Status = mirakana::runtime::RuntimeRpgSystemsStatus;

    const auto request = mirakana::runtime::RuntimeRpgSystemsRequest{
        .system_id = "rpg.package",
        .world_id = "world.package",
        .world_tick = 42U,
        .party_entity_ids = {entity("party.entity.0"), entity("party.entity.1")},
        .enemy_entity_ids = {entity("opponent.entity.0")},
        .stat_rows =
            {
                make_stat("party.entity.0", "health", 30, 30, 1U),
                make_stat("party.entity.0", "focus", 7, 10, 2U),
                make_stat("party.entity.0", "initiative", 12, 12, 3U),
                make_stat("party.entity.1", "health", 24, 24, 4U),
                make_stat("party.entity.1", "initiative", 8, 8, 5U),
                make_stat("opponent.entity.0", "health", 18, 18, 6U),
                make_stat("opponent.entity.0", "initiative", 10, 10, 7U),
            },
        .progression_rows =
            {
                make_progression("party.entity.0", "track.core", 3U, 120U, 1U, 1U),
                make_progression("party.entity.1", "track.core", 2U, 75U, 0U, 2U),
            },
        .skill_rows =
            {
                make_skill("party.entity.0", "skill.primary", 2U, "focus", 6, 1U),
                make_skill("party.entity.1", "skill.support", 2U, "", 0, 2U),
            },
        .equipment_rows =
            {
                make_equipment("party.entity.0", "slot.primary", "item.primary", 2U, "focus", 5, 1U),
                make_equipment("party.entity.1", "slot.support", "item.support", 2U, "", 0, 2U),
            },
        .combat_request =
            mirakana::runtime::RuntimeRpgCombatLoopRequest{
                .encounter_id = "encounter.package",
                .initiative_stat_id = "initiative",
                .max_rounds = 2U,
            },
        .reward_rows =
            {
                make_reward("reward.progress", "party.entity.0", RuntimeRpgRewardKind::experience, "", 50U, 1U),
                make_reward("reward.item", "party.entity.1", RuntimeRpgRewardKind::item, "item.resource", 1U, 2U),
            },
        .save_validation_rows =
            {
                make_save("party.entity.0", "profile", "state.party.0", 1U, 1U, 1U),
                make_save("party.entity.1", "profile", "state.party.1", 2U, 1U, 2U),
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_rpg_systems(request);

    MK_REQUIRE(plan.status == Status::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.party_member_count == 2U);
    MK_REQUIRE(plan.enemy_member_count == 1U);
    MK_REQUIRE(plan.stat_rows.size() == 7U);
    MK_REQUIRE(plan.progression_rows.size() == 2U);
    MK_REQUIRE(plan.skill_rows.size() == 2U);
    MK_REQUIRE(plan.equipment_rows.size() == 2U);
    MK_REQUIRE(plan.combat_loop_rows.size() == 6U);
    MK_REQUIRE(plan.reward_rows.size() == 2U);
    MK_REQUIRE(plan.save_validation_rows.size() == 2U);
    MK_REQUIRE(plan.stat_count == 7U);
    MK_REQUIRE(plan.progression_count == 2U);
    MK_REQUIRE(plan.unlocked_skill_count == 2U);
    MK_REQUIRE(plan.blocked_skill_count == 0U);
    MK_REQUIRE(plan.equipped_item_count == 2U);
    MK_REQUIRE(plan.blocked_equipment_count == 0U);
    MK_REQUIRE(plan.combat_turn_count == 6U);
    MK_REQUIRE(plan.combat_round_count == 2U);
    MK_REQUIRE(plan.reward_count == 2U);
    MK_REQUIRE(plan.save_validation_count == 2U);
    MK_REQUIRE(plan.repairable_save_validation_count == 1U);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.invoked_combat_execution);
    MK_REQUIRE(!plan.invoked_reward_application);
    MK_REQUIRE(!plan.invoked_save_io);
    MK_REQUIRE(plan.skill_rows[0].skill_id == "skill.primary");
    MK_REQUIRE(plan.skill_rows[0].status == RuntimeRpgSkillStatus::unlocked);
    MK_REQUIRE(plan.skill_rows[1].skill_id == "skill.support");
    MK_REQUIRE(plan.skill_rows[1].status == RuntimeRpgSkillStatus::unlocked);
    MK_REQUIRE(plan.equipment_rows[0].slot_id == "slot.primary");
    MK_REQUIRE(plan.equipment_rows[0].status == RuntimeRpgEquipmentStatus::equipped);
    MK_REQUIRE(plan.combat_loop_rows[0].round_index == 0U);
    MK_REQUIRE(plan.combat_loop_rows[0].entity_id.value == "party.entity.0");
    MK_REQUIRE(plan.combat_loop_rows[0].side == RuntimeRpgCombatSide::party);
    MK_REQUIRE(plan.combat_loop_rows[0].initiative_value == 12);
    MK_REQUIRE(plan.combat_loop_rows[0].action_kind == RuntimeRpgCombatActionKind::act);
    MK_REQUIRE(plan.combat_loop_rows[2].entity_id.value == "party.entity.1");
    MK_REQUIRE(plan.combat_loop_rows[2].side == RuntimeRpgCombatSide::party);
    MK_REQUIRE(plan.combat_loop_rows[3].round_index == 1U);
    MK_REQUIRE(plan.save_validation_rows[0].status == RuntimeRpgSaveValidationStatus::accepted);
    MK_REQUIRE(plan.save_validation_rows[1].status == RuntimeRpgSaveValidationStatus::repairable);
}

MK_TEST("runtime RPG systems keeps blocked skills and equipment as value rows") {
    using Status = mirakana::runtime::RuntimeRpgSystemsStatus;

    const auto plan = mirakana::runtime::plan_runtime_rpg_systems(mirakana::runtime::RuntimeRpgSystemsRequest{
        .system_id = "rpg.locked_rows",
        .world_id = "world.locked_rows",
        .world_tick = 7U,
        .party_entity_ids = {entity("party.entity.0")},
        .enemy_entity_ids = {},
        .stat_rows =
            {
                make_stat("party.entity.0", "focus", 2, 10, 1U),
                make_stat("party.entity.0", "initiative", 3, 3, 2U),
            },
        .progression_rows = {make_progression("party.entity.0", "track.core", 1U, 10U, 0U, 1U)},
        .skill_rows = {make_skill("party.entity.0", "skill.primary", 1U, "focus", 6, 1U)},
        .equipment_rows = {make_equipment("party.entity.0", "slot.primary", "item.primary", 1U, "focus", 5, 1U)},
        .combat_request =
            mirakana::runtime::RuntimeRpgCombatLoopRequest{
                .encounter_id = "encounter.locked",
                .initiative_stat_id = "initiative",
                .max_rounds = 1U,
            },
    });

    MK_REQUIRE(plan.status == Status::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.skill_rows.size() == 1U);
    MK_REQUIRE(plan.skill_rows[0].status == RuntimeRpgSkillStatus::blocked_prerequisite);
    MK_REQUIRE(plan.unlocked_skill_count == 0U);
    MK_REQUIRE(plan.blocked_skill_count == 1U);
    MK_REQUIRE(plan.equipment_rows.size() == 1U);
    MK_REQUIRE(plan.equipment_rows[0].status == RuntimeRpgEquipmentStatus::blocked_requirement);
    MK_REQUIRE(plan.equipped_item_count == 0U);
    MK_REQUIRE(plan.blocked_equipment_count == 1U);
}

MK_TEST("runtime RPG systems replay hash changes when normalized output fields change") {
    using Status = mirakana::runtime::RuntimeRpgSystemsStatus;

    const auto base_request = make_hash_sensitive_request();
    const auto base_plan = mirakana::runtime::plan_runtime_rpg_systems(base_request);

    MK_REQUIRE(base_plan.status == Status::ready);
    MK_REQUIRE(base_plan.replay_hash != 0U);
    MK_REQUIRE(base_plan.progression_rows[0].skill_points == 3U);
    MK_REQUIRE(base_plan.skill_rows[0].required_level == 2U);
    MK_REQUIRE(base_plan.reward_rows[0].item_id == "item.resource");
    MK_REQUIRE(base_plan.save_validation_rows[0].observed_schema_version == 2U);
    MK_REQUIRE(base_plan.combat_loop_rows[0].side == RuntimeRpgCombatSide::party);

    auto changed_progression = base_request;
    changed_progression.progression_rows[0].skill_points = 4U;
    MK_REQUIRE(mirakana::runtime::plan_runtime_rpg_systems(changed_progression).replay_hash != base_plan.replay_hash);

    auto changed_skill = base_request;
    changed_skill.skill_rows[0].required_level = 3U;
    MK_REQUIRE(mirakana::runtime::plan_runtime_rpg_systems(changed_skill).replay_hash != base_plan.replay_hash);

    auto changed_reward = base_request;
    changed_reward.reward_rows[0].item_id = "item.changed";
    MK_REQUIRE(mirakana::runtime::plan_runtime_rpg_systems(changed_reward).replay_hash != base_plan.replay_hash);

    auto changed_save = base_request;
    changed_save.save_validation_rows[0].observed_schema_version = 1U;
    MK_REQUIRE(mirakana::runtime::plan_runtime_rpg_systems(changed_save).replay_hash != base_plan.replay_hash);

    auto changed_side = base_request;
    changed_side.party_entity_ids.clear();
    changed_side.enemy_entity_ids = {entity("actor.0")};
    MK_REQUIRE(mirakana::runtime::plan_runtime_rpg_systems(changed_side).replay_hash != base_plan.replay_hash);
}

MK_TEST("runtime RPG systems fails closed when generated rows exceed the row budget") {
    using Code = mirakana::runtime::RuntimeRpgDiagnosticCode;
    using Status = mirakana::runtime::RuntimeRpgSystemsStatus;

    const auto plan = mirakana::runtime::plan_runtime_rpg_systems(mirakana::runtime::RuntimeRpgSystemsRequest{
        .system_id = "rpg.budget",
        .world_id = "world.budget",
        .world_tick = 5U,
        .party_entity_ids = {entity("actor.0")},
        .enemy_entity_ids = {},
        .stat_rows = {make_stat("actor.0", "initiative", 4, 4, 1U)},
        .combat_request =
            mirakana::runtime::RuntimeRpgCombatLoopRequest{
                .encounter_id = "encounter.budget",
                .initiative_stat_id = "initiative",
                .max_rounds = 2U,
            },
        .row_budget = 2U,
    });

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, Code::row_budget_exceeded) == 1U);
    MK_REQUIRE(plan.stat_rows.empty());
    MK_REQUIRE(plan.combat_loop_rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("runtime RPG systems rejects unsafe or inconsistent rows before planning") {
    using Code = mirakana::runtime::RuntimeRpgDiagnosticCode;
    using Status = mirakana::runtime::RuntimeRpgSystemsStatus;

    const auto request = mirakana::runtime::RuntimeRpgSystemsRequest{
        .system_id = "native.rpg",
        .world_id = "world.invalid",
        .world_tick = 3U,
        .party_entity_ids = {entity(""), entity("party.entity.0"), entity("party.entity.0")},
        .enemy_entity_ids = {entity("opponent.entity.0")},
        .stat_rows =
            {
                make_stat("party.entity.0", "health", 40, 30, 1U),
                make_stat("party.entity.0", "health", 20, 30, 2U),
                make_stat("missing.entity", "focus", 5, 5, 3U),
                make_stat("party.entity.0", "initiative", 5, 5, 4U),
            },
        .progression_rows =
            {
                make_progression("party.entity.0", "track.core", 0U, 0U, 0U, 1U),
                make_progression("missing.entity", "track.core", 1U, 0U, 0U, 2U),
            },
        .skill_rows =
            {
                make_skill("party.entity.0", "skill.primary", 1U, "unknown_stat", 1, 1U),
                make_skill("missing.entity", "skill.ghost", 1U, "", 0, 2U),
                make_skill("party.entity.0", "skill.primary", 1U, "", 0, 3U),
            },
        .equipment_rows =
            {
                make_equipment("party.entity.0", "slot.primary", "item.primary", 1U, "unknown_stat", 1, 1U),
                make_equipment("party.entity.0", "slot.primary", "item.backup", 1U, "", 0, 2U),
                make_equipment("missing.entity", "slot.secondary", "item.ghost", 1U, "", 0, 3U),
            },
        .combat_request =
            mirakana::runtime::RuntimeRpgCombatLoopRequest{
                .encounter_id = "encounter.invalid",
                .initiative_stat_id = "initiative",
                .max_rounds = 0U,
            },
        .reward_rows =
            {
                make_reward("reward.zero", "party.entity.0", RuntimeRpgRewardKind::experience, "", 0U, 1U),
                make_reward("reward.zero", "missing.entity", RuntimeRpgRewardKind::item, "item.resource", 1U, 2U),
            },
        .save_validation_rows =
            {
                make_save("party.entity.0", "profile", "state.party.0", 1U, 1U, 1U),
                make_save("missing.entity", "profile", "state.party.0", 1U, 1U, 2U),
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_rpg_systems(request);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.stat_rows.empty());
    MK_REQUIRE(plan.progression_rows.empty());
    MK_REQUIRE(plan.skill_rows.empty());
    MK_REQUIRE(plan.equipment_rows.empty());
    MK_REQUIRE(plan.combat_loop_rows.empty());
    MK_REQUIRE(plan.reward_rows.empty());
    MK_REQUIRE(plan.save_validation_rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
    MK_REQUIRE(diagnostic_count(plan, Code::unsupported_backend_reference) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_actor_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_actor_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_stat_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_stat_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_stat_actor) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_progression_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_progression_actor) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_skill_actor) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_skill_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_skill_stat) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_equipment_slot) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_equipment_actor) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_equipment_stat) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_combat_participant) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_combat_rounds) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_reward_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_reward_actor) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_reward_quantity) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_save_key) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_save_actor) == 1U);
}

int main() {
    return mirakana::test::run_all();
}
