// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/material.hpp"
#include "mirakana/renderer/rhi_frame_renderer.hpp"
#include "mirakana/renderer/rhi_postprocess_frame_renderer.hpp"
#include "mirakana/renderer/rhi_viewport_surface.hpp"
#include "mirakana/renderer/shadow_map.hpp"
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class HiddenTestWindow final {
  public:
    HiddenTestWindow() {
        instance_ = GetModuleHandleW(nullptr);

        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.lpfnWndProc = DefWindowProcW;
        window_class.hInstance = instance_;
        window_class.lpszClassName = class_name;
        registered_ = RegisterClassExW(&window_class) != 0;

        hwnd_ = CreateWindowExW(0, class_name, L"GameEngineD3D12TestWindow", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                CW_USEDEFAULT, 128, 128, nullptr, nullptr, instance_, nullptr);
    }

    HiddenTestWindow(const HiddenTestWindow&) = delete;
    HiddenTestWindow& operator=(const HiddenTestWindow&) = delete;

    ~HiddenTestWindow() {
        if (hwnd_ != nullptr) {
            DestroyWindow(hwnd_);
        }
        if (registered_) {
            UnregisterClassW(class_name, instance_);
        }
    }

    [[nodiscard]] bool valid() const noexcept {
        return hwnd_ != nullptr;
    }

    [[nodiscard]] HWND hwnd() const noexcept {
        return hwnd_;
    }

  private:
    static constexpr const wchar_t* class_name{L"GameEngineD3D12TestWindowClass"};
    HINSTANCE instance_{nullptr};
    HWND hwnd_{nullptr};
    bool registered_{false};
};

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_shader(const char* source, const char* entry_point,
                                                              const char* target) {
    Microsoft::WRL::ComPtr<ID3DBlob> bytecode;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry_point, target,
                                      D3DCOMPILE_ENABLE_STRICTNESS, 0, &bytecode, &errors);
    MK_REQUIRE(SUCCEEDED(result));
    return bytecode;
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_triangle_vertex_shader() {
    return compile_shader(
        "struct VsOut {"
        "  float4 position : SV_Position;"
        "  float4 color : COLOR0;"
        "};"
        "VsOut vs_main(uint vertex_id : SV_VertexID) {"
        "  float2 positions[3] = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };"
        "  float4 colors[3] = { float4(1.0, 0.0, 0.0, 1.0), float4(0.0, 1.0, 0.0, 1.0), float4(0.0, 0.0, 1.0, 1.0) };"
        "  VsOut output;"
        "  output.position = float4(positions[vertex_id], 0.0, 1.0);"
        "  output.color = colors[vertex_id];"
        "  return output;"
        "}",
        "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_triangle_pixel_shader() {
    return compile_shader("float4 ps_main(float4 position : SV_Position, float4 color : COLOR0) : SV_Target {"
                          "  return color;"
                          "}",
                          "ps_main", "ps_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_uav_write_compute_shader() {
    return compile_shader("RWByteAddressBuffer output_buffer : register(u0);"
                          "[numthreads(1, 1, 1)]"
                          "void cs_main(uint3 dispatch_id : SV_DispatchThreadID) {"
                          "  output_buffer.Store(dispatch_id.x * 4, 0xA0B0C000u + dispatch_id.x);"
                          "}",
                          "cs_main", "cs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_runtime_morph_position_compute_shader() {
    return compile_shader("RWByteAddressBuffer base_positions : register(u0);"
                          "RWByteAddressBuffer position_deltas : register(u1);"
                          "cbuffer MorphWeights : register(b2) {"
                          "  float weight0;"
                          "  float weight1;"
                          "};"
                          "RWByteAddressBuffer output_positions : register(u3);"
                          "[numthreads(1, 1, 1)]"
                          "void cs_main(uint3 dispatch_id : SV_DispatchThreadID) {"
                          "  const uint vertex_count = 3;"
                          "  const uint vertex = dispatch_id.x;"
                          "  const uint base_offset = vertex * 12;"
                          "  const uint delta0_offset = vertex * 12;"
                          "  const uint delta1_offset = (vertex_count + vertex) * 12;"
                          "  float3 base = float3("
                          "      asfloat(base_positions.Load(base_offset + 0)),"
                          "      asfloat(base_positions.Load(base_offset + 4)),"
                          "      asfloat(base_positions.Load(base_offset + 8)));"
                          "  float3 delta0 = float3("
                          "      asfloat(position_deltas.Load(delta0_offset + 0)),"
                          "      asfloat(position_deltas.Load(delta0_offset + 4)),"
                          "      asfloat(position_deltas.Load(delta0_offset + 8)));"
                          "  float3 delta1 = float3("
                          "      asfloat(position_deltas.Load(delta1_offset + 0)),"
                          "      asfloat(position_deltas.Load(delta1_offset + 4)),"
                          "      asfloat(position_deltas.Load(delta1_offset + 8)));"
                          "  float3 morphed = base + (delta0 * weight0) + (delta1 * weight1);"
                          "  output_positions.Store(base_offset + 0, asuint(morphed.x));"
                          "  output_positions.Store(base_offset + 4, asuint(morphed.y));"
                          "  output_positions.Store(base_offset + 8, asuint(morphed.z));"
                          "}",
                          "cs_main", "cs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob>
compile_runtime_morph_position_output_slot_compute_shader(std::uint32_t output_slot_index = 1) {
    MK_REQUIRE(output_slot_index < 2);
    const std::string output_slot = output_slot_index == 0 ? "output_slot0" : "output_slot1";
    const std::string source = "RWByteAddressBuffer base_positions : register(u0);"
                               "RWByteAddressBuffer position_deltas : register(u1);"
                               "cbuffer MorphWeights : register(b2) {"
                               "  float weight0;"
                               "  float weight1;"
                               "};"
                               "RWByteAddressBuffer output_slot0 : register(u3);"
                               "RWByteAddressBuffer output_slot1 : register(u4);"
                               "[numthreads(1, 1, 1)]"
                               "void cs_main(uint3 dispatch_id : SV_DispatchThreadID) {"
                               "  const uint vertex_count = 3;"
                               "  const uint vertex = dispatch_id.x;"
                               "  const uint base_offset = vertex * 12;"
                               "  const uint delta0_offset = vertex * 12;"
                               "  const uint delta1_offset = (vertex_count + vertex) * 12;"
                               "  float3 base = float3("
                               "      asfloat(base_positions.Load(base_offset + 0)),"
                               "      asfloat(base_positions.Load(base_offset + 4)),"
                               "      asfloat(base_positions.Load(base_offset + 8)));"
                               "  float3 delta0 = float3("
                               "      asfloat(position_deltas.Load(delta0_offset + 0)),"
                               "      asfloat(position_deltas.Load(delta0_offset + 4)),"
                               "      asfloat(position_deltas.Load(delta0_offset + 8)));"
                               "  float3 delta1 = float3("
                               "      asfloat(position_deltas.Load(delta1_offset + 0)),"
                               "      asfloat(position_deltas.Load(delta1_offset + 4)),"
                               "      asfloat(position_deltas.Load(delta1_offset + 8)));"
                               "  float3 morphed = base + (delta0 * weight0) + (delta1 * weight1);" +
                               output_slot + ".Store(base_offset + 0, asuint(morphed.x));" + output_slot +
                               ".Store(base_offset + 4, asuint(morphed.y));" + output_slot +
                               ".Store(base_offset + 8, asuint(morphed.z));"
                               "}";
    return compile_shader(source.c_str(), "cs_main", "cs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_runtime_morph_tangent_frame_compute_shader() {
    return compile_shader("RWByteAddressBuffer base_vertices : register(u0);"
                          "RWByteAddressBuffer position_deltas : register(u1);"
                          "cbuffer MorphWeights : register(b2) {"
                          "  float weight0;"
                          "  float weight1;"
                          "};"
                          "RWByteAddressBuffer output_positions : register(u3);"
                          "RWByteAddressBuffer normal_deltas : register(u4);"
                          "RWByteAddressBuffer tangent_deltas : register(u5);"
                          "RWByteAddressBuffer output_normals : register(u6);"
                          "RWByteAddressBuffer output_tangents : register(u7);"
                          "[numthreads(1, 1, 1)]"
                          "void cs_main(uint3 dispatch_id : SV_DispatchThreadID) {"
                          "  const uint vertex_count = 3;"
                          "  const uint vertex = dispatch_id.x;"
                          "  const uint base_vertex_offset = vertex * 48;"
                          "  const uint output_offset = vertex * 12;"
                          "  const uint delta0_offset = vertex * 12;"
                          "  const uint delta1_offset = (vertex_count + vertex) * 12;"
                          "  float3 base_position = asfloat(base_vertices.Load3(base_vertex_offset + 0));"
                          "  float3 base_normal = asfloat(base_vertices.Load3(base_vertex_offset + 12));"
                          "  float3 base_tangent = asfloat(base_vertices.Load3(base_vertex_offset + 32));"
                          "  float3 position_delta0 = asfloat(position_deltas.Load3(delta0_offset));"
                          "  float3 position_delta1 = asfloat(position_deltas.Load3(delta1_offset));"
                          "  float3 normal_delta0 = asfloat(normal_deltas.Load3(delta0_offset));"
                          "  float3 normal_delta1 = asfloat(normal_deltas.Load3(delta1_offset));"
                          "  float3 tangent_delta0 = asfloat(tangent_deltas.Load3(delta0_offset));"
                          "  float3 tangent_delta1 = asfloat(tangent_deltas.Load3(delta1_offset));"
                          "  float3 morphed_position = base_position + position_delta0 * weight0 +"
                          "                            position_delta1 * weight1;"
                          "  float3 morphed_normal = normalize(base_normal + normal_delta0 * weight0 +"
                          "                                   normal_delta1 * weight1);"
                          "  float3 morphed_tangent = normalize(base_tangent + tangent_delta0 * weight0 +"
                          "                                    tangent_delta1 * weight1);"
                          "  output_positions.Store(output_offset + 0, asuint(morphed_position.x));"
                          "  output_positions.Store(output_offset + 4, asuint(morphed_position.y));"
                          "  output_positions.Store(output_offset + 8, asuint(morphed_position.z));"
                          "  output_normals.Store(output_offset + 0, asuint(morphed_normal.x));"
                          "  output_normals.Store(output_offset + 4, asuint(morphed_normal.y));"
                          "  output_normals.Store(output_offset + 8, asuint(morphed_normal.z));"
                          "  output_tangents.Store(output_offset + 0, asuint(morphed_tangent.x));"
                          "  output_tangents.Store(output_offset + 4, asuint(morphed_tangent.y));"
                          "  output_tangents.Store(output_offset + 8, asuint(morphed_tangent.z));"
                          "}",
                          "cs_main", "cs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_depth_order_vertex_shader() {
    return compile_shader("struct VsOut {"
                          "  float4 position : SV_Position;"
                          "  float4 color : COLOR0;"
                          "};"
                          "VsOut vs_main(uint vertex_id : SV_VertexID) {"
                          "  float2 positions[6] = {"
                          "    float2(0.0, 0.75), float2(0.75, -0.75), float2(-0.75, -0.75),"
                          "    float2(0.0, 0.75), float2(0.75, -0.75), float2(-0.75, -0.75)"
                          "  };"
                          "  float depths[6] = { 0.25, 0.25, 0.25, 0.75, 0.75, 0.75 };"
                          "  float4 colors[6] = {"
                          "    float4(0.0, 1.0, 0.0, 1.0), float4(0.0, 1.0, 0.0, 1.0),"
                          "    float4(0.0, 1.0, 0.0, 1.0), float4(1.0, 0.0, 0.0, 1.0),"
                          "    float4(1.0, 0.0, 0.0, 1.0), float4(1.0, 0.0, 0.0, 1.0)"
                          "  };"
                          "  VsOut output;"
                          "  output.position = float4(positions[vertex_id], depths[vertex_id], 1.0);"
                          "  output.color = colors[vertex_id];"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_half_shadow_depth_vertex_shader() {
    return compile_shader("struct VsOut {"
                          "  float4 position : SV_Position;"
                          "  float4 color : COLOR0;"
                          "};"
                          "VsOut vs_main(uint vertex_id : SV_VertexID) {"
                          "  float2 positions[6] = {"
                          "    float2(-1.0, -1.0), float2(0.0, -1.0), float2(-1.0, 1.0),"
                          "    float2(-1.0, 1.0), float2(0.0, -1.0), float2(0.0, 1.0)"
                          "  };"
                          "  VsOut output;"
                          "  output.position = float4(positions[vertex_id], 0.35, 1.0);"
                          "  output.color = float4(0.0, 1.0, 0.0, 1.0);"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_solid_orange_pixel_shader() {
    return compile_shader("float4 ps_main(float4 position : SV_Position, float4 color : COLOR0) : SV_Target {"
                          "  return float4(1.0, 0.25, 0.0, 1.0);"
                          "}",
                          "ps_main", "ps_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_position_input_vertex_shader() {
    return compile_shader("struct VsIn {"
                          "  float3 position : POSITION;"
                          "};"
                          "struct VsOut {"
                          "  float4 position : SV_Position;"
                          "};"
                          "VsOut vs_main(VsIn input) {"
                          "  VsOut output;"
                          "  output.position = float4(input.position.xy, input.position.z, 1.0);"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_runtime_scene_material_vertex_shader() {
    return compile_shader("struct VsIn {"
                          "  float3 position : POSITION;"
                          "};"
                          "struct VsOut {"
                          "  float4 position : SV_Position;"
                          "  float2 uv : TEXCOORD0;"
                          "};"
                          "VsOut vs_main(VsIn input) {"
                          "  VsOut output;"
                          "  output.position = float4(input.position.xy, input.position.z, 1.0);"
                          "  output.uv = float2(0.5, 0.5);"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_skinned_position_vertex_shader() {
    return compile_shader("struct VsIn {"
                          "  float3 position : POSITION;"
                          "  float3 normal : NORMAL;"
                          "  float2 uv : TEXCOORD0;"
                          "  float4 tangent : TANGENT;"
                          "  uint4 joint_indices : BLENDINDICES;"
                          "  float4 joint_weights : BLENDWEIGHT;"
                          "};"
                          "struct VsOut {"
                          "  float4 position : SV_Position;"
                          "};"
                          "cbuffer JointPalette : register(b0, space1) {"
                          "  float4 joint0_row0;"
                          "  float4 joint0_row1;"
                          "  float4 joint0_row2;"
                          "  float4 joint0_row3;"
                          "};"
                          "VsOut vs_skinned(VsIn input) {"
                          "  VsOut output;"
                          "  float2 offset = (input.joint_indices.x == 0 ? joint0_row3.xy : float2(0.0, 0.0)) *"
                          "                  input.joint_weights.x;"
                          "  output.position = float4(input.position.xy + offset, input.position.z, 1.0);"
                          "  return output;"
                          "}",
                          "vs_skinned", "vs_5_1");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_compute_morph_skinned_position_vertex_shader() {
    return compile_shader("struct VsIn {"
                          "  float3 position : POSITION;"
                          "  uint4 joint_indices : BLENDINDICES;"
                          "  float4 joint_weights : BLENDWEIGHT;"
                          "};"
                          "struct VsOut {"
                          "  float4 position : SV_Position;"
                          "};"
                          "cbuffer JointPalette : register(b0, space1) {"
                          "  float4 joint0_row0;"
                          "  float4 joint0_row1;"
                          "  float4 joint0_row2;"
                          "  float4 joint0_row3;"
                          "};"
                          "VsOut vs_compute_morph_skinned(VsIn input) {"
                          "  VsOut output;"
                          "  float2 offset = (input.joint_indices.x == 0 ? joint0_row3.xy : float2(0.0, 0.0)) *"
                          "                  input.joint_weights.x;"
                          "  output.position = float4(input.position.xy + offset, input.position.z, 1.0);"
                          "  return output;"
                          "}",
                          "vs_compute_morph_skinned", "vs_5_1");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_morph_position_vertex_shader() {
    return compile_shader("struct VsIn {"
                          "  float3 position : POSITION;"
                          "};"
                          "struct VsOut {"
                          "  float4 position : SV_Position;"
                          "};"
                          "RWByteAddressBuffer MorphPositionDeltas : register(u0, space1);"
                          "cbuffer MorphWeights : register(b1, space1) {"
                          "  float weight0;"
                          "  float3 _padding0;"
                          "};"
                          "VsOut vs_morph(VsIn input, uint vertex_id : SV_VertexID) {"
                          "  VsOut output;"
                          "  float3 delta = asfloat(MorphPositionDeltas.Load3(vertex_id * 12));"
                          "  output.position = float4(input.position + (delta * weight0), 1.0);"
                          "  return output;"
                          "}",
                          "vs_morph", "vs_5_1");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_morph_normal_vertex_shader() {
    return compile_shader("struct VsIn {"
                          "  float3 position : POSITION;"
                          "  float3 normal : NORMAL;"
                          "  float2 uv : TEXCOORD0;"
                          "  float4 tangent : TANGENT;"
                          "};"
                          "struct VsOut {"
                          "  float4 position : SV_Position;"
                          "  float3 normal : NORMAL;"
                          "  float tangent_y : TEXCOORD0;"
                          "};"
                          "RWByteAddressBuffer MorphPositionDeltas : register(u0, space1);"
                          "cbuffer MorphWeights : register(b1, space1) {"
                          "  float weight0;"
                          "  float3 _padding0;"
                          "};"
                          "RWByteAddressBuffer MorphNormalDeltas : register(u2, space1);"
                          "RWByteAddressBuffer MorphTangentDeltas : register(u3, space1);"
                          "VsOut vs_morph(VsIn input, uint vertex_id : SV_VertexID) {"
                          "  VsOut output;"
                          "  float3 position_delta = asfloat(MorphPositionDeltas.Load3(vertex_id * 12));"
                          "  float3 normal_delta = asfloat(MorphNormalDeltas.Load3(vertex_id * 12));"
                          "  float3 tangent_delta = asfloat(MorphTangentDeltas.Load3(vertex_id * 12));"
                          "  float3 morphed_tangent = normalize(input.tangent.xyz + tangent_delta * weight0);"
                          "  float3 morphed_normal = normalize(input.normal + normal_delta * weight0 +"
                          "                                    morphed_tangent * 0.0);"
                          "  output.position = float4(input.position + (position_delta * weight0), 1.0);"
                          "  output.normal = morphed_normal;"
                          "  output.tangent_y = saturate(abs(morphed_tangent.y));"
                          "  return output;"
                          "}",
                          "vs_morph", "vs_5_1");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_morph_lit_pixel_shader() {
    return compile_shader("float4 ps_main(float4 position : SV_Position, float3 normal : NORMAL,"
                          "               float tangent_y : TEXCOORD0) : SV_Target {"
                          "  float light = saturate(dot(normalize(normal), normalize(float3(0.0, 1.0, 0.0))));"
                          "  light *= saturate(tangent_y);"
                          "  return float4(light, light, light, 1.0);"
                          "}",
                          "ps_main", "ps_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_solid_position_pixel_shader() {
    return compile_shader("float4 ps_main(float4 position : SV_Position) : SV_Target {"
                          "  return float4(0.2, 0.6, 0.9, 1.0);"
                          "}",
                          "ps_main", "ps_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_textured_triangle_vertex_shader() {
    return compile_shader("struct VsOut {"
                          "  float4 position : SV_Position;"
                          "  float2 uv : TEXCOORD0;"
                          "};"
                          "VsOut vs_main(uint vertex_id : SV_VertexID) {"
                          "  float2 positions[3] = { float2(0.0, 0.75), float2(0.75, -0.75), float2(-0.75, -0.75) };"
                          "  float2 uvs[3] = { float2(0.5, 0.0), float2(1.0, 1.0), float2(0.0, 1.0) };"
                          "  VsOut output;"
                          "  output.position = float4(positions[vertex_id], 0.0, 1.0);"
                          "  output.uv = uvs[vertex_id];"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_fullscreen_textured_vertex_shader() {
    return compile_shader("struct VsOut {"
                          "  float4 position : SV_Position;"
                          "  float2 uv : TEXCOORD0;"
                          "};"
                          "VsOut vs_main(uint vertex_id : SV_VertexID) {"
                          "  float2 positions[3] = { float2(-1.0, -1.0), float2(-1.0, 3.0), float2(3.0, -1.0) };"
                          "  VsOut output;"
                          "  output.position = float4(positions[vertex_id], 0.0, 1.0);"
                          "  output.uv = (positions[vertex_id] * 0.5) + float2(0.5, 0.5);"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_sampled_texture_pixel_shader() {
    return compile_shader("Texture2D sampled_texture : register(t0);"
                          "SamplerState sampled_sampler : register(s1);"
                          "float4 ps_main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target {"
                          "  return sampled_texture.Sample(sampled_sampler, uv);"
                          "}",
                          "ps_main", "ps_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_sampled_depth_pixel_shader() {
    return compile_shader("Texture2D<float> depth_texture : register(t0);"
                          "SamplerState depth_sampler : register(s1);"
                          "float4 ps_main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target {"
                          "  float depth = depth_texture.Sample(depth_sampler, uv);"
                          "  return float4(depth, depth, depth, 1.0);"
                          "}",
                          "ps_main", "ps_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_shadow_receiver_vertex_shader() {
    return compile_shader("struct VsOut {"
                          "  float4 position : SV_Position;"
                          "  float3 world_pos : TEXCOORD1;"
                          "};"
                          "VsOut vs_main(uint vertex_id : SV_VertexID) {"
                          "  float2 positions[6] = {"
                          "    float2(0.0, 0.75), float2(0.75, -0.75), float2(-0.75, -0.75),"
                          "    float2(0.0, 0.75), float2(0.75, -0.75), float2(-0.75, -0.75)"
                          "  };"
                          "  float depths[6] = { 0.25, 0.25, 0.25, 0.75, 0.75, 0.75 };"
                          "  VsOut output;"
                          "  output.position = float4(positions[vertex_id], depths[vertex_id], 1.0);"
                          "  output.world_pos = float3(positions[vertex_id], depths[vertex_id]);"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_shadow_receiver_pixel_shader() {
    return compile_shader("cbuffer ShadowReceiverCb : register(b2) {"
                          "  uint cascade_count;"
                          "  uint3 pad0;"
                          "  float4 splits0;"
                          "  float4 splits1;"
                          "  float4 splits2;"
                          "  row_major float4x4 clip_from_world[8];"
                          "  row_major float4x4 camera_view_from_world;"
                          "  uint4 pad1[8];"
                          "};"
                          "Texture2D<float> shadow_depth_texture : register(t0);"
                          "SamplerState shadow_sampler : register(s1);"
                          "float4 ps_main(float4 position : SV_Position, float3 world_pos : TEXCOORD1) : SV_Target {"
                          "  float4 lc = mul(float4(world_pos, 1.0), clip_from_world[0]);"
                          "  float invw = 1.0 / max(abs(lc.w), 1e-6);"
                          "  float receiver_depth = saturate(lc.z * invw);"
                          "  float2 ndc = lc.xy * invw;"
                          "  float2 tile_uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);"
                          "  float inv_cc = 1.0 / max((float)cascade_count, 1.0);"
                          "  float2 atlas_uv = float2(tile_uv.x * inv_cc, tile_uv.y);"
                          "  float shadow_depth = shadow_depth_texture.Sample(shadow_sampler, atlas_uv);"
                          "  float depth_bias = 0.002;"
                          "  float intensity = (receiver_depth > shadow_depth + depth_bias) ? 0.3 : 1.0;"
                          "  return float4(intensity, intensity, intensity, 1.0);"
                          "}",
                          "ps_main", "ps_5_0");
}

// Two axis-aligned quads in clip space: x in [-1, 0] at z=0.2 and x in [0, 1] at z=0.85. Used with identity
// light clip-from-world so the shadow map holds a vertical depth discontinuity; a fullscreen receiver at z=0.55
// is shadowed on the left and lit on the right, with a PCF-softened boundary at x=0.
[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_vertical_split_shadow_depth_vertex_shader() {
    return compile_shader("struct VsOut {"
                          "  float4 position : SV_Position;"
                          "  float4 color : COLOR0;"
                          "};"
                          "VsOut vs_main(uint vertex_id : SV_VertexID) {"
                          "  float2 positions[12] = {"
                          "    float2(-1.0, -1.0), float2(0.0, -1.0), float2(-1.0, 1.0),"
                          "    float2(0.0, -1.0), float2(0.0, 1.0), float2(-1.0, 1.0),"
                          "    float2(0.0, -1.0), float2(1.0, -1.0), float2(0.0, 1.0),"
                          "    float2(1.0, -1.0), float2(1.0, 1.0), float2(0.0, 1.0)"
                          "  };"
                          "  float depths[12] = {"
                          "    0.2, 0.2, 0.2, 0.2, 0.2, 0.2,"
                          "    0.85, 0.85, 0.85, 0.85, 0.85, 0.85"
                          "  };"
                          "  VsOut output;"
                          "  output.position = float4(positions[vertex_id], depths[vertex_id], 1.0);"
                          "  output.color = float4(0.0, 1.0, 0.0, 1.0);"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

// Fullscreen triangle with world_pos = (ndc.xy, 0.55) for directional shadow receiver tests using identity
// clip_from_world[0] (world == clip before divide; w==1).
[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_fullscreen_shadow_receiver_vertex_shader() {
    return compile_shader("struct VsOut {"
                          "  float4 position : SV_Position;"
                          "  float3 world_pos : TEXCOORD1;"
                          "};"
                          "VsOut vs_main(uint vertex_id : SV_VertexID) {"
                          "  float2 positions[3] = { float2(-1.0, -1.0), float2(-1.0, 3.0), float2(3.0, -1.0) };"
                          "  VsOut output;"
                          "  output.position = float4(positions[vertex_id], 0.0, 1.0);"
                          "  float2 ndc = positions[vertex_id];"
                          "  output.world_pos = float3(ndc.x, ndc.y, 0.55);"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_filtered_shadow_receiver_pixel_shader() {
    return compile_shader("cbuffer ShadowReceiverCb : register(b2) {"
                          "  uint cascade_count;"
                          "  uint3 pad0;"
                          "  float4 splits0;"
                          "  float4 splits1;"
                          "  float4 splits2;"
                          "  row_major float4x4 clip_from_world[8];"
                          "  row_major float4x4 camera_view_from_world;"
                          "  uint4 pad1[8];"
                          "};"
                          "Texture2D<float> shadow_depth_texture : register(t0);"
                          "SamplerState shadow_sampler : register(s1);"
                          "float2 receiver_atlas_uv(float3 world_pos) {"
                          "  float4 lc = mul(float4(world_pos, 1.0), clip_from_world[0]);"
                          "  float invw = 1.0 / max(abs(lc.w), 1e-6);"
                          "  float2 ndc = lc.xy * invw;"
                          "  float2 tile_uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);"
                          "  float inv_cc = 1.0 / max((float)cascade_count, 1.0);"
                          "  return float2(tile_uv.x * inv_cc, tile_uv.y);"
                          "}"
                          "float receiver_depth_from_clip(float3 world_pos) {"
                          "  float4 lc = mul(float4(world_pos, 1.0), clip_from_world[0]);"
                          "  float invw = 1.0 / max(abs(lc.w), 1e-6);"
                          "  return saturate(lc.z * invw);"
                          "}"
                          "float directional_shadow_pcf_3x3(float2 uv, float receiver_depth) {"
                          "  uint width = 1;"
                          "  uint height = 1;"
                          "  shadow_depth_texture.GetDimensions(width, height);"
                          "  float safe_width = width == 0 ? 1.0 : (float)width;"
                          "  float safe_height = height == 0 ? 1.0 : (float)height;"
                          "  float2 texel = 1.0 / float2(safe_width, safe_height);"
                          "  float occlusion = 0.0;"
                          "  [unroll] for (int y = -1; y <= 1; ++y) {"
                          "    [unroll] for (int x = -1; x <= 1; ++x) {"
                          "      float sample_depth = shadow_depth_texture.SampleLevel("
                          "          shadow_sampler, uv + float2((float)x, (float)y) * texel, 0.0);"
                          "      occlusion += sample_depth + 0.002 < receiver_depth ? 1.0 : 0.0;"
                          "    }"
                          "  }"
                          "  return occlusion / 9.0;"
                          "}"
                          "float4 ps_main(float4 position : SV_Position, float3 world_pos : TEXCOORD1) : SV_Target {"
                          "  float2 uv = receiver_atlas_uv(world_pos);"
                          "  float rd = receiver_depth_from_clip(world_pos);"
                          "  float intensity = lerp(1.0, 0.3, directional_shadow_pcf_3x3(uv, rd));"
                          "  return float4(intensity, intensity, intensity, 1.0);"
                          "}",
                          "ps_main", "ps_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_depth_aware_postprocess_pixel_shader() {
    return compile_shader("Texture2D scene_color_texture : register(t0);"
                          "SamplerState scene_color_sampler : register(s1);"
                          "Texture2D<float> scene_depth_texture : register(t2);"
                          "SamplerState scene_depth_sampler : register(s3);"
                          "float4 ps_main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target {"
                          "  float4 color = scene_color_texture.Sample(scene_color_sampler, uv);"
                          "  float depth = scene_depth_texture.Sample(scene_depth_sampler, uv);"
                          "  return float4(depth, color.g, 1.0 - depth, 1.0);"
                          "}",
                          "ps_main", "ps_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_runtime_material_sampled_pixel_shader() {
    return compile_shader("cbuffer MaterialFactors : register(b0) {"
                          "  float4 base_color;"
                          "  float3 emissive;"
                          "  float metallic;"
                          "  float roughness;"
                          "};"
                          "Texture2D base_color_texture : register(t1);"
                          "SamplerState base_color_sampler : register(s16);"
                          "float4 ps_main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target {"
                          "  float4 sampled = base_color_texture.Sample(base_color_sampler, uv);"
                          "  return float4(sampled.rgb * base_color.rgb, sampled.a);"
                          "}",
                          "ps_main", "ps_5_1");
}

void append_le_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
}

void append_le_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
}

void append_le_f32(std::vector<std::uint8_t>& bytes, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    append_le_u32(bytes, bits);
}

void append_vec3(std::vector<std::uint8_t>& bytes, float x, float y, float z) {
    append_le_f32(bytes, x);
    append_le_f32(bytes, y);
    append_le_f32(bytes, z);
}

[[nodiscard]] float read_le_f32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    float value = 0.0F;
    const auto source = std::span<const std::uint8_t>{bytes};
    std::memcpy(&value, source.subspan(offset).data(), sizeof(float));
    return value;
}

void append_skinned_test_vertex(std::vector<std::uint8_t>& bytes, float x, float y, float z) {
    append_le_f32(bytes, x);
    append_le_f32(bytes, y);
    append_le_f32(bytes, z);
    append_le_f32(bytes, 0.0F);
    append_le_f32(bytes, 0.0F);
    append_le_f32(bytes, 1.0F);
    append_le_f32(bytes, 0.5F);
    append_le_f32(bytes, 0.5F);
    append_le_f32(bytes, 1.0F);
    append_le_f32(bytes, 0.0F);
    append_le_f32(bytes, 0.0F);
    append_le_f32(bytes, 1.0F);
    append_le_u16(bytes, 0);
    append_le_u16(bytes, 0);
    append_le_u16(bytes, 0);
    append_le_u16(bytes, 0);
    append_le_f32(bytes, 1.0F);
    append_le_f32(bytes, 0.0F);
    append_le_f32(bytes, 0.0F);
    append_le_f32(bytes, 0.0F);
}

[[nodiscard]] std::string hex_encode(const std::vector<std::uint8_t>& bytes) {
    constexpr auto digits = std::string_view{"0123456789abcdef"};
    std::string encoded;
    encoded.reserve(bytes.size() * 2U);
    for (const auto byte : bytes) {
        encoded.push_back(digits[(byte >> 4U) & 0x0fU]);
        encoded.push_back(digits[byte & 0x0fU]);
    }
    return encoded;
}

[[nodiscard]] std::string d3d12_runtime_scene_texture_payload(mirakana::AssetId texture) {
    return "format=GameEngine.CookedTexture.v1\n"
           "asset.id=" +
           std::to_string(texture.value) +
           "\n"
           "asset.kind=texture\n"
           "texture.width=1\n"
           "texture.height=1\n"
           "texture.pixel_format=rgba8_unorm\n"
           "texture.source_bytes=4\n"
           "texture.data_hex=" +
           hex_encode(std::vector<std::uint8_t>{64, 200, 80, 255}) + "\n";
}

[[nodiscard]] std::string d3d12_runtime_scene_mesh_payload(mirakana::AssetId mesh) {
    std::vector<std::uint8_t> vertex_bytes;
    vertex_bytes.reserve(36);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, 0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, 0.75F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);

    std::vector<std::uint8_t> index_bytes;
    index_bytes.reserve(12);
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    return "format=GameEngine.CookedMesh.v2\n"
           "asset.id=" +
           std::to_string(mesh.value) +
           "\n"
           "asset.kind=mesh\n"
           "mesh.vertex_count=3\n"
           "mesh.index_count=3\n"
           "mesh.has_normals=false\n"
           "mesh.has_uvs=false\n"
           "mesh.has_tangent_frame=false\n"
           "mesh.vertex_data_hex=" +
           hex_encode(vertex_bytes) +
           "\n"
           "mesh.index_data_hex=" +
           hex_encode(index_bytes) + "\n";
}

[[nodiscard]] std::string d3d12_runtime_scene_material_payload(mirakana::AssetId material, mirakana::AssetId texture) {
    return mirakana::serialize_material_definition(mirakana::MaterialDefinition{
        .id = material,
        .name = "D3D12RuntimeSceneMaterial",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.5F, 0.5F, 1.0F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 1.0F,
            },
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture}},
        .double_sided = false,
    });
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage
make_d3d12_runtime_scene_package(mirakana::AssetId mesh, mirakana::AssetId material, mirakana::AssetId texture) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = texture,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/d3d12_runtime_scene_base_color.texture",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = d3d12_runtime_scene_texture_payload(texture),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{2},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = "assets/meshes/d3d12_runtime_scene_triangle.mesh",
            .content_hash = 2,
            .source_revision = 1,
            .dependencies = {},
            .content = d3d12_runtime_scene_mesh_payload(mesh),
        },
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{3},
            .asset = material,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/d3d12_runtime_scene_material.material",
            .content_hash = 3,
            .source_revision = 1,
            .dependencies = {texture},
            .content = d3d12_runtime_scene_material_payload(material, texture),
        },
    });
}

[[nodiscard]] mirakana::Scene make_d3d12_runtime_scene(mirakana::AssetId mesh, mirakana::AssetId material) {
    mirakana::Scene scene("D3D12RuntimeScene");
    const auto node = scene.create_node("Triangle");
    mirakana::SceneNodeComponents components;
    components.mesh_renderer = mirakana::MeshRendererComponent{.mesh = mesh, .material = material, .visible = true};
    scene.set_components(node, components);
    return scene;
}

} // namespace

MK_TEST("d3d12 backend scaffold reports windows sdk availability") {
    MK_REQUIRE(mirakana::rhi::d3d12::backend_kind() == mirakana::rhi::BackendKind::d3d12);
    MK_REQUIRE(mirakana::rhi::d3d12::backend_name() == "d3d12");
    MK_REQUIRE(mirakana::rhi::d3d12::compiled_with_windows_sdk());
}

MK_TEST("d3d12 backend probes dxgi and device support") {
    const auto probe = mirakana::rhi::d3d12::probe_runtime();

    MK_REQUIRE(probe.windows_sdk_available);
    MK_REQUIRE(probe.dxgi_factory_created);
    MK_REQUIRE(probe.adapter_count >= probe.hardware_adapter_count);
    MK_REQUIRE(probe.hardware_device_supported || probe.warp_device_supported);
}

MK_TEST("d3d12 backend bootstraps device command queue and fence") {
    const auto result = mirakana::rhi::d3d12::bootstrap_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.device_created);
    MK_REQUIRE(result.command_queue_created);
    MK_REQUIRE(result.fence_created);
    MK_REQUIRE(result.initial_fence_value == 0);
}

MK_TEST("d3d12 backend creates first gpu owned resources") {
    const auto result = mirakana::rhi::d3d12::bootstrap_resource_ownership(mirakana::rhi::d3d12::ResourceOwnershipDesc{
        .device = mirakana::rhi::d3d12::DeviceBootstrapDesc{.prefer_warp = false, .enable_debug_layer = false},
        .upload_buffer_size_bytes = 4096,
        .readback_buffer_size_bytes = 4096,
        .texture_extent = mirakana::rhi::Extent2D{.width = 4, .height = 4},
        .texture_format = mirakana::rhi::Format::rgba8_unorm,
    });

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.device_created);
    MK_REQUIRE(result.upload_buffer_created);
    MK_REQUIRE(result.readback_buffer_created);
    MK_REQUIRE(result.default_texture_created);
    MK_REQUIRE(result.texture_allocation_size_bytes > 0);
}

MK_TEST("d3d12 device context reports graphics and compute timestamp measurement support") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);
    MK_REQUIRE(context->valid());

    const auto graphics = context->queue_timestamp_measurement_support(mirakana::rhi::QueueKind::graphics);
    const auto compute = context->queue_timestamp_measurement_support(mirakana::rhi::QueueKind::compute);

    MK_REQUIRE(graphics.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(compute.queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(graphics.supported);
    MK_REQUIRE(compute.supported);
    MK_REQUIRE(graphics.frequency > 0);
    MK_REQUIRE(compute.frequency > 0);
    MK_REQUIRE(graphics.diagnostic == std::string_view{"d3d12 queue timestamp measurement ready"});
    MK_REQUIRE(compute.diagnostic == std::string_view{"d3d12 queue timestamp measurement ready"});
}

MK_TEST("d3d12 device context records backend private queue timestamp intervals") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);
    MK_REQUIRE(context->valid());

    const auto graphics = context->measure_queue_timestamp_interval(mirakana::rhi::QueueKind::graphics);
    const auto compute = context->measure_queue_timestamp_interval(mirakana::rhi::QueueKind::compute);

    MK_REQUIRE(graphics.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(compute.queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(graphics.status == mirakana::rhi::d3d12::QueueTimestampMeasurementStatus::ready);
    MK_REQUIRE(compute.status == mirakana::rhi::d3d12::QueueTimestampMeasurementStatus::ready);
    MK_REQUIRE(graphics.frequency > 0);
    MK_REQUIRE(compute.frequency > 0);
    MK_REQUIRE(graphics.begin_ticks <= graphics.end_ticks);
    MK_REQUIRE(compute.begin_ticks <= compute.end_ticks);
    MK_REQUIRE(graphics.elapsed_seconds >= 0.0);
    MK_REQUIRE(compute.elapsed_seconds >= 0.0);
    MK_REQUIRE(graphics.diagnostic == std::string_view{"d3d12 queue timestamp interval measured"});
    MK_REQUIRE(compute.diagnostic == std::string_view{"d3d12 queue timestamp interval measured"});
}

MK_TEST("d3d12 device context reports graphics and compute queue clock calibration") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);
    MK_REQUIRE(context->valid());

    const auto graphics = context->calibrate_queue_clock(mirakana::rhi::QueueKind::graphics);
    const auto compute = context->calibrate_queue_clock(mirakana::rhi::QueueKind::compute);

    MK_REQUIRE(graphics.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(compute.queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(graphics.status == mirakana::rhi::d3d12::QueueClockCalibrationStatus::ready);
    MK_REQUIRE(compute.status == mirakana::rhi::d3d12::QueueClockCalibrationStatus::ready);
    MK_REQUIRE(graphics.frequency > 0);
    MK_REQUIRE(compute.frequency > 0);
    MK_REQUIRE(graphics.cpu_timestamp > 0);
    MK_REQUIRE(compute.cpu_timestamp > 0);
    MK_REQUIRE(graphics.diagnostic == std::string_view{"d3d12 queue clock calibration ready"});
    MK_REQUIRE(compute.diagnostic == std::string_view{"d3d12 queue clock calibration ready"});
}

MK_TEST("d3d12 device context records backend private calibrated queue timing diagnostics") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);
    MK_REQUIRE(context->valid());

    const auto graphics = context->measure_calibrated_queue_timing(mirakana::rhi::QueueKind::graphics);
    const auto compute = context->measure_calibrated_queue_timing(mirakana::rhi::QueueKind::compute);

    MK_REQUIRE(graphics.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(compute.queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(graphics.status == mirakana::rhi::d3d12::QueueCalibratedTimingStatus::ready);
    MK_REQUIRE(compute.status == mirakana::rhi::d3d12::QueueCalibratedTimingStatus::ready);
    MK_REQUIRE(graphics.frequency > 0);
    MK_REQUIRE(compute.frequency > 0);
    MK_REQUIRE(graphics.qpc_frequency > 0);
    MK_REQUIRE(compute.qpc_frequency > 0);
    MK_REQUIRE(graphics.begin_ticks <= graphics.end_ticks);
    MK_REQUIRE(compute.begin_ticks <= compute.end_ticks);
    MK_REQUIRE(graphics.elapsed_seconds >= 0.0);
    MK_REQUIRE(compute.elapsed_seconds >= 0.0);
    MK_REQUIRE(graphics.calibration_before_cpu_timestamp > 0);
    MK_REQUIRE(compute.calibration_before_cpu_timestamp > 0);
    MK_REQUIRE(graphics.calibration_after_cpu_timestamp > 0);
    MK_REQUIRE(compute.calibration_after_cpu_timestamp > 0);
    MK_REQUIRE(graphics.calibrated_begin_cpu_timestamp <= graphics.calibrated_end_cpu_timestamp);
    MK_REQUIRE(compute.calibrated_begin_cpu_timestamp <= compute.calibrated_end_cpu_timestamp);
    MK_REQUIRE(graphics.diagnostic == std::string_view{"d3d12 calibrated queue timing measured"});
    MK_REQUIRE(compute.diagnostic == std::string_view{"d3d12 calibrated queue timing measured"});
}

MK_TEST("d3d12 backend rejects invalid resource ownership descriptions") {
    const auto result = mirakana::rhi::d3d12::bootstrap_resource_ownership(mirakana::rhi::d3d12::ResourceOwnershipDesc{
        .device = mirakana::rhi::d3d12::DeviceBootstrapDesc{.prefer_warp = false, .enable_debug_layer = false},
        .upload_buffer_size_bytes = 0,
        .readback_buffer_size_bytes = 4096,
        .texture_extent = mirakana::rhi::Extent2D{.width = 4, .height = 4},
        .texture_format = mirakana::rhi::Format::rgba8_unorm,
    });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(result.validation_failed);
    MK_REQUIRE(!result.upload_buffer_created);
}

MK_TEST("d3d12 device context owns committed resources across calls") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);
    MK_REQUIRE(context->valid());
    MK_REQUIRE(context->backend_kind() == mirakana::rhi::BackendKind::d3d12);

    const auto upload = context->create_committed_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 4096,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto texture = context->create_committed_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination,
    });

    MK_REQUIRE(upload.value == 1);
    MK_REQUIRE(texture.value == 2);
    MK_REQUIRE(context->stats().committed_buffers_created == 1);
    MK_REQUIRE(context->stats().committed_textures_created == 1);
    MK_REQUIRE(context->stats().committed_resources_alive == 2);
}

MK_TEST("d3d12 rhi exports shared textures for editor interop") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });
    MK_REQUIRE(device != nullptr);

    const auto texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource |
                 mirakana::rhi::TextureUsage::copy_source | mirakana::rhi::TextureUsage::shared,
    });

    const auto exported = mirakana::rhi::d3d12::export_shared_texture(*device, texture);

    MK_REQUIRE(exported.succeeded);
    MK_REQUIRE(!exported.invalid_texture);
    MK_REQUIRE(!exported.texture_not_shareable);
    MK_REQUIRE(exported.shared_handle.value != 0);
    MK_REQUIRE(exported.extent.width == 16);
    MK_REQUIRE(exported.extent.height == 16);
    MK_REQUIRE(exported.format == mirakana::rhi::Format::rgba8_unorm);
    MK_REQUIRE(device->stats().shared_texture_exports == 1);

    mirakana::rhi::d3d12::close_shared_texture_handle(exported.shared_handle);
}

MK_TEST("d3d12 rhi rejects shared texture export without shared usage") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });
    MK_REQUIRE(device != nullptr);

    const auto texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource |
                 mirakana::rhi::TextureUsage::copy_source,
    });

    const auto exported = mirakana::rhi::d3d12::export_shared_texture(*device, texture);

    MK_REQUIRE(!exported.succeeded);
    MK_REQUIRE(exported.texture_not_shareable);
    MK_REQUIRE(exported.shared_handle.value == 0);
    MK_REQUIRE(device->stats().shared_texture_export_failures == 1);
}

MK_TEST("d3d12 device context rejects invalid committed resource descriptions") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);
    const auto invalid_buffer = context->create_committed_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 0,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto invalid_texture = context->create_committed_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 0, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination,
    });

    MK_REQUIRE(invalid_buffer.value == 0);
    MK_REQUIRE(invalid_texture.value == 0);
    MK_REQUIRE(context->stats().committed_resources_alive == 0);
}

MK_TEST("d3d12 device context owns command allocators and lists") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(commands.value == 1);
    MK_REQUIRE(context->stats().command_allocators_created == 1);
    MK_REQUIRE(context->stats().command_lists_created == 1);
    MK_REQUIRE(context->stats().command_lists_alive == 1);

    MK_REQUIRE(context->close_command_list(commands));
    MK_REQUIRE(context->stats().command_lists_closed == 1);

    MK_REQUIRE(context->reset_command_list(commands));
    MK_REQUIRE(context->stats().command_lists_reset == 1);

    MK_REQUIRE(context->close_command_list(commands));
    MK_REQUIRE(context->stats().command_lists_closed == 2);
}

MK_TEST("d3d12 device context records committed texture aliasing barrier intent") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto first = context->create_committed_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto second = context->create_committed_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(context->texture_aliasing_barrier(commands, first, second));
    MK_REQUIRE(context->close_command_list(commands));
    MK_REQUIRE(context->stats().texture_aliasing_barriers == 1);
    MK_REQUIRE(context->stats().null_resource_aliasing_barriers == 1);
    MK_REQUIRE(context->stats().texture_transitions == 0);
}

MK_TEST("d3d12 device context creates placed transient texture resources") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto placed = context->create_placed_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });

    const auto stats = context->stats();
    MK_REQUIRE(placed.value != 0);
    MK_REQUIRE(stats.placed_texture_heaps_created == 1);
    MK_REQUIRE(stats.placed_textures_created == 1);
    MK_REQUIRE(stats.placed_resources_alive == 1);
    MK_REQUIRE(stats.placed_resource_activation_barriers == 0);
    MK_REQUIRE(stats.committed_textures_created == 0);

    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(context->activate_placed_texture(commands, placed));
    MK_REQUIRE(context->activate_placed_texture(commands, placed));
    MK_REQUIRE(context->close_command_list(commands));
    MK_REQUIRE(context->stats().placed_resource_activation_barriers == 1);

    MK_REQUIRE(context->reset_command_list(commands));
    MK_REQUIRE(context->activate_placed_texture(commands, placed));
    MK_REQUIRE(context->activate_placed_texture(commands, placed));
    MK_REQUIRE(context->close_command_list(commands));
    MK_REQUIRE(context->stats().placed_resource_activation_barriers == 2);

    const auto fence = context->execute_command_list(commands);
    MK_REQUIRE(fence.value != 0);
    MK_REQUIRE(context->wait_for_fence(fence, 0xFFFFFFFFU));

    const auto after_submit = context->create_command_list(mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(context->activate_placed_texture(after_submit, placed));
    MK_REQUIRE(context->close_command_list(after_submit));
    MK_REQUIRE(context->stats().placed_resource_activation_barriers == 2);

    context->destroy_committed_resource(placed);
    MK_REQUIRE(context->stats().placed_resources_alive == 0);
}

MK_TEST("d3d12 device context executes closed command lists and signals fences") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(context->execute_command_list(commands).value == 0);
    MK_REQUIRE(context->close_command_list(commands));

    const auto fence = context->execute_command_list(commands);

    MK_REQUIRE(fence.value == 1);
    MK_REQUIRE(context->stats().command_lists_executed == 1);
    MK_REQUIRE(context->stats().fence_signals == 1);
    MK_REQUIRE(context->stats().last_submitted_fence_value == 1);
    MK_REQUIRE(context->execute_command_list(commands).value == 0);
}

MK_TEST("d3d12 device context rejects invalid command list handles") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto invalid = mirakana::rhi::d3d12::NativeCommandListHandle{99};

    MK_REQUIRE(!context->close_command_list(invalid));
    MK_REQUIRE(!context->reset_command_list(invalid));
    MK_REQUIRE(context->execute_command_list(invalid).value == 0);
    MK_REQUIRE(context->stats().command_lists_alive == 0);
}

