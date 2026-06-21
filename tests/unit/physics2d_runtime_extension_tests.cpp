// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include <mirakana/physics/physics2d.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

namespace {

[[nodiscard]] bool nearly_equal(float lhs, float rhs, float epsilon = 0.0001F) noexcept {
    return std::abs(lhs - rhs) <= epsilon;
}

[[nodiscard]] mirakana::PhysicsBody2DDesc static_box(mirakana::Vec2 position, mirakana::Vec2 half_extents) {
    return mirakana::PhysicsBody2DDesc{
        .position = position,
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = half_extents,
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .trigger = false,
    };
}

[[nodiscard]] mirakana::PhysicsBody2DDesc trigger_box(mirakana::Vec2 position, mirakana::Vec2 half_extents) {
    auto desc = static_box(position, half_extents);
    desc.trigger = true;
    return desc;
}

[[nodiscard]] mirakana::PhysicsBody2DDesc dynamic_box(mirakana::Vec2 position, mirakana::Vec2 velocity,
                                                      mirakana::Vec2 half_extents) {
    auto desc = static_box(position, half_extents);
    desc.velocity = velocity;
    desc.mass = 1.0F;
    desc.dynamic = true;
    return desc;
}

[[nodiscard]] mirakana::PhysicsBody2DDesc static_circle(mirakana::Vec2 position, float radius) {
    return mirakana::PhysicsBody2DDesc{
        .position = position,
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = radius, .y = radius},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::circle,
        .radius = radius,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .trigger = false,
    };
}

[[nodiscard]] mirakana::PhysicsBody2DDesc dynamic_circle(mirakana::Vec2 position, mirakana::Vec2 velocity,
                                                         float radius) {
    auto desc = static_circle(position, radius);
    desc.velocity = velocity;
    desc.mass = 1.0F;
    desc.dynamic = true;
    return desc;
}

} // namespace

MK_TEST("2d physics runtime extension reports exact time of impact rows for supported shape pairs") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    const auto circle_target = world.create_body(static_circle(mirakana::Vec2{.x = 5.0F, .y = 0.0F}, 0.5F));
    const auto aabb_target =
        world.create_body(static_box(mirakana::Vec2{.x = 5.0F, .y = 3.0F}, mirakana::Vec2{.x = 0.5F, .y = 0.5F}));
    const auto second_aabb_target =
        world.create_body(static_box(mirakana::Vec2{.x = 5.0F, .y = 6.0F}, mirakana::Vec2{.x = 0.5F, .y = 0.5F}));
    const auto circle_vs_circle = world.create_body(
        dynamic_circle(mirakana::Vec2{.x = 0.0F, .y = 0.0F}, mirakana::Vec2{.x = 10.0F, .y = 0.0F}, 0.5F));
    const auto circle_vs_aabb = world.create_body(
        dynamic_circle(mirakana::Vec2{.x = 0.0F, .y = 3.0F}, mirakana::Vec2{.x = 10.0F, .y = 0.0F}, 0.25F));
    const auto aabb_vs_aabb =
        world.create_body(dynamic_box(mirakana::Vec2{.x = 0.0F, .y = 6.0F}, mirakana::Vec2{.x = 10.0F, .y = 0.0F},
                                      mirakana::Vec2{.x = 0.25F, .y = 0.25F}));

    const auto result = mirakana::simulate_physics2d_step(world, mirakana::Physics2DContinuousCollisionRequest{
                                                                     .delta_seconds = 1.0F,
                                                                     .skin_width = 0.0F,
                                                                     .include_triggers = false,
                                                                     .max_time_of_impact_rows = 8U,
                                                                     .max_kinematic_contact_rows = 8U,
                                                                     .max_joint_rows = 8U,
                                                                     .max_trigger_event_rows = 8U,
                                                                 });

    MK_REQUIRE(circle_target != mirakana::null_physics_body_2d);
    MK_REQUIRE(aabb_target != mirakana::null_physics_body_2d);
    MK_REQUIRE(second_aabb_target != mirakana::null_physics_body_2d);
    MK_REQUIRE(result.status == mirakana::Physics2DSimulateStepStatus::simulated);
    MK_REQUIRE(result.diagnostic == mirakana::Physics2DRuntimeDiagnostic::none);
    MK_REQUIRE(result.time_of_impact_rows.size() == 3U);
    MK_REQUIRE(result.time_of_impact_rows[0].body == circle_vs_circle);
    MK_REQUIRE(result.time_of_impact_rows[0].hit_body == circle_target);
    MK_REQUIRE(result.time_of_impact_rows[0].hit);
    MK_REQUIRE(nearly_equal(result.time_of_impact_rows[0].distance, 4.0F));
    MK_REQUIRE(nearly_equal(result.time_of_impact_rows[0].time_of_impact, 0.4F));
    MK_REQUIRE(result.time_of_impact_rows[1].body == circle_vs_aabb);
    MK_REQUIRE(result.time_of_impact_rows[1].hit_body == aabb_target);
    MK_REQUIRE(nearly_equal(result.time_of_impact_rows[1].distance, 4.25F));
    MK_REQUIRE(result.time_of_impact_rows[2].body == aabb_vs_aabb);
    MK_REQUIRE(result.time_of_impact_rows[2].hit_body == second_aabb_target);
    MK_REQUIRE(nearly_equal(result.time_of_impact_rows[2].distance, 4.25F));
    MK_REQUIRE(result.native_handle_exposure_count == 0U);
    MK_REQUIRE(result.middleware_dispatch_count == 0U);
}

