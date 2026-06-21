// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/entity_scale_culling.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeEntityScaleCullingEntityDesc
make_entity(std::string entity_id, mirakana::runtime::RuntimeEntityScaleCullingBoundsKind bounds_kind,
            mirakana::runtime::RuntimeEntityScaleCullingBounds2D bounds_2d,
            mirakana::runtime::RuntimeEntityScaleCullingBounds3D bounds_3d, std::uint32_t layer_mask,
            mirakana::runtime::RuntimeEntityScaleCullingUpdateBucket update_bucket, bool enabled,
            std::uint32_t source_index) {
    return mirakana::runtime::RuntimeEntityScaleCullingEntityDesc{
        .entity_id = std::move(entity_id),
        .bounds_kind = bounds_kind,
        .bounds_2d = bounds_2d,
        .bounds_3d = bounds_3d,
        .layer_mask = layer_mask,
        .update_bucket = update_bucket,
        .enabled = enabled,
        .source_index = source_index,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeEntityScaleCullingPlan& plan,
                                           mirakana::runtime::RuntimeEntityScaleCullingDiagnosticCode code) {
    std::size_t count{0};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime entity scale culling plans deterministic 2d and 3d decisions") {
    using BoundsKind = mirakana::runtime::RuntimeEntityScaleCullingBoundsKind;
    using Decision = mirakana::runtime::RuntimeEntityScaleCullingDecision;
    using Status = mirakana::runtime::RuntimeEntityScaleCullingPlanStatus;
    using UpdateBucket = mirakana::runtime::RuntimeEntityScaleCullingUpdateBucket;

    const auto empty_2d = mirakana::runtime::RuntimeEntityScaleCullingBounds2D{};
    const auto empty_3d = mirakana::runtime::RuntimeEntityScaleCullingBounds3D{};
    const auto request = mirakana::runtime::RuntimeEntityScaleCullingRequest{
        .entities =
            {
                make_entity("npc", BoundsKind::aabb_3d, empty_2d,
                            mirakana::runtime::RuntimeEntityScaleCullingBounds3D{
                                .min_x = -1.0F,
                                .min_y = -1.0F,
                                .min_z = -1.0F,
                                .max_x = 1.0F,
                                .max_y = 1.0F,
                                .max_z = 1.0F,
                            },
                            0x2U, UpdateBucket::priority, true, 4U),
                make_entity("tree", BoundsKind::aabb_2d,
                            mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                                .min_x = 0.0F,
                                .min_y = 0.0F,
                                .max_x = 1.0F,
                                .max_y = 1.0F,
                            },
                            empty_3d, 0x1U, UpdateBucket::normal, true, 2U),
                make_entity("sleeping", BoundsKind::aabb_3d, empty_2d,
                            mirakana::runtime::RuntimeEntityScaleCullingBounds3D{
                                .min_x = 0.0F,
                                .min_y = 0.0F,
                                .min_z = 0.0F,
                                .max_x = 1.0F,
                                .max_y = 1.0F,
                                .max_z = 1.0F,
                            },
                            0x1U, UpdateBucket::background, false, 5U),
                make_entity("crate", BoundsKind::aabb_2d,
                            mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                                .min_x = 20.0F,
                                .min_y = 20.0F,
                                .max_x = 21.0F,
                                .max_y = 21.0F,
                            },
                            empty_3d, 0x1U, UpdateBucket::background, true, 3U),
                make_entity("fx", BoundsKind::aabb_2d,
                            mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                                .min_x = 0.0F,
                                .min_y = 0.0F,
                                .max_x = 1.0F,
                                .max_y = 1.0F,
                            },
                            empty_3d, 0x4U, UpdateBucket::background, true, 1U),
            },
        .view =
            mirakana::runtime::RuntimeEntityScaleCullingViewDesc{
                .bounds_kind = BoundsKind::aabb_3d,
                .bounds_2d = empty_2d,
                .bounds_3d =
                    mirakana::runtime::RuntimeEntityScaleCullingBounds3D{
                        .min_x = -5.0F,
                        .min_y = -5.0F,
                        .min_z = -2.0F,
                        .max_x = 5.0F,
                        .max_y = 5.0F,
                        .max_z = 2.0F,
                    },
                .layer_mask = 0x3U,
                .max_visible_entities = 8U,
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_entity_scale_culling(request);

    MK_REQUIRE(plan.status == Status::planned);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.rows.size() == 5U);
    MK_REQUIRE(plan.rows[0].entity_id == "crate");
    MK_REQUIRE(plan.rows[0].decision == Decision::culled_outside_view);
    MK_REQUIRE(plan.rows[1].entity_id == "fx");
    MK_REQUIRE(plan.rows[1].decision == Decision::culled_layer_mask);
    MK_REQUIRE(plan.rows[2].entity_id == "npc");
    MK_REQUIRE(plan.rows[2].decision == Decision::visible);
    MK_REQUIRE(plan.rows[2].update_bucket == UpdateBucket::priority);
    MK_REQUIRE(plan.rows[3].entity_id == "sleeping");
    MK_REQUIRE(plan.rows[3].decision == Decision::culled_disabled);
    MK_REQUIRE(plan.rows[4].entity_id == "tree");
    MK_REQUIRE(plan.rows[4].decision == Decision::visible);
    MK_REQUIRE(plan.rows[4].update_bucket == UpdateBucket::normal);
    MK_REQUIRE(plan.projected_visible_count == 2U);
    MK_REQUIRE(plan.projected_culled_count == 3U);
    MK_REQUIRE(plan.projected_disabled_count == 1U);
    MK_REQUIRE(plan.projected_layer_culled_count == 1U);
    MK_REQUIRE(plan.projected_outside_view_culled_count == 1U);
}

