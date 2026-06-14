// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/environment_performance.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr std::array kRequiredWorkloads{
    EnvironmentOptimizationWorkload::preset_pack_flythrough,
    EnvironmentOptimizationWorkload::storm_precipitation,
    EnvironmentOptimizationWorkload::dense_volumetric_fog,
    EnvironmentOptimizationWorkload::volumetric_cloud_sunset,
    EnvironmentOptimizationWorkload::snowfield_material_weathering,
    EnvironmentOptimizationWorkload::weather_simulation_stress,
    EnvironmentOptimizationWorkload::asset_library_cold_load,
};

[[nodiscard]] std::uint8_t workload_sort_key(EnvironmentOptimizationWorkload workload) noexcept {
    return static_cast<std::uint8_t>(workload);
}

[[nodiscard]] std::string_view canonical_workload_id(EnvironmentOptimizationWorkload workload) noexcept {
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
    return "";
}

[[nodiscard]] bool is_supported_workload(EnvironmentOptimizationWorkload workload) noexcept {
    return !canonical_workload_id(workload).empty();
}

[[nodiscard]] bool is_id_char(char ch) noexcept {
    return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '.' || ch == '-';
}

[[nodiscard]] bool is_valid_id(std::string_view value) {
    return !value.empty() && std::ranges::all_of(value, is_id_char);
}

[[nodiscard]] bool metric_values_ready(const EnvironmentOptimizationMetricSet& metrics) noexcept {
    return metrics.cpu_frame_p95_us > 0U && metrics.gpu_frame_p95_us > 0U && metrics.memory_peak_bytes > 0U &&
           metrics.transient_gpu_bytes > 0U && metrics.upload_bytes > 0U && metrics.draw_count > 0U &&
           metrics.barrier_count > 0U && metrics.texture_residency_bytes > 0U && metrics.package_load_us > 0U;
}

[[nodiscard]] bool budget_values_ready(const EnvironmentOptimizationRegressionBudget& budget) noexcept {
    return budget.max_cpu_frame_p95_us > 0U && budget.max_gpu_frame_p95_us > 0U && budget.max_memory_peak_bytes > 0U &&
           budget.max_transient_gpu_bytes > 0U && budget.max_upload_bytes > 0U && budget.max_draw_count > 0U &&
           budget.max_barrier_count > 0U && budget.max_texture_residency_bytes > 0U && budget.max_package_load_us > 0U;
}

[[nodiscard]] bool over_budget(const EnvironmentOptimizationMeasurementRow& row) noexcept {
    return row.after.cpu_frame_p95_us > row.budget.max_cpu_frame_p95_us ||
           row.after.gpu_frame_p95_us > row.budget.max_gpu_frame_p95_us ||
           row.after.memory_peak_bytes > row.budget.max_memory_peak_bytes ||
           row.after.transient_gpu_bytes > row.budget.max_transient_gpu_bytes ||
           row.after.upload_bytes > row.budget.max_upload_bytes || row.after.draw_count > row.budget.max_draw_count ||
           row.after.dispatch_count > row.budget.max_dispatch_count ||
           row.after.barrier_count > row.budget.max_barrier_count ||
           row.after.texture_residency_bytes > row.budget.max_texture_residency_bytes ||
           row.after.package_load_us > row.budget.max_package_load_us ||
           row.after.stutter_frames > row.budget.max_stutter_frames;
}

[[nodiscard]] bool ready_row(const EnvironmentOptimizationMeasurementRow& row) noexcept {
    return row.status == EnvironmentOptimizationRowStatus::ready && row.backend == rhi::BackendKind::d3d12 &&
           row.before_after_ready && row.host_tool_versions_ready && row.profiler_artifact_ready &&
           row.repository_counters_ready && row.timestamp_query_evidence_ready && row.regression_budget_ready &&
           row.diagnostic_count == 0U && !row.broad_optimization_claimed && !row.native_handle_access &&
           !row.inferred_from_other_backend && metric_values_ready(row.before) && metric_values_ready(row.after) &&
           row.before.finite_samples && row.after.finite_samples && budget_values_ready(row.budget) &&
           !over_budget(row) && !row.host_os.empty() && !row.cpu_name.empty() && !row.gpu_name.empty() &&
           !row.gpu_driver_version.empty() && !row.profiler_tool.empty() && !row.profiler_tool_version.empty() &&
           !row.profiler_artifact_id.empty() && !row.resolution.empty() && row.sample_frames > 0U;
}

