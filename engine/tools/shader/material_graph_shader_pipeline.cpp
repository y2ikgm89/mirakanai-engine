// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/material_graph_shader_pipeline.hpp"

#include "mirakana/assets/shader_metadata.hpp"
#include "mirakana/assets/shader_pipeline.hpp"

#include <cctype>
#include <stdexcept>
#include <string>

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

[[nodiscard]] std::string join_under(std::string_view root, std::string_view child) {
    std::string out;
    if (!root.empty()) {
        out.assign(root);
        if (out.back() != '/') {
            out.push_back('/');
        }
    }
    out.append(child);
    return out;
}

[[nodiscard]] std::string source_parent_directory(std::string_view source_path) {
    const auto separator = source_path.find_last_of("/\\");
    if (separator == std::string_view::npos) {
        return {};
    }
    return std::string(source_path.substr(0, separator));
}

[[nodiscard]] ShaderCompileExecutionRequest
make_request(const MaterialGraphShaderExportDesc& export_desc, std::string_view artifact_output_root,
             std::string_view shader_cache_index_relative, const ShaderToolDescriptor& dxc_tool,
             ShaderCompileTarget target, ShaderSourceStage stage, std::string_view artifact_relative,
             std::string_view profile, std::string_view id_suffix) {
    const auto include_root = source_parent_directory(export_desc.hlsl_source_path);
    std::vector<std::string> include_paths;
    if (!include_root.empty()) {
        include_paths.push_back(include_root);
    }

    const auto source_id = AssetId::from_name(std::string{"material_graph_shader_export/"} + export_desc.export_name +
                                              "/" + std::string{id_suffix});

    ShaderSourceMetadata metadata;
    metadata.id = source_id;
    metadata.source_path = export_desc.hlsl_source_path;
    metadata.language = ShaderSourceLanguage::hlsl;
    metadata.stage = stage;
    metadata.entry_point = stage == ShaderSourceStage::vertex ? export_desc.vertex_entry : export_desc.fragment_entry;

    ShaderCompileRequest compile;
    compile.source = std::move(metadata);
    compile.target = target;
    compile.output_path = join_under(artifact_output_root, artifact_relative);
    compile.profile = std::string(profile);
    compile.include_paths = std::move(include_paths);
    compile.debug_symbols = true;
    compile.optimize = true;

    ShaderCompileExecutionRequest execution;
    execution.compile_request = std::move(compile);
    execution.tool = dxc_tool;
    execution.cache_index_path = join_under(artifact_output_root, shader_cache_index_relative);
    execution.allow_cache = true;
    execution.write_artifact_marker = true;
    return execution;
}

} // namespace

MaterialGraphShaderPipelinePlan plan_material_graph_shader_pipeline(const IFileSystem& filesystem,
                                                                    const MaterialGraphShaderExportDesc& export_desc,
                                                                    const std::string_view artifact_output_root,
                                                                    const std::string_view shader_cache_index_relative,
                                                                    const ShaderToolDescriptor& dxc_tool) {
    MaterialGraphShaderPipelinePlan plan;
    const auto export_diagnostics = validate_material_graph_shader_export(export_desc);
    if (!export_diagnostics.empty()) {
        plan.diagnostics.push_back(
            {.field = "export_desc", .message = "material graph shader export descriptor failed validation"});
        return plan;
    }
    if (artifact_output_root.empty() || is_absolute_or_parent_relative(artifact_output_root) ||
        !valid_token(std::string{artifact_output_root})) {
        plan.diagnostics.push_back({.field = "artifact_output_root",
                                    .message = "artifact output root must be a safe non-empty relative path"});
        return plan;
    }
    if (shader_cache_index_relative.empty() || is_absolute_or_parent_relative(shader_cache_index_relative) ||
        !valid_token(std::string{shader_cache_index_relative})) {
        plan.diagnostics.push_back({.field = "shader_cache_index_relative",
                                    .message = "shader cache index path must be a safe non-empty relative path"});
        return plan;
    }
    if (dxc_tool.kind != ShaderToolKind::dxc) {
        plan.diagnostics.push_back(
            {.field = "dxc_tool", .message = "shader pipeline planning requires a DXC tool descriptor"});
        return plan;
    }
    if (!filesystem.exists(export_desc.hlsl_source_path)) {
        plan.diagnostics.push_back({.field = "hlsl.path", .message = "hlsl source file is missing on the filesystem"});
        return plan;
    }

    const auto tail_root = join_under("material_graph_shader_exports", std::to_string(export_desc.export_id.value));

    const auto push_targets = [&](ShaderCompileTarget target, std::string_view target_tag) {
        const auto vs_name = std::string{"vs."} + std::string(target_tag) +
                             (target == ShaderCompileTarget::d3d12_dxil ? ".dxil" : ".spirv");
        const auto ps_name = std::string{"ps."} + std::string(target_tag) +
                             (target == ShaderCompileTarget::d3d12_dxil ? ".dxil" : ".spirv");
        plan.execution_requests.push_back(make_request(
            export_desc, artifact_output_root, shader_cache_index_relative, dxc_tool, target, ShaderSourceStage::vertex,
            join_under(tail_root, vs_name), "vs_6_0", std::string{target_tag} + "/vertex"));
        plan.execution_requests.push_back(make_request(export_desc, artifact_output_root, shader_cache_index_relative,
                                                       dxc_tool, target, ShaderSourceStage::fragment,
                                                       join_under(tail_root, ps_name), "ps_6_0",
                                                       std::string{target_tag} + "/fragment"));
    };

    push_targets(ShaderCompileTarget::d3d12_dxil, "d3d12");
    if (!dxc_tool.supports_spirv_codegen) {
        plan.diagnostics.push_back({.field = "dxc_tool.supports_spirv_codegen",
                                    .message = "DXC SPIR-V codegen is unavailable; Vulkan targets were skipped"});
    } else {
        push_targets(ShaderCompileTarget::vulkan_spirv, "vulkan");
    }

    plan.ok = true;
    return plan;
}

} // namespace mirakana