MK_TEST("runtime entity scale culling rejects duplicate missing invalid and zero layer rows") {
    using BoundsKind = mirakana::runtime::RuntimeEntityScaleCullingBoundsKind;
    using Code = mirakana::runtime::RuntimeEntityScaleCullingDiagnosticCode;
    using Status = mirakana::runtime::RuntimeEntityScaleCullingPlanStatus;
    using UpdateBucket = mirakana::runtime::RuntimeEntityScaleCullingUpdateBucket;

    const auto empty_3d = mirakana::runtime::RuntimeEntityScaleCullingBounds3D{};
    const auto valid_2d = mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
        .min_x = 0.0F,
        .min_y = 0.0F,
        .max_x = 1.0F,
        .max_y = 1.0F,
    };
    const auto invalid_2d = mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
        .min_x = 4.0F,
        .min_y = 0.0F,
        .max_x = 1.0F,
        .max_y = 1.0F,
    };
    const auto request = mirakana::runtime::RuntimeEntityScaleCullingRequest{
        .entities =
            {
                make_entity("hero", BoundsKind::aabb_2d, valid_2d, empty_3d, 0x1U, UpdateBucket::normal, true, 1U),
                make_entity("hero", BoundsKind::aabb_2d, valid_2d, empty_3d, 0x1U, UpdateBucket::normal, true, 2U),
                make_entity("", BoundsKind::aabb_2d, valid_2d, empty_3d, 0x1U, UpdateBucket::normal, true, 3U),
                make_entity("bad_bounds", BoundsKind::aabb_2d, invalid_2d, empty_3d, 0x1U, UpdateBucket::normal, true,
                            4U),
                make_entity("zero_layer", BoundsKind::aabb_2d, valid_2d, empty_3d, 0U, UpdateBucket::normal, true, 5U),
            },
        .view =
            mirakana::runtime::RuntimeEntityScaleCullingViewDesc{
                .bounds_kind = BoundsKind::aabb_2d,
                .bounds_2d =
                    mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                        .min_x = 8.0F,
                        .min_y = -1.0F,
                        .max_x = 2.0F,
                        .max_y = 1.0F,
                    },
                .bounds_3d = empty_3d,
                .layer_mask = 0x1U,
                .max_visible_entities = 8U,
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_entity_scale_culling(request);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(plan.diagnostics.size() == 5U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_entity_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_entity_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_entity_bounds) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::zero_layer_mask) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_view_bounds) == 1U);
}

