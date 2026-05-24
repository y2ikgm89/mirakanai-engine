// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/material_graph_shader_pipeline.hpp"

#include "mirakana/assets/shader_metadata.hpp"
#include "mirakana/assets/shader_pipeline.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>

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

MaterialGraphProductionAuthoringResult
plan_material_graph_production_authoring(const MaterialGraphProductionAuthoringDesc& desc,
                                         const ShaderToolDescriptor& dxc_tool) {
    MaterialGraphProductionAuthoringResult result;

    const auto add_failure = [&result](std::string diagnostic) {
        result.failures.push_back(MaterialInstancePackageUpdateFailure{std::move(diagnostic)});
    };

    if (desc.shader_graph_execution != "unsupported") {
        add_failure("shader graph execution is not supported by material graph production authoring");
    }
    if (desc.live_shader_generation != "unsupported") {
        add_failure("live shader generation is not supported by material graph production authoring");
    }
    if (desc.renderer_rhi_residency != "unsupported") {
        add_failure("renderer/RHI residency is not supported by material graph production authoring");
    }
    if (desc.package_streaming != "unsupported") {
        add_failure("package streaming is not supported by material graph production authoring");
    }
    if (!result.failures.empty()) {
        return result;
    }

    const auto package_update = plan_material_graph_package_update(MaterialGraphPackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = desc.package_index_content,
        .material_graph_content = desc.material_graph_content,
        .output_path = desc.material_output_path,
        .source_revision = desc.source_revision,
        .shader_graph = desc.shader_graph_execution,
        .live_shader_generation = desc.live_shader_generation,
        .renderer_rhi_residency = desc.renderer_rhi_residency,
        .package_streaming = desc.package_streaming,
    });
    if (!package_update.succeeded()) {
        result.failures = package_update.failures;
        return result;
    }

    MaterialGraphDesc graph;
    try {
        graph = deserialize_material_graph(desc.material_graph_content);
        result.hlsl_content = emit_material_graph_reviewed_hlsl_v0(graph);
    } catch (const std::exception& error) {
        add_failure(std::string("material graph HLSL emission failed: ") + error.what());
        return result;
    }

    MaterialGraphShaderExportDesc export_desc;
    export_desc.export_id = desc.export_id;
    export_desc.export_name = desc.export_name;
    export_desc.material_graph_path = desc.material_graph_path;
    export_desc.hlsl_source_path = desc.hlsl_output_path;
    export_desc.vertex_entry = desc.vertex_entry;
    export_desc.fragment_entry = desc.fragment_entry;

    try {
        result.shader_export_content = serialize_material_graph_shader_export(export_desc);
    } catch (const std::exception& error) {
        add_failure(std::string("material graph shader export is invalid: ") + error.what());
        return result;
    }

    if (desc.shader_export_output_path.empty() || is_absolute_or_parent_relative(desc.shader_export_output_path) ||
        !valid_token(desc.shader_export_output_path)) {
        add_failure("shader export output path must be a safe non-empty relative path");
        return result;
    }

    MemoryFileSystem planned_files;
    planned_files.write_text(desc.hlsl_output_path, result.hlsl_content);
    const auto shader_plan = plan_material_graph_shader_pipeline(planned_files, export_desc, desc.artifact_output_root,
                                                                 desc.shader_cache_index_relative, dxc_tool);
    if (!shader_plan.ok) {
        for (const auto& diagnostic : shader_plan.diagnostics) {
            add_failure(diagnostic.field + ": " + diagnostic.message);
        }
        if (shader_plan.diagnostics.empty()) {
            add_failure("material graph shader pipeline planning failed");
        }
        return result;
    }

    result.material_content = package_update.material_content;
    result.package_index_content = package_update.package_index_content;
    result.changed_files = package_update.changed_files;
    result.changed_files.push_back(MaterialInstancePackageChangedFile{
        .path = desc.hlsl_output_path,
        .content = result.hlsl_content,
        .content_hash = hash_asset_cooked_content(result.hlsl_content),
    });
    result.changed_files.push_back(MaterialInstancePackageChangedFile{
        .path = desc.shader_export_output_path,
        .content = result.shader_export_content,
        .content_hash = hash_asset_cooked_content(result.shader_export_content),
    });
    result.execution_requests = shader_plan.execution_requests;
    result.lowered_material_count = 1;
    result.shader_export_count = 1;
    result.d3d12_compile_request_count =
        static_cast<std::size_t>(std::ranges::count_if(result.execution_requests, [](const auto& request) {
            return request.compile_request.target == ShaderCompileTarget::d3d12_dxil;
        }));
    result.vulkan_compile_request_count =
        static_cast<std::size_t>(std::ranges::count_if(result.execution_requests, [](const auto& request) {
            return request.compile_request.target == ShaderCompileTarget::vulkan_spirv;
        }));
    return result;
}

} // namespace mirakana
