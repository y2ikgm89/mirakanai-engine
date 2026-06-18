// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/environment/commercial_readiness_v2.hpp"

#include <array>
#include <cstddef>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using ReadinessRow = mirakana::EnvironmentCommercialReadinessV2Row;
using RowStatus = mirakana::EnvironmentCommercialReadinessV2RowStatus;

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

[[nodiscard]] ReadinessRow ready_row(std::string_view id) {
    std::string recipe_id{"recipe."};
    recipe_id += id;
    return ReadinessRow{
        .id = std::string{id},
        .status = RowStatus::ready,
        .evidence_recipe_id = std::move(recipe_id),
    };
}

[[nodiscard]] bool contains(std::initializer_list<std::string_view> values, std::string_view target) noexcept {
    for (const auto value : values) {
        if (value == target) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::vector<ReadinessRow> all_ready_rows() {
    std::vector<ReadinessRow> rows;
    rows.reserve(kRequiredCommercialRows.size());
    for (const auto id : kRequiredCommercialRows) {
        rows.push_back(ready_row(id));
    }
    return rows;
}

[[nodiscard]] std::vector<ReadinessRow> all_ready_except(std::initializer_list<std::string_view> excluded_ids) {
    std::vector<ReadinessRow> rows;
    rows.reserve(kRequiredCommercialRows.size());
    for (const auto id : kRequiredCommercialRows) {
        if (!contains(excluded_ids, id)) {
            rows.push_back(ready_row(id));
        }
    }
    return rows;
}

void set_row_status(std::vector<ReadinessRow>& rows, std::string_view id, RowStatus status,
                    std::string_view host_gate_id) {
    for (auto& row : rows) {
        if (row.id == id) {
            row.status = status;
            row.evidence_host_gate_id = std::string{host_gate_id};
            return;
        }
    }
    MK_REQUIRE(false);
}

[[nodiscard]] mirakana::EnvironmentCommercialReadinessV2Result evaluate(const std::vector<ReadinessRow>& rows) {
    return mirakana::evaluate_environment_commercial_readiness_v2(
        std::span<const ReadinessRow>{rows.data(), rows.size()});
}

} // namespace

MK_TEST("missing_dependency_keeps_environment_highest_commercial_ready_0") {
    const std::vector<ReadinessRow> rows;

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.highest_commercial_ready);
    MK_REQUIRE(!result.commercial_ready);
    MK_REQUIRE(result.required_rows == kRequiredCommercialRows.size());
    MK_REQUIRE(result.ready_rows == 0U);
    MK_REQUIRE(result.dependency_gated_rows == kRequiredCommercialRows.size());
    MK_REQUIRE(result.host_gated_rows == 0U);
    MK_REQUIRE(result.blocked_rows == 0U);
    MK_REQUIRE(result.unsupported_rows == 0U);
    MK_REQUIRE(!result.native_handle_access);
    MK_REQUIRE(!result.diagnostics);
}

MK_TEST("all_dependencies_ready_promotes_environment_highest_commercial_ready_1") {
    const auto result = evaluate(all_ready_rows());

    MK_REQUIRE(result.highest_commercial_ready);
    MK_REQUIRE(result.commercial_ready);
    MK_REQUIRE(result.required_rows == kRequiredCommercialRows.size());
    MK_REQUIRE(result.ready_rows == kRequiredCommercialRows.size());
    MK_REQUIRE(result.host_gated_rows == 0U);
    MK_REQUIRE(result.dependency_gated_rows == 0U);
    MK_REQUIRE(result.blocked_rows == 0U);
    MK_REQUIRE(result.unsupported_rows == 0U);
    MK_REQUIRE(!result.native_handle_access);
    MK_REQUIRE(!result.diagnostics);
}

MK_TEST("d3d12_ready_does_not_promote_vulkan_or_metal") {
    const std::vector<ReadinessRow> rows{
        ready_row("environment_platform_windows_d3d12_ready"),
    };

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.highest_commercial_ready);
    MK_REQUIRE(!result.commercial_ready);
    MK_REQUIRE(result.required_rows == kRequiredCommercialRows.size());
    MK_REQUIRE(result.ready_rows == 1U);
    MK_REQUIRE(result.dependency_gated_rows == kRequiredCommercialRows.size() - 1U);
}

