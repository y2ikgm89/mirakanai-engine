// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>

namespace mirakana::rhi {

/// Backend-neutral snapshot of **diagnostic** memory signals for the active adapter / logical device.
/// Values are informational only: they do not enforce budgets, drive eviction, or replace OS allocator APIs.
struct RhiDeviceMemoryDiagnostics {
    /// `true` when OS-reported video memory counters are populated (Windows: DXGI `QueryVideoMemoryInfo` on the
    /// adapter that owns the D3D12 device). Always `false` on null and Vulkan until a host-gated extension path is
    /// wired.
    bool os_video_memory_budget_available{false};
    std::uint64_t local_video_memory_budget_bytes{0};
    std::uint64_t local_video_memory_usage_bytes{0};
    std::uint64_t non_local_video_memory_budget_bytes{0};
    std::uint64_t non_local_video_memory_usage_bytes{0};

    /// `true` when `committed_resources_byte_estimate` is a best-effort sum derived from engine-visible resource
    /// descriptions (null/Vulkan) or D3D12 `GetResourceAllocationInfo` over committed resources owned by the device
    /// context (D3D12).
    bool committed_resources_byte_estimate_available{false};
    /// Best-effort sum of committed buffer/texture bytes tracked by the RHI device implementation (see
    /// `committed_resources_byte_estimate_available`).
    std::uint64_t committed_resources_byte_estimate{0};
};

} // namespace mirakana::rhi
