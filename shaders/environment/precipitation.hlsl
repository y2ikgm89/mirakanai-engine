// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

Texture2D<float4> precipitation_particle_texture : register(t8);
Texture2D<float> precipitation_scene_depth : register(t9);
SamplerState precipitation_sampler : register(s8);

cbuffer PrecipitationConstants : register(b7) {
    float intensity;
    float fall_speed_mps;
    float wind_speed_mps;
    float particle_radius_mm;
    float time_seconds;
    float wetness_intensity;
    float occlusion_softness;
    float precipitation_kind_id;
};

struct PrecipitationVertexOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float alpha : TEXCOORD1;
};

PrecipitationVertexOut precipitation_vs_contract(uint vertex_id, uint instance_id) {
    const float2 quad_positions[4] = {
        float2(-1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, -1.0),
        float2(1.0, 1.0),
    };
    const float2 quad_uvs[4] = {
        float2(0.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 1.0),
        float2(1.0, 0.0),
    };

    const float drift = frac((float)instance_id * 0.61803398875 + (time_seconds * 0.07));
    const float column = frac((float)instance_id * 0.17320508075 + drift);
    const float fall = frac((time_seconds * max(fall_speed_mps, 0.0) * 0.05) + ((float)instance_id * 0.03125));
    const float wind = wind_speed_mps * 0.01;
    const float radius = max(particle_radius_mm, 0.01) * 0.002;

    PrecipitationVertexOut output;
    output.position = float4((column * 2.0 - 1.0) + wind, 1.0 - (fall * 2.0), 0.0, 1.0);
    output.position.xy += quad_positions[vertex_id] * radius;
    output.uv = quad_uvs[vertex_id];
    output.alpha = saturate(intensity);
    return output;
}

float precipitation_depth_occlusion(float2 uv, float particle_depth) {
    const float scene_depth = precipitation_scene_depth.Sample(precipitation_sampler, uv);
    const float softness = max(occlusion_softness, 0.0001);
    return saturate((scene_depth - particle_depth) / softness);
}

float4 precipitation_ps_contract(PrecipitationVertexOut input) : SV_Target0 {
    const float4 particle_sample = precipitation_particle_texture.Sample(precipitation_sampler, input.uv);
    const float depth_mask = precipitation_depth_occlusion(saturate(input.position.xy), input.position.z);
    const float wetness_boost = saturate(wetness_intensity * 0.25);
    const float alpha = saturate(particle_sample.a * input.alpha * depth_mask);
    const float3 color = saturate(particle_sample.rgb + wetness_boost);
    return float4(color, alpha);
}

#if defined(MK_PRECIPITATION_SHADER)
PrecipitationVertexOut main_vs(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID) {
    return precipitation_vs_contract(vertex_id, instance_id);
}

float4 main_ps(PrecipitationVertexOut input) : SV_Target0 {
    return precipitation_ps_contract(input);
}
#endif
