// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct Payload {
    uint color_index;
};

struct VertexOut {
    float4 position : SV_Position;
    float4 color : COLOR0;
};

[[vk::binding(0, 0)]] StructuredBuffer<uint> shader_payload : register(t0, space0);

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void mesh_main(in payload Payload payload_in,
               out vertices VertexOut vertices_out[3],
               out indices uint3 primitives_out[1]) {
    SetMeshOutputCounts(3, 1);
    const uint mesh_payload = shader_payload[1];
    const float red = (mesh_payload & 1u) == 0u ? 0.0 : 0.25;
    const float green = ((payload_in.color_index ^ mesh_payload) & 1u) == 0u ? 1.0 : 0.5;
    vertices_out[0].position = float4(0.0, 0.5, 0.0, 1.0);
    vertices_out[1].position = float4(0.5, -0.5, 0.0, 1.0);
    vertices_out[2].position = float4(-0.5, -0.5, 0.0, 1.0);
    vertices_out[0].color = float4(red, green, 0.0, 1.0);
    vertices_out[1].color = float4(red, green, 0.0, 1.0);
    vertices_out[2].color = float4(red, green, 0.0, 1.0);
    primitives_out[0] = uint3(0, 1, 2);
}
