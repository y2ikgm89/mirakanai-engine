// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct VsIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct VsOut {
    float4 position : SV_Position;
};

VsOut vs_shadow(VsIn input) {
    VsOut output;
    output.position = float4(input.position.xy, 0.35, 1.0);
    return output;
}

float4 ps_shadow(VsOut input) : SV_Target {
    float depth_tint = saturate(input.position.z);
    return float4(depth_tint * 0.0, 0.0, 0.0, 1.0);
}
