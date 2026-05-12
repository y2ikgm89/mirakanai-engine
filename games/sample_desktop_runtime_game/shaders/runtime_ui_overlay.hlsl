// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct NativeUiOverlayVertexIn {
    float2 position : POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float2 texture_flags : TEXCOORD1;
};

struct NativeUiOverlayVertexOut {
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float2 texture_flags : TEXCOORD1;
};

Texture2D ui_atlas : register(t0);
SamplerState ui_sampler : register(s1);

NativeUiOverlayVertexOut vs_native_ui_overlay(NativeUiOverlayVertexIn input) {
    NativeUiOverlayVertexOut output;
    output.position = float4(input.position, 0.0, 1.0);
    output.color = saturate(input.color);
    output.uv = saturate(input.uv);
    output.texture_flags = input.texture_flags;
    return output;
}

float4 ps_native_ui_overlay(NativeUiOverlayVertexOut input) : SV_Target {
    if (input.texture_flags.x > 0.5) {
        return saturate(ui_atlas.Sample(ui_sampler, input.uv) * input.color);
    }
    return input.color;
}
