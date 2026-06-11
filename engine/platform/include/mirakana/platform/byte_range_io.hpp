// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class ByteRangeIoBackendKind : std::uint8_t {
    unknown = 0,
    direct_storage,
};

struct ByteRangeIoReadRequest {
    std::string_view path;
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
};

struct ByteRangeIoReadRow {
    std::string path;
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
    std::vector<std::byte> bytes;
};

class IByteRangeIoExecutor {
  public:
    virtual ~IByteRangeIoExecutor() = default;

    [[nodiscard]] virtual ByteRangeIoBackendKind backend_kind() const noexcept = 0;
    [[nodiscard]] virtual bool available() const noexcept = 0;

    /// Executes the supplied byte-range requests before returning. The request span and every string_view inside it are
    /// call-bound; implementations must copy any data needed for later asynchronous work and must not retain references
    /// to the request storage after this call returns.
    [[nodiscard]] virtual std::vector<ByteRangeIoReadRow>
    read_ranges(std::span<const ByteRangeIoReadRequest> requests) = 0;
};

} // namespace mirakana
