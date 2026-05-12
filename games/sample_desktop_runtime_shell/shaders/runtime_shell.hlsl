// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct VsOut {
    float4 position : SV_Position;
    float4 color : COLOR0;
};

VsOut vs_main(uint vertex_id : SV_VertexID) {
    const float2 positions[3] = {
        float2(0.0, 0.65),
        float2(0.65, -0.55),
        float2(-0.65, -0.55),
    };
    const float4 colors[3] = {
        float4(0.2, 0.7, 1.0, 1.0),
        float4(0.1, 0.9, 0.45, 1.0),
        float4(1.0, 0.35, 0.2, 1.0),
    };

    VsOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.color = colors[vertex_id];
    return output;
}

float4 ps_main(VsOut input) : SV_Target {
    return input.color;
}
