// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace mirakana::runtime_rhi {

enum class EnvironmentPlatformEvidenceV2RowStatus : std::uint8_t {
    missing,
    ready,
    host_gated,
    blocked,
    unsupported,
};

enum class EnvironmentPlatformEvidenceV2PlatformId : std::uint8_t {
    windows_d3d12,
    windows_vulkan,
    linux_vulkan,
    macos_metal,
    ios_metal,
    android_vulkan,
};

struct EnvironmentPlatformEvidenceV2Row {
    std::string id;
    EnvironmentPlatformEvidenceV2PlatformId platform{EnvironmentPlatformEvidenceV2PlatformId::windows_d3d12};
    EnvironmentPlatformEvidenceV2RowStatus status{EnvironmentPlatformEvidenceV2RowStatus::missing};
    std::string validation_recipe_id;
    std::string host_gate_id;
    bool host_matches{false};
    bool vulkaninfo_ready{false};
    bool validation_layer_ready{false};
    bool dxc_spirv_codegen_ready{false};
    bool spirv_val_ready{false};
    bool linux_icd_runtime_ready{false};
    bool first_party_linux_runtime_host_ready{false};
    bool linux_package_script_ready{false};
    bool linux_installed_validator_ready{false};
    bool linux_package_smoke_ready{false};
    bool linux_vulkan_readback_ready{false};
    bool linux_vulkan_validation_log_clean{false};
    bool android_sdk_ready{false};
    bool android_ndk_ready{false};
    bool android_device_or_emulator_ready{false};
    bool android_vulkan_profile_ready{false};
    bool android_gpu_debuggable_ready{false};
    bool android_gpu_debug_layer_settings_ready{false};
    bool android_package_smoke_ready{false};
    bool android_vulkan_readback_ready{false};
    bool xcodebuild_ready{false};
    bool xcrun_metal_ready{false};
    bool metal_feature_set_table_checked{false};
    bool metal_command_queue_ready{false};
    bool metal_command_buffer_ready{false};
    bool metal_render_pipeline_ready{false};
    bool metal_compute_pipeline_ready{false};
    bool metal_texture_usage_rows_ready{false};
    bool metal_resource_synchronization_ready{false};
    bool metal_readback_ready{false};
    bool xcode_ios_sdk_ready{false};
    bool ios_simulator_or_device_ready{false};
    bool ios_metal_feature_set_checked{false};
    bool ios_package_smoke_ready{false};
    bool ios_metal_command_queue_ready{false};
    bool ios_metal_pipeline_ready{false};
    bool ios_metal_readback_ready{false};
    bool environment_platform_ready_counter{false};
    bool requires_host_evidence{true};
    bool inferred_from_other_platform{false};
    bool native_handle_access{false};
    bool diagnostics{false};
};

struct EnvironmentPlatformEvidenceV2Result {
    bool windows_d3d12_ready{false};
    bool windows_vulkan_ready{false};
    bool linux_vulkan_ready{false};
    bool macos_metal_ready{false};
    bool ios_metal_ready{false};
    bool android_vulkan_ready{false};
    bool platform_readiness_ready{false};
    bool all_platform_unconditional_ready{false};
    std::size_t required_rows{6};
    std::size_t ready_rows{0};
    std::size_t host_gated_rows{0};
    std::size_t blocked_rows{0};
    std::size_t unsupported_rows{0};
    std::size_t missing_rows{0};
    bool inferred_from_other_platform{false};
    bool native_handle_access{false};
    bool diagnostics{false};
};