MK_TEST("runtime entity scale culling rejects visible entity budget overflow") {
    using BoundsKind = mirakana::runtime::RuntimeEntityScaleCullingBoundsKind;
    using Code = mirakana::runtime::RuntimeEntityScaleCullingDiagnosticCode;
    using Status = mirakana::runtime::RuntimeEntityScaleCullingPlanStatus;
    using UpdateBucket = mirakana::runtime::RuntimeEntityScaleCullingUpdateBucket;

    const auto empty_3d = mirakana::runtime::RuntimeEntityScaleCullingBounds3D{};
    const auto bounds = mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
        .min_x = 0.0F,
        .min_y = 0.0F,
        .max_x = 1.0F,
        .max_y = 1.0F,
    };
    const auto request = mirakana::runtime::RuntimeEntityScaleCullingRequest{
        .entities =
            {
                make_entity("hero", BoundsKind::aabb_2d, bounds, empty_3d, 0x1U, UpdateBucket::normal, true, 1U),
                make_entity("npc", BoundsKind::aabb_2d, bounds, empty_3d, 0x1U, UpdateBucket::priority, true, 2U),
            },
        .view =
            mirakana::runtime::RuntimeEntityScaleCullingViewDesc{
                .bounds_kind = BoundsKind::aabb_2d,
                .bounds_2d =
                    mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                        .min_x = -1.0F,
                        .min_y = -1.0F,
                        .max_x = 2.0F,
                        .max_y = 2.0F,
                    },
                .bounds_3d = empty_3d,
                .layer_mask = 0x1U,
                .max_visible_entities = 1U,
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_entity_scale_culling(request);

    MK_REQUIRE(plan.status == Status::budget_exceeded);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(plan.projected_visible_count == 2U);
    MK_REQUIRE(plan.diagnostics.size() == 1U);
    MK_REQUIRE(plan.diagnostics[0].code == Code::visible_entity_budget_exceeded);
}

