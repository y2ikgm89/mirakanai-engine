// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/environment_backend_parity_v2.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace mirakana {
namespace {

constexpr std::array kRequiredFeatures{
    EnvironmentBackendParityV2Feature::physical_sky,
    EnvironmentBackendParityV2Feature::height_fog,
    EnvironmentBackendParityV2Feature::volumetric_fog,
    EnvironmentBackendParityV2Feature::volumetric_cloud,
    EnvironmentBackendParityV2Feature::cloud_layer,
    EnvironmentBackendParityV2Feature::rain_precipitation,
    EnvironmentBackendParityV2Feature::snow_precipitation,
    EnvironmentBackendParityV2Feature::material_weathering,
    EnvironmentBackendParityV2Feature::environment_lighting_ibl,
    EnvironmentBackendParityV2Feature::postprocess_depth_input,
    EnvironmentBackendParityV2Feature::texture_payload_rgba8_upload,
    EnvironmentBackendParityV2Feature::texture_payload_bc7_or_astc_upload,
    EnvironmentBackendParityV2Feature::weather_solver_gpu,
    EnvironmentBackendParityV2Feature::debug_profiling_policy,
    EnvironmentBackendParityV2Feature::quality_budget,
};

constexpr std::array kRequiredBackends{
    rhi::BackendKind::d3d12,
    rhi::BackendKind::vulkan,
    rhi::BackendKind::metal,
};

[[nodiscard]] std::size_t backend_index(rhi::BackendKind backend) noexcept {
    switch (backend) {
    case rhi::BackendKind::d3d12:
        return 0U;
    case rhi::BackendKind::vulkan:
        return 1U;
    case rhi::BackendKind::metal:
        return 2U;
    case rhi::BackendKind::null:
        break;
    }
    return kRequiredBackends.size();
}

[[nodiscard]] std::string_view canonical_feature_id(EnvironmentBackendParityV2Feature feature) noexcept {
    switch (feature) {
    case EnvironmentBackendParityV2Feature::physical_sky:
        return "physical_sky";
    case EnvironmentBackendParityV2Feature::height_fog:
        return "height_fog";
    case EnvironmentBackendParityV2Feature::volumetric_fog:
        return "volumetric_fog";
    case EnvironmentBackendParityV2Feature::volumetric_cloud:
        return "volumetric_cloud";
    case EnvironmentBackendParityV2Feature::cloud_layer:
        return "cloud_layer";
    case EnvironmentBackendParityV2Feature::rain_precipitation:
        return "rain_precipitation";
    case EnvironmentBackendParityV2Feature::snow_precipitation:
        return "snow_precipitation";
    case EnvironmentBackendParityV2Feature::material_weathering:
        return "material_weathering";
    case EnvironmentBackendParityV2Feature::environment_lighting_ibl:
        return "environment_lighting_ibl";
    case EnvironmentBackendParityV2Feature::postprocess_depth_input:
        return "postprocess_depth_input";
    case EnvironmentBackendParityV2Feature::texture_payload_rgba8_upload:
        return "texture_payload_rgba8_upload";
    case EnvironmentBackendParityV2Feature::texture_payload_bc7_or_astc_upload:
        return "texture_payload_bc7_or_astc_upload";
    case EnvironmentBackendParityV2Feature::weather_solver_gpu:
        return "weather_solver_gpu";
    case EnvironmentBackendParityV2Feature::debug_profiling_policy:
        return "debug_profiling_policy";
    case EnvironmentBackendParityV2Feature::quality_budget:
        return "quality_budget";
    }
    return "";
}

[[nodiscard]] std::size_t feature_index(EnvironmentBackendParityV2Feature feature) noexcept {
    for (std::size_t index{0U}; index < kRequiredFeatures.size(); ++index) {
        if (kRequiredFeatures[index] == feature) {
            return index;
        }
    }
    return kRequiredFeatures.size();
}

[[nodiscard]] std::string_view aggregate_recipe_id(rhi::BackendKind backend) noexcept {
    switch (backend) {
    case rhi::BackendKind::d3d12:
        return "desktop-runtime-sample-game-environment-ready-aggregate";
    case rhi::BackendKind::vulkan:
        return "desktop-runtime-sample-game-environment-vulkan-strict-aggregate";
    case rhi::BackendKind::metal:
        return "renderer-metal-environment-aggregate-apple-host-evidence";
    case rhi::BackendKind::null:
        break;
    }
    return "";
}

[[nodiscard]] std::string_view host_gate_id(rhi::BackendKind backend) noexcept {
    switch (backend) {
    case rhi::BackendKind::d3d12:
        return "d3d12-windows-primary";
    case rhi::BackendKind::vulkan:
        return "vulkan-strict";
    case rhi::BackendKind::metal:
        return "metal-apple";
    case rhi::BackendKind::null:
        break;
    }
    return "";
}

[[nodiscard]] bool ready_row(const EnvironmentBackendParityV2Row& row) noexcept {
    return row.status == EnvironmentBackendParityV2RowStatus::ready &&
           row.feature_id == canonical_feature_id(row.feature) &&
           row.aggregate_recipe_id == aggregate_recipe_id(row.backend) &&
           row.host_gate_id == host_gate_id(row.backend) && row.host_matches && row.feature_present &&
           row.backend_local_execution_ready && row.shader_or_pipeline_ready && row.synchronization_ready &&
           row.readback_or_output_ready && row.quality_budget_ready && row.resource_budget_ready &&
           row.package_counter_ready && !row.native_handle_access && !row.inferred_from_other_backend &&
           !row.diagnostics;
}

[[nodiscard]] bool host_gated_row(const EnvironmentBackendParityV2Row& row) noexcept {
    return row.status == EnvironmentBackendParityV2RowStatus::host_gated &&
           row.host_gate_id == host_gate_id(row.backend) && !row.host_matches;
}

[[nodiscard]] bool
backend_ready(const std::array<std::array<bool, kRequiredFeatures.size()>, kRequiredBackends.size()>& ready,
              std::size_t backend) noexcept {
    for (std::size_t feature{0U}; feature < kRequiredFeatures.size(); ++feature) {
        if (!ready[backend][feature]) {
            return false;
        }
    }
    return true;
}

} // namespace

