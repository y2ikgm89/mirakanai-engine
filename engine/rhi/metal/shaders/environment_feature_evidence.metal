#include <metal_stdlib>

using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float3 cube_direction;
};

vertex VertexOut mk_environment_feature_vertex(uint vertex_id [[vertex_id]]) {
    constexpr float2 positions[3] = {
        float2(-1.0f, -1.0f),
        float2(3.0f, -1.0f),
        float2(-1.0f, 3.0f),
    };

    const float2 position = positions[vertex_id];
    VertexOut out;
    out.position = float4(position, 0.0f, 1.0f);
    out.cube_direction = normalize(float3(position.x, position.y, 1.0f));
    return out;
}

fragment float4 mk_environment_feature_fragment(VertexOut in [[stage_in]],
                                                texturecube<float> environment_cube [[texture(0)]]) {
    constexpr sampler cube_sampler(coord::normalized, filter::nearest, address::clamp_to_edge);
    const float4 sampled = environment_cube.sample(cube_sampler, in.cube_direction);
    return float4(max(sampled.rgb, float3(0.25f, 0.5f, 0.75f)), 1.0f);
}

kernel void mk_environment_feature_compute(device uint* output [[buffer(0)]],
                                           uint index [[thread_position_in_grid]]) {
    output[index] = 0x4d4b4555u;
}
