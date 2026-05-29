// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class RendererProductionVfxProfilingStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    no_rows,
    invalid_request,
};

enum class RendererProductionVfxFeatureKind : std::uint8_t {
    gpu_particles = 0,
    postprocess_chain,
    backend_timing,
    crash_telemetry_handoff,
};

enum class RendererProductionVfxProfilingDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_required_backend,
    duplicate_required_backend,
    unsupported_backend,
    invalid_feature_row,
    broad_performance_claim,
    unsupported_native_handle_claim,
    invalid_gpu_particle_budget,
    invalid_postprocess_row,
    invalid_backend_timing,
    invalid_crash_telemetry_handoff,
    unsupported_crash_upload,
    missing_backend_parity,
    missing_backend_timing,
    missing_backend_synchronization_evidence,
    missing_backend_shader_validation,
    missing_backend_validation_evidence,
    missing_backend_host_evidence,
    missing_backend_capture_handoff,
    invalid_cpu_profile_row,
    invalid_package_counter_row,
    missing_cpu_profile_rows,
    missing_package_counter_rows,
    row_budget_exceeded,
};

struct RendererProductionVfxFeatureRow {
    std::string feature_id;
    RendererProductionVfxFeatureKind kind{RendererProductionVfxFeatureKind::gpu_particles};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool reviewed{false};
    bool request_broad_performance_claim{false};
    bool request_native_handle_access{false};
    std::uint32_t source_index{0U};
};

struct RendererProductionGpuParticleBudgetRow {
    std::string effect_id;
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::uint32_t max_particles{0U};
    std::uint32_t max_emitters{0U};
    std::uint32_t max_spawn_per_frame{0U};
    std::uint32_t simulation_budget_us{0U};
    std::uint32_t submission_budget_us{0U};
    bool requires_compute_simulation{false};
    bool requires_gpu_sort{false};
    std::uint32_t source_index{0U};
};

struct RendererProductionPostprocessRow {
    std::string chain_id;
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::uint32_t pass_count{0U};
    bool scene_color_available{false};
    bool scene_depth_available{false};
    bool hdr_available{false};
    bool history_available{false};
    bool backend_shader_evidence_ready{false};
    std::uint32_t source_index{0U};
};

struct RendererProductionBackendTimingRow {
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::string profile_zone_id;
    std::uint64_t gpu_timestamp_frequency_hz{0U};
    std::uint64_t begin_tick{0U};
    std::uint64_t end_tick{0U};
    std::uint64_t calibrated_cpu_begin_tick{0U};
    std::uint64_t calibrated_cpu_end_tick{0U};
    std::uint64_t max_clock_deviation_ns{0U};
    std::uint32_t debug_scope_count{0U};
    std::uint32_t debug_marker_count{0U};
    std::uint32_t resource_barrier_count{0U};
    std::uint32_t layout_transition_count{0U};
    std::uint32_t queue_wait_count{0U};
    bool queue_ownership_transfer_reviewed{false};
    std::uint32_t shader_validation_count{0U};
    bool backend_validation_ready{false};
    bool strict_host_recipe_ready{false};
    bool capture_handoff_ready{false};
    bool host_validated{false};
    std::uint32_t source_index{0U};
};

struct RendererProductionBackendEvidenceRow {
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::string profile_zone_id;
    std::uint32_t resource_barrier_count{0U};
    std::uint32_t layout_transition_count{0U};
    std::uint32_t queue_wait_count{0U};
    bool queue_ownership_transfer_reviewed{false};
    std::uint32_t shader_validation_count{0U};
    bool timing_ready{false};
    bool synchronization_ready{false};
    bool shader_validation_ready{false};
    bool backend_validation_ready{false};
    bool host_recipe_ready{false};
    bool capture_handoff_ready{false};
    bool host_evidence_ready{false};
    bool host_gated{false};
    std::uint32_t source_index{0U};
};

struct RendererProductionCpuProfileRow {
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::string profile_zone_id;
    std::uint64_t begin_tick{0U};
    std::uint64_t end_tick{0U};
    std::uint32_t budget_us{0U};
    std::uint32_t sample_count{0U};
    std::uint32_t source_index{0U};
};

