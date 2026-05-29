// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class GpuMemoryResidencyClass : std::uint8_t {
    unknown = 0,
    committed,
    placed,
    transient,
};

enum class GpuMemoryTransientHeapPolicy : std::uint8_t {
    none = 0,
    per_resource_heap,
    alias_group_heap,
};

enum class GpuMemoryUploadPressureKind : std::uint8_t {
    none = 0,
    ring_buffer,
    staging_pool,
};

enum class GpuMemoryDiagnosticCode : std::uint8_t {
    none = 0,
    no_memory_requests,
    invalid_byte_budget,
    zero_byte_budget,
    unsupported_residency_class,
    unsupported_transient_heap_policy,
    unsupported_upload_pressure_kind,
    unsupported_background_streaming,
    unsupported_automatic_eviction,
    missing_scene_resources,
    exceeds_declared_budget,
    missing_backend_memory_evidence,
    missing_os_video_memory_budget,
    missing_declared_budget_evidence,
    missing_residency_pressure_evidence,
    missing_package_counter_evidence,
};

struct GpuMemoryRequestDesc {
    GpuMemoryResidencyClass residency{GpuMemoryResidencyClass::unknown};
    std::uint64_t requested_bytes{0};
    GpuMemoryTransientHeapPolicy transient_heap{GpuMemoryTransientHeapPolicy::none};
    GpuMemoryUploadPressureKind upload_pressure{GpuMemoryUploadPressureKind::none};
    bool scene_resources_available{true};
    bool request_background_streaming{false};
    bool request_automatic_eviction{false};
    bool require_declared_budget_evidence{false};
    bool require_residency_pressure_evidence{false};
    bool require_package_counter_evidence{false};
    std::uint32_t source_index{0};
};

struct GpuMemoryPolicyDesc {
    std::span<const GpuMemoryRequestDesc> requests;
    std::uint64_t declared_local_budget_bytes{0};
    std::uint64_t declared_non_local_budget_bytes{0};
    bool os_video_memory_budget_available{false};
    std::uint64_t os_local_budget_bytes{0};
    std::uint64_t os_local_usage_bytes{0};
    std::uint64_t os_non_local_budget_bytes{0};
    std::uint64_t os_non_local_usage_bytes{0};
    bool committed_byte_estimate_available{false};
    std::uint64_t committed_byte_estimate{0};
    std::uint64_t transient_heap_allocations{0};
    std::uint64_t transient_placed_allocations{0};
    std::uint64_t transient_placed_resources_alive{0};
    std::uint64_t upload_bytes_written{0};
    std::uint64_t residency_pressure_event_count{0};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool require_backend_memory_evidence{false};
    bool backend_memory_evidence_ready{false};
    bool require_os_video_memory_budget{false};
    bool package_counter_evidence_ready{false};
};

struct GpuMemoryRequestRow {
    GpuMemoryResidencyClass residency{GpuMemoryResidencyClass::unknown};
    std::uint64_t requested_bytes{0};
    std::uint64_t counted_bytes{0};
    GpuMemoryTransientHeapPolicy transient_heap{GpuMemoryTransientHeapPolicy::none};
    GpuMemoryUploadPressureKind upload_pressure{GpuMemoryUploadPressureKind::none};
    bool uses_transient_heap{false};
    bool uses_upload_pressure{false};
    bool requires_declared_budget_evidence{false};
    bool requires_residency_pressure_evidence{false};
    bool requires_package_counter_evidence{false};
    bool declared_budget_evidence_ready{false};
    bool residency_pressure_evidence_ready{false};
    bool package_counter_evidence_ready{false};
    std::uint32_t source_index{0};
};

struct GpuMemoryDiagnostic {
    GpuMemoryDiagnosticCode code{GpuMemoryDiagnosticCode::none};
    std::size_t request_index{0};
    std::uint32_t source_index{0};
    std::string message;
};

struct GpuMemoryPolicyPlan {
    std::uint32_t request_count{0};
    std::uint64_t total_requested_bytes{0};
    std::uint64_t total_counted_bytes{0};
    std::uint64_t declared_local_budget_bytes{0};
    std::uint64_t declared_non_local_budget_bytes{0};
    std::uint64_t os_local_budget_bytes{0};
    std::uint64_t os_local_usage_bytes{0};
    std::uint64_t os_non_local_budget_bytes{0};
    std::uint64_t os_non_local_usage_bytes{0};
    std::uint64_t committed_byte_estimate{0};
    std::uint64_t transient_heap_allocations{0};
    std::uint64_t transient_placed_allocations{0};
    std::uint64_t transient_placed_resources_alive{0};
    std::uint64_t upload_bytes_written{0};
    std::uint64_t residency_pressure_event_count{0};
    std::uint32_t transient_heap_request_count{0};
    std::uint32_t upload_pressure_request_count{0};
    std::uint32_t declared_budget_request_count{0};
    std::uint32_t residency_pressure_request_count{0};
    std::uint32_t package_counter_request_count{0};
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool os_video_memory_budget_available{false};
    bool committed_byte_estimate_available{false};
    bool backend_memory_evidence_ready{false};
    bool memory_budget_evidence_ready{false};
    bool residency_pressure_evidence_ready{false};
    bool package_counter_evidence_ready{false};
    std::vector<GpuMemoryRequestRow> request_rows;
    std::vector<GpuMemoryDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] GpuMemoryPolicyPlan plan_gpu_memory_policy(const GpuMemoryPolicyDesc& desc);

[[nodiscard]] bool has_gpu_memory_policy_diagnostic(const GpuMemoryPolicyPlan& plan,
                                                    GpuMemoryDiagnosticCode code) noexcept;

struct GpuMemoryBackendEvidenceDesc {
    rhi::BackendKind backend{rhi::BackendKind::null};
    bool committed_byte_estimate_available{false};
    std::uint64_t committed_resources_byte_estimate{0};
    std::uint64_t upload_bytes_written{0};
    std::uint64_t transient_heap_allocations{0};
    std::uint64_t transient_placed_allocations{0};
    std::uint64_t transient_placed_resources_alive{0};
    std::uint64_t framegraph_barrier_steps_executed{0};
    bool os_video_memory_budget_available{false};
};

[[nodiscard]] bool gpu_memory_policy_backend_evidence_ready(const GpuMemoryBackendEvidenceDesc& desc) noexcept;

} // namespace mirakana
