// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/win32/win32_mavg_payload_io.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d12.h>
#include <dstorage.h>
#include <dstorageerr.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] std::int32_t hresult_to_i32(HRESULT result) noexcept {
    return static_cast<std::int32_t>(result);
}

void add_diagnostic(runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult& result,
                    runtime::RuntimeMavgPayloadNativeIoDiagnosticCode code, std::uint32_t request_index,
                    HRESULT hresult, std::string message) {
    result.diagnostics.push_back(runtime::RuntimeMavgPayloadNativeIoDiagnostic{
        .code = code,
        .request_index = request_index,
        .hresult = hresult_to_i32(hresult),
        .message = std::move(message),
    });
}

void add_diagnostic(runtime::RuntimeMavgPayloadNativeIoStatusBackendResult& result,
                    runtime::RuntimeMavgPayloadNativeIoDiagnosticCode code, std::uint32_t request_index,
                    std::uint64_t ticket, HRESULT hresult, std::string message) {
    result.diagnostics.push_back(runtime::RuntimeMavgPayloadNativeIoDiagnostic{
        .code = code,
        .request_index = request_index,
        .ticket = ticket,
        .hresult = hresult_to_i32(hresult),
        .message = std::move(message),
    });
}

[[nodiscard]] bool is_single_line_relative_path(std::string_view path) {
    if (path.empty()) {
        return false;
    }
    if (path.find('\n') != std::string_view::npos || path.find('\r') != std::string_view::npos ||
        path.find('\0') != std::string_view::npos) {
        return false;
    }

    const std::filesystem::path parsed{std::string(path)};
    if (parsed.is_absolute() || parsed.has_root_name() || parsed.has_root_directory()) {
        return false;
    }
    for (const auto& part : parsed) {
        if (part == "..") {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool range_fits_destination(std::span<std::uint8_t> destination, std::uint64_t offset,
                                          std::uint32_t size) noexcept {
    const auto destination_size = static_cast<std::uint64_t>(destination.size());
    return offset <= destination_size && static_cast<std::uint64_t>(size) <= destination_size - offset;
}

[[nodiscard]] std::uint8_t* destination_pointer(std::span<std::uint8_t> destination, std::uint64_t offset) {
    return destination.data() + static_cast<std::size_t>(offset);
}

[[nodiscard]] bool destination_range_end(std::uint64_t offset, std::uint32_t size, std::uint64_t& end) noexcept {
    const auto byte_size = static_cast<std::uint64_t>(size);
    if (offset > std::numeric_limits<std::uint64_t>::max() - byte_size) {
        return false;
    }
    end = offset + byte_size;
    return true;
}

[[nodiscard]] std::uint16_t clamp_queue_capacity(std::size_t requested_capacity) noexcept {
    const auto requested =
        requested_capacity == 0U ? static_cast<std::size_t>(DSTORAGE_MIN_QUEUE_CAPACITY) : requested_capacity;
    const auto clamped = std::clamp(requested, static_cast<std::size_t>(DSTORAGE_MIN_QUEUE_CAPACITY),
                                    static_cast<std::size_t>(DSTORAGE_MAX_QUEUE_CAPACITY));
    return static_cast<std::uint16_t>(clamped);
}

[[nodiscard]] std::string request_debug_name(const runtime::RuntimeMavgPayloadDirectStorageRequestRow& request) {
    if (!request.debug_name.empty()) {
        return request.debug_name;
    }
    return "mavg.payload.page." + std::to_string(request.page_index);
}

struct DirectStoragePendingSubmission {
    Microsoft::WRL::ComPtr<IDStorageQueue> queue;
    Microsoft::WRL::ComPtr<IDStorageStatusArray> status_array;
    Microsoft::WRL::ComPtr<ID3D12Fence> completion_fence;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_destination;
    std::vector<Microsoft::WRL::ComPtr<IDStorageFile>> files;
    std::vector<std::string> request_names;
    std::uint64_t native_fence_signal_value{0};
    bool used_directstorage_resource_destination{false};
    std::size_t directstorage_resource_destination_request_count{0};
    std::uint64_t directstorage_resource_destination_bytes{0};
};

} // namespace

struct Win32MavgPayloadDirectStorageDispatcher::Impl {
    explicit Impl(Win32MavgPayloadDirectStorageDispatcherDesc init_desc) : desc(std::move(init_desc)) {
        if (desc.max_inflight_submissions == 0U) {
            desc.max_inflight_submissions = 1U;
        }
        desc.queue_capacity = clamp_queue_capacity(desc.queue_capacity);
    }

    ~Impl() {
        close_all_pending_submissions();
    }

    struct SubmissionReservation {
        Impl* owner{nullptr};
        std::uint64_t ticket{0};
        bool committed{false};

        ~SubmissionReservation() {
            if (owner != nullptr && ticket != 0U && !committed) {
                owner->erase_pending_submission(ticket);
            }
        }

        SubmissionReservation(const SubmissionReservation&) = delete;
        SubmissionReservation& operator=(const SubmissionReservation&) = delete;
        SubmissionReservation(SubmissionReservation&&) = delete;
        SubmissionReservation& operator=(SubmissionReservation&&) = delete;

        void commit() noexcept {
            committed = true;
        }
    };

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult
    dispatch(std::span<const runtime::RuntimeMavgPayloadDirectStorageRequestRow> requests,
             const runtime::RuntimeMavgPayloadNativeIoDispatchBackendDesc& backend_desc) {
        runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult result{
            .backend = runtime::RuntimeMavgPayloadNativeIoBackend::directstorage,
            .used_native_directstorage = false,
            .used_win32_async_io = false,
            .executed_background_worker = false,
            .touched_renderer_or_rhi_handles = false,
        };

        if (desc.root_path.empty()) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, E_INVALIDARG,
                           "DirectStorage MAVG IO root path is empty");
            return result;
        }
        const auto use_resource_destination = backend_desc.use_directstorage_d3d12_buffer_destination;
        if (!use_resource_destination && backend_desc.destination_memory.empty()) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, E_INVALIDARG,
                           "DirectStorage MAVG IO requires caller-owned destination memory");
            return result;
        }
        if (!backend_desc.enqueue_status_after_requests) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, E_INVALIDARG,
                           "DirectStorage MAVG IO requires a status write for polling");
            return result;
        }
        std::uint64_t required_resource_destination_bytes = 0;
        for (const auto& request : requests) {
            if (!request.source_is_file || request.source_size == 0U ||
                request.destination_size != request.source_size) {
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, E_INVALIDARG,
                               "DirectStorage MAVG IO request must be a non-empty file-to-destination read");
                return result;
            }
            if (!use_resource_destination && !request.destination_is_memory) {
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, E_INVALIDARG,
                               "DirectStorage MAVG IO memory mode request must target caller-owned memory");
                return result;
            }
            if (!is_single_line_relative_path(request.source_file_path)) {
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, E_INVALIDARG,
                               "DirectStorage MAVG IO source path must be a relative single-line path");
                return result;
            }
            std::uint64_t destination_end = 0;
            if (!destination_range_end(request.destination_offset, request.destination_size, destination_end)) {
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, E_INVALIDARG,
                               "DirectStorage MAVG IO destination range overflows");
                return result;
            }
            if (use_resource_destination) {
                required_resource_destination_bytes = std::max(required_resource_destination_bytes, destination_end);
            } else if (!range_fits_destination(backend_desc.destination_memory, request.destination_offset,
                                               request.destination_size)) {
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, E_INVALIDARG,
                               "DirectStorage MAVG IO destination range is outside caller-owned memory");
                return result;
            }
        }
        if (use_resource_destination && required_resource_destination_bytes == 0U) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, E_INVALIDARG,
                           "DirectStorage MAVG IO resource destination size is empty");
            return result;
        }

        Microsoft::WRL::ComPtr<ID3D12Resource> resource_destination;
        if (use_resource_destination &&
            !create_resource_destination(result, required_resource_destination_bytes, resource_destination)) {
            return result;
        }
        Microsoft::WRL::ComPtr<ID3D12Fence> completion_fence;
        if (backend_desc.signal_fence_after_requests && !create_completion_fence(result, completion_fence)) {
            return result;
        }

        if (!ensure_factory(result)) {
            return result;
        }

        SubmissionReservation reservation{
            .owner = this,
            .ticket = reserve_pending_submission(result),
        };
        if (reservation.ticket == 0U) {
            return result;
        }

        DirectStoragePendingSubmission submission;
        submission.files.reserve(requests.size());
        submission.request_names.reserve(requests.size());
        submission.completion_fence = std::move(completion_fence);
        submission.resource_destination = std::move(resource_destination);
        submission.used_directstorage_resource_destination = use_resource_destination;
        if (submission.completion_fence.Get() != nullptr) {
            submission.native_fence_signal_value = 1U;
        }

        DSTORAGE_QUEUE_DESC queue_desc{};
        queue_desc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        queue_desc.Capacity = static_cast<UINT16>(desc.queue_capacity);
        queue_desc.Priority = DSTORAGE_PRIORITY_NORMAL;
        queue_desc.Name = use_resource_destination ? "mirakana.mavg.directstorage.file_to_d3d12_buffer"
                                                   : "mirakana.mavg.directstorage.file_to_memory";
        queue_desc.Device = use_resource_destination ? fence_device.Get() : nullptr;
        HRESULT hresult = factory->CreateQueue(&queue_desc, IID_PPV_ARGS(submission.queue.ReleaseAndGetAddressOf()));
        if (FAILED(hresult)) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, hresult,
                           "DirectStorage MAVG IO failed to create queue");
            return result;
        }

        hresult = factory->CreateStatusArray(1U, "mirakana.mavg.directstorage.status",
                                             IID_PPV_ARGS(submission.status_array.ReleaseAndGetAddressOf()));
        if (FAILED(hresult)) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, hresult,
                           "DirectStorage MAVG IO failed to create status array");
            return result;
        }

        for (const auto& request : requests) {
            Microsoft::WRL::ComPtr<IDStorageFile> file;
            const auto full_path = desc.root_path / std::filesystem::path{request.source_file_path};
            hresult = factory->OpenFile(full_path.wstring().c_str(), IID_PPV_ARGS(file.ReleaseAndGetAddressOf()));
            if (FAILED(hresult)) {
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, hresult, "DirectStorage MAVG IO failed to open source file");
                return result;
            }

            submission.request_names.push_back(request_debug_name(request));
            DSTORAGE_REQUEST storage_request{};
            storage_request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
            storage_request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
            storage_request.Source.File.Source = file.Get();
            storage_request.Source.File.Offset = request.source_file_offset;
            storage_request.Source.File.Size = request.source_size;
            if (use_resource_destination) {
                storage_request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
                storage_request.Destination.Buffer.Resource = submission.resource_destination.Get();
                storage_request.Destination.Buffer.Offset = request.destination_offset;
                storage_request.Destination.Buffer.Size = request.destination_size;
            } else {
                storage_request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
                storage_request.Destination.Memory.Buffer =
                    destination_pointer(backend_desc.destination_memory, request.destination_offset);
                storage_request.Destination.Memory.Size = request.destination_size;
            }
            storage_request.UncompressedSize = 0U;
            storage_request.CancellationTag = backend_desc.submission_tag;
            storage_request.Name = submission.request_names.back().c_str();
            submission.queue->EnqueueRequest(&storage_request);
            submission.files.push_back(std::move(file));

            result.enqueued_request_count += 1U;
            result.submitted_source_bytes += request.source_size;
            result.submitted_destination_bytes += request.destination_size;
            if (use_resource_destination) {
                result.directstorage_resource_destination_request_count += 1U;
            }
        }
        if (use_resource_destination) {
            result.used_directstorage_resource_destination = true;
            result.directstorage_resource_destination_bytes = result.submitted_destination_bytes;
            submission.directstorage_resource_destination_request_count =
                result.directstorage_resource_destination_request_count;
            submission.directstorage_resource_destination_bytes = result.directstorage_resource_destination_bytes;
        }

        if (submission.completion_fence.Get() != nullptr) {
            submission.queue->EnqueueSignal(submission.completion_fence.Get(), submission.native_fence_signal_value);
        }
        submission.queue->EnqueueStatus(submission.status_array.Get(), 0U);
        const auto native_fence_signal_value = submission.native_fence_signal_value;
        const auto native_fence_completed_value =
            submission.completion_fence.Get() != nullptr ? submission.completion_fence->GetCompletedValue() : 0U;

        {
            std::lock_guard lock(mutex);
            auto found = submissions.find(reservation.ticket);
            if (found == submissions.end()) {
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0,
                               HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
                               "DirectStorage MAVG IO reserved submission slot was lost");
                return result;
            }
            found->second = std::move(submission);
            found->second.queue->Submit();
        }
        reservation.commit();

        result.ticket = reservation.ticket;
        result.submitted_io_queue = true;
        result.enqueued_native_requests = true;
        result.submitted_native_queue = true;
        result.enqueued_status_write = true;
        result.signaled_native_fence = native_fence_signal_value != 0U;
        result.native_fence_signal_value = native_fence_signal_value;
        result.native_fence_completed_value = native_fence_completed_value;
        result.used_native_directstorage = true;
        result.used_win32_async_io = false;
        return result;
    }

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoStatusBackendResult poll_status(std::uint64_t ticket) {
        runtime::RuntimeMavgPayloadNativeIoStatusBackendResult result{
            .ticket = ticket,
            .status = runtime::RuntimeMavgPayloadNativeIoStatus::submitted,
            .used_native_directstorage = true,
            .used_win32_async_io = false,
            .signaled_native_fence = false,
            .executed_background_worker = false,
            .touched_renderer_or_rhi_handles = false,
        };

        std::lock_guard lock(mutex);
        const auto found = submissions.find(ticket);
        if (found == submissions.end()) {
            result.status = runtime::RuntimeMavgPayloadNativeIoStatus::failed;
            result.failed = true;
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::unknown_ticket, 0, ticket,
                           HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "DirectStorage MAVG IO ticket is unknown");
            return result;
        }

        auto& submission = found->second;
        result.signaled_native_fence = submission.native_fence_signal_value != 0U;
        result.native_fence_signal_value = submission.native_fence_signal_value;
        result.used_directstorage_resource_destination = submission.used_directstorage_resource_destination;
        result.directstorage_resource_destination_request_count =
            submission.directstorage_resource_destination_request_count;
        result.directstorage_resource_destination_bytes = submission.directstorage_resource_destination_bytes;
        if (submission.completion_fence.Get() != nullptr) {
            result.native_fence_completed_value = submission.completion_fence->GetCompletedValue();
        }

        if (!submission.status_array->IsComplete(0U)) {
            return result;
        }

        const auto hresult = submission.status_array->GetHResult(0U);
        result.hresult = hresult_to_i32(hresult);
        result.complete = true;
        if (FAILED(hresult)) {
            result.status = runtime::RuntimeMavgPayloadNativeIoStatus::failed;
            result.failed = true;
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::status_failed, 0, ticket, hresult,
                           "DirectStorage MAVG IO status array reported failure");
            submissions.erase(found);
            return result;
        }

        result.status = runtime::RuntimeMavgPayloadNativeIoStatus::complete;
        if (submission.completion_fence.Get() != nullptr) {
            result.native_fence_completed_value = submission.completion_fence->GetCompletedValue();
        }
        submissions.erase(found);
        return result;
    }

    [[nodiscard]] bool create_resource_destination(runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult& result,
                                                   std::uint64_t byte_size,
                                                   Microsoft::WRL::ComPtr<ID3D12Resource>& resource_destination) {
        if (!ensure_fence_device(result)) {
            return false;
        }

        D3D12_HEAP_PROPERTIES heap_properties{};
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CreationNodeMask = 1U;
        heap_properties.VisibleNodeMask = 1U;

        D3D12_RESOURCE_DESC resource_desc{};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Width = byte_size;
        resource_desc.Height = 1U;
        resource_desc.DepthOrArraySize = 1U;
        resource_desc.MipLevels = 1U;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count = 1U;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        const auto hresult = fence_device->CreateCommittedResource(
            &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(resource_destination.ReleaseAndGetAddressOf()));
        if (FAILED(hresult)) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, hresult,
                           "DirectStorage MAVG IO failed to create private D3D12 buffer destination");
            return false;
        }
        return true;
    }

    [[nodiscard]] bool create_completion_fence(runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult& result,
                                               Microsoft::WRL::ComPtr<ID3D12Fence>& completion_fence) {
        if (!ensure_fence_device(result)) {
            return false;
        }

        const auto hresult = fence_device->CreateFence(0U, D3D12_FENCE_FLAG_NONE,
                                                       IID_PPV_ARGS(completion_fence.ReleaseAndGetAddressOf()));
        if (FAILED(hresult)) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, hresult,
                           "DirectStorage MAVG IO failed to create private D3D12 completion fence");
            return false;
        }
        return true;
    }

    [[nodiscard]] bool ensure_fence_device(runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult& result) {
        std::lock_guard lock(mutex);
        if (fence_device.Get() != nullptr) {
            return true;
        }

        HRESULT hresult =
            D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(fence_device.ReleaseAndGetAddressOf()));
        if (SUCCEEDED(hresult)) {
            return true;
        }

        Microsoft::WRL::ComPtr<IDXGIFactory6> dxgi_factory;
        hresult = CreateDXGIFactory2(0U, IID_PPV_ARGS(dxgi_factory.ReleaseAndGetAddressOf()));
        if (FAILED(hresult)) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, hresult,
                           "DirectStorage MAVG IO failed to create a DXGI factory for private fence fallback");
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
        hresult = dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(warp_adapter.ReleaseAndGetAddressOf()));
        if (FAILED(hresult)) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, hresult,
                           "DirectStorage MAVG IO failed to select WARP for private fence fallback");
            return false;
        }

        hresult = D3D12CreateDevice(warp_adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                    IID_PPV_ARGS(fence_device.ReleaseAndGetAddressOf()));
        if (FAILED(hresult)) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, hresult,
                           "DirectStorage MAVG IO failed to create a private D3D12 fence device");
            return false;
        }
        return true;
    }

    [[nodiscard]] bool ensure_factory(runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult& result) {
        std::lock_guard lock(mutex);
        if (factory.Get() != nullptr) {
            return true;
        }

        const auto hresult = DStorageGetFactory(IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
        if (FAILED(hresult)) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, hresult,
                           "DirectStorage MAVG IO failed to create factory");
            return false;
        }
        return true;
    }

    [[nodiscard]] std::uint64_t
    reserve_pending_submission(runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult& result) {
        std::lock_guard lock(mutex);
        if (submissions.size() >= desc.max_inflight_submissions) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0, E_OUTOFMEMORY,
                           "DirectStorage MAVG IO submission limit reached");
            return 0U;
        }
        if (next_ticket == 0U) {
            next_ticket = 1U;
        }
        const auto ticket = next_ticket++;
        submissions.emplace(ticket, DirectStoragePendingSubmission{});
        return ticket;
    }

    static void close_pending_submission(DirectStoragePendingSubmission& submission) noexcept {
        if (submission.queue.Get() != nullptr) {
            submission.queue->Close();
        }
    }

    void erase_pending_submission(std::uint64_t ticket) noexcept {
        std::lock_guard lock(mutex);
        const auto found = submissions.find(ticket);
        if (found == submissions.end()) {
            return;
        }
        close_pending_submission(found->second);
        submissions.erase(found);
    }

    void close_all_pending_submissions() noexcept {
        std::lock_guard lock(mutex);
        for (auto& [ticket, submission] : submissions) {
            (void)ticket;
            close_pending_submission(submission);
        }
        submissions.clear();
    }

    Win32MavgPayloadDirectStorageDispatcherDesc desc;
    Microsoft::WRL::ComPtr<IDStorageFactory> factory;
    Microsoft::WRL::ComPtr<ID3D12Device> fence_device;
    std::mutex mutex;
    std::uint64_t next_ticket{1};
    std::unordered_map<std::uint64_t, DirectStoragePendingSubmission> submissions;
};

