// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

struct RuntimeMavgConventionalMeshUploadDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    const runtime::RuntimePackageStreamingExecutionResult* streaming_result{nullptr};
    const runtime::RuntimeResourceCatalogV2* resident_catalog{nullptr};
    const runtime::RuntimeMeshPayload* payload{nullptr};
    RuntimeMeshUploadOptions upload_options{};
};

struct RuntimeMavgConventionalMeshBinding {
    AssetId graph_asset;
    MeshGpuBinding binding;
};

struct RuntimeMavgConventionalMeshUploadDiagnostic {
    AssetId asset;
    std::uint32_t cluster_index{0};
    std::string code;
    std::string message;
};

struct RuntimeMavgConventionalMeshUploadResult {
    RuntimeMeshUploadResult upload;
    RuntimeMavgConventionalMeshBinding mesh_binding;
    std::vector<RuntimeMavgConventionalMeshUploadDiagnostic> diagnostics;
    std::vector<rhi::FenceValue> submitted_fences;
    std::uint64_t uploaded_bytes{0};
    std::size_t frame_graph_command_lists_submitted{0};
    std::size_t upload_queue_waits_recorded{0};
    std::size_t frame_graph_queue_waits_recorded{0};
    std::size_t frame_graph_barriers_recorded{0};
    std::size_t frame_graph_pass_callbacks_invoked{0};
    std::size_t graph_page_count{0};
    std::size_t graph_cluster_count{0};
    std::uint32_t payload_vertex_count{0};
    std::uint32_t payload_index_count{0};
    bool package_visible{false};
    bool conventional_mesh_ready{false};
    bool executed_gpu_culling{false};
    bool executed_indirect_draw{false};
    bool executed_mesh_shader{false};
    bool touched_native_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimeMavgConventionalMeshUploadResult
upload_runtime_mavg_conventional_mesh_binding(rhi::IRhiDevice& device,
                                              const RuntimeMavgConventionalMeshUploadDesc& desc);

} // namespace mirakana::runtime_rhi
