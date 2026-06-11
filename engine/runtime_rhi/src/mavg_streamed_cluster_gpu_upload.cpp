// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_streamed_cluster_gpu_upload.hpp"

#include "mirakana/assets/asset_source_format.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgStreamedClusterGpuUploadResult& result,
                    RuntimeMavgStreamedClusterGpuUploadDiagnosticCode code, AssetId graph_asset, AssetId page_asset,
                    std::uint32_t page_index, std::uint32_t cluster_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgStreamedClusterGpuUploadDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_asset = page_asset,
        .page_index = page_index,
        .cluster_index = cluster_index,
        .message = std::move(message),
    });
}

[[nodiscard]] std::string first_mavg_validation_message(const MavgClusterGraphValidationResult& validation) {
    if (validation.diagnostics.empty() || validation.diagnostics.front().message.empty()) {
        return "MAVG cluster graph validation failed";
    }
    return validation.diagnostics.front().message;
}

[[nodiscard]] std::string runtime_upload_queue_wait_diagnostic(const RuntimeUploadQueueWaitResult& wait_result) {
    if (wait_result.diagnostics.empty() || wait_result.diagnostics.front().message.empty()) {
        return "runtime upload queue wait failed";
    }
    return wait_result.diagnostics.front().message;
}

[[nodiscard]] const RuntimeMavgStreamedClusterPagePayloadRow*
find_payload_row(std::span<const RuntimeMavgStreamedClusterPagePayloadRow> page_payloads, AssetId graph_asset,
                 std::uint32_t page_index) noexcept {
    const auto found = std::ranges::find_if(page_payloads, [graph_asset, page_index](const auto& row) {
        return row.graph_asset == graph_asset && row.page_index == page_index;
    });
    if (found == page_payloads.end()) {
        return nullptr;
    }
    return &*found;
}

[[nodiscard]] bool contains_page_index(std::span<const std::uint32_t> page_indices, std::uint32_t page_index) noexcept {
    return std::ranges::find(page_indices, page_index) != page_indices.end();
}

