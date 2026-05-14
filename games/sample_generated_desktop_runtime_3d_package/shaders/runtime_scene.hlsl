// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

cbuffer MaterialFactors : register(b0) {
    float4 base_color;
    float3 emissive;
    float metallic;
    float roughness;
};

Texture2D base_color_texture : register(t1);
SamplerState base_color_sampler : register(s16);
#ifndef MK_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS
#define MK_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS 0
#endif
#if MK_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS
Texture2D<float> shadow_depth_texture : register(t0, space2);
SamplerState shadow_sampler : register(s1, space2);
#else
Texture2D<float> shadow_depth_texture : register(t0, space1);
SamplerState shadow_sampler : register(s1, space1);
#endif
RWByteAddressBuffer compute_base_vertices : register(u0, space0);
RWByteAddressBuffer compute_position_deltas : register(u1, space0);
RWByteAddressBuffer compute_output_positions : register(u3, space0);
RWByteAddressBuffer compute_normal_deltas : register(u4, space0);
RWByteAddressBuffer compute_tangent_deltas : register(u5, space0);
RWByteAddressBuffer compute_output_normals : register(u6, space0);
RWByteAddressBuffer compute_output_tangents : register(u7, space0);
RWByteAddressBuffer morph_position_deltas : register(u0, space1);
RWByteAddressBuffer morph_normal_deltas : register(u2, space1);
RWByteAddressBuffer morph_tangent_deltas : register(u3, space1);

cbuffer ComputeMorphWeights : register(b2, space0) {
    float compute_morph_weights[64];
};

cbuffer MorphWeights : register(b1, space1) {
    float morph_weights[64];
};

#if MK_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS
cbuffer ShadowReceiverConstants : register(b2, space2) {
#else
cbuffer ShadowReceiverConstants : register(b2, space1) {
#endif
    uint shadow_cascade_count;
    uint3 shadow_receiver_pad0;
    float4 shadow_cascade_splits0;
    float4 shadow_cascade_splits1;
    float4 shadow_cascade_splits2;
    row_major float4x4 shadow_clip_from_world_cascades[8];
    row_major float4x4 shadow_camera_view_from_world;
    uint4 shadow_receiver_pad1[8];
};

#define SKINNED_MAX_JOINTS 256
cbuffer JointPalette : register(b0, space1) {
    row_major float4x4 joint_from_rest[SKINNED_MAX_JOINTS];
};

struct VsIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct ComputeMorphVsIn {
    float3 position : POSITION;
};

struct ComputeMorphTangentFrameVsIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct ComputeMorphSkinnedVsIn {
    float3 position : POSITION;
    uint4 joint_indices : BLENDINDICES;
    float4 joint_weights : BLENDWEIGHT;
};

struct VsOut {
    float4 position : SV_Position;
    float3 world_position : TEXCOORD1;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
};

VsOut vs_main(VsIn input) {
    VsOut output;
    output.position = float4(input.position.xy, input.position.z, 1.0);
    output.world_position = input.position;
    output.normal = normalize(input.normal);
    output.uv = input.uv;
    output.tangent = normalize(input.tangent.xyz);
    return output;
}

VsOut vs_morph(VsIn input, uint vertex_id : SV_VertexID) {
    VsOut output;
    uint delta_offset = vertex_id * 12;
    float weight = morph_weights[0];
    float3 position_delta = asfloat(morph_position_deltas.Load3(delta_offset));
    float3 normal_delta = asfloat(morph_normal_deltas.Load3(delta_offset));
    float3 tangent_delta = asfloat(morph_tangent_deltas.Load3(delta_offset));
    float3 morphed_position = input.position + position_delta * weight;
    float3 morphed_tangent = normalize(input.tangent.xyz + tangent_delta * weight);
    float3 morphed_normal = normalize(input.normal + normal_delta * weight);
    output.position = float4(morphed_position.xy, morphed_position.z, 1.0);
    output.world_position = morphed_position;
    output.normal = morphed_normal;
    output.uv = input.uv;
    output.tangent = morphed_tangent;
    return output;
}

VsOut vs_compute_morph(ComputeMorphVsIn input, uint vertex_id : SV_VertexID) {
    float2 uvs[3] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.5, 1.0),
    };

    VsOut output;
    output.position = float4(input.position.xy, input.position.z, 1.0);
    output.world_position = input.position;
    output.normal = float3(0.0, 0.0, 1.0);
    output.uv = uvs[vertex_id];
    output.tangent = float3(1.0, 0.0, 0.0);
    return output;
}

