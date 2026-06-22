// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_broad_optimization_evidence.hpp"

#include <string_view>
#include <vector>

namespace {

using mirakana::runtime_rhi::RuntimeMavgBroadOptimizationBackend;
using mirakana::runtime_rhi::RuntimeMavgBroadOptimizationDiagnosticCode;
using mirakana::runtime_rhi::RuntimeMavgBroadOptimizationEvidenceRow;
using mirakana::runtime_rhi::RuntimeMavgBroadOptimizationMetrics;
using mirakana::runtime_rhi::RuntimeMavgBroadOptimizationProfilerTool;
using mirakana::runtime_rhi::RuntimeMavgBroadOptimizationStatus;
using mirakana::runtime_rhi::RuntimeMavgBroadOptimizationVendorClass;
using mirakana::runtime_rhi::RuntimeMavgBroadOptimizationWorkloadKind;

constexpr std::string_view kHash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

[[nodiscard]] RuntimeMavgBroadOptimizationProfilerTool
tool_for(RuntimeMavgBroadOptimizationBackend backend, RuntimeMavgBroadOptimizationVendorClass vendor) noexcept {
    if (backend == RuntimeMavgBroadOptimizationBackend::metal_apple_host) {
        return RuntimeMavgBroadOptimizationProfilerTool::apple_metal_tools;
    }
    if (vendor == RuntimeMavgBroadOptimizationVendorClass::nvidia) {
        return RuntimeMavgBroadOptimizationProfilerTool::nsight_graphics_gpu_trace;
    }
    if (vendor == RuntimeMavgBroadOptimizationVendorClass::amd) {
        return RuntimeMavgBroadOptimizationProfilerTool::radeon_gpu_profiler;
    }
    if (backend == RuntimeMavgBroadOptimizationBackend::vulkan) {
        return RuntimeMavgBroadOptimizationProfilerTool::intel_gpa;
    }
    return RuntimeMavgBroadOptimizationProfilerTool::pix_timing_capture;
}

[[nodiscard]] RuntimeMavgBroadOptimizationMetrics metrics(std::uint64_t cpu_p95, std::uint64_t gpu_p95,
                                                          std::uint64_t vram_peak) {
    return RuntimeMavgBroadOptimizationMetrics{
        .cpu_frame_p50_us = cpu_p95 - 2'000U,
        .cpu_frame_p95_us = cpu_p95,
        .cpu_frame_p99_us = cpu_p95 + 2'000U,
        .cpu_streaming_jobs_p95_us = cpu_p95 / 4U,
        .gpu_frame_p50_us = gpu_p95 - 2'000U,
        .gpu_frame_p95_us = gpu_p95,
        .gpu_frame_p99_us = gpu_p95 + 2'000U,
        .gpu_upload_p95_us = gpu_p95 / 5U,
        .gpu_draw_dispatch_p95_us = gpu_p95 / 2U,
        .vram_peak_bytes = vram_peak,
        .resident_page_bytes = vram_peak / 2U,
        .page_fault_count = 1,
        .io_bytes_per_second = 700'000'000,
        .io_request_count = 100,
        .heap_allocation_count = 16,
        .replay_hash = "replay-hash",
    };
}

[[nodiscard]] RuntimeMavgBroadOptimizationEvidenceRow make_row(RuntimeMavgBroadOptimizationBackend backend,
                                                               RuntimeMavgBroadOptimizationVendorClass vendor,
                                                               RuntimeMavgBroadOptimizationWorkloadKind workload) {
    return RuntimeMavgBroadOptimizationEvidenceRow{
        .backend = backend,
        .vendor = vendor,
        .workload = workload,
        .profiler_tool = tool_for(backend, vendor),
        .row_id = "mavg.broad.optimization.row",
        .package_hash_sha256 = kHash,
        .camera_script_id = "mavg-broad-camera-a",
        .host_class = "official-host-class",
        .adapter_id = "adapter",
        .driver_version = "driver",
        .profiler_tool_version = "tool-version",
        .profiler_tool_support_source_id = "official-profiler-support-source",
        .profiler_artifact_id = "artifact",
        .profiler_artifact_hash_sha256 = kHash,
        .baseline = metrics(20'000, 19'000, 900'000'000),
        .optimized = metrics(19'000, 18'000, 850'000'000),
        .profiler_capture_overhead_basis_points = 50,
        .reviewed = true,
        .official_profiler_tool = true,
        .profiler_tool_currently_supported = true,
        .internal_engine_counters_ready = true,
        .external_profiler_artifact_ready = true,
        .first_party_package = true,
        .cpu_timeline_available = true,
        .gpu_timeline_available = true,
        .io_timeline_available = true,
        .memory_timeline_available = true,
        .queue_timeline_available = true,
        .no_profiler_timestamp_overflow = true,
        .representative_capture = true,
    };
}

[[nodiscard]] std::vector<RuntimeMavgBroadOptimizationEvidenceRow> make_complete_rows() {
    std::vector<RuntimeMavgBroadOptimizationEvidenceRow> rows;
    for (const auto backend :
         {RuntimeMavgBroadOptimizationBackend::d3d12, RuntimeMavgBroadOptimizationBackend::vulkan}) {
        for (const auto vendor :
             {RuntimeMavgBroadOptimizationVendorClass::nvidia, RuntimeMavgBroadOptimizationVendorClass::amd,
              RuntimeMavgBroadOptimizationVendorClass::intel}) {
            rows.push_back(make_row(backend, vendor, RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package));
            rows.push_back(
                make_row(backend, vendor, RuntimeMavgBroadOptimizationWorkloadKind::first_party_gameplay_package));
        }
    }
    rows.push_back(make_row(RuntimeMavgBroadOptimizationBackend::metal_apple_host,
                            RuntimeMavgBroadOptimizationVendorClass::apple,
                            RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package));
    rows.push_back(make_row(RuntimeMavgBroadOptimizationBackend::metal_apple_host,
                            RuntimeMavgBroadOptimizationVendorClass::apple,
                            RuntimeMavgBroadOptimizationWorkloadKind::first_party_gameplay_package));
    return rows;
}

[[nodiscard]] bool has_code(const mirakana::runtime_rhi::RuntimeMavgBroadOptimizationEvidenceResult& result,
                            RuntimeMavgBroadOptimizationDiagnosticCode code) noexcept {
    return mirakana::runtime_rhi::has_runtime_mavg_broad_optimization_diagnostic(result, code);
}

} // namespace

