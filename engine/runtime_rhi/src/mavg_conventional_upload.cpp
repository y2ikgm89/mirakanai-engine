// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_conventional_upload.hpp"

#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgConventionalMeshUploadResult& result, AssetId asset, std::uint32_t cluster_index,
                    std::string code, std::string message) {
    result.diagnostics.push_back(RuntimeMavgConventionalMeshUploadDiagnostic{
        .asset = asset,
        .cluster_index = cluster_index,
        .code = std::move(code),
        .message = std::move(message),
    });
}

[[nodiscard]] std::string first_mavg_validation_message(const MavgClusterGraphValidationResult& validation) {
    if (validation.diagnostics.empty()) {
        return "MAVG cluster graph validation failed";
    }
    return validation.diagnostics.front().message.empty() ? "MAVG cluster graph validation failed"
                                                          : validation.diagnostics.front().message;
}

[[nodiscard]] std::string runtime_upload_queue_wait_diagnostic(const RuntimeUploadQueueWaitResult& wait_result) {
    if (wait_result.diagnostics.empty()) {
        return "runtime upload queue wait failed";
    }
    if (wait_result.diagnostics.front().message.empty()) {
        return "runtime upload queue wait failed";
    }
    return wait_result.diagnostics.front().message;
}

[[nodiscard]] bool cluster_range_within_payload(const MavgClusterGraphCluster& cluster,
                                                const runtime::RuntimeMeshPayload& payload) noexcept {
    const auto first_index = static_cast<std::uint64_t>(cluster.first_index);
    const auto index_count = static_cast<std::uint64_t>(cluster.index_count);
    if (first_index + index_count > payload.index_count) {
        return false;
    }

    if (cluster.vertex_base < 0) {
        return false;
    }
    const auto vertex_base = static_cast<std::uint64_t>(cluster.vertex_base);
    const auto vertex_count = static_cast<std::uint64_t>(cluster.vertex_count);
    return vertex_base + vertex_count <= payload.vertex_count;
}

[[nodiscard]] const runtime::RuntimeResourceRecordV2*
find_live_mavg_graph_record(const runtime::RuntimeResourceCatalogV2& catalog, AssetId graph_asset,
                            RuntimeMavgConventionalMeshUploadResult& result) {
    const auto handle = runtime::find_runtime_resource_v2(catalog, graph_asset);
    if (!handle.has_value()) {
        add_diagnostic(result, graph_asset, 0, "runtime-resource-not-live",
                       "MAVG cluster graph asset is not live in the resident runtime resource catalog");
        return nullptr;
    }

    const auto* const record = runtime::runtime_resource_record_v2(catalog, *handle);
    if (record == nullptr) {
        add_diagnostic(result, graph_asset, 0, "runtime-resource-not-live",
                       "MAVG cluster graph catalog handle is stale");
        return nullptr;
    }

    if (record->kind != AssetKind::mavg_cluster_graph) {
        add_diagnostic(result, graph_asset, 0, "runtime-resource-not-mavg-cluster-graph",
                       "resident runtime resource is not a MAVG cluster graph asset");
        return nullptr;
    }
    return record;
}

} // namespace

