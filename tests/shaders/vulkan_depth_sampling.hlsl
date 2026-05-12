struct DepthVertexInput {
    float3 position : POSITION;
    float4 color : COLOR0;
};

struct DepthVertexOutput {
    float4 position : SV_Position;
    float4 color : COLOR0;
};

struct SampleVertexOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

#if defined(MK_VULKAN_DEPTH_WRITE_VERTEX)
DepthVertexOutput main(DepthVertexInput input) {
    DepthVertexOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}
#elif defined(MK_VULKAN_DEPTH_WRITE_FRAGMENT)
float4 main(DepthVertexOutput input) : SV_Target0 {
    return input.color;
}
#elif defined(MK_VULKAN_DEPTH_SAMPLE_VERTEX)
SampleVertexOutput main(uint vertex_id : SV_VertexID) {
    const float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0),
    };

    SampleVertexOutput output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.uv = (positions[vertex_id] * 0.5) + float2(0.5, 0.5);
    return output;
}
#elif defined(MK_VULKAN_DEPTH_SAMPLE_FRAGMENT)
[[vk::binding(0, 0)]] Texture2D<float> depth_texture : register(t0, space0);
[[vk::binding(1, 0)]] SamplerState depth_sampler : register(s1, space0);

float4 main(SampleVertexOutput input) : SV_Target0 {
    const float depth = depth_texture.SampleLevel(depth_sampler, input.uv, 0.0);
    return float4(depth, depth, depth, 1.0);
}
#else
#error Define one MK_VULKAN_DEPTH_* shader variant before compiling this test shader.
#endif

