// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgBroadOptimizationBackend : std::uint8_t {
    d3d12,
    vulkan,
    metal_apple_host,
};

enum class RuntimeMavgBroadOptimizationVendorClass : std::uint8_t {
    nvidia,
    amd,
    intel,
    apple,
};

enum class RuntimeMavgBroadOptimizationWorkloadKind : std::uint8_t {
    mavg_stress_package,
    first_party_gameplay_package,
};

enum class RuntimeMavgBroadOptimizationProfilerTool : std::uint8_t {
    pix_timing_capture,
    nsight_graphics_gpu_trace,
    radeon_gpu_profiler,
    intel_gpa,
    apple_metal_tools,
};

enum class RuntimeMavgBroadOptimizationStatus : std::uint8_t {
    host_evidence_required,
    blocked,
    ready,
};

enum class RuntimeMavgBroadOptimizationDiagnosticCode : std::uint8_t {
    none,
    missing_required_vendor_backend_row,
    missing_identity,
    invalid_hash,
    missing_internal_counters,
    missing_official_profiler_artifact,
    profiler_tool_eol_or_unsupported,
    profiler_tool_backend_vendor_mismatch,
    profiler_missing_required_timelines,
    profiler_capture_not_representative,
    missing_required_metrics,
    replay_hash_mismatch,
    insufficient_cpu_improvement,
    insufficient_gpu_improvement,
    insufficient_memory_improvement,
    excessive_p99_regression,
    synthetic_only_evidence,
    native_handle_access,
    broad_engine_claim_not_allowed,
    broad_backend_readiness_not_allowed,
    insufficient_vendor_backend_coverage,
};

struct RuntimeMavgBroadOptimizationDiagnostic {
    RuntimeMavgBroadOptimizationDiagnosticCode code{RuntimeMavgBroadOptimizationDiagnosticCode::none};
    std::string message;
};

struct RuntimeMavgBroadOptimizationMetrics {
    std::uint64_t cpu_frame_p50_us{0};
    std::uint64_t cpu_frame_p95_us{0};
    std::uint64_t cpu_frame_p99_us{0};
    std::uint64_t cpu_streaming_jobs_p95_us{0};
    std::uint64_t gpu_frame_p50_us{0};
    std::uint64_t gpu_frame_p95_us{0};
    std::uint64_t gpu_frame_p99_us{0};
    std::uint64_t gpu_upload_p95_us{0};
    std::uint64_t gpu_draw_dispatch_p95_us{0};
    std::uint64_t vram_peak_bytes{0};
    std::uint64_t resident_page_bytes{0};
    std::uint64_t page_fault_count{0};
    std::uint64_t io_bytes_per_second{0};
    std::uint64_t io_request_count{0};
    std::uint64_t heap_allocation_count{0};
    std::string_view replay_hash;
};

struct RuntimeMavgBroadOptimizationEvidenceRow {
    RuntimeMavgBroadOptimizationBackend backend{RuntimeMavgBroadOptimizationBackend::d3d12};
    RuntimeMavgBroadOptimizationVendorClass vendor{RuntimeMavgBroadOptimizationVendorClass::nvidia};
    RuntimeMavgBroadOptimizationWorkloadKind workload{RuntimeMavgBroadOptimizationWorkloadKind::mavg_stress_package};
    RuntimeMavgBroadOptimizationProfilerTool profiler_tool{
        RuntimeMavgBroadOptimizationProfilerTool::pix_timing_capture};
    std::string_view row_id;
    std::string_view package_hash_sha256;
    std::string_view camera_script_id;
    std::string_view host_class;
    std::string_view adapter_id;
    std::string_view driver_version;
    std::string_view profiler_tool_version;
    std::string_view profiler_tool_support_source_id;
    std::string_view profiler_artifact_id;
    std::string_view profiler_artifact_hash_sha256;
    RuntimeMavgBroadOptimizationMetrics baseline;
    RuntimeMavgBroadOptimizationMetrics optimized;
    std::uint32_t profiler_capture_overhead_basis_points{0};
    bool reviewed{false};
    bool official_profiler_tool{false};
    bool profiler_tool_currently_supported{false};
    bool internal_engine_counters_ready{false};
    bool external_profiler_artifact_ready{false};
    bool first_party_package{false};
    bool cpu_timeline_available{false};
    bool gpu_timeline_available{false};
    bool io_timeline_available{false};
    bool memory_timeline_available{false};
    bool queue_timeline_available{false};
    bool no_profiler_timestamp_overflow{false};
    bool representative_capture{false};
    bool synthetic_only{false};
    bool native_handles_exposed{false};
    bool claims_broad_engine_ready{false};
    bool claims_broad_backend_readiness{false};
};

struct RuntimeMavgBroadOptimizationEvidenceDesc {
    std::span<const RuntimeMavgBroadOptimizationEvidenceRow> rows;
    std::uint32_t minimum_cpu_p95_improvement_basis_points{100};
    std::uint32_t minimum_gpu_p95_improvement_basis_points{100};
    std::uint32_t minimum_vram_peak_improvement_basis_points{100};
    std::uint32_t maximum_p99_regression_basis_points{200};
    std::uint32_t maximum_profiler_capture_overhead_basis_points{500};
};

struct RuntimeMavgBroadOptimizationEvidenceResult {
    RuntimeMavgBroadOptimizationStatus status{RuntimeMavgBroadOptimizationStatus::host_evidence_required};
    std::vector<RuntimeMavgBroadOptimizationDiagnostic> diagnostics;
    std::size_t required_evidence_rows{14U};
    std::size_t retained_evidence_rows{0U};
    std::size_t ready_evidence_rows{0U};
    std::size_t host_gated_rows{0U};
    std::size_t blocked_rows{0U};
    std::size_t eol_or_unsupported_profiler_rows{0U};
    std::size_t profiler_backend_vendor_mismatch_rows{0U};
    std::size_t official_profiler_rows{0U};
    std::size_t internal_counter_rows{0U};
    std::size_t mavg_stress_package_rows{0U};
    std::size_t gameplay_package_rows{0U};
    std::size_t d3d12_rows{0U};
    std::size_t vulkan_rows{0U};
    std::size_t metal_rows{0U};
    std::size_t nvidia_rows{0U};
    std::size_t amd_rows{0U};
    std::size_t intel_rows{0U};
    std::size_t apple_rows{0U};
    bool mavg_broad_cpu_gpu_memory_optimization_ready{false};
    bool mavg_package_visible_backend_readiness_ready{false};
    bool mavg_nanite_compatible{false};
    bool mavg_nanite_equivalent{false};
    bool mavg_nanite_superior{false};
    bool mavg_metal_mesh_lod_ready{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == RuntimeMavgBroadOptimizationStatus::ready && mavg_broad_cpu_gpu_memory_optimization_ready;
    }
};

[[nodiscard]] std::string_view
runtime_mavg_broad_optimization_status_label(RuntimeMavgBroadOptimizationStatus status) noexcept;
[[nodiscard]] RuntimeMavgBroadOptimizationEvidenceResult
evaluate_runtime_mavg_broad_optimization_evidence(const RuntimeMavgBroadOptimizationEvidenceDesc& desc);
[[nodiscard]] bool
has_runtime_mavg_broad_optimization_diagnostic(const RuntimeMavgBroadOptimizationEvidenceResult& result,
                                               RuntimeMavgBroadOptimizationDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