VsOut vs_compute_morph_tangent_frame(ComputeMorphTangentFrameVsIn input, uint vertex_id : SV_VertexID) {
    float2 uvs[3] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.5, 1.0),
    };

    VsOut output;
    output.position = float4(input.position.xy, input.position.z, 1.0);
    output.world_position = input.position;
    output.normal = normalize(input.normal);
    output.uv = uvs[vertex_id];
    output.tangent = normalize(input.tangent);
    return output;
}

VsOut vs_compute_morph_skinned(ComputeMorphSkinnedVsIn input, uint vertex_id : SV_VertexID) {
    float2 uvs[3] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.5, 1.0),
    };

    const float4 bind_pos = float4(input.position, 1.0);
    const float4 model_pos = mul(bind_pos, joint_from_rest[input.joint_indices.x]) * input.joint_weights.x +
                             mul(bind_pos, joint_from_rest[input.joint_indices.y]) * input.joint_weights.y +
                             mul(bind_pos, joint_from_rest[input.joint_indices.z]) * input.joint_weights.z +
                             mul(bind_pos, joint_from_rest[input.joint_indices.w]) * input.joint_weights.w;

    VsOut output;
    output.position = float4(model_pos.xyz, 1.0);
    output.world_position = model_pos.xyz;
    output.normal = float3(0.0, 0.0, 1.0);
    output.uv = uvs[vertex_id];
    output.tangent = float3(1.0, 0.0, 0.0);
    return output;
}

[numthreads(1, 1, 1)]
void cs_compute_morph_position(uint3 dispatch_id : SV_DispatchThreadID) {
    uint vertex_id = dispatch_id.x;
    uint source_offset = vertex_id * 48;
    uint position_offset = vertex_id * 12;
    float3 base_position = asfloat(compute_base_vertices.Load3(source_offset));
    float3 position_delta = asfloat(compute_position_deltas.Load3(position_offset));
    float3 morphed_position = base_position + position_delta * compute_morph_weights[0];
    compute_output_positions.Store3(position_offset, asuint(morphed_position));
}

[numthreads(1, 1, 1)]
void cs_compute_morph_tangent_frame(uint3 dispatch_id : SV_DispatchThreadID) {
    uint vertex_id = dispatch_id.x;
    uint source_offset = vertex_id * 48;
    uint tangent_offset = source_offset + 32;
    uint position_offset = vertex_id * 12;
    float weight = compute_morph_weights[0];
    float3 base_position = asfloat(compute_base_vertices.Load3(source_offset));
    float3 base_normal = asfloat(compute_base_vertices.Load3(source_offset + 12));
    float3 base_tangent = asfloat(compute_base_vertices.Load3(tangent_offset));
    float3 position_delta = asfloat(compute_position_deltas.Load3(position_offset));
    float3 normal_delta = asfloat(compute_normal_deltas.Load3(position_offset));
    float3 tangent_delta = asfloat(compute_tangent_deltas.Load3(position_offset));
    float3 morphed_position = base_position + position_delta * weight;
    float3 morphed_normal = normalize(base_normal + normal_delta * weight);
    float3 morphed_tangent = normalize(base_tangent + tangent_delta * weight);
    compute_output_positions.Store3(position_offset, asuint(morphed_position));
    compute_output_normals.Store3(position_offset, asuint(morphed_normal));
    compute_output_tangents.Store3(position_offset, asuint(morphed_tangent));
}

[numthreads(1, 1, 1)]
void cs_compute_morph_skinned_position(uint3 dispatch_id : SV_DispatchThreadID) {
    uint vertex_id = dispatch_id.x;
    uint source_offset = vertex_id * 12;
    uint position_offset = source_offset;
    float3 base_position = asfloat(compute_base_vertices.Load3(source_offset));
    float3 position_delta = asfloat(compute_position_deltas.Load3(position_offset));
    float3 morphed_position = base_position + position_delta * compute_morph_weights[0];
    compute_output_positions.Store3(position_offset, asuint(morphed_position));
}

