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
    float2 screen_uv : TEXCOORD2;
};

PrecipitationVertexOut precipitation_vs_contract(float2 quad_position,
                                                 float2 quad_uv,
                                                 uint instance_id,
                                                 float4 instance_center_radius,
                                                 float4 instance_motion_kind) {
    const float seed = frac(instance_motion_kind.x + (float)instance_id * 0.61803398875);
    const float fall_phase = frac(instance_motion_kind.z + (time_seconds * max(fall_speed_mps, 0.0) * 0.05));
    const float wind = wind_speed_mps * 0.01 * max(instance_motion_kind.y, 0.0);
    const float radius = max(particle_radius_mm, 0.01) * 0.002 * max(instance_center_radius.w, 1.0);
    const float2 center = instance_center_radius.xy + float2(wind + (seed * 0.02), -fall_phase * 0.02);

    PrecipitationVertexOut output;
    output.position = float4(center, saturate(instance_center_radius.z), 1.0);
    output.position.xy += quad_position * radius;
    output.uv = quad_uv;
    output.alpha = saturate(intensity);
    output.screen_uv = saturate((output.position.xy * 0.5) + 0.5);
    return output;
}

float precipitation_depth_occlusion(float2 uv, float particle_depth) {
    const float scene_depth = precipitation_scene_depth.Sample(precipitation_sampler, uv);
    const float softness = max(occlusion_softness, 0.0001);
    return saturate((scene_depth - particle_depth) / softness);
}

float4 precipitation_ps_contract(PrecipitationVertexOut input) : SV_Target0 {
    const float4 particle_sample = precipitation_particle_texture.Sample(precipitation_sampler, input.uv);
    const float depth_mask = precipitation_depth_occlusion(input.screen_uv, input.position.z);
    const float wetness_boost = saturate(wetness_intensity * 0.25);
    const float alpha = saturate(particle_sample.a * input.alpha * depth_mask);
    const float3 color = saturate(particle_sample.rgb + wetness_boost);
    return float4(color, alpha);
}

#if defined(MK_PRECIPITATION_SHADER)
PrecipitationVertexOut main_vs(float2 quad_position : POSITION,
                               float2 quad_uv : TEXCOORD0,
                               uint instance_id : SV_InstanceID,
                               float4 instance_center_radius : TEXCOORD1,
                               float4 instance_motion_kind : TEXCOORD2) {
    return precipitation_vs_contract(quad_position, quad_uv, instance_id, instance_center_radius, instance_motion_kind);
}

float4 main_ps(PrecipitationVertexOut input) : SV_Target0 {
    return precipitation_ps_contract(input);
}
#endif
