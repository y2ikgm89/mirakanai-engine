// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/material_graph_shader_export.hpp"

#include "mirakana/assets/material_graph.hpp"

#include <cctype>
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('=') == std::string_view::npos;
}

[[nodiscard]] bool is_absolute_or_parent_relative(std::string_view path) noexcept {
    if (path.empty()) {
        return true;
    }
    if (path.size() >= 2U && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':') {
        return true;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return true;
    }
    std::size_t start = 0;
    while (start <= path.size()) {
        const auto separator = path.find_first_of("/\\", start);
        const auto part =
            path.substr(start, separator == std::string_view::npos ? path.size() - start : separator - start);
        if (part == "..") {
            return true;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        start = separator + 1U;
    }
    return false;
}

[[nodiscard]] bool safe_repo_relative_path(std::string_view path) noexcept {
    return !path.empty() && !is_absolute_or_parent_relative(path);
}

[[nodiscard]] std::unordered_map<std::string, std::string> parse_unique_key_lines(std::string_view text) {
    std::unordered_map<std::string, std::string> values;
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto raw_line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                         : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
        if (raw_line.empty()) {
            continue;
        }
        const auto separator = raw_line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("material graph shader export line is missing '='");
        }
        auto [_, inserted] =
            values.emplace(std::string(raw_line.substr(0, separator)), std::string(raw_line.substr(separator + 1U)));
        if (!inserted) {
            throw std::invalid_argument("material graph shader export contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const std::unordered_map<std::string, std::string>& values,
                                                const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument("material graph shader export is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument("material graph shader export integer value is invalid");
    }
    return parsed;
}

[[nodiscard]] std::string float_literal(float value) {
    std::ostringstream stream;
    stream.setf(std::ios::fmtflags(0), std::ios::floatfield);
    stream.precision(8);
    stream << value;
    return stream.str();
}

} // namespace

bool operator==(const MaterialGraphShaderExportDesc& lhs, const MaterialGraphShaderExportDesc& rhs) noexcept {
    return lhs.export_id.value == rhs.export_id.value && lhs.export_name == rhs.export_name &&
           lhs.material_graph_path == rhs.material_graph_path && lhs.hlsl_source_path == rhs.hlsl_source_path &&
           lhs.vertex_entry == rhs.vertex_entry && lhs.fragment_entry == rhs.fragment_entry;
}

std::string serialize_material_graph_shader_export(const MaterialGraphShaderExportDesc& desc) {
    const auto diagnostics = validate_material_graph_shader_export(desc);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("material graph shader export is invalid for serialization");
    }
    std::ostringstream output;
    output << "format=" << material_graph_shader_export_format_id << '\n';
    output << "export.id=" << desc.export_id.value << '\n';
    output << "export.name=" << desc.export_name << '\n';
    if (!desc.material_graph_path.empty()) {
        output << "material_graph.path=" << desc.material_graph_path << '\n';
    }
    output << "hlsl.path=" << desc.hlsl_source_path << '\n';
    output << "entry.vertex=" << desc.vertex_entry << '\n';
    output << "entry.fragment=" << desc.fragment_entry << '\n';
    return output.str();
}

MaterialGraphShaderExportDesc deserialize_material_graph_shader_export(std::string_view text) {
    const auto values = parse_unique_key_lines(text);
    if (required_value(values, "format") != material_graph_shader_export_format_id) {
        throw std::invalid_argument("material graph shader export format is unsupported");
    }
    MaterialGraphShaderExportDesc desc;
    desc.export_id = AssetId{parse_u64(required_value(values, "export.id"))};
    desc.export_name = required_value(values, "export.name");
    const auto graph_it = values.find("material_graph.path");
    if (graph_it != values.end()) {
        desc.material_graph_path = graph_it->second;
    }
    desc.hlsl_source_path = required_value(values, "hlsl.path");
    desc.vertex_entry = required_value(values, "entry.vertex");
    desc.fragment_entry = required_value(values, "entry.fragment");

    const auto diagnostics = validate_material_graph_shader_export(desc);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("material graph shader export definition is invalid");
    }
    return desc;
}

