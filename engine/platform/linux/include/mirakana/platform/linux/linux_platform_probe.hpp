// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string_view>

namespace mirakana::platform::linux {

enum class LinuxVulkanSurfaceFamily : std::uint8_t {
    offscreen_compute,
};

struct LinuxPlatformRuntimeEvidence {
    bool host_matches_linux{false};
    bool first_party_platform_module_ready{false};
    bool exposes_native_handles{false};
    LinuxVulkanSurfaceFamily surface_family{LinuxVulkanSurfaceFamily::offscreen_compute};
};

[[nodiscard]] constexpr std::string_view linux_vulkan_surface_family_name(LinuxVulkanSurfaceFamily family) noexcept {
    switch (family) {
    case LinuxVulkanSurfaceFamily::offscreen_compute:
        return "offscreen_compute";
    }
    return "unknown";
}

[[nodiscard]] LinuxPlatformRuntimeEvidence query_linux_platform_runtime_evidence() noexcept;

} // namespace mirakana::platform::linux
