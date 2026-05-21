// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class DebugProfilingCaptureKind : std::uint8_t {
    none = 0,
    pix_gpu_handoff,
    vulkan_debug_utils,
    metal_capture,
};

enum class DebugProfilingDiagnosticCode : std::uint8_t {
    none = 0,
    no_profiling_requests,
    unsupported_capture_kind,
    missing_gpu_timestamp_support,
    missing_gpu_debug_marker_evidence,
    missing_frame_diagnostic_evidence,
    missing_scene_frame_resources,
    missing_backend_profiling_evidence,
    unsupported_automatic_capture_execution,
    unsupported_production_flame_graph,
    unsupported_crash_telemetry_export,
};

struct DebugProfilingRequestDesc {
    DebugProfilingCaptureKind capture_kind{DebugProfilingCaptureKind::none};
    bool require_gpu_timestamps{false};
    bool require_gpu_debug_markers{false};
    bool require_capture_handoff_evidence{false};
    bool scene_frame_resources_available{true};
    bool request_automatic_capture_execution{false};
    bool request_production_flame_graph{false};
    bool request_crash_telemetry_export{false};
    std::uint32_t source_index{0};
};

struct DebugProfilingPolicyDesc {
    std::span<const DebugProfilingRequestDesc> requests;
    std::uint64_t expected_frames{0};
    std::uint64_t frames_finished{0};
    std::uint64_t gpu_timestamp_ticks_per_second{0};
    std::uint64_t gpu_debug_scopes_begun{0};
    std::uint64_t gpu_debug_scopes_ended{0};
    std::uint64_t gpu_debug_markers_inserted{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    std::uint64_t framegraph_render_passes_recorded{0};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool require_backend_profiling_evidence{false};
    bool backend_profiling_evidence_ready{false};
};

struct DebugProfilingRequestRow {
    DebugProfilingCaptureKind capture_kind{DebugProfilingCaptureKind::none};
    bool requires_gpu_timestamps{false};
    bool requires_gpu_debug_markers{false};
    bool requires_capture_handoff_evidence{false};
    bool gpu_timestamps_ready{false};
    bool gpu_debug_markers_ready{false};
    bool capture_handoff_ready{false};
    std::uint32_t source_index{0};
};

struct DebugProfilingDiagnostic {
    DebugProfilingDiagnosticCode code{DebugProfilingDiagnosticCode::none};
    std::size_t request_index{0};
    std::uint32_t source_index{0};
    std::string message;
};

struct DebugProfilingPolicyPlan {
    std::uint32_t request_count{0};
    std::uint64_t expected_frames{0};
    std::uint64_t frames_finished{0};
    std::uint64_t gpu_timestamp_ticks_per_second{0};
    std::uint64_t gpu_debug_scopes_begun{0};
    std::uint64_t gpu_debug_scopes_ended{0};
    std::uint64_t gpu_debug_markers_inserted{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    std::uint64_t framegraph_render_passes_recorded{0};
    std::uint32_t gpu_timestamp_request_count{0};
    std::uint32_t gpu_debug_marker_request_count{0};
    std::uint32_t capture_handoff_request_count{0};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool backend_profiling_evidence_ready{false};
    bool frame_diagnostics_ready{false};
    std::vector<DebugProfilingRequestRow> request_rows;
    std::vector<DebugProfilingDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] DebugProfilingPolicyPlan plan_debug_profiling_policy(const DebugProfilingPolicyDesc& desc);

[[nodiscard]] bool has_debug_profiling_policy_diagnostic(const DebugProfilingPolicyPlan& plan,
                                                         DebugProfilingDiagnosticCode code) noexcept;

struct DebugProfilingBackendEvidenceDesc {
    rhi::BackendKind backend{rhi::BackendKind::null};
    std::uint64_t gpu_timestamp_ticks_per_second{0};
    std::uint64_t gpu_debug_scopes_begun{0};
    std::uint64_t gpu_debug_scopes_ended{0};
    std::uint64_t gpu_debug_markers_inserted{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    std::uint64_t framegraph_render_passes_recorded{0};
};

[[nodiscard]] bool
debug_profiling_policy_backend_evidence_ready(const DebugProfilingBackendEvidenceDesc& desc) noexcept;

} // namespace mirakana
