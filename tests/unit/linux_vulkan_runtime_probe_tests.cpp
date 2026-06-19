// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/platform/linux/linux_platform_probe.hpp"
#include "mirakana/runtime_host/linux/linux_vulkan_runtime_probe.hpp"

MK_TEST("linux vulkan runtime probe fails closed without packaged spir-v") {
    const auto result = mirakana::run_linux_vulkan_runtime_probe(mirakana::LinuxVulkanRuntimeProbeDesc{});

    MK_REQUIRE(result.first_party_runtime_host_ready);
    MK_REQUIRE(!result.probe_ready);
    MK_REQUIRE(!result.readback_ready);
    MK_REQUIRE(!result.exposes_native_handles);
    MK_REQUIRE(result.surface_family == mirakana::platform::linux::LinuxVulkanSurfaceFamily::offscreen_compute);
    MK_REQUIRE(result.failure_stage == 1U);
}

int main() {
    return mirakana::test::run_all();
}
