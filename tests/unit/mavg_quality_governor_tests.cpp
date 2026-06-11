// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/mavg_quality_governor.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgQualityGovernorLimits make_limits() {
    return mirakana::MavgQualityGovernorLimits{
        .max_cpu_frame_time_p95_us = 16'600U,
        .max_gpu_frame_time_p95_us = 16'600U,
        .max_mavg_cpu_selection_time_p95_us = 1'000U,
        .max_mavg_gpu_culling_time_p95_us = 1'000U,
        .max_screen_error_p99_micropixels = 500U,
        .max_fallback_rate_per_million = 100'000U,
        .max_page_miss_rate_per_million = 10'000U,
        .max_temporal_churn_per_million = 50'000U,
        .max_missing_required_geometry_count = 0U,
        .max_rt_consistency_diagnostic_count = 0U,
        .max_resident_gpu_bytes = 64ULL * 1024ULL * 1024ULL,
        .max_upload_bytes = 8ULL * 1024ULL * 1024ULL,
        .max_draw_count = 1'000U,
        .max_dispatch_count = 128U,
    };
}

[[nodiscard]] mirakana::MavgQualityGovernorCounterRow make_ready_row(std::uint32_t source_index = 1U) {
    return mirakana::MavgQualityGovernorCounterRow{
        .scene_id = "scene-a-static-dense",
        .package_target_id = "sample_desktop_runtime_game",
        .validation_recipe_id = "default",
        .benchmark_command_id = "dry-run",
        .backend = mirakana::rhi::BackendKind::d3d12,
        .feature = mirakana::MavgQualityGovernorFeatureKind::static_lod,
        .host_evidence = true,
        .backend_local_evidence = true,
        .no_hole_evidence = true,
        .fallback_evidence = true,
        .rt_consistency_evidence = false,
        .cpu_frame_time_p95_us = 12'000U,
        .gpu_frame_time_p95_us = 11'000U,
        .mavg_cpu_selection_time_p95_us = 400U,
        .mavg_gpu_culling_time_p95_us = 300U,
        .screen_error_p99_micropixels = 250U,
        .fallback_rate_per_million = 25'000U,
        .page_miss_rate_per_million = 1'000U,
        .temporal_churn_per_million = 4'000U,
        .missing_required_geometry_count = 0U,
        .rt_consistency_diagnostic_count = 0U,
        .resident_gpu_bytes = 16ULL * 1024ULL * 1024ULL,
        .upload_bytes = 2ULL * 1024ULL * 1024ULL,
        .draw_count = 120U,
        .dispatch_count = 8U,
        .source_index = source_index,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::MavgQualityGovernorResult& result,
                                           mirakana::MavgQualityGovernorDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("mavg quality governor accepts reviewed static lod benchmark counters") {
    const auto result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = make_limits(),
        .rows = {make_ready_row()},
        .row_budget = 8U,
        .seed = 42U,
    });

    MK_REQUIRE(result.status == mirakana::MavgQualityGovernorStatus::ready);
    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.row_count == 1U);
    MK_REQUIRE(result.ready_row_count == 1U);
    MK_REQUIRE(result.host_gated_row_count == 0U);
    MK_REQUIRE(result.unsupported_claim_row_count == 0U);
    MK_REQUIRE(result.replay_hash != 0U);
    MK_REQUIRE(!result.invoked_gpu_commands);
    MK_REQUIRE(!result.invoked_profiler_capture);
    MK_REQUIRE(!result.mutated_packages);
    MK_REQUIRE(!result.accessed_native_handles);
}

MK_TEST("mavg quality governor sorts rows and hashes deterministic replay inputs") {
    auto first = make_ready_row(2U);
    first.scene_id = "scene-c-streaming-pressure";
    first.feature = mirakana::MavgQualityGovernorFeatureKind::streaming_residency;
    auto second = make_ready_row(1U);
    second.scene_id = "scene-a-static-dense";
    second.feature = mirakana::MavgQualityGovernorFeatureKind::static_lod;

    const auto result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = make_limits(),
        .rows = {first, second},
        .row_budget = 8U,
        .seed = 99U,
    });
    const auto reordered = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = make_limits(),
        .rows = {second, first},
        .row_budget = 8U,
        .seed = 99U,
    });

    MK_REQUIRE(result.status == mirakana::MavgQualityGovernorStatus::ready);
    MK_REQUIRE(result.rows.size() == 2U);
    MK_REQUIRE(result.rows[0].scene_id == "scene-a-static-dense");
    MK_REQUIRE(result.rows[1].scene_id == "scene-c-streaming-pressure");
    MK_REQUIRE(result.replay_hash != 0U);
    MK_REQUIRE(result.replay_hash == reordered.replay_hash);
}