RuntimeMavgConventionalMeshUploadResult
upload_runtime_mavg_conventional_mesh_binding(rhi::IRhiDevice& device,
                                              const RuntimeMavgConventionalMeshUploadDesc& desc) {
    RuntimeMavgConventionalMeshUploadResult result;
    result.mesh_binding.graph_asset = desc.graph_asset;

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, desc.graph_asset, 0, "invalid-graph-asset", "MAVG graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, desc.graph_asset, 0, "missing-graph", "MAVG cluster graph document is required");
        invalid_inputs = true;
    }
    if (desc.streaming_result == nullptr) {
        add_diagnostic(result, desc.graph_asset, 0, "missing-streaming-result",
                       "runtime package streaming result is required");
        invalid_inputs = true;
    }
    if (desc.resident_catalog == nullptr) {
        add_diagnostic(result, desc.graph_asset, 0, "missing-resident-catalog",
                       "resident runtime resource catalog is required");
        invalid_inputs = true;
    }
    if (desc.payload == nullptr) {
        add_diagnostic(result, desc.graph_asset, 0, "missing-mavg-payload", "MAVG runtime mesh payload is required");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto& graph = *desc.graph;
    const auto& streaming_result = *desc.streaming_result;
    const auto& catalog = *desc.resident_catalog;
    const auto& payload = *desc.payload;
    result.graph_page_count = graph.pages.size();
    result.graph_cluster_count = graph.clusters.size();
    result.payload_vertex_count = payload.vertex_count;
    result.payload_index_count = payload.index_count;

    if (!streaming_result.succeeded()) {
        add_diagnostic(result, desc.graph_asset, 0, "package-streaming-not-committed",
                       "package streaming result must be committed before uploading MAVG conventional mesh bindings");
        return result;
    }
    if (graph.asset != desc.graph_asset) {
        add_diagnostic(result, desc.graph_asset, 0, "mavg-graph-asset-mismatch",
                       "MAVG graph document asset must match the requested graph asset");
    }
    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        add_diagnostic(result, desc.graph_asset, 0, "invalid-mavg-graph", first_mavg_validation_message(validation));
    }

    const auto* const record = find_live_mavg_graph_record(catalog, desc.graph_asset, result);
    if (payload.asset != desc.graph_asset) {
        add_diagnostic(result, desc.graph_asset, 0, "mavg-payload-asset-mismatch",
                       "MAVG runtime mesh payload asset must match the selected graph asset");
    }
    if (record != nullptr && payload.handle != record->package_handle) {
        add_diagnostic(result, desc.graph_asset, 0, "mavg-payload-handle-mismatch",
                       "MAVG runtime mesh payload handle must match the selected resident graph handle");
    }
    for (const auto& cluster : graph.clusters) {
        if (!cluster_range_within_payload(cluster, payload)) {
            add_diagnostic(result, desc.graph_asset, cluster.cluster_index, "mavg-draw-range-outside-payload",
                           "MAVG cluster draw range must fit inside the runtime mesh payload");
            break;
        }
    }
    if (!result.succeeded()) {
        return result;
    }

    result.upload = upload_runtime_mesh(device, payload, desc.upload_options);
    result.uploaded_bytes = result.upload.uploaded_vertex_bytes + result.upload.uploaded_index_bytes;
    result.frame_graph_command_lists_submitted = result.upload.frame_graph_command_lists_submitted;
    result.frame_graph_queue_waits_recorded = result.upload.frame_graph_queue_waits_recorded;
    result.frame_graph_barriers_recorded = result.upload.frame_graph_barriers_recorded;
    result.frame_graph_pass_callbacks_invoked = result.upload.frame_graph_pass_callbacks_invoked;
    if (result.upload.submitted_fence.value != 0) {
        result.submitted_fences.push_back(result.upload.submitted_fence);
    }
    if (!result.upload.succeeded()) {
        add_diagnostic(result, desc.graph_asset, 0, "mavg-mesh-upload-failed", result.upload.diagnostic);
        return result;
    }

    if (!desc.upload_options.wait_for_completion && result.upload.submitted_fence.value != 0) {
        const std::vector<rhi::FenceValue> upload_fences{result.upload.submitted_fence};
        const auto queue_wait = wait_for_runtime_uploads_on_queue(device, rhi::QueueKind::graphics, upload_fences);
        result.upload_queue_waits_recorded += queue_wait.queue_waits_recorded;
        if (!queue_wait.succeeded()) {
            add_diagnostic(result, desc.graph_asset, 0, "mavg-upload-queue-wait-failed",
                           runtime_upload_queue_wait_diagnostic(queue_wait));
            return result;
        }
    }

    result.mesh_binding = RuntimeMavgConventionalMeshBinding{
        .graph_asset = desc.graph_asset,
        .binding = make_runtime_mesh_gpu_binding(result.upload),
    };
    if (result.mesh_binding.binding.owner_device == nullptr) {
        add_diagnostic(result, desc.graph_asset, 0, "mavg-mesh-binding-empty",
                       "MAVG runtime mesh upload did not produce a renderer mesh binding");
        return result;
    }

    result.package_visible = true;
    result.conventional_mesh_ready = true;
    return result;
}

} // namespace mirakana::runtime_rhi
