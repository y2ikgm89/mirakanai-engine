// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_page_gpu_buffer_destination.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgPageBufferDestinationPlanResult& result,
                    RuntimeMavgPageBufferDestinationDiagnosticCode code, AssetId graph_asset, std::uint32_t page_index,
                    runtime::RuntimeResidentPackageMountIdV2 mount_id, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPageBufferDestinationDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .mount_id = mount_id,
        .message = std::move(message),
    });
}

[[nodiscard]] bool graph_contains_page(const MavgClusterGraphDocument& graph, std::uint32_t page_index) noexcept {
    return std::ranges::any_of(
        graph.pages, [page_index](const MavgClusterGraphPage& page) { return page.page_index == page_index; });
}

[[nodiscard]] bool contains_page_index(const std::vector<std::uint32_t>& pages, std::uint32_t page_index) noexcept {
    return std::ranges::find(pages, page_index) != pages.end();
}

[[nodiscard]] bool contains_mount_id(const std::vector<runtime::RuntimeResidentPackageMountIdV2>& mount_ids,
                                     runtime::RuntimeResidentPackageMountIdV2 mount_id) noexcept {
    return std::ranges::find(mount_ids, mount_id) != mount_ids.end();
}

[[nodiscard]] bool range_end(std::uint64_t offset, std::uint64_t size, std::uint64_t& end) noexcept {
    if (offset > std::numeric_limits<std::uint64_t>::max() - size) {
        return false;
    }
    end = offset + size;
    return true;
}

[[nodiscard]] bool request_fits_destination(const runtime::RuntimeMavgPayloadDirectStorageRequestRow& request,
                                            const RuntimeMavgPageBufferDestinationRow& destination) noexcept {
    std::uint64_t request_end = 0;
    std::uint64_t destination_end = 0;
    if (!range_end(request.destination_offset, request.destination_size, request_end) ||
        !range_end(destination.destination_offset, destination.destination_size, destination_end)) {
        return false;
    }
    return request.destination_offset >= destination.destination_offset && request_end <= destination_end;
}

[[nodiscard]] const RuntimeMavgPageBufferDestinationRow*
find_destination_by_page(std::span<const RuntimeMavgPageBufferDestinationRow> destinations,
                         std::uint32_t page_index) noexcept {
    const auto found = std::ranges::find_if(destinations, [page_index](const RuntimeMavgPageBufferDestinationRow& row) {
        return row.page_index == page_index;
    });
    return found == destinations.end() ? nullptr : &*found;
}

} // namespace