std::vector<MaterialGraphShaderExportDiagnostic>
validate_material_graph_shader_export(const MaterialGraphShaderExportDesc& desc) {
    std::vector<MaterialGraphShaderExportDiagnostic> diagnostics;
    if (desc.export_id.value == 0) {
        diagnostics.push_back({.code = MaterialGraphShaderExportDiagnosticCode::missing_key,
                               .field = "export.id",
                               .message = "export id must be non-zero"});
    }
    if (!valid_token(desc.export_name)) {
        diagnostics.push_back({.code = MaterialGraphShaderExportDiagnosticCode::invalid_token,
                               .field = "export.name",
                               .message = "export name token is invalid"});
    }
    if (!desc.material_graph_path.empty() && !valid_token(desc.material_graph_path)) {
        diagnostics.push_back({.code = MaterialGraphShaderExportDiagnosticCode::invalid_token,
                               .field = "material_graph.path",
                               .message = "material graph path token is invalid"});
    }
    if (!desc.material_graph_path.empty() && !safe_repo_relative_path(desc.material_graph_path)) {
        diagnostics.push_back({.code = MaterialGraphShaderExportDiagnosticCode::unsafe_path,
                               .field = "material_graph.path",
                               .message = "material graph path must be repository-relative without parent segments"});
    }
    if (!valid_token(desc.hlsl_source_path)) {
        diagnostics.push_back({.code = MaterialGraphShaderExportDiagnosticCode::invalid_token,
                               .field = "hlsl.path",
                               .message = "hlsl path token is invalid"});
    }
    if (!safe_repo_relative_path(desc.hlsl_source_path)) {
        diagnostics.push_back({.code = MaterialGraphShaderExportDiagnosticCode::unsafe_path,
                               .field = "hlsl.path",
                               .message = "hlsl path must be repository-relative without parent segments"});
    }
    if (!valid_token(desc.vertex_entry)) {
        diagnostics.push_back({.code = MaterialGraphShaderExportDiagnosticCode::invalid_token,
                               .field = "entry.vertex",
                               .message = "vertex entry token is invalid"});
    }
    if (!valid_token(desc.fragment_entry)) {
        diagnostics.push_back({.code = MaterialGraphShaderExportDiagnosticCode::invalid_token,
                               .field = "entry.fragment",
                               .message = "fragment entry token is invalid"});
    }
    return diagnostics;
}

