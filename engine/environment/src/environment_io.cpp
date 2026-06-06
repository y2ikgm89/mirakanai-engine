// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/environment_io.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <locale>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mirakana {
namespace {

constexpr std::string_view environment_profile_format = "GameEngine.EnvironmentProfile.v1";
constexpr std::string_view environment_profile_format_v2_value = "GameEngine.EnvironmentProfile.v2";

using KeyValues = std::unordered_map<std::string, std::string>;

[[nodiscard]] bool is_allowed_key(std::string_view key) noexcept {
    for (const std::string_view allowed : {
             "format",
             "id",
             "sky.model",
             "weather.kind",
             "sun.direction",
             "sun.color",
             "sun.illuminance_lux",
             "sun.angular_radius_radians",
             "sun.visible_disk",
             "sun.affects_atmosphere",
             "sun.affects_clouds",
             "sun.casts_environment_shadows",
             "moon.direction",
             "moon.color",
             "moon.illuminance_lux",
             "moon.angular_radius_radians",
             "moon.visible_disk",
             "moon.affects_atmosphere",
             "moon.affects_clouds",
             "moon.casts_environment_shadows",
             "atmosphere.planet_radius_km",
             "atmosphere.atmosphere_height_km",
             "atmosphere.rayleigh_scattering",
             "atmosphere.mie_scattering",
             "atmosphere.ground_albedo",
             "fog.enabled",
             "fog.density",
             "fog.height_falloff",
             "fog.albedo",
             "fog.anisotropy",
             "precipitation.kind",
             "precipitation.intensity",
             "precipitation.particle_radius_mm",
             "precipitation.fall_speed_mps",
             "precipitation.wind_speed_mps",
         }) {
        if (key == allowed) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] KeyValues parse_key_values(std::string_view text) {
    KeyValues values;
    std::size_t begin = 0;
    while (begin < text.size()) {
        const auto end = text.find('\n', begin);
        const auto line = text.substr(begin, end == std::string_view::npos ? text.size() - begin : end - begin);
        begin = end == std::string_view::npos ? text.size() : end + 1U;
        if (line.empty()) {
            continue;
        }
        if (line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument("environment profile line contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos || separator == 0U) {
            throw std::invalid_argument("environment profile line is missing '='");
        }
        const auto key = line.substr(0, separator);
        if (!is_allowed_key(key)) {
            throw std::invalid_argument("environment profile key is unsupported");
        }
        auto [_, inserted] = values.emplace(std::string{key}, std::string{line.substr(separator + 1U)});
        if (!inserted) {
            throw std::invalid_argument("environment profile contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] KeyValues parse_key_values_unrestricted(std::string_view text) {
    KeyValues values;
    std::size_t begin = 0;
    while (begin < text.size()) {
        const auto end = text.find('\n', begin);
        const auto line = text.substr(begin, end == std::string_view::npos ? text.size() - begin : end - begin);
        begin = end == std::string_view::npos ? text.size() : end + 1U;
        if (line.empty()) {
            continue;
        }
        if (line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument("environment profile line contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos || separator == 0U) {
            throw std::invalid_argument("environment profile line is missing '='");
        }
        const auto key = line.substr(0, separator);
        auto [_, inserted] = values.emplace(std::string{key}, std::string{line.substr(separator + 1U)});
        if (!inserted) {
            throw std::invalid_argument("environment profile contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const KeyValues& values, const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument("environment profile is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] const std::string* optional_value(const KeyValues& values, const std::string& key) noexcept {
    const auto it = values.find(key);
    return it == values.end() ? nullptr : &it->second;
}

[[nodiscard]] bool environment_float_character(char value) noexcept {
    return (value >= '0' && value <= '9') || value == '+' || value == '-' || value == '.' || value == 'e' ||
           value == 'E';
}

[[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix) noexcept {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] float parse_float(std::string_view value, std::string_view key) {
    if (value.empty()) {
        throw std::invalid_argument("environment profile float value is invalid: " + std::string{key});
    }
    for (const char character : value) {
        if (!environment_float_character(character)) {
            throw std::invalid_argument("environment profile float value is invalid: " + std::string{key});
        }
    }

    float parsed = 0.0F;
    std::istringstream stream{std::string{value}};
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> parsed;

    char trailing = '\0';
    if (!stream || (stream >> trailing) || !std::isfinite(parsed)) {
        throw std::invalid_argument("environment profile float value is invalid: " + std::string{key});
    }
    return parsed;
}

[[nodiscard]] std::int32_t parse_i32(std::string_view value, std::string_view key) {
    std::int32_t parsed{0};
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument("environment profile integer value is invalid: " + std::string{key});
    }
    return parsed;
}

[[nodiscard]] std::size_t parse_ordinal(std::string_view value, std::string_view key) {
    std::size_t parsed{0U};
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument("environment profile ordinal value is invalid: " + std::string{key});
    }
    return parsed;
}

[[nodiscard]] bool parse_bool(std::string_view value, std::string_view key) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument("environment profile bool value is invalid: " + std::string{key});
}

[[nodiscard]] Vec3 parse_vec3(std::string_view value, std::string_view key) {
    const auto first_separator = value.find(',');
    const auto second_separator =
        first_separator == std::string_view::npos ? std::string_view::npos : value.find(',', first_separator + 1U);
    if (first_separator == std::string_view::npos || second_separator == std::string_view::npos ||
        value.find(',', second_separator + 1U) != std::string_view::npos) {
        throw std::invalid_argument("environment profile vec3 value is invalid: " + std::string{key});
    }
    return Vec3{
        .x = parse_float(value.substr(0, first_separator), key),
        .y = parse_float(value.substr(first_separator + 1U, second_separator - first_separator - 1U), key),
        .z = parse_float(value.substr(second_separator + 1U), key),
    };
}

void read_optional_float(const KeyValues& values, const std::string& key, float& target) {
    if (const auto* value = optional_value(values, key); value != nullptr) {
        target = parse_float(*value, key);
    }
}

void read_optional_bool(const KeyValues& values, const std::string& key, bool& target) {
    if (const auto* value = optional_value(values, key); value != nullptr) {
        target = parse_bool(*value, key);
    }
}

void read_optional_vec3(const KeyValues& values, const std::string& key, Vec3& target) {
    if (const auto* value = optional_value(values, key); value != nullptr) {
        target = parse_vec3(*value, key);
    }
}

void read_sun_moon(const KeyValues& values, std::string_view prefix, EnvironmentSunMoonDesc& desc) {
    const std::string key_prefix{prefix};
    read_optional_vec3(values, key_prefix + ".direction", desc.direction);
    read_optional_vec3(values, key_prefix + ".color", desc.color);
    read_optional_float(values, key_prefix + ".illuminance_lux", desc.illuminance_lux);
    read_optional_float(values, key_prefix + ".angular_radius_radians", desc.angular_radius_radians);
    read_optional_bool(values, key_prefix + ".visible_disk", desc.visible_disk);
    read_optional_bool(values, key_prefix + ".affects_atmosphere", desc.affects_atmosphere);
    read_optional_bool(values, key_prefix + ".affects_clouds", desc.affects_clouds);
    read_optional_bool(values, key_prefix + ".casts_environment_shadows", desc.casts_environment_shadows);
}

[[nodiscard]] const char* bool_text(bool value) noexcept {
    return value ? "true" : "false";
}

void write_vec3(std::ostream& output, Vec3 value) {
    output << value.x << ',' << value.y << ',' << value.z;
}

void write_sun_moon(std::ostream& output, const EnvironmentSunMoonDesc& desc, std::string_view prefix,
                    std::string_view key_prefix) {
    output << key_prefix << prefix << ".direction=";
    write_vec3(output, desc.direction);
    output << '\n';
    output << key_prefix << prefix << ".color=";
    write_vec3(output, desc.color);
    output << '\n';
    output << key_prefix << prefix << ".illuminance_lux=" << desc.illuminance_lux << '\n';
    output << key_prefix << prefix << ".angular_radius_radians=" << desc.angular_radius_radians << '\n';
    output << key_prefix << prefix << ".visible_disk=" << bool_text(desc.visible_disk) << '\n';
    output << key_prefix << prefix << ".affects_atmosphere=" << bool_text(desc.affects_atmosphere) << '\n';
    output << key_prefix << prefix << ".affects_clouds=" << bool_text(desc.affects_clouds) << '\n';
    output << key_prefix << prefix << ".casts_environment_shadows=" << bool_text(desc.casts_environment_shadows)
           << '\n';
}

} // namespace

std::string_view environment_profile_format_v1() noexcept {
    return environment_profile_format;
}

std::string_view environment_profile_format_v2() noexcept {
    return environment_profile_format_v2_value;
}

std::string_view environment_sky_model_name(EnvironmentSkyModel value) noexcept {
    switch (value) {
    case EnvironmentSkyModel::none:
        return "none";
    case EnvironmentSkyModel::color:
        return "color";
    case EnvironmentSkyModel::gradient:
        return "gradient";
    case EnvironmentSkyModel::hdri:
        return "hdri";
    case EnvironmentSkyModel::physical_atmosphere:
        return "physical_atmosphere";
    }
    return "unknown";
}

EnvironmentSkyModel parse_environment_sky_model(std::string_view value) {
    if (value == "none") {
        return EnvironmentSkyModel::none;
    }
    if (value == "color") {
        return EnvironmentSkyModel::color;
    }
    if (value == "gradient") {
        return EnvironmentSkyModel::gradient;
    }
    if (value == "hdri") {
        return EnvironmentSkyModel::hdri;
    }
    if (value == "physical_atmosphere") {
        return EnvironmentSkyModel::physical_atmosphere;
    }
    throw std::invalid_argument("environment sky model is unsupported");
}

std::string_view environment_weather_kind_name(EnvironmentWeatherKind value) noexcept {
    switch (value) {
    case EnvironmentWeatherKind::clear:
        return "clear";
    case EnvironmentWeatherKind::cloudy:
        return "cloudy";
    case EnvironmentWeatherKind::rain:
        return "rain";
    case EnvironmentWeatherKind::storm:
        return "storm";
    case EnvironmentWeatherKind::snow:
        return "snow";
    case EnvironmentWeatherKind::foggy:
        return "foggy";
    case EnvironmentWeatherKind::dust:
        return "dust";
    case EnvironmentWeatherKind::ash:
        return "ash";
    }
    return "unknown";
}

EnvironmentWeatherKind parse_environment_weather_kind(std::string_view value) {
    if (value == "clear") {
        return EnvironmentWeatherKind::clear;
    }
    if (value == "cloudy") {
        return EnvironmentWeatherKind::cloudy;
    }
    if (value == "rain") {
        return EnvironmentWeatherKind::rain;
    }
    if (value == "storm") {
        return EnvironmentWeatherKind::storm;
    }
    if (value == "snow") {
        return EnvironmentWeatherKind::snow;
    }
    if (value == "foggy") {
        return EnvironmentWeatherKind::foggy;
    }
    if (value == "dust") {
        return EnvironmentWeatherKind::dust;
    }
    if (value == "ash") {
        return EnvironmentWeatherKind::ash;
    }
    throw std::invalid_argument("environment weather kind is unsupported");
}

std::string_view environment_precipitation_kind_name(EnvironmentPrecipitationKind value) noexcept {
    switch (value) {
    case EnvironmentPrecipitationKind::none:
        return "none";
    case EnvironmentPrecipitationKind::rain:
        return "rain";
    case EnvironmentPrecipitationKind::snow:
        return "snow";
    case EnvironmentPrecipitationKind::sleet:
        return "sleet";
    case EnvironmentPrecipitationKind::hail:
        return "hail";
    case EnvironmentPrecipitationKind::ash:
        return "ash";
    case EnvironmentPrecipitationKind::dust:
        return "dust";
    }
    return "unknown";
}

EnvironmentPrecipitationKind parse_environment_precipitation_kind(std::string_view value) {
    if (value == "none") {
        return EnvironmentPrecipitationKind::none;
    }
    if (value == "rain") {
        return EnvironmentPrecipitationKind::rain;
    }
    if (value == "snow") {
        return EnvironmentPrecipitationKind::snow;
    }
    if (value == "sleet") {
        return EnvironmentPrecipitationKind::sleet;
    }
    if (value == "hail") {
        return EnvironmentPrecipitationKind::hail;
    }
    if (value == "ash") {
        return EnvironmentPrecipitationKind::ash;
    }
    if (value == "dust") {
        return EnvironmentPrecipitationKind::dust;
    }
    throw std::invalid_argument("environment precipitation kind is unsupported");
}

std::string_view environment_quality_preset_name(EnvironmentQualityPreset value) noexcept {
    switch (value) {
    case EnvironmentQualityPreset::low:
        return "low";
    case EnvironmentQualityPreset::medium:
        return "medium";
    case EnvironmentQualityPreset::high:
        return "high";
    case EnvironmentQualityPreset::ultra:
        return "ultra";
    case EnvironmentQualityPreset::custom:
        return "custom";
    }
    return "unknown";
}

EnvironmentQualityPreset parse_environment_quality_preset(std::string_view value) {
    if (value == "low") {
        return EnvironmentQualityPreset::low;
    }
    if (value == "medium") {
        return EnvironmentQualityPreset::medium;
    }
    if (value == "high") {
        return EnvironmentQualityPreset::high;
    }
    if (value == "ultra") {
        return EnvironmentQualityPreset::ultra;
    }
    if (value == "custom") {
        return EnvironmentQualityPreset::custom;
    }
    throw std::invalid_argument("environment quality preset is unsupported");
}

std::string_view environment_volume_shape_name(EnvironmentVolumeShape value) noexcept {
    switch (value) {
    case EnvironmentVolumeShape::global:
        return "global";
    case EnvironmentVolumeShape::box:
        return "box";
    case EnvironmentVolumeShape::sphere:
        return "sphere";
    }
    return "unknown";
}

EnvironmentVolumeShape parse_environment_volume_shape(std::string_view value) {
    if (value == "global") {
        return EnvironmentVolumeShape::global;
    }
    if (value == "box") {
        return EnvironmentVolumeShape::box;
    }
    if (value == "sphere") {
        return EnvironmentVolumeShape::sphere;
    }
    throw std::invalid_argument("environment volume shape is unsupported");
}

std::string_view environment_volume_blend_mode_name(EnvironmentVolumeBlendMode value) noexcept {
    switch (value) {
    case EnvironmentVolumeBlendMode::weighted_override:
        return "weighted_override";
    case EnvironmentVolumeBlendMode::additive_density:
        return "additive_density";
    case EnvironmentVolumeBlendMode::subtractive_density:
        return "subtractive_density";
    }
    return "unknown";
}

EnvironmentVolumeBlendMode parse_environment_volume_blend_mode(std::string_view value) {
    if (value == "weighted_override") {
        return EnvironmentVolumeBlendMode::weighted_override;
    }
    if (value == "additive_density") {
        return EnvironmentVolumeBlendMode::additive_density;
    }
    if (value == "subtractive_density") {
        return EnvironmentVolumeBlendMode::subtractive_density;
    }
    throw std::invalid_argument("environment volume blend mode is unsupported");
}

void write_environment_profile_payload(std::ostream& output, const EnvironmentProfileDesc& desc,
                                       std::string_view key_prefix) {
    const auto validation = validate_environment_profile(desc);
    if (!validation.succeeded()) {
        throw std::invalid_argument("environment profile document is invalid");
    }

    output << key_prefix << "id=" << desc.id << '\n';
    output << key_prefix << "sky.model=" << environment_sky_model_name(desc.sky_model) << '\n';
    output << key_prefix << "weather.kind=" << environment_weather_kind_name(desc.weather) << '\n';
    write_sun_moon(output, desc.sun, "sun", key_prefix);
    write_sun_moon(output, desc.moon, "moon", key_prefix);
    output << key_prefix << "atmosphere.planet_radius_km=" << desc.atmosphere.planet_radius_km << '\n';
    output << key_prefix << "atmosphere.atmosphere_height_km=" << desc.atmosphere.atmosphere_height_km << '\n';
    output << key_prefix << "atmosphere.rayleigh_scattering=";
    write_vec3(output, desc.atmosphere.rayleigh_scattering);
    output << '\n';
    output << key_prefix << "atmosphere.mie_scattering=";
    write_vec3(output, desc.atmosphere.mie_scattering);
    output << '\n';
    output << key_prefix << "atmosphere.ground_albedo=";
    write_vec3(output, desc.atmosphere.ground_albedo);
    output << '\n';
    output << key_prefix << "fog.enabled=" << bool_text(desc.fog.enabled) << '\n';
    output << key_prefix << "fog.density=" << desc.fog.density << '\n';
    output << key_prefix << "fog.height_falloff=" << desc.fog.height_falloff << '\n';
    output << key_prefix << "fog.albedo=";
    write_vec3(output, desc.fog.albedo);
    output << '\n';
    output << key_prefix << "fog.anisotropy=" << desc.fog.anisotropy << '\n';
    output << key_prefix << "precipitation.kind=" << environment_precipitation_kind_name(desc.precipitation.kind)
           << '\n';
    output << key_prefix << "precipitation.intensity=" << desc.precipitation.intensity << '\n';
    output << key_prefix << "precipitation.particle_radius_mm=" << desc.precipitation.particle_radius_mm << '\n';
    output << key_prefix << "precipitation.fall_speed_mps=" << desc.precipitation.fall_speed_mps << '\n';
    output << key_prefix << "precipitation.wind_speed_mps=" << desc.precipitation.wind_speed_mps << '\n';
}

void write_environment_profile_v2_payload(std::ostream& output, const EnvironmentProfileDocumentV2& desc,
                                          std::string_view key_prefix) {
    const auto validation = validate_environment_profile_v2(desc);
    if (!validation.succeeded()) {
        throw std::invalid_argument("environment profile v2 document is invalid");
    }

    write_environment_profile_payload(output, desc.global_profile, std::string{key_prefix} + "global.");
    output << key_prefix << "quality.preset=" << environment_quality_preset_name(desc.quality_preset) << '\n';
    for (std::size_t index = 0U; index < desc.volumes.size(); ++index) {
        const auto& volume = desc.volumes[index];
        const auto prefix = std::string{key_prefix} + "volume." + std::to_string(index) + ".";
        output << prefix << "id=" << volume.id << '\n';
        output << prefix << "shape=" << environment_volume_shape_name(volume.shape) << '\n';
        output << prefix << "center_m=";
        write_vec3(output, volume.center_m);
        output << '\n';
        output << prefix << "extents_m=";
        write_vec3(output, volume.extents_m);
        output << '\n';
        output << prefix << "radius_m=" << volume.radius_m << '\n';
        output << prefix << "priority=" << volume.priority << '\n';
        output << prefix << "blend_weight=" << volume.blend_weight << '\n';
        output << prefix << "fade_distance_m=" << volume.fade_distance_m << '\n';
        output << prefix << "blend_mode=" << environment_volume_blend_mode_name(volume.blend_mode) << '\n';
        write_environment_profile_payload(output, volume.profile, prefix + "profile.");
    }
    for (std::size_t index = 0U; index < desc.weather_timeline.size(); ++index) {
        const auto& keyframe = desc.weather_timeline[index];
        const auto prefix = std::string{key_prefix} + "weather_keyframe." + std::to_string(index) + ".";
        output << prefix << "time_of_day_hours=" << keyframe.time_of_day_hours << '\n';
        output << prefix << "weather=" << environment_weather_kind_name(keyframe.weather) << '\n';
        output << prefix << "precipitation=" << environment_precipitation_kind_name(keyframe.precipitation) << '\n';
        output << prefix << "storm_intensity=" << keyframe.storm_intensity << '\n';
        output << prefix << "cloud_coverage=" << keyframe.cloud_coverage << '\n';
        output << prefix << "fog_density=" << keyframe.fog_density << '\n';
        output << prefix << "quality_preset=" << environment_quality_preset_name(keyframe.quality_preset) << '\n';
    }
}

std::string serialize_environment_profile(const EnvironmentProfileDesc& desc) {
    std::ostringstream output;
    output << "format=" << environment_profile_format << '\n';
    write_environment_profile_payload(output, desc);
    return output.str();
}

std::string serialize_environment_profile_v2(const EnvironmentProfileDocumentV2& desc) {
    std::ostringstream output;
    output << "format=" << environment_profile_format_v2_value << '\n';
    write_environment_profile_v2_payload(output, desc);
    return output.str();
}

EnvironmentProfileDesc deserialize_environment_profile(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != environment_profile_format) {
        throw std::invalid_argument("environment profile format is unsupported");
    }

    EnvironmentProfileDesc desc{};
    desc.id = required_value(values, "id");
    if (const auto* value = optional_value(values, "sky.model"); value != nullptr) {
        desc.sky_model = parse_environment_sky_model(*value);
    }
    if (const auto* value = optional_value(values, "weather.kind"); value != nullptr) {
        desc.weather = parse_environment_weather_kind(*value);
    }
    read_sun_moon(values, "sun", desc.sun);
    read_sun_moon(values, "moon", desc.moon);
    read_optional_float(values, "atmosphere.planet_radius_km", desc.atmosphere.planet_radius_km);
    read_optional_float(values, "atmosphere.atmosphere_height_km", desc.atmosphere.atmosphere_height_km);
    read_optional_vec3(values, "atmosphere.rayleigh_scattering", desc.atmosphere.rayleigh_scattering);
    read_optional_vec3(values, "atmosphere.mie_scattering", desc.atmosphere.mie_scattering);
    read_optional_vec3(values, "atmosphere.ground_albedo", desc.atmosphere.ground_albedo);
    read_optional_bool(values, "fog.enabled", desc.fog.enabled);
    read_optional_float(values, "fog.density", desc.fog.density);
    read_optional_float(values, "fog.height_falloff", desc.fog.height_falloff);
    read_optional_vec3(values, "fog.albedo", desc.fog.albedo);
    read_optional_float(values, "fog.anisotropy", desc.fog.anisotropy);
    if (const auto* value = optional_value(values, "precipitation.kind"); value != nullptr) {
        desc.precipitation.kind = parse_environment_precipitation_kind(*value);
    }
    read_optional_float(values, "precipitation.intensity", desc.precipitation.intensity);
    read_optional_float(values, "precipitation.particle_radius_mm", desc.precipitation.particle_radius_mm);
    read_optional_float(values, "precipitation.fall_speed_mps", desc.precipitation.fall_speed_mps);
    read_optional_float(values, "precipitation.wind_speed_mps", desc.precipitation.wind_speed_mps);

    if (!validate_environment_profile(desc).succeeded()) {
        throw std::invalid_argument("environment profile document is invalid");
    }
    return desc;
}

EnvironmentProfileDocumentV2 deserialize_environment_profile_v2(std::string_view text) {
    struct VolumeTextRow {
        EnvironmentVolumeDesc desc{};
        std::ostringstream profile_payload;
        bool has_profile_payload{false};
    };
    struct WeatherKeyframeTextRow {
        EnvironmentWeatherKeyframeDesc desc{};
    };

    const auto values = parse_key_values_unrestricted(text);
    if (required_value(values, "format") != environment_profile_format_v2_value) {
        throw std::invalid_argument("environment profile v2 format is unsupported");
    }

    EnvironmentProfileDocumentV2 document{};
    std::ostringstream global_payload;
    global_payload << "format=" << environment_profile_format << '\n';
    bool has_global_payload{false};
    std::unordered_map<std::size_t, VolumeTextRow> volume_rows;
    std::unordered_map<std::size_t, WeatherKeyframeTextRow> keyframe_rows;

    auto parse_volume_field = [&](std::string_view key, std::string_view suffix, std::string_view value) {
        const auto separator = suffix.find('.');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("environment profile v2 volume key is malformed");
        }
        const auto ordinal = parse_ordinal(suffix.substr(0, separator), key);
        const auto field = suffix.substr(separator + 1U);
        auto& row = volume_rows[ordinal];
        if (field == "id") {
            row.desc.id = std::string{value};
        } else if (field == "shape") {
            row.desc.shape = parse_environment_volume_shape(value);
        } else if (field == "center_m") {
            row.desc.center_m = parse_vec3(value, key);
        } else if (field == "extents_m") {
            row.desc.extents_m = parse_vec3(value, key);
        } else if (field == "radius_m") {
            row.desc.radius_m = parse_float(value, key);
        } else if (field == "priority") {
            row.desc.priority = parse_i32(value, key);
        } else if (field == "blend_weight") {
            row.desc.blend_weight = parse_float(value, key);
        } else if (field == "fade_distance_m") {
            row.desc.fade_distance_m = parse_float(value, key);
        } else if (field == "blend_mode") {
            row.desc.blend_mode = parse_environment_volume_blend_mode(value);
        } else if (starts_with(field, "profile.")) {
            row.has_profile_payload = true;
            row.profile_payload << field.substr(std::string_view{"profile."}.size()) << '=' << value << '\n';
        } else {
            throw std::invalid_argument("environment profile v2 volume field is unsupported");
        }
    };

    auto parse_keyframe_field = [&](std::string_view key, std::string_view suffix, std::string_view value) {
        const auto separator = suffix.find('.');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("environment profile v2 weather keyframe key is malformed");
        }
        const auto ordinal = parse_ordinal(suffix.substr(0, separator), key);
        const auto field = suffix.substr(separator + 1U);
        auto& row = keyframe_rows[ordinal];
        if (field == "time_of_day_hours") {
            row.desc.time_of_day_hours = parse_float(value, key);
        } else if (field == "weather") {
            row.desc.weather = parse_environment_weather_kind(value);
        } else if (field == "precipitation") {
            row.desc.precipitation = parse_environment_precipitation_kind(value);
        } else if (field == "storm_intensity") {
            row.desc.storm_intensity = parse_float(value, key);
        } else if (field == "cloud_coverage") {
            row.desc.cloud_coverage = parse_float(value, key);
        } else if (field == "fog_density") {
            row.desc.fog_density = parse_float(value, key);
        } else if (field == "quality_preset") {
            row.desc.quality_preset = parse_environment_quality_preset(value);
        } else {
            throw std::invalid_argument("environment profile v2 weather keyframe field is unsupported");
        }
    };

    for (const auto& [key, value] : values) {
        const std::string_view key_view{key};
        if (key_view == "format") {
            continue;
        }
        if (key_view == "quality.preset") {
            document.quality_preset = parse_environment_quality_preset(value);
        } else if (starts_with(key_view, "global.")) {
            has_global_payload = true;
            global_payload << key_view.substr(std::string_view{"global."}.size()) << '=' << value << '\n';
        } else if (starts_with(key_view, "volume.")) {
            parse_volume_field(key_view, key_view.substr(std::string_view{"volume."}.size()), value);
        } else if (starts_with(key_view, "weather_keyframe.")) {
            parse_keyframe_field(key_view, key_view.substr(std::string_view{"weather_keyframe."}.size()), value);
        } else {
            throw std::invalid_argument("environment profile v2 key is unsupported");
        }
    }

    if (!has_global_payload) {
        throw std::invalid_argument("environment profile v2 global profile is missing");
    }
    document.global_profile = deserialize_environment_profile(global_payload.str());

    std::vector<std::size_t> volume_ordinals;
    volume_ordinals.reserve(volume_rows.size());
    for (const auto& [ordinal, _] : volume_rows) {
        volume_ordinals.push_back(ordinal);
    }
    std::ranges::sort(volume_ordinals);
    for (std::size_t expected = 0U; expected < volume_ordinals.size(); ++expected) {
        if (volume_ordinals[expected] != expected) {
            throw std::invalid_argument("environment profile v2 volume ordinals are not contiguous");
        }
        auto& row = volume_rows.at(expected);
        if (row.has_profile_payload) {
            document.volumes.push_back(row.desc);
            document.volumes.back().profile = deserialize_environment_profile(
                "format=GameEngine.EnvironmentProfile.v1\n" + row.profile_payload.str());
        } else {
            document.volumes.push_back(row.desc);
        }
    }

    std::vector<std::size_t> keyframe_ordinals;
    keyframe_ordinals.reserve(keyframe_rows.size());
    for (const auto& [ordinal, _] : keyframe_rows) {
        keyframe_ordinals.push_back(ordinal);
    }
    std::ranges::sort(keyframe_ordinals);
    for (std::size_t expected = 0U; expected < keyframe_ordinals.size(); ++expected) {
        if (keyframe_ordinals[expected] != expected) {
            throw std::invalid_argument("environment profile v2 weather keyframe ordinals are not contiguous");
        }
        document.weather_timeline.push_back(keyframe_rows.at(expected).desc);
    }

    if (!validate_environment_profile_v2(document).succeeded()) {
        throw std::invalid_argument("environment profile v2 document is invalid");
    }
    return document;
}

} // namespace mirakana
