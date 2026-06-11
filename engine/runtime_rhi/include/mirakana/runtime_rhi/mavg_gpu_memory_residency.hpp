// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/renderer/gpu_memory_policy.hpp"
#include "mirakana/runtime/mavg_page_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgGpuMemoryResidencyDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_graph_asset,
    missing_graph,
    missing_gpu_memory_policy,
    gpu_memory_policy_failed,
    missing_memory_budget_evidence,
    missing_residency_pressure_evidence,
    missing_package_counter_evidence,
    invalid_target_budget,
    eviction_plan_failed,
};

struct RuntimeMavgGpuMemoryResidencyDiagnostic {
    RuntimeMavgGpuMemoryResidencyDiagnosticCode code{RuntimeMavgGpuMemoryResidencyDiagnosticCode::none};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    std::string message;
};

struct RuntimeMavgGpuMemoryResidencyDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    const GpuMemoryPolicyPlan* gpu_memory_policy{nullptr};
    std::span<const runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters;
    std::span<const runtime::RuntimeMavgResidentPageMountRow> resident_page_mounts;
    std::span<const runtime::RuntimeMavgPageStreamingRecencyRow> recency_rows;
    std::span<const runtime::RuntimeResidentPackageMountIdV2> caller_protected_mount_ids;
    runtime::RuntimePackageMountOverlay overlay{runtime::RuntimePackageMountOverlay::last_mount_wins};
    runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind policy_kind{
        runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency};
};

struct RuntimeMavgGpuMemoryResidencyResult {
    runtime::RuntimeResourceResidencyBudgetV2 target_budget{};
    runtime::RuntimeMavgPageStreamingEvictionReviewResult eviction_review;
    std::vector<RuntimeMavgGpuMemoryResidencyDiagnostic> diagnostics;
    std::uint64_t target_resident_content_bytes{0};
    std::size_t protected_mount_count{0};
    std::size_t eviction_candidate_count{0};
    std::size_t evicted_mount_count{0};
    bool applied_gpu_memory_pressure_policy{false};
    bool invoked_eviction_plan{false};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool invoked_catalog_refresh{false};
    bool touched_renderer_or_rhi_handles{false};
    bool invoked_direct_storage{false};
    bool proved_async_overlap_performance{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && eviction_review.succeeded() && applied_gpu_memory_pressure_policy;
    }
};

/// Converts a validated renderer GPU memory pressure plan into a MAVG resident package byte budget, then delegates to
/// the runtime MAVG automatic eviction planner. This is a value-only planning bridge: it does not mutate mounts,
/// refresh catalogs, read files, execute DirectStorage, prove async overlap/performance, or touch renderer/RHI/native
/// handles.
[[nodiscard]] RuntimeMavgGpuMemoryResidencyResult
plan_runtime_mavg_gpu_memory_pressure_residency(const runtime::RuntimeResidentPackageMountSetV2& mount_set,
                                                const RuntimeMavgGpuMemoryResidencyDesc& desc);

[[nodiscard]] bool
has_runtime_mavg_gpu_memory_residency_diagnostic(const RuntimeMavgGpuMemoryResidencyResult& result,
                                                 RuntimeMavgGpuMemoryResidencyDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
