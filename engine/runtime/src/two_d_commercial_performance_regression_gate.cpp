// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/two_d_commercial_performance_regression_gate.hpp"

#include <array>
#include <string>
#include <utility>

namespace mirakana::runtime {
namespace {

constexpr std::array<Runtime2DCommercialPerformanceWorkloadKind, 8U> kRequiredWorkloadKinds{
    Runtime2DCommercialPerformanceWorkloadKind::dense_sprites,
    Runtime2DCommercialPerformanceWorkloadKind::large_tilemap,
    Runtime2DCommercialPerformanceWorkloadKind::ui_heavy_scene,
    Runtime2DCommercialPerformanceWorkloadKind::animation_heavy_scene,
    Runtime2DCommercialPerformanceWorkloadKind::streaming_stress,
    Runtime2DCommercialPerformanceWorkloadKind::physics_stress,
    Runtime2DCommercialPerformanceWorkloadKind::audio_input_stress,
    Runtime2DCommercialPerformanceWorkloadKind::long_running_playtest,
};

constexpr std::array<Runtime2DCommercialPerformanceMetricKind, 11U> kRequiredMetricKinds{
    Runtime2DCommercialPerformanceMetricKind::cpu_frame_time,
    Runtime2DCommercialPerformanceMetricKind::gpu_frame_time,
    Runtime2DCommercialPerformanceMetricKind::input_to_present_latency,
    Runtime2DCommercialPerformanceMetricKind::present_pacing,
    Runtime2DCommercialPerformanceMetricKind::io_decompression_upload_overlap,
    Runtime2DCommercialPerformanceMetricKind::memory_high_water,
    Runtime2DCommercialPerformanceMetricKind::gpu_residency_pressure,
    Runtime2DCommercialPerformanceMetricKind::allocator_churn,
    Runtime2DCommercialPerformanceMetricKind::job_queue_depth,
    Runtime2DCommercialPerformanceMetricKind::cache_misses,
    Runtime2DCommercialPerformanceMetricKind::package_miss_pop_in,
};

constexpr std::array<Runtime2DCommercialPerformanceOfficialSourceKind, 7U> kRequiredOfficialSourceKinds{
    Runtime2DCommercialPerformanceOfficialSourceKind::microsoft_d3d12,
    Runtime2DCommercialPerformanceOfficialSourceKind::microsoft_wpt,
    Runtime2DCommercialPerformanceOfficialSourceKind::microsoft_pix,
    Runtime2DCommercialPerformanceOfficialSourceKind::khronos_vulkan_timestamp_debug_utils,
    Runtime2DCommercialPerformanceOfficialSourceKind::apple_xctrace_metal_capture,
    Runtime2DCommercialPerformanceOfficialSourceKind::linux_perf,
    Runtime2DCommercialPerformanceOfficialSourceKind::repository_legal_policy,
};

void append_diagnostic(std::vector<Runtime2DCommercialPerformanceDiagnostic>& diagnostics,
                       Runtime2DCommercialPerformanceDiagnosticCode code, std::string row_id, std::string message) {
    diagnostics.push_back(Runtime2DCommercialPerformanceDiagnostic{
        .code = code,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool is_empty_required_field(const std::string& value) noexcept {
    return value.empty();
}

[[nodiscard]] bool metric_over_budget(const Runtime2DCommercialPerformanceMetricRow& row) noexcept {
    if (row.host_gated_artifact) {
        return false;
    }
    return row.p50_value > row.p50_limit || row.p95_value > row.p95_limit || row.p99_value > row.p99_limit;
}

void evaluate_workloads(const Runtime2DCommercialPerformanceRegressionGateDesc& desc,
                        Runtime2DCommercialPerformanceRegressionGateResult& result) {
    for (const auto& row : desc.workload_rows) {
        if (row.ready && row.package_visible && !is_empty_required_field(row.id) &&
            !is_empty_required_field(row.host_class_id) && !is_empty_required_field(row.validation_recipe_id)) {
            ++result.workload_rows;
        } else {
            append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::workload_not_ready,
                              row.id,
                              "2D commercial performance workload rows must be ready, package-visible, and "
                              "host/recipe-scoped");
        }
    }

    for (const auto kind : kRequiredWorkloadKinds) {
        bool found = false;
        for (const auto& row : desc.workload_rows) {
            if (row.kind == kind && row.ready && row.package_visible && !is_empty_required_field(row.id) &&
                !is_empty_required_field(row.host_class_id) && !is_empty_required_field(row.validation_recipe_id)) {
                found = true;
                break;
            }
        }
        if (!found) {
            append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::workload_not_ready,
                              std::string{runtime_2d_commercial_performance_workload_kind_name(kind)},
                              "2D commercial performance regression gate is missing a required workload row");
        }
    }

    result.workload_gate_ready = result.workload_rows == kRequiredWorkloadKinds.size();
}

void evaluate_metrics(const Runtime2DCommercialPerformanceRegressionGateDesc& desc,
                      Runtime2DCommercialPerformanceRegressionGateResult& result) {
    for (const auto& row : desc.metric_rows) {
        const bool scoped = !is_empty_required_field(row.id) && !is_empty_required_field(row.host_class_id) &&
                            !is_empty_required_field(row.counter_name);
        const bool visible_or_artifact = row.package_visible || row.host_gated_artifact;
        if (row.ready && scoped && visible_or_artifact) {
            ++result.metric_rows;
        } else {
            append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::metric_not_ready,
                              row.id,
                              "2D commercial performance metric rows must be ready, scoped, and package-visible or "
                              "host-gated artifact-backed");
        }
        if (row.threshold_host_class_specific) {
            ++result.host_class_threshold_rows;
        } else {
            append_diagnostic(result.diagnostics,
                              Runtime2DCommercialPerformanceDiagnosticCode::host_threshold_not_ready, row.id,
                              "2D commercial performance metric rows must carry host-class-specific thresholds");
        }
        if (row.package_visible) {
            ++result.package_visible_metric_rows;
        }
        if (row.host_gated_artifact) {
            ++result.host_gated_profiler_artifact_rows;
        }
        if (row.kind == Runtime2DCommercialPerformanceMetricKind::cpu_frame_time) {
            result.cpu_frame_p50_us = row.p50_value;
            result.cpu_frame_p95_us = row.p95_value;
            result.cpu_frame_p99_us = row.p99_value;
        }
        if (metric_over_budget(row)) {
            ++result.over_budget_rows;
            append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::over_budget, row.id,
                              "2D commercial performance metric row exceeds its selected host-class threshold");
        }
    }

