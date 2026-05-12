// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct VsIn {
    [[vk::location(0)]] float3 position : POSITION;
};

struct VsOut {
    float4 position : SV_Position;
};

VsOut vs_main(VsIn input) {
    VsOut output;
    output.position = float4(input.position.xy, input.position.z, 1.0);
    return output;
}

float4 ps_main(float4 position : SV_Position) : SV_Target0 {
    return float4(0.2, 0.6, 0.9, 1.0);
}
