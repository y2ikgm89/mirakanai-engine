// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/environment_profile.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool scalar_in_range(float value, float minimum, float maximum) noexcept {
    return finite(value) && value >= minimum && value <= maximum;
}

[[nodiscard]] bool color_valid(Vec3 color) noexcept {
    return scalar_in_range(color.x, 0.0F, 1.0F) && scalar_in_range(color.y, 0.0F, 1.0F) &&
           scalar_in_range(color.z, 0.0F, 1.0F);
}

[[nodiscard]] bool direction_valid(Vec3 direction) noexcept {
    if (!finite(direction.x) || !finite(direction.y) || !finite(direction.z)) {
        return false;
    }
    constexpr float kMinimumDirectionLengthSquared{0.0001F};
    return dot(direction, direction) >= kMinimumDirectionLengthSquared;
}

[[nodiscard]] bool sky_model_valid(EnvironmentSkyModel value) noexcept {
    switch (value) {
    case EnvironmentSkyModel::none:
    case EnvironmentSkyModel::color:
    case EnvironmentSkyModel::gradient:
    case EnvironmentSkyModel::hdri:
    case EnvironmentSkyModel::physical_atmosphere:
        return true;
    }
    return false;
}

[[nodiscard]] bool weather_kind_valid(EnvironmentWeatherKind value) noexcept {
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

[[nodiscard]] bool precipitation_kind_valid(EnvironmentPrecipitationKind value) noexcept {
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

[[nodiscard]] bool quality_preset_valid(EnvironmentQualityPreset value) noexcept {
    switch (value) {
    case EnvironmentQualityPreset::low:
    case EnvironmentQualityPreset::medium:
    case EnvironmentQualityPreset::high:
    case EnvironmentQualityPreset::ultra:
    case EnvironmentQualityPreset::custom:
        return true;
    }
    return false;
}

[[nodiscard]] bool volume_shape_valid(EnvironmentVolumeShape value) noexcept {
    switch (value) {
    case EnvironmentVolumeShape::global:
    case EnvironmentVolumeShape::box:
    case EnvironmentVolumeShape::sphere:
        return true;
    }
    return false;
}

[[nodiscard]] bool volume_blend_mode_valid(EnvironmentVolumeBlendMode value) noexcept {
    switch (value) {
    case EnvironmentVolumeBlendMode::weighted_override:
    case EnvironmentVolumeBlendMode::additive_density:
    case EnvironmentVolumeBlendMode::subtractive_density:
        return true;
    }
    return false;
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    const auto value = static_cast<unsigned char>(ch);
    return (value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')) ||
           (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) ||
           (value >= static_cast<unsigned char>('0') && value <= static_cast<unsigned char>('9'));
}

[[nodiscard]] bool forbidden_profile_token(std::string_view token) noexcept {
    return token == "native" || token == "backend" || token == "handle" || token == "hwnd" || token == "d3d12" ||
           token == "vulkan" || token == "vk" || token == "metal" || token == "mtl" || token == "imgui" ||
           token == "dearimgui" || token == "sdl" || token == "sdl3" || token == "editor";
}

[[nodiscard]] bool has_forbidden_profile_token(std::string_view id) {
    std::string token;
    for (const auto ch : id) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (forbidden_profile_token(token)) {
            return true;
        }
        token.clear();
    }
    return forbidden_profile_token(token);
}

void add_diagnostic(EnvironmentProfileValidationResult& result, EnvironmentProfileDiagnosticCode code,
                    std::uint32_t profile_index, std::string field, std::string id, std::string message) {
    result.diagnostics.push_back(EnvironmentProfileDiagnostic{
        .code = code,
        .profile_index = profile_index,
        .field = std::move(field),
        .id = std::move(id),
        .message = std::move(message),
    });
}

void validate_sun_moon(EnvironmentProfileValidationResult& result, const EnvironmentSunMoonDesc& desc,
                       std::uint32_t profile_index, std::string_view prefix) {
    const std::string prefix_text{prefix};
    if (!direction_valid(desc.direction)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_direction, profile_index,
                       prefix_text + ".direction", {}, "environment sun/moon direction must be finite and non-zero");
    }
    if (!color_valid(desc.color)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_color, profile_index, prefix_text + ".color",
                       {}, "environment sun/moon color must be finite and in [0, 1]");
    }
    if (!finite(desc.illuminance_lux) || desc.illuminance_lux < 0.0F) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index,
                       prefix_text + ".illuminance_lux", {},
                       "environment sun/moon illuminance must be finite and non-negative");
    }
    if (!finite(desc.angular_radius_radians) || desc.angular_radius_radians <= 0.0F) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index,
                       prefix_text + ".angular_radius_radians", {},
                       "environment sun/moon angular radius must be finite and positive");
    }
}