    for (const auto kind : kRequiredMetricKinds) {
        bool found = false;
        for (const auto& row : desc.metric_rows) {
            if (row.kind == kind && row.ready && (row.package_visible || row.host_gated_artifact) &&
                row.threshold_host_class_specific) {
                found = true;
                break;
            }
        }
        if (!found) {
            append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::metric_not_ready,
                              std::string{runtime_2d_commercial_performance_metric_kind_name(kind)},
                              "2D commercial performance regression gate is missing a required metric row");
        }
    }

    result.metric_gate_ready = result.metric_rows == kRequiredMetricKinds.size() && result.over_budget_rows == 0U;
    result.host_threshold_gate_ready = result.host_class_threshold_rows == kRequiredMetricKinds.size();
}

void evaluate_official_sources(const Runtime2DCommercialPerformanceRegressionGateDesc& desc,
                               Runtime2DCommercialPerformanceRegressionGateResult& result) {
    for (const auto& row : desc.official_source_rows) {
        if (row.ready && row.official && row.public_docs_only && !is_empty_required_field(row.id) &&
            !is_empty_required_field(row.url)) {
            ++result.official_source_rows;
        } else {
            append_diagnostic(result.diagnostics,
                              Runtime2DCommercialPerformanceDiagnosticCode::official_source_not_ready, row.id,
                              "2D commercial performance official-source rows must be ready, official, "
                              "public-docs-only, and URL-backed");
        }
    }

    for (const auto kind : kRequiredOfficialSourceKinds) {
        bool found = false;
        for (const auto& row : desc.official_source_rows) {
            if (row.kind == kind && row.ready && row.official && row.public_docs_only &&
                !is_empty_required_field(row.id) && !is_empty_required_field(row.url)) {
                found = true;
                break;
            }
        }
        if (!found) {
            append_diagnostic(result.diagnostics,
                              Runtime2DCommercialPerformanceDiagnosticCode::official_source_not_ready,
                              std::string{runtime_2d_commercial_performance_official_source_kind_name(kind)},
                              "2D commercial performance regression gate is missing a required official-source row");
        }
    }

    result.official_source_ready = result.official_source_rows == kRequiredOfficialSourceKinds.size();
}

