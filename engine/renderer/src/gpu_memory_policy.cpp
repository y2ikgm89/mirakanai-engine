// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/gpu_memory_policy.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool is_supported_residency(GpuMemoryResidencyClass residency) noexcept {
    switch (residency) {
    case GpuMemoryResidencyClass::committed:
    case GpuMemoryResidencyClass::placed:
    case GpuMemoryResidencyClass::transient:
        return true;
    case GpuMemoryResidencyClass::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool is_supported_transient_heap(GpuMemoryTransientHeapPolicy policy) noexcept {
    return policy == GpuMemoryTransientHeapPolicy::none || policy == GpuMemoryTransientHeapPolicy::per_resource_heap ||
           policy == GpuMemoryTransientHeapPolicy::alias_group_heap;
}

[[nodiscard]] bool is_supported_upload_pressure(GpuMemoryUploadPressureKind kind) noexcept {
    return kind == GpuMemoryUploadPressureKind::none || kind == GpuMemoryUploadPressureKind::ring_buffer ||
           kind == GpuMemoryUploadPressureKind::staging_pool;
}

[[nodiscard]] bool transient_heap_required(GpuMemoryResidencyClass residency) noexcept {
    return residency == GpuMemoryResidencyClass::transient;
}

[[nodiscard]] std::string_view residency_name(GpuMemoryResidencyClass residency) noexcept {
    switch (residency) {
    case GpuMemoryResidencyClass::committed:
        return "committed";
    case GpuMemoryResidencyClass::placed:
        return "placed";
    case GpuMemoryResidencyClass::transient:
        return "transient";
    case GpuMemoryResidencyClass::unknown:
        return "unknown";
    }
    return "unknown";
}

[[nodiscard]] std::string_view transient_heap_name(GpuMemoryTransientHeapPolicy policy) noexcept {
    switch (policy) {
    case GpuMemoryTransientHeapPolicy::none:
        return "none";
    case GpuMemoryTransientHeapPolicy::per_resource_heap:
        return "per_resource_heap";
    case GpuMemoryTransientHeapPolicy::alias_group_heap:
        return "alias_group_heap";
    }
    return "unknown";
}

[[nodiscard]] std::string_view upload_pressure_name(GpuMemoryUploadPressureKind kind) noexcept {
    switch (kind) {
    case GpuMemoryUploadPressureKind::none:
        return "none";
    case GpuMemoryUploadPressureKind::ring_buffer:
        return "ring_buffer";
    case GpuMemoryUploadPressureKind::staging_pool:
        return "staging_pool";
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

void add_diagnostic(GpuMemoryPolicyPlan& plan, GpuMemoryDiagnosticCode code, std::size_t request_index,
                    std::uint32_t source_index, std::string message) {
    plan.diagnostics.push_back(GpuMemoryDiagnostic{
        .code = code,
        .request_index = request_index,
        .source_index = source_index,
        .message = std::move(message),
    });
}

[[nodiscard]] std::uint64_t counted_bytes_for_request(const GpuMemoryRequestDesc& request) noexcept {
    if (request.requested_bytes > 0) {
        return request.requested_bytes;
    }
    if (request.upload_pressure != GpuMemoryUploadPressureKind::none) {
        return 1;
    }
    return 0;
}

[[nodiscard]] bool memory_budget_evidence_ready(const GpuMemoryPolicyDesc& desc) noexcept {
    return desc.declared_local_budget_bytes > 0 || desc.declared_non_local_budget_bytes > 0;
}

[[nodiscard]] bool residency_pressure_evidence_ready(const GpuMemoryPolicyDesc& desc) noexcept {
    const auto os_usage_ready = desc.os_video_memory_budget_available &&
                                ((desc.os_local_budget_bytes > 0 && desc.os_local_usage_bytes > 0) ||
                                 (desc.os_non_local_budget_bytes > 0 && desc.os_non_local_usage_bytes > 0));
    return desc.residency_pressure_event_count > 0 || os_usage_ready || desc.transient_placed_resources_alive > 0;
}

} // namespace

GpuMemoryPolicyPlan plan_gpu_memory_policy(const GpuMemoryPolicyDesc& desc) {
    GpuMemoryPolicyPlan plan;
    plan.declared_local_budget_bytes = desc.declared_local_budget_bytes;
    plan.declared_non_local_budget_bytes = desc.declared_non_local_budget_bytes;
    plan.os_video_memory_budget_available = desc.os_video_memory_budget_available;
    plan.os_local_budget_bytes = desc.os_local_budget_bytes;
    plan.os_local_usage_bytes = desc.os_local_usage_bytes;
    plan.os_non_local_budget_bytes = desc.os_non_local_budget_bytes;
    plan.os_non_local_usage_bytes = desc.os_non_local_usage_bytes;
    plan.committed_byte_estimate = desc.committed_byte_estimate;
    plan.committed_byte_estimate_available = desc.committed_byte_estimate_available;
    plan.transient_heap_allocations = desc.transient_heap_allocations;
    plan.transient_placed_allocations = desc.transient_placed_allocations;
    plan.transient_placed_resources_alive = desc.transient_placed_resources_alive;
    plan.upload_bytes_written = desc.upload_bytes_written;
    plan.residency_pressure_event_count = desc.residency_pressure_event_count;
    plan.backend = desc.backend;
    plan.backend_memory_evidence_ready = desc.backend_memory_evidence_ready;
    plan.memory_budget_evidence_ready = memory_budget_evidence_ready(desc);
    plan.residency_pressure_evidence_ready = residency_pressure_evidence_ready(desc);
    plan.package_counter_evidence_ready = desc.package_counter_evidence_ready;

    if (desc.requests.empty()) {
        add_diagnostic(plan, GpuMemoryDiagnosticCode::no_memory_requests, 0, 0,
                       "gpu memory policy requires at least one memory request row");
    }
    if (desc.require_os_video_memory_budget && !desc.os_video_memory_budget_available) {
        add_diagnostic(plan, GpuMemoryDiagnosticCode::missing_os_video_memory_budget, 0, 0,
                       std::string{"gpu memory policy requires OS video memory budget for "} +
                           std::string{backend_name(desc.backend)});
    }
    if (desc.require_backend_memory_evidence && !desc.backend_memory_evidence_ready) {
        add_diagnostic(plan, GpuMemoryDiagnosticCode::missing_backend_memory_evidence, 0, 0,
                       std::string{"gpu memory policy requires backend memory evidence for "} +
                           std::string{backend_name(desc.backend)});
    }

    for (std::size_t index = 0; index < desc.requests.size(); ++index) {
        const auto& request = desc.requests[index];
        const auto source_index = request.source_index;

        if (!is_supported_residency(request.residency)) {
            add_diagnostic(plan, GpuMemoryDiagnosticCode::unsupported_residency_class, index, source_index,
                           std::string{"unsupported gpu memory residency class "} +
                               std::string{residency_name(request.residency)});
        }
        if (request.requested_bytes == 0 && request.upload_pressure == GpuMemoryUploadPressureKind::none) {
            add_diagnostic(plan, GpuMemoryDiagnosticCode::zero_byte_budget, index, source_index,
                           "gpu memory request requires a non-zero byte budget or upload-pressure row");
        }
        if (!request.scene_resources_available) {
            add_diagnostic(plan, GpuMemoryDiagnosticCode::missing_scene_resources, index, source_index,
                           "gpu memory request requires available scene resources");
        }
        if (!is_supported_transient_heap(request.transient_heap)) {
            add_diagnostic(plan, GpuMemoryDiagnosticCode::unsupported_transient_heap_policy, index, source_index,
                           std::string{"unsupported transient heap policy "} +
                               std::string{transient_heap_name(request.transient_heap)});
        }
        if (!is_supported_upload_pressure(request.upload_pressure)) {
            add_diagnostic(plan, GpuMemoryDiagnosticCode::unsupported_upload_pressure_kind, index, source_index,
                           std::string{"unsupported upload pressure kind "} +
                               std::string{upload_pressure_name(request.upload_pressure)});
        }
        if (transient_heap_required(request.residency) &&
            request.transient_heap == GpuMemoryTransientHeapPolicy::none) {
            add_diagnostic(plan, GpuMemoryDiagnosticCode::unsupported_transient_heap_policy, index, source_index,
                           "transient gpu memory requests require an explicit transient heap policy");
        }
        if (request.request_background_streaming) {
            add_diagnostic(plan, GpuMemoryDiagnosticCode::unsupported_background_streaming, index, source_index,
                           "background streaming is not supported by gpu memory policy v1");
        }
        if (request.request_automatic_eviction) {
            add_diagnostic(plan, GpuMemoryDiagnosticCode::unsupported_automatic_eviction, index, source_index,
                           "automatic eviction is not supported by gpu memory policy v1");
        }

        const auto counted_bytes = counted_bytes_for_request(request);
        const bool uses_transient_heap =
            request.transient_heap != GpuMemoryTransientHeapPolicy::none && transient_heap_required(request.residency);
        const bool uses_upload_pressure = request.upload_pressure != GpuMemoryUploadPressureKind::none;
        if (request.require_declared_budget_evidence) {
            ++plan.declared_budget_request_count;
            if (!plan.memory_budget_evidence_ready) {
                add_diagnostic(plan, GpuMemoryDiagnosticCode::missing_declared_budget_evidence, index, source_index,
                               "gpu memory policy requires declared local or non-local memory budget evidence");
            }
        }
        if (request.require_residency_pressure_evidence) {
            ++plan.residency_pressure_request_count;
            if (!plan.residency_pressure_evidence_ready) {
                add_diagnostic(plan, GpuMemoryDiagnosticCode::missing_residency_pressure_evidence, index, source_index,
                               "gpu memory policy requires residency pressure event, OS usage, or live placed "
                               "resource evidence");
            }
        }
        if (request.require_package_counter_evidence) {
            ++plan.package_counter_request_count;
            if (!plan.package_counter_evidence_ready) {
                add_diagnostic(plan, GpuMemoryDiagnosticCode::missing_package_counter_evidence, index, source_index,
                               "gpu memory policy requires package-visible renderer memory counters");
            }
        }

        plan.request_rows.push_back(GpuMemoryRequestRow{
            .residency = request.residency,
            .requested_bytes = request.requested_bytes,
            .counted_bytes = counted_bytes,
            .transient_heap = request.transient_heap,
            .upload_pressure = request.upload_pressure,
            .uses_transient_heap = uses_transient_heap,
            .uses_upload_pressure = uses_upload_pressure,
            .requires_declared_budget_evidence = request.require_declared_budget_evidence,
            .requires_residency_pressure_evidence = request.require_residency_pressure_evidence,
            .requires_package_counter_evidence = request.require_package_counter_evidence,
            .declared_budget_evidence_ready = plan.memory_budget_evidence_ready,
            .residency_pressure_evidence_ready = plan.residency_pressure_evidence_ready,
            .package_counter_evidence_ready = plan.package_counter_evidence_ready,
            .source_index = source_index,
        });

        ++plan.request_count;
        plan.total_requested_bytes += request.requested_bytes;
        plan.total_counted_bytes += counted_bytes;
        if (uses_transient_heap) {
            ++plan.transient_heap_request_count;
        }
        if (uses_upload_pressure) {
            ++plan.upload_pressure_request_count;
        }
    }

    if (desc.declared_local_budget_bytes > 0 && plan.total_counted_bytes > desc.declared_local_budget_bytes) {
        add_diagnostic(plan, GpuMemoryDiagnosticCode::exceeds_declared_budget, 0, 0,
                       "gpu memory policy exceeds declared_local_budget_bytes");
    }
    if (desc.declared_non_local_budget_bytes > 0 && plan.total_counted_bytes > desc.declared_non_local_budget_bytes) {
        add_diagnostic(plan, GpuMemoryDiagnosticCode::exceeds_declared_budget, 0, 0,
                       "gpu memory policy exceeds declared_non_local_budget_bytes");
    }

    return plan;
}

bool has_gpu_memory_policy_diagnostic(const GpuMemoryPolicyPlan& plan, GpuMemoryDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics,
                               [code](const GpuMemoryDiagnostic& diagnostic) { return diagnostic.code == code; });
}

bool gpu_memory_policy_backend_evidence_ready(const GpuMemoryBackendEvidenceDesc& desc) noexcept {
    const auto transient_pressure_ready =
        desc.transient_heap_allocations > 0 || desc.transient_placed_allocations > 0 ||
        desc.transient_placed_resources_alive > 0 || desc.framegraph_barrier_steps_executed > 0;
    const auto committed_ready = desc.committed_byte_estimate_available && desc.committed_resources_byte_estimate > 0;
    const auto upload_ready = desc.upload_bytes_written > 0;
    switch (desc.backend) {
    case rhi::BackendKind::d3d12:
        return committed_ready && upload_ready && (desc.os_video_memory_budget_available || transient_pressure_ready);
    case rhi::BackendKind::vulkan:
        return committed_ready && upload_ready && transient_pressure_ready;
    case rhi::BackendKind::null:
    case rhi::BackendKind::metal:
        return false;
    }
    return false;
}

} // namespace mirakana
