// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct VsOut {
    float4 position : SV_Position;
    float4 color : COLOR0;
};

VsOut vs_main(uint vertex_id : SV_VertexID) {
    const float2 positions[3] = {
        float2(0.0, 0.62),
        float2(0.68, -0.58),
        float2(-0.68, -0.58),
    };
    const float4 colors[3] = {
        float4(0.15, 0.68, 1.0, 1.0),
        float4(0.95, 0.85, 0.22, 1.0),
        float4(0.2, 0.9, 0.45, 1.0),
    };

    VsOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.color = colors[vertex_id];
    return output;
}

float4 ps_main(VsOut input) : SV_Target {
    return input.color;
}

struct NativeSpriteOverlayVertexIn {
    float2 position : POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float2 texture_flags : TEXCOORD1;
};

struct NativeSpriteOverlayVertexOut {
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float2 texture_flags : TEXCOORD1;
};

Texture2D sprite_atlas : register(t0);
SamplerState sprite_sampler : register(s1);

NativeSpriteOverlayVertexOut vs_native_sprite_overlay(NativeSpriteOverlayVertexIn input) {
    NativeSpriteOverlayVertexOut output;
    output.position = float4(input.position, 0.0, 1.0);
    output.color = saturate(input.color);
    output.uv = saturate(input.uv);
    output.texture_flags = input.texture_flags;
    return output;
}

float4 ps_native_sprite_overlay(NativeSpriteOverlayVertexOut input) : SV_Target {
    if (input.texture_flags.x > 0.5) {
        return saturate(sprite_atlas.Sample(sprite_sampler, input.uv) * input.color);
    }
    return input.color;
}