namespace detail {

[[nodiscard]] constexpr std::size_t platform_index(EnvironmentPlatformEvidenceV2PlatformId platform) {
    switch (platform) {
    case EnvironmentPlatformEvidenceV2PlatformId::windows_d3d12:
        return 0U;
    case EnvironmentPlatformEvidenceV2PlatformId::windows_vulkan:
        return 1U;
    case EnvironmentPlatformEvidenceV2PlatformId::linux_vulkan:
        return 2U;
    case EnvironmentPlatformEvidenceV2PlatformId::macos_metal:
        return 3U;
    case EnvironmentPlatformEvidenceV2PlatformId::ios_metal:
        return 4U;
    case EnvironmentPlatformEvidenceV2PlatformId::android_vulkan:
        return 5U;
    }
    return 0U;
}

[[nodiscard]] inline bool base_ready(const EnvironmentPlatformEvidenceV2Row& row, std::string_view id,
                                     std::string_view recipe, std::string_view host_gate) {
    return row.id == id && row.validation_recipe_id == recipe && row.host_gate_id == host_gate && row.host_matches &&
           row.environment_platform_ready_counter && !row.requires_host_evidence && !row.inferred_from_other_platform &&
           !row.native_handle_access && !row.diagnostics;
}

[[nodiscard]] inline bool windows_d3d12_ready(const EnvironmentPlatformEvidenceV2Row& row) {
    return base_ready(row, "environment_platform_windows_d3d12",
                      "desktop-runtime-sample-game-environment-platform-readiness", "d3d12-windows-primary");
}

[[nodiscard]] inline bool windows_vulkan_ready(const EnvironmentPlatformEvidenceV2Row& row) {
    return base_ready(row, "environment_platform_windows_vulkan",
                      "desktop-runtime-sample-game-environment-platform-windows-vulkan-evidence", "vulkan-strict") &&
           row.vulkaninfo_ready && row.validation_layer_ready && row.dxc_spirv_codegen_ready && row.spirv_val_ready;
}

[[nodiscard]] inline bool linux_vulkan_ready(const EnvironmentPlatformEvidenceV2Row& row) {
    return base_ready(row, "environment_platform_linux_vulkan", "environment-platform-linux-vulkan-package",
                      "linux-vulkan-runtime-host") &&
           row.vulkaninfo_ready && row.validation_layer_ready && row.dxc_spirv_codegen_ready && row.spirv_val_ready &&
           row.linux_icd_runtime_ready && row.first_party_linux_runtime_host_ready && row.linux_package_script_ready &&
           row.linux_installed_validator_ready && row.linux_package_smoke_ready && row.linux_vulkan_readback_ready &&
           row.linux_vulkan_validation_log_clean;
}

[[nodiscard]] inline bool macos_metal_ready(const EnvironmentPlatformEvidenceV2Row& row) {
    return base_ready(row, "environment_platform_macos_metal",
                      "renderer-metal-environment-aggregate-apple-host-evidence", "metal-apple") &&
           row.xcodebuild_ready && row.xcrun_metal_ready && row.metal_feature_set_table_checked &&
           row.metal_command_queue_ready && row.metal_command_buffer_ready && row.metal_render_pipeline_ready &&
           row.metal_compute_pipeline_ready && row.metal_texture_usage_rows_ready &&
           row.metal_resource_synchronization_ready && row.metal_readback_ready;
}

[[nodiscard]] inline bool ios_metal_ready(const EnvironmentPlatformEvidenceV2Row& row) {
    return base_ready(row, "environment_platform_ios_metal", "environment-platform-ios-metal-package",
                      "ios-metal-host") &&
           row.xcodebuild_ready && row.xcode_ios_sdk_ready && row.ios_simulator_or_device_ready &&
           row.ios_metal_feature_set_checked && row.ios_package_smoke_ready && row.ios_metal_command_queue_ready &&
           row.ios_metal_pipeline_ready && row.ios_metal_readback_ready;
}

[[nodiscard]] inline bool android_vulkan_ready(const EnvironmentPlatformEvidenceV2Row& row) {
    return base_ready(row, "environment_platform_android_vulkan", "environment-platform-android-vulkan-package",
                      "android-vulkan-runtime-host") &&
           row.validation_layer_ready && row.android_sdk_ready && row.android_ndk_ready &&
           row.android_device_or_emulator_ready && row.android_vulkan_profile_ready &&
           row.android_gpu_debuggable_ready && row.android_gpu_debug_layer_settings_ready &&
           row.android_package_smoke_ready && row.android_vulkan_readback_ready;
}

[[nodiscard]] inline bool exact_ready_for_platform(const EnvironmentPlatformEvidenceV2Row& row) {
    switch (row.platform) {
    case EnvironmentPlatformEvidenceV2PlatformId::windows_d3d12:
        return windows_d3d12_ready(row);
    case EnvironmentPlatformEvidenceV2PlatformId::windows_vulkan:
        return windows_vulkan_ready(row);
    case EnvironmentPlatformEvidenceV2PlatformId::linux_vulkan:
        return linux_vulkan_ready(row);
    case EnvironmentPlatformEvidenceV2PlatformId::macos_metal:
        return macos_metal_ready(row);
    case EnvironmentPlatformEvidenceV2PlatformId::ios_metal:
        return ios_metal_ready(row);
    case EnvironmentPlatformEvidenceV2PlatformId::android_vulkan:
        return android_vulkan_ready(row);
    }
    return false;
}

inline void mark_ready(EnvironmentPlatformEvidenceV2Result& result, EnvironmentPlatformEvidenceV2PlatformId platform) {
    switch (platform) {
    case EnvironmentPlatformEvidenceV2PlatformId::windows_d3d12:
        result.windows_d3d12_ready = true;
        break;
    case EnvironmentPlatformEvidenceV2PlatformId::windows_vulkan:
        result.windows_vulkan_ready = true;
        break;
    case EnvironmentPlatformEvidenceV2PlatformId::linux_vulkan:
        result.linux_vulkan_ready = true;
        break;
    case EnvironmentPlatformEvidenceV2PlatformId::macos_metal:
        result.macos_metal_ready = true;
        break;
    case EnvironmentPlatformEvidenceV2PlatformId::ios_metal:
        result.ios_metal_ready = true;
        break;
    case EnvironmentPlatformEvidenceV2PlatformId::android_vulkan:
        result.android_vulkan_ready = true;
        break;
    }
}

} // namespace detail