MK_TEST("d3d12 device context waits for submitted fences before allocator reset") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(context->close_command_list(commands));

    const auto fence = context->execute_command_list(commands);

    MK_REQUIRE(fence.value == 1);
    MK_REQUIRE(context->wait_for_fence(fence, 1000));
    MK_REQUIRE(context->completed_fence().value >= fence.value);
    MK_REQUIRE(context->reset_command_list(commands));
    MK_REQUIRE(context->stats().fence_waits == 1);
}

MK_TEST("d3d12 device context rejects waits for unsignaled fences") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    MK_REQUIRE(context->wait_for_fence(mirakana::rhi::FenceValue{}, 0));
    MK_REQUIRE(!context->wait_for_fence(mirakana::rhi::FenceValue{1}, 0));
    MK_REQUIRE(context->stats().fence_waits == 2);
    MK_REQUIRE(context->stats().fence_wait_timeouts == 1);
}

MK_TEST("d3d12 device context synchronizes copy queue with graphics fence") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto graphics_commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(context->close_command_list(graphics_commands));
    const auto graphics_fence = context->execute_command_list(graphics_commands);

    MK_REQUIRE(graphics_fence.value == 1);
    MK_REQUIRE(context->queue_wait_for_fence(mirakana::rhi::QueueKind::copy, graphics_fence));

    const auto copy_commands = context->create_command_list(mirakana::rhi::QueueKind::copy);

    MK_REQUIRE(copy_commands.value == 2);
    MK_REQUIRE(context->close_command_list(copy_commands));

    const auto copy_fence = context->execute_command_list(copy_commands);

    MK_REQUIRE(copy_fence.value == 1);
    MK_REQUIRE(copy_fence.queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(context->wait_for_fence(copy_fence, 1000));
    MK_REQUIRE(context->stats().queue_waits == 1);
    MK_REQUIRE(context->stats().command_lists_executed == 2);
    MK_REQUIRE(context->stats().fence_signals == 2);
    MK_REQUIRE(context->stats().graphics_queue_submits == 1);
    MK_REQUIRE(context->stats().copy_queue_submits == 1);
    MK_REQUIRE(context->stats().last_graphics_submitted_fence_value == graphics_fence.value);
    MK_REQUIRE(context->stats().last_copy_queue_wait_fence_value == graphics_fence.value);
    MK_REQUIRE(context->stats().last_queue_wait_fence_value == graphics_fence.value);
    MK_REQUIRE(context->stats().last_copy_submitted_fence_value == copy_fence.value);
}

