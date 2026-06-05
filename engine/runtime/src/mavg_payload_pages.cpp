// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/mavg_payload_pages.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

void add_diagnostic(RuntimeMavgPayloadPageSliceResult& result, RuntimeMavgPayloadPageSliceDiagnosticCode code,
                    std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPayloadPageSliceDiagnostic{
        .code = code,
        .page_index = page_index,
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimeMavgPayloadPageFileLoadResult& result, RuntimeMavgPayloadPageFileLoadDiagnosticCode code,
                    std::uint32_t page_index, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPayloadPageFileLoadDiagnostic{
        .code = code,
        .page_index = page_index,
        .message = std::move(message),
    });
}

[[nodiscard]] std::uint8_t hex_value(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<std::uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<std::uint8_t>(10 + value - 'a');
    }
    if (value >= 'A' && value <= 'F') {
        return static_cast<std::uint8_t>(10 + value - 'A');
    }
    throw std::invalid_argument("MAVG payload page data hex is invalid");
}

[[nodiscard]] std::vector<std::uint8_t> decode_hex_bytes(std::string_view encoded) {
    if ((encoded.size() % 2U) != 0U) {
        throw std::invalid_argument("MAVG payload page data hex length is invalid");
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(encoded.size() / 2U);
    for (std::size_t index = 0; index < encoded.size(); index += 2U) {
        const auto high = hex_value(encoded[index]);
        const auto low = hex_value(encoded[index + 1U]);
        bytes.push_back(static_cast<std::uint8_t>((high << 4U) | low));
    }
    return bytes;
}

[[nodiscard]] const MavgClusterPayloadPage* find_payload_page(const MavgClusterPayloadDocument& payload,
                                                              std::uint32_t page_index) noexcept {
    const auto found = std::ranges::find_if(
        payload.pages, [page_index](const MavgClusterPayloadPage& page) { return page.page_index == page_index; });
    return found == payload.pages.end() ? nullptr : &*found;
}

[[nodiscard]] bool has_graph_page(const MavgClusterGraphDocument& graph, std::uint32_t page_index) noexcept {
    return std::ranges::any_of(
        graph.pages, [page_index](const MavgClusterGraphPage& page) { return page.page_index == page_index; });
}

} // namespace

RuntimeMavgPayloadPageSliceResult
extract_runtime_mavg_payload_page_slices(const RuntimeMavgPayloadPageSliceDesc& desc) {
    RuntimeMavgPayloadPageSliceResult result;
    result.requested_page_count = desc.page_indices.size();
    result.invoked_file_io = false;
    result.mutated_mount_set = false;
    result.executed_background_worker = false;
    result.touched_renderer_or_rhi_handles = false;

    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadPageSliceDiagnosticCode::missing_graph, 0,
                       "MAVG payload page slice extraction requires a graph document");
        return result;
    }

    const auto graph_validation = validate_mavg_cluster_graph(*desc.graph);
    if (!graph_validation.valid()) {
        add_diagnostic(result, RuntimeMavgPayloadPageSliceDiagnosticCode::invalid_graph, 0,
                       "MAVG payload page slice graph validation failed");
        return result;
    }

    MavgClusterPayloadDocument payload;
    try {
        payload = deserialize_mavg_cluster_payload_document(desc.payload_text);
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgPayloadPageSliceDiagnosticCode::invalid_payload, 0, error.what());
        return result;
    }

    const auto payload_validation = validate_mavg_cluster_payload(payload, *desc.graph);
    if (!payload_validation.valid()) {
        add_diagnostic(result, RuntimeMavgPayloadPageSliceDiagnosticCode::invalid_payload, 0,
                       "MAVG payload page slice payload validation failed");
        return result;
    }

    std::unordered_set<std::uint32_t> requested_pages;
    for (const auto page_index : desc.page_indices) {
        if (!requested_pages.insert(page_index).second) {
            add_diagnostic(result, RuntimeMavgPayloadPageSliceDiagnosticCode::duplicate_requested_page, page_index,
                           "MAVG payload page slice request page appears more than once");
        }
        if (!has_graph_page(*desc.graph, page_index)) {
            add_diagnostic(result, RuntimeMavgPayloadPageSliceDiagnosticCode::unknown_page, page_index,
                           "MAVG payload page slice request references an unknown graph page");
        }
        if (find_payload_page(payload, page_index) == nullptr) {
            add_diagnostic(result, RuntimeMavgPayloadPageSliceDiagnosticCode::missing_payload_page, page_index,
                           "MAVG payload page slice request page is missing from the payload");
        }
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    std::vector<std::uint8_t> page_bytes;
    try {
        page_bytes = decode_hex_bytes(payload.page_data_hex);
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgPayloadPageSliceDiagnosticCode::invalid_payload, 0, error.what());
        return result;
    }

    result.pages.reserve(desc.page_indices.size());
    for (const auto page_index : desc.page_indices) {
        const auto* const page = find_payload_page(payload, page_index);
        if (page == nullptr) {
            add_diagnostic(result, RuntimeMavgPayloadPageSliceDiagnosticCode::missing_payload_page, page_index,
                           "MAVG payload page slice request page is missing from the payload");
            continue;
        }
        const auto offset = static_cast<std::size_t>(page->byte_offset);
        const auto size = static_cast<std::size_t>(page->byte_size);
        result.pages.push_back(RuntimeMavgPayloadPageSliceRow{
            .page_index = page->page_index,
            .byte_offset = page->byte_offset,
            .byte_size = page->byte_size,
            .payload_bytes = std::vector<std::uint8_t>(page_bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                                                       page_bytes.begin() + static_cast<std::ptrdiff_t>(offset + size)),
        });
    }
    result.extracted_page_count = result.pages.size();
    return result;
}

