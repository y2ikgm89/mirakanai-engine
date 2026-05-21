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

int main() {
    return mirakana::test::run_all();
}
