// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary
// GameEngine.MaterialGraphGeneratedHlsl
// Deterministic reviewed bridge for material graph authoring evidence.

cbuffer MaterialFactors : register(b0) {
    float4 base_color;
    float3 emissive;
    float metallic;
    float roughness;
};

cbuffer ScenePbrFrame : register(b6) {
    row_major float4x4 clip_from_world;
    row_major float4x4 world_from_object;
    float4 camera_position_aspect;
    float4 light_dir_intensity;
    float4 light_color_pad;
    float4 ambient_pad;
};

Texture2D<float4> base_color_texture : register(t1);
SamplerState base_color_sampler : register(s16);

struct VsOut {
    float4 position : SV_Position;
    float3 world_position : TEXCOORD1;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

VsOut VSMain(uint vertex_id : SV_VertexID) {
    float2 positions[3] = {float2(-0.75, -0.75), float2(0.75, -0.75), float2(0.0, 0.75)};
    float2 uvs[3] = {float2(0.0, 1.0), float2(1.0, 1.0), float2(0.5, 0.0)};

    VsOut output;
    float4 object_pos = float4(positions[vertex_id], 0.0, 1.0);
    float4 world_pos = mul(object_pos, world_from_object);
    output.position = mul(world_pos, clip_from_world);
    output.world_position = world_pos.xyz;
    output.normal = float3(0.0, 0.0, 1.0);
    output.uv = uvs[vertex_id];
    return output;
}

float4 PSMain(VsOut input) : SV_TARGET0 {
    float4 sampled = base_color_texture.Sample(base_color_sampler, input.uv);
    float3 lit_color = saturate((sampled.rgb * base_color.rgb) + emissive.rgb + ambient_pad.rgb * 0.04);
    return float4(lit_color, sampled.a * base_color.a);
}