void validate_profile_fields(EnvironmentProfileValidationResult& result, const EnvironmentProfileDesc& desc,
                             std::uint32_t profile_index) {
    if (desc.id.empty()) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::empty_profile_id, profile_index, "id", desc.id,
                       "environment profile id must not be empty");
    } else if (has_forbidden_profile_token(desc.id)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::forbidden_profile_id_token, profile_index, "id",
                       desc.id,
                       "environment profile id must not contain native, backend, editor, or middleware tokens");
    }

    if (!sky_model_valid(desc.sky_model)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_enum, profile_index, "sky_model", desc.id,
                       "environment sky model is not a supported enum value");
    }
    if (!weather_kind_valid(desc.weather)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_enum, profile_index, "weather", desc.id,
                       "environment weather kind is not a supported enum value");
    }
    if (!precipitation_kind_valid(desc.precipitation.kind)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_enum, profile_index, "precipitation.kind",
                       desc.id, "environment precipitation kind is not a supported enum value");
    }

    validate_sun_moon(result, desc.sun, profile_index, "sun");
    validate_sun_moon(result, desc.moon, profile_index, "moon");

    if (!finite(desc.atmosphere.planet_radius_km) || desc.atmosphere.planet_radius_km <= 0.0F) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index,
                       "atmosphere.planet_radius_km", desc.id,
                       "environment atmosphere planet radius must be finite and positive");
    }
    if (!finite(desc.atmosphere.atmosphere_height_km) || desc.atmosphere.atmosphere_height_km <= 0.0F) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index,
                       "atmosphere.atmosphere_height_km", desc.id,
                       "environment atmosphere height must be finite and positive");
    }
    if (!color_valid(desc.atmosphere.rayleigh_scattering) || !color_valid(desc.atmosphere.mie_scattering) ||
        !color_valid(desc.atmosphere.ground_albedo)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_color, profile_index, "atmosphere.color",
                       desc.id, "environment atmosphere scattering and albedo values must be finite and in [0, 1]");
    }

    if (!scalar_in_range(desc.fog.density, 0.0F, 1.0F)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index, "fog.density", desc.id,
                       "environment fog density must be finite and in [0, 1]");
    }
    if (!finite(desc.fog.height_falloff) || desc.fog.height_falloff <= 0.0F) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index, "fog.height_falloff",
                       desc.id, "environment fog height falloff must be finite and positive");
    }
    if (!color_valid(desc.fog.albedo)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_color, profile_index, "fog.albedo", desc.id,
                       "environment fog albedo must be finite and in [0, 1]");
    }
    if (!scalar_in_range(desc.fog.anisotropy, -1.0F, 1.0F)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index, "fog.anisotropy",
                       desc.id, "environment fog anisotropy must be finite and in [-1, 1]");
    }

    if (!scalar_in_range(desc.precipitation.intensity, 0.0F, 1.0F)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index,
                       "precipitation.intensity", desc.id,
                       "environment precipitation intensity must be finite and in [0, 1]");
    }
    if (!finite(desc.precipitation.particle_radius_mm) || desc.precipitation.particle_radius_mm <= 0.0F) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index,
                       "precipitation.particle_radius_mm", desc.id,
                       "environment precipitation particle radius must be finite and positive");
    }
    if (!finite(desc.precipitation.fall_speed_mps) || desc.precipitation.fall_speed_mps < 0.0F) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index,
                       "precipitation.fall_speed_mps", desc.id,
                       "environment precipitation fall speed must be finite and non-negative");
    }
    if (!finite(desc.precipitation.wind_speed_mps)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, profile_index,
                       "precipitation.wind_speed_mps", desc.id, "environment precipitation wind speed must be finite");
    }
}