struct RendererProductionPackageCounterRow {
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::string counter_id;
    bool ready{false};
    bool host_gated{false};
    std::uint32_t source_index{0U};
};

struct RendererProductionCrashTelemetryHandoffRow {
    std::string handoff_id;
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::uint32_t trace_event_count{0U};
    bool crash_dump_reviewed{false};
    bool symbolication_ready{false};
    bool telemetry_schema_reviewed{false};
    bool operator_handoff_ready{false};
    bool request_crash_upload{false};
    bool request_native_capture{false};
    std::uint32_t source_index{0U};
};

struct RendererProductionVfxProfilingRequest {
    std::vector<rhi::BackendKind> required_backends;
    std::vector<RendererProductionVfxFeatureRow> feature_rows;
    std::vector<RendererProductionGpuParticleBudgetRow> gpu_particle_budget_rows;
    std::vector<RendererProductionPostprocessRow> postprocess_rows;
    std::vector<RendererProductionBackendTimingRow> backend_timing_rows;
    std::vector<RendererProductionCpuProfileRow> cpu_profile_rows;
    std::vector<RendererProductionPackageCounterRow> package_counter_rows;
    std::vector<RendererProductionCrashTelemetryHandoffRow> crash_telemetry_handoff_rows;
    std::size_t row_budget{256U};
    std::uint64_t seed{0U};
};

struct RendererProductionVfxProfilingDiagnostic {
    RendererProductionVfxProfilingDiagnosticCode code{RendererProductionVfxProfilingDiagnosticCode::none};
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RendererProductionVfxProfilingPlan {
    RendererProductionVfxProfilingStatus status{RendererProductionVfxProfilingStatus::invalid_request};
    std::vector<RendererProductionVfxProfilingDiagnostic> diagnostics;
    std::vector<rhi::BackendKind> required_backends;
    std::vector<RendererProductionVfxFeatureRow> feature_rows;
    std::vector<RendererProductionGpuParticleBudgetRow> gpu_particle_budget_rows;
    std::vector<RendererProductionPostprocessRow> postprocess_rows;
    std::vector<RendererProductionBackendTimingRow> backend_timing_rows;
    std::vector<RendererProductionBackendEvidenceRow> backend_evidence_rows;
    std::vector<RendererProductionCpuProfileRow> cpu_profile_rows;
    std::vector<RendererProductionPackageCounterRow> package_counter_rows;
    std::vector<RendererProductionCrashTelemetryHandoffRow> crash_telemetry_handoff_rows;
    std::size_t feature_row_count{0U};
    std::size_t gpu_particle_budget_row_count{0U};
    std::size_t postprocess_row_count{0U};
    std::size_t backend_timing_row_count{0U};
    std::size_t backend_evidence_row_count{0U};
    std::size_t backend_evidence_ready_count{0U};
    std::size_t backend_evidence_host_gated_count{0U};
    std::size_t cpu_profile_row_count{0U};
    std::size_t package_counter_row_count{0U};
    std::size_t package_counter_ready_count{0U};
    std::size_t package_counter_host_gated_count{0U};
    std::size_t crash_telemetry_handoff_row_count{0U};
    std::size_t host_validated_backend_count{0U};
    std::size_t rejected_unsafe_row_count{0U};
    std::uint64_t replay_hash{0U};
    bool d3d12_host_evidence_ready{false};
    bool vulkan_strict_host_evidence_ready{false};
    bool metal_host_evidence_ready{false};
    bool requires_metal_host_evidence{false};
    bool has_metal_host_evidence{false};
    bool invoked_gpu_commands{false};
    bool invoked_native_capture{false};
    bool invoked_crash_upload{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews renderer production VFX/profile evidence rows without executing GPU commands, native captures, uploads,
/// external tools, crash export, or backend handle access. Backend-specific evidence never transfers across backends.
[[nodiscard]] RendererProductionVfxProfilingPlan
plan_renderer_production_vfx_profiling(const RendererProductionVfxProfilingRequest& request);

} // namespace mirakana