MK_TEST("mavg quality governor reports host gated rows without broad readiness") {
    auto row = make_ready_row();
    row.host_evidence = false;
    row.host_gate_required = true;
    row.backend = mirakana::rhi::BackendKind::metal;
    row.feature = mirakana::MavgQualityGovernorFeatureKind::benchmark_summary;

    const auto result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = make_limits(),
        .rows = {row},
        .row_budget = 8U,
        .seed = 7U,
    });

    MK_REQUIRE(result.status == mirakana::MavgQualityGovernorStatus::host_evidence_required);
    MK_REQUIRE(!result.ready);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.ready_row_count == 0U);
    MK_REQUIRE(result.host_gated_row_count == 1U);
    MK_REQUIRE(!result.selected_benchmark_harness_ready);
    MK_REQUIRE(!result.broad_mavg_benchmark_ready);
    MK_REQUIRE(result.replay_hash != 0U);
}

MK_TEST("mavg quality governor keeps metal and explicit host gates out of ready results") {
    auto metal_row = make_ready_row();
    metal_row.backend = mirakana::rhi::BackendKind::metal;
    metal_row.feature = mirakana::MavgQualityGovernorFeatureKind::benchmark_summary;

    const auto metal_result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = make_limits(),
        .rows = {metal_row},
        .row_budget = 8U,
        .seed = 8U,
    });

    MK_REQUIRE(metal_result.status == mirakana::MavgQualityGovernorStatus::host_evidence_required);
    MK_REQUIRE(!metal_result.ready);
    MK_REQUIRE(metal_result.ready_row_count == 0U);
    MK_REQUIRE(metal_result.host_gated_row_count == 1U);
    MK_REQUIRE(!metal_result.selected_benchmark_harness_ready);

    auto explicit_gate_row = make_ready_row();
    explicit_gate_row.host_gate_required = true;

    const auto explicit_gate_result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = make_limits(),
        .rows = {explicit_gate_row},
        .row_budget = 8U,
        .seed = 9U,
    });

    MK_REQUIRE(explicit_gate_result.status == mirakana::MavgQualityGovernorStatus::host_evidence_required);
    MK_REQUIRE(!explicit_gate_result.ready);
    MK_REQUIRE(explicit_gate_result.ready_row_count == 0U);
    MK_REQUIRE(explicit_gate_result.host_gated_row_count == 1U);
    MK_REQUIRE(!explicit_gate_result.selected_benchmark_harness_ready);
}

MK_TEST("mavg quality governor rejects missing evidence duplicate rows and unsafe claims") {
    auto row = make_ready_row();
    row.scene_id = "ID3D12Resource";
    row.host_evidence = false;
    row.backend_local_evidence = false;
    row.no_hole_evidence = false;
    row.fallback_evidence = false;
    row.request_native_handle_access = true;
    row.request_nanite_claim = true;
    row.request_benchmark_superiority_claim = true;
    row.request_broad_optimization_claim = true;
    row.request_backend_parity_claim = true;
    auto duplicate = row;
    duplicate.source_index = 2U;

    const auto result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = make_limits(),
        .rows = {row, duplicate},
        .row_budget = 8U,
        .seed = 13U,
    });

    MK_REQUIRE(result.status == mirakana::MavgQualityGovernorStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::invalid_row_id) == 2U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::duplicate_counter_row) == 1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::missing_host_evidence) == 2U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::missing_backend_local_evidence) ==
               2U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::missing_no_hole_evidence) == 2U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::missing_fallback_evidence) == 2U);
    MK_REQUIRE(
        diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::unsupported_native_handle_access) == 2U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::unsupported_nanite_claim) == 2U);
    MK_REQUIRE(diagnostic_count(
                   result, mirakana::MavgQualityGovernorDiagnosticCode::unsupported_benchmark_superiority_claim) == 2U);
    MK_REQUIRE(diagnostic_count(
                   result, mirakana::MavgQualityGovernorDiagnosticCode::unsupported_broad_optimization_claim) == 2U);
    MK_REQUIRE(
        diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::unsupported_backend_parity_claim) == 2U);
    MK_REQUIRE(result.rows.empty());
    MK_REQUIRE(result.row_count == 2U);
    MK_REQUIRE(result.unsupported_claim_row_count == 2U);
    MK_REQUIRE(result.replay_hash == 0U);
    MK_REQUIRE(!result.invoked_gpu_commands);
    MK_REQUIRE(!result.invoked_profiler_capture);
    MK_REQUIRE(!result.mutated_packages);
    MK_REQUIRE(!result.accessed_native_handles);
}

MK_TEST("mavg quality governor rejects native tokens across id separators") {
    for (const auto* scene_id : {"scene-native-handle", "vk-culling", "id3d12-resource"}) {
        auto row = make_ready_row();
        row.scene_id = scene_id;

        const auto result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
            .limits = make_limits(),
            .rows = {row},
            .row_budget = 8U,
            .seed = 14U,
        });

        MK_REQUIRE(result.status == mirakana::MavgQualityGovernorStatus::invalid_request);
        MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::invalid_row_id) == 1U);
        MK_REQUIRE(result.row_count == 1U);
        MK_REQUIRE(result.replay_hash == 0U);
    }
}

