// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/mavg_payload_pages.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>

namespace mirakana {

struct Win32MavgPayloadAsyncFileIoDispatcherDesc {
    std::filesystem::path root_path;
    std::size_t max_inflight_submissions{64};
};

class Win32MavgPayloadAsyncFileIoDispatcher final : public runtime::IRuntimeMavgPayloadNativeIoDispatcher {
  public:
    explicit Win32MavgPayloadAsyncFileIoDispatcher(Win32MavgPayloadAsyncFileIoDispatcherDesc desc);
    ~Win32MavgPayloadAsyncFileIoDispatcher() override;

    Win32MavgPayloadAsyncFileIoDispatcher(const Win32MavgPayloadAsyncFileIoDispatcher&) = delete;
    Win32MavgPayloadAsyncFileIoDispatcher& operator=(const Win32MavgPayloadAsyncFileIoDispatcher&) = delete;
    Win32MavgPayloadAsyncFileIoDispatcher(Win32MavgPayloadAsyncFileIoDispatcher&&) noexcept;
    Win32MavgPayloadAsyncFileIoDispatcher& operator=(Win32MavgPayloadAsyncFileIoDispatcher&&) noexcept;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoBackend backend() const noexcept override;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult
    dispatch(std::span<const runtime::RuntimeMavgPayloadDirectStorageRequestRow> requests,
             const runtime::RuntimeMavgPayloadNativeIoDispatchBackendDesc& desc) override;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoStatusBackendResult poll_status(std::uint64_t ticket) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

struct Win32MavgPayloadIocpFileIoDispatcherDesc {
    std::filesystem::path root_path;
    std::size_t max_inflight_submissions{64};
    std::size_t worker_thread_count{1};
};

class Win32MavgPayloadIocpFileIoDispatcher final : public runtime::IRuntimeMavgPayloadNativeIoDispatcher {
  public:
    explicit Win32MavgPayloadIocpFileIoDispatcher(Win32MavgPayloadIocpFileIoDispatcherDesc desc);
    ~Win32MavgPayloadIocpFileIoDispatcher() override;

    Win32MavgPayloadIocpFileIoDispatcher(const Win32MavgPayloadIocpFileIoDispatcher&) = delete;
    Win32MavgPayloadIocpFileIoDispatcher& operator=(const Win32MavgPayloadIocpFileIoDispatcher&) = delete;
    Win32MavgPayloadIocpFileIoDispatcher(Win32MavgPayloadIocpFileIoDispatcher&&) noexcept;
    Win32MavgPayloadIocpFileIoDispatcher& operator=(Win32MavgPayloadIocpFileIoDispatcher&&) noexcept;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoBackend backend() const noexcept override;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult
    dispatch(std::span<const runtime::RuntimeMavgPayloadDirectStorageRequestRow> requests,
             const runtime::RuntimeMavgPayloadNativeIoDispatchBackendDesc& desc) override;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoStatusBackendResult poll_status(std::uint64_t ticket) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#if defined(MK_RUNTIME_HOST_WIN32_ENABLE_DIRECTSTORAGE_SDK)
struct Win32MavgPayloadDirectStorageDispatcherDesc {
    std::filesystem::path root_path;
    std::size_t max_inflight_submissions{64};
    std::size_t queue_capacity{128};
};

class Win32MavgPayloadDirectStorageDispatcher final : public runtime::IRuntimeMavgPayloadNativeIoDispatcher {
  public:
    explicit Win32MavgPayloadDirectStorageDispatcher(Win32MavgPayloadDirectStorageDispatcherDesc desc);
    ~Win32MavgPayloadDirectStorageDispatcher() override;

    Win32MavgPayloadDirectStorageDispatcher(const Win32MavgPayloadDirectStorageDispatcher&) = delete;
    Win32MavgPayloadDirectStorageDispatcher& operator=(const Win32MavgPayloadDirectStorageDispatcher&) = delete;
    Win32MavgPayloadDirectStorageDispatcher(Win32MavgPayloadDirectStorageDispatcher&&) noexcept;
    Win32MavgPayloadDirectStorageDispatcher& operator=(Win32MavgPayloadDirectStorageDispatcher&&) noexcept;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoBackend backend() const noexcept override;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult
    dispatch(std::span<const runtime::RuntimeMavgPayloadDirectStorageRequestRow> requests,
             const runtime::RuntimeMavgPayloadNativeIoDispatchBackendDesc& desc) override;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoStatusBackendResult poll_status(std::uint64_t ticket) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
#endif

} // namespace mirakana
