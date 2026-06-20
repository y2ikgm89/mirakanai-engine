// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>

namespace mirakana_android {

struct AndroidVulkanReadbackSmokeResult {
    bool ready{false};
    bool validation_layer_enumerated{false};
    std::uint32_t expected_word{0};
    std::uint32_t actual_word{0};
    const char* failure_stage{"not_started"};
    int vulkan_result{0};
};

[[nodiscard]] AndroidVulkanReadbackSmokeResult run_android_vulkan_readback_smoke() noexcept;
void log_android_vulkan_readback_smoke_result(const AndroidVulkanReadbackSmokeResult& result) noexcept;

} // namespace mirakana_android
