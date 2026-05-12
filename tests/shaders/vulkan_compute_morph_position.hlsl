// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

[[vk::binding(0, 0)]] RWByteAddressBuffer base_positions : register(u0, space0);
[[vk::binding(1, 0)]] RWByteAddressBuffer position_deltas : register(u1, space0);
[[vk::binding(2, 0)]] cbuffer MorphWeights : register(b2, space0) {
    float weight0;
    float weight1;
};
[[vk::binding(3, 0)]] RWByteAddressBuffer output_positions : register(u3, space0);

[numthreads(1, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    const uint vertex_count = 3;
    const uint vertex = dispatch_thread_id.x;
    if (vertex >= vertex_count) {
        return;
    }

    const uint base_offset = vertex * 12;
    const uint delta0_offset = vertex * 12;
    const uint delta1_offset = (vertex_count + vertex) * 12;

    const float3 base = float3(
        asfloat(base_positions.Load(base_offset + 0)),
        asfloat(base_positions.Load(base_offset + 4)),
        asfloat(base_positions.Load(base_offset + 8)));
    const float3 delta0 = float3(
        asfloat(position_deltas.Load(delta0_offset + 0)),
        asfloat(position_deltas.Load(delta0_offset + 4)),
        asfloat(position_deltas.Load(delta0_offset + 8)));
    const float3 delta1 = float3(
        asfloat(position_deltas.Load(delta1_offset + 0)),
        asfloat(position_deltas.Load(delta1_offset + 4)),
        asfloat(position_deltas.Load(delta1_offset + 8)));
    const float3 morphed = base + (delta0 * weight0) + (delta1 * weight1);

    output_positions.Store(base_offset + 0, asuint(morphed.x));
    output_positions.Store(base_offset + 4, asuint(morphed.y));
    output_positions.Store(base_offset + 8, asuint(morphed.z));
}
