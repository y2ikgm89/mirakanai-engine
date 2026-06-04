// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

Texture2D<float4> volumetric_cloud_weather_map : register(t10);
Texture3D<float4> volumetric_cloud_shape_noise : register(t11);
Texture3D<float4> volumetric_cloud_erosion_noise : register(t12);
SamplerState volumetric_cloud_sampler : register(s10);

cbuffer VolumetricCloudConstants : register(b8) {
    float coverage;
    float density;
    float altitude_min_m;
    float altitude_max_m;
    float wind_u_mps;
    float wind_v_mps;
    float time_seconds;
    float temporal_history_weight;
    float cloud_darkening;
    float lightning_flash_intensity;
    float precipitation_boost;
    float exposure_response;
    uint primary_steps;
    uint light_steps;
    uint shadow_mode;
    uint _padding0;
};

struct VolumetricCloudVertexOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VolumetricCloudVertexOut volumetric_cloud_vs_contract(uint vertex_id) {
    const float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0),
    };
    const float2 uvs[3] = {
        float2(0.0, 1.0),
        float2(0.0, -1.0),
        float2(2.0, 1.0),
    };

    VolumetricCloudVertexOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.uv = uvs[vertex_id];
    return output;
}

float volumetric_cloud_density_contract(float3 sample_position) {
    const float2 weather_uv = frac(sample_position.xy + (float2(wind_u_mps, wind_v_mps) * time_seconds * 0.00005));
    const float weather_coverage = volumetric_cloud_weather_map.Sample(volumetric_cloud_sampler, weather_uv).r;
    const float shape = volumetric_cloud_shape_noise.Sample(volumetric_cloud_sampler, frac(sample_position)).r;
    const float erosion = volumetric_cloud_erosion_noise.Sample(volumetric_cloud_sampler, frac(sample_position * 2.0)).r;
    const float altitude_fraction = saturate((sample_position.z - altitude_min_m) / max(altitude_max_m - altitude_min_m, 1.0));
    const float altitude_shape = saturate(1.0 - abs((altitude_fraction * 2.0) - 1.0));
    return saturate(((weather_coverage + shape) * 0.5 - erosion * 0.35 + coverage - 0.5) * density * altitude_shape);
}

float4 volumetric_cloud_ps_contract(VolumetricCloudVertexOut input) : SV_Target0 {
    const uint bounded_primary_steps = min(max(primary_steps, 1U), 128U);
    float transmittance = 1.0;
    float scattering = 0.0;

    [loop]
    for (uint step = 0U; step < bounded_primary_steps; ++step) {
        const float step_fraction = ((float)step + 0.5) / (float)bounded_primary_steps;
        const float altitude = lerp(altitude_min_m, altitude_max_m, step_fraction);
        const float3 sample_position = float3(input.uv, altitude * 0.0001 + step_fraction);
        const float sample_density = volumetric_cloud_density_contract(sample_position);
        const float light_factor = saturate(1.0 - cloud_darkening + (lightning_flash_intensity * 0.00001));
        scattering += transmittance * sample_density * light_factor;
        transmittance *= saturate(1.0 - sample_density * 0.08);
    }

    const float alpha = saturate(1.0 - transmittance);
    const float storm_boost = saturate((precipitation_boost + exposure_response) * 0.25);
    const float3 cloud_color = saturate(float3(scattering, scattering, scattering) + storm_boost);
    return float4(cloud_color, alpha);
}

#if defined(MK_VOLUMETRIC_CLOUD_SHADER)
VolumetricCloudVertexOut main_vs(uint vertex_id : SV_VertexID) {
    return volumetric_cloud_vs_contract(vertex_id);
}

float4 main_ps(VolumetricCloudVertexOut input) : SV_Target0 {
    return volumetric_cloud_ps_contract(input);
}
#endif
