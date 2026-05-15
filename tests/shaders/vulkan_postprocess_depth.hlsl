// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct PostprocessVertexOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

#if defined(MK_VULKAN_POSTPROCESS_DEPTH_VERTEX)
PostprocessVertexOutput main(uint vertex_id : SV_VertexID) {
    const float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0),
    };

    PostprocessVertexOutput output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.uv = (positions[vertex_id] * 0.5) + float2(0.5, 0.5);
    return output;
}
#elif defined(MK_VULKAN_POSTPROCESS_DEPTH_FRAGMENT)
[[vk::binding(0, 0)]] Texture2D scene_color_texture : register(t0, space0);
[[vk::binding(1, 0)]] SamplerState scene_color_sampler : register(s1, space0);
[[vk::binding(2, 0)]] Texture2D<float> scene_depth_texture : register(t2, space0);
[[vk::binding(3, 0)]] SamplerState scene_depth_sampler : register(s3, space0);

float4 main(PostprocessVertexOutput input) : SV_Target0 {
    const float4 scene_color = scene_color_texture.SampleLevel(scene_color_sampler, input.uv, 0.0);
    const float scene_depth = scene_depth_texture.SampleLevel(scene_depth_sampler, input.uv, 0.0);
    return float4(scene_depth, scene_color.g, 1.0 - scene_depth, 1.0);
}
#else
#error Define one MK_VULKAN_POSTPROCESS_DEPTH_* shader variant before compiling this test shader.
#endif
