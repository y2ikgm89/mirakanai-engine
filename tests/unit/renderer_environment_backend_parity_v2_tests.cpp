// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/environment_backend_parity_v2.hpp"

#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

using BackendId = mirakana::rhi::BackendKind;
using Feature = mirakana::EnvironmentBackendParityV2Feature;
using Row = mirakana::EnvironmentBackendParityV2Row;
using RowStatus = mirakana::EnvironmentBackendParityV2RowStatus;
using Status = mirakana::EnvironmentBackendParityV2Status;

constexpr Feature kRequiredFeatures[] = {
    Feature::physical_sky,
    Feature::height_fog,
    Feature::volumetric_fog,
    Feature::volumetric_cloud,
    Feature::cloud_layer,
    Feature::rain_precipitation,
    Feature::snow_precipitation,
    Feature::material_weathering,
    Feature::environment_lighting_ibl,
    Feature::postprocess_depth_input,
    Feature::texture_payload_rgba8_upload,
    Feature::texture_payload_bc7_or_astc_upload,
    Feature::weather_solver_gpu,
    Feature::debug_profiling_policy,
    Feature::quality_budget,
};

[[nodiscard]] std::string feature_id(Feature feature) {
    switch (feature) {
    case Feature::physical_sky:
        return "physical_sky";
    case Feature::height_fog:
        return "height_fog";
    case Feature::volumetric_fog:
        return "volumetric_fog";
    case Feature::volumetric_cloud:
        return "volumetric_cloud";
    case Feature::cloud_layer:
        return "cloud_layer";
    case Feature::rain_precipitation:
        return "rain_precipitation";
    case Feature::snow_precipitation:
        return "snow_precipitation";
    case Feature::material_weathering:
        return "material_weathering";
    case Feature::environment_lighting_ibl:
        return "environment_lighting_ibl";
    case Feature::postprocess_depth_input:
        return "postprocess_depth_input";
    case Feature::texture_payload_rgba8_upload:
        return "texture_payload_rgba8_upload";
    case Feature::texture_payload_bc7_or_astc_upload:
        return "texture_payload_bc7_or_astc_upload";
    case Feature::weather_solver_gpu:
        return "weather_solver_gpu";
    case Feature::debug_profiling_policy:
        return "debug_profiling_policy";
    case Feature::quality_budget:
        return "quality_budget";
    }
    return "unknown";
}

[[nodiscard]] std::string aggregate_recipe_id(BackendId backend) {
    switch (backend) {
    case BackendId::d3d12:
        return "desktop-runtime-sample-game-environment-ready-aggregate";
    case BackendId::vulkan:
        return "desktop-runtime-sample-game-environment-vulkan-strict-aggregate";
    case BackendId::metal:
        return "renderer-metal-environment-aggregate-apple-host-evidence";
    case BackendId::null:
        break;
    }
    return "unsupported";
}

[[nodiscard]] std::string host_gate_id(BackendId backend) {
    switch (backend) {
    case BackendId::d3d12:
        return "d3d12-windows-primary";
    case BackendId::vulkan:
        return "vulkan-strict";
    case BackendId::metal:
        return "metal-apple";
    case BackendId::null:
        break;
    }
    return "unsupported";
}

[[nodiscard]] Row ready_row(BackendId backend, Feature feature, std::uint32_t source_index) {
    return Row{
        .backend = backend,
        .feature = feature,
        .feature_id = feature_id(feature),
        .status = RowStatus::ready,
        .aggregate_recipe_id = aggregate_recipe_id(backend),
        .host_gate_id = host_gate_id(backend),
        .host_matches = true,
        .feature_present = true,
        .backend_local_execution_ready = true,
        .shader_or_pipeline_ready = true,
        .synchronization_ready = true,
        .readback_or_output_ready = true,
        .quality_budget_ready = true,
        .resource_budget_ready = true,
        .package_counter_ready = true,
        .source_index = source_index,
    };
}

[[nodiscard]] std::vector<Row> ready_rows_for_backend(BackendId backend, std::uint32_t& source_index) {
    std::vector<Row> rows;
    rows.reserve(std::size(kRequiredFeatures));
    for (const auto feature : kRequiredFeatures) {
        rows.push_back(ready_row(backend, feature, source_index++));
    }
    return rows;
}

void append_rows(std::vector<Row>& rows, std::vector<Row> more_rows) {
    rows.insert(rows.end(), more_rows.begin(), more_rows.end());
}

[[nodiscard]] std::vector<Row> all_backend_rows_ready() {
    std::vector<Row> rows;
    rows.reserve(45U);
    std::uint32_t source_index{1U};
    append_rows(rows, ready_rows_for_backend(BackendId::d3d12, source_index));
    append_rows(rows, ready_rows_for_backend(BackendId::vulkan, source_index));
    append_rows(rows, ready_rows_for_backend(BackendId::metal, source_index));
    return rows;
}

} // namespace

