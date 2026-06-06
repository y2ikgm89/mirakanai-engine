// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/mavg_payload_pages.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
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

void add_diagnostic(RuntimeMavgPayloadDirectStorageRequestPlanResult& result,
                    RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode code, std::uint32_t page_index,
                    std::string message) {
    result.diagnostics.push_back(RuntimeMavgPayloadDirectStorageRequestPlanDiagnostic{
        .code = code,
        .page_index = page_index,
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimeMavgPayloadNativeIoDispatchResult& result, RuntimeMavgPayloadNativeIoDiagnosticCode code,
                    std::uint32_t request_index, std::uint64_t ticket, std::int32_t hresult, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPayloadNativeIoDiagnostic{
        .code = code,
        .request_index = request_index,
        .ticket = ticket,
        .hresult = hresult,
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimeMavgPayloadNativeIoStatusPollResult& result, RuntimeMavgPayloadNativeIoDiagnosticCode code,
                    std::uint64_t ticket, std::int32_t hresult, std::string message) {
    result.diagnostics.push_back(RuntimeMavgPayloadNativeIoDiagnostic{
        .code = code,
        .ticket = ticket,
        .hresult = hresult,
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

[[nodiscard]] bool fits_directstorage_request_size(std::uint64_t byte_size) noexcept {
    return byte_size <= std::numeric_limits<std::uint32_t>::max();
}

[[nodiscard]] bool range_overflows(std::uint64_t offset, std::uint64_t byte_size) noexcept {
    return offset > std::numeric_limits<std::uint64_t>::max() - byte_size;
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

RuntimeMavgPayloadDirectStorageRequestPlanResult
plan_runtime_mavg_payload_directstorage_requests(const RuntimeMavgPayloadDirectStorageRequestPlanDesc& desc) {
    RuntimeMavgPayloadDirectStorageRequestPlanResult result;
    result.requested_page_count = desc.page_indices.size();
    result.invoked_file_io = false;
    result.used_native_directstorage = false;
    result.requires_native_directstorage_sdk = false;
    result.enqueued_native_requests = false;
    result.submitted_native_queue = false;
    result.signaled_native_fence = false;
    result.mutated_mount_set = false;
    result.executed_background_worker = false;
    result.touched_renderer_or_rhi_handles = false;

    if (desc.graph == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::missing_graph, 0,
                       "MAVG payload DirectStorage request planning requires a graph document");
        return result;
    }
    if (desc.payload_blob_path.empty()) {
        add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::missing_payload_blob_path, 0,
                       "MAVG payload DirectStorage request planning requires a payload blob path");
        return result;
    }

    const auto graph_validation = validate_mavg_cluster_graph(*desc.graph);
    if (!graph_validation.valid()) {
        add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::invalid_graph, 0,
                       "MAVG payload DirectStorage request planning graph validation failed");
        return result;
    }

    MavgClusterPayloadDocument payload;
    try {
        payload = deserialize_mavg_cluster_payload_document(desc.payload_text);
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::invalid_payload, 0,
                       error.what());
        return result;
    }

    const auto payload_validation = validate_mavg_cluster_payload(payload, *desc.graph);
    if (!payload_validation.valid()) {
        add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::invalid_payload, 0,
                       "MAVG payload DirectStorage request planning payload validation failed");
        return result;
    }

    std::unordered_set<std::uint32_t> requested_pages;
    for (const auto page_index : desc.page_indices) {
        if (!requested_pages.insert(page_index).second) {
            add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::duplicate_requested_page,
                           page_index, "MAVG payload DirectStorage request page appears more than once");
        }
        if (!has_graph_page(*desc.graph, page_index)) {
            add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::unknown_page, page_index,
                           "MAVG payload DirectStorage request references an unknown graph page");
        }
        if (find_payload_page(payload, page_index) == nullptr) {
            add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::missing_payload_page,
                           page_index, "MAVG payload DirectStorage request page is missing from the payload");
        }
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    std::vector<RuntimeMavgPayloadDirectStorageRequestRow> planned_requests;
    planned_requests.reserve(desc.page_indices.size());
    auto destination_offset = desc.destination_base_offset;
    std::uint64_t total_source_bytes = 0;
    std::uint64_t total_destination_bytes = 0;
    for (std::size_t request_index = 0; request_index < desc.page_indices.size(); ++request_index) {
        const auto page_index = desc.page_indices[request_index];
        const auto* const page = find_payload_page(payload, page_index);
        if (page == nullptr) {
            add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::missing_payload_page,
                           page_index, "MAVG payload DirectStorage request page is missing from the payload");
            return result;
        }
        if (!fits_directstorage_request_size(page->byte_size)) {
            add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::page_request_too_large,
                           page_index, "MAVG payload DirectStorage request size must fit UINT32 request fields");
            return result;
        }
        if (range_overflows(destination_offset, page->byte_size) ||
            range_overflows(total_source_bytes, page->byte_size) ||
            range_overflows(total_destination_bytes, page->byte_size)) {
            add_diagnostic(result, RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::destination_range_overflow,
                           page_index, "MAVG payload DirectStorage request destination range overflows");
            return result;
        }

        const auto request_size = static_cast<std::uint32_t>(page->byte_size);
        planned_requests.push_back(RuntimeMavgPayloadDirectStorageRequestRow{
            .request_index = static_cast<std::uint32_t>(request_index),
            .page_index = page->page_index,
            .source_file_offset = page->byte_offset,
            .source_size = request_size,
            .source_file_path = std::string(desc.payload_blob_path),
            .destination_offset = destination_offset,
            .destination_size = request_size,
            .fence_wait_point = desc.fence_wait_point,
            .source_is_file = true,
            .destination_is_memory = true,
            .synchronized_with_fence = desc.synchronize_with_fence,
            .debug_name = "mavg.payload.page." + std::to_string(page->page_index),
        });

        destination_offset += page->byte_size;
        total_source_bytes += page->byte_size;
        total_destination_bytes += page->byte_size;
    }

    result.requests = std::move(planned_requests);
    result.planned_request_count = result.requests.size();
    result.total_source_bytes = total_source_bytes;
    result.total_destination_bytes = total_destination_bytes;
    result.requires_native_directstorage_sdk = true;
    return result;
}

