// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/byte_range_io.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace mirakana::win32 {

struct Win32DirectStorageByteRangeExecutorOptions {
    std::uint32_t queue_capacity{128};
    std::chrono::milliseconds completion_timeout{std::chrono::seconds{30}};
};

struct Win32DirectStorageReadDiagnostics {
    bool factory_ready{false};
    bool queue_ready{false};
    bool status_array_ready{false};
    bool submitted{false};
    std::uint32_t request_count{0};
    std::int32_t last_hresult{0};
};

class Win32DirectStorageByteRangeExecutor final : public IByteRangeIoExecutor {
  public:
    explicit Win32DirectStorageByteRangeExecutor(Win32DirectStorageByteRangeExecutorOptions options = {});
    ~Win32DirectStorageByteRangeExecutor() override;

    Win32DirectStorageByteRangeExecutor(const Win32DirectStorageByteRangeExecutor&) = delete;
    Win32DirectStorageByteRangeExecutor& operator=(const Win32DirectStorageByteRangeExecutor&) = delete;
    Win32DirectStorageByteRangeExecutor(Win32DirectStorageByteRangeExecutor&&) noexcept;
    Win32DirectStorageByteRangeExecutor& operator=(Win32DirectStorageByteRangeExecutor&&) noexcept;

    [[nodiscard]] ByteRangeIoBackendKind backend_kind() const noexcept override;
    [[nodiscard]] bool available() const noexcept override;
    [[nodiscard]] const Win32DirectStorageReadDiagnostics& diagnostics() const noexcept;
    [[nodiscard]] std::vector<ByteRangeIoReadRow>
    read_ranges(std::span<const ByteRangeIoReadRequest> requests) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana::win32