EnvironmentBackendParityV2Result
evaluate_environment_backend_parity_v2(std::span<const EnvironmentBackendParityV2Row> rows) {
    EnvironmentBackendParityV2Result result{
        .required_features = kRequiredFeatures.size(),
        .required_backends = kRequiredBackends.size(),
        .required_rows = kRequiredFeatures.size() * kRequiredBackends.size(),
        .missing_rows = 0U,
    };

    if (rows.empty()) {
        result.missing_rows = result.required_rows;
        result.status = EnvironmentBackendParityV2Status::no_rows;
        return result;
    }

    std::array<std::array<bool, kRequiredFeatures.size()>, kRequiredBackends.size()> seen{};
    std::array<std::array<bool, kRequiredFeatures.size()>, kRequiredBackends.size()> ready{};

    for (const auto& row : rows) {
        const auto backend = backend_index(row.backend);
        const auto feature = feature_index(row.feature);
        result.native_handle_access = result.native_handle_access || row.native_handle_access;
        result.inferred_from_other_backend = result.inferred_from_other_backend || row.inferred_from_other_backend;
        result.diagnostics =
            result.diagnostics || row.diagnostics || row.native_handle_access || row.inferred_from_other_backend;

        if (backend >= kRequiredBackends.size() || feature >= kRequiredFeatures.size() || seen[backend][feature]) {
            ++result.blocked_rows;
            result.diagnostics = true;
            continue;
        }
        seen[backend][feature] = true;

        switch (row.status) {
        case EnvironmentBackendParityV2RowStatus::ready:
            if (ready_row(row)) {
                ready[backend][feature] = true;
                ++result.ready_rows;
            } else {
                ++result.blocked_rows;
                result.diagnostics = true;
            }
            break;
        case EnvironmentBackendParityV2RowStatus::host_gated:
            ++result.host_gated_rows;
            if (!host_gated_row(row)) {
                result.diagnostics = true;
            }
            break;
        case EnvironmentBackendParityV2RowStatus::blocked:
            ++result.blocked_rows;
            result.diagnostics = true;
            break;
        case EnvironmentBackendParityV2RowStatus::unsupported:
            ++result.unsupported_rows;
            break;
        case EnvironmentBackendParityV2RowStatus::missing:
            ++result.missing_rows;
            break;
        }
    }

    for (std::size_t backend{0U}; backend < kRequiredBackends.size(); ++backend) {
        for (std::size_t feature{0U}; feature < kRequiredFeatures.size(); ++feature) {
            if (!seen[backend][feature]) {
                ++result.missing_rows;
            }
        }
    }

    result.d3d12_ready = backend_ready(ready, 0U);
    result.vulkan_ready = backend_ready(ready, 1U);
    result.metal_ready = backend_ready(ready, 2U);
    result.host_validated_backends =
        (result.d3d12_ready ? 1U : 0U) + (result.vulkan_ready ? 1U : 0U) + (result.metal_ready ? 1U : 0U);

    result.environment_backend_parity_ready =
        result.d3d12_ready && result.vulkan_ready && result.metal_ready && result.missing_rows == 0U &&
        result.host_gated_rows == 0U && result.blocked_rows == 0U && result.unsupported_rows == 0U &&
        !result.native_handle_access && !result.inferred_from_other_backend && !result.diagnostics;

    if (result.environment_backend_parity_ready) {
        result.status = EnvironmentBackendParityV2Status::ready;
    } else if (result.blocked_rows > 0U || result.diagnostics) {
        result.status = EnvironmentBackendParityV2Status::blocked;
    } else {
        result.status = EnvironmentBackendParityV2Status::host_evidence_required;
    }
    return result;
}

} // namespace mirakana