MK_TEST("2d physics runtime extension reports no-hit initial-overlap and row budget diagnostics") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    const auto overlap_target =
        world.create_body(static_box(mirakana::Vec2{.x = 0.0F, .y = 0.0F}, mirakana::Vec2{.x = 0.5F, .y = 0.5F}));
    const auto overlapping = world.create_body(
        dynamic_circle(mirakana::Vec2{.x = 0.0F, .y = 0.0F}, mirakana::Vec2{.x = 1.0F, .y = 0.0F}, 0.25F));
    const auto no_hit = world.create_body(
        dynamic_circle(mirakana::Vec2{.x = 0.0F, .y = 4.0F}, mirakana::Vec2{.x = 1.0F, .y = 0.0F}, 0.25F));
    const auto before_overlap = *world.find_body(overlapping);
    const auto rejected = mirakana::simulate_physics2d_step(world, mirakana::Physics2DContinuousCollisionRequest{
                                                                       .delta_seconds = 1.0F,
                                                                       .skin_width = 0.0F,
                                                                       .include_triggers = false,
                                                                       .max_time_of_impact_rows = 1U,
                                                                       .max_kinematic_contact_rows = 8U,
                                                                       .max_joint_rows = 8U,
                                                                       .max_trigger_event_rows = 8U,
                                                                   });

    MK_REQUIRE(overlap_target != mirakana::null_physics_body_2d);
    MK_REQUIRE(rejected.status == mirakana::Physics2DSimulateStepStatus::invalid_request);
    MK_REQUIRE(rejected.diagnostic == mirakana::Physics2DRuntimeDiagnostic::row_budget_exceeded);
    MK_REQUIRE(world.find_body(overlapping)->position == before_overlap.position);
    MK_REQUIRE(world.find_body(overlapping)->velocity == before_overlap.velocity);

    const auto accepted = mirakana::simulate_physics2d_step(world, mirakana::Physics2DContinuousCollisionRequest{
                                                                       .delta_seconds = 1.0F,
                                                                       .skin_width = 0.0F,
                                                                       .include_triggers = false,
                                                                       .max_time_of_impact_rows = 4U,
                                                                       .max_kinematic_contact_rows = 8U,
                                                                       .max_joint_rows = 8U,
                                                                       .max_trigger_event_rows = 8U,
                                                                   });

    MK_REQUIRE(accepted.status == mirakana::Physics2DSimulateStepStatus::simulated);
    MK_REQUIRE(accepted.time_of_impact_rows.size() == 2U);
    MK_REQUIRE(accepted.time_of_impact_rows[0].body == overlapping);
    MK_REQUIRE(accepted.time_of_impact_rows[0].hit_body == overlap_target);
    MK_REQUIRE(accepted.time_of_impact_rows[0].initial_overlap);
    MK_REQUIRE(accepted.time_of_impact_rows[0].time_of_impact == 0.0F);
    MK_REQUIRE(accepted.time_of_impact_rows[1].body == no_hit);
    MK_REQUIRE(!accepted.time_of_impact_rows[1].hit);
    MK_REQUIRE(accepted.time_of_impact_rows[1].hit_body == mirakana::null_physics_body_2d);
}