RuntimeMavgPayloadPageFileLoadResult
load_runtime_mavg_payload_file_pages(const RuntimeMavgPayloadPageFileLoadDesc& desc) {
    RuntimeMavgPayloadPageFileLoadResult result;
    result.requested_page_count = desc.page_indices.size();
    result.invoked_file_io = false;
    result.used_native_directstorage = false;
    result.mutated_mount_set = false;
    result.executed_background_worker = false;
    result.touched_renderer_or_rhi_handles = false;

    if (desc.filesystem == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::missing_filesystem, 0,
                       "MAVG payload page file loading requires a filesystem");
        return result;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::missing_graph, 0,
                       "MAVG payload page file loading requires a graph document");
        return result;
    }
    if (desc.payload_blob_path.empty()) {
        add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::missing_payload_blob_path, 0,
                       "MAVG payload page file loading requires a payload blob path");
        return result;
    }

    const auto graph_validation = validate_mavg_cluster_graph(*desc.graph);
    if (!graph_validation.valid()) {
        add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::invalid_graph, 0,
                       "MAVG payload page file loading graph validation failed");
        return result;
    }

    MavgClusterPayloadDocument payload;
    try {
        payload = deserialize_mavg_cluster_payload_document(desc.payload_text);
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::invalid_payload, 0, error.what());
        return result;
    }

    const auto payload_validation = validate_mavg_cluster_payload(payload, *desc.graph);
    if (!payload_validation.valid()) {
        add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::invalid_payload, 0,
                       "MAVG payload page file loading payload validation failed");
        return result;
    }

    std::unordered_set<std::uint32_t> requested_pages;
    for (const auto page_index : desc.page_indices) {
        if (!requested_pages.insert(page_index).second) {
            add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::duplicate_requested_page, page_index,
                           "MAVG payload page file loading request page appears more than once");
        }
        if (!has_graph_page(*desc.graph, page_index)) {
            add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::unknown_page, page_index,
                           "MAVG payload page file loading request references an unknown graph page");
        }
        if (find_payload_page(payload, page_index) == nullptr) {
            add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::missing_payload_page, page_index,
                           "MAVG payload page file loading request page is missing from the payload");
        }
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    std::vector<RuntimeMavgPayloadPageFileLoadRow> loaded_pages;
    loaded_pages.reserve(desc.page_indices.size());
    for (const auto page_index : desc.page_indices) {
        const auto* const page = find_payload_page(payload, page_index);
        if (page == nullptr) {
            add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::missing_payload_page, page_index,
                           "MAVG payload page file loading request page is missing from the payload");
            loaded_pages.clear();
            return result;
        }

        try {
            result.invoked_file_io = true;
            loaded_pages.push_back(RuntimeMavgPayloadPageFileLoadRow{
                .page_index = page->page_index,
                .byte_offset = page->byte_offset,
                .byte_size = page->byte_size,
                .payload_bytes =
                    desc.filesystem->read_byte_range(desc.payload_blob_path, page->byte_offset, page->byte_size),
            });
        } catch (const std::exception& error) {
            add_diagnostic(result, RuntimeMavgPayloadPageFileLoadDiagnosticCode::payload_file_read_failed, page_index,
                           error.what());
            loaded_pages.clear();
            return result;
        }
    }

    result.pages = std::move(loaded_pages);
    result.loaded_page_count = result.pages.size();
    return result;
}

} // namespace mirakana::runtime