void add_diagnostic(EnvironmentOptimizationMeasurementPlan& plan, EnvironmentOptimizationDiagnosticCode code,
                    EnvironmentOptimizationWorkload workload, std::string row_id, std::string message,
                    std::uint32_t source_index) {
    plan.diagnostics.push_back(EnvironmentOptimizationDiagnostic{
        .code = code,
        .workload = workload,
        .row_id = std::move(row_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_rows(std::vector<EnvironmentOptimizationMeasurementRow>& rows) {
    std::ranges::sort(rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.workload != rhs.workload) {
            return workload_sort_key(lhs.workload) < workload_sort_key(rhs.workload);
        }
        if (lhs.workload_id != rhs.workload_id) {
            return lhs.workload_id < rhs.workload_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_diagnostics(EnvironmentOptimizationMeasurementPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.workload != rhs.workload) {
            return workload_sort_key(lhs.workload) < workload_sort_key(rhs.workload);
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void validate_budget(EnvironmentOptimizationMeasurementPlan& plan,
                     const EnvironmentOptimizationMeasurementRequest& request) {
    if (request.rows.size() > request.row_budget) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::row_budget_exceeded,
                       EnvironmentOptimizationWorkload::preset_pack_flythrough, {},
                       "environment optimization measurement row budget exceeded", 0U);
    }
}

void validate_duplicate_rows(EnvironmentOptimizationMeasurementPlan& plan,
                             const EnvironmentOptimizationMeasurementRequest& request) {
    std::vector<EnvironmentOptimizationWorkload> seen;
    seen.reserve(request.rows.size());
    for (const auto& row : request.rows) {
        if (std::ranges::find(seen, row.workload) != seen.end()) {
            add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::duplicate_workload_row, row.workload,
                           row.workload_id, "environment optimization measurement allows one row per workload",
                           row.source_index);
            continue;
        }
        seen.push_back(row.workload);
    }
}

void validate_required_rows(EnvironmentOptimizationMeasurementPlan& plan,
                            const EnvironmentOptimizationMeasurementRequest& request) {
    if (!request.environment_backend_parity_ready && request.rows.size() < request.required_workload_count) {
        return;
    }
    for (const auto workload : kRequiredWorkloads) {
        const auto has_row =
            std::ranges::any_of(request.rows, [workload](const auto& row) { return row.workload == workload; });
        if (!has_row) {
            add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::missing_required_workload, workload, {},
                           "broad environment optimization requires every normalized workload row", 0U);
        }
    }
}

void validate_row_taxonomy(EnvironmentOptimizationMeasurementPlan& plan,
                           const EnvironmentOptimizationMeasurementRow& row) {
    bool valid{false};
    switch (row.status) {
    case EnvironmentOptimizationRowStatus::ready:
        valid = true;
        break;
    case EnvironmentOptimizationRowStatus::host_gated:
        valid = !row.before_after_ready && !row.profiler_artifact_ready;
        break;
    case EnvironmentOptimizationRowStatus::unsupported:
        valid = !row.before_after_ready && !row.regression_budget_ready;
        break;
    }
    if (!valid) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::invalid_row_taxonomy, row.workload, row.workload_id,
                       "environment optimization rows need explicit ready, host-gated, or unsupported state",
                       row.source_index);
    }
}

void validate_row_identity(EnvironmentOptimizationMeasurementPlan& plan,
                           const EnvironmentOptimizationMeasurementRow& row) {
    if (!is_supported_workload(row.workload) || !is_valid_id(row.workload_id)) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::invalid_workload_id, row.workload, row.workload_id,
                       "environment optimization rows require a normalized workload id", row.source_index);
    } else if (row.workload_id != canonical_workload_id(row.workload)) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::invalid_workload_id, row.workload, row.workload_id,
                       "environment optimization evidence cannot be transferred between workloads", row.source_index);
    }
    if (row.backend != rhi::BackendKind::d3d12) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::invalid_backend, row.workload, row.workload_id,
                       "phase 9 slice 1 accepts selected D3D12 primary measurement rows only", row.source_index);
    }
}