MK_TEST("2d physics runtime extension reports kinematic joint and trigger rows as deterministic data") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    const auto wall =
        world.create_body(static_box(mirakana::Vec2{.x = 3.0F, .y = 0.0F}, mirakana::Vec2{.x = 0.5F, .y = 1.0F}));
    const auto kinematic =
        world.create_body(dynamic_box(mirakana::Vec2{.x = 0.0F, .y = 0.0F}, mirakana::Vec2{.x = 0.0F, .y = 0.0F},
                                      mirakana::Vec2{.x = 0.5F, .y = 0.5F}));
    const auto first = world.create_body(
        dynamic_circle(mirakana::Vec2{.x = 0.0F, .y = 4.0F}, mirakana::Vec2{.x = 0.0F, .y = 0.0F}, 0.25F));
    const auto second = world.create_body(
        dynamic_circle(mirakana::Vec2{.x = 4.0F, .y = 4.0F}, mirakana::Vec2{.x = 0.0F, .y = 0.0F}, 0.25F));
    const auto trigger = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 7.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .trigger = true,
    });
    const auto trigger_guest = world.create_body(
        dynamic_circle(mirakana::Vec2{.x = 7.0F, .y = 0.0F}, mirakana::Vec2{.x = 0.0F, .y = 0.0F}, 0.25F));

    mirakana::Physics2DContinuousCollisionRequest request;
    request.delta_seconds = 1.0F;
    request.skin_width = 0.0F;
    request.include_triggers = true;
    request.max_time_of_impact_rows = 8U;
    request.max_kinematic_contact_rows = 4U;
    request.max_joint_rows = 8U;
    request.max_trigger_event_rows = 4U;
    request.kinematic_requests.push_back(mirakana::Physics2DKinematicContactResolutionRequest{
        .body = kinematic,
        .attempted_displacement = mirakana::Vec2{.x = 5.0F, .y = 1.0F},
        .skin_width = 0.0F,
        .max_iterations = 2U,
    });
    request.joints = std::vector<mirakana::Physics2DJointDesc>{
        mirakana::Physics2DJointDesc{
            .kind = mirakana::Physics2DJointKind::distance,
            .first = first,
            .second = second,
            .target_distance = 2.0F,
        },
        mirakana::Physics2DJointDesc{
            .kind = mirakana::Physics2DJointKind::hinge,
            .first = first,
            .second = second,
            .target_distance = 0.0F,
        },
        mirakana::Physics2DJointDesc{
            .kind = mirakana::Physics2DJointKind::prismatic,
            .first = first,
            .second = second,
            .axis = mirakana::Vec2{.x = 1.0F, .y = 0.0F},
            .minimum_translation = 1.0F,
            .maximum_translation = 3.0F,
        },
        mirakana::Physics2DJointDesc{
            .kind = mirakana::Physics2DJointKind::spring,
            .first = first,
            .second = second,
            .target_distance = 2.0F,
            .stiffness = 0.5F,
        },
    };
    request.previous_trigger_overlaps.push_back(mirakana::PhysicsTriggerOverlap2D{.first = trigger, .second = wall});

    const auto result = mirakana::simulate_physics2d_step(world, request);

    MK_REQUIRE(result.status == mirakana::Physics2DSimulateStepStatus::simulated);
    MK_REQUIRE(result.kinematic_contact_rows.size() == 1U);
    MK_REQUIRE(result.kinematic_contact_rows[0].body == kinematic);
    MK_REQUIRE(result.kinematic_contact_rows[0].hit_body == wall);
    MK_REQUIRE(result.kinematic_contact_rows[0].applied_displacement.x < 5.0F);
    MK_REQUIRE(result.kinematic_contact_rows[0].remaining_displacement.y > 0.0F);
    MK_REQUIRE(result.joint_rows.size() == 4U);
    MK_REQUIRE(result.joint_rows[0].kind == mirakana::Physics2DJointKind::distance);
    MK_REQUIRE(result.joint_rows[1].kind == mirakana::Physics2DJointKind::hinge);
    MK_REQUIRE(result.joint_rows[2].kind == mirakana::Physics2DJointKind::prismatic);
    MK_REQUIRE(result.joint_rows[3].kind == mirakana::Physics2DJointKind::spring);
    MK_REQUIRE(result.joint_rows[0].diagnostic == mirakana::Physics2DRuntimeDiagnostic::none);
    MK_REQUIRE(result.trigger_event_rows.size() == 2U);
    MK_REQUIRE(result.trigger_event_rows[0].kind == mirakana::Physics2DAreaTriggerEventKind::enter);
    MK_REQUIRE(result.trigger_event_rows[0].trigger_body == trigger);
    MK_REQUIRE(result.trigger_event_rows[0].other_body == trigger_guest);
    MK_REQUIRE(result.trigger_event_rows[1].kind == mirakana::Physics2DAreaTriggerEventKind::exit);
    MK_REQUIRE(result.trigger_event_rows[1].trigger_body == trigger);
    MK_REQUIRE(result.trigger_event_rows[1].other_body == wall);
}

