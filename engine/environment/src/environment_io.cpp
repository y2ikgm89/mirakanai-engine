// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/environment_io.hpp"

#include <cmath>
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

std::string serialize_environment_profile(const EnvironmentProfileDesc& desc) {
    std::ostringstream output;
    output << "format=" << environment_profile_format << '\n';
    write_environment_profile_payload(output, desc);
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

} // namespace mirakana
