// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/weather.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(EnvironmentPrecipitationPlan& plan, EnvironmentPrecipitationDiagnosticCode code, std::string field,
                    std::string message) {
    plan.diagnostics.push_back(EnvironmentPrecipitationDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] bool finite_in_range(float value, float minimum, float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

[[nodiscard]] bool valid_weather_kind(EnvironmentWeatherKind value) noexcept {
    switch (value) {
    case EnvironmentWeatherKind::clear:
    case EnvironmentWeatherKind::cloudy:
    case EnvironmentWeatherKind::rain:
    case EnvironmentWeatherKind::storm:
    case EnvironmentWeatherKind::snow:
    case EnvironmentWeatherKind::foggy:
    case EnvironmentWeatherKind::dust:
    case EnvironmentWeatherKind::ash:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_precipitation_kind(EnvironmentPrecipitationKind value) noexcept {
    switch (value) {
    case EnvironmentPrecipitationKind::none:
    case EnvironmentPrecipitationKind::rain:
    case EnvironmentPrecipitationKind::snow:
    case EnvironmentPrecipitationKind::sleet:
    case EnvironmentPrecipitationKind::hail:
    case EnvironmentPrecipitationKind::ash:
    case EnvironmentPrecipitationKind::dust:
        return true;
    }
    return false;
}

[[nodiscard]] EnvironmentPrecipitationKind
effective_precipitation_kind(EnvironmentWeatherKind weather, EnvironmentPrecipitationKind requested) noexcept {
    if (requested != EnvironmentPrecipitationKind::none || !valid_weather_kind(weather)) {
        return requested;
    }

    switch (weather) {
    case EnvironmentWeatherKind::rain:
    case EnvironmentWeatherKind::storm:
        return EnvironmentPrecipitationKind::rain;
    case EnvironmentWeatherKind::snow:
        return EnvironmentPrecipitationKind::snow;
    case EnvironmentWeatherKind::dust:
        return EnvironmentPrecipitationKind::dust;
    case EnvironmentWeatherKind::ash:
        return EnvironmentPrecipitationKind::ash;
    case EnvironmentWeatherKind::clear:
    case EnvironmentWeatherKind::cloudy:
    case EnvironmentWeatherKind::foggy:
        return EnvironmentPrecipitationKind::none;
    }
    return requested;
}

[[nodiscard]] bool precipitation_has_particles(EnvironmentPrecipitationKind kind, float intensity) noexcept {
    return kind != EnvironmentPrecipitationKind::none && finite_in_range(intensity, 0.0F, 1.0F) && intensity > 0.0F;
}

[[nodiscard]] bool precipitation_wets_surfaces(EnvironmentPrecipitationKind kind) noexcept {
    switch (kind) {
    case EnvironmentPrecipitationKind::rain:
    case EnvironmentPrecipitationKind::sleet:
    case EnvironmentPrecipitationKind::hail:
        return true;
    case EnvironmentPrecipitationKind::none:
    case EnvironmentPrecipitationKind::snow:
    case EnvironmentPrecipitationKind::ash:
    case EnvironmentPrecipitationKind::dust:
        return false;
    }
    return false;
}

[[nodiscard]] float base_spawn_rate(EnvironmentWeatherKind weather, EnvironmentPrecipitationKind kind) noexcept {
    if (weather == EnvironmentWeatherKind::storm) {
        return 3500.0F;
    }

    switch (kind) {
    case EnvironmentPrecipitationKind::rain:
        return 2400.0F;
    case EnvironmentPrecipitationKind::snow:
        return 900.0F;
    case EnvironmentPrecipitationKind::sleet:
        return 1700.0F;
    case EnvironmentPrecipitationKind::hail:
        return 700.0F;
    case EnvironmentPrecipitationKind::ash:
        return 500.0F;
    case EnvironmentPrecipitationKind::dust:
        return 650.0F;
    case EnvironmentPrecipitationKind::none:
        return 0.0F;
    }
    return 0.0F;
}

void validate_precipitation_desc(EnvironmentPrecipitationPlan& plan, const EnvironmentPrecipitationPlanDesc& desc) {
    if (!validate_environment_profile(desc.environment).succeeded()) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::invalid_environment_profile, "environment",
                       "weather precipitation planning requires a valid environment profile");
    }
    if (!valid_weather_kind(desc.environment.weather)) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::unsupported_weather_kind, "weather",
                       "weather precipitation planning requires a supported weather kind");
    }
    if (!valid_precipitation_kind(desc.environment.precipitation.kind)) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::unsupported_precipitation_kind,
                       "precipitation.kind", "weather precipitation planning requires a supported precipitation kind");
    }
    if (!finite_in_range(desc.environment.precipitation.intensity, 0.0F, 1.0F)) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::invalid_intensity, "precipitation.intensity",
                       "precipitation intensity must be finite and in [0, 1]");
    }
    if (!std::isfinite(desc.environment.precipitation.particle_radius_mm) ||
        desc.environment.precipitation.particle_radius_mm <= 0.0F) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::invalid_particle_radius,
                       "precipitation.particle_radius_mm", "precipitation particle radius must be finite and positive");
    }
    if (!std::isfinite(desc.environment.precipitation.fall_speed_mps) ||
        desc.environment.precipitation.fall_speed_mps < 0.0F) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::invalid_fall_speed, "precipitation.fall_speed_mps",
                       "precipitation fall speed must be finite and non-negative");
    }
    if (!std::isfinite(desc.environment.precipitation.wind_speed_mps)) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::invalid_wind_speed, "precipitation.wind_speed_mps",
                       "precipitation wind speed must be finite");
    }

    const bool scene_precipitation_reaches_geometry =
        desc.scene_geometry_occlusion_required && desc.environment.precipitation.intensity > 0.0F;
    if (scene_precipitation_reaches_geometry && !desc.occlusion_policy_available) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::missing_occlusion_policy,
                       "occlusion_policy_available",
                       "rain, snow, dust, or ash reaching scene geometry requires an occlusion policy row");
    }

    if (desc.request_backend_execution) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::unsupported_backend_execution,
                       "request_backend_execution",
                       "weather precipitation planning must not invoke renderer, physics, or audio backends");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::unsupported_native_handle_claim,
                       "request_native_handle_access",
                       "weather precipitation planning must not expose native renderer or audio handles");
    }
    if (desc.request_material_mutation) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::unsupported_material_mutation,
                       "request_material_mutation",
                       "weather precipitation planning may emit wetness intent rows but must not mutate materials");
    }
    if (desc.request_audio_playback) {
        add_diagnostic(plan, EnvironmentPrecipitationDiagnosticCode::unsupported_audio_playback,
                       "request_audio_playback",
                       "weather precipitation planning may emit audio handoff rows but must not play audio");
    }
}

