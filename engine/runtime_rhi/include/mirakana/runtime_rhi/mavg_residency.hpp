// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/mavg_page_streaming.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgPageResidencyActionDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    missing_graph,
    invalid_graph,
    invalid_resident_page_resource,
    duplicate_resident_page_resource,
    missing_resident_page_resource,
    invalid_selected_cluster,
    invalid_protected_mount,
    duplicate_protected_mount,
    invalid_eviction_candidate,
    duplicate_eviction_candidate,
    rhi_make_resident_failed,
    rhi_evict_failed,
};

struct RuntimeMavgPageResidencyActionDiagnostic {
    RuntimeMavgPageResidencyActionDiagnosticCode code{
        RuntimeMavgPageResidencyActionDiagnosticCode::invalid_graph_asset};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    std::string message;
};

struct RuntimeMavgResidentPageResourceRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    rhi::RhiResidencyResourceRef resource;
    std::uint64_t estimated_gpu_resident_bytes{0};
};

struct RuntimeMavgPageResidencyActionRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    rhi::RhiResidencyActionKind action{rhi::RhiResidencyActionKind::make_resident};
    rhi::RhiResidencyResourceRef resource;
    bool selected_page{false};
    bool protected_page{false};
    bool reviewed_eviction_candidate{false};
    bool skipped_protected_eviction_candidate{false};
};

struct RuntimeMavgPageResidencyActionDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    std::span<const runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters;
    std::span<const RuntimeMavgResidentPageResourceRow> resident_page_resources;
    std::span<const runtime::RuntimeResidentPackageMountIdV2> protected_mount_ids;
    std::span<const runtime::RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order;
    bool make_selected_pages_resident{true};
    bool evict_reviewed_candidates{true};
};

struct RuntimeMavgPageResidencyActionResult {
    std::vector<RuntimeMavgPageResidencyActionDiagnostic> diagnostics;
    std::vector<RuntimeMavgPageResidencyActionRow> action_rows;
    rhi::RhiResidencyActionResult make_resident_result;
    rhi::RhiResidencyActionResult evict_result;
    std::size_t input_resident_page_resource_count{0};
    std::size_t selected_page_resource_count{0};
    std::size_t protected_page_resource_count{0};
    std::size_t eviction_candidate_resource_count{0};
    std::size_t made_resident_count{0};
    std::size_t evicted_count{0};
    std::size_t protected_skip_count{0};
    bool invoked_rhi_residency_action{false};
    bool invoked_make_resident_action{false};
    bool invoked_evict_action{false};
    bool invoked_native_make_resident{false};
    bool invoked_native_evict{false};
    bool exposed_native_handles{false};
    bool enforced_allocator_budget{false};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool used_directstorage_resource_destination{false};
    bool used_gpu_decompression{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimeMavgPageResidencyActionResult
execute_runtime_mavg_page_residency_actions(rhi::IRhiDevice& device, const RuntimeMavgPageResidencyActionDesc& desc);

} // namespace mirakana::runtime_rhi
