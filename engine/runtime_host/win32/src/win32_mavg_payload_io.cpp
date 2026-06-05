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

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

class UniqueHandle final {
  public:
    UniqueHandle() = default;
    explicit UniqueHandle(HANDLE handle) noexcept : handle_(handle) {}

    ~UniqueHandle() {
        reset();
    }

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            reset(std::exchange(other.handle_, nullptr));
        }
        return *this;
    }

    [[nodiscard]] HANDLE get() const noexcept {
        return handle_;
    }

    [[nodiscard]] bool valid() const noexcept {
        return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
    }

    void reset(HANDLE handle = nullptr) noexcept {
        if (valid()) {
            CloseHandle(handle_);
        }
        handle_ = handle;
    }

  private:
    HANDLE handle_{nullptr};
};

struct PendingRead {
    UniqueHandle file;
    UniqueHandle event;
    OVERLAPPED overlapped{};
    std::uint32_t request_index{0};
    std::uint32_t expected_bytes{0};
    std::uint32_t transferred_bytes{0};
    bool complete{false};
};

struct PendingSubmission {
    std::vector<PendingRead> reads;
};

[[nodiscard]] std::int32_t hresult_from_win32(DWORD error) noexcept {
    return static_cast<std::int32_t>(HRESULT_FROM_WIN32(error));
}

void add_diagnostic(runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult& result,
                    runtime::RuntimeMavgPayloadNativeIoDiagnosticCode code, std::uint32_t request_index, DWORD error,
                    std::string message) {
    result.diagnostics.push_back(runtime::RuntimeMavgPayloadNativeIoDiagnostic{
        .code = code,
        .request_index = request_index,
        .hresult = hresult_from_win32(error),
        .message = std::move(message),
    });
}