MK_TEST("d3d12 device context rejects queue waits for unsignaled fences") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    MK_REQUIRE(!context->queue_wait_for_fence(mirakana::rhi::QueueKind::graphics, mirakana::rhi::FenceValue{1}));
    MK_REQUIRE(context->stats().queue_waits == 1);
    MK_REQUIRE(context->stats().queue_wait_failures == 1);
}

MK_TEST("d3d12 device context owns hwnd swapchain back buffers") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto swapchain = context->create_swapchain_for_window(mirakana::rhi::d3d12::NativeSwapchainDesc{
        .window = mirakana::rhi::d3d12::NativeWindowHandle{window.hwnd()},
        .swapchain =
            mirakana::rhi::SwapchainDesc{
                .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
                .format = mirakana::rhi::Format::bgra8_unorm,
                .buffer_count = 2,
                .vsync = false,
            },
    });

    const auto info = context->swapchain_info(swapchain);

    MK_REQUIRE(swapchain.value == 1);
    MK_REQUIRE(info.valid);
    MK_REQUIRE(info.extent.width == 64);
    MK_REQUIRE(info.extent.height == 64);
    MK_REQUIRE(info.format == mirakana::rhi::Format::bgra8_unorm);
    MK_REQUIRE(info.buffer_count == 2);
    MK_REQUIRE(info.render_target_view_count == 2);
    MK_REQUIRE(info.current_back_buffer < info.buffer_count);
    MK_REQUIRE(context->stats().swapchains_created == 1);
    MK_REQUIRE(context->stats().swapchain_back_buffers_created == 2);
    MK_REQUIRE(context->stats().render_target_view_heaps_created == 1);
    MK_REQUIRE(context->stats().render_target_views_created == 2);
    MK_REQUIRE(context->stats().swapchains_alive == 1);
}

MK_TEST("d3d12 device context presents and resizes hwnd swapchains") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto swapchain = context->create_swapchain_for_window(mirakana::rhi::d3d12::NativeSwapchainDesc{
        .window = mirakana::rhi::d3d12::NativeWindowHandle{window.hwnd()},
        .swapchain =
            mirakana::rhi::SwapchainDesc{
                .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
                .format = mirakana::rhi::Format::bgra8_unorm,
                .buffer_count = 2,
                .vsync = false,
            },
    });

    MK_REQUIRE(swapchain.value == 1);
    MK_REQUIRE(context->present_swapchain(swapchain));
    MK_REQUIRE(context->resize_swapchain(swapchain, mirakana::rhi::Extent2D{96, 80}));

    const auto info = context->swapchain_info(swapchain);

    MK_REQUIRE(info.valid);
    MK_REQUIRE(info.extent.width == 96);
    MK_REQUIRE(info.extent.height == 80);
    MK_REQUIRE(info.buffer_count == 2);
    MK_REQUIRE(info.render_target_view_count == 2);
    MK_REQUIRE(context->stats().swapchain_presents == 1);
    MK_REQUIRE(context->stats().swapchain_resizes == 1);
    MK_REQUIRE(context->stats().swapchain_back_buffers_created == 4);
    MK_REQUIRE(context->stats().render_target_view_heaps_created == 2);
    MK_REQUIRE(context->stats().render_target_views_created == 4);
}

MK_TEST("d3d12 device context rejects invalid swapchain descriptions") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto null_window = context->create_swapchain_for_window(mirakana::rhi::d3d12::NativeSwapchainDesc{
        .window = mirakana::rhi::d3d12::NativeWindowHandle{},
        .swapchain =
            mirakana::rhi::SwapchainDesc{
                .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
                .format = mirakana::rhi::Format::bgra8_unorm,
                .buffer_count = 2,
                .vsync = false,
            },
    });
    const auto single_buffer = context->create_swapchain_for_window(mirakana::rhi::d3d12::NativeSwapchainDesc{
        .window = mirakana::rhi::d3d12::NativeWindowHandle{reinterpret_cast<void*>(1)},
        .swapchain =
            mirakana::rhi::SwapchainDesc{
                .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
                .format = mirakana::rhi::Format::bgra8_unorm,
                .buffer_count = 1,
                .vsync = false,
            },
    });

    MK_REQUIRE(null_window.value == 0);
    MK_REQUIRE(single_buffer.value == 0);
    MK_REQUIRE(!context->present_swapchain(mirakana::rhi::d3d12::NativeSwapchainHandle{99}));
    MK_REQUIRE(
        !context->resize_swapchain(mirakana::rhi::d3d12::NativeSwapchainHandle{99}, mirakana::rhi::Extent2D{64, 64}));
    MK_REQUIRE(!context->swapchain_info(mirakana::rhi::d3d12::NativeSwapchainHandle{99}).valid);
    MK_REQUIRE(context->stats().swapchains_created == 0);
}

MK_TEST("d3d12 device context records swapchain back buffer transitions") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto swapchain = context->create_swapchain_for_window(mirakana::rhi::d3d12::NativeSwapchainDesc{
        .window = mirakana::rhi::d3d12::NativeWindowHandle{window.hwnd()},
        .swapchain =
            mirakana::rhi::SwapchainDesc{
                .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
                .format = mirakana::rhi::Format::bgra8_unorm,
                .buffer_count = 2,
                .vsync = false,
            },
    });
    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(swapchain.value == 1);
    MK_REQUIRE(commands.value == 1);
    MK_REQUIRE(context->transition_swapchain_back_buffer(commands, swapchain, mirakana::rhi::ResourceState::present,
                                                         mirakana::rhi::ResourceState::render_target));
    MK_REQUIRE(context->clear_swapchain_back_buffer(commands, swapchain, 0.1F, 0.2F, 0.3F, 1.0F));
    MK_REQUIRE(context->transition_swapchain_back_buffer(
        commands, swapchain, mirakana::rhi::ResourceState::render_target, mirakana::rhi::ResourceState::present));
    MK_REQUIRE(context->close_command_list(commands));

    const auto fence = context->execute_command_list(commands);

    MK_REQUIRE(fence.value == 1);
    MK_REQUIRE(context->wait_for_fence(fence, 1000));
    MK_REQUIRE(context->present_swapchain(swapchain));
    MK_REQUIRE(context->stats().swapchain_back_buffer_transitions == 2);
    MK_REQUIRE(context->stats().swapchain_back_buffer_clears == 1);
}

MK_TEST("d3d12 device context owns shader visible descriptor heaps") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto heap = context->create_descriptor_heap(mirakana::rhi::d3d12::NativeDescriptorHeapDesc{
        .kind = mirakana::rhi::d3d12::NativeDescriptorHeapKind::cbv_srv_uav,
        .capacity = 16,
        .shader_visible = true,
    });
    const auto info = context->descriptor_heap_info(heap);

    MK_REQUIRE(heap.value == 1);
    MK_REQUIRE(info.valid);
    MK_REQUIRE(info.kind == mirakana::rhi::d3d12::NativeDescriptorHeapKind::cbv_srv_uav);
    MK_REQUIRE(info.capacity == 16);
    MK_REQUIRE(info.shader_visible);
    MK_REQUIRE(info.descriptor_size > 0);
    MK_REQUIRE(context->stats().descriptor_heaps_created == 1);
    MK_REQUIRE(context->stats().shader_visible_descriptor_heaps_created == 1);
    MK_REQUIRE(context->stats().shader_visible_descriptors_reserved == 16);
}

MK_TEST("d3d12 device context creates descriptor table root signatures") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto root_signature = context
                                    ->create_root_signature(
                                        mirakana::rhi::d3d12::NativeRootSignatureDesc{
                                            .descriptor_sets =
                                                {
                                                    mirakana::rhi::DescriptorSetLayoutDesc{
                                                        {
                                                            mirakana::rhi::DescriptorBindingDesc{
                                                                .binding = 0,
                                                                .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                                                .count = 1,
                                                                .stages = mirakana::rhi::ShaderStageVisibility::vertex,
                                                            },
                                                            mirakana::rhi::DescriptorBindingDesc{
                                                                .binding = 1,
                                                                .type = mirakana::rhi::DescriptorType::sampled_texture,
                                                                .count = 4,
                                                                .stages =
                                                                    mirakana::rhi::ShaderStageVisibility::fragment,
                                                            },
                                                        }},
                                                },
                                            .push_constant_bytes = 64,
                                        });
    const auto info = context->root_signature_info(root_signature);

    MK_REQUIRE(root_signature.value == 1);
    MK_REQUIRE(info.valid);
    MK_REQUIRE(info.descriptor_table_count == 1);
    MK_REQUIRE(info.descriptor_range_count == 2);
    MK_REQUIRE(info.push_constant_bytes == 64);
    MK_REQUIRE(context->stats().root_signatures_created == 1);
    MK_REQUIRE(context->stats().descriptor_tables_created == 1);
    MK_REQUIRE(context->stats().descriptor_ranges_created == 2);
}

MK_TEST("d3d12 device context splits sampler descriptor tables by heap kind") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto
        root_signature = context
                             ->create_root_signature(
                                 mirakana::rhi::d3d12::NativeRootSignatureDesc{
                                     .descriptor_sets =
                                         {
                                             mirakana::rhi::DescriptorSetLayoutDesc{
                                                 {
                                                     mirakana::rhi::DescriptorBindingDesc{
                                                         .binding = 0,
                                                         .type = mirakana::rhi::DescriptorType::sampled_texture,
                                                         .count = 1,
                                                         .stages = mirakana::rhi::ShaderStageVisibility::fragment,
                                                     },
                                                     mirakana::rhi::DescriptorBindingDesc{
                                                         .binding = 1,
                                                         .type = mirakana::rhi::DescriptorType::sampler,
                                                         .count = 1,
                                                         .stages = mirakana::rhi::ShaderStageVisibility::fragment,
                                                     },
                                                 }},
                                         },
                                     .push_constant_bytes = 0,
                                 });
    const auto info = context->root_signature_info(root_signature);

    MK_REQUIRE(root_signature.value == 1);
    MK_REQUIRE(info.valid);
    MK_REQUIRE(info.descriptor_table_count == 2);
    MK_REQUIRE(info.descriptor_range_count == 2);
    MK_REQUIRE(context->stats().descriptor_tables_created == 2);
    MK_REQUIRE(context->stats().descriptor_ranges_created == 2);
}

MK_TEST("d3d12 device context writes native descriptor views") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto heap = context->create_descriptor_heap(mirakana::rhi::d3d12::NativeDescriptorHeapDesc{
        .kind = mirakana::rhi::d3d12::NativeDescriptorHeapKind::cbv_srv_uav,
        .capacity = 4,
        .shader_visible = true,
    });
    const auto uniform_buffer = context->create_committed_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform,
    });
    const auto storage_buffer = context->create_committed_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::storage,
    });
    const auto sampled_texture = context->create_committed_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto storage_texture = context->create_committed_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::storage,
    });

    MK_REQUIRE(heap.value == 1);
    MK_REQUIRE(uniform_buffer.value == 1);
    MK_REQUIRE(storage_buffer.value == 2);
    MK_REQUIRE(sampled_texture.value == 3);
    MK_REQUIRE(storage_texture.value == 4);
    MK_REQUIRE(context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        heap,
        0,
        uniform_buffer,
        mirakana::rhi::DescriptorType::uniform_buffer,
    }));
    MK_REQUIRE(context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        heap,
        1,
        storage_buffer,
        mirakana::rhi::DescriptorType::storage_buffer,
    }));
    MK_REQUIRE(context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        heap,
        2,
        sampled_texture,
        mirakana::rhi::DescriptorType::sampled_texture,
    }));
    MK_REQUIRE(context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        heap,
        3,
        storage_texture,
        mirakana::rhi::DescriptorType::storage_texture,
    }));
    MK_REQUIRE(context->stats().descriptor_views_written == 4);
}

MK_TEST("d3d12 device context writes native sampler descriptors") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto heap = context->create_descriptor_heap(mirakana::rhi::d3d12::NativeDescriptorHeapDesc{
        .kind = mirakana::rhi::d3d12::NativeDescriptorHeapKind::sampler,
        .capacity = 2,
        .shader_visible = true,
    });

    MK_REQUIRE(heap.value == 1);
    MK_REQUIRE(context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        heap,
        0,
        mirakana::rhi::d3d12::NativeResourceHandle{},
        mirakana::rhi::DescriptorType::sampler,
        mirakana::rhi::SamplerDesc{
            mirakana::rhi::SamplerFilter::nearest,
            mirakana::rhi::SamplerFilter::linear,
            mirakana::rhi::SamplerAddressMode::clamp_to_edge,
            mirakana::rhi::SamplerAddressMode::repeat,
            mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        },
    }));
    MK_REQUIRE(context->stats().descriptor_views_written == 1);
}

MK_TEST("d3d12 device context binds descriptor heaps root signatures and descriptor tables") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto heap = context->create_descriptor_heap(mirakana::rhi::d3d12::NativeDescriptorHeapDesc{
        .kind = mirakana::rhi::d3d12::NativeDescriptorHeapKind::cbv_srv_uav,
        .capacity = 8,
        .shader_visible = true,
    });
    const auto sampler_heap = context->create_descriptor_heap(mirakana::rhi::d3d12::NativeDescriptorHeapDesc{
        .kind = mirakana::rhi::d3d12::NativeDescriptorHeapKind::sampler,
        .capacity = 8,
        .shader_visible = true,
    });
    const auto root_signature = context
                                    ->create_root_signature(
                                        mirakana::rhi::d3d12::NativeRootSignatureDesc{
                                            .descriptor_sets =
                                                {
                                                    mirakana::rhi::DescriptorSetLayoutDesc{
                                                        {
                                                            mirakana::rhi::DescriptorBindingDesc{
                                                                .binding = 0,
                                                                .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                                                .count = 1,
                                                                .stages = mirakana::rhi::ShaderStageVisibility::vertex,
                                                            },
                                                            mirakana::rhi::DescriptorBindingDesc{
                                                                .binding = 1,
                                                                .type = mirakana::rhi::DescriptorType::sampler,
                                                                .count = 1,
                                                                .stages =
                                                                    mirakana::rhi::ShaderStageVisibility::fragment,
                                                            },
                                                        }},
                                                },
                                            .push_constant_bytes = 0,
                                        });
    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(heap.value == 1);
    MK_REQUIRE(sampler_heap.value == 2);
    MK_REQUIRE(root_signature.value == 1);
    MK_REQUIRE(commands.value == 1);
    MK_REQUIRE(context->set_graphics_root_signature(commands, root_signature));
    MK_REQUIRE(
        context->set_descriptor_heaps(commands, mirakana::rhi::d3d12::NativeDescriptorHeapBinding{heap, sampler_heap}));
    MK_REQUIRE(context->set_graphics_descriptor_table(commands, root_signature, 0, heap, 0));
    MK_REQUIRE(context->set_graphics_descriptor_table(commands, root_signature, 1, sampler_heap, 0));
    MK_REQUIRE(context->close_command_list(commands));

    const auto fence = context->execute_command_list(commands);

    MK_REQUIRE(fence.value == 1);
    MK_REQUIRE(context->wait_for_fence(fence, 1000));
    MK_REQUIRE(context->stats().root_signatures_bound == 1);
    MK_REQUIRE(context->stats().descriptor_heaps_bound == 2);
    MK_REQUIRE(context->stats().descriptor_tables_bound == 2);
}

MK_TEST("d3d12 device context rejects invalid descriptor and root signature work") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto invalid_heap = context->create_descriptor_heap(mirakana::rhi::d3d12::NativeDescriptorHeapDesc{
        .kind = mirakana::rhi::d3d12::NativeDescriptorHeapKind::cbv_srv_uav,
        .capacity = 0,
        .shader_visible = true,
    });
    const auto invalid_root_signature = context->create_root_signature(mirakana::rhi::d3d12::NativeRootSignatureDesc{
        .descriptor_sets =
            {
                mirakana::rhi::DescriptorSetLayoutDesc{{
                    mirakana::rhi::DescriptorBindingDesc{
                        .binding = 0,
                        .type = mirakana::rhi::DescriptorType::uniform_buffer,
                        .count = 0,
                        .stages = mirakana::rhi::ShaderStageVisibility::vertex,
                    },
                }},
            },
        .push_constant_bytes = 0,
    });
    const auto heap = context->create_descriptor_heap(mirakana::rhi::d3d12::NativeDescriptorHeapDesc{
        .kind = mirakana::rhi::d3d12::NativeDescriptorHeapKind::cbv_srv_uav,
        .capacity = 4,
        .shader_visible = true,
    });
    const auto root_signature = context->create_root_signature(mirakana::rhi::d3d12::NativeRootSignatureDesc{
        .descriptor_sets =
            {
                mirakana::rhi::DescriptorSetLayoutDesc{{
                    mirakana::rhi::DescriptorBindingDesc{
                        .binding = 0,
                        .type = mirakana::rhi::DescriptorType::uniform_buffer,
                        .count = 1,
                        .stages = mirakana::rhi::ShaderStageVisibility::vertex,
                    },
                }},
            },
        .push_constant_bytes = 0,
    });
    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(invalid_heap.value == 0);
    MK_REQUIRE(invalid_root_signature.value == 0);
    MK_REQUIRE(heap.value == 1);
    MK_REQUIRE(root_signature.value == 1);
    MK_REQUIRE(commands.value == 1);
    MK_REQUIRE(!context->set_descriptor_heaps(commands, mirakana::rhi::d3d12::NativeDescriptorHeapBinding{{99}, {}}));
    MK_REQUIRE(!context->set_graphics_root_signature(commands, mirakana::rhi::d3d12::NativeRootSignatureHandle{99}));
    MK_REQUIRE(!context->set_graphics_descriptor_table(commands, root_signature, 1, heap, 0));
    MK_REQUIRE(!context->set_graphics_descriptor_table(commands, root_signature, 0, heap, 4));
    MK_REQUIRE(context->stats().descriptor_heaps_created == 1);
    MK_REQUIRE(context->stats().root_signatures_created == 1);
    MK_REQUIRE(context->stats().descriptor_tables_bound == 0);
}

MK_TEST("d3d12 device context rejects invalid native descriptor writes") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto cbv_heap = context->create_descriptor_heap(mirakana::rhi::d3d12::NativeDescriptorHeapDesc{
        .kind = mirakana::rhi::d3d12::NativeDescriptorHeapKind::cbv_srv_uav,
        .capacity = 1,
        .shader_visible = true,
    });
    const auto sampler_heap = context->create_descriptor_heap(mirakana::rhi::d3d12::NativeDescriptorHeapDesc{
        .kind = mirakana::rhi::d3d12::NativeDescriptorHeapKind::sampler,
        .capacity = 1,
        .shader_visible = true,
    });
    const auto uniform_buffer = context->create_committed_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform,
    });
    const auto sampled_texture = context->create_committed_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });

    MK_REQUIRE(cbv_heap.value == 1);
    MK_REQUIRE(sampler_heap.value == 2);
    MK_REQUIRE(uniform_buffer.value == 1);
    MK_REQUIRE(sampled_texture.value == 2);
    MK_REQUIRE(!context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        cbv_heap,
        1,
        uniform_buffer,
        mirakana::rhi::DescriptorType::uniform_buffer,
    }));
    MK_REQUIRE(!context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        sampler_heap,
        0,
        uniform_buffer,
        mirakana::rhi::DescriptorType::uniform_buffer,
    }));
    MK_REQUIRE(!context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        cbv_heap,
        0,
        uniform_buffer,
        mirakana::rhi::DescriptorType::sampled_texture,
    }));
    MK_REQUIRE(!context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        cbv_heap,
        0,
        sampled_texture,
        mirakana::rhi::DescriptorType::storage_texture,
    }));
    MK_REQUIRE(!context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        cbv_heap,
        0,
        mirakana::rhi::d3d12::NativeResourceHandle{},
        mirakana::rhi::DescriptorType::sampler,
        mirakana::rhi::SamplerDesc{},
    }));
    MK_REQUIRE(!context->write_descriptor(mirakana::rhi::d3d12::NativeDescriptorWriteDesc{
        sampler_heap,
        0,
        sampled_texture,
        mirakana::rhi::DescriptorType::sampled_texture,
    }));
    MK_REQUIRE(context->stats().descriptor_views_written == 0);
}

