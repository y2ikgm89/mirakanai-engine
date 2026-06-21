// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct VertexOut {
    float4 position : SV_Position;
    float4 color : COLOR0;
};

float4 fragment_main(VertexOut input) : SV_Target {
    return input.color;
}
