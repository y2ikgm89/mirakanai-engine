// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/mavg_payload_page_loader.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

[[nodiscard]] std::string payload_format_prefix() {
    return "format=" + std::string(mavg_cluster_payload_format_v1()) + "\n";
}

void add_diagnostic(RuntimeMavgPayloadPageLoadResult& result, RuntimeMavgPayloadPageLoadDiagnosticCode code,
                    AssetId graph_asset, std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPayloadPageLoadDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .page_index = page_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool payload_starts_with_format(std::span<const std::byte> payload_bytes) noexcept {
    const std::string prefix = payload_format_prefix();
    if (payload_bytes.size() < prefix.size()) {
        return false;
    }
    for (std::size_t index = 0; index < prefix.size(); ++index) {
        if (std::to_integer<unsigned char>(payload_bytes[index]) != static_cast<unsigned char>(prefix[index])) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] const MavgClusterGraphPage* find_page(const MavgClusterGraphDocument& graph,
                                                    std::uint32_t page_index) noexcept {
    const auto found = std::ranges::find_if(
        graph.pages, [page_index](const MavgClusterGraphPage& page) { return page.page_index == page_index; });
    if (found == graph.pages.end()) {
        return nullptr;
    }
    return &*found;
}

[[nodiscard]] bool contains_page(std::span<const std::uint32_t> page_indices, std::uint32_t page_index,
                                 std::size_t before_index) noexcept {
    for (std::size_t index = 0; index < before_index; ++index) {
        if (page_indices[index] == page_index) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::vector<const MavgClusterGraphPage*>
validate_requested_pages(RuntimeMavgPayloadPageLoadResult& result, const MavgClusterGraphDocument& graph,
                         AssetId graph_asset, std::span<const std::uint32_t> page_indices) {
    std::vector<const MavgClusterGraphPage*> selected_pages;
    selected_pages.reserve(page_indices.size());
    for (std::size_t request_index = 0; request_index < page_indices.size(); ++request_index) {
        const auto page_index = page_indices[request_index];
        if (contains_page(page_indices, page_index, request_index)) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::duplicate_page_request, graph_asset,
                           page_index, "MAVG payload page load requests must be unique");
            continue;
        }
        const auto* const page = find_page(graph, page_index);
        if (page == nullptr) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::unknown_page, graph_asset, page_index,
                           "MAVG payload page load request references an unknown page");
            continue;
        }
        if (page->byte_offset > std::numeric_limits<std::uint64_t>::max() - page->byte_size) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::page_range_overflow, graph_asset,
                           page_index, "MAVG payload page load byte range overflows");
            continue;
        }
        selected_pages.push_back(page);
    }
    return selected_pages;
}

} // namespace

RuntimeMavgPayloadPageLoadResult load_runtime_mavg_payload_pages(const RuntimeMavgPayloadPageLoadDesc& desc) {
    RuntimeMavgPayloadPageLoadResult result;
    result.requested_page_count = desc.page_indices.size();
    result.payload_byte_count = desc.payload_bytes.size();

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_graph_asset, desc.graph_asset, 0,
                       "MAVG payload page load graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::missing_graph, desc.graph_asset, 0,
                       "MAVG payload page load graph document is required");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto& graph = *desc.graph;
    if (graph.asset != desc.graph_asset) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::graph_asset_mismatch, desc.graph_asset, 0,
                       "MAVG payload page load graph document asset must match the requested graph asset");
        invalid_inputs = true;
    }
    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_graph, desc.graph_asset, 0,
                       "MAVG payload page load graph validation failed");
        invalid_inputs = true;
    }
    if (!payload_starts_with_format(desc.payload_bytes)) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_payload_format, desc.graph_asset, 0,
                       "MAVG payload page load requires GameEngine.MavgClusterPayload.v1 text");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto selected_pages = validate_requested_pages(result, graph, desc.graph_asset, desc.page_indices);
    for (const auto* const page : selected_pages) {
        const auto range_end = page->byte_offset + page->byte_size;
        if (range_end > desc.payload_bytes.size()) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::page_range_out_of_bounds, desc.graph_asset,
                           page->page_index, "MAVG payload page load byte range exceeds payload bytes");
        }
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.loaded_pages.reserve(selected_pages.size());
    for (const auto* const page : selected_pages) {
        const auto begin = static_cast<std::size_t>(page->byte_offset);
        const auto byte_size = static_cast<std::size_t>(page->byte_size);
        const auto page_bytes = desc.payload_bytes.subspan(begin, byte_size);
        result.loaded_pages.push_back(RuntimeMavgPayloadPageRow{
            .graph_asset = desc.graph_asset,
            .page_index = page->page_index,
            .byte_offset = page->byte_offset,
            .byte_size = page->byte_size,
            .bytes = std::vector<std::byte>(page_bytes.begin(), page_bytes.end()),
        });
    }
    result.loaded_page_count = result.loaded_pages.size();
    return result;
}