void validate_volume_fields(EnvironmentProfileValidationResult& result, const EnvironmentVolumeDesc& volume,
                            std::uint32_t volume_index) {
    const auto field_prefix = std::string{"volume."} + std::to_string(volume_index);
    if (volume.id.empty()) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::empty_profile_id, volume_index, field_prefix + ".id",
                       volume.id, "environment volume id must not be empty");
    } else if (has_forbidden_profile_token(volume.id)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::forbidden_profile_id_token, volume_index,
                       field_prefix + ".id", volume.id,
                       "environment volume id must not contain native, backend, editor, or middleware tokens");
    }

    if (!volume_shape_valid(volume.shape)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_volume_shape, volume_index,
                       field_prefix + ".shape", volume.id, "environment volume shape is unsupported");
    }
    if (!volume_blend_mode_valid(volume.blend_mode)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_enum, volume_index,
                       field_prefix + ".blend_mode", volume.id, "environment volume blend mode is unsupported");
    }
    if (!scalar_in_range(volume.blend_weight, 0.0F, 1.0F)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, volume_index,
                       field_prefix + ".blend_weight", volume.id,
                       "environment volume blend weight must be finite and in [0, 1]");
    }
    if (!finite(volume.fade_distance_m) || volume.fade_distance_m < 0.0F) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_volume_fade, volume_index,
                       field_prefix + ".fade_distance_m", volume.id,
                       "environment volume fade distance must be finite and non-negative");
    }
    if (volume.shape == EnvironmentVolumeShape::box &&
        (!finite(volume.extents_m.x) || volume.extents_m.x <= 0.0F || !finite(volume.extents_m.y) ||
         volume.extents_m.y <= 0.0F || !finite(volume.extents_m.z) || volume.extents_m.z <= 0.0F)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, volume_index,
                       field_prefix + ".extents_m", volume.id,
                       "environment box volume extents must be finite and positive");
    }
    if (volume.shape == EnvironmentVolumeShape::sphere && (!finite(volume.radius_m) || volume.radius_m <= 0.0F)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, volume_index,
                       field_prefix + ".radius_m", volume.id,
                       "environment sphere volume radius must be finite and positive");
    }
    if (!finite(volume.center_m.x) || !finite(volume.center_m.y) || !finite(volume.center_m.z)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, volume_index,
                       field_prefix + ".center_m", volume.id, "environment volume center must be finite");
    }
    validate_profile_fields(result, volume.profile, volume_index);
}

void validate_weather_keyframe(EnvironmentProfileValidationResult& result, const EnvironmentWeatherKeyframeDesc& row,
                               std::uint32_t keyframe_index) {
    const auto field_prefix = std::string{"weather_keyframe."} + std::to_string(keyframe_index);
    if (!scalar_in_range(row.time_of_day_hours, 0.0F, 24.0F)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, keyframe_index,
                       field_prefix + ".time_of_day_hours", {},
                       "environment weather keyframe time must be finite and in [0, 24]");
    }
    if (!weather_kind_valid(row.weather) || !precipitation_kind_valid(row.precipitation) ||
        !quality_preset_valid(row.quality_preset)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_enum, keyframe_index, field_prefix, {},
                       "environment weather keyframe enum values must be supported");
    }
    if (!scalar_in_range(row.storm_intensity, 0.0F, 1.0F) || !scalar_in_range(row.cloud_coverage, 0.0F, 1.0F) ||
        !scalar_in_range(row.fog_density, 0.0F, 1.0F)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_scalar, keyframe_index, field_prefix, {},
                       "environment weather keyframe scalar values must be finite and in [0, 1]");
    }
}

void validate_duplicate_ids(EnvironmentProfileValidationResult& result,
                            std::span<const EnvironmentProfileDesc> profiles) {
    for (std::size_t index = 0U; index < profiles.size(); ++index) {
        for (std::size_t previous = 0U; previous < index; ++previous) {
            if (!profiles[index].id.empty() && profiles[index].id == profiles[previous].id) {
                add_diagnostic(result, EnvironmentProfileDiagnosticCode::duplicate_profile_id,
                               static_cast<std::uint32_t>(index), "id", profiles[index].id,
                               "environment profile ids must be unique within a validation batch");
                break;
            }
        }
    }
}

[[nodiscard]] bool profile_fields_valid_noexcept(const EnvironmentProfileDesc& desc) noexcept {
    return !desc.id.empty() && !has_forbidden_profile_token(desc.id) && sky_model_valid(desc.sky_model) &&
           weather_kind_valid(desc.weather) && precipitation_kind_valid(desc.precipitation.kind) &&
           direction_valid(desc.sun.direction) && direction_valid(desc.moon.direction) && color_valid(desc.sun.color) &&
           color_valid(desc.moon.color) && finite(desc.sun.illuminance_lux) && desc.sun.illuminance_lux >= 0.0F &&
           finite(desc.moon.illuminance_lux) && desc.moon.illuminance_lux >= 0.0F &&
           finite(desc.sun.angular_radius_radians) && desc.sun.angular_radius_radians > 0.0F &&
           finite(desc.moon.angular_radius_radians) && desc.moon.angular_radius_radians > 0.0F &&
           finite(desc.atmosphere.planet_radius_km) && desc.atmosphere.planet_radius_km > 0.0F &&
           finite(desc.atmosphere.atmosphere_height_km) && desc.atmosphere.atmosphere_height_km > 0.0F &&
           color_valid(desc.atmosphere.rayleigh_scattering) && color_valid(desc.atmosphere.mie_scattering) &&
           color_valid(desc.atmosphere.ground_albedo) && scalar_in_range(desc.fog.density, 0.0F, 1.0F) &&
           finite(desc.fog.height_falloff) && desc.fog.height_falloff > 0.0F && color_valid(desc.fog.albedo) &&
           scalar_in_range(desc.fog.anisotropy, -1.0F, 1.0F) &&
           scalar_in_range(desc.precipitation.intensity, 0.0F, 1.0F) && finite(desc.precipitation.particle_radius_mm) &&
           desc.precipitation.particle_radius_mm > 0.0F && finite(desc.precipitation.fall_speed_mps) &&
           desc.precipitation.fall_speed_mps >= 0.0F && finite(desc.precipitation.wind_speed_mps);
}

} // namespace

