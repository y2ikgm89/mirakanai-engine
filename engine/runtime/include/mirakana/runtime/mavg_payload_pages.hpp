// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/assets/mavg_cluster_payload.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeMavgPayloadPageSliceDiagnosticCode : std::uint8_t {
    missing_graph = 0,
    invalid_graph,
    invalid_payload,
    unknown_page,
    duplicate_requested_page,
    missing_payload_page,
};

enum class RuntimeMavgPayloadPageFileLoadDiagnosticCode : std::uint8_t {
    missing_filesystem = 0,
    missing_graph,
    missing_payload_blob_path,
    invalid_graph,
    invalid_payload,
    unknown_page,
    duplicate_requested_page,
    missing_payload_page,
    payload_file_read_failed,
};

enum class RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode : std::uint8_t {
    missing_graph = 0,
    missing_payload_blob_path,
    invalid_graph,
    invalid_payload,
    unknown_page,
    duplicate_requested_page,
    missing_payload_page,
    page_request_too_large,
    destination_range_overflow,
};

enum class RuntimeMavgPayloadDirectStorageFenceWaitPoint : std::uint8_t {
    before_destination_write = 0,
    before_gpu_work,
    before_source_access,
};

enum class RuntimeMavgPayloadNativeIoBackend : std::uint8_t {
    directstorage = 0,
    win32_async_file,
    test_adapter,
};

enum class RuntimeMavgPayloadNativeIoStatus : std::uint8_t {
    submitted = 0,
    complete,
    failed,
};

enum class RuntimeMavgPayloadNativeIoDiagnosticCode : std::uint8_t {
    missing_dispatcher = 0,
    missing_request_plan,
    invalid_request_plan,
    empty_request_plan,
    unsupported_backend,
    dispatch_failed,
    missing_ticket,
    unknown_ticket,
    status_failed,
};

struct RuntimeMavgPayloadPageSliceDiagnostic {
    RuntimeMavgPayloadPageSliceDiagnosticCode code{RuntimeMavgPayloadPageSliceDiagnosticCode::missing_graph};
    std::uint32_t page_index{0};
    std::string message;
};

struct RuntimeMavgPayloadPageFileLoadDiagnostic {
    RuntimeMavgPayloadPageFileLoadDiagnosticCode code{RuntimeMavgPayloadPageFileLoadDiagnosticCode::missing_filesystem};
    std::uint32_t page_index{0};
    std::string message;
};

struct RuntimeMavgPayloadDirectStorageRequestPlanDiagnostic {
    RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode code{
        RuntimeMavgPayloadDirectStorageRequestPlanDiagnosticCode::missing_graph};
    std::uint32_t page_index{0};
    std::string message;
};

struct RuntimeMavgPayloadNativeIoDiagnostic {
    RuntimeMavgPayloadNativeIoDiagnosticCode code{RuntimeMavgPayloadNativeIoDiagnosticCode::missing_dispatcher};
    std::uint32_t request_index{0};
    std::uint64_t ticket{0};
    std::int32_t hresult{0};
    std::string message;
};

struct RuntimeMavgPayloadPageSliceDesc {
    const MavgClusterGraphDocument* graph{nullptr};
    std::string_view payload_text;
    std::span<const std::uint32_t> page_indices;
};

struct RuntimeMavgPayloadPageFileLoadDesc {
    IFileSystem* filesystem{nullptr};
    const MavgClusterGraphDocument* graph{nullptr};
    std::string_view payload_text;
    std::string_view payload_blob_path;
    std::span<const std::uint32_t> page_indices;
};

struct RuntimeMavgPayloadDirectStorageRequestPlanDesc {
    const MavgClusterGraphDocument* graph{nullptr};
    std::string_view payload_text;
    std::string_view payload_blob_path;
    std::span<const std::uint32_t> page_indices;
    std::uint64_t destination_base_offset{0};
    RuntimeMavgPayloadDirectStorageFenceWaitPoint fence_wait_point{
        RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_destination_write};
    bool synchronize_with_fence{false};
};

