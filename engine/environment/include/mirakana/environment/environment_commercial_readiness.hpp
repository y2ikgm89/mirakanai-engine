// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class EnvironmentCommercialReadinessStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    blocked,
};

enum class EnvironmentCommercialReadinessRequirementKind : std::uint8_t {
    strict_vulkan_aggregate = 0,
    metal_host_aggregate,
    backend_parity,
    platform_windows_d3d12,
    platform_windows_vulkan,
    platform_linux_vulkan,
    platform_macos_metal,
    platform_ios_metal,
    platform_android_vulkan,
    broad_optimization,
    asset_pipeline_openexr_ktx_basis,
    aaa_preset_library,
    physical_weather_simulation,
    artist_workflow,
};

enum class EnvironmentCommercialReadinessRequirementStatus : std::uint8_t {
    ready = 0,
    host_gated,
    blocked,
    unsupported,
    missing,
};

struct EnvironmentCommercialReadinessRequirementInputRow {
    EnvironmentCommercialReadinessRequirementKind kind{
        EnvironmentCommercialReadinessRequirementKind::strict_vulkan_aggregate};
    EnvironmentCommercialReadinessRequirementStatus status{EnvironmentCommercialReadinessRequirementStatus::missing};
    std::string evidence_id;
    std::string package_counter;
    bool package_visible{false};
    bool legal_notice_current{false};
    bool validation_guarded{false};
    bool native_handle_access{false};
    bool broad_environment_ready_claimed{false};
};

struct EnvironmentCommercialReadinessDesc {
    std::vector<EnvironmentCommercialReadinessRequirementInputRow> requirements;
    bool optional_dependency_legal_records_current{false};
    bool adjacent_broad_non_claims_declared{false};
    bool request_commercial_ready{false};
};

struct EnvironmentCommercialReadinessRequirementRow {
    std::string row_id;
    EnvironmentCommercialReadinessRequirementKind kind{
        EnvironmentCommercialReadinessRequirementKind::strict_vulkan_aggregate};
    EnvironmentCommercialReadinessRequirementStatus status{EnvironmentCommercialReadinessRequirementStatus::missing};
    std::string evidence_id;
    std::string package_counter;
    bool package_visible{false};
    bool legal_notice_current{false};
    bool validation_guarded{false};
    bool native_handle_access{false};
    bool broad_environment_ready_claimed{false};
};

struct EnvironmentCommercialReadinessPlan {
    EnvironmentCommercialReadinessStatus status{EnvironmentCommercialReadinessStatus::blocked};
    std::vector<EnvironmentCommercialReadinessRequirementRow> rows;
    std::uint32_t required_row_count{0};
    std::uint32_t ready_row_count{0};
    std::uint32_t host_gated_row_count{0};
    std::uint32_t blocked_row_count{0};
    std::uint32_t missing_row_count{0};
    std::uint32_t package_visible_row_count{0};
    std::uint32_t validation_guarded_row_count{0};
    std::uint32_t legal_notice_current_row_count{0};
    std::uint32_t native_handle_access_count{0};
    bool optional_dependency_legal_records_current{false};
    bool adjacent_broad_non_claims_declared{false};
    bool broad_environment_ready_claimed{false};
    bool environment_commercial_ready{false};
    std::uint64_t replay_hash{0};
};

[[nodiscard]] std::string_view
environment_commercial_readiness_status_label(EnvironmentCommercialReadinessStatus status) noexcept;
[[nodiscard]] std::string_view
environment_commercial_readiness_requirement_id(EnvironmentCommercialReadinessRequirementKind kind) noexcept;
[[nodiscard]] std::string_view environment_commercial_readiness_requirement_status_label(
    EnvironmentCommercialReadinessRequirementStatus status) noexcept;

[[nodiscard]] EnvironmentCommercialReadinessPlan
plan_environment_commercial_readiness(const EnvironmentCommercialReadinessDesc& desc);

} // namespace mirakana
