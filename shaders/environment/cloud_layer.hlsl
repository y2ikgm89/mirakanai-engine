// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

Texture2D<float4> cloud_layer_cloud_map : register(t6);
Texture2D<float4> cloud_layer_flow_map : register(t7);
SamplerState cloud_layer_sampler : register(s6);

cbuffer CloudLayerConstants : register(b6) {
    float coverage;
    float opacity;
    float altitude_m;
    float wind_u_mps;
    float wind_v_mps;
    float time_seconds;
    float sky_tint_r;
    float sky_tint_g;
    float sky_tint_b;
    float time_of_day_response;
};

struct CloudLayerVertexOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

CloudLayerVertexOut cloud_layer_vs_contract(uint vertex_id) {
    const float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0),
    };
    const float2 uvs[3] = {
        float2(0.0, 1.0),
        float2(0.0, -1.0),
        float2(2.0, 1.0),
    };

    CloudLayerVertexOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.uv = uvs[vertex_id];
    return output;
}

float4 cloud_layer_ps_contract(CloudLayerVertexOut input) : SV_Target0 {
    const float2 flow = (cloud_layer_flow_map.Sample(cloud_layer_sampler, input.uv).rg * 2.0) - 1.0;
    const float2 wind = float2(wind_u_mps, wind_v_mps) * (time_seconds / max(altitude_m, 1.0));
    const float2 uv = frac(input.uv + (flow * 0.02) + wind);
    const float4 cloud_sample = cloud_layer_cloud_map.Sample(cloud_layer_sampler, uv);
    const float cloud_alpha = saturate((cloud_sample.a * opacity) + coverage - 0.5);
    const float3 sky_tint = saturate(float3(sky_tint_r, sky_tint_g, sky_tint_b));
    const float3 tinted_cloud = lerp(cloud_sample.rgb, cloud_sample.rgb * sky_tint, saturate(time_of_day_response));
    return float4(tinted_cloud, cloud_alpha);
}

#if defined(MK_CLOUD_LAYER_SHADER)
CloudLayerVertexOut main_vs(uint vertex_id : SV_VertexID) {
    return cloud_layer_vs_contract(vertex_id);
}

float4 main_ps(CloudLayerVertexOut input) : SV_Target0 {
    return cloud_layer_ps_contract(input);
}
#endif