MK_TEST("d3d12 device context owns shader bytecode and graphics pipeline state") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    const auto root_signature = context->create_root_signature(
        mirakana::rhi::d3d12::NativeRootSignatureDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = context->create_shader_module(mirakana::rhi::d3d12::NativeShaderBytecodeDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .bytecode = vertex_bytecode->GetBufferPointer(),
        .bytecode_size = vertex_bytecode->GetBufferSize(),
    });
    const auto pixel_shader = context->create_shader_module(mirakana::rhi::d3d12::NativeShaderBytecodeDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .bytecode = pixel_bytecode->GetBufferPointer(),
        .bytecode_size = pixel_bytecode->GetBufferSize(),
    });
    const auto pipeline = context->create_graphics_pipeline(mirakana::rhi::d3d12::NativeGraphicsPipelineDesc{
        .root_signature = root_signature,
        .vertex_shader = vertex_shader,
        .fragment_shader = pixel_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const auto info = context->graphics_pipeline_info(pipeline);

    MK_REQUIRE(root_signature.value == 1);
    MK_REQUIRE(vertex_shader.value == 1);
    MK_REQUIRE(pixel_shader.value == 2);
    MK_REQUIRE(pipeline.value == 1);
    MK_REQUIRE(info.valid);
    MK_REQUIRE(info.root_signature.value == root_signature.value);
    MK_REQUIRE(info.color_format == mirakana::rhi::Format::bgra8_unorm);
    MK_REQUIRE(info.topology == mirakana::rhi::PrimitiveTopology::triangle_list);
    MK_REQUIRE(context->stats().shader_modules_created == 2);
    MK_REQUIRE(context->stats().shader_bytecode_bytes_owned > 0);
    MK_REQUIRE(context->stats().graphics_pipelines_created == 1);
}

MK_TEST("d3d12 device context records first triangle draw to hwnd swapchain") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    const auto swapchain = context->create_swapchain_for_window(mirakana::rhi::d3d12::NativeSwapchainDesc{
        .window = mirakana::rhi::d3d12::NativeWindowHandle{window.hwnd()},
        .swapchain =
            mirakana::rhi::SwapchainDesc{
                .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
                .format = mirakana::rhi::Format::bgra8_unorm,
                .buffer_count = 2,
                .vsync = false,
            },
    });
    const auto root_signature = context->create_root_signature(
        mirakana::rhi::d3d12::NativeRootSignatureDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = context->create_shader_module(mirakana::rhi::d3d12::NativeShaderBytecodeDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .bytecode = vertex_bytecode->GetBufferPointer(),
        .bytecode_size = vertex_bytecode->GetBufferSize(),
    });
    const auto pixel_shader = context->create_shader_module(mirakana::rhi::d3d12::NativeShaderBytecodeDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .bytecode = pixel_bytecode->GetBufferPointer(),
        .bytecode_size = pixel_bytecode->GetBufferSize(),
    });
    const auto pipeline = context->create_graphics_pipeline(mirakana::rhi::d3d12::NativeGraphicsPipelineDesc{
        .root_signature = root_signature,
        .vertex_shader = vertex_shader,
        .fragment_shader = pixel_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(swapchain.value == 1);
    MK_REQUIRE(root_signature.value == 1);
    MK_REQUIRE(pipeline.value == 1);
    MK_REQUIRE(commands.value == 1);
    MK_REQUIRE(context->transition_swapchain_back_buffer(commands, swapchain, mirakana::rhi::ResourceState::present,
                                                         mirakana::rhi::ResourceState::render_target));
    MK_REQUIRE(context->set_swapchain_render_target(commands, swapchain));
    MK_REQUIRE(context->clear_swapchain_back_buffer(commands, swapchain, 0.0F, 0.0F, 0.0F, 1.0F));
    MK_REQUIRE(context->set_graphics_root_signature(commands, root_signature));
    MK_REQUIRE(context->bind_graphics_pipeline(commands, pipeline));
    MK_REQUIRE(context->draw(commands, 3, 1));
    MK_REQUIRE(context->transition_swapchain_back_buffer(
        commands, swapchain, mirakana::rhi::ResourceState::render_target, mirakana::rhi::ResourceState::present));
    MK_REQUIRE(context->close_command_list(commands));

    const auto fence = context->execute_command_list(commands);

    MK_REQUIRE(fence.value == 1);
    MK_REQUIRE(context->wait_for_fence(fence, 1000));
    MK_REQUIRE(context->present_swapchain(swapchain));
    MK_REQUIRE(context->stats().swapchain_render_targets_set == 1);
    MK_REQUIRE(context->stats().graphics_pipelines_bound == 1);
    MK_REQUIRE(context->stats().draw_calls == 1);
    MK_REQUIRE(context->stats().vertices_submitted == 3);
}

MK_TEST("d3d12 device context rejects invalid graphics pipeline and draw work") {
    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    const auto root_signature = context->create_root_signature(
        mirakana::rhi::d3d12::NativeRootSignatureDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = context->create_shader_module(mirakana::rhi::d3d12::NativeShaderBytecodeDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .bytecode = vertex_bytecode->GetBufferPointer(),
        .bytecode_size = vertex_bytecode->GetBufferSize(),
    });
    const auto pixel_shader = context->create_shader_module(mirakana::rhi::d3d12::NativeShaderBytecodeDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .bytecode = pixel_bytecode->GetBufferPointer(),
        .bytecode_size = pixel_bytecode->GetBufferSize(),
    });
    const auto invalid_shader = context->create_shader_module(mirakana::rhi::d3d12::NativeShaderBytecodeDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .bytecode = nullptr,
        .bytecode_size = vertex_bytecode->GetBufferSize(),
    });
    const auto invalid_pipeline = context->create_graphics_pipeline(mirakana::rhi::d3d12::NativeGraphicsPipelineDesc{
        .root_signature = root_signature,
        .vertex_shader = pixel_shader,
        .fragment_shader = vertex_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const auto pipeline = context->create_graphics_pipeline(mirakana::rhi::d3d12::NativeGraphicsPipelineDesc{
        .root_signature = root_signature,
        .vertex_shader = vertex_shader,
        .fragment_shader = pixel_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(invalid_shader.value == 0);
    MK_REQUIRE(invalid_pipeline.value == 0);
    MK_REQUIRE(pipeline.value == 1);
    MK_REQUIRE(commands.value == 1);
    MK_REQUIRE(!context->bind_graphics_pipeline(commands, pipeline));
    MK_REQUIRE(!context->draw(commands, 3, 1));
    MK_REQUIRE(context->set_graphics_root_signature(commands, root_signature));
    MK_REQUIRE(!context->bind_graphics_pipeline(commands, mirakana::rhi::d3d12::NativeGraphicsPipelineHandle{99}));
    MK_REQUIRE(context->bind_graphics_pipeline(commands, pipeline));
    MK_REQUIRE(!context->draw(commands, 0, 1));
    MK_REQUIRE(!context->draw(commands, 3, 0));
    MK_REQUIRE(context->stats().draw_calls == 0);
}

MK_TEST("d3d12 rhi device exposes backend neutral resources and pipeline creation") {
    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);
    MK_REQUIRE(device->backend_kind() == mirakana::rhi::BackendKind::d3d12);
    MK_REQUIRE(device->backend_name() == "d3d12");

    const auto buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform | mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    MK_REQUIRE(buffer.value == 1);
    MK_REQUIRE(texture.value == 1);
    MK_REQUIRE(vertex_shader.value == 1);
    MK_REQUIRE(fragment_shader.value == 2);
    MK_REQUIRE(layout.value == 1);
    MK_REQUIRE(pipeline.value == 1);
    MK_REQUIRE(device->stats().buffers_created == 1);
    MK_REQUIRE(device->stats().textures_created == 1);
    MK_REQUIRE(device->stats().shader_modules_created == 2);
    MK_REQUIRE(device->stats().pipeline_layouts_created == 1);
    MK_REQUIRE(device->stats().graphics_pipelines_created == 1);
}

MK_TEST("d3d12 rhi device dispatches a compute shader and reads back storage buffer bytes") {
    const auto compute_bytecode = compile_uav_write_compute_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto output = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 16,
        .usage = mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 16,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto set_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::storage_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::compute,
        },
    }});
    const auto descriptor_set = device->allocate_descriptor_set(set_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::storage_buffer, output)},
    });
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {set_layout}, .push_constant_bytes = 0});
    const auto shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "cs_main",
        .bytecode_size = compute_bytecode->GetBufferSize(),
        .bytecode = compute_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = layout,
        .compute_shader = shader,
    });

    auto compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->bind_compute_pipeline(pipeline);
    compute_commands->bind_descriptor_set(layout, 0, descriptor_set);
    compute_commands->dispatch(4, 1, 1);
    compute_commands->close();
    const auto compute_fence = device->submit(*compute_commands);
    device->wait(compute_fence);

    auto copy_commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);
    copy_commands->copy_buffer(
        output, readback,
        mirakana::rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = 16});
    copy_commands->close();
    const auto copy_fence = device->submit(*copy_commands);
    device->wait(copy_fence);

    const auto bytes = device->read_buffer(readback, 0, 16);

    MK_REQUIRE(bytes.size() == 16);
    MK_REQUIRE(bytes[0] == 0x00);
    MK_REQUIRE(bytes[1] == 0xC0);
    MK_REQUIRE(bytes[2] == 0xB0);
    MK_REQUIRE(bytes[3] == 0xA0);
    MK_REQUIRE(bytes[4] == 0x01);
    MK_REQUIRE(bytes[5] == 0xC0);
    MK_REQUIRE(bytes[6] == 0xB0);
    MK_REQUIRE(bytes[7] == 0xA0);
    MK_REQUIRE(bytes[12] == 0x03);
    MK_REQUIRE(bytes[13] == 0xC0);
    MK_REQUIRE(bytes[14] == 0xB0);
    MK_REQUIRE(bytes[15] == 0xA0);
    MK_REQUIRE(device->stats().compute_pipelines_created == 1);
    MK_REQUIRE(device->stats().compute_pipelines_bound == 1);
    MK_REQUIRE(device->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(device->stats().compute_dispatches == 1);
    MK_REQUIRE(device->stats().compute_workgroups_x == 4);
    MK_REQUIRE(device->stats().buffer_copies == 1);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi compute morph writes morphed runtime positions") {
    const auto compute_bytecode = compile_runtime_morph_position_compute_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    append_le_f32(vertex_bytes, 1.0F);
    append_le_f32(vertex_bytes, 2.0F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -2.0F);
    append_le_f32(vertex_bytes, 4.0F);
    append_le_f32(vertex_bytes, 0.5F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -1.0F);
    append_le_f32(vertex_bytes, 2.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_base_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{25},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target0;
    mirakana::MorphMeshCpuTargetSourceDocument target1;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_le_f32(target0.position_delta_bytes, 1.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 2.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
    }
    morph.targets.push_back(std::move(target0));
    morph.targets.push_back(std::move(target1));
    append_le_f32(morph.target_weight_bytes, 0.25F);
    append_le_f32(morph.target_weight_bytes, 0.5F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{26},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());
    MK_REQUIRE(morph_upload.target_count == 2);

    const auto compute_binding =
        mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(*device, mesh_upload, morph_upload);
    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.output_position_bytes == 36);

    const auto layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "cs_main",
        .bytecode_size = compute_bytecode->GetBufferSize(),
        .bytecode = compute_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = layout,
        .compute_shader = shader,
    });

    auto compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->bind_compute_pipeline(pipeline);
    compute_commands->bind_descriptor_set(layout, 0, compute_binding.descriptor_set);
    compute_commands->dispatch(compute_binding.vertex_count, 1, 1);
    compute_commands->close();
    const auto compute_fence = device->submit(*compute_commands);
    device->wait(compute_fence);

    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = compute_binding.output_position_bytes,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    auto copy_commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);
    copy_commands->copy_buffer(compute_binding.output_position_buffer, readback,
                               mirakana::rhi::BufferCopyRegion{.source_offset = 0,
                                                               .destination_offset = 0,
                                                               .size_bytes = compute_binding.output_position_bytes});
    copy_commands->close();
    const auto copy_fence = device->submit(*copy_commands);
    device->wait(copy_fence);

    const auto bytes = device->read_buffer(readback, 0, compute_binding.output_position_bytes);

    MK_REQUIRE(bytes.size() == 36);
    MK_REQUIRE(read_le_f32(bytes, 0) == 1.25F);
    MK_REQUIRE(read_le_f32(bytes, 4) == 3.0F);
    MK_REQUIRE(read_le_f32(bytes, 8) == 0.0F);
    MK_REQUIRE(read_le_f32(bytes, 12) == -1.75F);
    MK_REQUIRE(read_le_f32(bytes, 16) == 5.0F);
    MK_REQUIRE(read_le_f32(bytes, 20) == 0.5F);
    MK_REQUIRE(read_le_f32(bytes, 24) == 0.25F);
    MK_REQUIRE(read_le_f32(bytes, 28) == 0.0F);
    MK_REQUIRE(read_le_f32(bytes, 32) == 2.0F);
    MK_REQUIRE(device->stats().compute_pipelines_created == 1);
    MK_REQUIRE(device->stats().compute_dispatches == 1);
    MK_REQUIRE(device->stats().compute_workgroups_x == 3);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi compute morph output ring writes a selected position slot") {
    const auto compute_bytecode = compile_runtime_morph_position_output_slot_compute_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    append_le_f32(vertex_bytes, 1.0F);
    append_le_f32(vertex_bytes, 2.0F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -2.0F);
    append_le_f32(vertex_bytes, 4.0F);
    append_le_f32(vertex_bytes, 0.5F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -1.0F);
    append_le_f32(vertex_bytes, 2.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_output_ring_base_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{125},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target0;
    mirakana::MorphMeshCpuTargetSourceDocument target1;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_le_f32(target0.position_delta_bytes, 1.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 2.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
    }
    morph.targets.push_back(std::move(target0));
    morph.targets.push_back(std::move(target1));
    append_le_f32(morph.target_weight_bytes, 0.25F);
    append_le_f32(morph.target_weight_bytes, 0.5F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_output_ring_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{126},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());
    MK_REQUIRE(morph_upload.target_count == 2);

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_slot_count = 2;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        *device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.output_slots.size() == 2);
    MK_REQUIRE(compute_binding.output_slots[0].output_position_buffer.value !=
               compute_binding.output_slots[1].output_position_buffer.value);

    const auto layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "cs_main",
        .bytecode_size = compute_bytecode->GetBufferSize(),
        .bytecode = compute_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = layout,
        .compute_shader = shader,
    });

    auto compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->bind_compute_pipeline(pipeline);
    compute_commands->bind_descriptor_set(layout, 0, compute_binding.descriptor_set);
    compute_commands->dispatch(compute_binding.vertex_count, 1, 1);
    compute_commands->close();
    const auto compute_fence = device->submit(*compute_commands);
    device->wait(compute_fence);

    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = compute_binding.output_slots[1].output_position_bytes,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    auto copy_commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);
    copy_commands->copy_buffer(
        compute_binding.output_slots[1].output_position_buffer, readback,
        mirakana::rhi::BufferCopyRegion{.source_offset = 0,
                                        .destination_offset = 0,
                                        .size_bytes = compute_binding.output_slots[1].output_position_bytes});
    copy_commands->close();
    const auto copy_fence = device->submit(*copy_commands);
    device->wait(copy_fence);

    const auto bytes = device->read_buffer(readback, 0, compute_binding.output_slots[1].output_position_bytes);

    MK_REQUIRE(bytes.size() == 36);
    MK_REQUIRE(read_le_f32(bytes, 0) == 1.25F);
    MK_REQUIRE(read_le_f32(bytes, 4) == 3.0F);
    MK_REQUIRE(read_le_f32(bytes, 8) == 0.0F);
    MK_REQUIRE(read_le_f32(bytes, 12) == -1.75F);
    MK_REQUIRE(read_le_f32(bytes, 16) == 5.0F);
    MK_REQUIRE(read_le_f32(bytes, 20) == 0.5F);
    MK_REQUIRE(read_le_f32(bytes, 24) == 0.25F);
    MK_REQUIRE(read_le_f32(bytes, 28) == 0.0F);
    MK_REQUIRE(read_le_f32(bytes, 32) == 2.0F);
    MK_REQUIRE(device->stats().compute_pipelines_created == 1);
    MK_REQUIRE(device->stats().compute_dispatches == 1);
    MK_REQUIRE(device->stats().compute_workgroups_x == 3);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi compute morph writes morphed runtime normals and tangents") {
    const auto compute_bytecode = compile_runtime_morph_tangent_frame_compute_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    auto append_lit_vertex = [&vertex_bytes](float x, float y, float z) {
        append_le_f32(vertex_bytes, x);
        append_le_f32(vertex_bytes, y);
        append_le_f32(vertex_bytes, z);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
        append_le_f32(vertex_bytes, 0.5F);
        append_le_f32(vertex_bytes, 0.5F);
        append_le_f32(vertex_bytes, 1.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
    };
    append_lit_vertex(-0.6F, 0.75F, 0.0F);
    append_lit_vertex(0.15F, -0.75F, 0.0F);
    append_lit_vertex(-1.35F, -0.75F, 0.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_mesh_tangent_space_vertex_stride_bytes);

    std::vector<std::uint8_t> bind_positions;
    append_le_f32(bind_positions, -0.6F);
    append_le_f32(bind_positions, 0.75F);
    append_le_f32(bind_positions, 0.0F);
    append_le_f32(bind_positions, 0.15F);
    append_le_f32(bind_positions, -0.75F);
    append_le_f32(bind_positions, 0.0F);
    append_le_f32(bind_positions, -1.35F);
    append_le_f32(bind_positions, -0.75F);
    append_le_f32(bind_positions, 0.0F);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_tangent_frame_base_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{29},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = true,
        .has_tangent_frame = true,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = bind_positions;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_le_f32(morph.bind_normal_bytes, 0.0F);
        append_le_f32(morph.bind_normal_bytes, 0.0F);
        append_le_f32(morph.bind_normal_bytes, 1.0F);
        append_le_f32(morph.bind_tangent_bytes, 1.0F);
        append_le_f32(morph.bind_tangent_bytes, 0.0F);
        append_le_f32(morph.bind_tangent_bytes, 0.0F);
    }

    mirakana::MorphMeshCpuTargetSourceDocument target0;
    mirakana::MorphMeshCpuTargetSourceDocument target1;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.normal_delta_bytes, 0.0F);
        append_le_f32(target0.normal_delta_bytes, 1.0F);
        append_le_f32(target0.normal_delta_bytes, -1.0F);
        append_le_f32(target0.tangent_delta_bytes, -1.0F);
        append_le_f32(target0.tangent_delta_bytes, 0.0F);
        append_le_f32(target0.tangent_delta_bytes, 1.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.normal_delta_bytes, 0.0F);
        append_le_f32(target1.normal_delta_bytes, 0.0F);
        append_le_f32(target1.normal_delta_bytes, 0.0F);
        append_le_f32(target1.tangent_delta_bytes, 0.0F);
        append_le_f32(target1.tangent_delta_bytes, 0.0F);
        append_le_f32(target1.tangent_delta_bytes, 0.0F);
    }
    morph.targets.push_back(std::move(target0));
    morph.targets.push_back(std::move(target1));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    append_le_f32(morph.target_weight_bytes, 0.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_tangent_frame_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{30},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());
    MK_REQUIRE(morph_upload.uploaded_normal_delta_bytes == 72);
    MK_REQUIRE(morph_upload.uploaded_tangent_delta_bytes == 72);

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_normal_usage = mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_source;
    compute_options.output_tangent_usage =
        mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_source;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        *device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.output_normal_bytes == 36);
    MK_REQUIRE(compute_binding.output_tangent_bytes == 36);

    const auto layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "cs_main",
        .bytecode_size = compute_bytecode->GetBufferSize(),
        .bytecode = compute_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = layout,
        .compute_shader = shader,
    });

    auto compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->bind_compute_pipeline(pipeline);
    compute_commands->bind_descriptor_set(layout, 0, compute_binding.descriptor_set);
    compute_commands->dispatch(compute_binding.vertex_count, 1, 1);
    compute_commands->close();
    const auto compute_fence = device->submit(*compute_commands);
    device->wait_for_queue(mirakana::rhi::QueueKind::copy, compute_fence);

    const auto normal_readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = compute_binding.output_normal_bytes,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto tangent_readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = compute_binding.output_tangent_bytes,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    auto copy_commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);
    copy_commands->copy_buffer(compute_binding.output_normal_buffer, normal_readback,
                               mirakana::rhi::BufferCopyRegion{.source_offset = 0,
                                                               .destination_offset = 0,
                                                               .size_bytes = compute_binding.output_normal_bytes});
    copy_commands->copy_buffer(compute_binding.output_tangent_buffer, tangent_readback,
                               mirakana::rhi::BufferCopyRegion{.source_offset = 0,
                                                               .destination_offset = 0,
                                                               .size_bytes = compute_binding.output_tangent_bytes});
    copy_commands->close();
    const auto copy_fence = device->submit(*copy_commands);
    device->wait(copy_fence);

    const auto normal_bytes = device->read_buffer(normal_readback, 0, compute_binding.output_normal_bytes);
    const auto tangent_bytes = device->read_buffer(tangent_readback, 0, compute_binding.output_tangent_bytes);

    MK_REQUIRE(normal_bytes.size() == 36);
    MK_REQUIRE(tangent_bytes.size() == 36);
    for (std::uint32_t vertex = 0; vertex < 3; ++vertex) {
        const auto offset = static_cast<std::size_t>(vertex) * 12U;
        MK_REQUIRE(std::abs(read_le_f32(normal_bytes, offset + 0) - 0.0F) < 0.0001F);
        MK_REQUIRE(std::abs(read_le_f32(normal_bytes, offset + 4) - 1.0F) < 0.0001F);
        MK_REQUIRE(std::abs(read_le_f32(normal_bytes, offset + 8) - 0.0F) < 0.0001F);
        MK_REQUIRE(std::abs(read_le_f32(tangent_bytes, offset + 0) - 0.0F) < 0.0001F);
        MK_REQUIRE(std::abs(read_le_f32(tangent_bytes, offset + 4) - 0.0F) < 0.0001F);
        MK_REQUIRE(std::abs(read_le_f32(tangent_bytes, offset + 8) - 1.0F) < 0.0001F);
    }
    MK_REQUIRE(device->stats().compute_pipelines_created == 1);
    MK_REQUIRE(device->stats().compute_dispatches == 1);
    MK_REQUIRE(device->stats().compute_workgroups_x == 3);
    MK_REQUIRE(device->stats().buffer_reads == 2);
}

MK_TEST("d3d12 rhi frame renderer consumes compute morph output positions") {
    const auto compute_bytecode = compile_runtime_morph_position_compute_shader();
    const auto vertex_bytecode = compile_position_input_vertex_shader();
    const auto pixel_bytecode = compile_solid_position_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    append_le_f32(vertex_bytes, -0.6F);
    append_le_f32(vertex_bytes, 0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, 0.15F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -1.35F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_renderer_base_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{27},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target0;
    mirakana::MorphMeshCpuTargetSourceDocument target1;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_le_f32(target0.position_delta_bytes, 0.6F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
    }
    morph.targets.push_back(std::move(target0));
    morph.targets.push_back(std::move(target1));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    append_le_f32(morph.target_weight_bytes, 0.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_renderer_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{28},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        *device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());

    const auto compute_layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto compute_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "cs_main",
        .bytecode_size = compute_bytecode->GetBufferSize(),
        .bytecode = compute_bytecode->GetBufferPointer(),
    });
    const auto compute_pipeline = device->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = compute_layout,
        .compute_shader = compute_shader,
    });

    auto compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->bind_compute_pipeline(compute_pipeline);
    compute_commands->bind_descriptor_set(compute_layout, 0, compute_binding.descriptor_set);
    compute_commands->dispatch(compute_binding.vertex_count, 1, 1);
    compute_commands->close();
    const auto compute_fence = device->submit(*compute_commands);
    device->wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence);

    const auto mesh_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding);
    MK_REQUIRE(mesh_binding.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(mesh_binding.index_buffer.value == mesh_upload.index_buffer.value);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto render_layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto vertex_layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(mesh_payload);
    MK_REQUIRE(vertex_layout.succeeded());
    const auto render_pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = render_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = vertex_layout.vertex_buffers,
        .vertex_attributes = vertex_layout.vertex_attributes,
    });

    auto prepare_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = device->submit(*prepare_commands);
    device->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .graphics_pipeline = render_pipeline,
        .wait_for_completion = true,
    });
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = mesh_payload.asset,
        .material = mirakana::AssetId{},
        .world_from_node = mirakana::Mat4::identity(),
        .mesh_binding = mesh_binding,
    });
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };
    auto readback_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = device->submit(*readback_commands);
    device->wait(readback_fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 50 && bytes.at(center_pixel + 0U) <= 54);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 151 && bytes.at(center_pixel + 1U) <= 155);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 228 && bytes.at(center_pixel + 2U) <= 232);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);

    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(device->stats().compute_dispatches == 1);
    MK_REQUIRE(device->stats().queue_waits == 1);
    MK_REQUIRE(device->stats().queue_wait_failures == 0);
    MK_REQUIRE(device->stats().vertex_buffer_bindings == 1);
    MK_REQUIRE(device->stats().index_buffer_bindings == 1);
    MK_REQUIRE(device->stats().indexed_draw_calls == 1);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi frame renderer composes compute morphed positions with skinned joint palette") {
    const auto compute_bytecode = compile_runtime_morph_position_compute_shader();
    const auto vertex_bytecode = compile_compute_morph_skinned_position_vertex_shader();
    const auto pixel_bytecode = compile_solid_position_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    std::vector<std::uint8_t> position_bytes;
    append_vec3(position_bytes, -1.4F, 0.6F, 0.0F);
    append_vec3(position_bytes, -0.7F, -0.6F, 0.0F);
    append_vec3(position_bytes, -1.9F, -0.6F, 0.0F);

    std::vector<std::uint8_t> skinned_vertex_bytes;
    skinned_vertex_bytes.reserve(3U * mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);
    append_skinned_test_vertex(skinned_vertex_bytes, -1.4F, 0.6F, 0.0F);
    append_skinned_test_vertex(skinned_vertex_bytes, -0.7F, -0.6F, 0.0F);
    append_skinned_test_vertex(skinned_vertex_bytes, -1.9F, -0.6F, 0.0F);
    MK_REQUIRE(skinned_vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    std::vector<std::uint8_t> joint_palette_bytes;
    joint_palette_bytes.reserve(mirakana::runtime_rhi::runtime_skinned_mesh_joint_matrix_bytes);
    append_le_f32(joint_palette_bytes, 1.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 1.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 1.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.6F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 1.0F);
    MK_REQUIRE(joint_palette_bytes.size() == mirakana::runtime_rhi::runtime_skinned_mesh_joint_matrix_bytes);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_skinned_base_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{33},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = position_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    const mirakana::runtime::RuntimeSkinnedMeshPayload skinned_payload{
        .asset = mesh_payload.asset,
        .handle = mirakana::runtime::RuntimeAssetHandle{34},
        .vertex_count = 3,
        .index_count = 3,
        .joint_count = 1,
        .vertex_bytes = skinned_vertex_bytes,
        .index_bytes = index_bytes,
        .joint_palette_bytes = joint_palette_bytes,
    };
    const auto skinned_upload = mirakana::runtime_rhi::upload_runtime_skinned_mesh(*device, skinned_payload);
    MK_REQUIRE(skinned_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = position_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target0;
    mirakana::MorphMeshCpuTargetSourceDocument target1;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_vec3(target0.position_delta_bytes, 0.6F, 0.0F, 0.0F);
        append_vec3(target1.position_delta_bytes, 0.0F, 0.0F, 0.0F);
    }
    morph.targets.push_back(std::move(target0));
    morph.targets.push_back(std::move(target1));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    append_le_f32(morph.target_weight_bytes, 0.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_skinned_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{35},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        *device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());

    const auto compute_layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto compute_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "cs_main",
        .bytecode_size = compute_bytecode->GetBufferSize(),
        .bytecode = compute_bytecode->GetBufferPointer(),
    });
    const auto compute_pipeline = device->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = compute_layout,
        .compute_shader = compute_shader,
    });

    auto compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->bind_compute_pipeline(compute_pipeline);
    compute_commands->bind_descriptor_set(compute_layout, 0, compute_binding.descriptor_set);
    compute_commands->dispatch(compute_binding.vertex_count, 1, 1);
    compute_commands->close();
    const auto compute_fence = device->submit(*compute_commands);
    device->wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence);

    auto skinned_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_skinned_mesh_gpu_binding(skinned_upload, compute_binding);
    MK_REQUIRE(skinned_binding.mesh.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(skinned_binding.skin_attribute_vertex_buffer.value == skinned_upload.vertex_buffer.value);

    mirakana::rhi::DescriptorSetLayoutHandle joint_layout{};
    const auto joint_diagnostic = mirakana::runtime_rhi::attach_skinned_mesh_joint_descriptor_set(
        *device, skinned_upload, skinned_binding, joint_layout);
    MK_REQUIRE(joint_diagnostic.empty());
    MK_REQUIRE(joint_layout.value != 0);
    MK_REQUIRE(skinned_binding.joint_descriptor_set.value != 0);

    const auto material_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto material_buffer = device->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 256, .usage = mirakana::rhi::BufferUsage::uniform});
    const auto material_set = device->allocate_descriptor_set(material_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = material_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                material_buffer)},
    });

    const auto target_texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto render_layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_layout, joint_layout}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_compute_morph_skinned",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto render_pipeline =
        device->create_graphics_pipeline(
            mirakana::rhi::GraphicsPipelineDesc{
                .layout = render_layout,
                .vertex_shader = vertex_shader,
                .fragment_shader = fragment_shader,
                .color_format = mirakana::rhi::Format::rgba8_unorm,
                .depth_format = mirakana::rhi::Format::unknown,
                .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
                .vertex_buffers =
                    std::vector<mirakana::rhi::VertexBufferLayoutDesc>{
                        mirakana::rhi::VertexBufferLayoutDesc{
                            .binding = 0, .stride = mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes},
                        mirakana::rhi::VertexBufferLayoutDesc{
                            .binding = 3, .stride = mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes},
                    },
                .vertex_attributes =
                    std::vector<mirakana::rhi::VertexAttributeDesc>{
                        mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                           .binding = 0,
                                                           .offset = 0,
                                                           .format = mirakana::rhi::VertexFormat::float32x3,
                                                           .semantic = mirakana::rhi::VertexSemantic::position,
                                                           .semantic_index = 0},
                        mirakana::rhi::VertexAttributeDesc{
                            .location = 1,
                            .binding = 3,
                            .offset = 48,
                            .format = mirakana::rhi::VertexFormat::uint16x4,
                            .semantic = mirakana::rhi::VertexSemantic::joint_indices,
                            .semantic_index = 0},
                        mirakana::rhi::VertexAttributeDesc{
                            .location = 2,
                            .binding = 3,
                            .offset = 56,
                            .format = mirakana::rhi::VertexFormat::float32x4,
                            .semantic = mirakana::rhi::VertexSemantic::joint_weights,
                            .semantic_index = 0},
                    },
            });

    auto prepare_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target_texture, mirakana::rhi::ResourceState::copy_source,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = device->submit(*prepare_commands);
    device->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target_texture,
        .graphics_pipeline = render_pipeline,
        .wait_for_completion = true,
        .skinned_graphics_pipeline = render_pipeline,
    });
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = mesh_payload.asset,
        .material = mirakana::AssetId{},
        .world_from_node = mirakana::Mat4::identity(),
        .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = render_layout,
                                                         .descriptor_set = material_set,
                                                         .descriptor_set_index = 0,
                                                         .owner_device = device.get()},
        .gpu_skinning = true,
        .skinned_mesh = skinned_binding,
    });
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };
    auto readback_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target_texture, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target_texture, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = device->submit(*readback_commands);
    device->wait(readback_fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 50 && bytes.at(center_pixel + 0U) <= 54);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 151 && bytes.at(center_pixel + 1U) <= 155);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 228 && bytes.at(center_pixel + 2U) <= 232);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);

    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(renderer.stats().gpu_skinning_draws == 1);
    MK_REQUIRE(renderer.stats().skinned_palette_descriptor_binds == 1);
    MK_REQUIRE(device->stats().compute_dispatches == 1);
    MK_REQUIRE(device->stats().queue_waits == 1);
    MK_REQUIRE(device->stats().vertex_buffer_bindings == 2);
    MK_REQUIRE(device->stats().index_buffer_bindings == 1);
    MK_REQUIRE(device->stats().indexed_draw_calls == 1);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi device submits closed command lists and waits for fences") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(commands != nullptr);
    MK_REQUIRE(commands->queue_kind() == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(!commands->closed());

    commands->close();
    const auto fence = device->submit(*commands);
    device->wait(fence);

    MK_REQUIRE(fence.value == 1);
    MK_REQUIRE(device->stats().command_lists_begun == 1);
    MK_REQUIRE(device->stats().command_lists_submitted == 1);
    MK_REQUIRE(device->stats().fences_signaled == 1);
    MK_REQUIRE(device->stats().fence_waits == 1);
    MK_REQUIRE(device->stats().fence_wait_failures == 0);
    MK_REQUIRE(device->stats().last_submitted_fence_value == 1);
    MK_REQUIRE(device->stats().last_completed_fence_value >= fence.value);
}

