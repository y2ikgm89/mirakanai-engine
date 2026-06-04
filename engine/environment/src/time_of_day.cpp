// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/time_of_day.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

constexpr float pi = 3.14159265358979323846F;
constexpr float tau = pi * 2.0F;
constexpr float min_ev100 = -32.0F;
constexpr float max_ev100 = 32.0F;
constexpr std::uint64_t fnv_offset = 1469598103934665603ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;

void add_diagnostic(EnvironmentTimeOfDayPlan& plan, EnvironmentTimeOfDayDiagnosticCode code, std::string field,
                    std::uint32_t profile_index, std::string message) {
    plan.diagnostics.push_back(EnvironmentTimeOfDayDiagnostic{
        .code = code,
        .field = std::move(field),
        .profile_index = profile_index,
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

[[nodiscard]] bool valid_blend_mode(EnvironmentWeatherBlendMode value) noexcept {
    switch (value) {
    case EnvironmentWeatherBlendMode::linear:
    case EnvironmentWeatherBlendMode::smoothstep:
        return true;
    case EnvironmentWeatherBlendMode::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] Vec3 normalize_or_default(Vec3 value, Vec3 fallback) noexcept {
    const float value_length = length(value);
    if (!std::isfinite(value_length) || value_length <= 0.0F) {
        return fallback;
    }
    return value * (1.0F / value_length);
}

[[nodiscard]] Vec3 celestial_direction(float normalized_day_time, float phase_offset) noexcept {
    const float angle = (normalized_day_time + phase_offset) * tau;
    return normalize_or_default(Vec3{.x = std::cos(angle), .y = -std::sin(angle), .z = 0.0F},
                                Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F});
}

[[nodiscard]] float transition_weight(float progress, EnvironmentWeatherBlendMode mode) noexcept {
    const float clamped = std::clamp(progress, 0.0F, 1.0F);
    if (mode == EnvironmentWeatherBlendMode::smoothstep) {
        return clamped * clamped * (3.0F - (2.0F * clamped));
    }
    return clamped;
}

void mix_hash(std::uint64_t& hash, std::uint64_t value) noexcept {
    for (std::uint32_t index = 0; index < 8U; ++index) {
        hash ^= (value >> (index * 8U)) & 0xFFU;
        hash *= fnv_prime;
    }
}

void mix_hash(std::uint64_t& hash, std::string_view value) noexcept {
    for (const char ch : value) {
        hash ^= static_cast<unsigned char>(ch);
        hash *= fnv_prime;
    }
    mix_hash(hash, 0xFFU);
}

void mix_hash(std::uint64_t& hash, float value) noexcept {
    std::uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    mix_hash(hash, static_cast<std::uint64_t>(bits));
}

[[nodiscard]] std::uint64_t compute_replay_hash(const EnvironmentTimeOfDayPlan& plan) noexcept {
    std::uint64_t hash = fnv_offset;

    for (const auto& row : plan.celestial_rows) {
        mix_hash(hash, static_cast<std::uint64_t>(row.body));
        mix_hash(hash, row.direction.x);
        mix_hash(hash, row.direction.y);
        mix_hash(hash, row.direction.z);
        mix_hash(hash, row.illuminance_lux);
        mix_hash(hash, row.moon_phase);
        mix_hash(hash, static_cast<std::uint64_t>(row.atmosphere_light_index));
    }
    for (const auto& row : plan.profile_blend_rows) {
        mix_hash(hash, row.profile_id);
        mix_hash(hash, static_cast<std::uint64_t>(row.weather));
        mix_hash(hash, row.source_weight);
        mix_hash(hash, row.normalized_weight);
    }
    for (const auto& row : plan.weather_transition_rows) {
        mix_hash(hash, static_cast<std::uint64_t>(row.from_weather));
        mix_hash(hash, static_cast<std::uint64_t>(row.to_weather));
        mix_hash(hash, row.duration_seconds);
        mix_hash(hash, row.elapsed_seconds);
        mix_hash(hash, row.normalized_progress);
        mix_hash(hash, row.blend_weight);
        mix_hash(hash, static_cast<std::uint64_t>(row.blend_mode));
    }
    for (const auto& row : plan.exposure_rows) {
        mix_hash(hash, row.fixed_exposure_ev100);
        mix_hash(hash, row.min_exposure_ev100);
        mix_hash(hash, row.max_exposure_ev100);
        mix_hash(hash, row.auto_exposure_enabled ? std::uint64_t{1} : std::uint64_t{0});
        mix_hash(hash, row.adaptation_seconds);
    }

    return hash == 0U ? fnv_prime : hash;
}

void validate_desc(EnvironmentTimeOfDayPlan& plan, const EnvironmentTimeOfDayPlanDesc& desc) {
    if (!finite_in_range(desc.normalized_day_time, 0.0F, 1.0F)) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::invalid_day_time, "normalized_day_time", 0U,
                       "normalized day time must be finite and in [0, 1]");
    }
    if (!finite_in_range(desc.moon_phase, 0.0F, 1.0F)) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::invalid_moon_phase, "moon_phase", 0U,
                       "moon phase must be finite and in [0, 1]");
    }

    float total_weight = 0.0F;
    for (std::uint32_t index = 0U; index < desc.profile_blends.size(); ++index) {
        const auto& layer = desc.profile_blends[index];
        if (!validate_environment_profile(layer.profile).succeeded()) {
            add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::invalid_environment_profile, "profile_blends",
                           index, "time-of-day blending requires valid environment profiles");
        }
        if (!std::isfinite(layer.weight) || layer.weight < 0.0F) {
            add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::invalid_profile_blend_weight,
                           "profile_blends.weight", index, "profile blend weights must be finite and non-negative");
            continue;
        }
        total_weight += layer.weight;
    }
    if (desc.profile_blends.empty() || total_weight <= 0.0F) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::invalid_profile_blend_total_weight, "profile_blends",
                       0U, "time-of-day blending requires at least one profile with positive total weight");
    }

    if (!valid_weather_kind(desc.weather_transition.from_weather) ||
        !valid_weather_kind(desc.weather_transition.to_weather)) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::invalid_environment_profile, "weather_transition", 0U,
                       "weather transitions require supported weather kinds");
    }
    if (!std::isfinite(desc.weather_transition.duration_seconds) || desc.weather_transition.duration_seconds <= 0.0F) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::invalid_transition_duration,
                       "weather_transition.duration_seconds", 0U,
                       "weather transition duration must be finite and positive");
    }
    if (!std::isfinite(desc.weather_transition.elapsed_seconds) || desc.weather_transition.elapsed_seconds < 0.0F ||
        (std::isfinite(desc.weather_transition.duration_seconds) &&
         desc.weather_transition.elapsed_seconds > desc.weather_transition.duration_seconds)) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::invalid_transition_elapsed,
                       "weather_transition.elapsed_seconds", 0U,
                       "weather transition elapsed time must be finite and within the transition duration");
    }
    if (!valid_blend_mode(desc.weather_transition.blend_mode)) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::unsupported_blend_mode,
                       "weather_transition.blend_mode", 0U, "weather transition blend mode is unsupported");
    }

    const auto& exposure = desc.exposure;
    if (!finite_in_range(exposure.fixed_exposure_ev100, min_ev100, max_ev100) ||
        !finite_in_range(exposure.min_exposure_ev100, min_ev100, max_ev100) ||
        !finite_in_range(exposure.max_exposure_ev100, min_ev100, max_ev100) ||
        exposure.min_exposure_ev100 > exposure.max_exposure_ev100 ||
        (exposure.auto_exposure_enabled &&
         (!std::isfinite(exposure.adaptation_seconds) || exposure.adaptation_seconds <= 0.0F))) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::invalid_exposure, "exposure", 0U,
                       "exposure intent requires bounded EV100 values, ordered limits, and positive adaptation time");
    }

    if (desc.request_backend_execution) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::unsupported_backend_execution,
                       "request_backend_execution", 0U,
                       "time-of-day planning must not invoke renderer, RHI, or audio backends");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::unsupported_native_handle_claim,
                       "request_native_handle_access", 0U,
                       "time-of-day planning must not expose native renderer, platform, or RHI handles");
    }
    if (desc.request_profile_mutation) {
        add_diagnostic(plan, EnvironmentTimeOfDayDiagnosticCode::unsupported_profile_mutation,
                       "request_profile_mutation", 0U,
                       "time-of-day planning must not mutate source environment profiles");
    }
}

