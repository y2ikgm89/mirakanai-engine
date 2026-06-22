// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_broad_optimization_evidence.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

namespace mirakana::runtime_rhi {

std::string_view runtime_mavg_broad_optimization_status_label(RuntimeMavgBroadOptimizationStatus status) noexcept {
    switch (status) {
    case RuntimeMavgBroadOptimizationStatus::host_evidence_required:
        return "host_evidence_required";
    case RuntimeMavgBroadOptimizationStatus::blocked:
        return "blocked";
    case RuntimeMavgBroadOptimizationStatus::ready:
        return "ready";
    }
    return "unknown";
}

namespace {

struct RequiredClass {
    RuntimeMavgBroadOptimizationBackend backend{RuntimeMavgBroadOptimizationBackend::d3d12};
    RuntimeMavgBroadOptimizationVendorClass vendor{RuntimeMavgBroadOptimizationVendorClass::nvidia};
    RuntimeMavgBroadOptimizationWorkloadKind workload{RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package};
};

constexpr std::array<RequiredClass, 14> kRequiredRows{{
    {RuntimeMavgBroadOptimizationBackend::d3d12, RuntimeMavgBroadOptimizationVendorClass::nvidia,
     RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package},
    {RuntimeMavgBroadOptimizationBackend::d3d12, RuntimeMavgBroadOptimizationVendorClass::nvidia,
     RuntimeMavgBroadOptimizationWorkloadKind::first_party_gameplay_package},
    {RuntimeMavgBroadOptimizationBackend::d3d12, RuntimeMavgBroadOptimizationVendorClass::amd,
     RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package},
    {RuntimeMavgBroadOptimizationBackend::d3d12, RuntimeMavgBroadOptimizationVendorClass::amd,
     RuntimeMavgBroadOptimizationWorkloadKind::first_party_gameplay_package},
    {RuntimeMavgBroadOptimizationBackend::d3d12, RuntimeMavgBroadOptimizationVendorClass::intel,
     RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package},
    {RuntimeMavgBroadOptimizationBackend::d3d12, RuntimeMavgBroadOptimizationVendorClass::intel,
     RuntimeMavgBroadOptimizationWorkloadKind::first_party_gameplay_package},
    {RuntimeMavgBroadOptimizationBackend::vulkan, RuntimeMavgBroadOptimizationVendorClass::nvidia,
     RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package},
    {RuntimeMavgBroadOptimizationBackend::vulkan, RuntimeMavgBroadOptimizationVendorClass::nvidia,
     RuntimeMavgBroadOptimizationWorkloadKind::first_party_gameplay_package},
    {RuntimeMavgBroadOptimizationBackend::vulkan, RuntimeMavgBroadOptimizationVendorClass::amd,
     RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package},
    {RuntimeMavgBroadOptimizationBackend::vulkan, RuntimeMavgBroadOptimizationVendorClass::amd,
     RuntimeMavgBroadOptimizationWorkloadKind::first_party_gameplay_package},
    {RuntimeMavgBroadOptimizationBackend::vulkan, RuntimeMavgBroadOptimizationVendorClass::intel,
     RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package},
    {RuntimeMavgBroadOptimizationBackend::vulkan, RuntimeMavgBroadOptimizationVendorClass::intel,
     RuntimeMavgBroadOptimizationWorkloadKind::first_party_gameplay_package},
    {RuntimeMavgBroadOptimizationBackend::metal_apple_host, RuntimeMavgBroadOptimizationVendorClass::apple,
     RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package},
    {RuntimeMavgBroadOptimizationBackend::metal_apple_host, RuntimeMavgBroadOptimizationVendorClass::apple,
     RuntimeMavgBroadOptimizationWorkloadKind::first_party_gameplay_package},
}};

void add_diagnostic(RuntimeMavgBroadOptimizationEvidenceResult& result, RuntimeMavgBroadOptimizationDiagnosticCode code,
                    std::string message) {
    result.diagnostics.push_back(RuntimeMavgBroadOptimizationDiagnostic{.code = code, .message = std::move(message)});
}

[[nodiscard]] bool blank(std::string_view value) noexcept {
    return value.empty();
}

[[nodiscard]] bool valid_hash(std::string_view value) noexcept {
    if (value.size() != 64U) {
        return false;
    }
    return std::ranges::all_of(value, [](char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'); });
}

[[nodiscard]] bool positive_metrics(const RuntimeMavgBroadOptimizationMetrics& metrics) noexcept {
    return metrics.cpu_frame_p50_us > 0U && metrics.cpu_frame_p95_us > 0U && metrics.cpu_frame_p99_us > 0U &&
           metrics.cpu_streaming_jobs_p95_us > 0U && metrics.gpu_frame_p50_us > 0U && metrics.gpu_frame_p95_us > 0U &&
           metrics.gpu_frame_p99_us > 0U && metrics.gpu_upload_p95_us > 0U && metrics.gpu_draw_dispatch_p95_us > 0U &&
           metrics.vram_peak_bytes > 0U && metrics.resident_page_bytes > 0U && metrics.io_bytes_per_second > 0U &&
           metrics.io_request_count > 0U && metrics.heap_allocation_count > 0U && !blank(metrics.replay_hash);
}

[[nodiscard]] std::uint32_t improvement_basis_points(std::uint64_t baseline, std::uint64_t optimized) noexcept {
    if (baseline == 0U || optimized >= baseline) {
        return 0U;
    }
    return static_cast<std::uint32_t>(((baseline - optimized) * 10000U) / baseline);
}

[[nodiscard]] std::uint32_t regression_basis_points(std::uint64_t baseline, std::uint64_t candidate) noexcept {
    if (baseline == 0U || candidate <= baseline) {
        return 0U;
    }
    return static_cast<std::uint32_t>(((candidate - baseline) * 10000U) / baseline);
}

[[nodiscard]] bool matches_required(const RuntimeMavgBroadOptimizationEvidenceRow& row,
                                    const RequiredClass& required) noexcept {
    return row.backend == required.backend && row.vendor == required.vendor && row.workload == required.workload;
}

[[nodiscard]] bool has_required_row(std::span<const RuntimeMavgBroadOptimizationEvidenceRow> rows,
                                    const RequiredClass& required) noexcept {
    return std::ranges::any_of(rows, [&required](const RuntimeMavgBroadOptimizationEvidenceRow& row) {
        return matches_required(row, required);
    });
}

[[nodiscard]] bool is_row_blocked(const RuntimeMavgBroadOptimizationEvidenceRow& row) noexcept {
    return row.synthetic_only || row.native_handles_exposed || row.claims_broad_engine_ready ||
           row.claims_broad_backend_readiness;
}

[[nodiscard]] bool is_eol_or_unsupported_profiler_tool(const RuntimeMavgBroadOptimizationEvidenceRow& row) noexcept {
    return !row.profiler_tool_currently_supported ||
           row.profiler_tool == RuntimeMavgBroadOptimizationProfilerTool::intel_gpa;
}

[[nodiscard]] bool
is_profiler_tool_allowed_for_backend_vendor(const RuntimeMavgBroadOptimizationEvidenceRow& row) noexcept {
    switch (row.backend) {
    case RuntimeMavgBroadOptimizationBackend::d3d12:
        switch (row.vendor) {
        case RuntimeMavgBroadOptimizationVendorClass::nvidia:
            return row.profiler_tool == RuntimeMavgBroadOptimizationProfilerTool::pix_timing_capture ||
                   row.profiler_tool == RuntimeMavgBroadOptimizationProfilerTool::nsight_graphics_gpu_trace;
        case RuntimeMavgBroadOptimizationVendorClass::amd:
            return row.profiler_tool == RuntimeMavgBroadOptimizationProfilerTool::pix_timing_capture ||
                   row.profiler_tool == RuntimeMavgBroadOptimizationProfilerTool::radeon_gpu_profiler;
        case RuntimeMavgBroadOptimizationVendorClass::intel:
            return row.profiler_tool == RuntimeMavgBroadOptimizationProfilerTool::pix_timing_capture;
        case RuntimeMavgBroadOptimizationVendorClass::apple:
            return false;
        }
        break;
    case RuntimeMavgBroadOptimizationBackend::vulkan:
        switch (row.vendor) {
        case RuntimeMavgBroadOptimizationVendorClass::nvidia:
            return row.profiler_tool == RuntimeMavgBroadOptimizationProfilerTool::nsight_graphics_gpu_trace;
        case RuntimeMavgBroadOptimizationVendorClass::amd:
            return row.profiler_tool == RuntimeMavgBroadOptimizationProfilerTool::radeon_gpu_profiler;
        case RuntimeMavgBroadOptimizationVendorClass::intel:
        case RuntimeMavgBroadOptimizationVendorClass::apple:
            return false;
        }
        break;
    case RuntimeMavgBroadOptimizationBackend::metal_apple_host:
        return row.vendor == RuntimeMavgBroadOptimizationVendorClass::apple &&
               row.profiler_tool == RuntimeMavgBroadOptimizationProfilerTool::apple_metal_tools;
    }
    return false;
}

} // namespace

RuntimeMavgBroadOptimizationEvidenceResult
evaluate_runtime_mavg_broad_optimization_evidence(const RuntimeMavgBroadOptimizationEvidenceDesc& desc) {
    RuntimeMavgBroadOptimizationEvidenceResult result;
    result.required_evidence_rows = kRequiredRows.size();
    result.retained_evidence_rows = desc.rows.size();

    for (const auto& required : kRequiredRows) {
        if (!has_required_row(desc.rows, required)) {
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::missing_required_vendor_backend_row,
                           "MAVG broad optimization requires every D3D12/Vulkan vendor row and Apple Metal row for "
                           "both MAVG stress and gameplay workloads");
            ++result.host_gated_rows;
        }
    }

    for (const auto& row : desc.rows) {
        bool row_host_gated = false;
        bool row_blocked = false;

        switch (row.backend) {
        case RuntimeMavgBroadOptimizationBackend::d3d12:
            ++result.d3d12_rows;
            break;
        case RuntimeMavgBroadOptimizationBackend::vulkan:
            ++result.vulkan_rows;
            break;
        case RuntimeMavgBroadOptimizationBackend::metal_apple_host:
            ++result.metal_rows;
            break;
        }
        switch (row.vendor) {
        case RuntimeMavgBroadOptimizationVendorClass::nvidia:
            ++result.nvidia_rows;
            break;
        case RuntimeMavgBroadOptimizationVendorClass::amd:
            ++result.amd_rows;
            break;
        case RuntimeMavgBroadOptimizationVendorClass::intel:
            ++result.intel_rows;
            break;
        case RuntimeMavgBroadOptimizationVendorClass::apple:
            ++result.apple_rows;
            break;
        }
        if (row.workload == RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package) {
            ++result.mavg_stress_package_rows;
        } else {
            ++result.gameplay_package_rows;
        }

        if (blank(row.row_id) || blank(row.camera_script_id) || blank(row.host_class) || blank(row.adapter_id) ||
            blank(row.driver_version) || blank(row.profiler_tool_version) ||
            blank(row.profiler_tool_support_source_id) || blank(row.profiler_artifact_id)) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::missing_identity,
                           "MAVG broad optimization rows require stable workload, host, adapter, driver, and profiler "
                           "identity");
        }
        if (!valid_hash(row.package_hash_sha256) || !valid_hash(row.profiler_artifact_hash_sha256)) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::invalid_hash,
                           "MAVG broad optimization package and artifact hashes must be lowercase SHA-256");
        }
        if (!row.internal_engine_counters_ready) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::missing_internal_counters,
                           "MAVG broad optimization requires internal engine CPU/GPU/IO/memory counters");
        } else {
            ++result.internal_counter_rows;
        }
        if (!row.reviewed || !row.official_profiler_tool || !row.external_profiler_artifact_ready) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::missing_official_profiler_artifact,
                           "MAVG broad optimization requires reviewed official profiler artifacts");
        } else {
            ++result.official_profiler_rows;
        }
        if (is_eol_or_unsupported_profiler_tool(row)) {
            row_host_gated = true;
            ++result.eol_or_unsupported_profiler_rows;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::profiler_tool_eol_or_unsupported,
                           "MAVG broad optimization cannot substitute unsupported or end-of-life profiler evidence");
        }
        if (!is_profiler_tool_allowed_for_backend_vendor(row)) {
            row_host_gated = true;
            ++result.profiler_backend_vendor_mismatch_rows;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::profiler_tool_backend_vendor_mismatch,
                           "MAVG broad optimization profiler evidence must match the backend, vendor, and current "
                           "official tool support row");
        }
        if (!row.cpu_timeline_available || !row.gpu_timeline_available || !row.io_timeline_available ||
            !row.memory_timeline_available || !row.queue_timeline_available) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::profiler_missing_required_timelines,
                           "MAVG broad optimization requires CPU, GPU, IO, memory, and queue timelines");
        }
        if (!row.no_profiler_timestamp_overflow || !row.representative_capture ||
            row.profiler_capture_overhead_basis_points > desc.maximum_profiler_capture_overhead_basis_points) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::profiler_capture_not_representative,
                           "MAVG broad optimization profiler captures must be representative, overflow-free, and "
                           "below the selected capture-overhead threshold");
        }
        if (!positive_metrics(row.baseline) || !positive_metrics(row.optimized) || !row.first_party_package) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::missing_required_metrics,
                           "MAVG broad optimization requires complete CPU/GPU/memory/IO metrics for first-party "
                           "packages");
        }
        if (row.baseline.replay_hash != row.optimized.replay_hash || blank(row.baseline.replay_hash)) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::replay_hash_mismatch,
                           "MAVG broad optimization baseline and optimized replay hashes must match");
        }

        if (improvement_basis_points(row.baseline.cpu_frame_p95_us, row.optimized.cpu_frame_p95_us) <
            desc.minimum_cpu_p95_improvement_basis_points) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::insufficient_cpu_improvement,
                           "MAVG broad optimization requires CPU p95 improvement on every retained row");
        }
        if (improvement_basis_points(row.baseline.gpu_frame_p95_us, row.optimized.gpu_frame_p95_us) <
            desc.minimum_gpu_p95_improvement_basis_points) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::insufficient_gpu_improvement,
                           "MAVG broad optimization requires GPU p95 improvement on every retained row");
        }
        if (improvement_basis_points(row.baseline.vram_peak_bytes, row.optimized.vram_peak_bytes) <
            desc.minimum_vram_peak_improvement_basis_points) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::insufficient_memory_improvement,
                           "MAVG broad optimization requires VRAM peak improvement on every retained row");
        }
        if (regression_basis_points(row.baseline.cpu_frame_p99_us, row.optimized.cpu_frame_p99_us) >
                desc.maximum_p99_regression_basis_points ||
            regression_basis_points(row.baseline.gpu_frame_p99_us, row.optimized.gpu_frame_p99_us) >
                desc.maximum_p99_regression_basis_points) {
            row_host_gated = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::excessive_p99_regression,
                           "MAVG broad optimization p99 CPU/GPU regression exceeds the selected threshold");
        }
        if (is_row_blocked(row)) {
            row_blocked = true;
            add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::synthetic_only_evidence,
                           "MAVG broad optimization rejects synthetic-only, native-handle, broad engine, and broad "
                           "backend readiness claims");
            if (row.native_handles_exposed) {
                add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::native_handle_access,
                               "MAVG broad optimization must not expose native handles");
            }
            if (row.claims_broad_engine_ready) {
                add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::broad_engine_claim_not_allowed,
                               "MAVG broad optimization must not promote broad engine readiness");
            }
            if (row.claims_broad_backend_readiness) {
                add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::broad_backend_readiness_not_allowed,
                               "MAVG broad optimization must not promote broad MAVG backend readiness");
            }
        }

        if (row_blocked) {
            ++result.blocked_rows;
        } else if (row_host_gated) {
            ++result.host_gated_rows;
        } else {
            ++result.ready_evidence_rows;
        }
    }

    if (result.d3d12_rows == 0U || result.vulkan_rows == 0U || result.metal_rows == 0U || result.nvidia_rows == 0U ||
        result.amd_rows == 0U || result.intel_rows == 0U || result.apple_rows == 0U) {
        add_diagnostic(result, RuntimeMavgBroadOptimizationDiagnosticCode::insufficient_vendor_backend_coverage,
                       "MAVG broad optimization requires D3D12, Vulkan, Metal, NVIDIA, AMD, Intel, and Apple rows");
    }

    if (result.blocked_rows > 0U) {
        result.status = RuntimeMavgBroadOptimizationStatus::blocked;
        return result;
    }
    if (!result.diagnostics.empty() || result.ready_evidence_rows != result.required_evidence_rows) {
        result.status = RuntimeMavgBroadOptimizationStatus::host_evidence_required;
        return result;
    }

    result.status = RuntimeMavgBroadOptimizationStatus::ready;
    result.mavg_broad_cpu_gpu_memory_optimization_ready = true;
    return result;
}

bool has_runtime_mavg_broad_optimization_diagnostic(const RuntimeMavgBroadOptimizationEvidenceResult& result,
                                                    RuntimeMavgBroadOptimizationDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const RuntimeMavgBroadOptimizationDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

} // namespace mirakana::runtime_rhi