void append_audio_rows(EnvironmentPrecipitationPlan& plan, EnvironmentWeatherKind weather,
                       EnvironmentPrecipitationKind kind, float intensity, bool wets_surfaces) {
    if (kind == EnvironmentPrecipitationKind::rain || kind == EnvironmentPrecipitationKind::sleet ||
        kind == EnvironmentPrecipitationKind::hail) {
        plan.audio_handoff_rows.push_back(EnvironmentPrecipitationAudioHandoffRow{
            .cue = EnvironmentPrecipitationAudioCueKind::rain_loop,
            .intensity = intensity,
            .delay_seconds = 0.0F,
            .handoff_only = true,
        });
    }
    if (kind == EnvironmentPrecipitationKind::snow) {
        plan.audio_handoff_rows.push_back(EnvironmentPrecipitationAudioHandoffRow{
            .cue = EnvironmentPrecipitationAudioCueKind::snow_loop,
            .intensity = intensity,
            .delay_seconds = 0.0F,
            .handoff_only = true,
        });
    }
    if (kind == EnvironmentPrecipitationKind::dust) {
        plan.audio_handoff_rows.push_back(EnvironmentPrecipitationAudioHandoffRow{
            .cue = EnvironmentPrecipitationAudioCueKind::dust_wind,
            .intensity = intensity,
            .delay_seconds = 0.0F,
            .handoff_only = true,
        });
    }
    if (kind == EnvironmentPrecipitationKind::ash) {
        plan.audio_handoff_rows.push_back(EnvironmentPrecipitationAudioHandoffRow{
            .cue = EnvironmentPrecipitationAudioCueKind::ash_fall,
            .intensity = intensity,
            .delay_seconds = 0.0F,
            .handoff_only = true,
        });
    }
    if (wets_surfaces) {
        plan.audio_handoff_rows.push_back(EnvironmentPrecipitationAudioHandoffRow{
            .cue = EnvironmentPrecipitationAudioCueKind::indoor_muffling,
            .intensity = intensity,
            .delay_seconds = 0.0F,
            .handoff_only = true,
        });
    }
    if (weather == EnvironmentWeatherKind::storm) {
        plan.audio_handoff_rows.push_back(EnvironmentPrecipitationAudioHandoffRow{
            .cue = EnvironmentPrecipitationAudioCueKind::thunder_delay,
            .intensity = intensity,
            .delay_seconds = 1.5F,
            .handoff_only = true,
        });
        plan.audio_handoff_rows.push_back(EnvironmentPrecipitationAudioHandoffRow{
            .cue = EnvironmentPrecipitationAudioCueKind::storm_intensity,
            .intensity = intensity,
            .delay_seconds = 0.0F,
            .handoff_only = true,
        });
    }
}