Win32MavgPayloadDirectStorageDispatcher::Win32MavgPayloadDirectStorageDispatcher(
    Win32MavgPayloadDirectStorageDispatcherDesc desc)
    : impl_(std::make_unique<Impl>(std::move(desc))) {}

Win32MavgPayloadDirectStorageDispatcher::~Win32MavgPayloadDirectStorageDispatcher() = default;

Win32MavgPayloadDirectStorageDispatcher::Win32MavgPayloadDirectStorageDispatcher(
    Win32MavgPayloadDirectStorageDispatcher&&) noexcept = default;

Win32MavgPayloadDirectStorageDispatcher&
Win32MavgPayloadDirectStorageDispatcher::operator=(Win32MavgPayloadDirectStorageDispatcher&&) noexcept = default;

runtime::RuntimeMavgPayloadNativeIoBackend Win32MavgPayloadDirectStorageDispatcher::backend() const noexcept {
    return runtime::RuntimeMavgPayloadNativeIoBackend::directstorage;
}

runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult Win32MavgPayloadDirectStorageDispatcher::dispatch(
    std::span<const runtime::RuntimeMavgPayloadDirectStorageRequestRow> requests,
    const runtime::RuntimeMavgPayloadNativeIoDispatchBackendDesc& desc) {
    return impl_->dispatch(requests, desc);
}

runtime::RuntimeMavgPayloadNativeIoStatusBackendResult
Win32MavgPayloadDirectStorageDispatcher::poll_status(std::uint64_t ticket) {
    return impl_->poll_status(ticket);
}

} // namespace mirakana
