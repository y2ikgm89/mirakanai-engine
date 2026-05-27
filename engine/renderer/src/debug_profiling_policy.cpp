// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/debug_profiling_policy.hpp"

#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool is_supported_capture_kind(DebugProfilingCaptureKind kind) noexcept {
    switch (kind) {
    case DebugProfilingCaptureKind::none:
    case DebugProfilingCaptureKind::pix_gpu_handoff:
    case DebugProfilingCaptureKind::vulkan_debug_utils:
    case DebugProfilingCaptureKind::metal_capture:
        return true;
    }
    return false;
}

[[nodiscard]] std::string_view capture_kind_name(DebugProfilingCaptureKind kind) noexcept {
    switch (kind) {
    case DebugProfilingCaptureKind::none:
        return "none";
    case DebugProfilingCaptureKind::pix_gpu_handoff:
        return "pix_gpu_handoff";
    case DebugProfilingCaptureKind::vulkan_debug_utils:
        return "vulkan_debug_utils";
    case DebugProfilingCaptureKind::metal_capture:
        return "metal_capture";
    }
    return "unknown";
}

[[nodiscard]] std::string_view backend_name(rhi::BackendKind backend) noexcept {
    switch (backend) {
    case rhi::BackendKind::null:
        return "null";
    case rhi::BackendKind::d3d12:
        return "d3d12";
    case rhi::BackendKind::vulkan:
        return "vulkan";
    case rhi::BackendKind::metal:
        return "metal";
    }
    return "unknown";
}