MK_TEST("2d physics runtime extension reports trigger events after simulated body motion") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    const auto trigger =
        world.create_body(trigger_box(mirakana::Vec2{.x = 3.0F, .y = 0.0F}, mirakana::Vec2{.x = 1.0F, .y = 1.0F}));
    const auto guest = world.create_body(
        dynamic_circle(mirakana::Vec2{.x = 0.0F, .y = 0.0F}, mirakana::Vec2{.x = 3.0F, .y = 0.0F}, 0.25F));
    const auto before_guest = *world.find_body(guest);

    const auto rejected = mirakana::simulate_physics2d_step(world, mirakana::Physics2DContinuousCollisionRequest{
                                                                       .delta_seconds = 1.0F,
                                                                       .skin_width = 0.0F,
                                                                       .include_triggers = false,
                                                                       .max_time_of_impact_rows = 4U,
                                                                       .max_kinematic_contact_rows = 4U,
                                                                       .max_joint_rows = 4U,
                                                                       .max_trigger_event_rows = 0U,
                                                                   });

    MK_REQUIRE(rejected.status == mirakana::Physics2DSimulateStepStatus::invalid_request);
    MK_REQUIRE(rejected.diagnostic == mirakana::Physics2DRuntimeDiagnostic::row_budget_exceeded);
    MK_REQUIRE(world.find_body(guest)->position == before_guest.position);

    const auto accepted = mirakana::simulate_physics2d_step(world, mirakana::Physics2DContinuousCollisionRequest{
                                                                       .delta_seconds = 1.0F,
                                                                       .skin_width = 0.0F,
                                                                       .include_triggers = false,
                                                                       .max_time_of_impact_rows = 4U,
                                                                       .max_kinematic_contact_rows = 4U,
                                                                       .max_joint_rows = 4U,
                                                                       .max_trigger_event_rows = 2U,
                                                                   });

    MK_REQUIRE(accepted.status == mirakana::Physics2DSimulateStepStatus::simulated);
    MK_REQUIRE(accepted.trigger_event_rows.size() == 1U);
    MK_REQUIRE(accepted.trigger_event_rows[0].kind == mirakana::Physics2DAreaTriggerEventKind::enter);
    MK_REQUIRE(accepted.trigger_event_rows[0].trigger_body == trigger);
    MK_REQUIRE(accepted.trigger_event_rows[0].other_body == guest);
    MK_REQUIRE(nearly_equal(world.find_body(guest)->position.x, 3.0F));

    mirakana::Physics2DContinuousCollisionRequest exit_request;
    exit_request.delta_seconds = 1.0F;
    exit_request.max_time_of_impact_rows = 4U;
    exit_request.max_kinematic_contact_rows = 4U;
    exit_request.max_joint_rows = 4U;
    exit_request.max_trigger_event_rows = 2U;
    exit_request.previous_trigger_overlaps.push_back(
        mirakana::PhysicsTriggerOverlap2D{.first = trigger, .second = guest});

    world.find_body(guest)->velocity = mirakana::Vec2{.x = 3.0F, .y = 0.0F};
    const auto exited = mirakana::simulate_physics2d_step(world, exit_request);

    MK_REQUIRE(exited.status == mirakana::Physics2DSimulateStepStatus::simulated);
    MK_REQUIRE(exited.trigger_event_rows.size() == 1U);
    MK_REQUIRE(exited.trigger_event_rows[0].kind == mirakana::Physics2DAreaTriggerEventKind::exit);
    MK_REQUIRE(exited.trigger_event_rows[0].trigger_body == trigger);
    MK_REQUIRE(exited.trigger_event_rows[0].other_body == guest);
}

