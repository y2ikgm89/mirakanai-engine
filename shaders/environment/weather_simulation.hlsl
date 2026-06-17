// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct CellInput {
    float temperature_celsius;
    float vapor_water_kg_per_m2;
    float cloud_water_kg_per_m2;
    float surface_water_kg_per_m2;
    float surface_evaporation_kg_per_m2_s;
    float temperature_delta_celsius_per_s;
    float cloud_precipitation_rate_per_s;
    float padding0;
};

struct CellOutput {
    float temperature_celsius;
    float vapor_water_kg_per_m2;
    float cloud_water_kg_per_m2;
    float surface_water_kg_per_m2;
    float saturation_vapor_kg_per_m2;
    float evaporated_kg_per_m2;
    float condensed_kg_per_m2;
    float precipitated_kg_per_m2;
};

#if defined(__spirv__)
[[vk::binding(0, 0)]]
#endif
StructuredBuffer<CellInput> input_cells : register(t0);

#if defined(__spirv__)
[[vk::binding(1, 0)]]
#endif
RWStructuredBuffer<CellOutput> output_cells : register(u0);

#if defined(__spirv__)
[[vk::binding(2, 0)]]
#endif
cbuffer Constants : register(b0) {
    uint cell_count;
    float effective_timestep_s;
    float air_pressure_hpa;
    float mixing_height_m;
};

float saturation_vapor_kg_per_m2(float temperature_celsius, float pressure_hpa, float mixing_height) {
    const float temperature_kelvin = temperature_celsius + 273.15f;
    const float pressure_pa = pressure_hpa * 100.0f;
    const float saturation_hpa = 6.11f * pow(10.0f, (7.5f * temperature_celsius) /
                                                       (237.3f + temperature_celsius));
    const float saturation_pa = saturation_hpa * 100.0f;
    const float air_density = pressure_pa / (287.05f * temperature_kelvin);
    const float air_mass = air_density * mixing_height;
    const float specific_humidity = (0.622f * saturation_pa) / (pressure_pa - (0.378f * saturation_pa));
    return specific_humidity * air_mass;
}

[numthreads(64, 1, 1)]
void cs_environment_weather(uint3 dispatch_id : SV_DispatchThreadID) {
    const uint index = dispatch_id.x;
    if (index >= cell_count) {
        return;
    }

    const CellInput input = input_cells[index];
    CellOutput output;
    output.temperature_celsius = input.temperature_celsius +
                                 (input.temperature_delta_celsius_per_s * effective_timestep_s);

    const float evaporated = min(input.surface_water_kg_per_m2,
                                 input.surface_evaporation_kg_per_m2_s * effective_timestep_s);
    float vapor = input.vapor_water_kg_per_m2 + evaporated;
    float surface = input.surface_water_kg_per_m2 - evaporated;
    float cloud = input.cloud_water_kg_per_m2;

    const float saturation = saturation_vapor_kg_per_m2(output.temperature_celsius,
                                                        air_pressure_hpa,
                                                        mixing_height_m);
    const float condensed = max(0.0f, vapor - saturation);
    vapor -= condensed;
    cloud += condensed;

    const float precipitated = min(cloud, cloud * input.cloud_precipitation_rate_per_s * effective_timestep_s);
    cloud -= precipitated;
    surface += precipitated;

    output.vapor_water_kg_per_m2 = vapor;
    output.cloud_water_kg_per_m2 = cloud;
    output.surface_water_kg_per_m2 = surface;
    output.saturation_vapor_kg_per_m2 = saturation;
    output.evaporated_kg_per_m2 = evaporated;
    output.condensed_kg_per_m2 = condensed;
    output.precipitated_kg_per_m2 = precipitated;
    output_cells[index] = output;
}