void validate_freshness(EnvironmentOptimizationMeasurementPlan& plan,
                        const EnvironmentOptimizationMeasurementRequest& request,
                        const EnvironmentOptimizationMeasurementRow& row) {
    if (row.package_revision != request.expected_package_revision) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::stale_package_revision, row.workload,
                       row.workload_id, "environment optimization measurement requires the selected package revision",
                       row.source_index);
    }
    if (row.quality_tier != request.expected_quality_tier) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::quality_tier_mismatch, row.workload,
                       row.workload_id, "environment optimization measurement requires the selected quality tier",
                       row.source_index);
    }
}

void validate_measurement_evidence(EnvironmentOptimizationMeasurementPlan& plan,
                                   const EnvironmentOptimizationMeasurementRow& row) {
    if (row.status != EnvironmentOptimizationRowStatus::ready) {
        return;
    }
    if (row.host_os.empty() || row.cpu_name.empty() || row.gpu_name.empty() || row.gpu_driver_version.empty() ||
        row.profiler_tool.empty() || row.profiler_tool_version.empty()) {
        add_diagnostic(
            plan, EnvironmentOptimizationDiagnosticCode::missing_host_identity, row.workload, row.workload_id,
            "environment optimization evidence requires host CPU GPU driver and tool versions", row.source_index);
    }
    if (row.resolution.empty() || row.sample_frames == 0U) {
        add_diagnostic(
            plan, EnvironmentOptimizationDiagnosticCode::missing_measurement_window, row.workload, row.workload_id,
            "environment optimization evidence requires resolution warmup and sample frame context", row.source_index);
    }
    if (!row.before_after_ready || !metric_values_ready(row.before) || !metric_values_ready(row.after)) {
        add_diagnostic(
            plan, EnvironmentOptimizationDiagnosticCode::missing_before_after_metrics, row.workload, row.workload_id,
            "environment optimization evidence requires complete before and after metric rows", row.source_index);
    }
    if (!row.before.finite_samples || !row.after.finite_samples) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::non_finite_sample, row.workload, row.workload_id,
                       "environment optimization evidence requires finite measurement samples", row.source_index);
    }
    if (!row.host_tool_versions_ready || !row.repository_counters_ready || !row.timestamp_query_evidence_ready) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::missing_tool_evidence, row.workload,
                       row.workload_id,
                       "environment optimization evidence requires repository counters and official timestamp tooling",
                       row.source_index);
    }
    if (!row.profiler_artifact_ready || row.profiler_artifact_id.empty()) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::missing_profiler_artifact, row.workload,
                       row.workload_id, "environment optimization evidence requires a retained profiler artifact id",
                       row.source_index);
    }
    if (!row.regression_budget_ready || !budget_values_ready(row.budget)) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::missing_regression_budget, row.workload,
                       row.workload_id, "environment optimization evidence requires explicit regression budgets",
                       row.source_index);
    } else if (over_budget(row)) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::regression_budget_exceeded, row.workload,
                       row.workload_id, "environment optimization after metrics exceed a regression budget",
                       row.source_index);
    }
    if (row.diagnostic_count != 0U) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::diagnostics_nonzero, row.workload, row.workload_id,
                       "environment optimization ready rows require zero diagnostics", row.source_index);
    }
    if (row.broad_optimization_claimed) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::unsupported_broad_optimization_claim, row.workload,
                       row.workload_id, "single workload measurement must not claim broad optimization",
                       row.source_index);
    }
    if (row.native_handle_access) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::unsupported_native_handle_access, row.workload,
                       row.workload_id,
                       "environment optimization measurement must not expose native renderer or RHI handles",
                       row.source_index);
    }
    if (row.inferred_from_other_backend) {
        add_diagnostic(plan, EnvironmentOptimizationDiagnosticCode::unsupported_inferred_backend, row.workload,
                       row.workload_id, "environment optimization measurement cannot infer evidence from a backend",
                       row.source_index);
    }
}

