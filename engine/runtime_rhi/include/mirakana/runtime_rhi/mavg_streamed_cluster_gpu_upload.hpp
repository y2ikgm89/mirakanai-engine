// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_rhi/mavg_cluster_streaming_safe_point_adoption.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgStreamedClusterGpuUploadDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_graph_asset,
    missing_graph,
    missing_safe_point_adoption,
    adoption_not_committed,
    missing_resident_catalog,
    missing_page_payloads,
    invalid_graph,
    graph_asset_mismatch,
    adopted_row_mismatch,
    missing_page_payload,
    duplicate_page_payload,
    page_payload_mismatch,
    payload_handle_mismatch,
    mesh_payload_invalid,
    page_not_resident,
    cluster_range_outside_payload,
    mesh_upload_failed,
    mesh_binding_empty,
    upload_queue_wait_failed,
};

struct RuntimeMavgStreamedClusterGpuUploadDiagnostic {
    RuntimeMavgStreamedClusterGpuUploadDiagnosticCode code{RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::none};
    AssetId graph_asset;
    AssetId page_asset;
    std::uint32_t page_index{0};
    std::uint32_t cluster_index{0};
    std::string message;
};

struct RuntimeMavgStreamedClusterPagePayloadRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeMeshPayload payload;
};

struct RuntimeMavgStreamedClusterPageBindingRow {
    AssetId graph_asset;
    AssetId page_asset;
    std::uint32_t page_index{0};
    std::uint32_t uploaded_cluster_count{0};
    std::uint64_t uploaded_bytes{0};
    RuntimeMeshUploadResult upload;
    MeshGpuBinding binding;
};

struct RuntimeMavgStreamedClusterGpuUploadDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    const RuntimeMavgClusterStreamingSafePointAdoptionResult* safe_point_adoption{nullptr};
    const runtime::RuntimeResourceCatalogV2* resident_catalog{nullptr};
    std::span<const RuntimeMavgStreamedClusterPagePayloadRow> page_payloads;
    RuntimeMeshUploadOptions upload_options{};
};

struct RuntimeMavgStreamedClusterGpuUploadResult {
    std::vector<RuntimeMavgStreamedClusterPageBindingRow> page_bindings;
    std::vector<RuntimeMavgStreamedClusterGpuUploadDiagnostic> diagnostics;
    std::vector<rhi::FenceValue> submitted_fences;
    std::size_t input_adopted_page_count{0};
    std::size_t input_page_payload_count{0};
    std::size_t uploaded_page_count{0};
    std::size_t uploaded_cluster_count{0};
    std::uint64_t uploaded_bytes{0};
    std::size_t frame_graph_command_lists_submitted{0};
    std::size_t upload_queue_waits_recorded{0};
    std::size_t frame_graph_queue_waits_recorded{0};
    std::size_t frame_graph_barriers_recorded{0};
    std::size_t frame_graph_pass_callbacks_invoked{0};
    bool package_visible{false};
    bool streamed_cluster_pages_ready{false};
    bool invoked_gpu_upload{false};
    bool invoked_candidate_load{false};
    bool mutated_streaming_state{false};
    bool invoked_direct_storage{false};
    bool executed_backend{false};
    bool executed_mesh_shader{false};
    bool touched_native_handles{false};
    bool proved_async_overlap_performance{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && package_visible && streamed_cluster_pages_ready;
    }
};

/// Uploads caller-provided MAVG page mesh payloads that are already committed through safe-point adoption. This helper
/// does not reload candidates, mutate resident mounts/catalogs, execute DirectStorage, execute backend draws, expose
/// native handles, run mesh shaders, or prove async-overlap/performance.
[[nodiscard]] RuntimeMavgStreamedClusterGpuUploadResult
upload_runtime_mavg_streamed_cluster_pages(rhi::IRhiDevice& device,
                                           const RuntimeMavgStreamedClusterGpuUploadDesc& desc);

[[nodiscard]] bool has_runtime_mavg_streamed_cluster_gpu_upload_diagnostic(
    const RuntimeMavgStreamedClusterGpuUploadResult& result,
    RuntimeMavgStreamedClusterGpuUploadDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