MK_TEST("mavg quality governor rejects budget violations without accepting benchmark readiness") {
    auto row = make_ready_row();
    row.cpu_frame_time_p95_us = 17'000U;
    row.gpu_frame_time_p95_us = 18'000U;
    row.mavg_cpu_selection_time_p95_us = 2'000U;
    row.mavg_gpu_culling_time_p95_us = 2'000U;
    row.screen_error_p99_micropixels = 800U;
    row.fallback_rate_per_million = 200'000U;
    row.page_miss_rate_per_million = 20'000U;
    row.temporal_churn_per_million = 60'000U;
    row.missing_required_geometry_count = 1U;
    row.rt_consistency_diagnostic_count = 1U;
    row.rt_consistency_evidence = true;
    row.resident_gpu_bytes = 128ULL * 1024ULL * 1024ULL;
    row.upload_bytes = 16ULL * 1024ULL * 1024ULL;
    row.draw_count = 2'000U;
    row.dispatch_count = 256U;

    const auto result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = make_limits(),
        .rows = {row},
        .row_budget = 8U,
        .require_rt_consistency_evidence = true,
        .seed = 21U,
    });

    MK_REQUIRE(result.status == mirakana::MavgQualityGovernorStatus::budget_exceeded);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::cpu_frame_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::gpu_frame_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(result,
                                mirakana::MavgQualityGovernorDiagnosticCode::mavg_cpu_selection_budget_exceeded) == 1U);
    MK_REQUIRE(
        diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::mavg_gpu_culling_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::screen_error_budget_exceeded) ==
               1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::fallback_rate_budget_exceeded) ==
               1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::page_miss_rate_budget_exceeded) ==
               1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::temporal_churn_budget_exceeded) ==
               1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::missing_required_geometry) == 1U);
    MK_REQUIRE(
        diagnostic_count(result,
                         mirakana::MavgQualityGovernorDiagnosticCode::rt_consistency_diagnostic_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::resident_gpu_budget_exceeded) ==
               1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::upload_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::draw_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::dispatch_budget_exceeded) == 1U);
    MK_REQUIRE(!result.ready);
    MK_REQUIRE(result.rows.empty());
    MK_REQUIRE(result.row_count == 1U);
    MK_REQUIRE(result.ready_row_count == 1U);
    MK_REQUIRE(result.replay_hash == 0U);
}

MK_TEST("mavg quality governor replay hash includes budget policy") {
    const auto base_result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = make_limits(),
        .rows = {make_ready_row()},
        .row_budget = 8U,
        .seed = 144U,
    });

    auto relaxed_limits = make_limits();
    relaxed_limits.max_draw_count = 2'000U;
    const auto relaxed_result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = relaxed_limits,
        .rows = {make_ready_row()},
        .row_budget = 8U,
        .seed = 144U,
    });

    const auto tighter_row_budget_result =
        mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
            .limits = make_limits(),
            .rows = {make_ready_row()},
            .row_budget = 4U,
            .seed = 144U,
        });

    MK_REQUIRE(base_result.status == mirakana::MavgQualityGovernorStatus::ready);
    MK_REQUIRE(relaxed_result.status == mirakana::MavgQualityGovernorStatus::ready);
    MK_REQUIRE(tighter_row_budget_result.status == mirakana::MavgQualityGovernorStatus::ready);
    MK_REQUIRE(base_result.replay_hash != relaxed_result.replay_hash);
    MK_REQUIRE(base_result.replay_hash != tighter_row_budget_result.replay_hash);
}

MK_TEST("mavg quality governor requires explicit rt consistency evidence when selected") {
    auto row = make_ready_row();
    row.feature = mirakana::MavgQualityGovernorFeatureKind::ray_tracing;
    row.rt_consistency_evidence = false;

    const auto result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = make_limits(),
        .rows = {row},
        .row_budget = 8U,
        .require_rt_consistency_evidence = true,
        .seed = 34U,
    });

    MK_REQUIRE(result.status == mirakana::MavgQualityGovernorStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::missing_rt_consistency_evidence) ==
               1U);
    MK_REQUIRE(!mirakana::has_mavg_quality_governor_diagnostic(
        result, mirakana::MavgQualityGovernorDiagnosticCode::rt_consistency_diagnostic_budget_exceeded));
}

MK_TEST("mavg quality governor rejects invalid limits and row budgets") {
    const auto result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{
        .limits = mirakana::MavgQualityGovernorLimits{},
        .rows = {make_ready_row(), make_ready_row(2U)},
        .row_budget = 1U,
        .seed = 55U,
    });

    MK_REQUIRE(result.status == mirakana::MavgQualityGovernorStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::invalid_limits) == 1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::MavgQualityGovernorDiagnosticCode::row_budget_exceeded) == 1U);
}

MK_TEST("mavg quality governor reports no rows without benchmark readiness") {
    const auto result = mirakana::evaluate_mavg_quality_governor(mirakana::MavgQualityGovernorRequest{});

    MK_REQUIRE(result.status == mirakana::MavgQualityGovernorStatus::no_rows);
    MK_REQUIRE(!result.ready);
    MK_REQUIRE(result.row_count == 0U);
    MK_REQUIRE(result.replay_hash == 0U);
}

int main() {
    return mirakana::test::run_all();
}
