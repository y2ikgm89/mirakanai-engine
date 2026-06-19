// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/linux/linux_platform_probe.hpp"

namespace mirakana::platform::linux {

LinuxPlatformRuntimeEvidence query_linux_platform_runtime_evidence() noexcept {
    LinuxPlatformRuntimeEvidence evidence{};
#if defined(__linux__) && !defined(__ANDROID__)
    evidence.host_matches_linux = true;
    evidence.first_party_platform_module_ready = true;
#endif
    evidence.exposes_native_handles = false;
    evidence.surface_family = LinuxVulkanSurfaceFamily::offscreen_compute;
    return evidence;
}

} // namespace mirakana::platform::linux