MK_TEST("d3d12 rhi device reports invalid fence wait attempts") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    bool rejected_unsubmitted_fence = false;
    try {
        device->wait(mirakana::rhi::FenceValue{.value = 1});
    } catch (const std::logic_error&) {
        rejected_unsubmitted_fence = true;
    }

    MK_REQUIRE(rejected_unsubmitted_fence);
    MK_REQUIRE(device->stats().fence_waits == 1);
    MK_REQUIRE(device->stats().fence_wait_failures == 1);
    MK_REQUIRE(device->stats().last_submitted_fence_value == 0);
    MK_REQUIRE(device->stats().last_completed_fence_value == 0);
}

MK_TEST("d3d12 rhi device synchronizes graphics queue with compute fence") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    auto compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->close();
    const auto compute_fence = device->submit(*compute_commands);

    device->wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence);

    auto graphics_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    graphics_commands->close();
    const auto graphics_fence = device->submit(*graphics_commands);
    device->wait(graphics_fence);

    MK_REQUIRE(device->stats().queue_waits == 1);
    MK_REQUIRE(device->stats().queue_wait_failures == 0);
    MK_REQUIRE(device->stats().compute_queue_submits == 1);
    MK_REQUIRE(device->stats().graphics_queue_submits == 1);
    MK_REQUIRE(device->stats().last_compute_submitted_fence_value == compute_fence.value);
    MK_REQUIRE(device->stats().last_graphics_queue_wait_fence_value == compute_fence.value);
    MK_REQUIRE(device->stats().last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(device->stats().last_queue_wait_fence_value == compute_fence.value);
    MK_REQUIRE(device->stats().last_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(device->stats().last_graphics_submitted_fence_value == graphics_fence.value);
    MK_REQUIRE(device->stats().last_submitted_fence_value == graphics_fence.value);
    MK_REQUIRE(device->stats().last_submitted_fence_queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(device->stats().last_completed_fence_value >= graphics_fence.value);
    MK_REQUIRE(device->stats().last_compute_submit_sequence < device->stats().last_graphics_queue_wait_sequence);
    MK_REQUIRE(device->stats().last_graphics_queue_wait_sequence < device->stats().last_graphics_submit_sequence);
}

MK_TEST("d3d12 rhi device uses per queue fence identity for colliding fence values") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    auto graphics_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    graphics_commands->close();
    const auto graphics_fence = device->submit(*graphics_commands);

    auto compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->close();
    const auto compute_fence = device->submit(*compute_commands);

    auto copy_commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);
    copy_commands->close();
    const auto copy_fence = device->submit(*copy_commands);

    MK_REQUIRE(graphics_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(compute_fence.queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(copy_fence.queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(graphics_fence.value == 1);
    MK_REQUIRE(compute_fence.value == 1);
    MK_REQUIRE(copy_fence.value == 1);

    device->wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence);
    auto graphics_after_wait = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    graphics_after_wait->close();
    const auto graphics_after_wait_fence = device->submit(*graphics_after_wait);
    device->wait(graphics_after_wait_fence);

    MK_REQUIRE(graphics_after_wait_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(graphics_after_wait_fence.value == 2);
    MK_REQUIRE(device->stats().last_graphics_queue_wait_fence_value == compute_fence.value);
    MK_REQUIRE(device->stats().last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(device->stats().last_graphics_submitted_fence_value == graphics_after_wait_fence.value);
    MK_REQUIRE(device->stats().last_compute_submit_sequence < device->stats().last_graphics_queue_wait_sequence);
    MK_REQUIRE(device->stats().last_graphics_queue_wait_sequence < device->stats().last_graphics_submit_sequence);
}

MK_TEST("d3d12 rhi device reports compute graphics async overlap as serial dependency when graphics waits") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    auto compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->close();
    const auto compute_fence = device->submit(*compute_commands);

    device->wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence);

    auto graphics_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    graphics_commands->close();
    const auto graphics_fence = device->submit(*graphics_commands);
    device->wait(graphics_fence);

    const auto diagnostics = mirakana::rhi::diagnose_compute_graphics_async_overlap_readiness(
        device->stats(), device->gpu_timestamp_ticks_per_second());

    MK_REQUIRE(diagnostics.status == mirakana::rhi::RhiAsyncOverlapReadinessStatus::not_proven_serial_dependency);
    MK_REQUIRE(diagnostics.compute_queue_submitted);
    MK_REQUIRE(diagnostics.graphics_queue_waited_for_compute);
    MK_REQUIRE(diagnostics.graphics_queue_submitted_after_wait);
    MK_REQUIRE(diagnostics.same_frame_graphics_wait_serializes_compute);
    MK_REQUIRE(diagnostics.last_compute_submitted_fence_value == compute_fence.value);
    MK_REQUIRE(diagnostics.last_graphics_queue_wait_fence_value == compute_fence.value);
    MK_REQUIRE(diagnostics.last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(diagnostics.last_graphics_submitted_fence_value == graphics_fence.value);
    MK_REQUIRE(diagnostics.last_graphics_queue_wait_sequence < diagnostics.last_graphics_submit_sequence);
    MK_REQUIRE(diagnostics.gpu_timestamps_available == (device->gpu_timestamp_ticks_per_second() > 0));
}

MK_TEST("d3d12 rhi device reports pipelined compute graphics output slot scheduling as a timing candidate") {
    const auto previous_slot_bytecode = compile_runtime_morph_position_output_slot_compute_shader(0);
    const auto current_slot_bytecode = compile_runtime_morph_position_output_slot_compute_shader(1);
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    append_le_f32(vertex_bytes, 1.0F);
    append_le_f32(vertex_bytes, 2.0F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -2.0F);
    append_le_f32(vertex_bytes, 4.0F);
    append_le_f32(vertex_bytes, 0.5F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -1.0F);
    append_le_f32(vertex_bytes, 2.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_pipelined_schedule_base_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{127},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target0;
    mirakana::MorphMeshCpuTargetSourceDocument target1;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_le_f32(target0.position_delta_bytes, 1.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target0.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
        append_le_f32(target1.position_delta_bytes, 2.0F);
        append_le_f32(target1.position_delta_bytes, 0.0F);
    }
    morph.targets.push_back(std::move(target0));
    morph.targets.push_back(std::move(target1));
    append_le_f32(morph.target_weight_bytes, 0.25F);
    append_le_f32(morph.target_weight_bytes, 0.5F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_pipelined_schedule_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{128},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    compute_options.output_slot_count = 2;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        *device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.output_slots.size() == 2);

    const auto layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto previous_slot_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "cs_main",
        .bytecode_size = previous_slot_bytecode->GetBufferSize(),
        .bytecode = previous_slot_bytecode->GetBufferPointer(),
    });
    const auto current_slot_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "cs_main",
        .bytecode_size = current_slot_bytecode->GetBufferSize(),
        .bytecode = current_slot_bytecode->GetBufferPointer(),
    });
    const auto previous_slot_pipeline = device->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = layout,
        .compute_shader = previous_slot_shader,
    });
    const auto current_slot_pipeline = device->create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = layout,
        .compute_shader = current_slot_shader,
    });
    const auto stats_before_schedule = device->stats();

    auto previous_compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    previous_compute_commands->bind_compute_pipeline(previous_slot_pipeline);
    previous_compute_commands->bind_descriptor_set(layout, 0, compute_binding.descriptor_set);
    previous_compute_commands->dispatch(compute_binding.vertex_count, 1, 1);
    previous_compute_commands->close();
    const auto previous_compute_fence = device->submit(*previous_compute_commands);
    device->wait(previous_compute_fence);

    auto current_compute_commands = device->begin_command_list(mirakana::rhi::QueueKind::compute);
    current_compute_commands->bind_compute_pipeline(current_slot_pipeline);
    current_compute_commands->bind_descriptor_set(layout, 0, compute_binding.descriptor_set);
    current_compute_commands->dispatch(compute_binding.vertex_count, 1, 1);
    current_compute_commands->close();
    const auto current_compute_fence = device->submit(*current_compute_commands);

    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = compute_binding.output_slots[0].output_position_bytes,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    device->wait_for_queue(mirakana::rhi::QueueKind::graphics, previous_compute_fence);

    auto graphics_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    graphics_commands->copy_buffer(
        compute_binding.output_slots[0].output_position_buffer, readback,
        mirakana::rhi::BufferCopyRegion{.source_offset = 0,
                                        .destination_offset = 0,
                                        .size_bytes = compute_binding.output_slots[0].output_position_bytes});
    graphics_commands->close();
    const auto graphics_fence = device->submit(*graphics_commands);
    device->wait(graphics_fence);

    const mirakana::rhi::RhiPipelinedComputeGraphicsScheduleEvidence schedule{
        .output_slot_count = 2,
        .current_compute_output_slot_index = 1,
        .graphics_consumed_output_slot_index = 0,
        .previous_compute_fence = previous_compute_fence,
        .current_compute_fence = current_compute_fence,
    };
    const auto timestamp_frequency = device->gpu_timestamp_ticks_per_second();
    MK_REQUIRE(timestamp_frequency > 0);
    const auto diagnostics = mirakana::rhi::diagnose_pipelined_compute_graphics_async_overlap_readiness(
        device->stats(), schedule, timestamp_frequency);

    MK_REQUIRE(diagnostics.status == mirakana::rhi::RhiAsyncOverlapReadinessStatus::ready_for_backend_private_timing);
    MK_REQUIRE(diagnostics.output_ring_available);
    MK_REQUIRE(diagnostics.compute_and_graphics_use_distinct_output_slots);
    MK_REQUIRE(diagnostics.graphics_queue_waited_for_previous_compute);
    MK_REQUIRE(!diagnostics.graphics_queue_waited_for_current_compute);
    MK_REQUIRE(!diagnostics.same_frame_graphics_wait_serializes_compute);
    MK_REQUIRE(diagnostics.previous_compute_submitted_fence_value == previous_compute_fence.value);
    MK_REQUIRE(diagnostics.last_compute_submitted_fence_value == current_compute_fence.value);
    MK_REQUIRE(diagnostics.last_graphics_queue_wait_fence_value == previous_compute_fence.value);
    MK_REQUIRE(diagnostics.last_graphics_submitted_fence_value == graphics_fence.value);

    device->wait(current_compute_fence);
    const auto overlap = mirakana::rhi::d3d12::diagnose_rhi_device_submitted_command_compute_graphics_overlap(
        *device, diagnostics, current_compute_fence, graphics_fence);

    MK_REQUIRE(overlap.schedule_ready);
    MK_REQUIRE(overlap.compute_timing_ready);
    MK_REQUIRE(overlap.graphics_timing_ready);
    MK_REQUIRE(overlap.status == mirakana::rhi::d3d12::QueueCalibratedOverlapStatus::measured_overlapping ||
               overlap.status == mirakana::rhi::d3d12::QueueCalibratedOverlapStatus::measured_non_overlapping);
    MK_REQUIRE(overlap.compute_begin_cpu_timestamp <= overlap.compute_end_cpu_timestamp);
    MK_REQUIRE(overlap.graphics_begin_cpu_timestamp <= overlap.graphics_end_cpu_timestamp);
    MK_REQUIRE(overlap.compute_elapsed_seconds >= 0.0);
    MK_REQUIRE(overlap.graphics_elapsed_seconds >= 0.0);
    MK_REQUIRE(!overlap.diagnostic.empty());

    const auto bytes = device->read_buffer(readback, 0, compute_binding.output_slots[0].output_position_bytes);
    MK_REQUIRE(bytes.size() == 36);
    MK_REQUIRE(read_le_f32(bytes, 0) == 1.25F);
    MK_REQUIRE(read_le_f32(bytes, 4) == 3.0F);
    MK_REQUIRE(read_le_f32(bytes, 8) == 0.0F);
    MK_REQUIRE(read_le_f32(bytes, 12) == -1.75F);
    MK_REQUIRE(read_le_f32(bytes, 16) == 5.0F);
    MK_REQUIRE(read_le_f32(bytes, 20) == 0.5F);
    MK_REQUIRE(read_le_f32(bytes, 24) == 0.25F);
    MK_REQUIRE(read_le_f32(bytes, 28) == 0.0F);
    MK_REQUIRE(read_le_f32(bytes, 32) == 2.0F);
    const auto final_stats = device->stats();
    MK_REQUIRE(final_stats.compute_queue_submits == stats_before_schedule.compute_queue_submits + 2);
    MK_REQUIRE(final_stats.graphics_queue_submits == stats_before_schedule.graphics_queue_submits + 1);
    MK_REQUIRE(final_stats.compute_dispatches == stats_before_schedule.compute_dispatches + 2);
    MK_REQUIRE(final_stats.buffer_reads == stats_before_schedule.buffer_reads + 1);
}

MK_TEST("d3d12 rhi device reports invalid queue wait attempts") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    bool rejected_unsubmitted_fence = false;
    try {
        device->wait_for_queue(mirakana::rhi::QueueKind::graphics, mirakana::rhi::FenceValue{.value = 1});
    } catch (const std::logic_error&) {
        rejected_unsubmitted_fence = true;
    }

    MK_REQUIRE(rejected_unsubmitted_fence);
    MK_REQUIRE(device->stats().queue_waits == 1);
    MK_REQUIRE(device->stats().queue_wait_failures == 1);
    MK_REQUIRE(device->stats().fence_waits == 0);
    MK_REQUIRE(device->stats().last_submitted_fence_value == 0);
    MK_REQUIRE(device->stats().last_completed_fence_value == 0);
}

MK_TEST("d3d12 rhi device rejects swapchain presents outside frame sequencing") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto swapchain = device->create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .surface = mirakana::rhi::SurfaceHandle{reinterpret_cast<std::uintptr_t>(window.hwnd())},
    });

    auto copy_commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);
    const auto frame = device->acquire_swapchain_frame(swapchain);
    bool rejected_copy_queue_present = false;
    try {
        copy_commands->present(frame);
    } catch (const std::logic_error&) {
        rejected_copy_queue_present = true;
    }
    copy_commands->close();

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    bool rejected_present_without_render = false;
    try {
        commands->present(frame);
    } catch (const std::invalid_argument&) {
        rejected_present_without_render = true;
    }

    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = frame,
            },
    });

    bool rejected_present_inside_render_pass = false;
    try {
        commands->present(frame);
    } catch (const std::logic_error&) {
        rejected_present_inside_render_pass = true;
    }

    commands->end_render_pass();
    commands->present(frame);

    bool rejected_duplicate_present = false;
    try {
        commands->present(frame);
    } catch (const std::invalid_argument&) {
        rejected_duplicate_present = true;
    }
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    MK_REQUIRE(swapchain.value != 0);
    MK_REQUIRE(rejected_copy_queue_present);
    MK_REQUIRE(rejected_present_without_render);
    MK_REQUIRE(rejected_present_inside_render_pass);
    MK_REQUIRE(rejected_duplicate_present);
    MK_REQUIRE(device->stats().swapchains_created == 1);
    MK_REQUIRE(device->stats().present_calls == 1);
    MK_REQUIRE(device->stats().command_lists_submitted == 1);
}

MK_TEST("d3d12 rhi device resizes swapchains between submitted frames") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto swapchain = device->create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .surface = mirakana::rhi::SurfaceHandle{reinterpret_cast<std::uintptr_t>(window.hwnd())},
    });

    bool rejected_zero_extent = false;
    try {
        device->resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 0, .height = 64});
    } catch (const std::invalid_argument&) {
        rejected_zero_extent = true;
    }

    device->resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 96, .height = 80});

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    const auto frame = device->acquire_swapchain_frame(swapchain);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = frame,
            },
    });
    commands->end_render_pass();
    commands->present(frame);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    MK_REQUIRE(rejected_zero_extent);
    MK_REQUIRE(device->stats().swapchains_created == 1);
    MK_REQUIRE(device->stats().swapchain_resizes == 1);
    MK_REQUIRE(device->stats().present_calls == 1);
}

MK_TEST("d3d12 rhi device rejects multiple pending frames for one swapchain") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto swapchain = device->create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .surface = mirakana::rhi::SurfaceHandle{reinterpret_cast<std::uintptr_t>(window.hwnd())},
    });

    auto first = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    const auto first_frame = device->acquire_swapchain_frame(swapchain);
    first->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = first_frame,
            },
    });
    first->end_render_pass();

    auto second = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    bool rejected_second_frame = false;
    mirakana::rhi::SwapchainFrameHandle second_frame;
    try {
        second_frame = device->acquire_swapchain_frame(swapchain);
        second->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = mirakana::rhi::TextureHandle{},
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                    .swapchain_frame = second_frame,
                },
        });
    } catch (const std::invalid_argument&) {
        rejected_second_frame = true;
    }

    if (!rejected_second_frame) {
        second->end_render_pass();
        second->present(second_frame);
        second->close();
        first->close();
    } else {
        first->present(first_frame);
        first->close();
        const auto first_fence = device->submit(*first);
        device->wait(first_fence);

        auto third = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
        const auto third_frame = device->acquire_swapchain_frame(swapchain);
        third->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = mirakana::rhi::TextureHandle{},
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                    .swapchain_frame = third_frame,
                },
        });
        third->end_render_pass();
        third->present(third_frame);
        third->close();
        const auto third_fence = device->submit(*third);
        device->wait(third_fence);
    }

    MK_REQUIRE(rejected_second_frame);
    MK_REQUIRE(device->stats().swapchains_created == 1);
    MK_REQUIRE(device->stats().present_calls == 2);
    MK_REQUIRE(device->stats().swapchain_frames_acquired == 2);
    MK_REQUIRE(device->stats().swapchain_frames_released == 2);
}

MK_TEST("d3d12 rhi device releases acquired swapchain frames before submit") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto swapchain = device->create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .surface = mirakana::rhi::SurfaceHandle{reinterpret_cast<std::uintptr_t>(window.hwnd())},
    });

    const auto first_frame = device->acquire_swapchain_frame(swapchain);
    bool rejected_second_acquire = false;
    try {
        (void)device->acquire_swapchain_frame(swapchain);
    } catch (const std::invalid_argument&) {
        rejected_second_acquire = true;
    }

    device->release_swapchain_frame(first_frame);
    const auto second_frame = device->acquire_swapchain_frame(swapchain);
    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = second_frame,
            },
    });
    commands->end_render_pass();
    commands->present(second_frame);
    commands->close();
    const auto fence = device->submit(*commands);
    device->wait(fence);

    MK_REQUIRE(rejected_second_acquire);
    MK_REQUIRE(device->stats().swapchain_frames_acquired == 2);
    MK_REQUIRE(device->stats().swapchain_frames_released == 2);
    MK_REQUIRE(device->stats().present_calls == 1);
}

MK_TEST("d3d12 rhi device records swapchain render pass and first draw") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto swapchain = device->create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .surface = mirakana::rhi::SurfaceHandle{reinterpret_cast<std::uintptr_t>(window.hwnd())},
    });
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    const auto frame = device->acquire_swapchain_frame(swapchain);

    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = frame,
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->present(frame);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    MK_REQUIRE(device->stats().swapchains_created == 1);
    MK_REQUIRE(device->stats().resource_transitions == 2);
    MK_REQUIRE(device->stats().render_passes_begun == 1);
    MK_REQUIRE(device->stats().graphics_pipelines_bound == 1);
    MK_REQUIRE(device->stats().draw_calls == 1);
    MK_REQUIRE(device->stats().vertices_submitted == 3);
    MK_REQUIRE(device->stats().present_calls == 1);
}

MK_TEST("d3d12 rhi frame renderer releases failed swapchain begin once") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto swapchain = device->create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .surface = mirakana::rhi::SurfaceHandle{reinterpret_cast<std::uintptr_t>(window.hwnd())},
    });
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto incompatible_pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = mirakana::rhi::TextureHandle{},
        .swapchain = swapchain,
        .graphics_pipeline = incompatible_pipeline,
        .wait_for_completion = true,
    });

    bool rejected_incompatible_pipeline = false;
    try {
        renderer.begin_frame();
        renderer.draw_mesh(mirakana::MeshCommand{});
        renderer.end_frame();
    } catch (const std::runtime_error& ex) {
        rejected_incompatible_pipeline =
            std::string_view{ex.what()}.find("d3d12 rhi graphics pipeline format must match the active render pass") !=
            std::string_view::npos;
    } catch (const std::invalid_argument&) {
        rejected_incompatible_pipeline = true;
    }

    MK_REQUIRE(rejected_incompatible_pipeline);
    MK_REQUIRE(!renderer.frame_active());
    MK_REQUIRE(renderer.stats().frames_started == 1);
    MK_REQUIRE(renderer.stats().frames_finished == 0);
    MK_REQUIRE(renderer.stats().framegraph_passes_executed == 0);
    MK_REQUIRE(device->stats().swapchain_frames_acquired == 1);
    MK_REQUIRE(device->stats().swapchain_frames_released == 1);
    MK_REQUIRE(device->stats().graphics_pipelines_bound == 0);
}

