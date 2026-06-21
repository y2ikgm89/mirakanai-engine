// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_directstorage_byte_range_io.hpp"

#include "win32_utf.hpp"

#if defined(MK_ENABLE_WIN32_DIRECTSTORAGE)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <dstorage.h>
#include <windows.h>
#include <wrl/client.h>
#endif

#include <algorithm>
#include <chrono>
#include <limits>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace mirakana::win32 {
namespace {

#if defined(MK_ENABLE_WIN32_DIRECTSTORAGE)
[[nodiscard]] std::int32_t hresult_to_i32(HRESULT result) noexcept {
    return static_cast<std::int32_t>(result);
}
#endif

} // namespace

struct Win32DirectStorageByteRangeExecutor::Impl {
    explicit Impl(Win32DirectStorageByteRangeExecutorOptions init_options) : options(init_options) {
#if defined(MK_ENABLE_WIN32_DIRECTSTORAGE)
        const HRESULT factory_result = DStorageGetFactory(IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
        diagnostics.last_hresult = hresult_to_i32(factory_result);
        diagnostics.factory_ready = SUCCEEDED(factory_result) && factory != nullptr;
        if (!diagnostics.factory_ready) {
            return;
        }

        DSTORAGE_QUEUE_DESC queue_desc{};
        queue_desc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        queue_desc.Capacity = static_cast<UINT16>(std::clamp(options.queue_capacity,
                                                             static_cast<std::uint32_t>(DSTORAGE_MIN_QUEUE_CAPACITY),
                                                             static_cast<std::uint32_t>(DSTORAGE_MAX_QUEUE_CAPACITY)));
        queue_desc.Priority = DSTORAGE_PRIORITY_NORMAL;
        queue_desc.Name = "mirakana.mavg.directstorage.byte_range";
        queue_desc.Device = nullptr;

        const HRESULT queue_result = factory->CreateQueue(&queue_desc, IID_PPV_ARGS(queue.ReleaseAndGetAddressOf()));
        diagnostics.last_hresult = hresult_to_i32(queue_result);
        diagnostics.queue_ready = SUCCEEDED(queue_result) && queue != nullptr;
        if (!diagnostics.queue_ready) {
            return;
        }

        const HRESULT status_result = factory->CreateStatusArray(1, "mirakana.mavg.directstorage.byte_range.status",
                                                                 IID_PPV_ARGS(status_array.ReleaseAndGetAddressOf()));
        diagnostics.last_hresult = hresult_to_i32(status_result);
        diagnostics.status_array_ready = SUCCEEDED(status_result) && status_array != nullptr;
        if (diagnostics.status_array_ready) {
            diagnostics.last_hresult = 0;
        }
#else
        diagnostics.last_hresult = 0;
#endif
    }

    [[nodiscard]] bool available() const noexcept {
        return diagnostics.factory_ready && diagnostics.queue_ready && diagnostics.status_array_ready;
    }

#if defined(MK_ENABLE_WIN32_DIRECTSTORAGE)
    [[nodiscard]] std::vector<ByteRangeIoReadRow> read_ranges_sdk(std::span<const ByteRangeIoReadRequest> requests) {
        diagnostics.submitted = false;
        diagnostics.request_count = 0;
        diagnostics.last_hresult = 0;
        diagnostics.factory_ready = factory != nullptr;
        diagnostics.queue_ready = queue != nullptr;
        diagnostics.status_array_ready = status_array != nullptr;

        if (requests.empty() || !available()) {
            return {};
        }

        std::vector<Microsoft::WRL::ComPtr<IDStorageFile>> files;
        std::vector<std::string> request_names;
        std::vector<DSTORAGE_REQUEST> sdk_requests;
        std::vector<ByteRangeIoReadRow> rows;
        files.reserve(requests.size());
        request_names.reserve(requests.size());
        sdk_requests.reserve(requests.size());
        rows.reserve(requests.size());

        for (std::size_t index = 0; index < requests.size(); ++index) {
            const auto& request = requests[index];
            if (request.byte_size > std::numeric_limits<UINT32>::max()) {
                diagnostics.last_hresult = hresult_to_i32(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
                return {};
            }

            Microsoft::WRL::ComPtr<IDStorageFile> file;
            const auto wide_path = detail::wide_from_utf8(request.path);
            const HRESULT open_result =
                factory->OpenFile(wide_path.c_str(), IID_PPV_ARGS(file.ReleaseAndGetAddressOf()));
            diagnostics.last_hresult = hresult_to_i32(open_result);
            if (FAILED(open_result) || file == nullptr) {
                return {};
            }

            rows.push_back(ByteRangeIoReadRow{
                .path = std::string(request.path),
                .byte_offset = request.byte_offset,
                .byte_size = request.byte_size,
                .bytes = std::vector<std::byte>(static_cast<std::size_t>(request.byte_size)),
            });
            files.push_back(std::move(file));
            request_names.push_back("mirakana.mavg.directstorage.byte_range." + std::to_string(index));

            DSTORAGE_REQUEST sdk_request{};
            sdk_request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
            sdk_request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
            sdk_request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
            sdk_request.Source.File.Source = files.back().Get();
            sdk_request.Source.File.Offset = request.byte_offset;
            sdk_request.Source.File.Size = static_cast<UINT32>(request.byte_size);
            sdk_request.Destination.Memory.Buffer = rows.back().bytes.data();
            sdk_request.Destination.Memory.Size = static_cast<UINT32>(request.byte_size);
            sdk_request.Name = request_names.back().c_str();
            sdk_requests.push_back(sdk_request);
        }

        for (const auto& sdk_request : sdk_requests) {
            queue->EnqueueRequest(&sdk_request);
            ++diagnostics.request_count;
        }
        queue->EnqueueStatus(status_array.Get(), 0);
        queue->Submit();
        diagnostics.submitted = true;

        const auto timeout =
            options.completion_timeout.count() <= 0 ? std::chrono::milliseconds{1} : options.completion_timeout;
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        for (;;) {
            const HRESULT status = status_array->GetHResult(0);
            diagnostics.last_hresult = hresult_to_i32(status);
            if (status == S_OK) {
                diagnostics.last_hresult = 0;
                return rows;
            }
            if (status != E_PENDING) {
                return {};
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                diagnostics.last_hresult = hresult_to_i32(HRESULT_FROM_WIN32(WAIT_TIMEOUT));
                return {};
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
    }

    Microsoft::WRL::ComPtr<IDStorageFactory> factory;
    Microsoft::WRL::ComPtr<IDStorageQueue> queue;
    Microsoft::WRL::ComPtr<IDStorageStatusArray> status_array;
#endif

    Win32DirectStorageByteRangeExecutorOptions options;
    Win32DirectStorageReadDiagnostics diagnostics;
};

Win32DirectStorageByteRangeExecutor::Win32DirectStorageByteRangeExecutor(
    Win32DirectStorageByteRangeExecutorOptions options)
    : impl_(std::make_unique<Impl>(options)) {}

Win32DirectStorageByteRangeExecutor::~Win32DirectStorageByteRangeExecutor() = default;
Win32DirectStorageByteRangeExecutor::Win32DirectStorageByteRangeExecutor(
    Win32DirectStorageByteRangeExecutor&&) noexcept = default;
Win32DirectStorageByteRangeExecutor&
Win32DirectStorageByteRangeExecutor::operator=(Win32DirectStorageByteRangeExecutor&&) noexcept = default;

ByteRangeIoBackendKind Win32DirectStorageByteRangeExecutor::backend_kind() const noexcept {
    return ByteRangeIoBackendKind::direct_storage;
}

bool Win32DirectStorageByteRangeExecutor::available() const noexcept {
    return impl_->available();
}

const Win32DirectStorageReadDiagnostics& Win32DirectStorageByteRangeExecutor::diagnostics() const noexcept {
    return impl_->diagnostics;
}

std::vector<ByteRangeIoReadRow>
Win32DirectStorageByteRangeExecutor::read_ranges(std::span<const ByteRangeIoReadRequest> requests) {
#if defined(MK_ENABLE_WIN32_DIRECTSTORAGE)
    return impl_->read_ranges_sdk(requests);
#else
    (void)requests;
    impl_->diagnostics.submitted = false;
    impl_->diagnostics.request_count = 0;
    return {};
#endif
}

} // namespace mirakana::win32
