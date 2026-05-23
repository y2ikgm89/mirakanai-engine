// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/sprite_collision_hitbox.hpp"

#include <limits>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeSpriteCollisionHitboxRequest sprite_hitbox_request() {
    using namespace mirakana::runtime;

    return RuntimeSpriteCollisionHitboxRequest{
        .boxes =
            std::vector<RuntimeSpriteCollisionBoxDesc>{
                RuntimeSpriteCollisionBoxDesc{
                    .id = "player.slash",
                    .frame_id = "player.attack.1",
                    .entity_id = "player",
                    .kind = RuntimeSpriteCollisionBoxKind::hitbox,
                    .center_x = 0.35F,
                    .center_y = 0.0F,
                    .half_width = 0.35F,
                    .half_height = 0.25F,
                    .layer = 0x1U,
                    .mask = 0x2U,
                    .gameplay_kind = RuntimeGameplayInteractionKind::damage,
                    .gameplay_amount = 2,
                    .gameplay_feedback_id = "feedback.enemy_hit",
                    .source_index = 0U,
                },
                RuntimeSpriteCollisionBoxDesc{
                    .id = "enemy.body",
                    .frame_id = "enemy.idle.0",
                    .entity_id = "enemy",
                    .kind = RuntimeSpriteCollisionBoxKind::hurtbox,
                    .center_x = 0.0F,
                    .center_y = 0.0F,
                    .half_width = 0.45F,
                    .half_height = 0.5F,
                    .layer = 0x2U,
                    .mask = 0x1U,
                    .gameplay_kind = RuntimeGameplayInteractionKind::damage,
                    .gameplay_amount = 0,
                    .gameplay_feedback_id = {},
                    .source_index = 1U,
                },
                RuntimeSpriteCollisionBoxDesc{
                    .id = "crate.body",
                    .frame_id = "crate.idle.0",
                    .entity_id = "crate",
                    .kind = RuntimeSpriteCollisionBoxKind::hurtbox,
                    .center_x = 0.0F,
                    .center_y = 0.0F,
                    .half_width = 0.5F,
                    .half_height = 0.5F,
                    .layer = 0x4U,
                    .mask = 0x1U,
                    .gameplay_kind = RuntimeGameplayInteractionKind::damage,
                    .gameplay_amount = 0,
                    .gameplay_feedback_id = {},
                    .source_index = 2U,
                },
            },
        .frame_poses =
            std::vector<RuntimeSpriteCollisionFramePoseDesc>{
                RuntimeSpriteCollisionFramePoseDesc{
                    .frame_id = "player.attack.1",
                    .entity_id = "player",
                    .world_x = 0.0F,
                    .world_y = 0.0F,
                    .active = true,
                    .source_index = 0U,
                },
                RuntimeSpriteCollisionFramePoseDesc{
                    .frame_id = "enemy.idle.0",
                    .entity_id = "enemy",
                    .world_x = 0.6F,
                    .world_y = 0.0F,
                    .active = true,
                    .source_index = 1U,
                },
                RuntimeSpriteCollisionFramePoseDesc{
                    .frame_id = "crate.idle.0",
                    .entity_id = "crate",
                    .world_x = 6.0F,
                    .world_y = 0.0F,
                    .active = true,
                    .source_index = 2U,
                },
            },
        .max_hit_rows = std::numeric_limits<std::size_t>::max(),
    };
}

} // namespace

MK_TEST("runtime sprite collision hitboxes emit deterministic gameplay events") {
    using Kind = mirakana::runtime::RuntimeGameplayInteractionKind;

    const auto request = sprite_hitbox_request();
    const auto first = mirakana::runtime::plan_runtime_sprite_collision_hitboxes(request);
    const auto second = mirakana::runtime::plan_runtime_sprite_collision_hitboxes(request);

    MK_REQUIRE(first.succeeded);
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.rows == second.rows);
    MK_REQUIRE(first.gameplay_events == second.gameplay_events);
    MK_REQUIRE(first.rows.size() == 1U);
    MK_REQUIRE(first.gameplay_events.size() == 1U);

    const auto& row = first.rows[0];
    MK_REQUIRE(row.hitbox_id == "player.slash");
    MK_REQUIRE(row.hurtbox_id == "enemy.body");
    MK_REQUIRE(row.hitbox_frame_id == "player.attack.1");
    MK_REQUIRE(row.hurtbox_frame_id == "enemy.idle.0");
    MK_REQUIRE(row.source_entity_id == "player");
    MK_REQUIRE(row.target_entity_id == "enemy");
    MK_REQUIRE(row.gameplay_event_id == "12:player.slash|10:enemy.body");
    MK_REQUIRE(row.hitbox_layer == 0x1U);
    MK_REQUIRE(row.hurtbox_layer == 0x2U);

    const auto& event = first.gameplay_events[0];
    MK_REQUIRE(event.id == "12:player.slash|10:enemy.body");
    MK_REQUIRE(event.kind == Kind::damage);
    MK_REQUIRE(event.source_entity_id == "player");
    MK_REQUIRE(event.target_entity_id == "enemy");
    MK_REQUIRE(event.feedback_id == "feedback.enemy_hit");
    MK_REQUIRE(event.amount == 2);
}

