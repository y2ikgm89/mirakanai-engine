// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

enum class Runtime2DCommercialPerformanceWorkloadKind : std::uint8_t {
    dense_sprites,
    large_tilemap,
    ui_heavy_scene,
    animation_heavy_scene,
    streaming_stress,
    physics_stress,
    audio_input_stress,
    long_running_playtest,
};

enum class Runtime2DCommercialPerformanceMetricKind : std::uint8_t {
    cpu_frame_time,
    gpu_frame_time,
    input_to_present_latency,
    present_pacing,
    io_decompression_upload_overlap,
    memory_high_water,
    gpu_residency_pressure,
    allocator_churn,
    job_queue_depth,
    cache_misses,
    package_miss_pop_in,
};

enum class Runtime2DCommercialPerformanceOfficialSourceKind : std::uint8_t {
    microsoft_d3d12,
    microsoft_wpt,
    microsoft_pix,
    khronos_vulkan_timestamp_debug_utils,
    apple_xctrace_metal_capture,
    linux_perf,
    repository_legal_policy,
};

enum class Runtime2DCommercialPerformanceDiagnosticCode : std::uint8_t {
    workload_not_ready,
    metric_not_ready,
    host_threshold_not_ready,
    over_budget,
    official_source_not_ready,
    selected_package_claim_missing,
    broad_optimization_claim,
    cross_vendor_parity_claim,
    cross_backend_parity_claim,
    public_native_handles,
    renderer_rhi_residency_claim,
    allocator_gpu_budget_enforcement_claim,
    pgo_lto_default_lane_claim,
    profiler_artifact_ready_without_artifact_claim,
    external_engine_compatibility_claim,
    legal_approval_claim,
};

struct Runtime2DCommercialPerformanceWorkloadRow {
    std::string id;
    Runtime2DCommercialPerformanceWorkloadKind kind{Runtime2DCommercialPerformanceWorkloadKind::dense_sprites};
    std::string host_class_id;
    std::string validation_recipe_id;
    bool ready{false};
    bool package_visible{false};
};

struct Runtime2DCommercialPerformanceMetricRow {
    std::string id;
    Runtime2DCommercialPerformanceMetricKind kind{Runtime2DCommercialPerformanceMetricKind::cpu_frame_time};
    std::string host_class_id;
    std::string counter_name;
    std::uint64_t p50_value{0U};
    std::uint64_t p95_value{0U};
    std::uint64_t p99_value{0U};
    std::uint64_t p50_limit{0U};
    std::uint64_t p95_limit{0U};
    std::uint64_t p99_limit{0U};
    bool ready{false};
    bool package_visible{false};
    bool host_gated_artifact{false};
    bool threshold_host_class_specific{false};
};

struct Runtime2DCommercialPerformanceOfficialSourceRow {
    std::string id;
    Runtime2DCommercialPerformanceOfficialSourceKind kind{
        Runtime2DCommercialPerformanceOfficialSourceKind::microsoft_d3d12};
    std::string url;
    bool ready{false};
    bool official{false};
    bool public_docs_only{false};
};

struct Runtime2DCommercialPerformanceDiagnostic {
    Runtime2DCommercialPerformanceDiagnosticCode code{Runtime2DCommercialPerformanceDiagnosticCode::workload_not_ready};
    std::string row_id;
    std::string message;
};

struct Runtime2DCommercialPerformanceRegressionGateDesc {
    std::vector<Runtime2DCommercialPerformanceWorkloadRow> workload_rows;
    std::vector<Runtime2DCommercialPerformanceMetricRow> metric_rows;
    std::array<Runtime2DCommercialPerformanceOfficialSourceRow, 7U> official_source_rows{};
    bool selected_package_regression_gate_claim{false};
    bool broad_optimization_claim{false};
    bool cross_vendor_parity_claim{false};
    bool cross_backend_parity_claim{false};
    bool public_native_handles{false};
    bool renderer_rhi_residency_claim{false};
    bool allocator_gpu_budget_enforcement_claim{false};
    bool pgo_lto_default_lane_claim{false};
    bool profiler_artifact_ready_without_artifact_claim{false};
    bool external_engine_compatibility_claim{false};
    bool legal_approval_claim{false};
};

struct Runtime2DCommercialPerformanceRegressionGateResult {
    bool ready{false};
    bool workload_gate_ready{false};
    bool metric_gate_ready{false};
    bool official_source_ready{false};
    bool host_threshold_gate_ready{false};
    std::vector<Runtime2DCommercialPerformanceDiagnostic> diagnostics;
    std::size_t workload_rows{0U};
    std::size_t metric_rows{0U};
    std::size_t host_class_threshold_rows{0U};
    std::size_t package_visible_metric_rows{0U};
    std::size_t host_gated_profiler_artifact_rows{0U};
    std::size_t official_source_rows{0U};
    std::size_t over_budget_rows{0U};
    std::size_t broad_optimization_claim_rows{0U};
    std::size_t cross_vendor_parity_claim_rows{0U};
    std::size_t cross_backend_parity_claim_rows{0U};
    std::size_t native_handle_access_rows{0U};
    std::size_t renderer_rhi_residency_claim_rows{0U};
    std::size_t allocator_gpu_budget_enforcement_claim_rows{0U};
    std::size_t pgo_lto_default_lane_claim_rows{0U};
    std::size_t profiler_artifact_ready_without_artifact_rows{0U};
    std::size_t external_engine_claim_rows{0U};
    std::size_t legal_approval_claim_rows{0U};
    std::uint64_t cpu_frame_p50_us{0U};
    std::uint64_t cpu_frame_p95_us{0U};
    std::uint64_t cpu_frame_p99_us{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] std::string_view
runtime_2d_commercial_performance_workload_kind_name(Runtime2DCommercialPerformanceWorkloadKind kind) noexcept;
[[nodiscard]] std::string_view
runtime_2d_commercial_performance_metric_kind_name(Runtime2DCommercialPerformanceMetricKind kind) noexcept;
[[nodiscard]] std::string_view runtime_2d_commercial_performance_official_source_kind_name(
    Runtime2DCommercialPerformanceOfficialSourceKind kind) noexcept;
[[nodiscard]] Runtime2DCommercialPerformanceRegressionGateResult
evaluate_runtime_2d_commercial_performance_regression_gate(
    const Runtime2DCommercialPerformanceRegressionGateDesc& desc);

} // namespace mirakana::runtime