MK_TEST("runtime entity scale culling reports lod update and draw intent rows deterministically") {
    using BoundsKind = mirakana::runtime::RuntimeEntityScaleCullingBoundsKind;
    using DrawIntent = mirakana::runtime::RuntimeEntityScaleCullingDrawIntentKind;
    using Status = mirakana::runtime::RuntimeEntityScaleCullingPlanStatus;
    using UpdateBucket = mirakana::runtime::RuntimeEntityScaleCullingUpdateBucket;

    const auto empty_2d = mirakana::runtime::RuntimeEntityScaleCullingBounds2D{};
    const auto empty_3d = mirakana::runtime::RuntimeEntityScaleCullingBounds3D{};
    auto hero = make_entity("hero", BoundsKind::aabb_2d,
                            mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                                .min_x = -1.0F,
                                .min_y = -1.0F,
                                .max_x = 1.0F,
                                .max_y = 1.0F,
                            },
                            empty_3d, 0x1U, UpdateBucket::priority, true, 2U);
    hero.budget_protected = true;
    hero.lod_bands = {
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 3U,
            .max_view_distance = 4.0F,
            .draw_cost = 99U,
            .update_cost = 99U,
            .update_interval_frames = 9U,
            .draw_intent = DrawIntent::custom,
        },
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 2U,
            .max_view_distance = 64.0F,
            .draw_cost = 8U,
            .update_cost = 3U,
            .update_interval_frames = 6U,
            .draw_intent = DrawIntent::sprite_2d,
        },
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 0U,
            .max_view_distance = 4.0F,
            .draw_cost = 2U,
            .update_cost = 1U,
            .update_interval_frames = 1U,
            .draw_intent = DrawIntent::sprite_2d,
        },
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 1U,
            .max_view_distance = 12.0F,
            .draw_cost = 4U,
            .update_cost = 2U,
            .update_interval_frames = 3U,
            .draw_intent = DrawIntent::sprite_2d,
        },
    };

    auto mesh = make_entity("mesh", BoundsKind::aabb_3d, empty_2d,
                            mirakana::runtime::RuntimeEntityScaleCullingBounds3D{
                                .min_x = 9.0F,
                                .min_y = -1.0F,
                                .min_z = -1.0F,
                                .max_x = 11.0F,
                                .max_y = 1.0F,
                                .max_z = 1.0F,
                            },
                            0x1U, UpdateBucket::normal, true, 1U);
    mesh.lod_bands = {
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 1U,
            .max_view_distance = 20.0F,
            .draw_cost = 5U,
            .update_cost = 2U,
            .update_interval_frames = 4U,
            .draw_intent = DrawIntent::mesh_3d,
        },
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 0U,
            .max_view_distance = 5.0F,
            .draw_cost = 9U,
            .update_cost = 4U,
            .update_interval_frames = 1U,
            .draw_intent = DrawIntent::mesh_3d,
        },
    };

    auto disabled = make_entity("sleeping", BoundsKind::aabb_2d,
                                mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                                    .min_x = 0.0F,
                                    .min_y = 0.0F,
                                    .max_x = 1.0F,
                                    .max_y = 1.0F,
                                },
                                empty_3d, 0x1U, UpdateBucket::background, false, 3U);
    disabled.lod_bands = {
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 0U,
            .max_view_distance = 10.0F,
            .draw_cost = 20U,
            .update_cost = 20U,
            .update_interval_frames = 1U,
            .draw_intent = DrawIntent::sprite_2d,
        },
    };

    const auto request = mirakana::runtime::RuntimeEntityScaleCullingRequest{
        .entities = {mesh, disabled, hero},
        .view =
            mirakana::runtime::RuntimeEntityScaleCullingViewDesc{
                .bounds_kind = BoundsKind::aabb_3d,
                .bounds_2d = empty_2d,
                .bounds_3d =
                    mirakana::runtime::RuntimeEntityScaleCullingBounds3D{
                        .min_x = -16.0F,
                        .min_y = -16.0F,
                        .min_z = -8.0F,
                        .max_x = 16.0F,
                        .max_y = 16.0F,
                        .max_z = 8.0F,
                    },
                .layer_mask = 0x1U,
                .max_visible_entities = 8U,
                .max_projected_draw_cost = 32U,
                .max_projected_update_cost = 16U,
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_entity_scale_culling(request);

    MK_REQUIRE(plan.status == Status::planned);
    MK_REQUIRE(plan.rows.size() == 3U);
    MK_REQUIRE(plan.rows[0].entity_id == "hero");
    MK_REQUIRE(plan.rows[0].lod_index == 0U);
    MK_REQUIRE(plan.rows[0].draw_intent == DrawIntent::sprite_2d);
    MK_REQUIRE(plan.rows[0].projected_draw_cost == 2U);
    MK_REQUIRE(plan.rows[0].projected_update_cost == 1U);
    MK_REQUIRE(plan.rows[0].update_interval_frames == 1U);
    MK_REQUIRE(plan.rows[0].budget_protected);
    MK_REQUIRE(plan.rows[1].entity_id == "mesh");
    MK_REQUIRE(plan.rows[1].lod_index == 1U);
    MK_REQUIRE(plan.rows[1].draw_intent == DrawIntent::mesh_3d);
    MK_REQUIRE(plan.rows[1].projected_draw_cost == 5U);
    MK_REQUIRE(plan.rows[1].projected_update_cost == 2U);
    MK_REQUIRE(plan.rows[1].update_interval_frames == 4U);
    MK_REQUIRE(plan.rows[2].entity_id == "sleeping");
    MK_REQUIRE(plan.rows[2].draw_intent == DrawIntent::none);
    MK_REQUIRE(plan.rows[2].projected_draw_cost == 0U);
    MK_REQUIRE(plan.rows[2].projected_update_cost == 0U);
    MK_REQUIRE(plan.projected_visible_count == 2U);
    MK_REQUIRE(plan.projected_draw_cost == 7U);
    MK_REQUIRE(plan.projected_update_cost == 3U);
    MK_REQUIRE(plan.projected_protected_visible_count == 1U);
}

MK_TEST("runtime entity scale culling rejects invalid lod bands and draw update budget overflow") {
    using BoundsKind = mirakana::runtime::RuntimeEntityScaleCullingBoundsKind;
    using Code = mirakana::runtime::RuntimeEntityScaleCullingDiagnosticCode;
    using DrawIntent = mirakana::runtime::RuntimeEntityScaleCullingDrawIntentKind;
    using Status = mirakana::runtime::RuntimeEntityScaleCullingPlanStatus;
    using UpdateBucket = mirakana::runtime::RuntimeEntityScaleCullingUpdateBucket;

    const auto empty_3d = mirakana::runtime::RuntimeEntityScaleCullingBounds3D{};
    const auto bounds = mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
        .min_x = 0.0F,
        .min_y = 0.0F,
        .max_x = 1.0F,
        .max_y = 1.0F,
    };
    auto invalid_lod =
        make_entity("bad_lod", BoundsKind::aabb_2d, bounds, empty_3d, 0x1U, UpdateBucket::normal, true, 1U);
    invalid_lod.lod_bands = {
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 0U,
            .max_view_distance = -1.0F,
            .draw_cost = 1U,
            .update_cost = 1U,
            .update_interval_frames = 1U,
            .draw_intent = DrawIntent::sprite_2d,
        },
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 0U,
            .max_view_distance = 8.0F,
            .draw_cost = 1U,
            .update_cost = 1U,
            .update_interval_frames = 0U,
            .draw_intent = DrawIntent::sprite_2d,
        },
    };

    const auto invalid_plan =
        mirakana::runtime::plan_runtime_entity_scale_culling(mirakana::runtime::RuntimeEntityScaleCullingRequest{
            .entities = {invalid_lod},
            .view =
                mirakana::runtime::RuntimeEntityScaleCullingViewDesc{
                    .bounds_kind = BoundsKind::aabb_2d,
                    .bounds_2d =
                        mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                            .min_x = -2.0F,
                            .min_y = -2.0F,
                            .max_x = 2.0F,
                            .max_y = 2.0F,
                        },
                    .bounds_3d = empty_3d,
                    .layer_mask = 0x1U,
                    .max_visible_entities = 8U,
                },
        });

    MK_REQUIRE(invalid_plan.status == Status::invalid_request);
    MK_REQUIRE(invalid_plan.rows.empty());
    MK_REQUIRE(diagnostic_count(invalid_plan, Code::invalid_lod_band) == 2U);

    auto expensive =
        make_entity("expensive", BoundsKind::aabb_2d, bounds, empty_3d, 0x1U, UpdateBucket::priority, true, 2U);
    expensive.budget_protected = true;
    expensive.lod_bands = {
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 0U,
            .max_view_distance = 4.0F,
            .draw_cost = 9U,
            .update_cost = 5U,
            .update_interval_frames = 1U,
            .draw_intent = DrawIntent::sprite_2d,
        },
    };
    auto support = make_entity("support", BoundsKind::aabb_2d, bounds, empty_3d, 0x1U, UpdateBucket::normal, true, 3U);
    support.lod_bands = {
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 0U,
            .max_view_distance = 4.0F,
            .draw_cost = 4U,
            .update_cost = 2U,
            .update_interval_frames = 2U,
            .draw_intent = DrawIntent::sprite_2d,
        },
    };

    const auto budget_plan =
        mirakana::runtime::plan_runtime_entity_scale_culling(mirakana::runtime::RuntimeEntityScaleCullingRequest{
            .entities = {support, expensive},
            .view =
                mirakana::runtime::RuntimeEntityScaleCullingViewDesc{
                    .bounds_kind = BoundsKind::aabb_2d,
                    .bounds_2d =
                        mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                            .min_x = -2.0F,
                            .min_y = -2.0F,
                            .max_x = 2.0F,
                            .max_y = 2.0F,
                        },
                    .bounds_3d = empty_3d,
                    .layer_mask = 0x1U,
                    .max_visible_entities = 8U,
                    .max_projected_draw_cost = 10U,
                    .max_projected_update_cost = 6U,
                },
        });

    MK_REQUIRE(budget_plan.status == Status::budget_exceeded);
    MK_REQUIRE(budget_plan.rows.empty());
    MK_REQUIRE(budget_plan.projected_visible_count == 2U);
    MK_REQUIRE(budget_plan.projected_draw_cost == 13U);
    MK_REQUIRE(budget_plan.projected_update_cost == 7U);
    MK_REQUIRE(budget_plan.projected_protected_visible_count == 1U);
    MK_REQUIRE(diagnostic_count(budget_plan, Code::draw_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(budget_plan, Code::update_budget_exceeded) == 1U);
}

