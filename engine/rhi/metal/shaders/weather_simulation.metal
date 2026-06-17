// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <metal_stdlib>
using namespace metal;

struct WeatherCellInput {
    float temperature_celsius;
    float vapor_water_kg_per_m2;
    float cloud_water_kg_per_m2;
    float surface_water_kg_per_m2;
    float surface_evaporation_kg_per_m2_s;
    float temperature_delta_celsius_per_s;
    float cloud_precipitation_rate_per_s;
    float padding0;
};

struct WeatherCellOutput {
    float temperature_celsius;
    float vapor_water_kg_per_m2;
    float cloud_water_kg_per_m2;
    float surface_water_kg_per_m2;
    float saturation_vapor_kg_per_m2;
    float evaporated_kg_per_m2;
    float condensed_kg_per_m2;
    float precipitated_kg_per_m2;
};

struct WeatherConstants {
    float effective_timestep_s;
    float air_pressure_hpa;
    float mixing_height_m;
    uint cell_count;
};

static float saturation_vapor_kg_per_m2(float temperature_celsius, float air_pressure_hpa, float mixing_height_m) {
    const float vapor_pressure_hpa = 6.112f * exp((17.67f * temperature_celsius) / (temperature_celsius + 243.5f));
    const float mixing_ratio = 0.622f * vapor_pressure_hpa / max(air_pressure_hpa - vapor_pressure_hpa, 1.0f);
    const float air_density_kg_per_m3 = 1.225f;
    return max(0.0f, mixing_ratio * air_density_kg_per_m3 * mixing_height_m);
}

[[kernel]] void mk_environment_weather_solver(
    const device WeatherCellInput* cells [[buffer(0)]],
    device WeatherCellOutput* output [[buffer(1)]],
    constant WeatherConstants& constants [[buffer(2)]],
    uint id [[thread_position_in_grid]]) {
    if (id >= constants.cell_count) {
        return;
    }

    const WeatherCellInput cell = cells[id];
    const float timestep = max(constants.effective_timestep_s, 0.0f);
    const float temperature = cell.temperature_celsius + cell.temperature_delta_celsius_per_s * timestep;
    const float saturation =
        saturation_vapor_kg_per_m2(temperature, constants.air_pressure_hpa, constants.mixing_height_m);
    const float evaporated = min(max(cell.surface_water_kg_per_m2, 0.0f),
                                 max(cell.surface_evaporation_kg_per_m2_s, 0.0f) * timestep);
    float vapor = max(cell.vapor_water_kg_per_m2, 0.0f) + evaporated;
    float cloud = max(cell.cloud_water_kg_per_m2, 0.0f);
    float surface = max(cell.surface_water_kg_per_m2, 0.0f) - evaporated;

    const float condensed = max(vapor - saturation, 0.0f);
    vapor -= condensed;
    cloud += condensed;

    const float precipitated = min(cloud, max(cell.cloud_precipitation_rate_per_s, 0.0f) * timestep * cloud);
    cloud -= precipitated;
    surface += precipitated;

    output[id] = WeatherCellOutput{
        temperature,
        vapor,
        cloud,
        surface,
        saturation,
        evaporated,
        condensed,
        precipitated,
    };
}