float3 evaluate_lit_color(VsOut input, float4 sampled, float shadow_scale) {
    float3 normal = normalize(input.normal);
    float3 tangent = normalize(input.tangent);
    float3 light_direction = normalize(float3(0.25, 0.35, 0.9));
    float direct_light = saturate(dot(normal, light_direction)) * shadow_scale;
    float tangent_light = 0.65 + 0.35 * saturate(abs(tangent.y));
    return sampled.rgb * base_color.rgb * (0.18 + 0.82 * direct_light) * tangent_light + emissive.rgb;
}

float4 ps_main(VsOut input) : SV_Target {
    float4 sampled = base_color_texture.Sample(base_color_sampler, input.uv);
    return float4(saturate(evaluate_lit_color(input, sampled, 1.0)), sampled.a * base_color.a);
}

float shadow_split_distance(uint index) {
    float splits[12] = {shadow_cascade_splits0.x, shadow_cascade_splits0.y, shadow_cascade_splits0.z,
                        shadow_cascade_splits0.w, shadow_cascade_splits1.x, shadow_cascade_splits1.y,
                        shadow_cascade_splits1.z, shadow_cascade_splits1.w, shadow_cascade_splits2.x,
                        shadow_cascade_splits2.y, shadow_cascade_splits2.z, shadow_cascade_splits2.w};
    return splits[index];
}

uint select_shadow_cascade(float view_depth_along_forward) {
    if (shadow_cascade_count <= 1u) {
        return 0u;
    }
    const uint last = shadow_cascade_count - 1u;
    for (uint i = 0u; i < last; ++i) {
        if (view_depth_along_forward < shadow_split_distance(i + 1u)) {
            return i;
        }
    }
    return last;
}

float shadow_view_depth_along_forward(float3 world_position) {
    return world_position.x * shadow_camera_view_from_world._m20 + world_position.y * shadow_camera_view_from_world._m21 +
           world_position.z * shadow_camera_view_from_world._m22 + shadow_camera_view_from_world._m23;
}

float2 shadow_tile_uv_from_clip(float4 light_clip) {
    const float inv_w = 1.0 / max(abs(light_clip.w), 1e-6);
    const float2 ndc = light_clip.xy * inv_w;
    const float u = ndc.x * 0.5 + 0.5;
    const float v = 0.5 - ndc.y * 0.5;
    return float2(u, v);
}

float2 shadow_atlas_uv(uint cascade_index, float2 tile_uv) {
    const float inv_cc = 1.0 / max(float(shadow_cascade_count), 1.0);
    return float2((float(cascade_index) + tile_uv.x) * inv_cc, tile_uv.y);
}

float directional_shadow_pcf_3x3(float2 atlas_uv, float receiver_depth) {
    uint width = 1;
    uint height = 1;
    shadow_depth_texture.GetDimensions(width, height);

    float safe_width = width == 0 ? 1.0 : (float)width;
    float safe_height = height == 0 ? 1.0 : (float)height;
    float2 texel = 1.0 / float2(safe_width, safe_height);
    float occlusion = 0.0;

    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            float sample_depth =
                shadow_depth_texture.SampleLevel(shadow_sampler, atlas_uv + float2((float)x, (float)y) * texel, 0.0);
            occlusion += sample_depth + 0.003 < receiver_depth ? 1.0 : 0.0;
        }
    }
    return occlusion / 9.0;
}

float4 ps_shadow_receiver(VsOut input) : SV_Target {
    const uint cascade = select_shadow_cascade(shadow_view_depth_along_forward(input.world_position));
    const float4 light_clip =
        mul(float4(input.world_position, 1.0), shadow_clip_from_world_cascades[cascade]);
    const float inv_w = 1.0 / max(abs(light_clip.w), 1e-6);
    const float receiver_depth = saturate(light_clip.z * inv_w);
    const float2 tile_uv = shadow_tile_uv_from_clip(light_clip);
    const float2 atlas_uv = shadow_atlas_uv(cascade, tile_uv);
    float4 sampled = base_color_texture.Sample(base_color_sampler, input.uv);
    const float shadow_factor = lerp(1.0, 0.42, directional_shadow_pcf_3x3(atlas_uv, receiver_depth));
    return float4(saturate(evaluate_lit_color(input, sampled, shadow_factor)), sampled.a * base_color.a);
}