[[nodiscard]] inline EnvironmentPlatformEvidenceV2Result
evaluate_environment_platform_evidence_v2(std::span<const EnvironmentPlatformEvidenceV2Row> rows) {
    EnvironmentPlatformEvidenceV2Result result{};
    std::array<bool, 6U> seen{};

    for (const auto& row : rows) {
        const auto index = detail::platform_index(row.platform);
        if (seen[index]) {
            ++result.blocked_rows;
            result.diagnostics = true;
            continue;
        }
        seen[index] = true;

        result.inferred_from_other_platform = result.inferred_from_other_platform || row.inferred_from_other_platform;
        result.native_handle_access = result.native_handle_access || row.native_handle_access;
        result.diagnostics =
            result.diagnostics || row.diagnostics || row.inferred_from_other_platform || row.native_handle_access;

        switch (row.status) {
        case EnvironmentPlatformEvidenceV2RowStatus::ready:
            if (detail::exact_ready_for_platform(row)) {
                ++result.ready_rows;
                detail::mark_ready(result, row.platform);
            } else {
                ++result.blocked_rows;
                result.diagnostics = true;
            }
            break;
        case EnvironmentPlatformEvidenceV2RowStatus::host_gated:
            ++result.host_gated_rows;
            break;
        case EnvironmentPlatformEvidenceV2RowStatus::blocked:
            ++result.blocked_rows;
            result.diagnostics = true;
            break;
        case EnvironmentPlatformEvidenceV2RowStatus::unsupported:
            ++result.unsupported_rows;
            break;
        case EnvironmentPlatformEvidenceV2RowStatus::missing:
            break;
        }
    }

    for (const auto platform_seen : seen) {
        if (!platform_seen) {
            ++result.missing_rows;
        }
    }

    result.platform_readiness_ready = result.windows_d3d12_ready && result.windows_vulkan_ready &&
                                      result.linux_vulkan_ready && result.macos_metal_ready && result.ios_metal_ready &&
                                      result.android_vulkan_ready && result.missing_rows == 0U &&
                                      result.host_gated_rows == 0U && result.blocked_rows == 0U &&
                                      result.unsupported_rows == 0U && !result.diagnostics;
    result.all_platform_unconditional_ready = result.platform_readiness_ready;
    return result;
}

} // namespace mirakana::runtime_rhi
