// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime_rhi/mavg_page_gpu_resource_update.hpp"
#include "mirakana/runtime_rhi/mavg_residency.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    missing_graph,
    invalid_graph,
    missing_readiness_result,
    invalid_readiness_result,
    readiness_not_ready,
    residency_action_failed,
};

struct RuntimeMavgPageGpuResourceResidencyExecutionDiagnostic {
    RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode code{
        RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::invalid_graph_asset};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    std::string message;
};

struct RuntimeMavgPageGpuResourceResidencyExecutionDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    const RuntimeMavgPageGpuResourceUpdateReadinessResult* readiness_result{nullptr};
    std::span<const runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters;
    std::span<const runtime::RuntimeResidentPackageMountIdV2> protected_mount_ids;
    std::span<const runtime::RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order;
    bool make_selected_pages_resident{true};
    bool evict_reviewed_candidates{true};
};

struct RuntimeMavgPageGpuResourceResidencyExecutionResult {
    std::vector<RuntimeMavgPageGpuResourceResidencyExecutionDiagnostic> diagnostics;
    RuntimeMavgPageResidencyActionResult residency_action_result;
    std::size_t input_ready_resource_count{0};
    std::size_t selected_page_resource_count{0};
    std::size_t protected_page_resource_count{0};
    std::size_t eviction_candidate_resource_count{0};
    std::size_t made_resident_count{0};
    std::size_t evicted_count{0};
    std::size_t protected_skip_count{0};
    bool consumed_gpu_resource_update_readiness{false};
    bool used_directstorage_resource_destination{false};
    bool used_directstorage_caller_owned_rhi_resource_destination{false};
    bool directstorage_status_complete{false};
    bool observed_native_queue_submission{false};
    bool invoked_rhi_residency_action{false};
    bool invoked_make_resident_action{false};
    bool invoked_evict_action{false};
    bool invoked_native_make_resident{false};
    bool invoked_native_evict{false};
    bool invoked_file_io{false};
    bool submitted_native_queue{false};
    bool allocated_rhi_resources{false};
    bool enforced_allocator_budget{false};
    bool mutated_mount_set{false};
    bool used_gpu_decompression{false};
    bool exposed_native_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && residency_action_result.succeeded();
    }
};

[[nodiscard]] RuntimeMavgPageGpuResourceResidencyExecutionResult
execute_runtime_mavg_page_gpu_resource_residency_actions(rhi::IRhiDevice& device,
                                                         const RuntimeMavgPageGpuResourceResidencyExecutionDesc& desc);

} // namespace mirakana::runtime_rhi