void evaluate_claims(const Runtime2DCommercialPerformanceRegressionGateDesc& desc,
                     Runtime2DCommercialPerformanceRegressionGateResult& result) {
    if (!desc.selected_package_regression_gate_claim) {
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialPerformanceDiagnosticCode::selected_package_claim_missing, {},
                          "2D commercial performance regression gate must be scoped to the selected package lane");
    }
    if (desc.broad_optimization_claim) {
        ++result.broad_optimization_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::broad_optimization_claim,
                          {},
                          "2D commercial performance regression gate must not claim broad optimized-game readiness");
    }
    if (desc.cross_vendor_parity_claim) {
        ++result.cross_vendor_parity_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::cross_vendor_parity_claim,
                          {}, "2D commercial performance regression gate must not infer cross-vendor parity");
    }
    if (desc.cross_backend_parity_claim) {
        ++result.cross_backend_parity_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::cross_backend_parity_claim,
                          {}, "2D commercial performance regression gate must not infer cross-backend parity");
    }
    if (desc.public_native_handles) {
        ++result.native_handle_access_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::public_native_handles, {},
                          "2D commercial performance regression gate must not expose native or backend handles");
    }
    if (desc.renderer_rhi_residency_claim) {
        ++result.renderer_rhi_residency_claim_rows;
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialPerformanceDiagnosticCode::renderer_rhi_residency_claim, {},
                          "2D commercial performance regression gate must not claim renderer/RHI residency");
    }
    if (desc.allocator_gpu_budget_enforcement_claim) {
        ++result.allocator_gpu_budget_enforcement_claim_rows;
        append_diagnostic(
            result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::allocator_gpu_budget_enforcement_claim,
            {}, "2D commercial performance regression gate must not claim allocator or GPU budget enforcement");
    }
    if (desc.pgo_lto_default_lane_claim) {
        ++result.pgo_lto_default_lane_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::pgo_lto_default_lane_claim,
                          {}, "PGO/LTO performance lanes must stay separate from default debug/dev validation");
    }
    if (desc.profiler_artifact_ready_without_artifact_claim) {
        ++result.profiler_artifact_ready_without_artifact_rows;
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialPerformanceDiagnosticCode::profiler_artifact_ready_without_artifact_claim,
                          {}, "Profiler artifact rows must not be promoted without retained host artifacts");
    }
    if (desc.external_engine_compatibility_claim) {
        ++result.external_engine_claim_rows;
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialPerformanceDiagnosticCode::external_engine_compatibility_claim, {},
                          "2D commercial performance regression gate must not claim external commercial engine "
                          "compatibility");
    }
    if (desc.legal_approval_claim) {
        ++result.legal_approval_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::legal_approval_claim, {},
                          "2D commercial performance regression gate can provide engineering review input but not "
                          "legal approval");
    }
}

} // namespace

bool Runtime2DCommercialPerformanceRegressionGateResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

std::string_view
runtime_2d_commercial_performance_workload_kind_name(Runtime2DCommercialPerformanceWorkloadKind kind) noexcept {
    switch (kind) {
    case Runtime2DCommercialPerformanceWorkloadKind::dense_sprites:
        return "dense_sprites";
    case Runtime2DCommercialPerformanceWorkloadKind::large_tilemap:
        return "large_tilemap";
    case Runtime2DCommercialPerformanceWorkloadKind::ui_heavy_scene:
        return "ui_heavy_scene";
    case Runtime2DCommercialPerformanceWorkloadKind::animation_heavy_scene:
        return "animation_heavy_scene";
    case Runtime2DCommercialPerformanceWorkloadKind::streaming_stress:
        return "streaming_stress";
    case Runtime2DCommercialPerformanceWorkloadKind::physics_stress:
        return "physics_stress";
    case Runtime2DCommercialPerformanceWorkloadKind::audio_input_stress:
        return "audio_input_stress";
    case Runtime2DCommercialPerformanceWorkloadKind::long_running_playtest:
        return "long_running_playtest";
    }
    return "unknown";
}