std::string emit_material_graph_reviewed_hlsl_v0(const MaterialGraphDesc& graph) {
    if (!validate_material_graph(graph).empty()) {
        throw std::invalid_argument("material graph must validate before HLSL emission");
    }
    const auto material = lower_material_graph_to_definition(graph);
    std::ostringstream hlsl;
    hlsl << "// GameEngine.MaterialGraphGeneratedHlsl.v0\n";
    hlsl << "// Deterministic stub from lowered MaterialDefinition; reviewed compile only.\n";
    hlsl << "// Descriptor bindings align with `build_material_pipeline_binding_metadata` (scene frame uses b6).\n";
    hlsl << "// lowered.material_id=" << material.id.value << '\n';
    hlsl << "cbuffer MaterialFactors : register(b0) {\n";
    hlsl << "  float4 base_color;\n";
    hlsl << "  float3 emissive;\n";
    hlsl << "  float metallic;\n";
    hlsl << "  float roughness;\n";
    hlsl << "  float shading_lit;\n";
    hlsl << "  float3 _pad_material;\n";
    hlsl << "};\n\n";
    hlsl << "cbuffer ScenePbrFrame : register(b6) {\n";
    hlsl << "  row_major float4x4 clip_from_world;\n";
    hlsl << "  row_major float4x4 world_from_object;\n";
    hlsl << "  float4 camera_position_aspect;\n";
    hlsl << "  float4 light_dir_intensity;\n";
    hlsl << "  float4 light_color_pad;\n";
    hlsl << "  float4 ambient_pad;\n";
    hlsl << "};\n\n";
    hlsl << "Texture2D<float4> base_color_texture : register(t1);\n";
    hlsl << "SamplerState base_color_sampler : register(s16);\n\n";
    hlsl << "static const float k_pi = 3.14159265358979323846;\n\n";
    hlsl << "struct VsOut {\n";
    hlsl << "  float4 position : SV_Position;\n";
    hlsl << "  float3 world_position : TEXCOORD1;\n";
    hlsl << "  float3 normal : NORMAL;\n";
    hlsl << "  float2 uv : TEXCOORD0;\n";
    hlsl << "};\n\n";
    hlsl << "VsOut VSMain(uint vertex_id : SV_VertexID) {\n";
    hlsl << "  float2 positions[3] = { float2(-0.75, -0.75), float2(0.75, -0.75), float2(0.0, 0.75) };\n";
    hlsl << "  float2 uvs[3] = { float2(0.0, 1.0), float2(1.0, 1.0), float2(0.5, 0.0) };\n";
    hlsl << "  float3 normals[3] = { float3(0.0, 0.0, 1.0), float3(0.0, 0.0, 1.0), float3(0.0, 0.0, 1.0) };\n";
    hlsl << "  VsOut output;\n";
    hlsl << "  float4 object_pos = float4(positions[vertex_id], 0.0, 1.0);\n";
    hlsl << "  float4 world_pos = mul(object_pos, world_from_object);\n";
    hlsl << "  output.position = mul(world_pos, clip_from_world);\n";
    hlsl << "  output.world_position = world_pos.xyz;\n";
    hlsl << "  output.normal = normalize(mul(float4(normals[vertex_id], 0.0), world_from_object).xyz);\n";
    hlsl << "  output.uv = uvs[vertex_id];\n";
    hlsl << "  return output;\n";
    hlsl << "}\n\n";
    hlsl << "float3 fresnel_schlick(float cos_theta, float3 f0) {\n";
    hlsl << "  return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);\n";
    hlsl << "}\n\n";
    hlsl << "float distribution_ggx(float n_dot_h, float roughness) {\n";
    hlsl << "  float a = roughness * roughness;\n";
    hlsl << "  float a2 = a * a;\n";
    hlsl << "  float denom = (n_dot_h * n_dot_h) * (a2 - 1.0) + 1.0;\n";
    hlsl << "  return a2 / max(k_pi * denom * denom, 1e-6);\n";
    hlsl << "}\n\n";
    hlsl << "float geometry_schlick_ggx(float n_dot_x, float roughness) {\n";
    hlsl << "  float r = roughness + 1.0;\n";
    hlsl << "  float k = (r * r) / 8.0;\n";
    hlsl << "  return n_dot_x / max(n_dot_x * (1.0 - k) + k, 1e-6);\n";
    hlsl << "}\n\n";
    hlsl << "float geometry_smith(float n_dot_v, float n_dot_l, float roughness) {\n";
    hlsl << "  return geometry_schlick_ggx(n_dot_v, roughness) * geometry_schlick_ggx(n_dot_l, roughness);\n";
    hlsl << "}\n\n";
    hlsl << "float3 evaluate_lit_color(VsOut input, float4 sampled) {\n";
    hlsl << "  if (shading_lit < 0.5) {\n";
    hlsl << "    return sampled.rgb * base_color.rgb + emissive.rgb;\n";
    hlsl << "  }\n";
    hlsl << "  float3 albedo = sampled.rgb * base_color.rgb;\n";
    hlsl << "  float3 n = normalize(input.normal);\n";
    hlsl << "  float3 v = normalize(camera_position_aspect.xyz - input.world_position);\n";
    hlsl << "  float3 l = normalize(light_dir_intensity.xyz);\n";
    hlsl << "  float3 h = normalize(v + l);\n";
    hlsl << "  float n_dot_v = saturate(dot(n, v));\n";
    hlsl << "  float n_dot_l = saturate(dot(n, l));\n";
    hlsl << "  float n_dot_h = saturate(dot(n, h));\n";
    hlsl << "  float h_dot_v = saturate(dot(h, v));\n";
    hlsl << "  float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);\n";
    hlsl << "  float3 fresnel = fresnel_schlick(h_dot_v, f0);\n";
    hlsl << "  float alpha = max(roughness, 0.04);\n";
    hlsl << "  float d = distribution_ggx(n_dot_h, alpha);\n";
    hlsl << "  float g = geometry_smith(n_dot_v, n_dot_l, alpha);\n";
    hlsl << "  float3 specular = (d * g) * fresnel / max(4.0 * n_dot_v * n_dot_l, 1e-4);\n";
    hlsl << "  float3 kd = (1.0 - fresnel) * (1.0 - metallic);\n";
    hlsl << "  float3 diffuse = kd * albedo / k_pi;\n";
    hlsl << "  float3 radiance = light_color_pad.rgb * light_dir_intensity.w;\n";
    hlsl << "  float3 direct = (diffuse + specular) * radiance * n_dot_l;\n";
    hlsl << "  float3 ambient = ambient_pad.rgb * albedo;\n";
    hlsl << "  return direct + ambient + emissive.rgb;\n";
    hlsl << "}\n\n";
    hlsl << "float4 PSMain(VsOut input) : SV_TARGET0 {\n";
    hlsl << "  float4 sampled = base_color_texture.Sample(base_color_sampler, input.uv);\n";
    hlsl << "  float3 color = evaluate_lit_color(input, sampled);\n";
    hlsl << "  return float4(saturate(color), sampled.a * base_color.a);\n";
    hlsl << "}\n";
    return hlsl.str();
}

} // namespace mirakana
