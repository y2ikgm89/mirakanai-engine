// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/environment_platform_evidence_v2.hpp"

#include <string>
#include <vector>

namespace {

using PlatformRow = mirakana::runtime_rhi::EnvironmentPlatformEvidenceV2Row;
using PlatformRowStatus = mirakana::runtime_rhi::EnvironmentPlatformEvidenceV2RowStatus;
using PlatformId = mirakana::runtime_rhi::EnvironmentPlatformEvidenceV2PlatformId;

[[nodiscard]] PlatformRow ready_windows_vulkan_row() {
    return PlatformRow{
        .id = "environment_platform_windows_vulkan",
        .platform = PlatformId::windows_vulkan,
        .status = PlatformRowStatus::ready,
        .validation_recipe_id = "desktop-runtime-sample-game-environment-platform-windows-vulkan-evidence",
        .host_gate_id = "vulkan-strict",
        .host_matches = true,
        .vulkaninfo_ready = true,
        .validation_layer_ready = true,
        .dxc_spirv_codegen_ready = true,
        .spirv_val_ready = true,
        .environment_platform_ready_counter = true,
        .requires_host_evidence = false,
    };
}

[[nodiscard]] PlatformRow ready_linux_vulkan_row() {
    return PlatformRow{
        .id = "environment_platform_linux_vulkan",
        .platform = PlatformId::linux_vulkan,
        .status = PlatformRowStatus::ready,
        .validation_recipe_id = "environment-platform-linux-vulkan-package",
        .host_gate_id = "linux-vulkan-runtime-host",
        .host_matches = true,
        .vulkaninfo_ready = true,
        .validation_layer_ready = true,
        .dxc_spirv_codegen_ready = true,
        .spirv_val_ready = true,
        .linux_icd_runtime_ready = true,
        .first_party_linux_runtime_host_ready = true,
        .linux_package_script_ready = true,
        .linux_installed_validator_ready = true,
        .environment_platform_ready_counter = true,
        .requires_host_evidence = false,
    };
}

[[nodiscard]] PlatformRow ready_android_vulkan_row() {
    return PlatformRow{
        .id = "environment_platform_android_vulkan",
        .platform = PlatformId::android_vulkan,
        .status = PlatformRowStatus::ready,
        .validation_recipe_id = "environment-platform-android-vulkan-package",
        .host_gate_id = "android-vulkan-runtime-host",
        .host_matches = true,
        .validation_layer_ready = true,
        .android_sdk_ready = true,
        .android_ndk_ready = true,
        .android_device_or_emulator_ready = true,
        .android_vulkan_profile_ready = true,
        .android_validation_layer_packaged = true,
        .android_package_smoke_ready = true,
        .android_vulkan_readback_ready = true,
        .environment_platform_ready_counter = true,
        .requires_host_evidence = false,
    };
}

} // namespace

MK_TEST("windows_vulkan_evidence_does_not_promote_linux_or_android_vulkan") {
    const std::vector<PlatformRow> rows{
        ready_windows_vulkan_row(),
    };

    const auto result = mirakana::runtime_rhi::evaluate_environment_platform_evidence_v2(rows);

    MK_REQUIRE(result.windows_vulkan_ready);
    MK_REQUIRE(!result.linux_vulkan_ready);
    MK_REQUIRE(!result.android_vulkan_ready);
    MK_REQUIRE(!result.all_platform_unconditional_ready);
    MK_REQUIRE(result.ready_rows == 1U);
    MK_REQUIRE(result.host_gated_rows == 0U);
    MK_REQUIRE(result.missing_rows >= 2U);
    MK_REQUIRE(!result.inferred_from_other_platform);
}

MK_TEST("linux_vulkan_ready_requires_exact_linux_host_gate_and_tool_rows") {
    auto row = ready_linux_vulkan_row();
    row.validation_layer_ready = false;
    row.host_gate_id = "vulkan-strict";

    const auto blocked =
        mirakana::runtime_rhi::evaluate_environment_platform_evidence_v2(std::vector<PlatformRow>{row});

    MK_REQUIRE(!blocked.linux_vulkan_ready);
    MK_REQUIRE(blocked.blocked_rows == 1U);
    MK_REQUIRE(blocked.diagnostics);

    const auto ready = mirakana::runtime_rhi::evaluate_environment_platform_evidence_v2(
        std::vector<PlatformRow>{ready_linux_vulkan_row()});

    MK_REQUIRE(ready.linux_vulkan_ready);
    MK_REQUIRE(!ready.android_vulkan_ready);
    MK_REQUIRE(!ready.all_platform_unconditional_ready);
    MK_REQUIRE(ready.ready_rows == 1U);
    MK_REQUIRE(!ready.diagnostics);
}

MK_TEST("android_vulkan_ready_requires_android_device_validation_layer_package_and_readback") {
    auto row = ready_android_vulkan_row();
    row.android_validation_layer_packaged = false;
    row.android_vulkan_readback_ready = false;

    const auto blocked =
        mirakana::runtime_rhi::evaluate_environment_platform_evidence_v2(std::vector<PlatformRow>{row});

    MK_REQUIRE(!blocked.android_vulkan_ready);
    MK_REQUIRE(blocked.blocked_rows == 1U);
    MK_REQUIRE(blocked.diagnostics);

    const auto ready = mirakana::runtime_rhi::evaluate_environment_platform_evidence_v2(
        std::vector<PlatformRow>{ready_android_vulkan_row()});

    MK_REQUIRE(ready.android_vulkan_ready);
    MK_REQUIRE(!ready.linux_vulkan_ready);
    MK_REQUIRE(!ready.all_platform_unconditional_ready);
    MK_REQUIRE(ready.ready_rows == 1U);
    MK_REQUIRE(!ready.diagnostics);
}

MK_TEST("native_handle_or_inferred_platform_rows_keep_ready_false") {
    auto linux_row = ready_linux_vulkan_row();
    linux_row.inferred_from_other_platform = true;
    auto android_row = ready_android_vulkan_row();
    android_row.native_handle_access = true;

    const auto result = mirakana::runtime_rhi::evaluate_environment_platform_evidence_v2(
        std::vector<PlatformRow>{linux_row, android_row});

    MK_REQUIRE(!result.linux_vulkan_ready);
    MK_REQUIRE(!result.android_vulkan_ready);
    MK_REQUIRE(!result.all_platform_unconditional_ready);
    MK_REQUIRE(result.inferred_from_other_platform);
    MK_REQUIRE(result.native_handle_access);
    MK_REQUIRE(result.diagnostics);
}

int main() {
    return mirakana::test::run_all();
}