MK_TEST("d3d12 rhi device records texture render pass and first draw") {
    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);

    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    MK_REQUIRE(device->stats().textures_created == 1);
    MK_REQUIRE(device->stats().render_passes_begun == 1);
    MK_REQUIRE(device->stats().graphics_pipelines_bound == 1);
    MK_REQUIRE(device->stats().draw_calls == 1);
    MK_REQUIRE(device->stats().vertices_submitted == 3);
    MK_REQUIRE(device->stats().present_calls == 0);
}

MK_TEST("d3d12 rhi device clears texture render targets to requested color bytes") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 1024,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 4,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
    };

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color =
                    mirakana::rhi::ClearColorValue{.red = 0.25F, .green = 0.5F, .blue = 0.75F, .alpha = 1.0F},
            },
    });
    commands->end_render_pass();
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(target, readback, footprint);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    const auto bytes = device->read_buffer(readback, 0, 1024);
    MK_REQUIRE(bytes.size() == 1024);
    MK_REQUIRE(bytes.at(0) >= 63 && bytes.at(0) <= 65);
    MK_REQUIRE(bytes.at(1) >= 127 && bytes.at(1) <= 129);
    MK_REQUIRE(bytes.at(2) >= 190 && bytes.at(2) <= 192);
    MK_REQUIRE(bytes.at(3) == 255);
    MK_REQUIRE(device->stats().render_passes_begun == 1);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi viewport surface readback reflects renderer clear color") {
    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 4, .height = 4},
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .wait_for_completion = true,
    });

    surface.render_frame(
        mirakana::RhiViewportRenderDesc{.graphics_pipeline = pipeline,
                                        .clear_color = mirakana::Color{.r = 0.0F, .g = 0.25F, .b = 0.5F, .a = 1.0F}},
        [](mirakana::IRenderer&) {});
    const auto frame = surface.readback_color_frame();

    MK_REQUIRE(frame.pixels.size() == static_cast<std::size_t>(frame.row_pitch) * frame.extent.height);
    MK_REQUIRE(frame.pixels.at(0) == 0);
    MK_REQUIRE(frame.pixels.at(1) >= 63 && frame.pixels.at(1) <= 65);
    MK_REQUIRE(frame.pixels.at(2) >= 127 && frame.pixels.at(2) <= 129);
    MK_REQUIRE(frame.pixels.at(3) == 255);
    MK_REQUIRE(device->stats().render_passes_begun == 1);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi device draws first triangle into texture readback bytes") {
    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_solid_orange_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(target, readback, footprint);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 250);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 60 && bytes.at(center_pixel + 1U) <= 68);
    MK_REQUIRE(bytes.at(center_pixel + 2U) <= 5);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    MK_REQUIRE(device->stats().draw_calls == 1);
    MK_REQUIRE(device->stats().vertices_submitted == 3);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi device depth tests overlapping geometry into texture readback bytes") {
    const auto vertex_bytecode = compile_depth_order_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto depth = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    mirakana::rhi::GraphicsPipelineDesc pipeline_desc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto pipeline = device->create_graphics_pipeline(pipeline_desc);
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->draw(6, 1);
    commands->end_render_pass();
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(target, readback, footprint);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) <= 5);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 250);
    MK_REQUIRE(bytes.at(center_pixel + 2U) <= 5);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    MK_REQUIRE(device->stats().draw_calls == 1);
    MK_REQUIRE(device->stats().vertices_submitted == 6);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi device visibly samples texture and sampler descriptors") {
    const auto vertex_bytecode = compile_textured_triangle_vertex_shader();
    const auto pixel_bytecode = compile_sampled_texture_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto upload = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto sampled_texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto sampler = device->create_sampler(mirakana::rhi::SamplerDesc{
        .min_filter = mirakana::rhi::SamplerFilter::nearest,
        .mag_filter = mirakana::rhi::SamplerFilter::nearest,
        .address_u = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
    });
    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });

    std::array<std::uint8_t, 256> upload_bytes{};
    upload_bytes[0] = 32;
    upload_bytes[1] = 180;
    upload_bytes[2] = 224;
    upload_bytes[3] = 255;
    device->write_buffer(upload, 0, upload_bytes);

    const auto set_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 1,
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto descriptor_set = device->allocate_descriptor_set(set_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 sampled_texture)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 1,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(sampler)},
    });

    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {set_layout}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const mirakana::rhi::BufferTextureCopyRegion upload_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 1,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
    };
    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->copy_buffer_to_texture(upload, sampled_texture, upload_footprint);
    commands->transition_texture(sampled_texture, mirakana::rhi::ResourceState::copy_destination,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->bind_descriptor_set(layout, 0, descriptor_set);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(target, readback, readback_footprint);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 30 && bytes.at(center_pixel + 0U) <= 34);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 178 && bytes.at(center_pixel + 1U) <= 182);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 222 && bytes.at(center_pixel + 2U) <= 226);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    MK_REQUIRE(device->stats().samplers_created == 1);
    MK_REQUIRE(device->stats().descriptor_writes == 2);
    MK_REQUIRE(device->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(device->stats().buffer_texture_copies == 1);
    MK_REQUIRE(device->stats().draw_calls == 1);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
}

MK_TEST("d3d12 rhi device visibly samples depth textures after depth write") {
    const auto depth_vertex_bytecode = compile_depth_order_vertex_shader();
    const auto color_pixel_bytecode = compile_triangle_pixel_shader();
    const auto sample_vertex_bytecode = compile_textured_triangle_vertex_shader();
    const auto sample_pixel_bytecode = compile_sampled_depth_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto depth_color = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto sampled_depth = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto sample_target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto sampler = device->create_sampler(mirakana::rhi::SamplerDesc{
        .min_filter = mirakana::rhi::SamplerFilter::nearest,
        .mag_filter = mirakana::rhi::SamplerFilter::nearest,
        .address_u = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
    });

    const auto sampled_depth_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 1,
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto sampled_depth_set = device->allocate_descriptor_set(sampled_depth_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = sampled_depth_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 sampled_depth)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = sampled_depth_set,
        .binding = 1,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(sampler)},
    });

    const auto depth_layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto sample_layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {sampled_depth_layout}, .push_constant_bytes = 0});
    const auto depth_vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = depth_vertex_bytecode->GetBufferSize(),
        .bytecode = depth_vertex_bytecode->GetBufferPointer(),
    });
    const auto depth_fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = color_pixel_bytecode->GetBufferSize(),
        .bytecode = color_pixel_bytecode->GetBufferPointer(),
    });
    const auto sample_vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = sample_vertex_bytecode->GetBufferSize(),
        .bytecode = sample_vertex_bytecode->GetBufferPointer(),
    });
    const auto sample_fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = sample_pixel_bytecode->GetBufferSize(),
        .bytecode = sample_pixel_bytecode->GetBufferPointer(),
    });

    mirakana::rhi::GraphicsPipelineDesc depth_pipeline_desc{
        .layout = depth_layout,
        .vertex_shader = depth_vertex_shader,
        .fragment_shader = depth_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    depth_pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto depth_pipeline = device->create_graphics_pipeline(depth_pipeline_desc);
    const auto sample_pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = sample_layout,
        .vertex_shader = sample_vertex_shader,
        .fragment_shader = sample_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = depth_color,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = sampled_depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    commands->bind_graphics_pipeline(depth_pipeline);
    commands->draw(6, 1);
    commands->end_render_pass();
    commands->transition_texture(sampled_depth, mirakana::rhi::ResourceState::depth_write,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->transition_texture(sample_target, mirakana::rhi::ResourceState::copy_source,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = sample_target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
    });
    commands->bind_graphics_pipeline(sample_pipeline);
    commands->bind_descriptor_set(sample_layout, 0, sampled_depth_set);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->transition_texture(sample_target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(sample_target, readback, readback_footprint);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 60 && bytes.at(center_pixel + 0U) <= 70);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 60 && bytes.at(center_pixel + 1U) <= 70);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 60 && bytes.at(center_pixel + 2U) <= 70);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    MK_REQUIRE(device->stats().descriptor_writes >= 2);
    MK_REQUIRE(device->stats().descriptor_sets_bound >= 1);
    MK_REQUIRE(device->stats().draw_calls >= 2);
    MK_REQUIRE(device->stats().texture_buffer_copies >= 1);
}

MK_TEST("d3d12 rhi device samples scene depth in a postprocess pass readback") {
    const auto depth_vertex_bytecode = compile_depth_order_vertex_shader();
    const auto color_pixel_bytecode = compile_triangle_pixel_shader();
    const auto postprocess_vertex_bytecode = compile_fullscreen_textured_vertex_shader();
    const auto postprocess_pixel_bytecode = compile_depth_aware_postprocess_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto scene_color = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto scene_depth = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto postprocess_target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto sampler_desc = mirakana::rhi::SamplerDesc{
        .min_filter = mirakana::rhi::SamplerFilter::nearest,
        .mag_filter = mirakana::rhi::SamplerFilter::nearest,
        .address_u = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
    };
    const auto scene_color_sampler = device->create_sampler(sampler_desc);
    const auto scene_depth_sampler = device->create_sampler(sampler_desc);

    const auto postprocess_set_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::postprocess_scene_color_texture_binding(),
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::postprocess_scene_color_sampler_binding(),
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::postprocess_scene_depth_texture_binding(),
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::postprocess_scene_depth_sampler_binding(),
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto postprocess_set = device->allocate_descriptor_set(postprocess_set_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = postprocess_set,
        .binding = mirakana::postprocess_scene_color_texture_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 scene_color)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = postprocess_set,
        .binding = mirakana::postprocess_scene_color_sampler_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(scene_color_sampler)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = postprocess_set,
        .binding = mirakana::postprocess_scene_depth_texture_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 scene_depth)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = postprocess_set,
        .binding = mirakana::postprocess_scene_depth_sampler_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(scene_depth_sampler)},
    });

    const auto scene_layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto postprocess_layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {postprocess_set_layout}, .push_constant_bytes = 0});
    const auto scene_vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = depth_vertex_bytecode->GetBufferSize(),
        .bytecode = depth_vertex_bytecode->GetBufferPointer(),
    });
    const auto scene_fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = color_pixel_bytecode->GetBufferSize(),
        .bytecode = color_pixel_bytecode->GetBufferPointer(),
    });
    const auto postprocess_vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = postprocess_vertex_bytecode->GetBufferSize(),
        .bytecode = postprocess_vertex_bytecode->GetBufferPointer(),
    });
    const auto postprocess_fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = postprocess_pixel_bytecode->GetBufferSize(),
        .bytecode = postprocess_pixel_bytecode->GetBufferPointer(),
    });

    mirakana::rhi::GraphicsPipelineDesc scene_pipeline_desc{
        .layout = scene_layout,
        .vertex_shader = scene_vertex_shader,
        .fragment_shader = scene_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    scene_pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto scene_pipeline = device->create_graphics_pipeline(scene_pipeline_desc);
    const auto postprocess_pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = postprocess_layout,
        .vertex_shader = postprocess_vertex_shader,
        .fragment_shader = postprocess_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = scene_color,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color =
                    mirakana::rhi::ClearColorValue{.red = 0.125F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = scene_depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    commands->bind_graphics_pipeline(scene_pipeline);
    commands->draw(6, 1);
    commands->end_render_pass();
    commands->transition_texture(scene_color, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->transition_texture(scene_depth, mirakana::rhi::ResourceState::depth_write,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->transition_texture(postprocess_target, mirakana::rhi::ResourceState::copy_source,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = postprocess_target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
    });
    commands->bind_graphics_pipeline(postprocess_pipeline);
    commands->bind_descriptor_set(postprocess_layout, 0, postprocess_set);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->transition_texture(scene_depth, mirakana::rhi::ResourceState::shader_read,
                                 mirakana::rhi::ResourceState::depth_write);
    commands->transition_texture(postprocess_target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(postprocess_target, readback, readback_footprint);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    const auto corner_pixel = 0U;
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 60 && bytes.at(center_pixel + 0U) <= 70);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 250);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 188 && bytes.at(center_pixel + 2U) <= 196);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    MK_REQUIRE(bytes.at(corner_pixel + 0U) >= 248);
    MK_REQUIRE(bytes.at(corner_pixel + 1U) <= 5);
    MK_REQUIRE(bytes.at(corner_pixel + 2U) <= 5);
    MK_REQUIRE(bytes.at(corner_pixel + 3U) == 255);
    MK_REQUIRE(device->stats().samplers_created == 2);
    MK_REQUIRE(device->stats().descriptor_writes == 4);
    MK_REQUIRE(device->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(device->stats().draw_calls == 2);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
}

MK_TEST("d3d12 rhi device darkens a directional shadow receiver from sampled depth readback") {
    const auto depth_vertex_bytecode = compile_depth_order_vertex_shader();
    const auto color_pixel_bytecode = compile_triangle_pixel_shader();
    const auto receiver_vertex_bytecode = compile_shadow_receiver_vertex_shader();
    const auto receiver_pixel_bytecode = compile_shadow_receiver_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto shadow_map_plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(shadow_map_plan.succeeded());
    mirakana::DirectionalShadowLightSpacePlan light_space_plan;
    light_space_plan.clip_from_world_cascades.push_back(mirakana::Mat4::identity());
    const auto receiver_plan = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &shadow_map_plan,
        .light_space = &light_space_plan,
        .depth_bias = 0.002F,
        .lit_intensity = 1.0F,
        .shadow_intensity = 0.3F,
    });
    MK_REQUIRE(receiver_plan.succeeded());

    std::array<std::uint8_t, mirakana::shadow_receiver_constants_byte_size()> receiver_constants{};
    mirakana::pack_shadow_receiver_constants(receiver_constants, light_space_plan,
                                             shadow_map_plan.directional_cascade_count, mirakana::Mat4::identity());

    const auto shadow_color = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto shadow_depth = device->create_texture(receiver_plan.depth_texture);
    const auto receiver_target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto receiver_pass_depth = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto sampler = device->create_sampler(receiver_plan.sampler);
    const auto receiver_constants_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::shadow_receiver_constants_byte_size(),
        .usage = mirakana::rhi::BufferUsage::uniform | mirakana::rhi::BufferUsage::copy_source,
    });
    device->write_buffer(receiver_constants_buffer, 0,
                         std::span<const std::uint8_t>(receiver_constants.data(), receiver_constants.size()));
    const auto receiver_layout = device->create_descriptor_set_layout(receiver_plan.descriptor_set_layout);
    const auto receiver_set = device->allocate_descriptor_set(receiver_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = receiver_set,
        .binding = mirakana::shadow_receiver_depth_texture_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 shadow_depth)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = receiver_set,
        .binding = mirakana::shadow_receiver_sampler_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(sampler)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = receiver_set,
        .binding = mirakana::shadow_receiver_constants_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                receiver_constants_buffer)},
    });

    const auto shadow_layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto receiver_pipeline_layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {receiver_layout}, .push_constant_bytes = 0});
    const auto depth_vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = depth_vertex_bytecode->GetBufferSize(),
        .bytecode = depth_vertex_bytecode->GetBufferPointer(),
    });
    const auto depth_fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = color_pixel_bytecode->GetBufferSize(),
        .bytecode = color_pixel_bytecode->GetBufferPointer(),
    });
    const auto receiver_vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = receiver_vertex_bytecode->GetBufferSize(),
        .bytecode = receiver_vertex_bytecode->GetBufferPointer(),
    });
    const auto receiver_fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = receiver_pixel_bytecode->GetBufferSize(),
        .bytecode = receiver_pixel_bytecode->GetBufferPointer(),
    });

    mirakana::rhi::GraphicsPipelineDesc shadow_pipeline_desc{
        .layout = shadow_layout,
        .vertex_shader = depth_vertex_shader,
        .fragment_shader = depth_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    shadow_pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto shadow_pipeline = device->create_graphics_pipeline(shadow_pipeline_desc);
    const auto receiver_pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = receiver_pipeline_layout,
        .vertex_shader = receiver_vertex_shader,
        .fragment_shader = receiver_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {},
        .vertex_attributes = {},
        .depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                            .depth_write_enabled = true,
                                                            .depth_compare = mirakana::rhi::CompareOp::less_equal},
    });
    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = shadow_color,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = shadow_depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    commands->bind_graphics_pipeline(shadow_pipeline);
    commands->draw(6, 1);
    commands->end_render_pass();
    commands->transition_texture(shadow_depth, mirakana::rhi::ResourceState::depth_write,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->transition_texture(receiver_target, mirakana::rhi::ResourceState::copy_source,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = receiver_target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = receiver_pass_depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    commands->bind_graphics_pipeline(receiver_pipeline);
    commands->bind_descriptor_set(receiver_pipeline_layout, 0, receiver_set);
    commands->draw(6, 1);
    commands->end_render_pass();
    commands->transition_texture(receiver_target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(receiver_target, readback, readback_footprint);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 248);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 248);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 248);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);
    MK_REQUIRE(device->stats().samplers_created >= 1);
    MK_REQUIRE(device->stats().descriptor_writes >= 3);
    MK_REQUIRE(device->stats().descriptor_sets_bound >= 1);
    MK_REQUIRE(device->stats().draw_calls >= 2);
    MK_REQUIRE(device->stats().texture_buffer_copies >= 1);
}

MK_TEST("d3d12 rhi device filters a directional shadow receiver from sampled depth readback") {
    const auto depth_vertex_bytecode = compile_vertical_split_shadow_depth_vertex_shader();
    const auto color_pixel_bytecode = compile_triangle_pixel_shader();
    const auto receiver_vertex_bytecode = compile_fullscreen_shadow_receiver_vertex_shader();
    const auto receiver_pixel_bytecode = compile_filtered_shadow_receiver_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto shadow_map_plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(shadow_map_plan.succeeded());
    mirakana::DirectionalShadowLightSpacePlan light_space_plan_b;
    light_space_plan_b.clip_from_world_cascades.push_back(mirakana::Mat4::identity());
    const auto receiver_plan = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &shadow_map_plan,
        .light_space = &light_space_plan_b,
        .depth_bias = 0.002F,
        .lit_intensity = 1.0F,
        .shadow_intensity = 0.3F,
        .filter_mode = mirakana::ShadowReceiverFilterMode::fixed_pcf_3x3,
        .filter_radius_texels = 1.0F,
    });
    MK_REQUIRE(receiver_plan.succeeded());
    MK_REQUIRE(receiver_plan.filter_mode == mirakana::ShadowReceiverFilterMode::fixed_pcf_3x3);
    MK_REQUIRE(receiver_plan.filter_radius_texels == 1.0F);
    MK_REQUIRE(receiver_plan.filter_tap_count == 9);

    std::array<std::uint8_t, mirakana::shadow_receiver_constants_byte_size()> receiver_constants_b{};
    mirakana::pack_shadow_receiver_constants(receiver_constants_b, light_space_plan_b,
                                             shadow_map_plan.directional_cascade_count, mirakana::Mat4::identity());

    const auto shadow_color = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto shadow_depth = device->create_texture(receiver_plan.depth_texture);
    const auto receiver_target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto sampler = device->create_sampler(receiver_plan.sampler);
    const auto receiver_constants_buffer_b = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = mirakana::shadow_receiver_constants_byte_size(),
        .usage = mirakana::rhi::BufferUsage::uniform | mirakana::rhi::BufferUsage::copy_source,
    });
    device->write_buffer(receiver_constants_buffer_b, 0,
                         std::span<const std::uint8_t>(receiver_constants_b.data(), receiver_constants_b.size()));
    const auto receiver_layout = device->create_descriptor_set_layout(receiver_plan.descriptor_set_layout);
    const auto receiver_set = device->allocate_descriptor_set(receiver_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = receiver_set,
        .binding = mirakana::shadow_receiver_depth_texture_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 shadow_depth)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = receiver_set,
        .binding = mirakana::shadow_receiver_sampler_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(sampler)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = receiver_set,
        .binding = mirakana::shadow_receiver_constants_binding(),
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                receiver_constants_buffer_b)},
    });

    const auto shadow_layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto receiver_pipeline_layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {receiver_layout}, .push_constant_bytes = 0});
    const auto depth_vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = depth_vertex_bytecode->GetBufferSize(),
        .bytecode = depth_vertex_bytecode->GetBufferPointer(),
    });
    const auto depth_fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = color_pixel_bytecode->GetBufferSize(),
        .bytecode = color_pixel_bytecode->GetBufferPointer(),
    });
    const auto receiver_vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = receiver_vertex_bytecode->GetBufferSize(),
        .bytecode = receiver_vertex_bytecode->GetBufferPointer(),
    });
    const auto receiver_fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = receiver_pixel_bytecode->GetBufferSize(),
        .bytecode = receiver_pixel_bytecode->GetBufferPointer(),
    });

    mirakana::rhi::GraphicsPipelineDesc shadow_pipeline_desc{
        .layout = shadow_layout,
        .vertex_shader = depth_vertex_shader,
        .fragment_shader = depth_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    shadow_pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto shadow_pipeline = device->create_graphics_pipeline(shadow_pipeline_desc);
    // Receiver pass has no depth attachment: a fullscreen tri at world z=0.55 samples a shadow map with a
    // vertical depth split (~0.2 left, ~0.85 right). PCF softens the terminator at x=0; left stays mostly in shadow,
    // right mostly lit.
    const auto receiver_pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = receiver_pipeline_layout,
        .vertex_shader = receiver_vertex_shader,
        .fragment_shader = receiver_fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = shadow_color,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = shadow_depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{1.0F},
            },
    });
    commands->bind_graphics_pipeline(shadow_pipeline);
    commands->draw(12, 1);
    commands->end_render_pass();
    commands->transition_texture(shadow_depth, mirakana::rhi::ResourceState::depth_write,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->transition_texture(receiver_target, mirakana::rhi::ResourceState::copy_source,
                                 mirakana::rhi::ResourceState::render_target);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = receiver_target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = mirakana::rhi::SwapchainFrameHandle{},
                .clear_color = mirakana::rhi::ClearColorValue{.red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 1.0F},
            },
    });
    commands->bind_graphics_pipeline(receiver_pipeline);
    commands->bind_descriptor_set(receiver_pipeline_layout, 0, receiver_set);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->transition_texture(receiver_target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::copy_source);
    commands->copy_texture_to_buffer(receiver_target, readback, readback_footprint);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    MK_REQUIRE(bytes.size() == 256 * 64);
    const auto row_stride = 256U;
    const auto y = 32U;
    const auto left_px = (y * row_stride) + (8U * 4U);
    const auto right_px = (y * row_stride) + (56U * 4U);
    const auto mid_px = (y * row_stride) + (32U * 4U);
    // Left: receiver z=0.55 vs shadow ~0.2 -> mostly occluded (PCF ~1 -> intensity ~0.3).
    MK_REQUIRE(bytes.at(left_px + 0U) <= 110);
    MK_REQUIRE(bytes.at(left_px + 1U) <= 110);
    MK_REQUIRE(bytes.at(left_px + 2U) <= 110);
    MK_REQUIRE(bytes.at(left_px + 3U) == 255);
    // Right: receiver z=0.55 vs shadow ~0.85 -> mostly lit.
    MK_REQUIRE(bytes.at(right_px + 0U) >= 220);
    MK_REQUIRE(bytes.at(right_px + 1U) >= 220);
    MK_REQUIRE(bytes.at(right_px + 2U) >= 220);
    MK_REQUIRE(bytes.at(right_px + 3U) == 255);
    // Near the vertical shadow-map split, PCF mixes taps so intensity sits strictly between lit and fully shadowed.
    MK_REQUIRE(bytes.at(mid_px + 0U) > 90 && bytes.at(mid_px + 0U) < 220);
    MK_REQUIRE(bytes.at(mid_px + 3U) == 255);
    MK_REQUIRE(device->stats().samplers_created >= 1);
    MK_REQUIRE(device->stats().descriptor_writes >= 3);
    MK_REQUIRE(device->stats().descriptor_sets_bound >= 1);
    MK_REQUIRE(device->stats().draw_calls >= 2);
    MK_REQUIRE(device->stats().texture_buffer_copies >= 1);
}