MK_TEST("2d physics runtime extension uses kinematic iterations for secondary slide blockers") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    const auto wall =
        world.create_body(static_box(mirakana::Vec2{.x = 3.0F, .y = 0.0F}, mirakana::Vec2{.x = 0.5F, .y = 4.0F}));
    const auto ceiling =
        world.create_body(static_box(mirakana::Vec2{.x = 2.0F, .y = 4.0F}, mirakana::Vec2{.x = 1.0F, .y = 0.5F}));
    const auto kinematic =
        world.create_body(dynamic_box(mirakana::Vec2{.x = 0.0F, .y = 0.0F}, mirakana::Vec2{.x = 0.0F, .y = 0.0F},
                                      mirakana::Vec2{.x = 0.5F, .y = 0.5F}));

    mirakana::Physics2DContinuousCollisionRequest request;
    request.delta_seconds = 0.0F;
    request.max_time_of_impact_rows = 4U;
    request.max_kinematic_contact_rows = 4U;
    request.max_joint_rows = 4U;
    request.max_trigger_event_rows = 4U;
    request.kinematic_requests.push_back(mirakana::Physics2DKinematicContactResolutionRequest{
        .body = kinematic,
        .attempted_displacement = mirakana::Vec2{.x = 5.0F, .y = 5.0F},
        .skin_width = 0.0F,
        .max_iterations = 2U,
    });

    const auto result = mirakana::simulate_physics2d_step(world, request);

    MK_REQUIRE(ceiling != mirakana::null_physics_body_2d);
    MK_REQUIRE(result.status == mirakana::Physics2DSimulateStepStatus::simulated);
    MK_REQUIRE(result.kinematic_contact_rows.size() == 1U);
    MK_REQUIRE(result.kinematic_contact_rows[0].hit_body == wall);
    MK_REQUIRE(result.kinematic_contact_rows[0].diagnostic == mirakana::Physics2DRuntimeDiagnostic::none);
    MK_REQUIRE(result.kinematic_contact_rows[0].remaining_displacement.y < 2.0F);
    MK_REQUIRE(nearly_equal(world.find_body(kinematic)->position.x, 2.0F));
    MK_REQUIRE(nearly_equal(world.find_body(kinematic)->position.y, 3.0F));

    mirakana::PhysicsWorld2D limited_world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    const auto limited_wall = limited_world.create_body(
        static_box(mirakana::Vec2{.x = 3.0F, .y = 0.0F}, mirakana::Vec2{.x = 0.5F, .y = 4.0F}));
    const auto limited_kinematic = limited_world.create_body(dynamic_box(mirakana::Vec2{.x = 0.0F, .y = 0.0F},
                                                                         mirakana::Vec2{.x = 0.0F, .y = 0.0F},
                                                                         mirakana::Vec2{.x = 0.5F, .y = 0.5F}));
    request.kinematic_requests[0] = mirakana::Physics2DKinematicContactResolutionRequest{
        .body = limited_kinematic,
        .attempted_displacement = mirakana::Vec2{.x = 5.0F, .y = 1.0F},
        .skin_width = 0.0F,
        .max_iterations = 1U,
    };

    const auto limited = mirakana::simulate_physics2d_step(limited_world, request);

    MK_REQUIRE(limited.status == mirakana::Physics2DSimulateStepStatus::simulated);
    MK_REQUIRE(limited.kinematic_contact_rows.size() == 1U);
    MK_REQUIRE(limited.kinematic_contact_rows[0].hit_body == limited_wall);
    MK_REQUIRE(limited.kinematic_contact_rows[0].diagnostic == mirakana::Physics2DRuntimeDiagnostic::iteration_limit);
    MK_REQUIRE(limited.kinematic_contact_rows[0].remaining_displacement.y == 0.0F);
}

int main() {
    return mirakana::test::run_all();
}
