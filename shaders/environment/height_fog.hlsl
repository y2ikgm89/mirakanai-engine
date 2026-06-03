// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

Texture2D<float4> scene_color_texture : register(t0);
SamplerState scene_color_sampler : register(s1);
Texture2D<float> scene_depth_texture : register(t2);
SamplerState scene_depth_sampler : register(s3);

struct HeightFogVertexOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

cbuffer HeightFogConstants : register(b4) {
    float density : packoffset(c0.x);
    float height_falloff : packoffset(c0.y);
    float height_offset_m : packoffset(c0.z);
    float start_distance_m : packoffset(c0.w);
    float cutoff_distance_m : packoffset(c1.x);
    float max_opacity : packoffset(c1.y);
    float sky_affect : packoffset(c1.z);
    float directional_inscattering_anisotropy : packoffset(c1.w);
    float inscattering_r : packoffset(c2.x);
    float inscattering_g : packoffset(c2.y);
    float inscattering_b : packoffset(c2.z);
    float directional_inscattering_r : packoffset(c2.w);
    float directional_inscattering_g : packoffset(c3.x);
    float directional_inscattering_b : packoffset(c3.y);
};

float height_fog_contract_factor(float scene_depth) {
    const float distance_m = scene_depth * cutoff_distance_m;
    const float height_term = exp2(-max(height_falloff, 0.0001) * max(height_offset_m, 0.0) * 0.001);
    const float distance_term = saturate((distance_m - start_distance_m) / max(cutoff_distance_m - start_distance_m, 1.0));
    const float density_term = 1.0 - exp2(-max(density, 0.0) * distance_term * height_term);
    return saturate(min(density_term, max_opacity));
}

float3 height_fog_contract_color(float2 uv) {
    const float directional_term = saturate((uv.y * 0.5) + (directional_inscattering_anisotropy * 0.5) + 0.5);
    const float3 inscattering = float3(inscattering_r, inscattering_g, inscattering_b);
    const float3 directional = float3(directional_inscattering_r, directional_inscattering_g, directional_inscattering_b);
    return lerp(inscattering, directional, directional_term * sky_affect);
}

HeightFogVertexOutput height_fog_vs_main(uint vertex_id : SV_VertexID) {
    const float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0),
    };

    HeightFogVertexOutput output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.uv = (positions[vertex_id] * 0.5) + float2(0.5, 0.5);
    return output;
}

float4 height_fog_ps_main(HeightFogVertexOutput input) : SV_Target0 {
    const float4 scene_color = scene_color_texture.Sample(scene_color_sampler, input.uv);
    const float scene_depth = scene_depth_texture.Sample(scene_depth_sampler, input.uv);
    const float fog_factor = height_fog_contract_factor(scene_depth);
    return float4(lerp(scene_color.rgb, height_fog_contract_color(input.uv), fog_factor), scene_color.a);
}

#if defined(MK_HEIGHT_FOG_VERTEX)
HeightFogVertexOutput main(uint vertex_id : SV_VertexID) {
    return height_fog_vs_main(vertex_id);
}
#elif defined(MK_HEIGHT_FOG_FRAGMENT)
float4 main(HeightFogVertexOutput input) : SV_Target0 {
    return height_fog_ps_main(input);
}
#else
// The shared contract can also be compiled by selecting height_fog_vs_main or height_fog_ps_main explicitly.
#endif