std::string_view
runtime_2d_commercial_performance_metric_kind_name(Runtime2DCommercialPerformanceMetricKind kind) noexcept {
    switch (kind) {
    case Runtime2DCommercialPerformanceMetricKind::cpu_frame_time:
        return "cpu_frame_time";
    case Runtime2DCommercialPerformanceMetricKind::gpu_frame_time:
        return "gpu_frame_time";
    case Runtime2DCommercialPerformanceMetricKind::input_to_present_latency:
        return "input_to_present_latency";
    case Runtime2DCommercialPerformanceMetricKind::present_pacing:
        return "present_pacing";
    case Runtime2DCommercialPerformanceMetricKind::io_decompression_upload_overlap:
        return "io_decompression_upload_overlap";
    case Runtime2DCommercialPerformanceMetricKind::memory_high_water:
        return "memory_high_water";
    case Runtime2DCommercialPerformanceMetricKind::gpu_residency_pressure:
        return "gpu_residency_pressure";
    case Runtime2DCommercialPerformanceMetricKind::allocator_churn:
        return "allocator_churn";
    case Runtime2DCommercialPerformanceMetricKind::job_queue_depth:
        return "job_queue_depth";
    case Runtime2DCommercialPerformanceMetricKind::cache_misses:
        return "cache_misses";
    case Runtime2DCommercialPerformanceMetricKind::package_miss_pop_in:
        return "package_miss_pop_in";
    }
    return "unknown";
}

std::string_view runtime_2d_commercial_performance_official_source_kind_name(
    Runtime2DCommercialPerformanceOfficialSourceKind kind) noexcept {
    switch (kind) {
    case Runtime2DCommercialPerformanceOfficialSourceKind::microsoft_d3d12:
        return "microsoft_d3d12";
    case Runtime2DCommercialPerformanceOfficialSourceKind::microsoft_wpt:
        return "microsoft_wpt";
    case Runtime2DCommercialPerformanceOfficialSourceKind::microsoft_pix:
        return "microsoft_pix";
    case Runtime2DCommercialPerformanceOfficialSourceKind::khronos_vulkan_timestamp_debug_utils:
        return "khronos_vulkan_timestamp_debug_utils";
    case Runtime2DCommercialPerformanceOfficialSourceKind::apple_xctrace_metal_capture:
        return "apple_xctrace_metal_capture";
    case Runtime2DCommercialPerformanceOfficialSourceKind::linux_perf:
        return "linux_perf";
    case Runtime2DCommercialPerformanceOfficialSourceKind::repository_legal_policy:
        return "repository_legal_policy";
    }
    return "unknown";
}

Runtime2DCommercialPerformanceRegressionGateResult evaluate_runtime_2d_commercial_performance_regression_gate(
    const Runtime2DCommercialPerformanceRegressionGateDesc& desc) {
    Runtime2DCommercialPerformanceRegressionGateResult result;

    evaluate_workloads(desc, result);
    evaluate_metrics(desc, result);
    evaluate_official_sources(desc, result);
    evaluate_claims(desc, result);

    result.ready =
        desc.selected_package_regression_gate_claim && result.workload_gate_ready && result.metric_gate_ready &&
        result.official_source_ready && result.host_threshold_gate_ready && result.over_budget_rows == 0U &&
        result.broad_optimization_claim_rows == 0U && result.cross_vendor_parity_claim_rows == 0U &&
        result.cross_backend_parity_claim_rows == 0U && result.native_handle_access_rows == 0U &&
        result.renderer_rhi_residency_claim_rows == 0U && result.allocator_gpu_budget_enforcement_claim_rows == 0U &&
        result.pgo_lto_default_lane_claim_rows == 0U && result.profiler_artifact_ready_without_artifact_rows == 0U &&
        result.external_engine_claim_rows == 0U && result.legal_approval_claim_rows == 0U;
    return result;
}

} // namespace mirakana::runtime
