// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct ReceiverVertexOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 world_pos : TEXCOORD1;
};

#if defined(MK_VULKAN_SHADOW_RECEIVER_VERTEX)
ReceiverVertexOutput main(uint vertex_id : SV_VertexID) {
    const float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0),
    };

    ReceiverVertexOutput output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.uv = (positions[vertex_id] * 0.5) + float2(0.5, 0.5);
    const float2 ndc = positions[vertex_id];
    output.world_pos = float3(ndc.x, ndc.y, 0.5);
    return output;
}
#elif defined(MK_VULKAN_SHADOW_RECEIVER_FRAGMENT)
[[vk::binding(0, 0)]] Texture2D<float> shadow_depth_texture : register(t0, space0);
[[vk::binding(1, 0)]] SamplerState shadow_sampler : register(s1, space0);
[[vk::binding(2, 0)]] cbuffer ShadowReceiverCb : register(b2) {
    uint cascade_count;
    uint3 pad0;
    float4 splits0;
    float4 splits1;
    float4 splits2;
    row_major float4x4 clip_from_world[8];
    row_major float4x4 camera_view_from_world;
    uint4 pad1[8];
};

float2 receiver_atlas_uv(float3 world_pos) {
    float4 lc = mul(float4(world_pos, 1.0), clip_from_world[0]);
    float invw = 1.0 / max(abs(lc.w), 1e-6);
    float2 ndc = lc.xy * invw;
    float2 tile_uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
    float inv_cc = 1.0 / max((float)cascade_count, 1.0);
    return float2(tile_uv.x * inv_cc, tile_uv.y);
}

float receiver_depth_from_clip(float3 world_pos) {
    float4 lc = mul(float4(world_pos, 1.0), clip_from_world[0]);
    float invw = 1.0 / max(abs(lc.w), 1e-6);
    // Match D3D12/Vulkan depth buffer: NDC z after divide is in [0, 1].
    return saturate(lc.z * invw);
}

float directional_shadow_pcf_3x3(float2 uv, float receiver_depth) {
    uint width = 1;
    uint height = 1;
    shadow_depth_texture.GetDimensions(width, height);

    float safe_width = width == 0 ? 1.0 : (float)width;
    float safe_height = height == 0 ? 1.0 : (float)height;
    float2 texel = 1.0 / float2(safe_width, safe_height);
    float occlusion = 0.0;

    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            const float sample_depth =
                shadow_depth_texture.SampleLevel(shadow_sampler, uv + float2((float)x, (float)y) * texel, 0.0);
            occlusion += sample_depth + 0.002 < receiver_depth ? 1.0 : 0.0;
        }
    }
    return occlusion / 9.0;
}

float4 main(ReceiverVertexOutput input) : SV_Target0 {
    const float2 atlas_uv = receiver_atlas_uv(input.world_pos);
    const float receiver_depth = receiver_depth_from_clip(input.world_pos);
    const float lit_intensity = 1.0;
    const float shadow_intensity = 0.3;
    const float occlusion = directional_shadow_pcf_3x3(atlas_uv, receiver_depth);
    const float intensity = lerp(lit_intensity, shadow_intensity, occlusion);
    return float4(intensity, intensity, intensity, 1.0);
}
#else
#error Define one MK_VULKAN_SHADOW_RECEIVER_* shader variant before compiling this test shader.
#endif

