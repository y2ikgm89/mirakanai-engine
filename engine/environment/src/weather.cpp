// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/weather.hpp"

#include <algorithm>
#include <cmath>
#include <span>
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

void add_diagnostic(EnvironmentWeatherAudioPlaybackPlan& plan, EnvironmentWeatherAudioDiagnosticCode code,
                    std::string field, std::string message) {
    plan.diagnostics.push_back(EnvironmentWeatherAudioDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] bool finite_in_range(float value, float minimum, float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

[[nodiscard]] bool valid_audio_token(std::string_view value) noexcept {
    return !value.empty() && value.find_first_of("\r\n=") == std::string_view::npos;
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
            .cue = EnvironmentPrecipitationAudioCueKind::wind_loop,
            .intensity = intensity,
            .delay_seconds = 0.0F,
            .handoff_only = true,
        });
    }
}

[[nodiscard]] bool valid_audio_cue(EnvironmentPrecipitationAudioCueKind cue) noexcept {
    switch (cue) {
    case EnvironmentPrecipitationAudioCueKind::rain_loop:
    case EnvironmentPrecipitationAudioCueKind::snow_loop:
    case EnvironmentPrecipitationAudioCueKind::indoor_muffling:
    case EnvironmentPrecipitationAudioCueKind::thunder_delay:
    case EnvironmentPrecipitationAudioCueKind::storm_intensity:
    case EnvironmentPrecipitationAudioCueKind::wind_loop:
    case EnvironmentPrecipitationAudioCueKind::dust_wind:
    case EnvironmentPrecipitationAudioCueKind::ash_fall:
        return true;
    case EnvironmentPrecipitationAudioCueKind::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] const EnvironmentWeatherAudioCueBinding*
find_audio_cue_binding(std::span<const EnvironmentWeatherAudioCueBinding> bindings,
                       EnvironmentPrecipitationAudioCueKind cue) noexcept {
    const auto it = std::ranges::find_if(bindings, [cue](const auto& binding) { return binding.cue == cue; });
    return it == bindings.end() ? nullptr : &*it;
}

void validate_weather_audio_desc(EnvironmentWeatherAudioPlaybackPlan& plan,
                                 const EnvironmentWeatherAudioPlaybackDesc& desc) {
    if (desc.handoff_rows.empty()) {
        add_diagnostic(plan, EnvironmentWeatherAudioDiagnosticCode::missing_handoff_rows, "handoff_rows",
                       "weather audio playback planning requires precipitation audio handoff rows");
    }
    if (desc.request_runtime_backend_execution) {
        add_diagnostic(plan, EnvironmentWeatherAudioDiagnosticCode::unsupported_runtime_backend_execution,
                       "request_runtime_backend_execution",
                       "environment weather audio planning must not execute runtime, audio, or host backends");
    }
    if (desc.request_environment_audio_device_ownership) {
        add_diagnostic(plan, EnvironmentWeatherAudioDiagnosticCode::unsupported_audio_device_ownership,
                       "request_environment_audio_device_ownership",
                       "MK_environment emits value rows and must not own audio devices");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, EnvironmentWeatherAudioDiagnosticCode::unsupported_native_handle_access,
                       "request_native_handle_access",
                       "MK_environment weather audio planning must not expose native audio handles");
    }

    for (auto lhs = desc.cue_bindings.begin(); lhs != desc.cue_bindings.end(); ++lhs) {
        if (!valid_audio_cue(lhs->cue) || !valid_audio_token(lhs->cue_id) || !valid_audio_token(lhs->clip_asset_ref) ||
            !valid_audio_token(lhs->bus) || !finite_in_range(lhs->base_gain, 0.0F, 16.0F)) {
            add_diagnostic(plan, EnvironmentWeatherAudioDiagnosticCode::invalid_cue_binding, "cue_bindings",
                           "weather audio cue bindings require supported cue ids, asset refs, buses, and finite gain");
        }
        for (auto rhs = lhs + 1; rhs != desc.cue_bindings.end(); ++rhs) {
            if (lhs->cue == rhs->cue) {
                add_diagnostic(plan, EnvironmentWeatherAudioDiagnosticCode::duplicate_cue_binding, "cue_bindings",
                               "weather audio cue bindings must not duplicate a cue kind");
            }
        }
    }

    for (const auto& handoff : desc.handoff_rows) {
        if (!valid_audio_cue(handoff.cue)) {
            add_diagnostic(plan, EnvironmentWeatherAudioDiagnosticCode::unsupported_audio_cue, "handoff_rows.cue",
                           "weather audio handoff rows require a supported cue kind");
        }
        if (!finite_in_range(handoff.intensity, 0.0F, 1.0F)) {
            add_diagnostic(plan, EnvironmentWeatherAudioDiagnosticCode::invalid_handoff_intensity,
                           "handoff_rows.intensity", "weather audio handoff intensity must be finite and in [0, 1]");
        }
        if (find_audio_cue_binding(desc.cue_bindings, handoff.cue) == nullptr) {
            add_diagnostic(plan, EnvironmentWeatherAudioDiagnosticCode::missing_cue_binding, "cue_bindings",
                           "weather audio handoff rows require a matching cue binding");
        }
    }
}

