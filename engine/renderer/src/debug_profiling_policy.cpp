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

[[nodiscard]] bool is_supported_package_counter(DebugProfilingPackageCounterKind kind) noexcept {
    switch (kind) {
    case DebugProfilingPackageCounterKind::cpu_profile_zone_count:
    case DebugProfilingPackageCounterKind::cpu_profile_budget_microseconds:
    case DebugProfilingPackageCounterKind::gpu_profile_budget_microseconds:
    case DebugProfilingPackageCounterKind::gpu_timestamp_frequency:
    case DebugProfilingPackageCounterKind::gpu_debug_marker_count:
    case DebugProfilingPackageCounterKind::frame_diagnostic_count:
    case DebugProfilingPackageCounterKind::trace_capture_handoff:
        return true;
    case DebugProfilingPackageCounterKind::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] std::string_view package_counter_name(DebugProfilingPackageCounterKind kind) noexcept {
    switch (kind) {
    case DebugProfilingPackageCounterKind::cpu_profile_zone_count:
        return "cpu_profile_zone_count";
    case DebugProfilingPackageCounterKind::cpu_profile_budget_microseconds:
        return "cpu_profile_budget_microseconds";
    case DebugProfilingPackageCounterKind::gpu_profile_budget_microseconds:
        return "gpu_profile_budget_microseconds";
    case DebugProfilingPackageCounterKind::gpu_timestamp_frequency:
        return "gpu_timestamp_frequency";
    case DebugProfilingPackageCounterKind::gpu_debug_marker_count:
        return "gpu_debug_marker_count";
    case DebugProfilingPackageCounterKind::frame_diagnostic_count:
        return "frame_diagnostic_count";
    case DebugProfilingPackageCounterKind::trace_capture_handoff:
        return "trace_capture_handoff";
    case DebugProfilingPackageCounterKind::unknown:
        return "unknown";
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

[[nodiscard]] bool capture_handoff_backend_matches(DebugProfilingCaptureKind kind, rhi::BackendKind backend) noexcept {
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
    plan.cpu_profile_zone_count = desc.cpu_profile_zone_count;
    plan.cpu_profile_budget_microseconds = desc.cpu_profile_budget_microseconds;
    plan.gpu_profile_budget_microseconds = desc.gpu_profile_budget_microseconds;
    plan.gpu_timestamp_ticks_per_second = desc.gpu_timestamp_ticks_per_second;
    plan.gpu_debug_scopes_begun = desc.gpu_debug_scopes_begun;
    plan.gpu_debug_scopes_ended = desc.gpu_debug_scopes_ended;
    plan.gpu_debug_markers_inserted = desc.gpu_debug_markers_inserted;
    plan.framegraph_barrier_steps_executed = desc.framegraph_barrier_steps_executed;
    plan.framegraph_render_passes_recorded = desc.framegraph_render_passes_recorded;
    plan.backend = desc.backend;
    plan.backend_profiling_evidence_ready = desc.backend_profiling_evidence_ready;
    plan.frame_diagnostics_ready = frame_diagnostics_ready(desc);
    plan.cpu_profile_zone_evidence_ready = desc.cpu_profile_zone_count > 0;
    plan.profile_budget_ready = desc.cpu_profile_budget_microseconds > 0 && desc.gpu_profile_budget_microseconds > 0;
    plan.trace_capture_handoff_review_ready = desc.trace_capture_handoff_review_ready;

    if (desc.requests.empty()) {
        add_diagnostic(plan, DebugProfilingDiagnosticCode::no_profiling_requests, 0, 0,
                       "debug profiling policy requires at least one request row");
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
    if (desc.require_cpu_profile_zone_evidence && !plan.cpu_profile_zone_evidence_ready) {
        add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_cpu_profile_zone_evidence, 0, 0,
                       "debug profiling policy requires package-visible CPU profile zone evidence");
    }
    if (desc.require_profile_budget_evidence && !plan.profile_budget_ready) {
        add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_profile_budget_evidence, 0, 0,
                       "debug profiling policy requires CPU and GPU profile budget evidence");
    }
    if (desc.require_package_counter_evidence && desc.package_counters.empty()) {
        add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_package_counter_evidence, 0, 0,
                       "debug profiling policy requires package-visible counter rows");
    }

    for (std::size_t counter_index = 0; counter_index < desc.package_counters.size(); ++counter_index) {
        const auto& counter = desc.package_counters[counter_index];
        ++plan.package_counter_count;
        const auto supported = is_supported_package_counter(counter.kind);
        const auto ready = supported && counter.value > 0;
        if (!supported) {
            add_diagnostic(
                plan, DebugProfilingDiagnosticCode::invalid_package_counter, counter_index, counter.source_index,
                "unsupported debug profiling package counter kind=" + std::string(package_counter_name(counter.kind)));
        }
        if (counter.required && !ready) {
            add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_package_counter_evidence, counter_index,
                           counter.source_index,
                           "debug profiling package counter requires a positive value for " +
                               std::string(package_counter_name(counter.kind)));
        }
        if (ready) {
            ++plan.package_counter_ready_count;
        }
        plan.package_counter_rows.push_back(DebugProfilingPackageCounterRow{
            .kind = counter.kind,
            .value = counter.value,
            .required = counter.required,
            .ready = ready,
            .source_index = counter.source_index,
        });
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
        const auto handoff_backend_matches = capture_handoff_backend_matches(request.capture_kind, desc.backend);
        const auto handoff_ready = request.capture_kind == DebugProfilingCaptureKind::none ||
                                   (handoff_backend_matches && desc.trace_capture_handoff_review_ready);

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
            if (!handoff_backend_matches) {
                add_diagnostic(plan, DebugProfilingDiagnosticCode::unsupported_capture_kind, request_index,
                               request.source_index,
                               "capture handoff evidence is host-gated for capture_kind=" +
                                   std::string(capture_kind_name(request.capture_kind)) + " on backend " +
                                   std::string(backend_name(desc.backend)));
            } else if (!handoff_ready) {
                add_diagnostic(plan, DebugProfilingDiagnosticCode::missing_trace_capture_handoff_evidence,
                               request_index, request.source_index,
                               "trace capture handoff review evidence is required for capture_kind=" +
                                   std::string(capture_kind_name(request.capture_kind)));
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
