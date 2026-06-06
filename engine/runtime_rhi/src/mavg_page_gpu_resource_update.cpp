// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_page_gpu_resource_update.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgPageGpuResourceUpdateReadinessResult& result,
                    RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode code, AssetId graph_asset,
                    std::uint32_t page_index, runtime::RuntimeResidentPackageMountIdV2 mount_id, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPageGpuResourceUpdateReadinessDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .mount_id = mount_id,
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_page_index(const std::vector<std::uint32_t>& page_indices,
                                       std::uint32_t page_index) noexcept {
    return std::ranges::find(page_indices, page_index) != page_indices.end();
}

[[nodiscard]] bool contains_mount_id(const std::vector<runtime::RuntimeResidentPackageMountIdV2>& mount_ids,
                                     runtime::RuntimeResidentPackageMountIdV2 mount_id) noexcept {
    return std::ranges::find(mount_ids, mount_id) != mount_ids.end();
}

[[nodiscard]] bool destination_row_shape_valid(const RuntimeMavgPageBufferDestinationPlanRow& row,
                                               AssetId graph_asset) noexcept {
    return row.graph_asset == graph_asset && row.mount_id.value != 0 && row.buffer.value != 0 &&
           row.destination_size != 0 && row.destination_range_size != 0 && row.estimated_gpu_resident_bytes != 0;
}

void append_ready_row(RuntimeMavgPageGpuResourceUpdateReadinessResult& result,
                      const RuntimeMavgPageBufferDestinationPlanRow& row) {
    result.resident_page_resources.push_back(RuntimeMavgResidentPageResourceRow{
        .graph_asset = row.graph_asset,
        .page_index = row.page_index,
        .mount_id = row.mount_id,
        .resource = {.kind = rhi::RhiResidencyResourceKind::buffer, .buffer = row.buffer},
        .estimated_gpu_resident_bytes = row.estimated_gpu_resident_bytes,
    });
    result.update_rows.push_back(RuntimeMavgPageGpuResourceUpdateRow{
        .graph_asset = row.graph_asset,
        .request_index = row.request_index,
        .page_index = row.page_index,
        .mount_id = row.mount_id,
        .buffer = row.buffer,
        .destination_offset = row.destination_offset,
        .destination_size = row.destination_size,
        .destination_range_offset = row.destination_range_offset,
        .destination_range_size = row.destination_range_size,
        .estimated_gpu_resident_bytes = row.estimated_gpu_resident_bytes,
        .directstorage_resource_destination_complete = true,
        .ready_for_residency_action = true,
    });
}

} // namespace

