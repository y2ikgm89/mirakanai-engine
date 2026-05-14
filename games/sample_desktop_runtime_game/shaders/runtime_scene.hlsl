// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

// Cook-Torrance GGX metallic-roughness shading aligned with common glTF-style BRDF practice.
// Constant buffers match `mirakana::pack_runtime_material_factors` and `mirakana::pack_scene_pbr_frame_gpu` layouts.
// `ScenePbrFrame` uses register(b6) to match material descriptor binding index 6 (D3D12 root `BaseShaderRegister`).

cbuffer MaterialFactors : register(b0) {
    float4 base_color;
    float3 emissive;
    float metallic;
    float roughness;
    float shading_lit;
    float3 _pad_material;
};

cbuffer ScenePbrFrame : register(b6) {
    row_major float4x4 clip_from_world;
    row_major float4x4 world_from_object;
    float4 camera_position_aspect;
    float4 light_dir_intensity;
    float4 light_color_pad;
    float4 ambient_pad;
};

Texture2D base_color_texture : register(t1);
SamplerState base_color_sampler : register(s16);

// Directional shadow receiver resources: default `space1` (descriptor set index 1) for `[material, shadow]` layouts.
// When GPU skinning inserts a joint palette set at index 1, the same entry points are recompiled with
// `MK_SAMPLE_SKINNED_SCENE_SHADOW_RECEIVER_PS` so shadow sampling moves to `space2` (descriptor set index 2).
#ifndef MK_SAMPLE_SKINNED_SCENE_SHADOW_RECEIVER_PS
#define MK_SAMPLE_SKINNED_SCENE_SHADOW_RECEIVER_PS 0
#endif

#if MK_SAMPLE_SKINNED_SCENE_SHADOW_RECEIVER_PS
Texture2D<float> shadow_depth_texture : register(t0, space2);
SamplerState shadow_sampler : register(s1, space2);

cbuffer ShadowReceiverConstants : register(b2, space2) {
    uint shadow_cascade_count;
    uint3 shadow_receiver_pad0;
    float4 shadow_cascade_splits0;
    float4 shadow_cascade_splits1;
    float4 shadow_cascade_splits2;
    row_major float4x4 shadow_clip_from_world_cascades[8];
    row_major float4x4 shadow_camera_view_from_world;
    uint4 shadow_receiver_pad1[8];
};
#else
Texture2D<float> shadow_depth_texture : register(t0, space1);
SamplerState shadow_sampler : register(s1, space1);

cbuffer ShadowReceiverConstants : register(b2, space1) {
    uint shadow_cascade_count;
    uint3 shadow_receiver_pad0;
    float4 shadow_cascade_splits0;
    float4 shadow_cascade_splits1;
    float4 shadow_cascade_splits2;
    row_major float4x4 shadow_clip_from_world_cascades[8];
    row_major float4x4 shadow_camera_view_from_world;
    uint4 shadow_receiver_pad1[8];
};
#endif

static const float k_pi = 3.14159265358979323846;

struct VsIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct VsOut {
    float4 position : SV_Position;
    float3 world_position : TEXCOORD1;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

VsOut vs_main(VsIn input) {
    VsOut output;
    float4 world_pos = mul(float4(input.position, 1.0), world_from_object);
    output.position = mul(world_pos, clip_from_world);
    output.world_position = world_pos.xyz;
    float3 world_n = mul(float4(input.normal, 0.0), world_from_object).xyz;
    output.normal = normalize(world_n);
    output.uv = input.uv;
    return output;
}

float3 fresnel_schlick(float cos_theta, float3 f0) {
    return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);
}

float distribution_ggx(float n_dot_h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = (n_dot_h * n_dot_h) * (a2 - 1.0) + 1.0;
    return a2 / max(k_pi * denom * denom, 1e-6);
}

float geometry_schlick_ggx(float n_dot_x, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return n_dot_x / max(n_dot_x * (1.0 - k) + k, 1e-6);
}

float geometry_smith(float n_dot_v, float n_dot_l, float roughness) {
    return geometry_schlick_ggx(n_dot_v, roughness) * geometry_schlick_ggx(n_dot_l, roughness);
}

