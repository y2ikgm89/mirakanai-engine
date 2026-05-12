// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

[[vk::binding(0, 0)]] RWByteAddressBuffer base_vertices : register(u0, space0);
[[vk::binding(1, 0)]] RWByteAddressBuffer position_deltas : register(u1, space0);
[[vk::binding(2, 0)]] cbuffer MorphWeights : register(b2, space0) {
    float weight0;
    float weight1;
};
[[vk::binding(3, 0)]] RWByteAddressBuffer output_positions : register(u3, space0);
[[vk::binding(4, 0)]] RWByteAddressBuffer normal_deltas : register(u4, space0);
[[vk::binding(5, 0)]] RWByteAddressBuffer tangent_deltas : register(u5, space0);
[[vk::binding(6, 0)]] RWByteAddressBuffer output_normals : register(u6, space0);
[[vk::binding(7, 0)]] RWByteAddressBuffer output_tangents : register(u7, space0);

[numthreads(1, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    const uint vertex_count = 3;
    const uint vertex = dispatch_thread_id.x;
    if (vertex >= vertex_count) {
        return;
    }

    const uint base_vertex_offset = vertex * 48;
    const uint output_offset = vertex * 12;
    const uint delta0_offset = vertex * 12;
    const uint delta1_offset = (vertex_count + vertex) * 12;

    const float3 base_position = float3(
        asfloat(base_vertices.Load(base_vertex_offset + 0)),
        asfloat(base_vertices.Load(base_vertex_offset + 4)),
        asfloat(base_vertices.Load(base_vertex_offset + 8)));
    const float3 base_normal = float3(
        asfloat(base_vertices.Load(base_vertex_offset + 12)),
        asfloat(base_vertices.Load(base_vertex_offset + 16)),
        asfloat(base_vertices.Load(base_vertex_offset + 20)));
    const float3 base_tangent = float3(
        asfloat(base_vertices.Load(base_vertex_offset + 32)),
        asfloat(base_vertices.Load(base_vertex_offset + 36)),
        asfloat(base_vertices.Load(base_vertex_offset + 40)));

    const float3 position_delta0 = float3(
        asfloat(position_deltas.Load(delta0_offset + 0)),
        asfloat(position_deltas.Load(delta0_offset + 4)),
        asfloat(position_deltas.Load(delta0_offset + 8)));
    const float3 position_delta1 = float3(
        asfloat(position_deltas.Load(delta1_offset + 0)),
        asfloat(position_deltas.Load(delta1_offset + 4)),
        asfloat(position_deltas.Load(delta1_offset + 8)));
    const float3 normal_delta0 = float3(
        asfloat(normal_deltas.Load(delta0_offset + 0)),
        asfloat(normal_deltas.Load(delta0_offset + 4)),
        asfloat(normal_deltas.Load(delta0_offset + 8)));
    const float3 normal_delta1 = float3(
        asfloat(normal_deltas.Load(delta1_offset + 0)),
        asfloat(normal_deltas.Load(delta1_offset + 4)),
        asfloat(normal_deltas.Load(delta1_offset + 8)));
    const float3 tangent_delta0 = float3(
        asfloat(tangent_deltas.Load(delta0_offset + 0)),
        asfloat(tangent_deltas.Load(delta0_offset + 4)),
        asfloat(tangent_deltas.Load(delta0_offset + 8)));
    const float3 tangent_delta1 = float3(
        asfloat(tangent_deltas.Load(delta1_offset + 0)),
        asfloat(tangent_deltas.Load(delta1_offset + 4)),
        asfloat(tangent_deltas.Load(delta1_offset + 8)));

    const float3 morphed_position = base_position + (position_delta0 * weight0) + (position_delta1 * weight1);
    const float3 morphed_normal = normalize(base_normal + (normal_delta0 * weight0) + (normal_delta1 * weight1));
    const float3 morphed_tangent = normalize(base_tangent + (tangent_delta0 * weight0) + (tangent_delta1 * weight1));

    output_positions.Store(output_offset + 0, asuint(morphed_position.x));
    output_positions.Store(output_offset + 4, asuint(morphed_position.y));
    output_positions.Store(output_offset + 8, asuint(morphed_position.z));
    output_normals.Store(output_offset + 0, asuint(morphed_normal.x));
    output_normals.Store(output_offset + 4, asuint(morphed_normal.y));
    output_normals.Store(output_offset + 8, asuint(morphed_normal.z));
    output_tangents.Store(output_offset + 0, asuint(morphed_tangent.x));
    output_tangents.Store(output_offset + 4, asuint(morphed_tangent.y));
    output_tangents.Store(output_offset + 8, asuint(morphed_tangent.z));
}
