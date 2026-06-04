// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class EnvironmentSkyModel : std::uint8_t {
    none = 0,
    color,
    gradient,
    hdri,
    physical_atmosphere,
};

enum class EnvironmentWeatherKind : std::uint8_t {
    clear = 0,
    cloudy,
    rain,
    storm,
    snow,
    foggy,
    dust,
    ash,
};

enum class EnvironmentPrecipitationKind : std::uint8_t {
    none = 0,
    rain,
    snow,
    sleet,
    hail,
    ash,
    dust,
};

enum class EnvironmentProfileValidationStatus : std::uint8_t {
    valid = 0,
    invalid,
};

enum class EnvironmentProfileDiagnosticCode : std::uint8_t {
    none = 0,
    empty_profile_id,
    duplicate_profile_id,
    forbidden_profile_id_token,
    invalid_enum,
    invalid_direction,
    invalid_scalar,
    invalid_color,
};

struct EnvironmentSunMoonDesc {
    Vec3 direction{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    Vec3 color{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float illuminance_lux{100000.0F};
    float angular_radius_radians{0.00465F};
    bool visible_disk{true};
    bool affects_atmosphere{true};
    bool affects_clouds{true};
    bool casts_environment_shadows{true};
};

struct EnvironmentAtmosphereDesc {
    float planet_radius_km{6360.0F};
    float atmosphere_height_km{100.0F};
    Vec3 rayleigh_scattering{.x = 0.002F, .y = 0.005F, .z = 0.01F};
    Vec3 mie_scattering{.x = 0.004F, .y = 0.004F, .z = 0.004F};
    Vec3 ground_albedo{.x = 0.35F, .y = 0.35F, .z = 0.35F};
};

struct EnvironmentFogDesc {
    bool enabled{false};
    float density{0.0F};
    float height_falloff{1.0F};
    Vec3 albedo{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float anisotropy{0.0F};
};

struct EnvironmentPrecipitationDesc {
    EnvironmentPrecipitationKind kind{EnvironmentPrecipitationKind::none};
    float intensity{0.0F};
    float particle_radius_mm{0.5F};
    float fall_speed_mps{5.0F};
    float wind_speed_mps{0.0F};
};

struct EnvironmentProfileDesc {
    std::string id{"default_environment"};
    EnvironmentSkyModel sky_model{EnvironmentSkyModel::physical_atmosphere};
    EnvironmentWeatherKind weather{EnvironmentWeatherKind::clear};
    EnvironmentSunMoonDesc sun{};
    EnvironmentSunMoonDesc moon{
        .direction = Vec3{.x = 0.2F, .y = -1.0F, .z = 0.1F},
        .color = Vec3{.x = 0.6F, .y = 0.7F, .z = 1.0F},
        .illuminance_lux = 0.25F,
        .angular_radius_radians = 0.00465F,
        .visible_disk = true,
        .affects_atmosphere = true,
        .affects_clouds = true,
        .casts_environment_shadows = false,
    };
    EnvironmentAtmosphereDesc atmosphere{};
    EnvironmentFogDesc fog{};
    EnvironmentPrecipitationDesc precipitation{};
};

struct EnvironmentProfileDiagnostic {
    EnvironmentProfileDiagnosticCode code{EnvironmentProfileDiagnosticCode::none};
    std::uint32_t profile_index{0U};
    std::string field;
    std::string id;
    std::string message;
};

struct EnvironmentProfileValidationResult {
    EnvironmentProfileValidationStatus status{EnvironmentProfileValidationStatus::invalid};
    std::vector<EnvironmentProfileDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] EnvironmentProfileValidationResult validate_environment_profile(const EnvironmentProfileDesc& desc);

[[nodiscard]] EnvironmentProfileValidationResult
validate_environment_profiles(std::span<const EnvironmentProfileDesc> profiles);

[[nodiscard]] bool is_valid_environment_profile(const EnvironmentProfileDesc& desc) noexcept;

[[nodiscard]] bool has_environment_profile_diagnostic(const EnvironmentProfileValidationResult& result,
                                                      EnvironmentProfileDiagnosticCode code) noexcept;

} // namespace mirakana
