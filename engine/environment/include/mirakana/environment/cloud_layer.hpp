// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class EnvironmentCloudLayerMode : std::uint8_t {
    none = 0,
    equirectangular_2d,
    volumetric,
};

enum class EnvironmentCloudIblContributionMode : std::uint8_t {
    unknown = 0,
    none,
    sky_tint_only,
    ambient_factor,
};

enum class EnvironmentCloudLayerValidationStatus : std::uint8_t {
    valid = 0,
    invalid,
};

enum class EnvironmentCloudLayerDiagnosticCode : std::uint8_t {
    none = 0,
    unsupported_mode,
    invalid_coverage,
    invalid_opacity,
    invalid_altitude,
    invalid_wind_velocity,
    invalid_cloud_map_reference,
    invalid_flow_map_reference,
    invalid_sky_tint_response,
    invalid_time_of_day_response,
    invalid_ibl_contribution_mode,
    invalid_ibl_contribution,
    unsupported_volumetric_clouds,
    unsupported_native_handle_claim,
};

struct EnvironmentCloudLayerDesc {
    EnvironmentCloudLayerMode mode{EnvironmentCloudLayerMode::equirectangular_2d};
    float coverage{0.0F};
    float opacity{1.0F};
    float altitude_m{2000.0F};
    Vec2 wind_velocity_mps{};
    std::string cloud_map_asset_ref;
    std::string flow_map_asset_ref;
    Vec3 sky_tint_response{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float time_of_day_response{0.0F};
    EnvironmentCloudIblContributionMode ibl_contribution_mode{EnvironmentCloudIblContributionMode::none};
    float ibl_contribution{0.0F};
    bool request_volumetric_clouds{false};
    bool request_native_handle_access{false};
};

struct EnvironmentCloudLayerDiagnostic {
    EnvironmentCloudLayerDiagnosticCode code{EnvironmentCloudLayerDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct EnvironmentCloudLayerValidationResult {
    EnvironmentCloudLayerValidationStatus status{EnvironmentCloudLayerValidationStatus::invalid};
    std::vector<EnvironmentCloudLayerDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] EnvironmentCloudLayerValidationResult
validate_environment_cloud_layer(const EnvironmentCloudLayerDesc& desc);

[[nodiscard]] bool is_valid_environment_cloud_layer(const EnvironmentCloudLayerDesc& desc) noexcept;

[[nodiscard]] bool has_environment_cloud_layer_diagnostic(const EnvironmentCloudLayerValidationResult& result,
                                                          EnvironmentCloudLayerDiagnosticCode code) noexcept;

} // namespace mirakana
