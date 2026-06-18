// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/commercial_readiness_v2.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace mirakana {
namespace {

constexpr std::array<std::string_view, 16> kRequiredCommercialRows{
    "environment_strict_vulkan_aggregate_ready",
    "environment_metal_aggregate_ready",
    "environment_backend_parity_ready",
    "environment_platform_windows_d3d12_ready",
    "environment_platform_windows_vulkan_ready",
    "environment_platform_linux_vulkan_ready",
    "environment_platform_macos_metal_ready",
    "environment_platform_ios_metal_ready",
    "environment_platform_android_vulkan_ready",
    "environment_platform_readiness_ready",
    "environment_all_platform_unconditional_ready",
    "environment_broad_optimization_ready",
    "environment_asset_pipeline_openexr_ktx_basis_full_ready",
    "environment_aaa_preset_asset_library_ready",
    "environment_physical_weather_simulation_ready",
    "environment_artist_workflow_production_ready",
};

[[nodiscard]] const EnvironmentCommercialReadinessV2Row*
find_row(std::span<const EnvironmentCommercialReadinessV2Row> rows, std::string_view id) noexcept {
    for (const auto& row : rows) {
        if (row.id == id) {
            return &row;
        }
    }
    return nullptr;
}

[[nodiscard]] std::size_t required_row_index(std::string_view id) noexcept {
    for (std::size_t index{0U}; index < kRequiredCommercialRows.size(); ++index) {
        if (kRequiredCommercialRows[index] == id) {
            return index;
        }
    }
    return kRequiredCommercialRows.size();
}

void count_required_row(EnvironmentCommercialReadinessV2Result& result,
                        const EnvironmentCommercialReadinessV2Row& row) noexcept {
    result.native_handle_access = result.native_handle_access || row.native_handle_access;
    result.diagnostics = result.diagnostics || row.diagnostics;

    switch (row.status) {
    case EnvironmentCommercialReadinessV2RowStatus::ready:
        if (row.evidence_recipe_id.empty()) {
            ++result.dependency_gated_rows;
        } else {
            ++result.ready_rows;
        }
        break;
    case EnvironmentCommercialReadinessV2RowStatus::host_gated:
        ++result.host_gated_rows;
        break;
    case EnvironmentCommercialReadinessV2RowStatus::dependency_gated:
    case EnvironmentCommercialReadinessV2RowStatus::missing:
        ++result.dependency_gated_rows;
        break;
    case EnvironmentCommercialReadinessV2RowStatus::blocked:
        ++result.blocked_rows;
        break;
    case EnvironmentCommercialReadinessV2RowStatus::unsupported:
        ++result.unsupported_rows;
        break;
    }
}

} // namespace

EnvironmentCommercialReadinessV2Result
evaluate_environment_commercial_readiness_v2(std::span<const EnvironmentCommercialReadinessV2Row> rows) {
    EnvironmentCommercialReadinessV2Result result{
        .required_rows = kRequiredCommercialRows.size(),
    };

    for (const auto& row : rows) {
        result.native_handle_access = result.native_handle_access || row.native_handle_access;
        result.diagnostics = result.diagnostics || row.diagnostics;
    }

    std::array<std::uint8_t, kRequiredCommercialRows.size()> required_row_counts{};
    for (const auto& row : rows) {
        const auto index = required_row_index(row.id);
        if (index < required_row_counts.size()) {
            ++required_row_counts[index];
            if (required_row_counts[index] > 1U) {
                result.diagnostics = true;
            }
        }
    }

    for (const auto id : kRequiredCommercialRows) {
        const auto* row = find_row(rows, id);
        if (row == nullptr) {
            ++result.dependency_gated_rows;
            continue;
        }
        count_required_row(result, *row);
    }

    const bool all_rows_ready = result.ready_rows == result.required_rows && result.host_gated_rows == 0U &&
                                result.dependency_gated_rows == 0U && result.blocked_rows == 0U &&
                                result.unsupported_rows == 0U && !result.native_handle_access && !result.diagnostics;
    result.commercial_ready = all_rows_ready;
    result.highest_commercial_ready = all_rows_ready;
    return result;
}

} // namespace mirakana
