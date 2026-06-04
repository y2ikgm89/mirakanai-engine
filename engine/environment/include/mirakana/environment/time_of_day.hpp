// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/environment/environment_profile.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class EnvironmentTimeOfDayPlanStatus : std::uint8_t {
    blocked = 0,
    planned,
};

enum class EnvironmentCelestialBodyKind : std::uint8_t {
    unknown = 0,
    sun,
    moon,
};

enum class EnvironmentWeatherBlendMode : std::uint8_t {
    unknown = 0,
    linear,
    smoothstep,
};

enum class EnvironmentTimeOfDayDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_day_time,
    invalid_moon_phase,
    invalid_environment_profile,
    invalid_profile_blend_weight,
    invalid_profile_blend_total_weight,
    invalid_transition_duration,
    invalid_transition_elapsed,
    unsupported_blend_mode,
    invalid_exposure,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
    unsupported_profile_mutation,
};

struct EnvironmentWeatherBlendLayerDesc {
    EnvironmentProfileDesc profile;
    float weight{1.0F};
};

struct EnvironmentWeatherTransitionDesc {
    EnvironmentWeatherKind from_weather{EnvironmentWeatherKind::clear};
    EnvironmentWeatherKind to_weather{EnvironmentWeatherKind::clear};
    float duration_seconds{1.0F};
    float elapsed_seconds{0.0F};
    EnvironmentWeatherBlendMode blend_mode{EnvironmentWeatherBlendMode::linear};
};

struct EnvironmentExposureIntentDesc {
    float fixed_exposure_ev100{12.0F};
    float min_exposure_ev100{-6.0F};
    float max_exposure_ev100{18.0F};
    bool auto_exposure_enabled{false};
    float adaptation_seconds{1.0F};
};

struct EnvironmentTimeOfDayPlanDesc {
    float normalized_day_time{0.5F};
    float moon_phase{0.5F};
    std::span<const EnvironmentWeatherBlendLayerDesc> profile_blends;
    EnvironmentWeatherTransitionDesc weather_transition;
    EnvironmentExposureIntentDesc exposure;
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
    bool request_profile_mutation{false};
};

struct EnvironmentCelestialRow {
    EnvironmentCelestialBodyKind body{EnvironmentCelestialBodyKind::unknown};
    Vec3 direction{};
    Vec3 color{};
    float illuminance_lux{0.0F};
    float angular_radius_radians{0.0F};
    float moon_phase{0.0F};
    std::uint32_t atmosphere_light_index{0};
    bool visible_disk{false};
    bool affects_atmosphere{false};
    bool affects_clouds{false};
    bool casts_environment_shadows{false};
};

struct EnvironmentProfileBlendRow {
    std::string profile_id;
    EnvironmentWeatherKind weather{EnvironmentWeatherKind::clear};
    float source_weight{0.0F};
    float normalized_weight{0.0F};
};

struct EnvironmentWeatherTransitionRow {
    EnvironmentWeatherKind from_weather{EnvironmentWeatherKind::clear};
    EnvironmentWeatherKind to_weather{EnvironmentWeatherKind::clear};
    float duration_seconds{0.0F};
    float elapsed_seconds{0.0F};
    float normalized_progress{0.0F};
    float blend_weight{0.0F};
    EnvironmentWeatherBlendMode blend_mode{EnvironmentWeatherBlendMode::unknown};
};

struct EnvironmentExposureIntentRow {
    float fixed_exposure_ev100{0.0F};
    float min_exposure_ev100{0.0F};
    float max_exposure_ev100{0.0F};
    bool auto_exposure_enabled{false};
    float adaptation_seconds{0.0F};
};

struct EnvironmentTimeOfDayDiagnostic {
    EnvironmentTimeOfDayDiagnosticCode code{EnvironmentTimeOfDayDiagnosticCode::none};
    std::string field;
    std::uint32_t profile_index{0};
    std::string message;
};

struct EnvironmentTimeOfDayPlan {
    EnvironmentTimeOfDayPlanStatus status{EnvironmentTimeOfDayPlanStatus::blocked};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool mutates_profiles{false};
    std::uint64_t replay_hash{0};
    std::vector<EnvironmentCelestialRow> celestial_rows;
    std::vector<EnvironmentProfileBlendRow> profile_blend_rows;
    std::vector<EnvironmentWeatherTransitionRow> weather_transition_rows;
    std::vector<EnvironmentExposureIntentRow> exposure_rows;
    std::vector<EnvironmentTimeOfDayDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] EnvironmentTimeOfDayPlan plan_environment_time_of_day(const EnvironmentTimeOfDayPlanDesc& desc);

[[nodiscard]] bool has_environment_time_of_day_diagnostic(const EnvironmentTimeOfDayPlan& plan,
                                                          EnvironmentTimeOfDayDiagnosticCode code) noexcept;

} // namespace mirakana
