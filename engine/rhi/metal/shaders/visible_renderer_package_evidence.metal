#include <metal_stdlib>

using namespace metal;

struct VisibleRendererPackageVertexOut {
    float4 position [[position]];
    float2 uv;
};

vertex VisibleRendererPackageVertexOut mk_visible_renderer_package_vertex(uint vertex_id [[vertex_id]]) {
    constexpr float2 positions[3] = {
        float2(-1.0f, -1.0f),
        float2(3.0f, -1.0f),
        float2(-1.0f, 3.0f),
    };

    const float2 position = positions[vertex_id];
    VisibleRendererPackageVertexOut out;
    out.position = float4(position, 0.0f, 1.0f);
    out.uv = position * 0.5f + 0.5f;
    return out;
}

fragment float4 mk_visible_renderer_package_fragment(VisibleRendererPackageVertexOut in [[stage_in]]) {
    const float3 albedo = float3(0.82f, 0.47f, 0.21f);
    const float3 normal = normalize(float3(in.uv.x - 0.5f, in.uv.y - 0.5f, 1.0f));
    const float3 light_direction = normalize(float3(0.35f, 0.7f, 1.0f));
    const float ndotl = max(dot(normal, light_direction), 0.0f);
    const float3 lit = albedo * (0.18f + 0.82f * ndotl);
    const float3 postprocessed = pow(lit / (lit + float3(1.0f)), float3(1.0f / 2.2f));
    return float4(postprocessed, 1.0f);
}
