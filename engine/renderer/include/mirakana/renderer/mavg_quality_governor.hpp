// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgQualityGovernorStatus : std::uint8_t {
    ready = 0,
    budget_exceeded,
    host_evidence_required,
    no_rows,
    invalid_request,
};

enum class MavgQualityGovernorFeatureKind : std::uint8_t {
    static_lod = 0,
    streaming_residency,
    gpu_culling,
    mesh_shader,
    deformation,
    ray_tracing,
    benchmark_summary,
};

enum class MavgQualityGovernorDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_limits,
    invalid_row_id,
    duplicate_counter_row,
    row_budget_exceeded,
    missing_host_evidence,
    missing_backend_local_evidence,
    missing_no_hole_evidence,
    missing_fallback_evidence,
    missing_rt_consistency_evidence,
    cpu_frame_budget_exceeded,
    gpu_frame_budget_exceeded,
    mavg_cpu_selection_budget_exceeded,
    mavg_gpu_culling_budget_exceeded,
    screen_error_budget_exceeded,
    fallback_rate_budget_exceeded,
    page_miss_rate_budget_exceeded,
    temporal_churn_budget_exceeded,
    missing_required_geometry,
    rt_consistency_diagnostic_budget_exceeded,
    resident_gpu_budget_exceeded,
    upload_budget_exceeded,
    draw_budget_exceeded,
    dispatch_budget_exceeded,
    unsupported_native_handle_access,
    unsupported_nanite_claim,
    unsupported_benchmark_superiority_claim,
    unsupported_broad_optimization_claim,
    unsupported_backend_parity_claim,
};

struct MavgQualityGovernorLimits {
    std::uint32_t max_cpu_frame_time_p95_us{0U};
    std::uint32_t max_gpu_frame_time_p95_us{0U};
    std::uint32_t max_mavg_cpu_selection_time_p95_us{0U};
    std::uint32_t max_mavg_gpu_culling_time_p95_us{0U};
    std::uint32_t max_screen_error_p99_micropixels{0U};
    std::uint32_t max_fallback_rate_per_million{0U};
    std::uint32_t max_page_miss_rate_per_million{0U};
    std::uint32_t max_temporal_churn_per_million{0U};
    std::uint32_t max_missing_required_geometry_count{0U};
    std::uint32_t max_rt_consistency_diagnostic_count{0U};
    std::uint64_t max_resident_gpu_bytes{0U};
    std::uint64_t max_upload_bytes{0U};
    std::uint32_t max_draw_count{0U};
    std::uint32_t max_dispatch_count{0U};
};

struct MavgQualityGovernorCounterRow {
    std::string scene_id;
    std::string package_target_id;
    std::string validation_recipe_id;
    std::string benchmark_command_id;
    rhi::BackendKind backend{rhi::BackendKind::null};
    MavgQualityGovernorFeatureKind feature{MavgQualityGovernorFeatureKind::static_lod};
    bool host_evidence{false};
    bool host_gate_required{false};
    bool backend_local_evidence{false};
    bool no_hole_evidence{false};
    bool fallback_evidence{false};
    bool rt_consistency_evidence{false};
    std::uint32_t cpu_frame_time_p95_us{0U};
    std::uint32_t gpu_frame_time_p95_us{0U};
    std::uint32_t mavg_cpu_selection_time_p95_us{0U};
    std::uint32_t mavg_gpu_culling_time_p95_us{0U};
    std::uint32_t screen_error_p99_micropixels{0U};
    std::uint32_t fallback_rate_per_million{0U};
    std::uint32_t page_miss_rate_per_million{0U};
    std::uint32_t temporal_churn_per_million{0U};
    std::uint32_t missing_required_geometry_count{0U};
    std::uint32_t rt_consistency_diagnostic_count{0U};
    std::uint64_t resident_gpu_bytes{0U};
    std::uint64_t upload_bytes{0U};
    std::uint32_t draw_count{0U};
    std::uint32_t dispatch_count{0U};
    bool request_native_handle_access{false};
    bool request_nanite_claim{false};
    bool request_benchmark_superiority_claim{false};
    bool request_broad_optimization_claim{false};
    bool request_backend_parity_claim{false};
    std::uint32_t source_index{0U};
};

struct MavgQualityGovernorRequest {
    MavgQualityGovernorLimits limits;
    std::vector<MavgQualityGovernorCounterRow> rows;
    std::size_t row_budget{512U};
    bool require_rt_consistency_evidence{false};
    std::uint64_t seed{0U};
};

struct MavgQualityGovernorDiagnostic {
    MavgQualityGovernorDiagnosticCode code{MavgQualityGovernorDiagnosticCode::none};
    std::string scene_id;
    rhi::BackendKind backend{rhi::BackendKind::null};
    MavgQualityGovernorFeatureKind feature{MavgQualityGovernorFeatureKind::static_lod};
    std::string field;
    std::uint64_t value{0U};
    std::uint64_t budget{0U};
    std::string message;
    std::uint32_t source_index{0U};
};

struct MavgQualityGovernorResult {
    MavgQualityGovernorStatus status{MavgQualityGovernorStatus::invalid_request};
    std::vector<MavgQualityGovernorDiagnostic> diagnostics;
    std::vector<MavgQualityGovernorCounterRow> rows;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t host_gated_row_count{0U};
    std::size_t unsupported_claim_row_count{0U};
    std::uint64_t replay_hash{0U};
    bool ready{false};
    bool selected_benchmark_harness_ready{false};
    bool broad_mavg_benchmark_ready{false};
    bool invoked_gpu_commands{false};
    bool invoked_profiler_capture{false};
    bool mutated_packages{false};
    bool accessed_native_handles{false};
};

/// Evaluates caller-supplied MAVG benchmark and quality rows without executing GPU work,
/// profiler capture, package mutation, validation recipes, or native handle access.
[[nodiscard]] MavgQualityGovernorResult evaluate_mavg_quality_governor(const MavgQualityGovernorRequest& request);

[[nodiscard]] bool has_mavg_quality_governor_diagnostic(const MavgQualityGovernorResult& result,
                                                        MavgQualityGovernorDiagnosticCode code) noexcept;

} // namespace mirakana