float3 evaluate_lit_color(VsOut input, float4 sampled, float shadow_scale) {
    if (shading_lit < 0.5) {
        return sampled.rgb * base_color.rgb + emissive.rgb;
    }

    float3 albedo = sampled.rgb * base_color.rgb;
    float3 n = normalize(input.normal);
    float3 v = normalize(camera_position_aspect.xyz - input.world_position);
    float3 l = normalize(light_dir_intensity.xyz);
    float3 h = normalize(v + l);

    float n_dot_v = saturate(dot(n, v));
    float n_dot_l = saturate(dot(n, l));
    float n_dot_h = saturate(dot(n, h));
    float h_dot_v = saturate(dot(h, v));

    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 fresnel = fresnel_schlick(h_dot_v, f0);

    float alpha = max(roughness, 0.04);
    float d = distribution_ggx(n_dot_h, alpha);
    float g = geometry_smith(n_dot_v, n_dot_l, alpha);

    float3 specular = (d * g) * fresnel / max(4.0 * n_dot_v * n_dot_l, 1e-4);
    float3 kd = (1.0 - fresnel) * (1.0 - metallic);
    float3 diffuse = kd * albedo / k_pi;

    float3 radiance = light_color_pad.rgb * light_dir_intensity.w;
    float3 direct = (diffuse + specular) * radiance * n_dot_l * shadow_scale;
    float3 ambient = ambient_pad.rgb * albedo;
    return direct + ambient + emissive.rgb;
}

float4 ps_main(VsOut input) : SV_Target {
    float4 sampled = base_color_texture.Sample(base_color_sampler, input.uv);
    float3 color = evaluate_lit_color(input, sampled, 1.0);
    return float4(saturate(color), sampled.a * base_color.a);
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
    // Depth-buffer comparison uses post-projective z/w in [0, 1] (D3D12 and Vulkan NDC depth), not OpenGL [-1, 1] z.
    const float receiver_depth = saturate(light_clip.z * inv_w);
    const float2 tile_uv = shadow_tile_uv_from_clip(light_clip);
    const float2 atlas_uv = shadow_atlas_uv(cascade, tile_uv);
    float4 sampled = base_color_texture.Sample(base_color_sampler, input.uv);
    const float shadow_factor = lerp(1.0, 0.42, directional_shadow_pcf_3x3(atlas_uv, receiver_depth));
    float3 color = evaluate_lit_color(input, sampled, shadow_factor);
    return float4(saturate(color), sampled.a * base_color.a);
}

// GPU skinning v1: joint palette is bound at descriptor **set index 1** (maps to `RegisterSpace == 1` in the D3D12
// RHI backend). This entry point pairs with a root signature that places the joint palette at set index 1 after the
// material/scene set (set index 0). Optional sets (for example directional shadow receiver) follow at higher indices.
#define SKINNED_MAX_JOINTS 256
cbuffer JointPalette : register(b0, space1) {
    row_major float4x4 joint_from_rest[SKINNED_MAX_JOINTS];
};

struct VsSkinnedIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
    uint4 joint_indices : BLENDINDICES;
    float4 joint_weights : BLENDWEIGHT;
};

VsOut vs_skinned(VsSkinnedIn input) {
    const float4 bind_pos = float4(input.position, 1.0);
    const float4 model_pos = mul(bind_pos, joint_from_rest[input.joint_indices.x]) * input.joint_weights.x +
                               mul(bind_pos, joint_from_rest[input.joint_indices.y]) * input.joint_weights.y +
                               mul(bind_pos, joint_from_rest[input.joint_indices.z]) * input.joint_weights.z +
                               mul(bind_pos, joint_from_rest[input.joint_indices.w]) * input.joint_weights.w;
    VsOut output;
    const float4 world_pos = mul(model_pos, world_from_object);
    output.position = mul(world_pos, clip_from_world);
    output.world_position = world_pos.xyz;
    const float3 world_n = mul(float4(input.normal, 0.0), world_from_object).xyz;
    output.normal = normalize(world_n);
    output.uv = input.uv;
    return output;
}
