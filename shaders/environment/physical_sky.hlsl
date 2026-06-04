// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct PhysicalSkyVertexOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

cbuffer PhysicalSkyConstants : register(b0) {
    float planet_radius_km : packoffset(c0.x);
    float atmosphere_height_km : packoffset(c0.y);
    float rayleigh_density_height_km : packoffset(c0.z);
    float mie_density_height_km : packoffset(c0.w);
    float mie_anisotropy : packoffset(c1.x);
    float ozone_density_height_km : packoffset(c1.y);
    float sun_angular_radius_radians : packoffset(c1.z);
    float solar_illuminance_lux : packoffset(c1.w);
};

float3 physical_sky_contract_color(float2 uv) {
    const float horizon = saturate(uv.y);
    const float rayleigh = saturate(rayleigh_density_height_km / 32.0);
    const float mie = saturate(mie_density_height_km / 8.0);
    const float ozone = saturate(ozone_density_height_km / 64.0);
    const float sun_disk = smoothstep(0.0, max(sun_angular_radius_radians, 0.0001), abs(uv.x - 0.5));
    const float solar = saturate(solar_illuminance_lux / 120000.0);
    return lerp(float3(0.03, 0.08, 0.18), float3(0.35 + rayleigh, 0.45 + ozone, 0.7), horizon) +
           (1.0 - sun_disk) * solar * float3(1.0, 0.88 + mie, 0.62);
}

PhysicalSkyVertexOutput physical_sky_vs_main(uint vertex_id : SV_VertexID) {
    const float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0),
    };

    PhysicalSkyVertexOutput output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.uv = (positions[vertex_id] * 0.5) + float2(0.5, 0.5);
    return output;
}

float4 physical_sky_ps_main(PhysicalSkyVertexOutput input) : SV_Target0 {
    return float4(physical_sky_contract_color(input.uv), 1.0);
}

#if defined(MK_PHYSICAL_SKY_VERTEX)
PhysicalSkyVertexOutput main(uint vertex_id : SV_VertexID) {
    return physical_sky_vs_main(vertex_id);
}
#elif defined(MK_PHYSICAL_SKY_FRAGMENT)
float4 main(PhysicalSkyVertexOutput input) : SV_Target0 {
    return physical_sky_ps_main(input);
}
#else
// The shared contract can also be compiled by selecting physical_sky_vs_main or physical_sky_ps_main explicitly.
#endif