RuntimeMavgPayloadPageLoadResult
load_runtime_mavg_payload_pages_from_filesystem(const RuntimeMavgPayloadFilesystemPageLoadDesc& desc) {
    RuntimeMavgPayloadPageLoadResult result;
    result.requested_page_count = desc.page_indices.size();
    result.invoked_file_io = desc.filesystem != nullptr;

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_graph_asset, desc.graph_asset, 0,
                       "MAVG payload filesystem page load graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::missing_graph, desc.graph_asset, 0,
                       "MAVG payload filesystem page load graph document is required");
        invalid_inputs = true;
    }
    if (desc.filesystem == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::missing_filesystem, desc.graph_asset, 0,
                       "MAVG payload filesystem page load requires a filesystem");
        invalid_inputs = true;
    }
    if (desc.payload_path.empty()) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::missing_payload_path, desc.graph_asset, 0,
                       "MAVG payload filesystem page load requires a payload path");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto& graph = *desc.graph;
    if (graph.asset != desc.graph_asset) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::graph_asset_mismatch, desc.graph_asset, 0,
                       "MAVG payload filesystem page load graph document asset must match the requested graph asset");
        invalid_inputs = true;
    }
    if (desc.payload_path != graph.cluster_payload_uri) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::payload_path_mismatch, desc.graph_asset, 0,
                       "MAVG payload filesystem page load path must match the graph payload uri");
        invalid_inputs = true;
    }
    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_graph, desc.graph_asset, 0,
                       "MAVG payload filesystem page load graph validation failed");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto prefix = payload_format_prefix();
    std::vector<std::byte> prefix_bytes;
    try {
        prefix_bytes = desc.filesystem->read_binary_range(desc.payload_path, 0, prefix.size());
        result.filesystem_read_byte_count += prefix_bytes.size();
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_payload_format, desc.graph_asset, 0,
                       std::string("MAVG payload filesystem page load failed to read format prefix: ") + error.what());
        return result;
    }
    if (!payload_starts_with_format(prefix_bytes)) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_payload_format, desc.graph_asset, 0,
                       "MAVG payload filesystem page load requires GameEngine.MavgClusterPayload.v1 text");
        return result;
    }

    const auto selected_pages = validate_requested_pages(result, graph, desc.graph_asset, desc.page_indices);
    if (!result.diagnostics.empty()) {
        result.payload_byte_count = result.filesystem_read_byte_count;
        return result;
    }

    std::vector<RuntimeMavgPayloadPageRow> loaded_pages;
    loaded_pages.reserve(selected_pages.size());
    for (const auto* const page : selected_pages) {
        try {
            auto bytes = desc.filesystem->read_binary_range(desc.payload_path, page->byte_offset, page->byte_size);
            result.filesystem_read_byte_count += bytes.size();
            loaded_pages.push_back(RuntimeMavgPayloadPageRow{
                .graph_asset = desc.graph_asset,
                .page_index = page->page_index,
                .byte_offset = page->byte_offset,
                .byte_size = page->byte_size,
                .bytes = std::move(bytes),
            });
        } catch (const std::out_of_range& error) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::page_range_out_of_bounds, desc.graph_asset,
                           page->page_index,
                           std::string("MAVG payload filesystem page load byte range exceeds payload bytes: ") +
                               error.what());
        } catch (const std::exception& error) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::page_range_read_failed, desc.graph_asset,
                           page->page_index,
                           std::string("MAVG payload filesystem page load byte-range read failed: ") + error.what());
        }
    }
    result.payload_byte_count = result.filesystem_read_byte_count;
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.loaded_pages = std::move(loaded_pages);
    result.loaded_page_count = result.loaded_pages.size();
    return result;
}