void validate_rows(EnvironmentOptimizationMeasurementPlan& plan,
                   const EnvironmentOptimizationMeasurementRequest& request) {
    for (const auto& row : request.rows) {
        validate_row_taxonomy(plan, row);
        validate_row_identity(plan, row);
        validate_freshness(plan, request, row);
        validate_measurement_evidence(plan, row);
    }
}

[[nodiscard]] bool has_ready_workload(const EnvironmentOptimizationMeasurementPlan& plan,
                                      EnvironmentOptimizationWorkload workload) {
    const auto row_it =
        std::ranges::find_if(plan.rows, [workload](const auto& row) { return row.workload == workload; });
    return row_it != plan.rows.end() && ready_row(*row_it);
}

[[nodiscard]] bool has_ready_backend_workload(const EnvironmentOptimizationMeasurementPlan& plan,
                                              EnvironmentOptimizationWorkload workload, rhi::BackendKind backend) {
    const auto row_it = std::ranges::find_if(
        plan.rows, [workload, backend](const auto& row) { return row.workload == workload && row.backend == backend; });
    return row_it != plan.rows.end() && ready_row(*row_it);
}

void summarize(EnvironmentOptimizationMeasurementPlan& plan) {
    for (const auto& row : plan.rows) {
        if (row.before_after_ready && metric_values_ready(row.before) && metric_values_ready(row.after)) {
            ++plan.before_after_pair_count;
        }
        if (row.regression_budget_ready && budget_values_ready(row.budget)) {
            ++plan.regression_budget_row_count;
        }
        if (row.regression_budget_ready && budget_values_ready(row.budget) && over_budget(row)) {
            ++plan.over_budget_row_count;
        }
        if (ready_row(row)) {
            ++plan.measured_workload_count;
        }
    }
    plan.d3d12_preset_pack_flythrough_measured = has_ready_backend_workload(
        plan, EnvironmentOptimizationWorkload::preset_pack_flythrough, rhi::BackendKind::d3d12);
    plan.d3d12_storm_precipitation_measured =
        has_ready_backend_workload(plan, EnvironmentOptimizationWorkload::storm_precipitation, rhi::BackendKind::d3d12);
    plan.d3d12_dense_volumetric_fog_measured = has_ready_backend_workload(
        plan, EnvironmentOptimizationWorkload::dense_volumetric_fog, rhi::BackendKind::d3d12);
    const auto every_required_workload_ready = std::ranges::all_of(
        kRequiredWorkloads, [&plan](const auto workload) { return has_ready_workload(plan, workload); });
    plan.environment_broad_optimization_ready = every_required_workload_ready && plan.environment_backend_parity_ready;
}

void hash_mix(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value;
    hash *= 1099511628211ULL;
}

void hash_string(std::uint64_t& hash, std::string_view value) noexcept {
    for (const auto ch : value) {
        hash_mix(hash, static_cast<unsigned char>(ch));
    }
    hash_mix(hash, 0xffU);
}

void hash_metrics(std::uint64_t& hash, const EnvironmentOptimizationMetricSet& metrics) noexcept {
    hash_mix(hash, metrics.cpu_frame_p95_us);
    hash_mix(hash, metrics.gpu_frame_p95_us);
    hash_mix(hash, metrics.memory_peak_bytes);
    hash_mix(hash, metrics.transient_gpu_bytes);
    hash_mix(hash, metrics.upload_bytes);
    hash_mix(hash, metrics.draw_count);
    hash_mix(hash, metrics.dispatch_count);
    hash_mix(hash, metrics.barrier_count);
    hash_mix(hash, metrics.texture_residency_bytes);
    hash_mix(hash, metrics.package_load_us);
    hash_mix(hash, metrics.stutter_frames);
    hash_mix(hash, metrics.finite_samples ? 1U : 0U);
}