MK_TEST("d3d12_vulkan_ready_metal_missing_keeps_parity_0") {
    std::vector<Row> rows;
    std::uint32_t source_index{1U};
    append_rows(rows, ready_rows_for_backend(BackendId::d3d12, source_index));
    append_rows(rows, ready_rows_for_backend(BackendId::vulkan, source_index));

    const auto result = mirakana::evaluate_environment_backend_parity_v2(rows);

    MK_REQUIRE(result.status == Status::host_evidence_required);
    MK_REQUIRE(result.d3d12_ready);
    MK_REQUIRE(result.vulkan_ready);
    MK_REQUIRE(!result.metal_ready);
    MK_REQUIRE(!result.environment_backend_parity_ready);
    MK_REQUIRE(result.required_rows == 45U);
    MK_REQUIRE(result.ready_rows == 30U);
    MK_REQUIRE(result.missing_rows == 15U);
    MK_REQUIRE(result.host_validated_backends == 2U);
    MK_REQUIRE(!result.native_handle_access);
    MK_REQUIRE(!result.inferred_from_other_backend);
}

MK_TEST("macos_metal_ready_ios_metal_missing_keeps_all_platform_0") {
    const auto result = mirakana::evaluate_environment_backend_parity_v2(all_backend_rows_ready());

    MK_REQUIRE(result.status == Status::ready);
    MK_REQUIRE(result.environment_backend_parity_ready);
    MK_REQUIRE(!result.environment_all_platform_unconditional_ready);
    MK_REQUIRE(result.metal_ready);
    MK_REQUIRE(result.host_validated_backends == 3U);
}

MK_TEST("ready_rows_with_native_handle_access_keep_parity_0") {
    auto rows = all_backend_rows_ready();
    rows.front().native_handle_access = true;

    const auto result = mirakana::evaluate_environment_backend_parity_v2(rows);

    MK_REQUIRE(result.status == Status::blocked);
    MK_REQUIRE(!result.environment_backend_parity_ready);
    MK_REQUIRE(result.native_handle_access);
    MK_REQUIRE(result.diagnostics);
    MK_REQUIRE(result.blocked_rows == 1U);
}

MK_TEST("ready_rows_with_diagnostics_keep_parity_0") {
    auto rows = all_backend_rows_ready();
    rows[7].diagnostics = true;

    const auto result = mirakana::evaluate_environment_backend_parity_v2(rows);

    MK_REQUIRE(result.status == Status::blocked);
    MK_REQUIRE(!result.environment_backend_parity_ready);
    MK_REQUIRE(result.diagnostics);
    MK_REQUIRE(result.blocked_rows == 1U);
}

MK_TEST("explicit_missing_row_counts_as_missing_and_keeps_parity_0") {
    auto rows = all_backend_rows_ready();
    rows.front().status = RowStatus::missing;

    const auto result = mirakana::evaluate_environment_backend_parity_v2(rows);

    MK_REQUIRE(result.status == Status::host_evidence_required);
    MK_REQUIRE(!result.environment_backend_parity_ready);
    MK_REQUIRE(!result.d3d12_ready);
    MK_REQUIRE(result.vulkan_ready);
    MK_REQUIRE(result.metal_ready);
    MK_REQUIRE(result.ready_rows == 44U);
    MK_REQUIRE(result.missing_rows == 1U);
    MK_REQUIRE(result.blocked_rows == 0U);
    MK_REQUIRE(!result.diagnostics);
}

MK_TEST("all_backend_rows_ready_promotes_backend_parity_1") {
    const auto result = mirakana::evaluate_environment_backend_parity_v2(all_backend_rows_ready());

    MK_REQUIRE(result.status == Status::ready);
    MK_REQUIRE(result.environment_backend_parity_ready);
    MK_REQUIRE(result.required_rows == 45U);
    MK_REQUIRE(result.required_features == 15U);
    MK_REQUIRE(result.ready_rows == 45U);
    MK_REQUIRE(result.missing_rows == 0U);
    MK_REQUIRE(result.host_gated_rows == 0U);
    MK_REQUIRE(result.blocked_rows == 0U);
    MK_REQUIRE(result.host_validated_backends == 3U);
    MK_REQUIRE(result.d3d12_ready);
    MK_REQUIRE(result.vulkan_ready);
    MK_REQUIRE(result.metal_ready);
    MK_REQUIRE(!result.invoked_gpu_commands);
    MK_REQUIRE(!result.native_handle_access);
    MK_REQUIRE(!result.diagnostics);
}

int main() {
    return mirakana::test::run_all();
}