MK_TEST("runtime entity scale culling projects visible 2d sprite draw intent rows without renderer ownership") {
    using BoundsKind = mirakana::runtime::RuntimeEntityScaleCullingBoundsKind;
    using DrawIntent = mirakana::runtime::RuntimeEntityScaleCullingDrawIntentKind;
    using IntentStatus = mirakana::runtime::RuntimeEntityScaleCullingSpriteDrawIntentStatus;
    using Status = mirakana::runtime::RuntimeEntityScaleCullingPlanStatus;
    using UpdateBucket = mirakana::runtime::RuntimeEntityScaleCullingUpdateBucket;

    const auto empty_3d = mirakana::runtime::RuntimeEntityScaleCullingBounds3D{};
    const auto bounds = mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
        .min_x = 0.0F,
        .min_y = 0.0F,
        .max_x = 1.0F,
        .max_y = 1.0F,
    };
    auto hero = make_entity("hero", BoundsKind::aabb_2d, bounds, empty_3d, 0x1U, UpdateBucket::priority, true, 2U);
    hero.budget_protected = true;
    hero.lod_bands = {
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 0U,
            .max_view_distance = 8.0F,
            .draw_cost = 2U,
            .update_cost = 1U,
            .update_interval_frames = 1U,
            .draw_intent = DrawIntent::sprite_2d,
        },
    };
    auto mesh = make_entity("mesh", BoundsKind::aabb_3d, {},
                            mirakana::runtime::RuntimeEntityScaleCullingBounds3D{
                                .min_x = 0.0F,
                                .min_y = 0.0F,
                                .min_z = 0.0F,
                                .max_x = 1.0F,
                                .max_y = 1.0F,
                                .max_z = 1.0F,
                            },
                            0x1U, UpdateBucket::normal, true, 3U);
    mesh.lod_bands = {
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 0U,
            .max_view_distance = 8.0F,
            .draw_cost = 1U,
            .update_cost = 1U,
            .update_interval_frames = 1U,
            .draw_intent = DrawIntent::mesh_3d,
        },
    };
    const auto plan =
        mirakana::runtime::plan_runtime_entity_scale_culling(mirakana::runtime::RuntimeEntityScaleCullingRequest{
            .entities = {mesh, hero},
            .view =
                mirakana::runtime::RuntimeEntityScaleCullingViewDesc{
                    .bounds_kind = BoundsKind::aabb_2d,
                    .bounds_2d =
                        mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                            .min_x = -2.0F,
                            .min_y = -2.0F,
                            .max_x = 2.0F,
                            .max_y = 2.0F,
                        },
                    .bounds_3d = empty_3d,
                    .layer_mask = 0x1U,
                    .max_visible_entities = 8U,
                },
        });
    MK_REQUIRE(plan.status == Status::planned);

    const auto intents = mirakana::runtime::plan_runtime_entity_scale_culling_sprite_draw_intents(
        mirakana::runtime::RuntimeEntityScaleCullingSpriteDrawIntentRequest{
            .culling_plan = &plan,
            .max_draw_intent_rows = 4U,
        });

    MK_REQUIRE(intents.status == IntentStatus::ready);
    MK_REQUIRE(intents.succeeded());
    MK_REQUIRE(intents.logical_sprite_rows == 2U);
    MK_REQUIRE(intents.visible_sprite_rows == 1U);
    MK_REQUIRE(intents.culled_sprite_rows == 0U);
    MK_REQUIRE(intents.non_sprite_visible_rows == 1U);
    MK_REQUIRE(intents.rows.size() == 1U);
    MK_REQUIRE(intents.rows[0].entity_id == "hero");
    MK_REQUIRE(intents.rows[0].source_index == 2U);
    MK_REQUIRE(intents.rows[0].lod_index == 0U);
    MK_REQUIRE(intents.rows[0].projected_draw_cost == 2U);
    MK_REQUIRE(intents.rows[0].stable_order == 2U);
    MK_REQUIRE(intents.rows[0].budget_protected);
    MK_REQUIRE(!intents.invoked_scene_mutation);
    MK_REQUIRE(!intents.requested_renderer_ownership);
    MK_REQUIRE(!intents.requested_native_handle_access);
}

