// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/cloud_layer.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

constexpr float max_altitude_m = 20000.0F;
constexpr float max_abs_wind_mps = 250.0F;

void add_diagnostic(EnvironmentCloudLayerValidationResult& result, EnvironmentCloudLayerDiagnosticCode code,
                    std::string field, std::string message) {
    result.diagnostics.push_back(EnvironmentCloudLayerDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] bool finite_in_range(float value, float minimum, float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

[[nodiscard]] bool valid_unit_color(Vec3 value) noexcept {
    return finite_in_range(value.x, 0.0F, 1.0F) && finite_in_range(value.y, 0.0F, 1.0F) &&
           finite_in_range(value.z, 0.0F, 1.0F);
}

[[nodiscard]] bool valid_wind_velocity(Vec2 value) noexcept {
    return finite_in_range(value.x, -max_abs_wind_mps, max_abs_wind_mps) &&
           finite_in_range(value.y, -max_abs_wind_mps, max_abs_wind_mps);
}

[[nodiscard]] bool contains_forbidden_token(std::string_view value) noexcept {
    constexpr std::string_view forbidden_tokens[] = {
        "native", "backend", "d3d", "d3d12", "dxgi", "vulkan", "metal", "imgui", "sdl", "sdl3",
    };
    return std::ranges::any_of(forbidden_tokens,
                               [value](std::string_view token) { return value.find(token) != std::string_view::npos; });
}

[[nodiscard]] bool valid_asset_reference(std::string_view value) noexcept {
    if (value.empty() || value.size() > 256U || contains_forbidden_token(value) ||
        value.find("..") != std::string_view::npos) {
        return false;
    }
    return std::ranges::all_of(value, [](char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '/';
    });
}

[[nodiscard]] bool valid_ibl_mode(EnvironmentCloudIblContributionMode mode) noexcept {
    switch (mode) {
    case EnvironmentCloudIblContributionMode::none:
    case EnvironmentCloudIblContributionMode::sky_tint_only:
    case EnvironmentCloudIblContributionMode::ambient_factor:
        return true;
    case EnvironmentCloudIblContributionMode::unknown:
        return false;
    }
    return false;
}

} // namespace

bool EnvironmentCloudLayerValidationResult::succeeded() const noexcept {
    return diagnostics.empty();
}

EnvironmentCloudLayerValidationResult validate_environment_cloud_layer(const EnvironmentCloudLayerDesc& desc) {
    EnvironmentCloudLayerValidationResult result{
        .status = EnvironmentCloudLayerValidationStatus::valid,
    };

    if (desc.mode != EnvironmentCloudLayerMode::equirectangular_2d) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::unsupported_mode, "mode",
                       "cloud layer v1 supports only the cheap equirectangular 2D sky layer mode");
    }
    if (!finite_in_range(desc.coverage, 0.0F, 1.0F)) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::invalid_coverage, "coverage",
                       "cloud layer coverage must be finite and in [0, 1]");
    }
    if (!finite_in_range(desc.opacity, 0.0F, 1.0F)) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::invalid_opacity, "opacity",
                       "cloud layer opacity must be finite and in [0, 1]");
    }
    if (!finite_in_range(desc.altitude_m, 0.1F, max_altitude_m)) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::invalid_altitude, "altitude_m",
                       "cloud layer altitude must be finite and positive");
    }
    if (!valid_wind_velocity(desc.wind_velocity_mps)) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::invalid_wind_velocity, "wind_velocity_mps",
                       "cloud layer wind velocity must be finite and bounded");
    }
    if (!valid_asset_reference(desc.cloud_map_asset_ref)) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::invalid_cloud_map_reference, "cloud_map_asset_ref",
                       "cloud layer cloud map reference must be a first-party asset id without native/backend tokens");
    }
    if (!valid_asset_reference(desc.flow_map_asset_ref)) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::invalid_flow_map_reference, "flow_map_asset_ref",
                       "cloud layer flow map reference must be a first-party asset id without native/backend tokens");
    }
    if (!valid_unit_color(desc.sky_tint_response)) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::invalid_sky_tint_response, "sky_tint_response",
                       "cloud layer sky tint response must be a finite unit linear color");
    }
    if (!finite_in_range(desc.time_of_day_response, 0.0F, 1.0F)) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::invalid_time_of_day_response,
                       "time_of_day_response", "cloud layer time-of-day response must be finite and in [0, 1]");
    }
    if (!valid_ibl_mode(desc.ibl_contribution_mode)) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::invalid_ibl_contribution_mode,
                       "ibl_contribution_mode", "cloud layer IBL contribution mode must be supported");
    }
    if (!finite_in_range(desc.ibl_contribution, 0.0F, 1.0F)) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::invalid_ibl_contribution, "ibl_contribution",
                       "cloud layer IBL contribution must be finite and in [0, 1]");
    }
    if (desc.request_volumetric_clouds) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::unsupported_volumetric_clouds,
                       "request_volumetric_clouds",
                       "cloud layer v1 does not claim volumetric cloud rendering or froxel volume execution");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(result, EnvironmentCloudLayerDiagnosticCode::unsupported_native_handle_claim,
                       "request_native_handle_access", "cloud layer v1 must not expose native renderer handles");
    }

    if (!result.succeeded()) {
        result.status = EnvironmentCloudLayerValidationStatus::invalid;
    }

    return result;
}

bool is_valid_environment_cloud_layer(const EnvironmentCloudLayerDesc& desc) noexcept {
    return validate_environment_cloud_layer(desc).succeeded();
}

bool has_environment_cloud_layer_diagnostic(const EnvironmentCloudLayerValidationResult& result,
                                            EnvironmentCloudLayerDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