RuntimeMavgPayloadPageLoadResult
load_runtime_mavg_payload_pages_from_direct_storage(const RuntimeMavgPayloadDirectStoragePageLoadDesc& desc) {
    RuntimeMavgPayloadPageLoadResult result;
    result.requested_page_count = desc.page_indices.size();

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_graph_asset, desc.graph_asset, 0,
                       "MAVG payload DirectStorage page load graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::missing_graph, desc.graph_asset, 0,
                       "MAVG payload DirectStorage page load graph document is required");
        invalid_inputs = true;
    }
    if (desc.direct_storage == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::missing_direct_storage_executor,
                       desc.graph_asset, 0, "MAVG payload DirectStorage page load requires a DirectStorage executor");
        invalid_inputs = true;
    }
    if (desc.payload_path.empty()) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::missing_payload_path, desc.graph_asset, 0,
                       "MAVG payload DirectStorage page load requires a payload path");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto& graph = *desc.graph;
    if (graph.asset != desc.graph_asset) {
        add_diagnostic(
            result, RuntimeMavgPayloadPageLoadDiagnosticCode::graph_asset_mismatch, desc.graph_asset, 0,
            "MAVG payload DirectStorage page load graph document asset must match the requested graph asset");
        invalid_inputs = true;
    }
    if (desc.payload_path != graph.cluster_payload_uri) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::payload_path_mismatch, desc.graph_asset, 0,
                       "MAVG payload DirectStorage page load path must match the graph payload uri");
        invalid_inputs = true;
    }
    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_graph, desc.graph_asset, 0,
                       "MAVG payload DirectStorage page load graph validation failed");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }
    if (desc.direct_storage->backend_kind() != ByteRangeIoBackendKind::direct_storage) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::direct_storage_unavailable, desc.graph_asset,
                       0, "MAVG payload DirectStorage page load requires a DirectStorage byte-range executor");
        return result;
    }
    if (!desc.direct_storage->available()) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::direct_storage_unavailable, desc.graph_asset,
                       0, "MAVG payload DirectStorage page load executor is unavailable on this host");
        return result;
    }

    const auto selected_pages = validate_requested_pages(result, graph, desc.graph_asset, desc.page_indices);
    if (!result.diagnostics.empty()) {
        return result;
    }

    const auto prefix = payload_format_prefix();
    const std::vector<ByteRangeIoReadRequest> prefix_request{
        ByteRangeIoReadRequest{
            .path = desc.payload_path,
            .byte_offset = 0,
            .byte_size = prefix.size(),
        },
    };
    std::vector<ByteRangeIoReadRow> prefix_rows;
    try {
        result.executed_direct_storage = true;
        prefix_rows = desc.direct_storage->read_ranges(prefix_request);
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::direct_storage_page_read_failed,
                       desc.graph_asset, 0,
                       std::string("MAVG payload DirectStorage page load byte-range read failed: ") + error.what());
        result.loaded_pages.clear();
        result.loaded_page_count = 0;
        result.payload_byte_count = 0;
        return result;
    }
    if (prefix_rows.size() != 1U) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::direct_storage_result_mismatch,
                       desc.graph_asset, 0,
                       "MAVG payload DirectStorage page load returned an unexpected prefix row count");
        return result;
    }
    if (prefix_rows[0].path != desc.payload_path || prefix_rows[0].byte_offset != 0 ||
        prefix_rows[0].byte_size != prefix.size() || !payload_starts_with_format(prefix_rows[0].bytes)) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::invalid_payload_format, desc.graph_asset, 0,
                       "MAVG payload DirectStorage page load requires GameEngine.MavgClusterPayload.v1 text");
        return result;
    }

    std::vector<ByteRangeIoReadRequest> requests;
    requests.reserve(selected_pages.size());
    for (const auto* const page : selected_pages) {
        requests.push_back(ByteRangeIoReadRequest{
            .path = desc.payload_path,
            .byte_offset = page->byte_offset,
            .byte_size = page->byte_size,
        });
    }

    std::vector<ByteRangeIoReadRow> range_rows;
    try {
        range_rows = desc.direct_storage->read_ranges(requests);
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::direct_storage_page_read_failed,
                       desc.graph_asset, 0,
                       std::string("MAVG payload DirectStorage page load byte-range read failed: ") + error.what());
        result.loaded_pages.clear();
        result.loaded_page_count = 0;
        result.payload_byte_count = prefix_rows[0].bytes.size();
        return result;
    }

    if (range_rows.size() != requests.size()) {
        add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::direct_storage_result_mismatch,
                       desc.graph_asset, 0,
                       "MAVG payload DirectStorage page load returned an unexpected page row count");
        return result;
    }
    std::vector<RuntimeMavgPayloadPageRow> loaded_pages;
    loaded_pages.reserve(selected_pages.size());
    for (std::size_t selected_index = 0; selected_index < selected_pages.size(); ++selected_index) {
        const auto& request = requests[selected_index];
        const auto& page = *selected_pages[selected_index];
        const auto& range_row = range_rows[selected_index];
        if (range_row.path != desc.payload_path || range_row.byte_offset != request.byte_offset ||
            range_row.byte_size != request.byte_size || range_row.bytes.size() != request.byte_size) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::direct_storage_result_mismatch,
                           desc.graph_asset, page.page_index,
                           "MAVG payload DirectStorage page load returned a row that does not match the request");
        }
        loaded_pages.push_back(RuntimeMavgPayloadPageRow{
            .graph_asset = desc.graph_asset,
            .page_index = page.page_index,
            .byte_offset = range_row.byte_offset,
            .byte_size = range_row.byte_size,
            .bytes = range_row.bytes,
        });
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.payload_byte_count = prefix_rows[0].bytes.size();
    for (const auto& row : range_rows) {
        result.payload_byte_count += row.bytes.size();
    }
    result.loaded_pages = std::move(loaded_pages);
    result.loaded_page_count = result.loaded_pages.size();
    return result;
}

} // namespace mirakana::runtime