MK_TEST("runtime rhi mavg broad optimization evidence host gates complete matrix with eol intel vulkan profiler") {
    const auto rows = make_complete_rows();
    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_broad_optimization_evidence({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgBroadOptimizationStatus::host_evidence_required);
    MK_REQUIRE(result.required_evidence_rows == 14U);
    MK_REQUIRE(result.ready_evidence_rows == 12U);
    MK_REQUIRE(result.eol_or_unsupported_profiler_rows == 2U);
    MK_REQUIRE(result.profiler_backend_vendor_mismatch_rows == 2U);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
    MK_REQUIRE(!result.mavg_metal_mesh_lod_ready);
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::profiler_tool_eol_or_unsupported));
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::profiler_tool_backend_vendor_mismatch));
}

MK_TEST("runtime rhi mavg broad optimization evidence host gates missing required rows") {
    const std::vector<RuntimeMavgBroadOptimizationEvidenceRow> rows{
        make_row(RuntimeMavgBroadOptimizationBackend::d3d12, RuntimeMavgBroadOptimizationVendorClass::nvidia,
                 RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package),
    };

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_broad_optimization_evidence({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgBroadOptimizationStatus::host_evidence_required);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::missing_required_vendor_backend_row));
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::insufficient_vendor_backend_coverage));
}

MK_TEST("runtime rhi mavg broad optimization evidence rejects unsupported profiler rows") {
    auto rows = make_complete_rows();
    rows[0].profiler_tool_currently_supported = false;
    rows[0].io_timeline_available = false;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_broad_optimization_evidence({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgBroadOptimizationStatus::host_evidence_required);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::profiler_tool_eol_or_unsupported));
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::profiler_missing_required_timelines));
}

MK_TEST("runtime rhi mavg broad optimization evidence rejects profiler backend vendor mismatch") {
    auto rows = make_complete_rows();
    rows[0].profiler_tool = RuntimeMavgBroadOptimizationProfilerTool::apple_metal_tools;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_broad_optimization_evidence({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgBroadOptimizationStatus::host_evidence_required);
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::profiler_tool_backend_vendor_mismatch));
}

MK_TEST("runtime rhi mavg broad optimization evidence enforces cpu gpu memory metrics") {
    auto rows = make_complete_rows();
    rows[0].optimized.cpu_frame_p95_us = rows[0].baseline.cpu_frame_p95_us;
    rows[0].optimized.gpu_frame_p95_us = rows[0].baseline.gpu_frame_p95_us;
    rows[0].optimized.vram_peak_bytes = rows[0].baseline.vram_peak_bytes;
    rows[0].optimized.cpu_frame_p99_us = rows[0].baseline.cpu_frame_p99_us + 1'000U;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_broad_optimization_evidence({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgBroadOptimizationStatus::host_evidence_required);
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::insufficient_cpu_improvement));
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::insufficient_gpu_improvement));
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::insufficient_memory_improvement));
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::excessive_p99_regression));
}

MK_TEST("runtime rhi mavg broad optimization evidence blocks synthetic native and broad claims") {
    auto rows = make_complete_rows();
    rows[0].synthetic_only = true;
    rows[0].native_handles_exposed = true;
    rows[0].claims_broad_engine_ready = true;
    rows[0].claims_broad_backend_readiness = true;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_broad_optimization_evidence({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgBroadOptimizationStatus::blocked);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::synthetic_only_evidence));
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::native_handle_access));
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::broad_engine_claim_not_allowed));
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::broad_backend_readiness_not_allowed));
}

MK_TEST("runtime rhi mavg broad optimization evidence requires matching replay hashes") {
    auto rows = make_complete_rows();
    rows[0].optimized.replay_hash = "different-replay";

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_broad_optimization_evidence({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgBroadOptimizationStatus::host_evidence_required);
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::replay_hash_mismatch));
}

MK_TEST("runtime rhi mavg broad optimization evidence records profiler support source and overhead") {
    auto rows = make_complete_rows();
    rows[0].profiler_tool_support_source_id = "";
    rows[1].profiler_capture_overhead_basis_points = 600;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_broad_optimization_evidence({.rows = rows});

    MK_REQUIRE(result.status == RuntimeMavgBroadOptimizationStatus::host_evidence_required);
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::missing_identity));
    MK_REQUIRE(has_code(result, RuntimeMavgBroadOptimizationDiagnosticCode::profiler_capture_not_representative));
}

int main() {
    return mirakana::test::run_all();
}