MK_TEST("d3d12 rhi frame renderer visibly samples runtime material texture bindings") {
    const auto vertex_bytecode = compile_textured_triangle_vertex_shader();
    const auto pixel_bytecode = compile_runtime_material_sampled_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto texture_asset = mirakana::AssetId::from_name("textures/runtime_base_color");
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture_asset,
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .width = 1,
        .height = 1,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 4,
        .bytes = std::vector<std::uint8_t>{64, 200, 80, 255},
    };
    const auto texture_upload = mirakana::runtime_rhi::upload_runtime_texture(*device, payload);
    MK_REQUIRE(texture_upload.succeeded());
    MK_REQUIRE(texture_upload.texture.value != 0);
    MK_REQUIRE(texture_upload.copy_recorded);

    const mirakana::MaterialDefinition material{
        .id = mirakana::AssetId::from_name("materials/runtime_base_color"),
        .name = "RuntimeBaseColor",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.5F, 0.5F, 1.0F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 1.0F,
            },
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_asset}},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);
    const auto material_binding = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
        *device, metadata, material.factors,
        {mirakana::runtime_rhi::RuntimeMaterialTextureResource{.slot = mirakana::MaterialTextureSlot::base_color,
                                                               .texture = texture_upload.texture,
                                                               .owner_device = texture_upload.owner_device}});

    MK_REQUIRE(material_binding.succeeded());
    MK_REQUIRE(material_binding.descriptor_set_layout.value != 0);
    MK_REQUIRE(material_binding.descriptor_set.value != 0);
    MK_REQUIRE(material_binding.samplers.size() == 1);
    MK_REQUIRE(material_binding.writes.size() == 4);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    auto prepare_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = device->submit(*prepare_commands);
    device->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = mirakana::AssetId{},
        .material = material.id,
        .world_from_node = mirakana::Mat4::identity(),
        .mesh_binding = mirakana::MeshGpuBinding{},
        .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = layout,
                                                         .descriptor_set = material_binding.descriptor_set,
                                                         .descriptor_set_index = 0,
                                                         .owner_device = device.get()},
    });
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };
    auto readback_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = device->submit(*readback_commands);
    device->wait(readback_fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 30 && bytes.at(center_pixel + 0U) <= 34);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 98 && bytes.at(center_pixel + 1U) <= 102);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 78 && bytes.at(center_pixel + 2U) <= 82);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);

    MK_REQUIRE(renderer.stats().frames_started == 1);
    MK_REQUIRE(renderer.stats().frames_finished == 1);
    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(device->stats().samplers_created == 1);
    MK_REQUIRE(device->stats().descriptor_writes == 4);
    MK_REQUIRE(device->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(device->stats().buffer_texture_copies == 1);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().draw_calls == 1);
}

MK_TEST("d3d12 rhi frame renderer visibly draws uploaded runtime mesh vertices") {
    const auto vertex_bytecode = compile_position_input_vertex_shader();
    const auto pixel_bytecode = compile_solid_position_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    vertex_bytes.reserve(36);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, 0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, 0.75F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);

    std::vector<std::uint8_t> index_bytes;
    index_bytes.reserve(12);
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mirakana::AssetId::from_name("meshes/uploaded_visible_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{2},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    const auto upload = mirakana::runtime_rhi::upload_runtime_mesh(*device, payload);
    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.vertex_stride == 12);
    MK_REQUIRE(upload.index_format == mirakana::rhi::IndexFormat::uint32);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0, .stride = upload.vertex_stride, .input_rate = mirakana::rhi::VertexInputRate::vertex}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
            .semantic_index = 0,
        }},
    });

    auto prepare_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = device->submit(*prepare_commands);
    device->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = payload.asset,
        .material = mirakana::AssetId{},
        .world_from_node = mirakana::Mat4::identity(),
        .mesh_binding = mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(upload),
        .material_binding = mirakana::MaterialGpuBinding{},
    });
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };
    auto readback_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = device->submit(*readback_commands);
    device->wait(readback_fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 50 && bytes.at(center_pixel + 0U) <= 54);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 151 && bytes.at(center_pixel + 1U) <= 155);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 228 && bytes.at(center_pixel + 2U) <= 232);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);

    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(device->stats().vertex_buffer_bindings == 1);
    MK_REQUIRE(device->stats().index_buffer_bindings == 1);
    MK_REQUIRE(device->stats().indexed_draw_calls == 1);
    MK_REQUIRE(device->stats().indices_submitted == 3);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
}

MK_TEST("d3d12 rhi frame renderer visibly draws uploaded runtime skinned mesh vertices") {
    const auto vertex_bytecode = compile_skinned_position_vertex_shader();
    const auto pixel_bytecode = compile_solid_position_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    vertex_bytes.reserve(3U * mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);
    append_skinned_test_vertex(vertex_bytes, -0.6F, 0.75F, 0.0F);
    append_skinned_test_vertex(vertex_bytes, 0.15F, -0.75F, 0.0F);
    append_skinned_test_vertex(vertex_bytes, -1.35F, -0.75F, 0.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    index_bytes.reserve(12);
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    std::vector<std::uint8_t> joint_palette_bytes;
    joint_palette_bytes.reserve(mirakana::runtime_rhi::runtime_skinned_mesh_joint_matrix_bytes);
    append_le_f32(joint_palette_bytes, 1.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 1.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 1.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.6F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 0.0F);
    append_le_f32(joint_palette_bytes, 1.0F);
    MK_REQUIRE(joint_palette_bytes.size() == mirakana::runtime_rhi::runtime_skinned_mesh_joint_matrix_bytes);

    const mirakana::runtime::RuntimeSkinnedMeshPayload payload{
        .asset = mirakana::AssetId::from_name("meshes/uploaded_visible_skinned_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{3},
        .vertex_count = 3,
        .index_count = 3,
        .joint_count = 1,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
        .joint_palette_bytes = joint_palette_bytes,
    };
    const auto upload = mirakana::runtime_rhi::upload_runtime_skinned_mesh(*device, payload);
    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.vertex_stride == mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);
    MK_REQUIRE(upload.index_format == mirakana::rhi::IndexFormat::uint32);

    auto skinned_binding = mirakana::runtime_rhi::make_runtime_skinned_mesh_gpu_binding(upload);
    mirakana::rhi::DescriptorSetLayoutHandle joint_layout{};
    const auto joint_diagnostic =
        mirakana::runtime_rhi::attach_skinned_mesh_joint_descriptor_set(*device, upload, skinned_binding, joint_layout);
    MK_REQUIRE(joint_diagnostic.empty());
    MK_REQUIRE(joint_layout.value != 0);
    MK_REQUIRE(skinned_binding.joint_descriptor_set.value != 0);

    const auto material_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto material_buffer = device->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 256, .usage = mirakana::rhi::BufferUsage::uniform});
    const auto material_set = device->allocate_descriptor_set(material_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = material_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                material_buffer)},
    });

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_layout, joint_layout}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_skinned",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto vertex_layout = mirakana::runtime_rhi::make_runtime_skinned_mesh_vertex_layout_desc(payload);
    MK_REQUIRE(vertex_layout.succeeded());
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = vertex_layout.vertex_buffers,
        .vertex_attributes = vertex_layout.vertex_attributes,
    });

    auto prepare_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = device->submit(*prepare_commands);
    device->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
        .skinned_graphics_pipeline = pipeline,
    });
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = payload.asset,
        .material = mirakana::AssetId{},
        .world_from_node = mirakana::Mat4::identity(),
        .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = layout,
                                                         .descriptor_set = material_set,
                                                         .descriptor_set_index = 0,
                                                         .owner_device = device.get()},
        .gpu_skinning = true,
        .skinned_mesh = skinned_binding,
    });
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };
    auto readback_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = device->submit(*readback_commands);
    device->wait(readback_fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 50 && bytes.at(center_pixel + 0U) <= 54);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 151 && bytes.at(center_pixel + 1U) <= 155);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 228 && bytes.at(center_pixel + 2U) <= 232);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);

    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(renderer.stats().gpu_skinning_draws == 1);
    MK_REQUIRE(renderer.stats().skinned_palette_descriptor_binds == 1);
    MK_REQUIRE(device->stats().descriptor_sets_bound == 2);
    MK_REQUIRE(device->stats().vertex_buffer_bindings == 1);
    MK_REQUIRE(device->stats().index_buffer_bindings == 1);
    MK_REQUIRE(device->stats().indexed_draw_calls == 1);
    MK_REQUIRE(device->stats().indices_submitted == 3);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi frame renderer visibly draws uploaded runtime morph mesh vertices") {
    const auto vertex_bytecode = compile_morph_position_vertex_shader();
    const auto pixel_bytecode = compile_solid_position_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    vertex_bytes.reserve(3U * mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    append_le_f32(vertex_bytes, -0.6F);
    append_le_f32(vertex_bytes, 0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, 0.15F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    append_le_f32(vertex_bytes, -1.35F);
    append_le_f32(vertex_bytes, -0.75F);
    append_le_f32(vertex_bytes, 0.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    index_bytes.reserve(12);
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/uploaded_visible_morph_base_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{21},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*device, mesh_payload);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument morph_target;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_le_f32(morph_target.position_delta_bytes, 0.6F);
        append_le_f32(morph_target.position_delta_bytes, 0.0F);
        append_le_f32(morph_target.position_delta_bytes, 0.0F);
    }
    morph.targets.push_back(std::move(morph_target));
    append_le_f32(morph.target_weight_bytes, 1.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/uploaded_visible_morph_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{22},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());
    auto morph_binding = mirakana::runtime_rhi::make_runtime_morph_mesh_gpu_binding(morph_upload);
    mirakana::rhi::DescriptorSetLayoutHandle morph_layout{};
    const auto morph_diagnostic =
        mirakana::runtime_rhi::attach_morph_mesh_descriptor_set(*device, morph_upload, morph_binding, morph_layout);
    MK_REQUIRE(morph_diagnostic.empty());
    MK_REQUIRE(morph_layout.value != 0);
    MK_REQUIRE(morph_binding.morph_descriptor_set.value != 0);

    const auto material_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto material_buffer = device->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 256, .usage = mirakana::rhi::BufferUsage::uniform});
    const auto material_set = device->allocate_descriptor_set(material_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = material_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                material_buffer)},
    });

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_layout, morph_layout}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_morph",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto vertex_layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(mesh_payload);
    MK_REQUIRE(vertex_layout.succeeded());
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = vertex_layout.vertex_buffers,
        .vertex_attributes = vertex_layout.vertex_attributes,
    });

    auto prepare_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = device->submit(*prepare_commands);
    device->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
        .morph_graphics_pipeline = pipeline,
    });
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = mesh_payload.asset,
        .material = mirakana::AssetId{},
        .world_from_node = mirakana::Mat4::identity(),
        .mesh_binding = mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(mesh_upload),
        .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = layout,
                                                         .descriptor_set = material_set,
                                                         .descriptor_set_index = 0,
                                                         .owner_device = device.get()},
        .gpu_morphing = true,
        .morph_mesh = morph_binding,
    });
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };
    auto readback_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = device->submit(*readback_commands);
    device->wait(readback_fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 50 && bytes.at(center_pixel + 0U) <= 54);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 151 && bytes.at(center_pixel + 1U) <= 155);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 228 && bytes.at(center_pixel + 2U) <= 232);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);

    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(renderer.stats().gpu_morph_draws == 1);
    MK_REQUIRE(renderer.stats().morph_descriptor_binds == 1);
    MK_REQUIRE(device->stats().descriptor_sets_bound == 2);
    MK_REQUIRE(device->stats().vertex_buffer_bindings == 1);
    MK_REQUIRE(device->stats().index_buffer_bindings == 1);
    MK_REQUIRE(device->stats().indexed_draw_calls == 1);
    MK_REQUIRE(device->stats().indices_submitted == 3);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi frame renderer visibly shades uploaded runtime morph normals") {
    const auto vertex_bytecode = compile_morph_normal_vertex_shader();
    const auto pixel_bytecode = compile_morph_lit_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    std::vector<std::uint8_t> vertex_bytes;
    vertex_bytes.reserve(3U * mirakana::runtime_rhi::runtime_mesh_tangent_space_vertex_stride_bytes);
    const auto append_vertex = [&vertex_bytes](float x, float y, float z) {
        append_le_f32(vertex_bytes, x);
        append_le_f32(vertex_bytes, y);
        append_le_f32(vertex_bytes, z);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
    };
    append_vertex(-0.75F, -0.75F, 0.0F);
    append_vertex(0.75F, -0.75F, 0.0F);
    append_vertex(0.0F, 0.75F, 0.0F);
    MK_REQUIRE(vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_mesh_tangent_space_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    index_bytes.reserve(12);
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/uploaded_visible_morph_normal_triangle"),
        .handle = mirakana::runtime::RuntimeAssetHandle{23},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = true,
        .has_tangent_frame = true,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(*device, mesh_payload);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_le_f32(morph.bind_position_bytes, 0.0F);
        append_le_f32(morph.bind_position_bytes, 0.0F);
        append_le_f32(morph.bind_position_bytes, 0.0F);
        append_le_f32(morph.bind_normal_bytes, 0.0F);
        append_le_f32(morph.bind_normal_bytes, 0.0F);
        append_le_f32(morph.bind_normal_bytes, 1.0F);
        append_le_f32(morph.bind_tangent_bytes, 1.0F);
        append_le_f32(morph.bind_tangent_bytes, 0.0F);
        append_le_f32(morph.bind_tangent_bytes, 0.0F);
    }
    mirakana::MorphMeshCpuTargetSourceDocument morph_target;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_le_f32(morph_target.position_delta_bytes, 0.0F);
        append_le_f32(morph_target.position_delta_bytes, 0.0F);
        append_le_f32(morph_target.position_delta_bytes, 0.0F);
        append_le_f32(morph_target.normal_delta_bytes, 0.0F);
        append_le_f32(morph_target.normal_delta_bytes, 1.0F);
        append_le_f32(morph_target.normal_delta_bytes, -1.0F);
        append_le_f32(morph_target.tangent_delta_bytes, -1.0F);
        append_le_f32(morph_target.tangent_delta_bytes, 1.0F);
        append_le_f32(morph_target.tangent_delta_bytes, 0.0F);
    }
    morph.targets.push_back(std::move(morph_target));
    append_le_f32(morph.target_weight_bytes, 1.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/uploaded_visible_morph_normal_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{24},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(*device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());
    MK_REQUIRE(morph_upload.uploaded_normal_delta_bytes == 36);
    MK_REQUIRE(morph_upload.uploaded_tangent_delta_bytes == 36);
    auto morph_binding = mirakana::runtime_rhi::make_runtime_morph_mesh_gpu_binding(morph_upload);
    mirakana::rhi::DescriptorSetLayoutHandle morph_layout{};
    const auto morph_diagnostic =
        mirakana::runtime_rhi::attach_morph_mesh_descriptor_set(*device, morph_upload, morph_binding, morph_layout);
    MK_REQUIRE(morph_diagnostic.empty());
    MK_REQUIRE(morph_layout.value != 0);
    MK_REQUIRE(morph_binding.normal_delta_buffer.value != 0);
    MK_REQUIRE(morph_binding.tangent_delta_buffer.value != 0);

    const auto material_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto material_buffer = device->create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 256, .usage = mirakana::rhi::BufferUsage::uniform});
    const auto material_set = device->allocate_descriptor_set(material_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = material_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                material_buffer)},
    });

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_layout, morph_layout}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_morph",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto vertex_layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(mesh_payload);
    MK_REQUIRE(vertex_layout.succeeded());
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = vertex_layout.vertex_buffers,
        .vertex_attributes = vertex_layout.vertex_attributes,
    });

    auto prepare_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = device->submit(*prepare_commands);
    device->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
        .morph_graphics_pipeline = pipeline,
    });
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = mesh_payload.asset,
        .material = mirakana::AssetId{},
        .world_from_node = mirakana::Mat4::identity(),
        .mesh_binding = mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(mesh_upload),
        .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = layout,
                                                         .descriptor_set = material_set,
                                                         .descriptor_set_index = 0,
                                                         .owner_device = device.get()},
        .gpu_morphing = true,
        .morph_mesh = morph_binding,
    });
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };
    auto readback_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = device->submit(*readback_commands);
    device->wait(readback_fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 240);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 240);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 240);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);

    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(renderer.stats().gpu_morph_draws == 1);
    MK_REQUIRE(renderer.stats().morph_descriptor_binds == 1);
}

MK_TEST("d3d12 rhi frame renderer visibly draws cooked runtime scene gpu palette") {
    const auto vertex_bytecode = compile_runtime_scene_material_vertex_shader();
    const auto pixel_bytecode = compile_runtime_material_sampled_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto mesh = mirakana::AssetId::from_name("meshes/d3d12_runtime_scene_triangle");
    const auto material = mirakana::AssetId::from_name("materials/d3d12_runtime_scene_material");
    const auto texture = mirakana::AssetId::from_name("textures/d3d12_runtime_scene_base_color");
    const auto package = make_d3d12_runtime_scene_package(mesh, material, texture);
    const auto scene = make_d3d12_runtime_scene(mesh, material);
    const auto packet = mirakana::build_scene_render_packet(scene);

    const auto gpu_bindings =
        mirakana::runtime_scene_rhi::build_runtime_scene_gpu_binding_palette(*device, package, packet);

    MK_REQUIRE(gpu_bindings.succeeded());
    MK_REQUIRE(gpu_bindings.palette.mesh_count() == 1);
    MK_REQUIRE(gpu_bindings.palette.material_count() == 1);
    MK_REQUIRE(gpu_bindings.material_pipeline_layouts.size() == 1);

    const auto* mesh_binding = gpu_bindings.palette.find_mesh(mesh);
    const auto* material_binding = gpu_bindings.palette.find_material(material);
    MK_REQUIRE(mesh_binding != nullptr);
    MK_REQUIRE(material_binding != nullptr);
    MK_REQUIRE(mesh_binding->owner_device == device.get());
    MK_REQUIRE(material_binding->owner_device == device.get());
    MK_REQUIRE(material_binding->pipeline_layout.value == gpu_bindings.material_pipeline_layouts[0].value);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = material_binding->pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0, .stride = mesh_binding->vertex_stride, .input_rate = mirakana::rhi::VertexInputRate::vertex}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
            .semantic_index = 0,
        }},
    });

    auto prepare_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    prepare_commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                         mirakana::rhi::ResourceState::render_target);
    prepare_commands->close();
    const auto prepare_fence = device->submit(*prepare_commands);
    device->wait(prepare_fence);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });
    renderer.begin_frame();
    const auto submitted = mirakana::submit_scene_render_packet(
        renderer, packet,
        mirakana::SceneRenderSubmitDesc{.fallback_mesh_color =
                                            mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                                        .material_palette = nullptr,
                                        .gpu_bindings = &gpu_bindings.palette});
    renderer.end_frame();

    const mirakana::rhi::BufferTextureCopyRegion readback_footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 64,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
    };
    auto readback_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    readback_commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                          mirakana::rhi::ResourceState::copy_source);
    readback_commands->copy_texture_to_buffer(target, readback, readback_footprint);
    readback_commands->close();
    const auto readback_fence = device->submit(*readback_commands);
    device->wait(readback_fence);

    const auto bytes = device->read_buffer(readback, 0, 256 * 64);
    const auto center_pixel = (32U * 256U) + (32U * 4U);
    MK_REQUIRE(bytes.size() == 256 * 64);
    MK_REQUIRE(bytes.at(center_pixel + 0U) >= 30 && bytes.at(center_pixel + 0U) <= 34);
    MK_REQUIRE(bytes.at(center_pixel + 1U) >= 98 && bytes.at(center_pixel + 1U) <= 102);
    MK_REQUIRE(bytes.at(center_pixel + 2U) >= 78 && bytes.at(center_pixel + 2U) <= 82);
    MK_REQUIRE(bytes.at(center_pixel + 3U) == 255);

    MK_REQUIRE(submitted.meshes_submitted == 1);
    MK_REQUIRE(submitted.mesh_gpu_bindings_resolved == 1);
    MK_REQUIRE(submitted.material_gpu_bindings_resolved == 1);
    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(device->stats().descriptor_sets_bound == 1);
    MK_REQUIRE(device->stats().indexed_draw_calls == 1);
    MK_REQUIRE(device->stats().indices_submitted == 3);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().buffer_reads == 1);
}

MK_TEST("d3d12 rhi device submits a renderer texture frame") {
    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = device.get(),
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });
    renderer.begin_frame();
    renderer.draw_sprite(mirakana::SpriteCommand{.transform = mirakana::Transform2D{},
                                                 .color = mirakana::Color{.r = 1.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F}});
    renderer.draw_mesh(mirakana::MeshCommand{.transform = mirakana::Transform3D{},
                                             .color = mirakana::Color{.r = 0.0F, .g = 1.0F, .b = 0.0F, .a = 1.0F}});
    renderer.end_frame();

    MK_REQUIRE(renderer.stats().frames_started == 1);
    MK_REQUIRE(renderer.stats().frames_finished == 1);
    MK_REQUIRE(renderer.stats().sprites_submitted == 1);
    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(device->stats().render_passes_begun == 1);
    MK_REQUIRE(device->stats().graphics_pipelines_bound == 1);
    MK_REQUIRE(device->stats().draw_calls == 2);
    MK_REQUIRE(device->stats().vertices_submitted == 6);
    MK_REQUIRE(device->stats().command_lists_submitted == 1);
}

MK_TEST("d3d12 rhi device allocates and validates descriptor set writes") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto uniform_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform,
    });
    const auto sampled_texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto sampler = device->create_sampler(mirakana::rhi::SamplerDesc{
        .min_filter = mirakana::rhi::SamplerFilter::nearest,
        .mag_filter = mirakana::rhi::SamplerFilter::nearest,
        .address_u = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = mirakana::rhi::SamplerAddressMode::clamp_to_edge,
    });
    const auto layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::vertex,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 1,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 2,
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto set = device->allocate_descriptor_set(layout);

    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                uniform_buffer)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = set,
        .binding = 1,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 sampled_texture)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = set,
        .binding = 2,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(sampler)},
    });

    MK_REQUIRE(layout.value != 0);
    MK_REQUIRE(set.value != 0);
    MK_REQUIRE(device->stats().descriptor_set_layouts_created == 1);
    MK_REQUIRE(device->stats().descriptor_sets_allocated == 1);
    MK_REQUIRE(device->stats().descriptor_writes == 3);
    MK_REQUIRE(device->stats().samplers_created == 1);
}

MK_TEST("d3d12 rhi device binds descriptor sets during a swapchain draw") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto uniform_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform,
    });
    const auto sampled_texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto uniform_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::vertex,
        },
    }});
    const auto texture_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto uniform_set = device->allocate_descriptor_set(uniform_layout);
    const auto texture_set = device->allocate_descriptor_set(texture_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = uniform_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                uniform_buffer)},
    });
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = texture_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 sampled_texture)},
    });

    const auto swapchain = device->create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .surface = mirakana::rhi::SurfaceHandle{reinterpret_cast<std::uintptr_t>(window.hwnd())},
    });
    const auto layout = device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{.descriptor_sets =
                                                                                             {
                                                                                                 uniform_layout,
                                                                                                 texture_layout,
                                                                                             },
                                                                                         .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    const auto frame = device->acquire_swapchain_frame(swapchain);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = frame,
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->bind_descriptor_set(layout, 0, uniform_set);
    commands->bind_descriptor_set(layout, 1, texture_set);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->present(frame);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    MK_REQUIRE(device->stats().descriptor_sets_bound == 2);
    MK_REQUIRE(device->stats().draw_calls == 1);
    MK_REQUIRE(device->stats().present_calls == 1);
}

MK_TEST("d3d12 rhi device rejects invalid descriptor set writes") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto uniform_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform,
    });
    const auto sampled_texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto misaligned_storage_buffer = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 258,
        .usage = mirakana::rhi::BufferUsage::storage,
    });
    const auto layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::vertex,
        },
    }});
    const auto storage_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::storage_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto set = device->allocate_descriptor_set(layout);
    const auto storage_set = device->allocate_descriptor_set(storage_layout);

    bool rejected_unknown_layout = false;
    try {
        (void)device->allocate_descriptor_set(mirakana::rhi::DescriptorSetLayoutHandle{999});
    } catch (const std::invalid_argument&) {
        rejected_unknown_layout = true;
    }

    bool rejected_wrong_resource_type = false;
    try {
        device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
            .set = set,
            .binding = 0,
            .array_element = 0,
            .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                     sampled_texture)},
        });
    } catch (const std::invalid_argument&) {
        rejected_wrong_resource_type = true;
    }

    bool rejected_unknown_binding = false;
    try {
        device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
            .set = set,
            .binding = 2,
            .array_element = 0,
            .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                    uniform_buffer)},
        });
    } catch (const std::invalid_argument&) {
        rejected_unknown_binding = true;
    }

    bool rejected_native_descriptor_write = false;
    try {
        device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
            .set = storage_set,
            .binding = 0,
            .array_element = 0,
            .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::storage_buffer,
                                                                    misaligned_storage_buffer)},
        });
    } catch (const std::invalid_argument&) {
        rejected_native_descriptor_write = true;
    }

    MK_REQUIRE(rejected_unknown_layout);
    MK_REQUIRE(rejected_wrong_resource_type);
    MK_REQUIRE(rejected_unknown_binding);
    MK_REQUIRE(rejected_native_descriptor_write);
    MK_REQUIRE(device->stats().descriptor_writes == 0);
}