void hash_budget(std::uint64_t& hash, const EnvironmentOptimizationRegressionBudget& budget) noexcept {
    hash_mix(hash, budget.max_cpu_frame_p95_us);
    hash_mix(hash, budget.max_gpu_frame_p95_us);
    hash_mix(hash, budget.max_memory_peak_bytes);
    hash_mix(hash, budget.max_transient_gpu_bytes);
    hash_mix(hash, budget.max_upload_bytes);
    hash_mix(hash, budget.max_draw_count);
    hash_mix(hash, budget.max_dispatch_count);
    hash_mix(hash, budget.max_barrier_count);
    hash_mix(hash, budget.max_texture_residency_bytes);
    hash_mix(hash, budget.max_package_load_us);
    hash_mix(hash, budget.max_stutter_frames);
}

[[nodiscard]] std::uint64_t compute_replay_hash(const EnvironmentOptimizationMeasurementPlan& plan,
                                                const EnvironmentOptimizationMeasurementRequest& request) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, request.seed);
    hash_mix(hash, request.environment_backend_parity_ready ? 1U : 0U);
    hash_mix(hash, request.required_workload_count);
    for (const auto& row : plan.rows) {
        hash_string(hash, row.workload_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.workload));
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, static_cast<std::uint8_t>(row.status));
        hash_string(hash, row.host_os);
        hash_string(hash, row.cpu_name);
        hash_string(hash, row.gpu_name);
        hash_string(hash, row.gpu_driver_version);
        hash_string(hash, row.profiler_tool);
        hash_string(hash, row.profiler_tool_version);
        hash_string(hash, row.profiler_artifact_id);
        hash_string(hash, row.package_revision);
        hash_string(hash, row.quality_tier);
        hash_string(hash, row.resolution);
        hash_mix(hash, row.warmup_frames);
        hash_mix(hash, row.sample_frames);
        hash_metrics(hash, row.before);
        hash_metrics(hash, row.after);
        hash_budget(hash, row.budget);
        hash_mix(hash, row.before_after_ready ? 1U : 0U);
        hash_mix(hash, row.host_tool_versions_ready ? 1U : 0U);
        hash_mix(hash, row.profiler_artifact_ready ? 1U : 0U);
        hash_mix(hash, row.repository_counters_ready ? 1U : 0U);
        hash_mix(hash, row.timestamp_query_evidence_ready ? 1U : 0U);
        hash_mix(hash, row.regression_budget_ready ? 1U : 0U);
        hash_mix(hash, row.diagnostic_count);
        hash_mix(hash, row.broad_optimization_claimed ? 1U : 0U);
        hash_mix(hash, row.native_handle_access ? 1U : 0U);
        hash_mix(hash, row.inferred_from_other_backend ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    return hash == 0U ? 1U : hash;
}

} // namespace

bool EnvironmentOptimizationMeasurementPlan::succeeded() const noexcept {
    return status == EnvironmentOptimizationMeasurementStatus::ready && diagnostics.empty() &&
           environment_broad_optimization_ready;
}

EnvironmentOptimizationMeasurementPlan
plan_environment_optimization_measurement(const EnvironmentOptimizationMeasurementRequest& request) {
    EnvironmentOptimizationMeasurementPlan plan;
    plan.rows = request.rows;
    plan.row_count = request.rows.size();
    plan.required_workload_count = request.required_workload_count;
    plan.environment_backend_parity_ready = request.environment_backend_parity_ready;
    sort_rows(plan.rows);

    if (request.rows.empty()) {
        plan.status = EnvironmentOptimizationMeasurementStatus::no_rows;
        return plan;
    }

    validate_budget(plan, request);
    validate_duplicate_rows(plan, request);
    validate_required_rows(plan, request);
    validate_rows(plan, request);
    summarize(plan);
    sort_diagnostics(plan);

    if (!plan.diagnostics.empty()) {
        plan.status = EnvironmentOptimizationMeasurementStatus::invalid_request;
        return plan;
    }

    plan.replay_hash = compute_replay_hash(plan, request);
    if (plan.environment_broad_optimization_ready) {
        plan.status = EnvironmentOptimizationMeasurementStatus::ready;
    } else {
        plan.status = EnvironmentOptimizationMeasurementStatus::host_evidence_required;
    }
    return plan;
}

} // namespace mirakana
