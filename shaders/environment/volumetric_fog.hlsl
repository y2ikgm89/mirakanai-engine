// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

Texture2D<float> scene_depth_texture : register(t2);
SamplerState scene_depth_sampler : register(s3);

cbuffer VolumetricFogConstants : register(b5) {
    uint froxel_width;
    uint froxel_height;
    uint froxel_depth_slices;
    float range_m;
    float density;
    float anisotropy;
    float temporal_history_weight;
    float albedo_r;
    float albedo_g;
    float albedo_b;
};

struct VolumetricFogContractSample {
    float density;
    float3 albedo;
    float phase;
};

float volumetric_fog_contract_depth_fraction(uint z) {
    return saturate((float(z) + 0.5) / max(float(froxel_depth_slices), 1.0));
}

float volumetric_fog_contract_phase(float cos_theta) {
    const float g = clamp(anisotropy, -0.99, 0.99);
    const float g2 = g * g;
    return (1.0 - g2) / max(pow(1.0 + g2 - (2.0 * g * cos_theta), 1.5), 0.0001);
}

VolumetricFogContractSample volumetric_fog_contract_sample(uint3 froxel_id) {
    const float depth_fraction = volumetric_fog_contract_depth_fraction(froxel_id.z);
    const float distance_m = depth_fraction * max(range_m, 0.0001);
    const float extinction = 1.0 - exp2(-max(density, 0.0) * distance_m);

    VolumetricFogContractSample sample;
    sample.density = saturate(extinction);
    sample.albedo = float3(albedo_r, albedo_g, albedo_b);
    sample.phase = volumetric_fog_contract_phase(0.5);
    return sample;
}

void volumetric_fog_cs_contract(uint3 dispatch_thread_id) {
    if (dispatch_thread_id.x >= froxel_width || dispatch_thread_id.y >= froxel_height ||
        dispatch_thread_id.z >= froxel_depth_slices) {
        return;
    }

    const VolumetricFogContractSample sample = volumetric_fog_contract_sample(dispatch_thread_id);
    const float history = saturate(temporal_history_weight);
    const float contract_value = lerp(sample.density, sample.phase, history);
    (void)contract_value;
}

#if defined(MK_VOLUMETRIC_FOG_COMPUTE)
[numthreads(4, 4, 4)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    volumetric_fog_cs_contract(dispatch_thread_id);
}
#else
// The shared contract can also be compiled by selecting volumetric_fog_cs_contract explicitly.
#endif
