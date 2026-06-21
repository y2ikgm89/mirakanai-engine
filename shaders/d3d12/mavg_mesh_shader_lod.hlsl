// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct Payload {
    uint color_index;
};

struct VertexOut {
    float4 position : SV_Position;
    float4 color : COLOR0;
};

[numthreads(1, 1, 1)]
void as_main(uint3 dispatch_id : SV_DispatchThreadID) {
    Payload payload_out;
    payload_out.color_index = dispatch_id.x;
    DispatchMesh(1, 1, 1, payload_out);
}

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void ms_main(in payload Payload payload_in,
             out vertices VertexOut vertices_out[3],
             out indices uint3 primitives_out[1]) {
    SetMeshOutputCounts(3, 1);
    vertices_out[0].position = float4(0.0, 0.5, 0.0, 1.0);
    vertices_out[1].position = float4(0.5, -0.5, 0.0, 1.0);
    vertices_out[2].position = float4(-0.5, -0.5, 0.0, 1.0);
    vertices_out[0].color = float4(0.0, 1.0, 0.0, 1.0);
    vertices_out[1].color = float4(0.0, 1.0, 0.0, 1.0);
    vertices_out[2].color = float4(0.0, 1.0, 0.0, 1.0);
    primitives_out[0] = uint3(0, 1, 2);
}

float4 ps_main(VertexOut input) : SV_Target {
    return input.color;
}
