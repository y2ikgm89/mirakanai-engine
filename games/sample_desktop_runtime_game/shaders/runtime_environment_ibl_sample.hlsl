// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct EnvironmentIblSampleVsOut {
    float4 position : SV_Position;
    float3 direction : TEXCOORD0;
};

EnvironmentIblSampleVsOut vs_environment_ibl_sample(uint vertex_id : SV_VertexID) {
    const float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0),
    };
    EnvironmentIblSampleVsOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.direction = float3(1.0, 0.0, 0.0);
    return output;
}

TextureCube<float4> environment_ibl : register(t0);
SamplerState environment_sampler : register(s0);

float4 ps_environment_ibl_sample(EnvironmentIblSampleVsOut input) : SV_Target {
    return environment_ibl.SampleLevel(environment_sampler, normalize(input.direction), 0.0);
}