void add_diagnostic(runtime::RuntimeMavgPayloadNativeIoStatusBackendResult& result,
                    runtime::RuntimeMavgPayloadNativeIoDiagnosticCode code, std::uint32_t request_index,
                    std::uint64_t ticket, DWORD error, std::string message) {
    result.diagnostics.push_back(runtime::RuntimeMavgPayloadNativeIoDiagnostic{
        .code = code,
        .request_index = request_index,
        .ticket = ticket,
        .hresult = hresult_from_win32(error),
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
    if (parsed.is_absolute()) {
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

void cancel_pending(PendingSubmission& submission) noexcept {
    for (auto& read : submission.reads) {
        if (read.complete || !read.file.valid()) {
            continue;
        }

        CancelIoEx(read.file.get(), &read.overlapped);
        DWORD ignored_bytes = 0;
        GetOverlappedResult(read.file.get(), &read.overlapped, &ignored_bytes, TRUE);
        read.complete = true;
    }
}

} // namespace

struct Win32MavgPayloadAsyncFileIoDispatcher::Impl {
    explicit Impl(Win32MavgPayloadAsyncFileIoDispatcherDesc init_desc) : desc(std::move(init_desc)) {
        if (desc.max_inflight_submissions == 0U) {
            desc.max_inflight_submissions = 1U;
        }
    }

    ~Impl() {
        for (auto& [_, submission] : submissions) {
            cancel_pending(submission);
        }
    }

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult
    dispatch(std::span<const runtime::RuntimeMavgPayloadDirectStorageRequestRow> requests,
             const runtime::RuntimeMavgPayloadNativeIoDispatchBackendDesc& backend_desc) {
        runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult result{
            .backend = runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file,
            .used_win32_async_io = false,
            .executed_background_worker = false,
            .touched_renderer_or_rhi_handles = false,
        };

        if (desc.root_path.empty()) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0,
                           ERROR_INVALID_PARAMETER, "Win32 MAVG async IO root path is empty");
            return result;
        }
        if (submissions.size() >= desc.max_inflight_submissions) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0,
                           ERROR_NOT_ENOUGH_MEMORY, "Win32 MAVG async IO submission limit reached");
            return result;
        }
        if (backend_desc.destination_memory.empty()) {
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed, 0,
                           ERROR_INVALID_PARAMETER, "Win32 MAVG async IO requires caller-owned destination memory");
            return result;
        }

        PendingSubmission submission;
        submission.reads.reserve(requests.size());
        for (const auto& request : requests) {
            if (!request.source_is_file || !request.destination_is_memory || request.source_size == 0U ||
                request.destination_size != request.source_size) {
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, ERROR_INVALID_PARAMETER,
                               "Win32 MAVG async IO request must be a non-empty file-to-memory read");
                cancel_pending(submission);
                return result;
            }
            if (!is_single_line_relative_path(request.source_file_path)) {
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, ERROR_INVALID_PARAMETER,
                               "Win32 MAVG async IO source path must be a relative single-line path");
                cancel_pending(submission);
                return result;
            }
            if (!range_fits_destination(backend_desc.destination_memory, request.destination_offset,
                                        request.destination_size)) {
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, ERROR_INVALID_PARAMETER,
                               "Win32 MAVG async IO destination range is outside caller-owned memory");
                cancel_pending(submission);
                return result;
            }

            auto& pending = submission.reads.emplace_back();
            pending.request_index = request.request_index;
            pending.expected_bytes = request.source_size;
            pending.overlapped.Offset = static_cast<DWORD>(request.source_file_offset & 0xffffffffULL);
            pending.overlapped.OffsetHigh = static_cast<DWORD>((request.source_file_offset >> 32U) & 0xffffffffULL);

            const auto full_path = desc.root_path / std::filesystem::path{request.source_file_path};
            pending.file =
                UniqueHandle{CreateFileW(full_path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr)};
            if (!pending.file.valid()) {
                const auto error = GetLastError();
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, error, "Win32 MAVG async IO failed to open source file");
                cancel_pending(submission);
                return result;
            }

            pending.event = UniqueHandle{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
            if (!pending.event.valid()) {
                const auto error = GetLastError();
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                               request.request_index, error, "Win32 MAVG async IO failed to create completion event");
                cancel_pending(submission);
                return result;
            }
            pending.overlapped.hEvent = pending.event.get();

            const auto read_started = ReadFile(
                pending.file.get(), destination_pointer(backend_desc.destination_memory, request.destination_offset),
                request.source_size, nullptr, &pending.overlapped);
            if (read_started != 0) {
                DWORD transferred = 0;
                if (GetOverlappedResult(pending.file.get(), &pending.overlapped, &transferred, FALSE) == 0) {
                    const auto error = GetLastError();
                    add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                                   request.request_index, error,
                                   "Win32 MAVG async IO immediate read completion query failed");
                    cancel_pending(submission);
                    return result;
                }
                if (transferred != request.source_size) {
                    add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                                   request.request_index, ERROR_HANDLE_EOF,
                                   "Win32 MAVG async IO completed fewer bytes than requested");
                    cancel_pending(submission);
                    return result;
                }
                pending.transferred_bytes = transferred;
                pending.complete = true;
            } else {
                const auto error = GetLastError();
                if (error != ERROR_IO_PENDING) {
                    add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::dispatch_failed,
                                   request.request_index, error, "Win32 MAVG async IO ReadFile failed");
                    cancel_pending(submission);
                    return result;
                }
            }

            result.enqueued_request_count += 1U;
            result.submitted_source_bytes += request.source_size;
            result.submitted_destination_bytes += request.destination_size;
        }

        const auto ticket = next_ticket++;
        result.ticket = ticket;
        result.submitted_io_queue = true;
        result.enqueued_native_requests = true;
        result.submitted_native_queue = false;
        result.enqueued_status_write = backend_desc.enqueue_status_after_requests;
        result.signaled_native_fence = false;
        result.used_native_directstorage = false;
        result.used_win32_async_io = true;
        submissions.emplace(ticket, std::move(submission));
        return result;
    }

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoStatusBackendResult poll_status(std::uint64_t ticket) {
        runtime::RuntimeMavgPayloadNativeIoStatusBackendResult result{
            .ticket = ticket,
            .status = runtime::RuntimeMavgPayloadNativeIoStatus::submitted,
            .used_native_directstorage = false,
            .used_win32_async_io = true,
            .signaled_native_fence = false,
            .executed_background_worker = false,
            .touched_renderer_or_rhi_handles = false,
        };

        auto it = submissions.find(ticket);
        if (it == submissions.end()) {
            result.status = runtime::RuntimeMavgPayloadNativeIoStatus::failed;
            result.failed = true;
            add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::unknown_ticket, 0, ticket,
                           ERROR_NOT_FOUND, "Win32 MAVG async IO ticket is unknown");
            return result;
        }

        auto& submission = it->second;
        bool all_complete = true;
        for (auto& read : submission.reads) {
            if (read.complete) {
                continue;
            }

            DWORD transferred = 0;
            if (GetOverlappedResult(read.file.get(), &read.overlapped, &transferred, FALSE) == 0) {
                const auto error = GetLastError();
                if (error == ERROR_IO_INCOMPLETE) {
                    all_complete = false;
                    continue;
                }

                result.status = runtime::RuntimeMavgPayloadNativeIoStatus::failed;
                result.complete = true;
                result.failed = true;
                result.hresult = hresult_from_win32(error);
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::status_failed,
                               read.request_index, ticket, error, "Win32 MAVG async IO status query failed");
                cancel_pending(submission);
                submissions.erase(it);
                return result;
            }

            if (transferred != read.expected_bytes) {
                result.status = runtime::RuntimeMavgPayloadNativeIoStatus::failed;
                result.complete = true;
                result.failed = true;
                result.hresult = hresult_from_win32(ERROR_HANDLE_EOF);
                add_diagnostic(result, runtime::RuntimeMavgPayloadNativeIoDiagnosticCode::status_failed,
                               read.request_index, ticket, ERROR_HANDLE_EOF,
                               "Win32 MAVG async IO completed fewer bytes than requested");
                cancel_pending(submission);
                submissions.erase(it);
                return result;
            }

            read.transferred_bytes = transferred;
            read.complete = true;
        }

        if (!all_complete) {
            return result;
        }

        result.status = runtime::RuntimeMavgPayloadNativeIoStatus::complete;
        result.complete = true;
        submissions.erase(it);
        return result;
    }

    Win32MavgPayloadAsyncFileIoDispatcherDesc desc;
    std::uint64_t next_ticket{1};
    std::unordered_map<std::uint64_t, PendingSubmission> submissions;
};