void append_weather_audio_trigger_rows(EnvironmentWeatherAudioPlaybackPlan& plan,
                                       const EnvironmentWeatherAudioPlaybackDesc& desc) {
    plan.trigger_rows.reserve(desc.handoff_rows.size());
    for (const auto& handoff : desc.handoff_rows) {
        const auto* binding = find_audio_cue_binding(desc.cue_bindings, handoff.cue);
        if (binding == nullptr) {
            continue;
        }
        plan.trigger_rows.push_back(EnvironmentWeatherAudioTriggerRow{
            .cue = handoff.cue,
            .cue_id = binding->cue_id,
            .clip_asset_ref = binding->clip_asset_ref,
            .bus = binding->bus,
            .gain = std::clamp(handoff.intensity * binding->base_gain, 0.0F, 16.0F),
            .delay_seconds = handoff.delay_seconds,
            .looping = binding->looping,
            .one_shot = binding->one_shot,
            .device_owned_by_environment = false,
            .native_handle_access = false,
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

bool EnvironmentWeatherAudioPlaybackPlan::succeeded() const noexcept {
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

std::vector<EnvironmentWeatherAudioCueBinding> default_environment_weather_audio_cue_bindings() {
    return {
        EnvironmentWeatherAudioCueBinding{
            .cue = EnvironmentPrecipitationAudioCueKind::rain_loop,
            .cue_id = "environment.weather.rain.loop",
            .clip_asset_ref = "audio/environment/rain_loop",
            .bus = "environment.weather",
            .base_gain = 1.0F,
            .looping = true,
            .one_shot = false,
        },
        EnvironmentWeatherAudioCueBinding{
            .cue = EnvironmentPrecipitationAudioCueKind::snow_loop,
            .cue_id = "environment.weather.snow.ambience",
            .clip_asset_ref = "audio/environment/snow_ambience",
            .bus = "environment.weather",
            .base_gain = 0.85F,
            .looping = true,
            .one_shot = false,
        },
        EnvironmentWeatherAudioCueBinding{
            .cue = EnvironmentPrecipitationAudioCueKind::indoor_muffling,
            .cue_id = "environment.weather.indoor_muffle",
            .clip_asset_ref = "audio/environment/indoor_muffle",
            .bus = "environment.weather",
            .base_gain = 0.35F,
            .looping = true,
            .one_shot = false,
        },
        EnvironmentWeatherAudioCueBinding{
            .cue = EnvironmentPrecipitationAudioCueKind::thunder_delay,
            .cue_id = "environment.weather.thunder.one_shot",
            .clip_asset_ref = "audio/environment/thunder_one_shot",
            .bus = "environment.weather",
            .base_gain = 1.0F,
            .looping = false,
            .one_shot = true,
        },
        EnvironmentWeatherAudioCueBinding{
            .cue = EnvironmentPrecipitationAudioCueKind::storm_intensity,
            .cue_id = "environment.weather.storm.intensity",
            .clip_asset_ref = "audio/environment/storm_intensity",
            .bus = "environment.weather",
            .base_gain = 0.75F,
            .looping = true,
            .one_shot = false,
        },
        EnvironmentWeatherAudioCueBinding{
            .cue = EnvironmentPrecipitationAudioCueKind::wind_loop,
            .cue_id = "environment.weather.wind.loop",
            .clip_asset_ref = "audio/environment/wind_loop",
            .bus = "environment.weather",
            .base_gain = 0.7F,
            .looping = true,
            .one_shot = false,
        },
        EnvironmentWeatherAudioCueBinding{
            .cue = EnvironmentPrecipitationAudioCueKind::dust_wind,
            .cue_id = "environment.weather.dust.wind",
            .clip_asset_ref = "audio/environment/dust_wind",
            .bus = "environment.weather",
            .base_gain = 0.65F,
            .looping = true,
            .one_shot = false,
        },
        EnvironmentWeatherAudioCueBinding{
            .cue = EnvironmentPrecipitationAudioCueKind::ash_fall,
            .cue_id = "environment.weather.ash.fall",
            .clip_asset_ref = "audio/environment/ash_fall",
            .bus = "environment.weather",
            .base_gain = 0.55F,
            .looping = true,
            .one_shot = false,
        },
    };
}

EnvironmentWeatherAudioPlaybackPlan
plan_environment_weather_audio_playback(const EnvironmentWeatherAudioPlaybackDesc& desc) {
    EnvironmentWeatherAudioPlaybackPlan plan{
        .status = EnvironmentWeatherAudioPlaybackPlanStatus::planned,
    };

    validate_weather_audio_desc(plan, desc);
    if (!plan.succeeded()) {
        plan.status = EnvironmentWeatherAudioPlaybackPlanStatus::blocked;
        return plan;
    }

    append_weather_audio_trigger_rows(plan, desc);
    if (plan.trigger_rows.empty()) {
        add_diagnostic(plan, EnvironmentWeatherAudioDiagnosticCode::missing_handoff_rows, "trigger_rows",
                       "weather audio playback planning requires at least one trigger row");
        plan.status = EnvironmentWeatherAudioPlaybackPlanStatus::blocked;
    }
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

bool has_environment_weather_audio_diagnostic(const EnvironmentWeatherAudioPlaybackPlan& plan,
                                              EnvironmentWeatherAudioDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

bool has_environment_weather_audio_trigger(const EnvironmentWeatherAudioPlaybackPlan& plan,
                                           EnvironmentPrecipitationAudioCueKind cue) noexcept {
    return std::ranges::any_of(plan.trigger_rows, [cue](const auto& row) { return row.cue == cue; });
}

} // namespace mirakana
