// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary
//
// ScenePbrFrame uses register(b6) to match material descriptor binding index 6.

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

float4 ps_main(VsOut input) : SV_Target {
    float4 sampled = base_color_texture.Sample(base_color_sampler, input.uv);
    if (shading_lit < 0.5) {
        float3 color = sampled.rgb * base_color.rgb + emissive.rgb;
        return float4(saturate(color), sampled.a * base_color.a);
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
    float3 direct = (diffuse + specular) * radiance * n_dot_l;
    float3 ambient = ambient_pad.rgb * albedo;
    float3 color = saturate(direct + ambient + emissive.rgb);
    return float4(color, sampled.a * base_color.a);
}