RuntimeMavgPayloadNativeIoDispatchResult
dispatch_runtime_mavg_payload_native_io_requests(const RuntimeMavgPayloadNativeIoDispatchDesc& desc) {
    RuntimeMavgPayloadNativeIoDispatchResult result;
    result.backend = desc.required_backend;
    result.mutated_mount_set = false;
    result.executed_background_worker = false;
    result.touched_renderer_or_rhi_handles = false;

    if (desc.dispatcher == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::missing_dispatcher, 0, 0, 0,
                       "MAVG payload native IO dispatch requires a caller-owned dispatcher");
        return result;
    }
    result.backend = desc.dispatcher->backend();

    if (desc.request_plan == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::missing_request_plan, 0, 0, 0,
                       "MAVG payload native IO dispatch requires a request plan");
        return result;
    }
    if (!desc.request_plan->succeeded()) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::invalid_request_plan, 0, 0, 0,
                       "MAVG payload native IO dispatch requires a successful request plan");
        return result;
    }
    if (desc.request_plan->requests.empty()) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::empty_request_plan, 0, 0, 0,
                       "MAVG payload native IO dispatch requires at least one planned request");
        return result;
    }
    if (desc.dispatcher->backend() != desc.required_backend) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::unsupported_backend, 0, 0, 0,
                       "MAVG payload native IO dispatcher backend does not match the required backend");
        return result;
    }

    RuntimeMavgPayloadNativeIoDispatchBackendResult backend_result;
    try {
        backend_result = desc.dispatcher->dispatch(
            desc.request_plan->requests,
            RuntimeMavgPayloadNativeIoDispatchBackendDesc{
                .required_backend = desc.required_backend,
                .submission_tag = desc.submission_tag,
                .destination_memory = desc.destination_memory,
                .require_native_directstorage = desc.require_native_directstorage,
                .enqueue_status_after_requests = desc.enqueue_status_after_requests,
                .signal_fence_after_requests = desc.signal_fence_after_requests,
                .use_directstorage_d3d12_buffer_destination = desc.use_directstorage_d3d12_buffer_destination,
            });
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, 0, 0, error.what());
        return result;
    }

    result.diagnostics = std::move(backend_result.diagnostics);
    result.ticket = backend_result.ticket;
    result.backend = backend_result.backend;
    result.request_count = backend_result.enqueued_request_count;
    result.total_source_bytes = backend_result.submitted_source_bytes;
    result.total_destination_bytes = backend_result.submitted_destination_bytes;
    result.submitted_io_queue = backend_result.submitted_io_queue;
    result.enqueued_native_requests = backend_result.enqueued_native_requests;
    result.submitted_native_queue = backend_result.submitted_native_queue;
    result.enqueued_status_write = backend_result.enqueued_status_write;
    result.signaled_native_fence = backend_result.signaled_native_fence;
    result.native_fence_signal_value = backend_result.native_fence_signal_value;
    result.native_fence_completed_value = backend_result.native_fence_completed_value;
    result.used_directstorage_resource_destination = backend_result.used_directstorage_resource_destination;
    result.directstorage_resource_destination_request_count =
        backend_result.directstorage_resource_destination_request_count;
    result.directstorage_resource_destination_bytes = backend_result.directstorage_resource_destination_bytes;
    result.used_native_directstorage = backend_result.used_native_directstorage;
    result.used_win32_async_io = backend_result.used_win32_async_io;
    result.executed_background_worker = backend_result.executed_background_worker;
    result.touched_renderer_or_rhi_handles = backend_result.touched_renderer_or_rhi_handles;

    if (desc.require_native_directstorage && !backend_result.used_native_directstorage) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::unsupported_backend, 0, result.ticket, 0,
                       "MAVG payload native IO dispatch required a native DirectStorage backend");
    }
    if (!backend_result.submitted_io_queue && result.diagnostics.empty()) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, result.ticket, 0,
                       "MAVG payload native IO dispatcher did not submit the IO queue");
    }
    return result;
}

