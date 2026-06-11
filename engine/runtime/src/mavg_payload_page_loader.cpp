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
    const std::string prefix = "format=" + std::string(mavg_cluster_payload_format_v1()) + "\n";
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

    std::vector<const MavgClusterGraphPage*> selected_pages;
    selected_pages.reserve(desc.page_indices.size());
    for (std::size_t request_index = 0; request_index < desc.page_indices.size(); ++request_index) {
        const auto page_index = desc.page_indices[request_index];
        if (contains_page(desc.page_indices, page_index, request_index)) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::duplicate_page_request, desc.graph_asset,
                           page_index, "MAVG payload page load requests must be unique");
            continue;
        }
        const auto* const page = find_page(graph, page_index);
        if (page == nullptr) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::unknown_page, desc.graph_asset, page_index,
                           "MAVG payload page load request references an unknown page");
            continue;
        }
        if (page->byte_offset > std::numeric_limits<std::uint64_t>::max() - page->byte_size) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::page_range_overflow, desc.graph_asset,
                           page_index, "MAVG payload page load byte range overflows");
            continue;
        }
        const auto range_end = page->byte_offset + page->byte_size;
        if (range_end > desc.payload_bytes.size()) {
            add_diagnostic(result, RuntimeMavgPayloadPageLoadDiagnosticCode::page_range_out_of_bounds, desc.graph_asset,
                           page_index, "MAVG payload page load byte range exceeds payload bytes");
            continue;
        }
        selected_pages.push_back(page);
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.loaded_pages.reserve(selected_pages.size());
    for (const auto* const page : selected_pages) {
        const auto begin = static_cast<std::size_t>(page->byte_offset);
        const auto end = static_cast<std::size_t>(page->byte_offset + page->byte_size);
        result.loaded_pages.push_back(RuntimeMavgPayloadPageRow{
            .graph_asset = desc.graph_asset,
            .page_index = page->page_index,
            .byte_offset = page->byte_offset,
            .byte_size = page->byte_size,
            .bytes = std::vector<std::byte>(desc.payload_bytes.begin() + begin, desc.payload_bytes.begin() + end),
        });
    }
    result.loaded_page_count = result.loaded_pages.size();
    return result;
}

} // namespace mirakana::runtime
