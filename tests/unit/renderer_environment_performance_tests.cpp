// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/environment_performance.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::EnvironmentOptimizationDiagnosticCode;
using mirakana::EnvironmentOptimizationMeasurementStatus;
using mirakana::EnvironmentOptimizationRowStatus;
using mirakana::EnvironmentOptimizationWorkload;

constexpr EnvironmentOptimizationWorkload kRequiredWorkloads[] = {
    EnvironmentOptimizationWorkload::preset_pack_flythrough,
    EnvironmentOptimizationWorkload::storm_precipitation,
    EnvironmentOptimizationWorkload::dense_volumetric_fog,
    EnvironmentOptimizationWorkload::volumetric_cloud_sunset,
    EnvironmentOptimizationWorkload::snowfield_material_weathering,
    EnvironmentOptimizationWorkload::weather_simulation_stress,
    EnvironmentOptimizationWorkload::asset_library_cold_load,
};

[[nodiscard]] std::string workload_id(EnvironmentOptimizationWorkload workload) {
    switch (workload) {
    case EnvironmentOptimizationWorkload::preset_pack_flythrough:
        return "preset_pack_flythrough";
    case EnvironmentOptimizationWorkload::storm_precipitation:
        return "storm_precipitation";
    case EnvironmentOptimizationWorkload::dense_volumetric_fog:
        return "dense_volumetric_fog";
    case EnvironmentOptimizationWorkload::volumetric_cloud_sunset:
        return "volumetric_cloud_sunset";
    case EnvironmentOptimizationWorkload::snowfield_material_weathering:
        return "snowfield_material_weathering";
    case EnvironmentOptimizationWorkload::weather_simulation_stress:
        return "weather_simulation_stress";
    case EnvironmentOptimizationWorkload::asset_library_cold_load:
        return "asset_library_cold_load";
    }
    return "unknown";
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet before_metrics(std::uint64_t offset = 0U) {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 16000U + offset,
        .gpu_frame_p95_us = 14000U + offset,
        .memory_peak_bytes = (512U * 1024U * 1024U) + offset,
        .transient_gpu_bytes = (128U * 1024U * 1024U) + offset,
        .upload_bytes = (32U * 1024U * 1024U) + offset,
        .draw_count = 120U + offset,
        .dispatch_count = 8U,
        .barrier_count = 42U + offset,
        .texture_residency_bytes = (384U * 1024U * 1024U) + offset,
        .package_load_us = 45000U + offset,
        .stutter_frames = 1U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMetricSet after_metrics(std::uint64_t offset = 0U) {
    return mirakana::EnvironmentOptimizationMetricSet{
        .cpu_frame_p95_us = 15000U + offset,
        .gpu_frame_p95_us = 13200U + offset,
        .memory_peak_bytes = (500U * 1024U * 1024U) + offset,
        .transient_gpu_bytes = (112U * 1024U * 1024U) + offset,
        .upload_bytes = (24U * 1024U * 1024U) + offset,
        .draw_count = 110U + offset,
        .dispatch_count = 8U,
        .barrier_count = 36U,
        .texture_residency_bytes = (360U * 1024U * 1024U) + offset,
        .package_load_us = 40000U + offset,
        .stutter_frames = 0U,
        .finite_samples = true,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationRegressionBudget regression_budget() {
    return mirakana::EnvironmentOptimizationRegressionBudget{
        .max_cpu_frame_p95_us = 16670U,
        .max_gpu_frame_p95_us = 16670U,
        .max_memory_peak_bytes = 512U * 1024U * 1024U,
        .max_transient_gpu_bytes = 128U * 1024U * 1024U,
        .max_upload_bytes = 32U * 1024U * 1024U,
        .max_draw_count = 120U,
        .max_dispatch_count = 8U,
        .max_barrier_count = 42U,
        .max_texture_residency_bytes = 384U * 1024U * 1024U,
        .max_package_load_us = 45000U,
        .max_stutter_frames = 1U,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMeasurementRow make_ready_row(EnvironmentOptimizationWorkload workload,
                                                                             std::uint32_t source_index) {
    return mirakana::EnvironmentOptimizationMeasurementRow{
        .workload_id = workload_id(workload),
        .workload = workload,
        .backend = mirakana::rhi::BackendKind::d3d12,
        .status = EnvironmentOptimizationRowStatus::ready,
        .host_os = "Windows 11 24H2",
        .cpu_name = "host-cpu",
        .gpu_name = "Microsoft WARP D3D12",
        .gpu_driver_version = "10.0.26100",
        .profiler_tool = "WPR+PIX+D3D12TimestampQuery",
        .profiler_tool_version = "windows-sdk-10.0.26100",
        .profiler_artifact_id = "artifacts/environment/preset-pack-flythrough-d3d12.etl",
        .package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .quality_tier = "high",
        .resolution = "1920x1080",
        .warmup_frames = 30U,
        .sample_frames = 120U,
        .before = before_metrics(source_index),
        .after = after_metrics(source_index),
        .budget = regression_budget(),
        .before_after_ready = true,
        .host_tool_versions_ready = true,
        .profiler_artifact_ready = true,
        .repository_counters_ready = true,
        .timestamp_query_evidence_ready = true,
        .regression_budget_ready = true,
        .diagnostic_count = 0U,
        .broad_optimization_claimed = false,
        .native_handle_access = false,
        .inferred_from_other_backend = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::EnvironmentOptimizationMeasurementRequest make_request(bool include_all_workloads) {
    std::vector<mirakana::EnvironmentOptimizationMeasurementRow> rows;
    std::uint32_t source_index{1U};
    rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::preset_pack_flythrough, source_index++));
    if (include_all_workloads) {
        for (const auto workload : kRequiredWorkloads) {
            if (workload == EnvironmentOptimizationWorkload::preset_pack_flythrough) {
                continue;
            }
            rows.push_back(make_ready_row(workload, source_index++));
        }
    }

    return mirakana::EnvironmentOptimizationMeasurementRequest{
        .rows = std::move(rows),
        .expected_package_revision = "sample_desktop_runtime_game:environment-commercial-v1",
        .expected_quality_tier = "high",
        .environment_backend_parity_ready = include_all_workloads,
        .required_workload_count = 7U,
        .row_budget = 16U,
        .seed = 20260614U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::EnvironmentOptimizationMeasurementPlan& plan,
                                           EnvironmentOptimizationDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("environment optimization measurement records d3d12 flythrough without broad readiness promotion") {
    const auto plan = mirakana::plan_environment_optimization_measurement(make_request(false));

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 1U);
    MK_REQUIRE(plan.required_workload_count == 7U);
    MK_REQUIRE(plan.measured_workload_count == 1U);
    MK_REQUIRE(plan.before_after_pair_count == 1U);
    MK_REQUIRE(plan.regression_budget_row_count == 1U);
    MK_REQUIRE(plan.over_budget_row_count == 0U);
    MK_REQUIRE(plan.d3d12_preset_pack_flythrough_measured);
    MK_REQUIRE(!plan.d3d12_storm_precipitation_measured);
    MK_REQUIRE(!plan.d3d12_dense_volumetric_fog_measured);
    MK_REQUIRE(!plan.d3d12_volumetric_cloud_sunset_measured);
    MK_REQUIRE(!plan.d3d12_snowfield_material_weathering_measured);
    MK_REQUIRE(!plan.d3d12_weather_simulation_stress_measured);
    MK_REQUIRE(!plan.environment_broad_optimization_ready);
    MK_REQUIRE(!plan.exposed_native_handles);
    MK_REQUIRE(!plan.invoked_gpu_commands);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("environment optimization measurement records d3d12 storm precipitation without broad readiness promotion") {
    auto request = make_request(false);
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::storm_precipitation, 2U));

    const auto plan = mirakana::plan_environment_optimization_measurement(request);

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 2U);
    MK_REQUIRE(plan.required_workload_count == 7U);
    MK_REQUIRE(plan.measured_workload_count == 2U);
    MK_REQUIRE(plan.before_after_pair_count == 2U);
    MK_REQUIRE(plan.regression_budget_row_count == 2U);
    MK_REQUIRE(plan.over_budget_row_count == 0U);
    MK_REQUIRE(plan.d3d12_preset_pack_flythrough_measured);
    MK_REQUIRE(plan.d3d12_storm_precipitation_measured);
    MK_REQUIRE(!plan.d3d12_dense_volumetric_fog_measured);
    MK_REQUIRE(!plan.d3d12_volumetric_cloud_sunset_measured);
    MK_REQUIRE(!plan.d3d12_snowfield_material_weathering_measured);
    MK_REQUIRE(!plan.d3d12_weather_simulation_stress_measured);
    MK_REQUIRE(!plan.environment_broad_optimization_ready);
    MK_REQUIRE(!plan.exposed_native_handles);
    MK_REQUIRE(!plan.invoked_gpu_commands);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("environment optimization measurement records d3d12 dense volumetric fog without broad readiness promotion") {
    auto request = make_request(false);
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::storm_precipitation, 2U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::dense_volumetric_fog, 3U));

    const auto plan = mirakana::plan_environment_optimization_measurement(request);

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 3U);
    MK_REQUIRE(plan.required_workload_count == 7U);
    MK_REQUIRE(plan.measured_workload_count == 3U);
    MK_REQUIRE(plan.before_after_pair_count == 3U);
    MK_REQUIRE(plan.regression_budget_row_count == 3U);
    MK_REQUIRE(plan.over_budget_row_count == 0U);
    MK_REQUIRE(plan.d3d12_preset_pack_flythrough_measured);
    MK_REQUIRE(plan.d3d12_storm_precipitation_measured);
    MK_REQUIRE(plan.d3d12_dense_volumetric_fog_measured);
    MK_REQUIRE(!plan.d3d12_volumetric_cloud_sunset_measured);
    MK_REQUIRE(!plan.d3d12_snowfield_material_weathering_measured);
    MK_REQUIRE(!plan.d3d12_weather_simulation_stress_measured);
    MK_REQUIRE(!plan.environment_broad_optimization_ready);
    MK_REQUIRE(!plan.exposed_native_handles);
    MK_REQUIRE(!plan.invoked_gpu_commands);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST(
    "environment optimization measurement records d3d12 volumetric cloud sunset without broad readiness promotion") {
    auto request = make_request(false);
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::storm_precipitation, 2U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::dense_volumetric_fog, 3U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::volumetric_cloud_sunset, 4U));

    const auto plan = mirakana::plan_environment_optimization_measurement(request);

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 4U);
    MK_REQUIRE(plan.required_workload_count == 7U);
    MK_REQUIRE(plan.measured_workload_count == 4U);
    MK_REQUIRE(plan.before_after_pair_count == 4U);
    MK_REQUIRE(plan.regression_budget_row_count == 4U);
    MK_REQUIRE(plan.over_budget_row_count == 0U);
    MK_REQUIRE(plan.d3d12_preset_pack_flythrough_measured);
    MK_REQUIRE(plan.d3d12_storm_precipitation_measured);
    MK_REQUIRE(plan.d3d12_dense_volumetric_fog_measured);
    MK_REQUIRE(plan.d3d12_volumetric_cloud_sunset_measured);
    MK_REQUIRE(!plan.d3d12_snowfield_material_weathering_measured);
    MK_REQUIRE(!plan.d3d12_weather_simulation_stress_measured);
    MK_REQUIRE(!plan.environment_broad_optimization_ready);
    MK_REQUIRE(!plan.exposed_native_handles);
    MK_REQUIRE(!plan.invoked_gpu_commands);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("environment optimization measurement records d3d12 snowfield material weathering without broad readiness "
        "promotion") {
    auto request = make_request(false);
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::storm_precipitation, 2U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::dense_volumetric_fog, 3U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::volumetric_cloud_sunset, 4U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::snowfield_material_weathering, 5U));

    const auto plan = mirakana::plan_environment_optimization_measurement(request);

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 5U);
    MK_REQUIRE(plan.required_workload_count == 7U);
    MK_REQUIRE(plan.measured_workload_count == 5U);
    MK_REQUIRE(plan.before_after_pair_count == 5U);
    MK_REQUIRE(plan.regression_budget_row_count == 5U);
    MK_REQUIRE(plan.over_budget_row_count == 0U);
    MK_REQUIRE(plan.d3d12_preset_pack_flythrough_measured);
    MK_REQUIRE(plan.d3d12_storm_precipitation_measured);
    MK_REQUIRE(plan.d3d12_dense_volumetric_fog_measured);
    MK_REQUIRE(plan.d3d12_volumetric_cloud_sunset_measured);
    MK_REQUIRE(plan.d3d12_snowfield_material_weathering_measured);
    MK_REQUIRE(!plan.d3d12_weather_simulation_stress_measured);
    MK_REQUIRE(!plan.environment_broad_optimization_ready);
    MK_REQUIRE(!plan.exposed_native_handles);
    MK_REQUIRE(!plan.invoked_gpu_commands);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("environment optimization measurement records d3d12 weather simulation stress without broad readiness "
        "promotion") {
    auto request = make_request(false);
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::storm_precipitation, 2U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::dense_volumetric_fog, 3U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::volumetric_cloud_sunset, 4U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::snowfield_material_weathering, 5U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::weather_simulation_stress, 6U));

    const auto plan = mirakana::plan_environment_optimization_measurement(request);

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 6U);
    MK_REQUIRE(plan.required_workload_count == 7U);
    MK_REQUIRE(plan.measured_workload_count == 6U);
    MK_REQUIRE(plan.before_after_pair_count == 6U);
    MK_REQUIRE(plan.regression_budget_row_count == 6U);
    MK_REQUIRE(plan.over_budget_row_count == 0U);
    MK_REQUIRE(plan.d3d12_preset_pack_flythrough_measured);
    MK_REQUIRE(plan.d3d12_storm_precipitation_measured);
    MK_REQUIRE(plan.d3d12_dense_volumetric_fog_measured);
    MK_REQUIRE(plan.d3d12_volumetric_cloud_sunset_measured);
    MK_REQUIRE(plan.d3d12_snowfield_material_weathering_measured);
    MK_REQUIRE(plan.d3d12_weather_simulation_stress_measured);
    MK_REQUIRE(!plan.d3d12_asset_library_cold_load_measured);
    MK_REQUIRE(!plan.environment_broad_optimization_ready);
    MK_REQUIRE(!plan.exposed_native_handles);
    MK_REQUIRE(!plan.invoked_gpu_commands);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("environment optimization measurement records d3d12 asset library cold load without backend parity "
        "promotion") {
    auto request = make_request(false);
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::storm_precipitation, 2U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::dense_volumetric_fog, 3U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::volumetric_cloud_sunset, 4U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::snowfield_material_weathering, 5U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::weather_simulation_stress, 6U));
    request.rows.push_back(make_ready_row(EnvironmentOptimizationWorkload::asset_library_cold_load, 7U));

    const auto plan = mirakana::plan_environment_optimization_measurement(request);

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 7U);
    MK_REQUIRE(plan.required_workload_count == 7U);
    MK_REQUIRE(plan.measured_workload_count == 7U);
    MK_REQUIRE(plan.before_after_pair_count == 7U);
    MK_REQUIRE(plan.regression_budget_row_count == 7U);
    MK_REQUIRE(plan.over_budget_row_count == 0U);
    MK_REQUIRE(plan.d3d12_preset_pack_flythrough_measured);
    MK_REQUIRE(plan.d3d12_storm_precipitation_measured);
    MK_REQUIRE(plan.d3d12_dense_volumetric_fog_measured);
    MK_REQUIRE(plan.d3d12_volumetric_cloud_sunset_measured);
    MK_REQUIRE(plan.d3d12_snowfield_material_weathering_measured);
    MK_REQUIRE(plan.d3d12_weather_simulation_stress_measured);
    MK_REQUIRE(plan.d3d12_asset_library_cold_load_measured);
    MK_REQUIRE(!plan.environment_broad_optimization_ready);
    MK_REQUIRE(!plan.exposed_native_handles);
    MK_REQUIRE(!plan.invoked_gpu_commands);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("environment optimization measurement is broad ready only with every workload and backend parity") {
    const auto plan = mirakana::plan_environment_optimization_measurement(make_request(true));

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.row_count == 7U);
    MK_REQUIRE(plan.required_workload_count == 7U);
    MK_REQUIRE(plan.measured_workload_count == 7U);
    MK_REQUIRE(plan.before_after_pair_count == 7U);
    MK_REQUIRE(plan.regression_budget_row_count == 7U);
    MK_REQUIRE(plan.over_budget_row_count == 0U);
    MK_REQUIRE(plan.d3d12_preset_pack_flythrough_measured);
    MK_REQUIRE(plan.d3d12_storm_precipitation_measured);
    MK_REQUIRE(plan.d3d12_dense_volumetric_fog_measured);
    MK_REQUIRE(plan.d3d12_volumetric_cloud_sunset_measured);
    MK_REQUIRE(plan.d3d12_snowfield_material_weathering_measured);
    MK_REQUIRE(plan.d3d12_weather_simulation_stress_measured);
    MK_REQUIRE(plan.d3d12_asset_library_cold_load_measured);
    MK_REQUIRE(plan.environment_broad_optimization_ready);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("environment optimization measurement rejects missing host tool identity and before after evidence") {
    auto request = make_request(false);
    auto& row = request.rows[0];
    row.host_os.clear();
    row.cpu_name.clear();
    row.gpu_name.clear();
    row.gpu_driver_version.clear();
    row.profiler_tool.clear();
    row.profiler_tool_version.clear();
    row.profiler_artifact_id.clear();
    row.sample_frames = 0U;
    row.before_after_ready = false;
    row.host_tool_versions_ready = false;
    row.profiler_artifact_ready = false;
    row.timestamp_query_evidence_ready = false;

    const auto plan = mirakana::plan_environment_optimization_measurement(request);

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::missing_host_identity) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::missing_measurement_window) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::missing_before_after_metrics) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::missing_tool_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::missing_profiler_artifact) == 1U);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("environment optimization measurement rejects over budget samples and unsupported claims") {
    auto request = make_request(false);
    auto& row = request.rows[0];
    row.after.cpu_frame_p95_us = 18000U;
    row.after.gpu_frame_p95_us = 19000U;
    row.after.finite_samples = false;
    row.diagnostic_count = 3U;
    row.broad_optimization_claimed = true;
    row.native_handle_access = true;
    row.inferred_from_other_backend = true;

    const auto plan = mirakana::plan_environment_optimization_measurement(request);

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::non_finite_sample) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::regression_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::diagnostics_nonzero) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::unsupported_broad_optimization_claim) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::unsupported_native_handle_access) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::unsupported_inferred_backend) == 1U);
    MK_REQUIRE(plan.over_budget_row_count == 1U);
    MK_REQUIRE(!plan.environment_broad_optimization_ready);
}

MK_TEST("environment optimization measurement rejects missing workload rows and duplicates") {
    auto request = make_request(true);
    request.rows.pop_back();
    request.rows.push_back(request.rows.front());
    request.rows.back().source_index = 99U;

    const auto plan = mirakana::plan_environment_optimization_measurement(request);

    MK_REQUIRE(plan.status == EnvironmentOptimizationMeasurementStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::missing_required_workload) == 1U);
    MK_REQUIRE(diagnostic_count(plan, EnvironmentOptimizationDiagnosticCode::duplicate_workload_row) == 1U);
    MK_REQUIRE(!plan.environment_broad_optimization_ready);
    MK_REQUIRE(plan.replay_hash == 0U);
}

int main() {
    return mirakana::test::run_all();
}
