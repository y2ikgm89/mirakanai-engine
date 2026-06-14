// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class EnvironmentOptimizationMeasurementStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    no_rows,
    invalid_request,
};

enum class EnvironmentOptimizationWorkload : std::uint8_t {
    preset_pack_flythrough = 0,
    storm_precipitation,
    dense_volumetric_fog,
    volumetric_cloud_sunset,
    snowfield_material_weathering,
    weather_simulation_stress,
    asset_library_cold_load,
};

enum class EnvironmentOptimizationRowStatus : std::uint8_t {
    ready = 0,
    host_gated,
    unsupported,
};

enum class EnvironmentOptimizationDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_workload_id,
    duplicate_workload_row,
    missing_required_workload,
    invalid_row_taxonomy,
    invalid_backend,
    stale_package_revision,
    quality_tier_mismatch,
    missing_host_identity,
    missing_measurement_window,
    missing_before_after_metrics,
    non_finite_sample,
    missing_tool_evidence,
    missing_profiler_artifact,
    missing_regression_budget,
    regression_budget_exceeded,
    diagnostics_nonzero,
    unsupported_broad_optimization_claim,
    unsupported_native_handle_access,
    unsupported_inferred_backend,
    row_budget_exceeded,
};

struct EnvironmentOptimizationMetricSet {
    std::uint64_t cpu_frame_p95_us{0U};
    std::uint64_t gpu_frame_p95_us{0U};
    std::uint64_t memory_peak_bytes{0U};
    std::uint64_t transient_gpu_bytes{0U};
    std::uint64_t upload_bytes{0U};
    std::uint64_t draw_count{0U};
    std::uint64_t dispatch_count{0U};
    std::uint64_t barrier_count{0U};
    std::uint64_t texture_residency_bytes{0U};
    std::uint64_t package_load_us{0U};
    std::uint64_t stutter_frames{0U};
    bool finite_samples{false};
};

struct EnvironmentOptimizationRegressionBudget {
    std::uint64_t max_cpu_frame_p95_us{0U};
    std::uint64_t max_gpu_frame_p95_us{0U};
    std::uint64_t max_memory_peak_bytes{0U};
    std::uint64_t max_transient_gpu_bytes{0U};
    std::uint64_t max_upload_bytes{0U};
    std::uint64_t max_draw_count{0U};
    std::uint64_t max_dispatch_count{0U};
    std::uint64_t max_barrier_count{0U};
    std::uint64_t max_texture_residency_bytes{0U};
    std::uint64_t max_package_load_us{0U};
    std::uint64_t max_stutter_frames{0U};
};

struct EnvironmentOptimizationMeasurementRow {
    std::string workload_id;
    EnvironmentOptimizationWorkload workload{EnvironmentOptimizationWorkload::preset_pack_flythrough};
    rhi::BackendKind backend{rhi::BackendKind::null};
    EnvironmentOptimizationRowStatus status{EnvironmentOptimizationRowStatus::ready};
    std::string host_os;
    std::string cpu_name;
    std::string gpu_name;
    std::string gpu_driver_version;
    std::string profiler_tool;
    std::string profiler_tool_version;
    std::string profiler_artifact_id;
    std::string package_revision;
    std::string quality_tier;
    std::string resolution;
    std::uint32_t warmup_frames{0U};
    std::uint32_t sample_frames{0U};
    EnvironmentOptimizationMetricSet before;
    EnvironmentOptimizationMetricSet after;
    EnvironmentOptimizationRegressionBudget budget;
    bool before_after_ready{false};
    bool host_tool_versions_ready{false};
    bool profiler_artifact_ready{false};
    bool repository_counters_ready{false};
    bool timestamp_query_evidence_ready{false};
    bool regression_budget_ready{false};
    std::uint32_t diagnostic_count{0U};
    bool broad_optimization_claimed{false};
    bool native_handle_access{false};
    bool inferred_from_other_backend{false};
    std::uint32_t source_index{0U};
};

struct EnvironmentOptimizationMeasurementRequest {
    std::vector<EnvironmentOptimizationMeasurementRow> rows;
    std::string expected_package_revision;
    std::string expected_quality_tier;
    bool environment_backend_parity_ready{false};
    std::size_t required_workload_count{7U};
    std::size_t row_budget{64U};
    std::uint64_t seed{0U};
};

struct EnvironmentOptimizationDiagnostic {
    EnvironmentOptimizationDiagnosticCode code{EnvironmentOptimizationDiagnosticCode::none};
    EnvironmentOptimizationWorkload workload{EnvironmentOptimizationWorkload::preset_pack_flythrough};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct EnvironmentOptimizationMeasurementPlan {
    EnvironmentOptimizationMeasurementStatus status{EnvironmentOptimizationMeasurementStatus::invalid_request};
    std::vector<EnvironmentOptimizationDiagnostic> diagnostics;
    std::vector<EnvironmentOptimizationMeasurementRow> rows;
    std::size_t row_count{0U};
    std::size_t required_workload_count{7U};
    std::size_t measured_workload_count{0U};
    std::size_t before_after_pair_count{0U};
    std::size_t regression_budget_row_count{0U};
    std::size_t over_budget_row_count{0U};
    std::uint64_t replay_hash{0U};
    bool d3d12_preset_pack_flythrough_measured{false};
    bool d3d12_storm_precipitation_measured{false};
    bool d3d12_dense_volumetric_fog_measured{false};
    bool d3d12_volumetric_cloud_sunset_measured{false};
    bool d3d12_snowfield_material_weathering_measured{false};
    bool d3d12_weather_simulation_stress_measured{false};
    bool d3d12_asset_library_cold_load_measured{false};
    bool environment_backend_parity_ready{false};
    bool environment_broad_optimization_ready{false};
    bool invoked_gpu_commands{false};
    bool exposed_native_handles{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews measured environment optimization evidence rows. The planner is value-only: it never starts profiling,
/// submits GPU commands, or exposes native renderer handles. Broad optimization readiness requires all normalized
/// workloads plus backend parity; a single D3D12 flythrough measurement remains host-evidence-required.
[[nodiscard]] EnvironmentOptimizationMeasurementPlan
plan_environment_optimization_measurement(const EnvironmentOptimizationMeasurementRequest& request);

} // namespace mirakana