RuntimeMavgPageGpuResourceUpdateReadinessResult
make_runtime_mavg_page_gpu_resource_update_readiness(const RuntimeMavgPageGpuResourceUpdateReadinessDesc& desc) {
    RuntimeMavgPageGpuResourceUpdateReadinessResult result;

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::invalid_graph_asset,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource update readiness graph asset id must be non-zero");
        invalid_inputs = true;
    }

    const auto* const plan = desc.buffer_destination_plan;
    if (plan == nullptr) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::missing_buffer_destination_plan,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource update readiness requires a buffer destination plan");
        invalid_inputs = true;
    } else {
        result.input_destination_row_count = plan->destination_rows.size();
        if (!plan->succeeded() || plan->planned_destination_count != plan->destination_rows.size()) {
            add_diagnostic(result,
                           RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::invalid_buffer_destination_plan,
                           desc.graph_asset, 0, {},
                           "MAVG page GPU resource update readiness requires a successful buffer destination plan");
            invalid_inputs = true;
        }
    }

    const auto* const dispatch = desc.dispatch_result;
    if (dispatch == nullptr) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::missing_dispatch_result,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource update readiness requires native IO dispatch evidence");
        invalid_inputs = true;
    } else if (!dispatch->succeeded() || !dispatch->enqueued_native_requests || !dispatch->submitted_native_queue) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::invalid_dispatch_result,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource update readiness requires successful DirectStorage dispatch evidence");
        invalid_inputs = true;
    }

    const auto* const status = desc.status_result;
    if (status == nullptr) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::missing_status_result,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource update readiness requires native IO status evidence");
        invalid_inputs = true;
    } else if (status->failed || status->status == runtime::RuntimeMavgPayloadNativeIoStatus::failed ||
               !status->diagnostics.empty()) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::failed_status_result,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource update readiness requires non-failed DirectStorage status evidence");
        invalid_inputs = true;
    } else if (!status->complete || status->status != runtime::RuntimeMavgPayloadNativeIoStatus::complete) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::incomplete_status_result,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource update readiness requires complete DirectStorage status evidence");
        invalid_inputs = true;
    }

    if (dispatch != nullptr && status != nullptr && dispatch->ticket != status->ticket) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::ticket_mismatch,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource update readiness dispatch and status tickets must match");
        invalid_inputs = true;
    }

    if (plan != nullptr && dispatch != nullptr) {
        if (dispatch->request_count != plan->planned_destination_count ||
            dispatch->directstorage_resource_destination_request_count != plan->planned_destination_count) {
            add_diagnostic(result,
                           RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::submitted_request_count_mismatch,
                           desc.graph_asset, 0, {},
                           "MAVG page GPU resource update readiness dispatch request counts must match the plan");
            invalid_inputs = true;
        }
        if (dispatch->total_destination_bytes != plan->total_destination_bytes ||
            dispatch->directstorage_resource_destination_bytes != plan->total_destination_bytes) {
            add_diagnostic(
                result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::submitted_destination_bytes_mismatch,
                desc.graph_asset, 0, {},
                "MAVG page GPU resource update readiness dispatch destination bytes must match the plan");
            invalid_inputs = true;
        }
    }

    if (plan != nullptr && status != nullptr) {
        if (status->directstorage_resource_destination_request_count != plan->planned_destination_count) {
            add_diagnostic(result,
                           RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::submitted_request_count_mismatch,
                           desc.graph_asset, 0, {},
                           "MAVG page GPU resource update readiness status request count must match the plan");
            invalid_inputs = true;
        }
        if (status->directstorage_resource_destination_bytes != plan->total_destination_bytes) {
            add_diagnostic(result,
                           RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::status_destination_bytes_mismatch,
                           desc.graph_asset, 0, {},
                           "MAVG page GPU resource update readiness status destination bytes must match the plan");
            invalid_inputs = true;
        }
    }

    if (dispatch != nullptr && status != nullptr &&
        (!dispatch->used_directstorage_resource_destination ||
         !dispatch->used_directstorage_caller_owned_rhi_resource_destination ||
         !status->used_directstorage_resource_destination ||
         !status->used_directstorage_caller_owned_rhi_resource_destination)) {
        add_diagnostic(result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::resource_destination_not_used,
                       desc.graph_asset, 0, {},
                       "MAVG page GPU resource update readiness requires caller-owned RHI DirectStorage destination "
                       "evidence from dispatch and status");
        invalid_inputs = true;
    }

    std::vector<std::uint32_t> page_indices;
    std::vector<runtime::RuntimeResidentPackageMountIdV2> mount_ids;
    if (plan != nullptr) {
        for (const auto& row : plan->destination_rows) {
            if (!destination_row_shape_valid(row, desc.graph_asset)) {
                add_diagnostic(result, RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::invalid_destination_row,
                               row.graph_asset, row.page_index, row.mount_id,
                               "MAVG page GPU resource update rows must match the graph, use non-zero mount and "
                               "buffer handles, and carry positive byte sizes");
                invalid_inputs = true;
                continue;
            }
            if (contains_page_index(page_indices, row.page_index) || contains_mount_id(mount_ids, row.mount_id)) {
                ++result.duplicate_destination_row_count;
                add_diagnostic(result,
                               RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::duplicate_destination_row,
                               row.graph_asset, row.page_index, row.mount_id,
                               "MAVG page GPU resource update rows must be unique by page index and mount id");
                invalid_inputs = true;
                continue;
            }
            page_indices.push_back(row.page_index);
            mount_ids.push_back(row.mount_id);
        }
    }

    if (invalid_inputs) {
        return result;
    }

    result.resident_page_resources.reserve(plan->destination_rows.size());
    result.update_rows.reserve(plan->destination_rows.size());
    for (const auto& row : plan->destination_rows) {
        append_ready_row(result, row);
        result.ready_destination_bytes += row.destination_size;
        result.ready_estimated_gpu_resident_bytes += row.estimated_gpu_resident_bytes;
    }

    result.ready_resource_count = result.resident_page_resources.size();
    result.used_directstorage_resource_destination = true;
    result.used_directstorage_caller_owned_rhi_resource_destination = true;
    result.directstorage_status_complete = true;
    result.observed_native_queue_submission = true;
    result.ready_for_residency_actions = true;
    return result;
}

} // namespace mirakana::runtime_rhi