struct RuntimeMavgPayloadNativeIoDispatchBackendDesc {
    RuntimeMavgPayloadNativeIoBackend required_backend{RuntimeMavgPayloadNativeIoBackend::directstorage};
    std::uint64_t submission_tag{0};
    std::span<std::uint8_t> destination_memory;
    bool require_native_directstorage{true};
    bool enqueue_status_after_requests{true};
    bool signal_fence_after_requests{false};
};

struct RuntimeMavgPayloadNativeIoDispatchDesc;
struct RuntimeMavgPayloadNativeIoStatusPollDesc;
struct RuntimeMavgPayloadNativeIoDispatchBackendResult;
struct RuntimeMavgPayloadNativeIoStatusBackendResult;

struct RuntimeMavgPayloadPageSliceRow {
    std::uint32_t page_index{0};
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
    std::vector<std::uint8_t> payload_bytes;
};

struct RuntimeMavgPayloadPageSliceResult {
    std::vector<RuntimeMavgPayloadPageSliceRow> pages;
    std::vector<RuntimeMavgPayloadPageSliceDiagnostic> diagnostics;
    std::size_t requested_page_count{0};
    std::size_t extracted_page_count{0};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool executed_background_worker{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RuntimeMavgPayloadPageFileLoadRow {
    std::uint32_t page_index{0};
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
    std::vector<std::uint8_t> payload_bytes;
};

struct RuntimeMavgPayloadPageFileLoadResult {
    std::vector<RuntimeMavgPayloadPageFileLoadRow> pages;
    std::vector<RuntimeMavgPayloadPageFileLoadDiagnostic> diagnostics;
    std::size_t requested_page_count{0};
    std::size_t loaded_page_count{0};
    bool invoked_file_io{false};
    bool used_native_directstorage{false};
    bool mutated_mount_set{false};
    bool executed_background_worker{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RuntimeMavgPayloadDirectStorageRequestRow {
    std::uint32_t request_index{0};
    std::uint32_t page_index{0};
    std::uint64_t source_file_offset{0};
    std::uint32_t source_size{0};
    std::string source_file_path;
    std::uint64_t destination_offset{0};
    std::uint32_t destination_size{0};
    RuntimeMavgPayloadDirectStorageFenceWaitPoint fence_wait_point{
        RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_destination_write};
    bool source_is_file{true};
    bool destination_is_memory{true};
    bool synchronized_with_fence{false};
    std::string debug_name;
};

struct RuntimeMavgPayloadDirectStorageRequestPlanResult {
    std::vector<RuntimeMavgPayloadDirectStorageRequestRow> requests;
    std::vector<RuntimeMavgPayloadDirectStorageRequestPlanDiagnostic> diagnostics;
    std::size_t requested_page_count{0};
    std::size_t planned_request_count{0};
    std::uint64_t total_source_bytes{0};
    std::uint64_t total_destination_bytes{0};
    bool invoked_file_io{false};
    bool used_native_directstorage{false};
    bool requires_native_directstorage_sdk{false};
    bool enqueued_native_requests{false};
    bool submitted_native_queue{false};
    bool signaled_native_fence{false};
    bool mutated_mount_set{false};
    bool executed_background_worker{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RuntimeMavgPayloadNativeIoDispatchBackendResult {
    std::vector<RuntimeMavgPayloadNativeIoDiagnostic> diagnostics;
    std::uint64_t ticket{0};
    RuntimeMavgPayloadNativeIoBackend backend{RuntimeMavgPayloadNativeIoBackend::directstorage};
    std::size_t enqueued_request_count{0};
    std::uint64_t submitted_source_bytes{0};
    std::uint64_t submitted_destination_bytes{0};
    bool submitted_io_queue{false};
    bool enqueued_native_requests{false};
    bool submitted_native_queue{false};
    bool enqueued_status_write{false};
    bool signaled_native_fence{false};
    bool used_native_directstorage{false};
    bool used_win32_async_io{false};
    bool executed_background_worker{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && submitted_io_queue;
    }
};

struct RuntimeMavgPayloadNativeIoStatusBackendResult {
    std::vector<RuntimeMavgPayloadNativeIoDiagnostic> diagnostics;
    std::uint64_t ticket{0};
    RuntimeMavgPayloadNativeIoStatus status{RuntimeMavgPayloadNativeIoStatus::submitted};
    std::int32_t hresult{0};
    bool complete{false};
    bool failed{false};
    bool used_native_directstorage{false};
    bool used_win32_async_io{false};
    bool signaled_native_fence{false};
    bool executed_background_worker{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && !failed;
    }
};

class IRuntimeMavgPayloadNativeIoDispatcher {
  public:
    virtual ~IRuntimeMavgPayloadNativeIoDispatcher() = default;

    [[nodiscard]] virtual RuntimeMavgPayloadNativeIoBackend backend() const noexcept = 0;

    [[nodiscard]] virtual RuntimeMavgPayloadNativeIoDispatchBackendResult
    dispatch(std::span<const RuntimeMavgPayloadDirectStorageRequestRow> requests,
             const RuntimeMavgPayloadNativeIoDispatchBackendDesc& desc) = 0;

    [[nodiscard]] virtual RuntimeMavgPayloadNativeIoStatusBackendResult poll_status(std::uint64_t ticket) = 0;
};

struct RuntimeMavgPayloadNativeIoDispatchDesc {
    IRuntimeMavgPayloadNativeIoDispatcher* dispatcher{nullptr};
    const RuntimeMavgPayloadDirectStorageRequestPlanResult* request_plan{nullptr};
    RuntimeMavgPayloadNativeIoBackend required_backend{RuntimeMavgPayloadNativeIoBackend::directstorage};
    std::uint64_t submission_tag{0};
    std::span<std::uint8_t> destination_memory;
    bool require_native_directstorage{true};
    bool enqueue_status_after_requests{true};
    bool signal_fence_after_requests{false};
};

struct RuntimeMavgPayloadNativeIoDispatchResult {
    std::vector<RuntimeMavgPayloadNativeIoDiagnostic> diagnostics;
    std::uint64_t ticket{0};
    RuntimeMavgPayloadNativeIoBackend backend{RuntimeMavgPayloadNativeIoBackend::directstorage};
    std::size_t request_count{0};
    std::uint64_t total_source_bytes{0};
    std::uint64_t total_destination_bytes{0};
    bool submitted_io_queue{false};
    bool enqueued_native_requests{false};
    bool submitted_native_queue{false};
    bool enqueued_status_write{false};
    bool signaled_native_fence{false};
    bool used_native_directstorage{false};
    bool used_win32_async_io{false};
    bool mutated_mount_set{false};
    bool executed_background_worker{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && submitted_io_queue;
    }
};

struct RuntimeMavgPayloadNativeIoStatusPollDesc {
    IRuntimeMavgPayloadNativeIoDispatcher* dispatcher{nullptr};
    std::uint64_t ticket{0};
};

struct RuntimeMavgPayloadNativeIoStatusPollResult {
    std::vector<RuntimeMavgPayloadNativeIoDiagnostic> diagnostics;
    std::uint64_t ticket{0};
    RuntimeMavgPayloadNativeIoStatus status{RuntimeMavgPayloadNativeIoStatus::submitted};
    std::int32_t hresult{0};
    bool complete{false};
    bool failed{false};
    bool used_native_directstorage{false};
    bool used_win32_async_io{false};
    bool signaled_native_fence{false};
    bool mutated_mount_set{false};
    bool executed_background_worker{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && !failed;
    }
};

[[nodiscard]] RuntimeMavgPayloadPageSliceResult
extract_runtime_mavg_payload_page_slices(const RuntimeMavgPayloadPageSliceDesc& desc);

[[nodiscard]] RuntimeMavgPayloadPageFileLoadResult
load_runtime_mavg_payload_file_pages(const RuntimeMavgPayloadPageFileLoadDesc& desc);

[[nodiscard]] RuntimeMavgPayloadDirectStorageRequestPlanResult
plan_runtime_mavg_payload_directstorage_requests(const RuntimeMavgPayloadDirectStorageRequestPlanDesc& desc);

[[nodiscard]] RuntimeMavgPayloadNativeIoDispatchResult
dispatch_runtime_mavg_payload_native_io_requests(const RuntimeMavgPayloadNativeIoDispatchDesc& desc);

[[nodiscard]] RuntimeMavgPayloadNativeIoStatusPollResult
poll_runtime_mavg_payload_native_io_status(const RuntimeMavgPayloadNativeIoStatusPollDesc& desc);

} // namespace mirakana::runtime
