// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/sprite_batch.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using DrawIntentRows = std::vector<mirakana::SpriteBatchProductionDrawIntentRow>;

[[nodiscard]] mirakana::SpriteBatchProductionDrawIntentRow
draw_intent(std::string sprite_id, std::uint64_t stable_order, std::int32_t sorting_layer, std::uint32_t material_lane,
            mirakana::AssetId atlas_page, std::uint64_t upload_bytes) {
    return mirakana::SpriteBatchProductionDrawIntentRow{
        .sprite_id = std::move(sprite_id),
        .stable_order = stable_order,
        .sorting_layer = sorting_layer,
        .material_lane = material_lane,
        .atlas_page = atlas_page,
        .upload_bytes = upload_bytes,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::SpriteBatchProductionThroughputPlan& plan,
                                           mirakana::SpriteBatchProductionThroughputDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("sprite production throughput stable sorts and groups by material atlas lanes") {
    using Kind = mirakana::SpriteBatchProductionWorkloadKind;
    using Status = mirakana::SpriteBatchProductionThroughputStatus;
    using Timing = mirakana::SpriteBatchProductionTimingStatus;

    const auto atlas_a = mirakana::AssetId{1U};
    const auto atlas_b = mirakana::AssetId{2U};
    const DrawIntentRows draw_intents{
        draw_intent("world-fg-b", 4U, 1, 1U, atlas_b, 64U),  draw_intent("world-bg-a1", 1U, 0, 0U, atlas_a, 64U),
        draw_intent("world-bg-a0", 0U, 0, 0U, atlas_a, 64U), draw_intent("world-bg-b", 2U, 0, 0U, atlas_b, 64U),
        draw_intent("world-fx-b", 3U, 0, 1U, atlas_b, 64U),
    };

    const std::array<mirakana::SpriteBatchProductionWorkloadDesc, 1> workloads{
        mirakana::SpriteBatchProductionWorkloadDesc{
            .workload_id = "dense_arena_512",
            .kind = Kind::dense_arena_512,
            .draw_intents = std::span<const mirakana::SpriteBatchProductionDrawIntentRow>{draw_intents},
            .logical_sprite_rows = 512U,
            .culled_sprite_rows = 0U,
            .budgets =
                mirakana::SpriteBatchProductionBudgetDesc{
                    .max_visible_sprites = 512U,
                    .max_draw_rows = 8U,
                    .max_instance_rows = 512U,
                    .max_upload_bytes = 4'096U,
                    .max_atlas_pages = 2U,
                    .max_material_lanes = 2U,
                    .max_cpu_frame_time_us = 16'670U,
                },
            .measurement =
                mirakana::SpriteBatchProductionMeasurementDesc{
                    .host_measurement_available = true,
                    .cpu_frame_time_us = 4'200U,
                    .retained_trace_artifact_path =
                        "out/performance/sample_2d_desktop_runtime_package/2d-sprite-throughput.trace.json",
                    .retained_profile_artifact_path =
                        "out/performance/sample_2d_desktop_runtime_package/2d-sprite-throughput.profile.json",
                    .retained_artifact_hash = 9'817U,
                },
        },
    };

    const auto plan = mirakana::plan_sprite_batch_production_throughput(
        mirakana::SpriteBatchProductionThroughputDesc{.workloads = workloads});

    MK_REQUIRE(plan.status == Status::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.rows.size() == 1U);
    MK_REQUIRE(plan.rows[0].workload_id == "dense_arena_512");
    MK_REQUIRE(plan.rows[0].kind == Kind::dense_arena_512);
    MK_REQUIRE(plan.rows[0].visible_sprite_rows == 5U);
    MK_REQUIRE(plan.rows[0].logical_sprite_rows == 512U);
    MK_REQUIRE(plan.rows[0].draw_rows == 4U);
    MK_REQUIRE(plan.rows[0].instance_rows == 5U);
    MK_REQUIRE(plan.rows[0].upload_bytes == 320U);
    MK_REQUIRE(plan.rows[0].atlas_page_count == 2U);
    MK_REQUIRE(plan.rows[0].material_lane_count == 2U);
    MK_REQUIRE(plan.rows[0].timing_status == Timing::measured);
    MK_REQUIRE(plan.rows[0].retained_trace_artifact_path.ends_with("2d-sprite-throughput.trace.json"));
    MK_REQUIRE(plan.rows[0].retained_profile_artifact_path.ends_with("2d-sprite-throughput.profile.json"));
    MK_REQUIRE(plan.rows[0].retained_artifact_hash == 9'817U);
    MK_REQUIRE(plan.rows[0].draw_groups.size() == 4U);
    MK_REQUIRE(plan.rows[0].draw_groups[0].first_stable_order == 0U);
    MK_REQUIRE(plan.rows[0].draw_groups[0].last_stable_order == 1U);
    MK_REQUIRE(plan.rows[0].draw_groups[0].instance_count == 2U);
    MK_REQUIRE(plan.rows[0].draw_groups[1].first_stable_order == 2U);
    MK_REQUIRE(plan.rows[0].draw_groups[2].material_lane == 1U);
    MK_REQUIRE(plan.rows[0].draw_groups[3].sorting_layer == 1);
    MK_REQUIRE(plan.total_draw_rows == 4U);
    MK_REQUIRE(plan.total_instance_rows == 5U);
    MK_REQUIRE(plan.total_upload_bytes == 320U);
    MK_REQUIRE(plan.measured_workload_rows == 1U);
    MK_REQUIRE(plan.host_gated_workload_rows == 0U);
}

MK_TEST("sprite production throughput reports dense and projectile rows without broad optimization claims") {
    using Kind = mirakana::SpriteBatchProductionWorkloadKind;
    using Status = mirakana::SpriteBatchProductionThroughputStatus;
    using Timing = mirakana::SpriteBatchProductionTimingStatus;

    const auto atlas_a = mirakana::AssetId::from_name("textures/dense-atlas-a");
    const auto atlas_b = mirakana::AssetId::from_name("textures/dense-atlas-b");
    const auto atlas_c = mirakana::AssetId::from_name("textures/dense-atlas-c");
    const auto atlas_d = mirakana::AssetId::from_name("textures/dense-atlas-d");
    DrawIntentRows dense_4096;
    dense_4096.reserve(4'096U);
    for (std::uint64_t index = 0U; index < 4'096U; ++index) {
        const auto atlas = std::array{atlas_a, atlas_b, atlas_c, atlas_d}[static_cast<std::size_t>(index % 4U)];
        dense_4096.push_back(draw_intent("dense-" + std::to_string(index), index, 0,
                                         static_cast<std::uint32_t>(index % 3U), atlas, 64U));
    }

    DrawIntentRows projectile_draws;
    projectile_draws.reserve(768U);
    for (std::uint64_t index = 0U; index < 768U; ++index) {
        projectile_draws.push_back(draw_intent("projectile-" + std::to_string(index), index, 0, 0U, atlas_a, 32U));
    }

    const std::array<mirakana::SpriteBatchProductionWorkloadDesc, 2> workloads{
        mirakana::SpriteBatchProductionWorkloadDesc{
            .workload_id = "dense_arena_4096",
            .kind = Kind::dense_arena_4096,
            .draw_intents = std::span<const mirakana::SpriteBatchProductionDrawIntentRow>{dense_4096},
            .logical_sprite_rows = 4'096U,
            .culled_sprite_rows = 0U,
            .budgets =
                mirakana::SpriteBatchProductionBudgetDesc{
                    .max_visible_sprites = 4'096U,
                    .max_draw_rows = 16U,
                    .max_instance_rows = 4'096U,
                    .max_upload_bytes = 262'144U,
                    .max_atlas_pages = 4U,
                    .max_material_lanes = 3U,
                    .max_cpu_frame_time_us = 16'670U,
                },
            .measurement = mirakana::SpriteBatchProductionMeasurementDesc{.host_measurement_available = false},
        },
        mirakana::SpriteBatchProductionWorkloadDesc{
            .workload_id = "projectile_storm_12000",
            .kind = Kind::projectile_storm_12000,
            .draw_intents = std::span<const mirakana::SpriteBatchProductionDrawIntentRow>{projectile_draws},
            .logical_sprite_rows = 12'000U,
            .culled_sprite_rows = 11'232U,
            .budgets =
                mirakana::SpriteBatchProductionBudgetDesc{
                    .max_visible_sprites = 768U,
                    .max_draw_rows = 1U,
                    .max_instance_rows = 768U,
                    .max_upload_bytes = 24'576U,
                    .max_atlas_pages = 1U,
                    .max_material_lanes = 1U,
                    .max_cpu_frame_time_us = 16'670U,
                },
            .measurement = mirakana::SpriteBatchProductionMeasurementDesc{.host_measurement_available = false},
        },
    };

    const auto plan = mirakana::plan_sprite_batch_production_throughput(
        mirakana::SpriteBatchProductionThroughputDesc{.workloads = workloads});

    MK_REQUIRE(plan.status == Status::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.rows.size() == 2U);
    MK_REQUIRE(plan.rows[0].workload_id == "dense_arena_4096");
    MK_REQUIRE(plan.rows[0].visible_sprite_rows == 4'096U);
    MK_REQUIRE(plan.rows[0].draw_rows == 12U);
    MK_REQUIRE(plan.rows[0].atlas_page_count == 4U);
    MK_REQUIRE(plan.rows[0].material_lane_count == 3U);
    MK_REQUIRE(plan.rows[0].timing_status == Timing::host_gated);
    MK_REQUIRE(plan.rows[1].workload_id == "projectile_storm_12000");
    MK_REQUIRE(plan.rows[1].logical_sprite_rows == 12'000U);
    MK_REQUIRE(plan.rows[1].visible_sprite_rows == 768U);
    MK_REQUIRE(plan.rows[1].culled_sprite_rows == 11'232U);
    MK_REQUIRE(plan.rows[1].draw_rows == 1U);
    MK_REQUIRE(plan.rows[1].timing_status == Timing::host_gated);
    MK_REQUIRE(plan.host_gated_workload_rows == 2U);
    MK_REQUIRE(!plan.claimed_broad_optimization);
    MK_REQUIRE(!plan.claimed_cross_backend_parity);
    MK_REQUIRE(!plan.claimed_metal_readiness);
}

MK_TEST("sprite production throughput fails closed on budget overflow lane overflow and missing artifacts") {
    using Code = mirakana::SpriteBatchProductionThroughputDiagnosticCode;
    using Kind = mirakana::SpriteBatchProductionWorkloadKind;
    using Status = mirakana::SpriteBatchProductionThroughputStatus;

    const auto atlas_a = mirakana::AssetId::from_name("textures/dense-atlas-a");
    const auto atlas_b = mirakana::AssetId::from_name("textures/dense-atlas-b");
    const DrawIntentRows draw_intents{
        draw_intent("a", 0U, 0, 0U, atlas_a, 128U),
        draw_intent("b", 1U, 0, 1U, atlas_b, 128U),
    };
    const std::array<mirakana::SpriteBatchProductionWorkloadDesc, 1> workloads{
        mirakana::SpriteBatchProductionWorkloadDesc{
            .workload_id = "overflow",
            .kind = Kind::custom,
            .draw_intents = std::span<const mirakana::SpriteBatchProductionDrawIntentRow>{draw_intents},
            .logical_sprite_rows = 2U,
            .culled_sprite_rows = 0U,
            .budgets =
                mirakana::SpriteBatchProductionBudgetDesc{
                    .max_visible_sprites = 1U,
                    .max_draw_rows = 1U,
                    .max_instance_rows = 1U,
                    .max_upload_bytes = 127U,
                    .max_atlas_pages = 1U,
                    .max_material_lanes = 1U,
                    .max_cpu_frame_time_us = 1'000U,
                },
            .measurement =
                mirakana::SpriteBatchProductionMeasurementDesc{
                    .host_measurement_available = true,
                    .cpu_frame_time_us = 2'000U,
                },
        },
    };

    const auto plan = mirakana::plan_sprite_batch_production_throughput(
        mirakana::SpriteBatchProductionThroughputDesc{.workloads = workloads});

    MK_REQUIRE(plan.status == Status::budget_exceeded);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.rows.size() == 1U);
    MK_REQUIRE(plan.rows[0].over_budget);
    MK_REQUIRE(diagnostic_count(plan, Code::visible_sprite_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::draw_row_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::instance_row_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::upload_byte_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::atlas_page_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::material_lane_overflow) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::cpu_frame_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_retained_timing_artifact) == 1U);
}

int main() {
    return mirakana::test::run_all();
}