MK_TEST("runtime entity scale culling sprite draw intents fail closed on invalid plans and budgets") {
    using BoundsKind = mirakana::runtime::RuntimeEntityScaleCullingBoundsKind;
    using Code = mirakana::runtime::RuntimeEntityScaleCullingSpriteDrawIntentDiagnosticCode;
    using DrawIntent = mirakana::runtime::RuntimeEntityScaleCullingDrawIntentKind;
    using IntentStatus = mirakana::runtime::RuntimeEntityScaleCullingSpriteDrawIntentStatus;
    using UpdateBucket = mirakana::runtime::RuntimeEntityScaleCullingUpdateBucket;

    const auto invalid = mirakana::runtime::plan_runtime_entity_scale_culling_sprite_draw_intents(
        mirakana::runtime::RuntimeEntityScaleCullingSpriteDrawIntentRequest{});
    MK_REQUIRE(invalid.status == IntentStatus::invalid_request);
    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(invalid.diagnostics.size() == 1U);
    MK_REQUIRE(invalid.diagnostics[0].code == Code::missing_culling_plan);

    const auto empty_3d = mirakana::runtime::RuntimeEntityScaleCullingBounds3D{};
    const auto bounds = mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
        .min_x = 0.0F,
        .min_y = 0.0F,
        .max_x = 1.0F,
        .max_y = 1.0F,
    };
    auto first = make_entity("first", BoundsKind::aabb_2d, bounds, empty_3d, 0x1U, UpdateBucket::normal, true, 1U);
    first.lod_bands = {
        mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc{
            .lod_index = 0U,
            .max_view_distance = 8.0F,
            .draw_cost = 1U,
            .update_cost = 1U,
            .update_interval_frames = 1U,
            .draw_intent = DrawIntent::sprite_2d,
        },
    };
    auto second = make_entity("second", BoundsKind::aabb_2d, bounds, empty_3d, 0x1U, UpdateBucket::normal, true, 2U);
    second.lod_bands = first.lod_bands;
    const auto plan =
        mirakana::runtime::plan_runtime_entity_scale_culling(mirakana::runtime::RuntimeEntityScaleCullingRequest{
            .entities = {first, second},
            .view =
                mirakana::runtime::RuntimeEntityScaleCullingViewDesc{
                    .bounds_kind = BoundsKind::aabb_2d,
                    .bounds_2d =
                        mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                            .min_x = -2.0F,
                            .min_y = -2.0F,
                            .max_x = 2.0F,
                            .max_y = 2.0F,
                        },
                    .bounds_3d = empty_3d,
                    .layer_mask = 0x1U,
                    .max_visible_entities = 8U,
                },
        });

    const auto budget = mirakana::runtime::plan_runtime_entity_scale_culling_sprite_draw_intents(
        mirakana::runtime::RuntimeEntityScaleCullingSpriteDrawIntentRequest{
            .culling_plan = &plan,
            .max_draw_intent_rows = 1U,
        });

    MK_REQUIRE(budget.status == IntentStatus::budget_exceeded);
    MK_REQUIRE(!budget.succeeded());
    MK_REQUIRE(budget.rows.empty());
    MK_REQUIRE(budget.visible_sprite_rows == 2U);
    MK_REQUIRE(budget.diagnostics.size() == 1U);
    MK_REQUIRE(budget.diagnostics[0].code == Code::draw_intent_budget_exceeded);
}

int main() {
    return mirakana::test::run_all();
}