bool EnvironmentProfileValidationResult::succeeded() const noexcept {
    return status == EnvironmentProfileValidationStatus::valid && diagnostics.empty();
}

EnvironmentProfileValidationResult validate_environment_profile_v2(const EnvironmentProfileDocumentV2& desc) {
    EnvironmentProfileValidationResult result{};
    validate_profile_fields(result, desc.global_profile, 0U);
    if (!quality_preset_valid(desc.quality_preset)) {
        add_diagnostic(result, EnvironmentProfileDiagnosticCode::invalid_enum, 0U, "quality.preset", {},
                       "environment profile v2 quality preset is unsupported");
    }
    for (std::size_t index = 0U; index < desc.volumes.size(); ++index) {
        validate_volume_fields(result, desc.volumes[index], static_cast<std::uint32_t>(index));
    }
    for (std::size_t index = 0U; index < desc.weather_timeline.size(); ++index) {
        validate_weather_keyframe(result, desc.weather_timeline[index], static_cast<std::uint32_t>(index));
    }
    result.status = result.diagnostics.empty() ? EnvironmentProfileValidationStatus::valid
                                               : EnvironmentProfileValidationStatus::invalid;
    return result;
}

EnvironmentProfileValidationResult validate_environment_profile(const EnvironmentProfileDesc& desc) {
    const EnvironmentProfileDesc profiles[] = {desc};
    return validate_environment_profiles(profiles);
}

EnvironmentProfileValidationResult validate_environment_profiles(std::span<const EnvironmentProfileDesc> profiles) {
    EnvironmentProfileValidationResult result{};
    for (std::size_t index = 0U; index < profiles.size(); ++index) {
        validate_profile_fields(result, profiles[index], static_cast<std::uint32_t>(index));
    }
    validate_duplicate_ids(result, profiles);
    result.status = result.diagnostics.empty() ? EnvironmentProfileValidationStatus::valid
                                               : EnvironmentProfileValidationStatus::invalid;
    return result;
}

bool is_valid_environment_profile(const EnvironmentProfileDesc& desc) noexcept {
    return profile_fields_valid_noexcept(desc);
}

std::vector<EnvironmentVolumeDesc> sorted_environment_volume_rows(const EnvironmentProfileDocumentV2& desc) {
    auto rows = desc.volumes;
    std::ranges::sort(rows, [](const EnvironmentVolumeDesc& lhs, const EnvironmentVolumeDesc& rhs) {
        if (lhs.priority != rhs.priority) {
            return lhs.priority > rhs.priority;
        }
        return lhs.id < rhs.id;
    });
    return rows;
}

std::uint64_t environment_volume_blend_hash(std::span<const EnvironmentVolumeDesc> volumes) noexcept {
    std::uint64_t hash{1469598103934665603ULL};
    auto mix = [&hash](std::uint64_t value) noexcept {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    auto mix_text = [&mix](std::string_view value) noexcept {
        for (const auto ch : value) {
            mix(static_cast<unsigned char>(ch));
        }
        mix(0xffU);
    };
    auto mix_float = [&mix](float value) noexcept {
        if (!std::isfinite(value)) {
            mix(0U);
            return;
        }
        mix(static_cast<std::uint64_t>(value * 10000.0F));
    };

    for (const auto& volume : volumes) {
        mix_text(volume.id);
        mix(static_cast<std::uint64_t>(volume.shape));
        mix(static_cast<std::uint64_t>(volume.blend_mode));
        mix(static_cast<std::uint64_t>(volume.priority));
        mix_float(volume.blend_weight);
        mix_float(volume.fade_distance_m);
        mix_float(volume.radius_m);
        mix_float(volume.extents_m.x);
        mix_float(volume.extents_m.y);
        mix_float(volume.extents_m.z);
    }
    return hash;
}

bool has_environment_profile_diagnostic(const EnvironmentProfileValidationResult& result,
                                        EnvironmentProfileDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
