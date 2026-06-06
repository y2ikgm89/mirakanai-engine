// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/environment/environment_profile.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class EnvironmentPrecipitationPlanStatus : std::uint8_t {
    blocked = 0,
    planned,
};

enum class EnvironmentPrecipitationDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_environment_profile,
    unsupported_weather_kind,
    unsupported_precipitation_kind,
    invalid_intensity,
    invalid_particle_radius,
    invalid_fall_speed,
    invalid_wind_speed,
    missing_occlusion_policy,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
    unsupported_material_mutation,
    unsupported_audio_playback,
};

enum class EnvironmentPrecipitationAudioCueKind : std::uint8_t {
    unknown = 0,
    rain_loop,
    snow_loop,
    indoor_muffling,
    thunder_delay,
    storm_intensity,
    wind_loop,
    dust_wind,
    ash_fall,
};

enum class EnvironmentWeatherAudioPlaybackPlanStatus : std::uint8_t {
    blocked = 0,
    planned,
};

enum class EnvironmentWeatherAudioDiagnosticCode : std::uint8_t {
    none = 0,
    missing_handoff_rows,
    unsupported_audio_cue,
    missing_cue_binding,
    duplicate_cue_binding,
    invalid_cue_binding,
    invalid_handoff_intensity,
    unsupported_runtime_backend_execution,
    unsupported_audio_device_ownership,
    unsupported_native_handle_access,
};

struct EnvironmentPrecipitationPlanDesc {
    EnvironmentProfileDesc environment;
    bool scene_geometry_occlusion_required{false};
    bool occlusion_policy_available{false};
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
    bool request_material_mutation{false};
    bool request_audio_playback{false};
};

struct EnvironmentWeatherRow {
    EnvironmentWeatherKind weather{EnvironmentWeatherKind::clear};
    EnvironmentPrecipitationKind precipitation_kind{EnvironmentPrecipitationKind::none};
    float intensity{0.0F};
    bool precipitation_enabled{false};
    bool wet_surface_enabled{false};
    bool audio_handoff_enabled{false};
};

struct EnvironmentPrecipitationParticleRow {
    EnvironmentPrecipitationKind kind{EnvironmentPrecipitationKind::none};
    float intensity{0.0F};
    float spawn_rate_per_second{0.0F};
    float particle_radius_mm{0.0F};
    float fall_speed_mps{0.0F};
    float wind_speed_mps{0.0F};
    bool camera_near_only{true};
    bool gpu_particle_intent{true};
};

struct EnvironmentSurfaceWetnessRow {
    bool enabled{false};
    float intensity{0.0F};
    bool splash_intent{false};
    bool ripple_intent{false};
    bool mutates_materials{false};
};

struct EnvironmentPrecipitationOcclusionRow {
    bool required{false};
    bool available{false};
    bool uses_scene_geometry_depth_mask{false};
    bool uses_indoor_volume_mask{false};
};

struct EnvironmentPrecipitationAudioHandoffRow {
    EnvironmentPrecipitationAudioCueKind cue{EnvironmentPrecipitationAudioCueKind::unknown};
    float intensity{0.0F};
    float delay_seconds{0.0F};
    bool handoff_only{true};
};

struct EnvironmentWeatherAudioCueBinding {
    EnvironmentPrecipitationAudioCueKind cue{EnvironmentPrecipitationAudioCueKind::unknown};
    std::string cue_id;
    std::string clip_asset_ref;
    std::string bus{"environment.weather"};
    float base_gain{1.0F};
    bool looping{true};
    bool one_shot{false};
};

struct EnvironmentWeatherAudioPlaybackDesc {
    std::vector<EnvironmentPrecipitationAudioHandoffRow> handoff_rows;
    std::vector<EnvironmentWeatherAudioCueBinding> cue_bindings;
    bool request_runtime_backend_execution{false};
    bool request_environment_audio_device_ownership{false};
    bool request_native_handle_access{false};
};

struct EnvironmentWeatherAudioTriggerRow {
    EnvironmentPrecipitationAudioCueKind cue{EnvironmentPrecipitationAudioCueKind::unknown};
    std::string cue_id;
    std::string clip_asset_ref;
    std::string bus{"environment.weather"};
    float gain{1.0F};
    float delay_seconds{0.0F};
    bool looping{true};
    bool one_shot{false};
    bool device_owned_by_environment{false};
    bool native_handle_access{false};
};

struct EnvironmentWeatherAudioDiagnostic {
    EnvironmentWeatherAudioDiagnosticCode code{EnvironmentWeatherAudioDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct EnvironmentWeatherAudioPlaybackPlan {
    EnvironmentWeatherAudioPlaybackPlanStatus status{EnvironmentWeatherAudioPlaybackPlanStatus::blocked};
    bool owns_audio_device{false};
    bool exposes_native_handles{false};
    bool invokes_backend{false};
    std::vector<EnvironmentWeatherAudioTriggerRow> trigger_rows;
    std::vector<EnvironmentWeatherAudioDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct EnvironmentPrecipitationDiagnostic {
    EnvironmentPrecipitationDiagnosticCode code{EnvironmentPrecipitationDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct EnvironmentPrecipitationPlan {
    EnvironmentPrecipitationPlanStatus status{EnvironmentPrecipitationPlanStatus::blocked};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool mutates_materials{false};
    bool plays_audio{false};
    std::vector<EnvironmentWeatherRow> weather_rows;
    std::vector<EnvironmentPrecipitationParticleRow> particle_rows;
    std::vector<EnvironmentSurfaceWetnessRow> wetness_rows;
    std::vector<EnvironmentPrecipitationOcclusionRow> occlusion_rows;
    std::vector<EnvironmentPrecipitationAudioHandoffRow> audio_handoff_rows;
    std::vector<EnvironmentPrecipitationDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] EnvironmentPrecipitationPlan plan_environment_precipitation(const EnvironmentPrecipitationPlanDesc& desc);

[[nodiscard]] std::vector<EnvironmentWeatherAudioCueBinding> default_environment_weather_audio_cue_bindings();

[[nodiscard]] EnvironmentWeatherAudioPlaybackPlan
plan_environment_weather_audio_playback(const EnvironmentWeatherAudioPlaybackDesc& desc);

[[nodiscard]] bool has_environment_precipitation_diagnostic(const EnvironmentPrecipitationPlan& plan,
                                                            EnvironmentPrecipitationDiagnosticCode code) noexcept;

[[nodiscard]] bool has_environment_precipitation_audio_cue(const EnvironmentPrecipitationPlan& plan,
                                                           EnvironmentPrecipitationAudioCueKind cue) noexcept;

[[nodiscard]] bool has_environment_weather_audio_diagnostic(const EnvironmentWeatherAudioPlaybackPlan& plan,
                                                            EnvironmentWeatherAudioDiagnosticCode code) noexcept;

[[nodiscard]] bool has_environment_weather_audio_trigger(const EnvironmentWeatherAudioPlaybackPlan& plan,
                                                         EnvironmentPrecipitationAudioCueKind cue) noexcept;

} // namespace mirakana