MK_TEST("runtime sprite collision hitboxes fail closed on invalid authoring rows") {
    using Code = mirakana::runtime::RuntimeSpriteCollisionDiagnosticCode;
    using namespace mirakana::runtime;

    auto request = sprite_hitbox_request();
    request.boxes[0].half_width = 0.0F;
    request.boxes[1].id = request.boxes[0].id;
    request.boxes[2].frame_id = "missing.frame";

    const auto plan = plan_runtime_sprite_collision_hitboxes(request);

    MK_REQUIRE(!plan.succeeded);
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(plan.gameplay_events.empty());
    MK_REQUIRE(plan.diagnostics.size() == 3U);
    MK_REQUIRE(plan.diagnostics[0].code == Code::duplicate_box_id);
    MK_REQUIRE(plan.diagnostics[0].box_id == "player.slash");
    MK_REQUIRE(plan.diagnostics[1].code == Code::invalid_box_bounds);
    MK_REQUIRE(plan.diagnostics[1].box_id == "player.slash");
    MK_REQUIRE(plan.diagnostics[2].code == Code::missing_frame_pose);
    MK_REQUIRE(plan.diagnostics[2].frame_id == "missing.frame");
}

MK_TEST("runtime sprite collision hitboxes reject reviewed hit budgets before emitting events") {
    using Code = mirakana::runtime::RuntimeSpriteCollisionDiagnosticCode;

    auto request = sprite_hitbox_request();
    request.max_hit_rows = 0U;

    const auto plan = mirakana::runtime::plan_runtime_sprite_collision_hitboxes(request);

    MK_REQUIRE(!plan.succeeded);
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(plan.gameplay_events.empty());
    MK_REQUIRE(plan.diagnostics.size() == 1U);
    MK_REQUIRE(plan.diagnostics[0].code == Code::hit_budget_exceeded);
    MK_REQUIRE(plan.diagnostics[0].box_id == "player.slash");
    MK_REQUIRE(plan.diagnostics[0].other_box_id == "enemy.body");
}

MK_TEST("runtime sprite collision hitboxes respect active poses and reciprocal layer masks") {
    auto inactive_pose_request = sprite_hitbox_request();
    inactive_pose_request.frame_poses[1].active = false;

    const auto inactive_pose_plan = mirakana::runtime::plan_runtime_sprite_collision_hitboxes(inactive_pose_request);

    MK_REQUIRE(inactive_pose_plan.succeeded);
    MK_REQUIRE(inactive_pose_plan.diagnostics.empty());
    MK_REQUIRE(inactive_pose_plan.rows.empty());
    MK_REQUIRE(inactive_pose_plan.gameplay_events.empty());

    auto filtered_request = sprite_hitbox_request();
    filtered_request.boxes[0].mask = 0x4U;

    const auto filtered_plan = mirakana::runtime::plan_runtime_sprite_collision_hitboxes(filtered_request);

    MK_REQUIRE(filtered_plan.succeeded);
    MK_REQUIRE(filtered_plan.diagnostics.empty());
    MK_REQUIRE(filtered_plan.rows.empty());
    MK_REQUIRE(filtered_plan.gameplay_events.empty());
}

MK_TEST("runtime sprite collision hitboxes keep authored tuple and event ids unambiguous") {
    using namespace mirakana::runtime;

    auto request = sprite_hitbox_request();
    request.boxes[0].id = "a";
    request.boxes[1].id = "b->c";
    request.boxes[2].id = "c";
    request.boxes[2].layer = 0x8U;
    request.boxes[2].mask = 0x4U;
    request.frame_poses[2].world_x = 0.0F;
    request.boxes.push_back(RuntimeSpriteCollisionBoxDesc{
        .id = "a->b",
        .frame_id = "player.attack.2",
        .entity_id = "player2",
        .kind = RuntimeSpriteCollisionBoxKind::hitbox,
        .center_x = 0.35F,
        .center_y = 0.0F,
        .half_width = 0.35F,
        .half_height = 0.25F,
        .layer = 0x4U,
        .mask = 0x8U,
        .gameplay_kind = RuntimeGameplayInteractionKind::damage,
        .gameplay_amount = 1,
        .gameplay_feedback_id = "feedback.second_hit",
        .source_index = 3U,
    });
    request.frame_poses.push_back(RuntimeSpriteCollisionFramePoseDesc{
        .frame_id = "player.attack.2",
        .entity_id = "player2",
        .world_x = 0.0F,
        .world_y = 0.0F,
        .active = true,
        .source_index = 3U,
    });
    request.frame_poses.push_back(RuntimeSpriteCollisionFramePoseDesc{
        .frame_id = "frame\nentity",
        .entity_id = "suffix",
        .world_x = 0.0F,
        .world_y = 0.0F,
        .active = true,
        .source_index = 4U,
    });
    request.frame_poses.push_back(RuntimeSpriteCollisionFramePoseDesc{
        .frame_id = "frame",
        .entity_id = "entity\nsuffix",
        .world_x = 0.0F,
        .world_y = 0.0F,
        .active = true,
        .source_index = 5U,
    });

    const auto plan = plan_runtime_sprite_collision_hitboxes(request);

    MK_REQUIRE(plan.succeeded);
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.rows.size() == 2U);
    MK_REQUIRE(plan.gameplay_events.size() == 2U);
    MK_REQUIRE(plan.gameplay_events[0].id != plan.gameplay_events[1].id);
}

int main() {
    return mirakana::test::run_all();
}