void add_diagnostic(DebugProfilingPolicyPlan& plan, DebugProfilingDiagnosticCode code, std::size_t request_index,
                    std::uint32_t source_index, std::string message) {
    plan.diagnostics.push_back(DebugProfilingDiagnostic{
        .code = code,
        .request_index = request_index,
        .source_index = source_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool gpu_timestamps_ready(const DebugProfilingPolicyDesc& desc) noexcept {
    return desc.gpu_timestamp_ticks_per_second > 0;
}

[[nodiscard]] bool gpu_debug_markers_ready(const DebugProfilingPolicyDesc& desc) noexcept {
    return desc.gpu_debug_scopes_begun > 0 || desc.gpu_debug_scopes_ended > 0 || desc.gpu_debug_markers_inserted > 0;
}

[[nodiscard]] bool cpu_profile_zone_evidence_ready(const DebugProfilingPolicyDesc& desc) noexcept {
    return desc.cpu_profile_zone_count > 0;
}

[[nodiscard]] bool trace_capture_handoff_evidence_ready(const DebugProfilingPolicyDesc& desc) noexcept {
    return desc.trace_capture_handoff_row_count > 0;
}

[[nodiscard]] bool capture_handoff_ready(DebugProfilingCaptureKind kind, rhi::BackendKind backend) noexcept {
    switch (kind) {
    case DebugProfilingCaptureKind::pix_gpu_handoff:
        return backend == rhi::BackendKind::d3d12;
    case DebugProfilingCaptureKind::vulkan_debug_utils:
        return backend == rhi::BackendKind::vulkan;
    case DebugProfilingCaptureKind::metal_capture:
        return backend == rhi::BackendKind::metal;
    case DebugProfilingCaptureKind::none:
        return true;
    }
    return false;
}

[[nodiscard]] bool frame_diagnostics_ready(const DebugProfilingPolicyDesc& desc) noexcept {
    return desc.frames_finished > 0 && desc.framegraph_barrier_steps_executed > 0 &&
           desc.framegraph_render_passes_recorded > 0;
}

} // namespace

DebugProfilingPolicyPlan plan_debug_profiling_policy(const DebugProfilingPolicyDesc& desc) {
    DebugProfilingPolicyPlan plan;
    plan.expected_frames = desc.expected_frames;
    plan.frames_finished = desc.frames_finished;
    plan.gpu_timestamp_ticks_per_second = desc.gpu_timestamp_ticks_per_second;
    plan.gpu_debug_scopes_begun = desc.gpu_debug_scopes_begun;
    plan.gpu_debug_scopes_ended = desc.gpu_debug_scopes_ended;
    plan.gpu_debug_markers_inserted = desc.gpu_debug_markers_inserted;
    plan.cpu_profile_zone_count = desc.cpu_profile_zone_count;
    plan.trace_capture_handoff_row_count = desc.trace_capture_handoff_row_count;
    plan.framegraph_barrier_steps_executed = desc.framegraph_barrier_steps_executed;
    plan.framegraph_render_passes_recorded = desc.framegraph_render_passes_recorded;
    plan.backend = desc.backend;
    plan.backend_profiling_evidence_ready = desc.backend_profiling_evidence_ready;
    plan.frame_diagnostics_ready = frame_diagnostics_ready(desc);
    plan.cpu_profile_zone_evidence_ready = cpu_profile_zone_evidence_ready(desc);
    plan.trace_capture_handoff_evidence_ready = trace_capture_handoff_evidence_ready(desc);
    plan.package_counter_evidence_ready = desc.package_counter_evidence_ready;

    if (desc.requests.empty()) {
        add_diagnostic(plan, DebugProfilingDiagnosticCode::no_profiling_requests, 0, 0,
                       "debug profiling policy requires at least one request row");
        return plan;
    }

    if (!plan.frame_diagnostics_ready) {
        add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_frame_diagnostic_evidence, 0, 0,
                       "frame diagnostics require finished frames plus framegraph barrier and render-pass counters");
    }

    if (desc.require_backend_profiling_evidence && !desc.backend_profiling_evidence_ready) {
        add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_backend_profiling_evidence, 0, 0,
                       "backend profiling evidence is required but not ready for backend " +
                           std::string(backend_name(desc.backend)));
    }

    for (std::size_t request_index = 0; request_index < desc.requests.size(); ++request_index) {
        const auto& request = desc.requests[request_index];
        ++plan.request_count;

        if (!is_supported_capture_kind(request.capture_kind)) {
            add_diagnostic(plan, DebugProfilingDiagnosticCode::unsupported_capture_kind, request_index,
                           request.source_index,
                           "unsupported capture kind for request source_index=" + std::to_string(request.source_index));
            continue;
        }

        if (request.request_automatic_capture_execution) {
            add_diagnostic(plan, DebugProfilingDiagnosticCode::unsupported_automatic_capture_execution, request_index,
                           request.source_index,
                           "automatic external capture execution remains operator-owned and unsupported");
        }
        if (request.request_production_flame_graph) {
            add_diagnostic(plan, DebugProfilingDiagnosticCode::unsupported_production_flame_graph, request_index,
                           request.source_index, "production flame graphs remain unsupported");
        }
        if (request.request_crash_telemetry_export) {
            add_diagnostic(plan, DebugProfilingDiagnosticCode::unsupported_crash_telemetry_export, request_index,
                           request.source_index, "crash telemetry export remains unsupported");
        }
        if (!request.scene_frame_resources_available) {
            add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_scene_frame_resources, request_index,
                           request.source_index,
                           "scene frame resources are unavailable for request source_index=" +
                               std::to_string(request.source_index));
        }

        const auto timestamps_ready = gpu_timestamps_ready(desc);
        const auto markers_ready = gpu_debug_markers_ready(desc);
        const auto handoff_ready = capture_handoff_ready(request.capture_kind, desc.backend);
        const auto cpu_zones_ready = cpu_profile_zone_evidence_ready(desc);
        const auto trace_handoff_ready = trace_capture_handoff_evidence_ready(desc);
        const auto package_counter_ready = desc.package_counter_evidence_ready;

        if (request.require_gpu_timestamps) {
            ++plan.gpu_timestamp_request_count;
            if (!timestamps_ready) {
                add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_gpu_timestamp_support, request_index,
                               request.source_index,
                               "GPU timestamp support is unavailable for backend " +
                                   std::string(backend_name(desc.backend)));
            }
        }
        if (request.require_gpu_debug_markers) {
            ++plan.gpu_debug_marker_request_count;
            if (!markers_ready) {
                add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_gpu_debug_marker_evidence, request_index,
                               request.source_index,
                               "GPU debug marker or scope evidence is missing for request source_index=" +
                                   std::to_string(request.source_index));
            }
        }
        if (request.require_capture_handoff_evidence) {
            ++plan.capture_handoff_request_count;
            if (!handoff_ready) {
                add_diagnostic(plan, DebugProfilingDiagnosticCode::unsupported_capture_kind, request_index,
                               request.source_index,
                               "capture handoff evidence is host-gated for capture_kind=" +
                                   std::string(capture_kind_name(request.capture_kind)) + " on backend " +
                                   std::string(backend_name(desc.backend)));
            }
        }
        if (request.require_cpu_profile_zone_evidence) {
            ++plan.cpu_profile_zone_request_count;
            if (!cpu_zones_ready) {
                add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_cpu_profile_zone_evidence, request_index,
                               request.source_index,
                               "CPU profile zone evidence is required for broad renderer profiling");
            }
        }
        if (request.require_trace_capture_handoff_evidence) {
            ++plan.trace_capture_handoff_request_count;
            if (!trace_handoff_ready) {
                add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_trace_capture_handoff_evidence,
                               request_index, request.source_index,
                               "trace/capture handoff review evidence is required without executing external capture");
            }
        }
        if (request.require_package_counter_evidence) {
            ++plan.package_counter_request_count;
            if (!package_counter_ready) {
                add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_package_counter_evidence, request_index,
                               request.source_index,
                               "package-visible renderer profiling counters are required for broad profiling evidence");
            }
        }

        plan.request_rows.push_back(DebugProfilingRequestRow{
            .capture_kind = request.capture_kind,
            .requires_gpu_timestamps = request.require_gpu_timestamps,
            .requires_gpu_debug_markers = request.require_gpu_debug_markers,
            .requires_capture_handoff_evidence = request.require_capture_handoff_evidence,
            .gpu_timestamps_ready = timestamps_ready,
            .gpu_debug_markers_ready = markers_ready,
            .capture_handoff_ready = handoff_ready,
            .cpu_profile_zone_evidence_ready = cpu_zones_ready,
            .trace_capture_handoff_evidence_ready = trace_handoff_ready,
            .package_counter_evidence_ready = package_counter_ready,
            .source_index = request.source_index,
        });
    }

    return plan;
}

bool has_debug_profiling_policy_diagnostic(const DebugProfilingPolicyPlan& plan,
                                           DebugProfilingDiagnosticCode code) noexcept {
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

bool debug_profiling_policy_backend_evidence_ready(const DebugProfilingBackendEvidenceDesc& desc) noexcept {
    const auto gpu_debug_ready =
        desc.gpu_debug_scopes_begun > 0 || desc.gpu_debug_scopes_ended > 0 || desc.gpu_debug_markers_inserted > 0;
    const auto frame_diagnostics_ready =
        desc.framegraph_barrier_steps_executed > 0 && desc.framegraph_render_passes_recorded > 0;
    switch (desc.backend) {
    case rhi::BackendKind::d3d12:
        return desc.gpu_timestamp_ticks_per_second > 0 && gpu_debug_ready && frame_diagnostics_ready;
    case rhi::BackendKind::vulkan:
        return gpu_debug_ready && frame_diagnostics_ready;
    case rhi::BackendKind::null:
    case rhi::BackendKind::metal:
        return false;
    }
    return false;
}

} // namespace mirakana