[[nodiscard]] std::uint32_t cluster_count_for_page(const MavgClusterGraphDocument& graph,
                                                   std::uint32_t page_index) noexcept {
    std::uint32_t count = 0;
    for (const auto& cluster : graph.clusters) {
        if (cluster.page_index == page_index) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] bool page_cluster_ranges_fit_payload(const MavgClusterGraphDocument& graph, std::uint32_t page_index,
                                                   const runtime::RuntimeMeshPayload& payload,
                                                   std::uint32_t& failing_cluster_index) noexcept {
    for (const auto& cluster : graph.clusters) {
        if (cluster.page_index != page_index) {
            continue;
        }

        const auto first_index = static_cast<std::uint64_t>(cluster.first_index);
        const auto index_count = static_cast<std::uint64_t>(cluster.index_count);
        if (first_index + index_count > payload.index_count) {
            failing_cluster_index = cluster.cluster_index;
            return false;
        }

        if (cluster.vertex_base < 0) {
            failing_cluster_index = cluster.cluster_index;
            return false;
        }
        const auto vertex_base = static_cast<std::uint64_t>(cluster.vertex_base);
        const auto vertex_count = static_cast<std::uint64_t>(cluster.vertex_count);
        if (vertex_base + vertex_count > payload.vertex_count) {
            failing_cluster_index = cluster.cluster_index;
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::uint64_t mesh_index_stride_for_upload(rhi::IndexFormat format) noexcept {
    switch (format) {
    case rhi::IndexFormat::uint16:
        return 2;
    case rhi::IndexFormat::uint32:
        return 4;
    case rhi::IndexFormat::unknown:
        break;
    }
    return 0;
}

[[nodiscard]] bool checked_byte_count(std::uint32_t count, std::uint64_t stride, std::uint64_t& bytes) noexcept {
    if (stride == 0 || count > (std::numeric_limits<std::uint64_t>::max)() / stride) {
        return false;
    }
    bytes = static_cast<std::uint64_t>(count) * stride;
    return true;
}

[[nodiscard]] bool mesh_payload_matches_upload_contract(const runtime::RuntimeMeshPayload& payload,
                                                        const RuntimeMeshUploadOptions& options) {
    const MeshSourceDocument document{
        .vertex_count = payload.vertex_count,
        .index_count = payload.index_count,
        .has_normals = payload.has_normals,
        .has_uvs = payload.has_uvs,
        .has_tangent_frame = payload.has_tangent_frame,
        .vertex_bytes = payload.vertex_bytes,
        .index_bytes = payload.index_bytes,
    };
    if (!is_valid_mesh_source_document(document) || payload.vertex_bytes.empty() || payload.index_bytes.empty()) {
        return false;
    }

    std::uint32_t vertex_stride = options.vertex_stride;
    if (options.derive_vertex_layout_from_payload) {
        const auto layout = make_runtime_mesh_vertex_layout_desc(payload);
        if (!layout.succeeded()) {
            return false;
        }
        vertex_stride = layout.vertex_stride;
    }

    std::uint64_t expected_vertex_size = 0;
    if (!checked_byte_count(payload.vertex_count, vertex_stride, expected_vertex_size) ||
        static_cast<std::uint64_t>(payload.vertex_bytes.size()) != expected_vertex_size) {
        return false;
    }

    std::uint64_t expected_index_size = 0;
    return checked_byte_count(payload.index_count, mesh_index_stride_for_upload(options.index_format),
                              expected_index_size) &&
           static_cast<std::uint64_t>(payload.index_bytes.size()) == expected_index_size;
}

void reset_publication(RuntimeMavgStreamedClusterGpuUploadResult& result) {
    result.page_bindings.clear();
    result.submitted_fences.clear();
    result.uploaded_page_count = 0;
    result.uploaded_cluster_count = 0;
    result.uploaded_bytes = 0;
    result.frame_graph_command_lists_submitted = 0;
    result.upload_queue_waits_recorded = 0;
    result.frame_graph_queue_waits_recorded = 0;
    result.frame_graph_barriers_recorded = 0;
    result.frame_graph_pass_callbacks_invoked = 0;
    result.package_visible = false;
    result.streamed_cluster_pages_ready = false;
}

void validate_payload_rows(RuntimeMavgStreamedClusterGpuUploadResult& result,
                           const RuntimeMavgStreamedClusterGpuUploadDesc& desc) {
    std::vector<std::uint32_t> seen_payload_pages;
    for (const auto& payload_row : desc.page_payloads) {
        if (payload_row.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::page_payload_mismatch,
                           payload_row.graph_asset, payload_row.payload.asset, payload_row.page_index, 0,
                           "MAVG streamed cluster page payload graph asset must match descriptor graph asset");
            continue;
        }
        if (contains_page_index(seen_payload_pages, payload_row.page_index)) {
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::duplicate_page_payload,
                           desc.graph_asset, payload_row.payload.asset, payload_row.page_index, 0,
                           "MAVG streamed cluster page payload rows must be unique by page");
            continue;
        }
        seen_payload_pages.push_back(payload_row.page_index);
    }
}

void validate_adopted_rows(RuntimeMavgStreamedClusterGpuUploadResult& result,
                           const RuntimeMavgStreamedClusterGpuUploadDesc& desc) {
    const auto& adoption = *desc.safe_point_adoption;
    for (const auto& adopted : adoption.adopted_rows) {
        if (adopted.graph_asset != desc.graph_asset) {
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::adopted_row_mismatch,
                           adopted.graph_asset, {}, adopted.page_index, 0,
                           "MAVG streamed cluster adoption rows must match descriptor graph asset");
            continue;
        }
        if (!adopted.resident_mount.succeeded()) {
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::adopted_row_mismatch,
                           desc.graph_asset, {}, adopted.page_index, 0,
                           "MAVG streamed cluster adoption rows must be resident-mounted before upload");
            continue;
        }

        const auto* const payload_row = find_payload_row(desc.page_payloads, desc.graph_asset, adopted.page_index);
        if (payload_row == nullptr) {
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::missing_page_payload,
                           desc.graph_asset, {}, adopted.page_index, 0,
                           "MAVG streamed cluster upload requires a payload row for every adopted page");
            continue;
        }

        const auto handle = runtime::find_runtime_resource_v2(*desc.resident_catalog, payload_row->payload.asset);
        if (!handle.has_value()) {
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::page_not_resident,
                           desc.graph_asset, payload_row->payload.asset, adopted.page_index, 0,
                           "MAVG streamed cluster page payload asset is not resident in the catalog");
            continue;
        }

        const auto* const record = runtime::runtime_resource_record_v2(*desc.resident_catalog, *handle);
        if (record == nullptr || record->kind != AssetKind::mesh) {
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::page_not_resident,
                           desc.graph_asset, payload_row->payload.asset, adopted.page_index, 0,
                           "MAVG streamed cluster page resident catalog row must be a mesh asset");
            continue;
        }
        if (payload_row->payload.handle != record->package_handle) {
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::payload_handle_mismatch,
                           desc.graph_asset, payload_row->payload.asset, adopted.page_index, 0,
                           "MAVG streamed cluster page payload handle must match the resident catalog row");
            continue;
        }
        std::uint32_t failing_cluster_index = 0;
        if (!page_cluster_ranges_fit_payload(*desc.graph, adopted.page_index, payload_row->payload,
                                             failing_cluster_index)) {
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::cluster_range_outside_payload,
                           desc.graph_asset, payload_row->payload.asset, adopted.page_index, failing_cluster_index,
                           "MAVG streamed cluster page cluster draw ranges must fit inside the page payload");
        }
        if (!mesh_payload_matches_upload_contract(payload_row->payload, desc.upload_options)) {
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::mesh_payload_invalid,
                           desc.graph_asset, payload_row->payload.asset, adopted.page_index, 0,
                           "MAVG streamed cluster page mesh payload must satisfy the runtime mesh upload contract");
        }
    }
}