MK_TEST("d3d12 rhi device rejects invalid descriptor layouts pipeline layouts and pipeline descriptions") {
    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    bool rejected_duplicate_binding = false;
    try {
        (void)device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
            mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                                 .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                                 .count = 1,
                                                 .stages = mirakana::rhi::ShaderStageVisibility::vertex},
            mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                                 .type = mirakana::rhi::DescriptorType::sampled_texture,
                                                 .count = 1,
                                                 .stages = mirakana::rhi::ShaderStageVisibility::fragment},
        }});
    } catch (const std::invalid_argument&) {
        rejected_duplicate_binding = true;
    }

    bool rejected_empty_visibility = false;
    try {
        (void)device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
            mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                                 .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                                 .count = 1,
                                                 .stages = mirakana::rhi::ShaderStageVisibility::none},
        }});
    } catch (const std::invalid_argument&) {
        rejected_empty_visibility = true;
    }

    bool rejected_unknown_pipeline_layout_set = false;
    try {
        (void)device->create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
            .descriptor_sets = {mirakana::rhi::DescriptorSetLayoutHandle{999}}, .push_constant_bytes = 0});
    } catch (const std::invalid_argument&) {
        rejected_unknown_pipeline_layout_set = true;
    }

    bool rejected_unaligned_push_constants = false;
    try {
        (void)device->create_pipeline_layout(
            mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 2});
    } catch (const std::invalid_argument&) {
        rejected_unaligned_push_constants = true;
    }

    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto compute_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute,
        .entry_point = "cs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });

    bool rejected_unknown_color_format = false;
    try {
        (void)device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
            .layout = layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::unknown,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        });
    } catch (const std::invalid_argument&) {
        rejected_unknown_color_format = true;
    }

    bool rejected_wrong_shader_stage = false;
    try {
        (void)device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
            .layout = layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = compute_shader,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        });
    } catch (const std::invalid_argument&) {
        rejected_wrong_shader_stage = true;
    }

    bool rejected_invalid_topology = false;
    try {
        (void)device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
            .layout = layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = static_cast<mirakana::rhi::PrimitiveTopology>(99),
        });
    } catch (const std::invalid_argument&) {
        rejected_invalid_topology = true;
    }

    bool rejected_depth_state_without_format = false;
    try {
        mirakana::rhi::GraphicsPipelineDesc desc{
            .layout = layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        };
        desc.depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                                .depth_write_enabled = true,
                                                                .depth_compare = mirakana::rhi::CompareOp::less_equal};
        (void)device->create_graphics_pipeline(desc);
    } catch (const std::invalid_argument&) {
        rejected_depth_state_without_format = true;
    }

    bool rejected_depth_write_without_test = false;
    try {
        mirakana::rhi::GraphicsPipelineDesc desc{
            .layout = layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        };
        desc.depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = false,
                                                                .depth_write_enabled = true,
                                                                .depth_compare = mirakana::rhi::CompareOp::less_equal};
        (void)device->create_graphics_pipeline(desc);
    } catch (const std::invalid_argument&) {
        rejected_depth_write_without_test = true;
    }

    MK_REQUIRE(rejected_duplicate_binding);
    MK_REQUIRE(rejected_empty_visibility);
    MK_REQUIRE(rejected_unknown_pipeline_layout_set);
    MK_REQUIRE(rejected_unaligned_push_constants);
    MK_REQUIRE(rejected_unknown_color_format);
    MK_REQUIRE(rejected_wrong_shader_stage);
    MK_REQUIRE(rejected_invalid_topology);
    MK_REQUIRE(rejected_depth_state_without_format);
    MK_REQUIRE(rejected_depth_write_without_test);
    MK_REQUIRE(device->stats().descriptor_set_layouts_created == 0);
    MK_REQUIRE(device->stats().pipeline_layouts_created == 1);
    MK_REQUIRE(device->stats().graphics_pipelines_created == 0);
}

MK_TEST("d3d12 rhi device validates native depth attachments and render pass pipeline formats") {
    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto depth = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto small_depth = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto state_test_depth = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto no_depth_pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const auto incompatible_color_pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    mirakana::rhi::GraphicsPipelineDesc depth_pipeline_desc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    depth_pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto depth_pipeline = device->create_graphics_pipeline(depth_pipeline_desc);

    bool rejected_color_format_depth_texture = false;
    try {
        (void)device->create_texture(mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = mirakana::rhi::Format::rgba8_unorm,
            .usage = mirakana::rhi::TextureUsage::depth_stencil,
        });
    } catch (const std::invalid_argument&) {
        rejected_color_format_depth_texture = true;
    }

    const auto sampled_depth = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto sampled_depth_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto sampled_depth_set = device->allocate_descriptor_set(sampled_depth_layout);
    device->update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = sampled_depth_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 sampled_depth)},
    });

    bool rejected_depth_copy_usage = false;
    try {
        (void)device->create_texture(mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = mirakana::rhi::Format::depth24_stencil8,
            .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::copy_source,
        });
    } catch (const std::invalid_argument&) {
        rejected_depth_copy_usage = true;
    }

    bool rejected_unknown_depth_attachment = false;
    try {
        auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = mirakana::rhi::TextureHandle{999}},
        });
    } catch (const std::invalid_argument&) {
        rejected_unknown_depth_attachment = true;
    }

    bool rejected_wrong_usage_depth_attachment = false;
    try {
        auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = target},
        });
    } catch (const std::invalid_argument&) {
        rejected_wrong_usage_depth_attachment = true;
    }

    bool rejected_depth_extent_mismatch = false;
    try {
        auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = small_depth},
        });
    } catch (const std::invalid_argument&) {
        rejected_depth_extent_mismatch = true;
    }

    bool rejected_bad_depth_clear = false;
    try {
        auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
            .depth =
                mirakana::rhi::RenderPassDepthAttachment{
                    .texture = depth,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                    .clear_depth = mirakana::rhi::ClearDepthValue{1.25F},
                },
        });
    } catch (const std::invalid_argument&) {
        rejected_bad_depth_clear = true;
    }

    bool rejected_depth_wrong_state = false;
    try {
        auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->transition_texture(state_test_depth, mirakana::rhi::ResourceState::depth_write,
                                     mirakana::rhi::ResourceState::shader_read);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = state_test_depth},
        });
    } catch (const std::invalid_argument&) {
        rejected_depth_wrong_state = true;
    }

    auto depth_pass_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    depth_pass_commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            },
        .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = depth},
    });

    bool rejected_no_depth_pipeline_in_depth_pass = false;
    try {
        depth_pass_commands->bind_graphics_pipeline(no_depth_pipeline);
    } catch (const std::invalid_argument&) {
        rejected_no_depth_pipeline_in_depth_pass = true;
    }
    depth_pass_commands->end_render_pass();
    depth_pass_commands->close();

    auto color_only_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    color_only_commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            },
    });

    bool rejected_depth_pipeline_in_color_only_pass = false;
    try {
        color_only_commands->bind_graphics_pipeline(depth_pipeline);
    } catch (const std::invalid_argument&) {
        rejected_depth_pipeline_in_color_only_pass = true;
    }

    bool rejected_color_format_pipeline_mismatch = false;
    try {
        color_only_commands->bind_graphics_pipeline(incompatible_color_pipeline);
    } catch (const std::invalid_argument&) {
        rejected_color_format_pipeline_mismatch = true;
    }
    color_only_commands->end_render_pass();
    color_only_commands->close();

    MK_REQUIRE(rejected_color_format_depth_texture);
    MK_REQUIRE(rejected_depth_copy_usage);
    MK_REQUIRE(rejected_unknown_depth_attachment);
    MK_REQUIRE(rejected_wrong_usage_depth_attachment);
    MK_REQUIRE(rejected_depth_extent_mismatch);
    MK_REQUIRE(rejected_bad_depth_clear);
    MK_REQUIRE(rejected_depth_wrong_state);
    MK_REQUIRE(rejected_no_depth_pipeline_in_depth_pass);
    MK_REQUIRE(rejected_depth_pipeline_in_color_only_pass);
    MK_REQUIRE(rejected_color_format_pipeline_mismatch);

    const auto bound_pipelines_before_sampled_depth = device->stats().graphics_pipelines_bound;
    auto sampled_depth_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    sampled_depth_commands->transition_texture(sampled_depth, mirakana::rhi::ResourceState::depth_write,
                                               mirakana::rhi::ResourceState::shader_read);

    bool rejected_sampled_depth_attachment_while_shader_read = false;
    try {
        sampled_depth_commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = sampled_depth},
        });
    } catch (const std::invalid_argument&) {
        rejected_sampled_depth_attachment_while_shader_read = true;
    }
    if (!rejected_sampled_depth_attachment_while_shader_read) {
        sampled_depth_commands->end_render_pass();
    }
    MK_REQUIRE(rejected_sampled_depth_attachment_while_shader_read);

    sampled_depth_commands->transition_texture(sampled_depth, mirakana::rhi::ResourceState::shader_read,
                                               mirakana::rhi::ResourceState::depth_write);
    sampled_depth_commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            },
        .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = sampled_depth},
    });
    sampled_depth_commands->bind_graphics_pipeline(depth_pipeline);
    sampled_depth_commands->end_render_pass();
    sampled_depth_commands->close();
    (void)device->submit(*sampled_depth_commands);

    MK_REQUIRE(device->stats().descriptor_writes >= 1);
    MK_REQUIRE(device->stats().graphics_pipelines_bound == bound_pipelines_before_sampled_depth + 1);
}

MK_TEST("d3d12 rhi device rejects incompatible descriptor set command binding") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    const auto vertex_bytecode = compile_triangle_vertex_shader();
    const auto pixel_bytecode = compile_triangle_pixel_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto swapchain = device->create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .surface = mirakana::rhi::SurfaceHandle{reinterpret_cast<std::uintptr_t>(window.hwnd())},
    });
    const auto uniform_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::vertex,
        },
    }});
    const auto texture_layout = device->create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto incompatible_set = device->allocate_descriptor_set(uniform_layout);
    const auto layout = device->create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {texture_layout}, .push_constant_bytes = 0});
    const auto vertex_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = vertex_bytecode->GetBufferSize(),
        .bytecode = vertex_bytecode->GetBufferPointer(),
    });
    const auto fragment_shader = device->create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = pixel_bytecode->GetBufferSize(),
        .bytecode = pixel_bytecode->GetBufferPointer(),
    });
    const auto pipeline = device->create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    const auto frame = device->acquire_swapchain_frame(swapchain);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = frame,
            },
    });
    commands->bind_graphics_pipeline(pipeline);

    bool rejected_incompatible_set = false;
    try {
        commands->bind_descriptor_set(layout, 0, incompatible_set);
    } catch (const std::invalid_argument&) {
        rejected_incompatible_set = true;
    }
    commands->end_render_pass();
    commands->present(frame);
    commands->close();
    const auto fence = device->submit(*commands);
    device->wait(fence);

    MK_REQUIRE(rejected_incompatible_set);
    MK_REQUIRE(device->stats().descriptor_sets_bound == 0);
    MK_REQUIRE(device->stats().present_calls == 1);
}

MK_TEST("d3d12 rhi device records buffer copies on the copy queue") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto source = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 1024,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto destination = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 1024,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const std::vector<std::uint8_t> bytes{0x01, 0x02, 0x03, 0x04};
    device->write_buffer(source, 128, bytes);
    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);

    commands->copy_buffer(
        source, destination,
        mirakana::rhi::BufferCopyRegion{.source_offset = 128, .destination_offset = 256, .size_bytes = 4});
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);
    const auto copied = device->read_buffer(destination, 256, 4);

    MK_REQUIRE(device->stats().buffer_copies == 1);
    MK_REQUIRE(device->stats().bytes_copied == 4);
    MK_REQUIRE(device->stats().buffer_writes == 1);
    MK_REQUIRE(device->stats().bytes_written == 4);
    MK_REQUIRE(device->stats().command_lists_submitted == 1);
    MK_REQUIRE(copied == bytes);
}

MK_TEST("d3d12 runtime rhi texture upload stages padded texture bytes") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = mirakana::AssetId::from_name("textures/d3d12_upload"),
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .width = 4,
        .height = 2,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 32,
        .bytes = std::vector<std::uint8_t>(32, std::uint8_t{0x7f}),
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(*device, payload);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.texture.value != 0);
    MK_REQUIRE(result.upload_buffer.value != 0);
    MK_REQUIRE(result.copy_recorded);
    MK_REQUIRE(result.copy_region.buffer_row_length == 64);
    MK_REQUIRE(result.uploaded_bytes == 512);

    const auto stats = device->stats();
    MK_REQUIRE(stats.textures_created == 1);
    MK_REQUIRE(stats.buffers_created == 1);
    MK_REQUIRE(stats.buffer_writes == 1);
    MK_REQUIRE(stats.bytes_written == 512);
    MK_REQUIRE(stats.buffer_texture_copies == 1);
    MK_REQUIRE(stats.command_lists_submitted == 1);
    MK_REQUIRE(stats.fence_waits == 1);
}

MK_TEST("d3d12 rhi device rejects invalid buffer copy commands") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto source = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 128,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto destination = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 128,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto wrong_usage_destination = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 128,
        .usage = mirakana::rhi::BufferUsage::vertex,
    });
    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);

    bool rejected_wrong_usage = false;
    try {
        commands->copy_buffer(
            source, wrong_usage_destination,
            mirakana::rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = 16});
    } catch (const std::invalid_argument&) {
        rejected_wrong_usage = true;
    }

    bool rejected_out_of_range = false;
    try {
        commands->copy_buffer(
            source, destination,
            mirakana::rhi::BufferCopyRegion{.source_offset = 64, .destination_offset = 0, .size_bytes = 128});
    } catch (const std::invalid_argument&) {
        rejected_out_of_range = true;
    }
    commands->close();

    MK_REQUIRE(rejected_wrong_usage);
    MK_REQUIRE(rejected_out_of_range);
    MK_REQUIRE(device->stats().buffer_copies == 0);
}

MK_TEST("d3d12 rhi device acquires releases and invalidates transient resources") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto transient_buffer = device->acquire_transient_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 128,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto transient_texture = device->acquire_transient_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination,
    });
    const auto destination = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 128,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    MK_REQUIRE(device->stats().transient_texture_placed_resources_alive == 1);

    device->release_transient(transient_buffer.lease);

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);
    bool rejected_released_buffer = false;
    try {
        commands->copy_buffer(
            transient_buffer.buffer, destination,
            mirakana::rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = 16});
    } catch (const std::invalid_argument&) {
        rejected_released_buffer = true;
    }
    commands->close();

    bool rejected_double_release = false;
    try {
        device->release_transient(transient_buffer.lease);
    } catch (const std::invalid_argument&) {
        rejected_double_release = true;
    }

    const auto upload = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 4096,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };
    auto texture_commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);
    texture_commands->copy_buffer_to_texture(upload, transient_texture.texture, footprint);
    texture_commands->close();
    const auto texture_fence = device->submit(*texture_commands);

    device->release_transient(transient_texture.lease);
    device->wait(texture_fence);

    MK_REQUIRE(transient_buffer.lease.value == 1);
    MK_REQUIRE(transient_buffer.buffer.value != 0);
    MK_REQUIRE(transient_texture.lease.value == 2);
    MK_REQUIRE(transient_texture.texture.value != 0);
    MK_REQUIRE(rejected_released_buffer);
    MK_REQUIRE(rejected_double_release);
    MK_REQUIRE(device->stats().transient_resources_acquired == 2);
    MK_REQUIRE(device->stats().transient_resources_released == 2);
    MK_REQUIRE(device->stats().transient_resources_active == 0);
    MK_REQUIRE(device->stats().transient_texture_heap_allocations == 1);
    MK_REQUIRE(device->stats().transient_texture_placed_allocations == 1);
    MK_REQUIRE(device->stats().transient_texture_placed_resources_alive == 0);
}

MK_TEST("d3d12 rhi device records texture aliasing barrier commands") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto first = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto second = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->texture_aliasing_barrier(first, second);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    const auto stats = device->stats();
    MK_REQUIRE(stats.texture_aliasing_barriers == 1);
    MK_REQUIRE(stats.resource_transitions == 0);
}

MK_TEST("d3d12 rhi device records buffer texture copies with aligned footprints") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto upload = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 4096,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 4096,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto upload_target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination,
    });
    const auto readback_source = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_source,
    });
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };
    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);

    commands->copy_buffer_to_texture(upload, upload_target, footprint);
    commands->copy_texture_to_buffer(readback_source, readback, footprint);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    MK_REQUIRE(device->stats().buffer_texture_copies == 1);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().command_lists_submitted == 1);
}

MK_TEST("d3d12 rhi device reads copy destination buffer ranges") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 4096,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });

    const auto bytes = device->read_buffer(readback, 0, 32);

    MK_REQUIRE(bytes.size() == 32);
    MK_REQUIRE(device->stats().buffer_reads == 1);
    MK_REQUIRE(device->stats().bytes_read == 32);
}

MK_TEST("d3d12 rhi device rejects unaligned buffer texture footprints") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto upload = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 4096,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination,
    });
    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);

    bool rejected_offset = false;
    try {
        commands->copy_buffer_to_texture(
            upload, texture,
            mirakana::rhi::BufferTextureCopyRegion{
                .buffer_offset = 4,
                .buffer_row_length = 64,
                .buffer_image_height = 8,
                .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
                .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
            });
    } catch (const std::invalid_argument&) {
        rejected_offset = true;
    }

    bool rejected_row_pitch = false;
    try {
        commands->copy_buffer_to_texture(
            upload, texture,
            mirakana::rhi::BufferTextureCopyRegion{
                .buffer_offset = 0,
                .buffer_row_length = 8,
                .buffer_image_height = 8,
                .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
                .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
            });
    } catch (const std::invalid_argument&) {
        rejected_row_pitch = true;
    }
    commands->close();

    MK_REQUIRE(rejected_offset);
    MK_REQUIRE(rejected_row_pitch);
    MK_REQUIRE(device->stats().buffer_texture_copies == 0);
}

MK_TEST("d3d12 rhi device records standalone texture transitions") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);

    commands->transition_texture(texture, mirakana::rhi::ResourceState::copy_destination,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->close();

    const auto fence = device->submit(*commands);
    device->wait(fence);

    MK_REQUIRE(device->stats().resource_transitions == 1);
    MK_REQUIRE(device->stats().command_lists_submitted == 1);
}

MK_TEST("d3d12 rhi device rejects invalid standalone texture transitions") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);

    bool rejected_unknown_texture = false;
    try {
        commands->transition_texture(mirakana::rhi::TextureHandle{999}, mirakana::rhi::ResourceState::copy_destination,
                                     mirakana::rhi::ResourceState::shader_read);
    } catch (const std::invalid_argument&) {
        rejected_unknown_texture = true;
    }

    bool rejected_invalid_state = false;
    try {
        commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::shader_read);
    } catch (const std::invalid_argument&) {
        rejected_invalid_state = true;
    }
    commands->close();

    MK_REQUIRE(rejected_unknown_texture);
    MK_REQUIRE(rejected_invalid_state);
    MK_REQUIRE(device->stats().resource_transitions == 0);
}

MK_TEST("d3d12 rhi device tracks texture states before render and copy commands") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto target = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::copy_source,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 1024,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 4,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
    };

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);

    bool rejected_render_target_state = false;
    try {
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
        });
    } catch (const std::invalid_argument&) {
        rejected_render_target_state = true;
    }

    bool rejected_transition_before_state = false;
    try {
        commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                     mirakana::rhi::ResourceState::copy_source);
    } catch (const std::invalid_argument&) {
        rejected_transition_before_state = true;
    }

    commands->transition_texture(target, mirakana::rhi::ResourceState::copy_source,
                                 mirakana::rhi::ResourceState::render_target);

    bool rejected_copy_source_state = false;
    try {
        commands->copy_texture_to_buffer(target, readback, footprint);
    } catch (const std::invalid_argument&) {
        rejected_copy_source_state = true;
    }
    commands->close();

    MK_REQUIRE(rejected_render_target_state);
    MK_REQUIRE(rejected_transition_before_state);
    MK_REQUIRE(rejected_copy_source_state);
    MK_REQUIRE(device->stats().resource_transitions == 1);
    MK_REQUIRE(device->stats().texture_buffer_copies == 0);
}

MK_TEST("d3d12 rhi device rejects stale cross queue texture state submissions") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    const auto texture = device->create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_source | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto readback = device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256 * 8,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const mirakana::rhi::BufferTextureCopyRegion footprint{
        .buffer_offset = 0,
        .buffer_row_length = 64,
        .buffer_image_height = 8,
        .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
        .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
    };

    auto graphics_commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    graphics_commands->transition_texture(texture, mirakana::rhi::ResourceState::copy_source,
                                          mirakana::rhi::ResourceState::shader_read);
    graphics_commands->close();

    auto copy_commands = device->begin_command_list(mirakana::rhi::QueueKind::copy);
    copy_commands->copy_texture_to_buffer(texture, readback, footprint);
    copy_commands->close();

    const auto fence = device->submit(*graphics_commands);
    device->wait(fence);

    bool rejected_stale_submission = false;
    try {
        (void)device->submit(*copy_commands);
    } catch (const std::logic_error&) {
        rejected_stale_submission = true;
    }

    MK_REQUIRE(rejected_stale_submission);
    MK_REQUIRE(device->stats().resource_transitions == 1);
    MK_REQUIRE(device->stats().texture_buffer_copies == 1);
    MK_REQUIRE(device->stats().command_lists_submitted == 1);
}

MK_TEST("d3d12 rhi device rejects unsupported or invalid backend neutral work") {
    const auto vertex_bytecode = compile_triangle_vertex_shader();
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(device != nullptr);

    bool rejected_swapchain_without_surface = false;
    try {
        (void)device->create_swapchain(mirakana::rhi::SwapchainDesc{
            .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
            .format = mirakana::rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = false,
        });
    } catch (const std::invalid_argument&) {
        rejected_swapchain_without_surface = true;
    }

    bool rejected_shader_without_bytecode = false;
    try {
        (void)device->create_shader(mirakana::rhi::ShaderDesc{
            .stage = mirakana::rhi::ShaderStage::vertex,
            .entry_point = "vs_main",
            .bytecode_size = vertex_bytecode->GetBufferSize(),
            .bytecode = nullptr,
        });
    } catch (const std::invalid_argument&) {
        rejected_shader_without_bytecode = true;
    }

    auto commands = device->begin_command_list(mirakana::rhi::QueueKind::graphics);
    bool rejected_submit_open_list = false;
    try {
        (void)device->submit(*commands);
    } catch (const std::logic_error&) {
        rejected_submit_open_list = true;
    }

    MK_REQUIRE(rejected_swapchain_without_surface);
    MK_REQUIRE(rejected_shader_without_bytecode);
    MK_REQUIRE(rejected_submit_open_list);
}

MK_TEST("d3d12 device context rejects invalid swapchain transitions") {
    HiddenTestWindow window;
    MK_REQUIRE(window.valid());

    auto context = mirakana::rhi::d3d12::DeviceContext::create(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });

    MK_REQUIRE(context != nullptr);

    const auto swapchain = context->create_swapchain_for_window(mirakana::rhi::d3d12::NativeSwapchainDesc{
        .window = mirakana::rhi::d3d12::NativeWindowHandle{window.hwnd()},
        .swapchain =
            mirakana::rhi::SwapchainDesc{
                .extent = mirakana::rhi::Extent2D{.width = 64, .height = 64},
                .format = mirakana::rhi::Format::bgra8_unorm,
                .buffer_count = 2,
                .vsync = false,
            },
    });
    const auto commands = context->create_command_list(mirakana::rhi::QueueKind::graphics);

    MK_REQUIRE(swapchain.value == 1);
    MK_REQUIRE(commands.value == 1);
    MK_REQUIRE(!context->transition_swapchain_back_buffer(mirakana::rhi::d3d12::NativeCommandListHandle{99}, swapchain,
                                                          mirakana::rhi::ResourceState::present,
                                                          mirakana::rhi::ResourceState::render_target));
    MK_REQUIRE(!context->transition_swapchain_back_buffer(commands, mirakana::rhi::d3d12::NativeSwapchainHandle{99},
                                                          mirakana::rhi::ResourceState::present,
                                                          mirakana::rhi::ResourceState::render_target));
    MK_REQUIRE(!context->transition_swapchain_back_buffer(commands, swapchain, mirakana::rhi::ResourceState::undefined,
                                                          mirakana::rhi::ResourceState::present));
    MK_REQUIRE(!context->clear_swapchain_back_buffer(commands, mirakana::rhi::d3d12::NativeSwapchainHandle{99}, 0.1F,
                                                     0.2F, 0.3F, 1.0F));
    MK_REQUIRE(!context->clear_swapchain_back_buffer(mirakana::rhi::d3d12::NativeCommandListHandle{99}, swapchain, 0.1F,
                                                     0.2F, 0.3F, 1.0F));
    MK_REQUIRE(context->stats().swapchain_back_buffer_transitions == 0);
    MK_REQUIRE(context->stats().swapchain_back_buffer_clears == 0);
}

MK_TEST("d3d12 rhi memory diagnostics reports committed resource bytes and optional DXGI video memory") {
    auto device = mirakana::rhi::d3d12::create_rhi_device(mirakana::rhi::d3d12::DeviceBootstrapDesc{
        .prefer_warp = false,
        .enable_debug_layer = false,
    });
    MK_REQUIRE(device != nullptr);

    (void)device->create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 2048,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });

    const auto mem = device->memory_diagnostics();
    MK_REQUIRE(mem.committed_resources_byte_estimate_available);
    MK_REQUIRE(mem.committed_resources_byte_estimate >= 2048U);
}

int main() {
    return mirakana::test::run_all();
}