RuntimeMavgPayloadNativeIoStatusPollResult
poll_runtime_mavg_payload_native_io_status(const RuntimeMavgPayloadNativeIoStatusPollDesc& desc) {
    RuntimeMavgPayloadNativeIoStatusPollResult result;
    result.ticket = desc.ticket;
    result.mutated_mount_set = false;
    result.executed_background_worker = false;
    result.touched_renderer_or_rhi_handles = false;

    if (desc.dispatcher == nullptr) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::missing_dispatcher, 0, 0,
                       "MAVG payload native IO status polling requires a caller-owned dispatcher");
        return result;
    }
    if (desc.ticket == 0U) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::missing_ticket, 0, 0,
                       "MAVG payload native IO status polling requires a non-zero ticket");
        return result;
    }

    RuntimeMavgPayloadNativeIoStatusBackendResult backend_result;
    try {
        backend_result = desc.dispatcher->poll_status(desc.ticket);
    } catch (const std::exception& error) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::status_failed, desc.ticket, 0, error.what());
        result.failed = true;
        result.status = RuntimeMavgPayloadNativeIoStatus::failed;
        return result;
    }

    result.diagnostics = std::move(backend_result.diagnostics);
    result.ticket = backend_result.ticket;
    result.status = backend_result.status;
    result.hresult = backend_result.hresult;
    result.complete = backend_result.complete;
    result.failed = backend_result.failed;
    result.used_native_directstorage = backend_result.used_native_directstorage;
    result.used_win32_async_io = backend_result.used_win32_async_io;
    result.signaled_native_fence = backend_result.signaled_native_fence;
    result.native_fence_signal_value = backend_result.native_fence_signal_value;
    result.native_fence_completed_value = backend_result.native_fence_completed_value;
    result.used_directstorage_resource_destination = backend_result.used_directstorage_resource_destination;
    result.directstorage_resource_destination_request_count =
        backend_result.directstorage_resource_destination_request_count;
    result.directstorage_resource_destination_bytes = backend_result.directstorage_resource_destination_bytes;
    result.executed_background_worker = backend_result.executed_background_worker;
    result.touched_renderer_or_rhi_handles = backend_result.touched_renderer_or_rhi_handles;

    if (backend_result.failed && result.diagnostics.empty()) {
        add_diagnostic(result, RuntimeMavgPayloadNativeIoDiagnosticCode::status_failed, result.ticket,
                       backend_result.hresult, "MAVG payload native IO backend reported failed status");
    }
    return result;
}

} // namespace mirakana::runtime
