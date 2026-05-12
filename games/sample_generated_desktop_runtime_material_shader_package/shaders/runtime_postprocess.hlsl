// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

Texture2D scene_color_texture : register(t0);
SamplerState scene_color_sampler : register(s1);

struct VsOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VsOut vs_postprocess(uint vertex_id : SV_VertexID) {
    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0),
    };
    float2 uvs[3] = {
        float2(0.0, 1.0),
        float2(0.0, -1.0),
        float2(2.0, 1.0),
    };

    VsOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.uv = uvs[vertex_id];
    return output;
}

float4 ps_postprocess(VsOut input) : SV_Target {
    float4 scene = scene_color_texture.Sample(scene_color_sampler, input.uv);
    float3 graded = saturate(scene.rgb * float3(1.04, 1.02, 0.98) + float3(0.012, 0.008, 0.0));
    return float4(graded, scene.a);
}