void append_plan_rows(EnvironmentPrecipitationPlan& plan, const EnvironmentPrecipitationPlanDesc& desc) {
    const auto weather = desc.environment.weather;
    const auto kind = effective_precipitation_kind(weather, desc.environment.precipitation.kind);
    const auto intensity = desc.environment.precipitation.intensity;
    const bool has_particles = precipitation_has_particles(kind, intensity);
    const bool wets_surfaces = has_particles && precipitation_wets_surfaces(kind);

    plan.weather_rows.push_back(EnvironmentWeatherRow{
        .weather = weather,
        .precipitation_kind = kind,
        .intensity = intensity,
        .precipitation_enabled = has_particles,
        .wet_surface_enabled = wets_surfaces,
        .audio_handoff_enabled = has_particles,
    });

    if (!has_particles) {
        return;
    }

    plan.particle_rows.push_back(EnvironmentPrecipitationParticleRow{
        .kind = kind,
        .intensity = intensity,
        .spawn_rate_per_second = base_spawn_rate(weather, kind) * intensity,
        .particle_radius_mm = desc.environment.precipitation.particle_radius_mm,
        .fall_speed_mps = desc.environment.precipitation.fall_speed_mps,
        .wind_speed_mps = desc.environment.precipitation.wind_speed_mps,
        .camera_near_only = true,
        .gpu_particle_intent = true,
    });
    plan.occlusion_rows.push_back(EnvironmentPrecipitationOcclusionRow{
        .required = desc.scene_geometry_occlusion_required,
        .available = desc.occlusion_policy_available,
        .uses_scene_geometry_depth_mask = desc.scene_geometry_occlusion_required && desc.occlusion_policy_available,
        .uses_indoor_volume_mask = desc.occlusion_policy_available,
    });

    if (wets_surfaces) {
        plan.wetness_rows.push_back(EnvironmentSurfaceWetnessRow{
            .enabled = true,
            .intensity = intensity,
            .splash_intent = true,
            .ripple_intent = true,
            .mutates_materials = false,
        });
    }

    append_audio_rows(plan, weather, kind, intensity, wets_surfaces);
}

} // namespace

bool EnvironmentPrecipitationPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

EnvironmentPrecipitationPlan plan_environment_precipitation(const EnvironmentPrecipitationPlanDesc& desc) {
    EnvironmentPrecipitationPlan plan{
        .status = EnvironmentPrecipitationPlanStatus::planned,
    };

    validate_precipitation_desc(plan, desc);

    if (!plan.succeeded()) {
        plan.status = EnvironmentPrecipitationPlanStatus::blocked;
        return plan;
    }

    append_plan_rows(plan, desc);
    return plan;
}

bool has_environment_precipitation_diagnostic(const EnvironmentPrecipitationPlan& plan,
                                              EnvironmentPrecipitationDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

bool has_environment_precipitation_audio_cue(const EnvironmentPrecipitationPlan& plan,
                                             EnvironmentPrecipitationAudioCueKind cue) noexcept {
    return std::ranges::any_of(plan.audio_handoff_rows, [cue](const auto& row) { return row.cue == cue; });
}

} // namespace mirakana