MK_TEST("macos_metal_ready_does_not_promote_ios_metal") {
    const std::vector<ReadinessRow> rows{
        ready_row("environment_platform_macos_metal_ready"),
    };

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.highest_commercial_ready);
    MK_REQUIRE(!result.commercial_ready);
    MK_REQUIRE(result.ready_rows == 1U);
    MK_REQUIRE(result.dependency_gated_rows == kRequiredCommercialRows.size() - 1U);
}

MK_TEST("backend_parity_ready_does_not_promote_all_platform_when_linux_android_ios_missing") {
    const auto rows = all_ready_except({
        "environment_platform_linux_vulkan_ready",
        "environment_platform_ios_metal_ready",
        "environment_platform_android_vulkan_ready",
    });

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.highest_commercial_ready);
    MK_REQUIRE(!result.commercial_ready);
    MK_REQUIRE(result.ready_rows == kRequiredCommercialRows.size() - 3U);
    MK_REQUIRE(result.dependency_gated_rows == 3U);
}

MK_TEST("asset_library_counts_without_license_keep_ready_0") {
    auto rows = all_ready_rows();
    set_row_status(rows, "environment_aaa_preset_asset_library_ready", RowStatus::dependency_gated,
                   "environment.aaa_preset_asset_library.license_records");

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.highest_commercial_ready);
    MK_REQUIRE(!result.commercial_ready);
    MK_REQUIRE(result.ready_rows == kRequiredCommercialRows.size() - 1U);
    MK_REQUIRE(result.dependency_gated_rows == 1U);
}

MK_TEST("weather_visual_quality_without_solver_validation_keeps_ready_0") {
    auto rows = all_ready_rows();
    set_row_status(rows, "environment_physical_weather_simulation_ready", RowStatus::dependency_gated,
                   "environment.physical_weather_simulation.solver_validation");

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.highest_commercial_ready);
    MK_REQUIRE(!result.commercial_ready);
    MK_REQUIRE(result.ready_rows == kRequiredCommercialRows.size() - 1U);
    MK_REQUIRE(result.dependency_gated_rows == 1U);
}

MK_TEST("artist_workflow_value_rows_without_visible_shell_keep_ready_0") {
    auto rows = all_ready_rows();
    set_row_status(rows, "environment_artist_workflow_production_ready", RowStatus::dependency_gated,
                   "environment.artist_workflow.visible_shell_execution");

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.highest_commercial_ready);
    MK_REQUIRE(!result.commercial_ready);
    MK_REQUIRE(result.ready_rows == kRequiredCommercialRows.size() - 1U);
    MK_REQUIRE(result.dependency_gated_rows == 1U);
}

MK_TEST("native_handle_access_keeps_ready_0") {
    auto rows = all_ready_rows();
    rows.front().native_handle_access = true;

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.highest_commercial_ready);
    MK_REQUIRE(!result.commercial_ready);
    MK_REQUIRE(result.ready_rows == kRequiredCommercialRows.size());
    MK_REQUIRE(result.native_handle_access);
}

MK_TEST("diagnostics_keep_ready_0") {
    auto rows = all_ready_rows();
    rows[11].diagnostics = true;

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.highest_commercial_ready);
    MK_REQUIRE(!result.commercial_ready);
    MK_REQUIRE(result.ready_rows == kRequiredCommercialRows.size());
    MK_REQUIRE(result.diagnostics);
}

MK_TEST("duplicate_required_row_id_keeps_ready_0") {
    auto rows = all_ready_rows();
    rows.push_back(ReadinessRow{
        .id = "environment_broad_optimization_ready",
        .status = RowStatus::blocked,
        .evidence_recipe_id = "recipe.duplicate.broad_optimization",
    });

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.highest_commercial_ready);
    MK_REQUIRE(!result.commercial_ready);
    MK_REQUIRE(result.ready_rows == kRequiredCommercialRows.size());
    MK_REQUIRE(result.diagnostics);
}

int main() {
    return mirakana::test::run_all();
}