Win32MavgPayloadAsyncFileIoDispatcher::Win32MavgPayloadAsyncFileIoDispatcher(
    Win32MavgPayloadAsyncFileIoDispatcherDesc desc)
    : impl_(std::make_unique<Impl>(std::move(desc))) {}

Win32MavgPayloadAsyncFileIoDispatcher::~Win32MavgPayloadAsyncFileIoDispatcher() = default;

Win32MavgPayloadAsyncFileIoDispatcher::Win32MavgPayloadAsyncFileIoDispatcher(
    Win32MavgPayloadAsyncFileIoDispatcher&&) noexcept = default;

Win32MavgPayloadAsyncFileIoDispatcher&
Win32MavgPayloadAsyncFileIoDispatcher::operator=(Win32MavgPayloadAsyncFileIoDispatcher&&) noexcept = default;

runtime::RuntimeMavgPayloadNativeIoBackend Win32MavgPayloadAsyncFileIoDispatcher::backend() const noexcept {
    return runtime::RuntimeMavgPayloadNativeIoBackend::win32_async_file;
}

runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult Win32MavgPayloadAsyncFileIoDispatcher::dispatch(
    std::span<const runtime::RuntimeMavgPayloadDirectStorageRequestRow> requests,
    const runtime::RuntimeMavgPayloadNativeIoDispatchBackendDesc& desc) {
    return impl_->dispatch(requests, desc);
}

runtime::RuntimeMavgPayloadNativeIoStatusBackendResult
Win32MavgPayloadAsyncFileIoDispatcher::poll_status(std::uint64_t ticket) {
    return impl_->poll_status(ticket);
}

} // namespace mirakana