void accumulate_upload_counters(RuntimeMavgStreamedClusterGpuUploadResult& result,
                                const RuntimeMeshUploadResult& upload) {
    result.uploaded_bytes += upload.uploaded_vertex_bytes + upload.uploaded_index_bytes;
    result.frame_graph_command_lists_submitted += upload.frame_graph_command_lists_submitted;
    result.frame_graph_queue_waits_recorded += upload.frame_graph_queue_waits_recorded;
    result.frame_graph_barriers_recorded += upload.frame_graph_barriers_recorded;
    result.frame_graph_pass_callbacks_invoked += upload.frame_graph_pass_callbacks_invoked;
    if (upload.submitted_fence.value != 0) {
        result.submitted_fences.push_back(upload.submitted_fence);
    }
}

} // namespace

RuntimeMavgStreamedClusterGpuUploadResult
upload_runtime_mavg_streamed_cluster_pages(rhi::IRhiDevice& device,
                                           const RuntimeMavgStreamedClusterGpuUploadDesc& desc) {
    RuntimeMavgStreamedClusterGpuUploadResult result;
    result.invoked_candidate_load = false;
    result.mutated_streaming_state = false;
    result.invoked_direct_storage = false;
    result.executed_backend = false;
    result.executed_mesh_shader = false;
    result.touched_native_handles = false;
    result.proved_async_overlap_performance = false;
    result.input_page_payload_count = desc.page_payloads.size();

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::invalid_graph_asset, desc.graph_asset,
                       {}, 0, 0, "MAVG streamed cluster upload graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::missing_graph, desc.graph_asset, {},
                       0, 0, "MAVG streamed cluster upload requires a graph document");
        invalid_inputs = true;
    }
    if (desc.safe_point_adoption == nullptr) {
        add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::missing_safe_point_adoption,
                       desc.graph_asset, {}, 0, 0,
                       "MAVG streamed cluster upload requires safe-point adoption evidence");
        invalid_inputs = true;
    } else {
        result.input_adopted_page_count = desc.safe_point_adoption->adopted_rows.size();
    }
    if (desc.resident_catalog == nullptr) {
        add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::missing_resident_catalog,
                       desc.graph_asset, {}, 0, 0,
                       "MAVG streamed cluster upload requires a resident runtime resource catalog");
        invalid_inputs = true;
    }
    if (desc.page_payloads.empty()) {
        add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::missing_page_payloads,
                       desc.graph_asset, {}, 0, 0,
                       "MAVG streamed cluster upload requires at least one page payload row");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    if (!desc.safe_point_adoption->succeeded()) {
        add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::adoption_not_committed,
                       desc.graph_asset, {}, 0, 0,
                       "MAVG streamed cluster upload requires committed safe-point adoption evidence");
        return result;
    }

    if (desc.graph->asset != desc.graph_asset) {
        add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::graph_asset_mismatch,
                       desc.graph_asset, {}, 0, 0,
                       "MAVG streamed cluster graph document asset must match descriptor graph asset");
    }
    const auto validation = validate_mavg_cluster_graph(*desc.graph);
    if (!validation.valid()) {
        add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::invalid_graph, desc.graph_asset, {},
                       0, 0, first_mavg_validation_message(validation));
    }

    validate_payload_rows(result, desc);
    validate_adopted_rows(result, desc);
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.page_bindings.reserve(desc.safe_point_adoption->adopted_rows.size());
    for (const auto& adopted : desc.safe_point_adoption->adopted_rows) {
        const auto* const payload_row = find_payload_row(desc.page_payloads, desc.graph_asset, adopted.page_index);
        result.invoked_gpu_upload = true;
        auto upload = upload_runtime_mesh(device, payload_row->payload, desc.upload_options);
        accumulate_upload_counters(result, upload);
        if (!upload.succeeded()) {
            reset_publication(result);
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::mesh_upload_failed,
                           desc.graph_asset, payload_row->payload.asset, adopted.page_index, 0, upload.diagnostic);
            return result;
        }

        if (!desc.upload_options.wait_for_completion && upload.submitted_fence.value != 0) {
            const std::vector<rhi::FenceValue> upload_fences{upload.submitted_fence};
            const auto queue_wait = wait_for_runtime_uploads_on_queue(device, rhi::QueueKind::graphics, upload_fences);
            result.upload_queue_waits_recorded += queue_wait.queue_waits_recorded;
            if (!queue_wait.succeeded()) {
                reset_publication(result);
                add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::upload_queue_wait_failed,
                               desc.graph_asset, payload_row->payload.asset, adopted.page_index, 0,
                               runtime_upload_queue_wait_diagnostic(queue_wait));
                return result;
            }
        }

        auto binding = make_runtime_mesh_gpu_binding(upload);
        if (binding.owner_device == nullptr) {
            reset_publication(result);
            add_diagnostic(result, RuntimeMavgStreamedClusterGpuUploadDiagnosticCode::mesh_binding_empty,
                           desc.graph_asset, payload_row->payload.asset, adopted.page_index, 0,
                           "MAVG streamed cluster page upload did not produce a mesh binding");
            return result;
        }

        const auto uploaded_cluster_count = cluster_count_for_page(*desc.graph, adopted.page_index);
        result.uploaded_cluster_count += uploaded_cluster_count;
        result.page_bindings.push_back(RuntimeMavgStreamedClusterPageBindingRow{
            .graph_asset = desc.graph_asset,
            .page_asset = payload_row->payload.asset,
            .page_index = adopted.page_index,
            .uploaded_cluster_count = uploaded_cluster_count,
            .uploaded_bytes = upload.uploaded_vertex_bytes + upload.uploaded_index_bytes,
            .upload = std::move(upload),
            .binding = binding,
        });
    }

    result.uploaded_page_count = result.page_bindings.size();
    result.package_visible = true;
    result.streamed_cluster_pages_ready = true;
    return result;
}

bool has_runtime_mavg_streamed_cluster_gpu_upload_diagnostic(
    const RuntimeMavgStreamedClusterGpuUploadResult& result,
    RuntimeMavgStreamedClusterGpuUploadDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana::runtime_rhi