RuntimeMavgPageBufferDestinationPlanResult
plan_runtime_mavg_page_buffer_destinations(const RuntimeMavgPageBufferDestinationDesc& desc) {
    RuntimeMavgPageBufferDestinationPlanResult result;
    result.input_destination_row_count = desc.destination_rows.size();
    result.invoked_file_io = false;
    result.used_native_directstorage = false;
    result.submitted_native_queue = false;
    result.used_directstorage_resource_destination = false;
    result.used_gpu_decompression = false;
    result.allocated_rhi_resources = false;
    result.enforced_allocator_budget = false;
    result.mutated_mount_set = false;
    result.exposed_native_handles = false;

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::invalid_graph_asset, desc.graph_asset, 0,
                       {}, "MAVG page GPU buffer destination graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::missing_graph, desc.graph_asset, 0, {},
                       "MAVG page GPU buffer destination planning requires a graph document");
        invalid_inputs = true;
    } else {
        const auto validation = validate_mavg_cluster_graph(*desc.graph);
        if (desc.graph->asset != desc.graph_asset || !validation.valid()) {
            add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::invalid_graph, desc.graph->asset, 0,
                           {}, "MAVG page GPU buffer destination graph must match the requested asset and validate");
            invalid_inputs = true;
        }
    }
    if (desc.request_plan == nullptr) {
        add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::missing_request_plan, desc.graph_asset,
                       0, {}, "MAVG page GPU buffer destination planning requires a DirectStorage request plan");
        invalid_inputs = true;
    } else {
        result.requested_page_count = desc.request_plan->planned_request_count;
        if (!desc.request_plan->succeeded()) {
            add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::invalid_request_plan,
                           desc.graph_asset, 0, {},
                           "MAVG page GPU buffer destination planning requires a successful request plan");
            invalid_inputs = true;
        }
    }

    std::vector<std::uint32_t> destination_pages;
    std::vector<runtime::RuntimeResidentPackageMountIdV2> destination_mounts;
    if (desc.graph != nullptr) {
        for (const auto& row : desc.destination_rows) {
            if (row.graph_asset != desc.graph_asset || !graph_contains_page(*desc.graph, row.page_index) ||
                row.mount_id.value == 0 || row.buffer.value == 0 || row.destination_size == 0 ||
                row.estimated_gpu_resident_bytes == 0) {
                add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::invalid_destination_row,
                               row.graph_asset, row.page_index, row.mount_id,
                               "MAVG page GPU buffer destination rows must match the graph, reference a known page, "
                               "use non-zero mount and buffer handles, and carry positive byte sizes");
                invalid_inputs = true;
                continue;
            }
            std::uint64_t ignored_end = 0;
            if (!range_end(row.destination_offset, row.destination_size, ignored_end)) {
                add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::destination_range_mismatch,
                               row.graph_asset, row.page_index, row.mount_id,
                               "MAVG page GPU buffer destination row range overflows");
                invalid_inputs = true;
                continue;
            }
            if (contains_page_index(destination_pages, row.page_index) ||
                contains_mount_id(destination_mounts, row.mount_id)) {
                ++result.duplicate_destination_row_count;
                add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::duplicate_destination_row,
                               row.graph_asset, row.page_index, row.mount_id,
                               "MAVG page GPU buffer destination rows must be unique by page index and mount id");
                invalid_inputs = true;
                continue;
            }
            destination_pages.push_back(row.page_index);
            destination_mounts.push_back(row.mount_id);
        }
    }

    if (invalid_inputs) {
        return result;
    }

    result.destination_rows.reserve(desc.request_plan->requests.size());
    for (const auto& request : desc.request_plan->requests) {
        if (desc.graph != nullptr && !graph_contains_page(*desc.graph, request.page_index)) {
            add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::invalid_request_plan,
                           desc.graph_asset, request.page_index, {},
                           "MAVG page GPU buffer destination request plan references an unknown graph page");
            result.destination_rows.clear();
            return result;
        }

        const auto* const destination = find_destination_by_page(desc.destination_rows, request.page_index);
        if (destination == nullptr) {
            ++result.missing_destination_row_count;
            add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::missing_destination_row,
                           desc.graph_asset, request.page_index, {},
                           "MAVG page GPU buffer destination planning requires a destination row for every request");
            result.destination_rows.clear();
            return result;
        }
        if (!request_fits_destination(request, *destination)) {
            add_diagnostic(result, RuntimeMavgPageBufferDestinationDiagnosticCode::destination_range_mismatch,
                           desc.graph_asset, request.page_index, destination->mount_id,
                           "MAVG page GPU buffer destination request range must fit the destination row range");
            result.destination_rows.clear();
            return result;
        }

        result.destination_rows.push_back(RuntimeMavgPageBufferDestinationPlanRow{
            .graph_asset = desc.graph_asset,
            .request_index = request.request_index,
            .page_index = request.page_index,
            .mount_id = destination->mount_id,
            .buffer = destination->buffer,
            .source_file_offset = request.source_file_offset,
            .source_size = request.source_size,
            .source_file_path = request.source_file_path,
            .destination_offset = request.destination_offset,
            .destination_size = request.destination_size,
            .destination_range_offset = destination->destination_offset,
            .destination_range_size = destination->destination_size,
            .estimated_gpu_resident_bytes = destination->estimated_gpu_resident_bytes,
            .fence_wait_point = request.fence_wait_point,
            .synchronized_with_fence = request.synchronized_with_fence,
        });
        result.total_destination_bytes += request.destination_size;
        result.total_estimated_gpu_resident_bytes += destination->estimated_gpu_resident_bytes;
    }

    result.planned_destination_count = result.destination_rows.size();
    return result;
}

} // namespace mirakana::runtime_rhi