void append_rows(EnvironmentTimeOfDayPlan& plan, const EnvironmentTimeOfDayPlanDesc& desc) {
    const auto& profile = desc.profile_blends.front().profile;
    plan.celestial_rows.push_back(EnvironmentCelestialRow{
        .body = EnvironmentCelestialBodyKind::sun,
        .direction = celestial_direction(desc.normalized_day_time, 0.0F),
        .color = profile.sun.color,
        .illuminance_lux = profile.sun.illuminance_lux,
        .angular_radius_radians = profile.sun.angular_radius_radians,
        .moon_phase = 0.0F,
        .atmosphere_light_index = 0U,
        .visible_disk = profile.sun.visible_disk,
        .affects_atmosphere = profile.sun.affects_atmosphere,
        .affects_clouds = profile.sun.affects_clouds,
        .casts_environment_shadows = profile.sun.casts_environment_shadows,
    });
    plan.celestial_rows.push_back(EnvironmentCelestialRow{
        .body = EnvironmentCelestialBodyKind::moon,
        .direction = celestial_direction(desc.normalized_day_time, 0.5F),
        .color = profile.moon.color,
        .illuminance_lux = profile.moon.illuminance_lux,
        .angular_radius_radians = profile.moon.angular_radius_radians,
        .moon_phase = desc.moon_phase,
        .atmosphere_light_index = 1U,
        .visible_disk = profile.moon.visible_disk,
        .affects_atmosphere = profile.moon.affects_atmosphere,
        .affects_clouds = profile.moon.affects_clouds,
        .casts_environment_shadows = profile.moon.casts_environment_shadows,
    });

    float total_weight = 0.0F;
    for (const auto& layer : desc.profile_blends) {
        total_weight += layer.weight;
    }
    for (const auto& layer : desc.profile_blends) {
        plan.profile_blend_rows.push_back(EnvironmentProfileBlendRow{
            .profile_id = layer.profile.id,
            .weather = layer.profile.weather,
            .source_weight = layer.weight,
            .normalized_weight = layer.weight / total_weight,
        });
    }

    const float normalized_progress =
        std::clamp(desc.weather_transition.elapsed_seconds / desc.weather_transition.duration_seconds, 0.0F, 1.0F);
    plan.weather_transition_rows.push_back(EnvironmentWeatherTransitionRow{
        .from_weather = desc.weather_transition.from_weather,
        .to_weather = desc.weather_transition.to_weather,
        .duration_seconds = desc.weather_transition.duration_seconds,
        .elapsed_seconds = desc.weather_transition.elapsed_seconds,
        .normalized_progress = normalized_progress,
        .blend_weight = transition_weight(normalized_progress, desc.weather_transition.blend_mode),
        .blend_mode = desc.weather_transition.blend_mode,
    });

    plan.exposure_rows.push_back(EnvironmentExposureIntentRow{
        .fixed_exposure_ev100 = desc.exposure.fixed_exposure_ev100,
        .min_exposure_ev100 = desc.exposure.min_exposure_ev100,
        .max_exposure_ev100 = desc.exposure.max_exposure_ev100,
        .auto_exposure_enabled = desc.exposure.auto_exposure_enabled,
        .adaptation_seconds = desc.exposure.adaptation_seconds,
    });

    plan.replay_hash = compute_replay_hash(plan);
}

} // namespace

bool EnvironmentTimeOfDayPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

EnvironmentTimeOfDayPlan plan_environment_time_of_day(const EnvironmentTimeOfDayPlanDesc& desc) {
    EnvironmentTimeOfDayPlan plan{
        .status = EnvironmentTimeOfDayPlanStatus::planned,
    };

    validate_desc(plan, desc);

    if (!plan.succeeded()) {
        plan.status = EnvironmentTimeOfDayPlanStatus::blocked;
        return plan;
    }

    append_rows(plan, desc);
    return plan;
}

bool has_environment_time_of_day_diagnostic(const EnvironmentTimeOfDayPlan& plan,
                                            EnvironmentTimeOfDayDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
