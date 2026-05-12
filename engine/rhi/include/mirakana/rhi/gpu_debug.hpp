// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace mirakana::rhi {

/// Maximum UTF-8 byte length accepted for `IRhiCommandList` debug scopes and markers on backends that
/// forward labels to native APIs with a bounded copy (D3D12 `BeginEvent` / `SetMarker`, future Vulkan
/// `VK_EXT_debug_utils`).
inline constexpr std::size_t max_rhi_debug_label_bytes = 256;

[[nodiscard]] constexpr bool is_valid_rhi_debug_label(std::string_view name) noexcept {
    if (name.empty() || name.size() > max_rhi_debug_label_bytes) {
        return false;
    }
    for (unsigned char byte : name) {
        if (byte < 0x20U || byte > 0x7eU) {
            return false;
        }
    }
    return true;
}

/// Validates debug labels for GPU marker adapters. Printable ASCII only so D3D12 metadata `0` ANSI
/// payloads and future Vulkan UTF-8 paths stay consistent across backends.
inline void validate_rhi_debug_label(std::string_view name) {
    if (!is_valid_rhi_debug_label(name)) {
        throw std::invalid_argument(
            "rhi debug label must be non-empty printable ASCII with at most 256 bytes (no NUL characters)");
    }
}

} // namespace mirakana::rhi
