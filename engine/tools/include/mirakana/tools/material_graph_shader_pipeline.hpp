// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/material_graph_shader_export.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/tools/material_tool.hpp"
#include "mirakana/tools/shader_compile_action.hpp"
#include "mirakana/tools/shader_toolchain.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

/**
 * Diagnostics for `plan_material_graph_shader_pipeline` when inputs, tool readiness, or filesystem state block work.
 */
struct MaterialGraphShaderPipelineDiagnostic {
    std::string field;
    std::string message;
};

/**
 * Planned `ShaderCompileExecutionRequest` rows for DXC DXIL + DXC SPIR-V targets (vertex + fragment each).
 * Execution remains host-owned through `ShaderToolProcessRunner` / `execute_shader_compile_action`.
 */
struct MaterialGraphShaderPipelinePlan {
    bool ok{false};
    std::vector<ShaderCompileExecutionRequest> execution_requests;
    std::vector<MaterialGraphShaderPipelineDiagnostic> diagnostics;
};

[[nodiscard]] MaterialGraphShaderPipelinePlan
plan_material_graph_shader_pipeline(const IFileSystem& filesystem, const MaterialGraphShaderExportDesc& export_desc,
                                    std::string_view artifact_output_root, std::string_view shader_cache_index_relative,
                                    const ShaderToolDescriptor& dxc_tool);

/**
 * Reviewed material graph production-authoring workflow.
 *
 * This value-only plan lowers one validated `GameEngine.MaterialGraph.v1` document into a runtime material package row,
 * emits the reviewed HLSL v0 bridge plus a `GameEngine.MaterialGraphShaderExport.v0` descriptor, and returns shell-free
 * DXC compile execution requests. It does not execute compilers, mutate files, stream packages, create renderer/RHI
 * residency, or support arbitrary shader graph execution.
 */
struct MaterialGraphProductionAuthoringDesc {
    std::string package_index_path;
    std::string package_index_content;
    std::string material_graph_path;
    std::string material_graph_content;
    std::string material_output_path;
    std::string hlsl_output_path;
    std::string shader_export_output_path;
    std::uint64_t source_revision{0};
    AssetId export_id{};
    std::string export_name;
    std::string vertex_entry{"VSMain"};
    std::string fragment_entry{"PSMain"};
    std::string artifact_output_root;
    std::string shader_cache_index_relative;
    std::string shader_graph_execution{"unsupported"};
    std::string live_shader_generation{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
};

struct MaterialGraphProductionAuthoringResult {
    std::string material_content;
    std::string package_index_content;
    std::string hlsl_content;
    std::string shader_export_content;
    std::vector<MaterialInstancePackageChangedFile> changed_files;
    std::vector<ShaderCompileExecutionRequest> execution_requests;
    std::vector<MaterialInstancePackageUpdateFailure> failures;
    std::size_t lowered_material_count{0};
    std::size_t shader_export_count{0};
    std::size_t d3d12_compile_request_count{0};
    std::size_t vulkan_compile_request_count{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

[[nodiscard]] MaterialGraphProductionAuthoringResult
plan_material_graph_production_authoring(const MaterialGraphProductionAuthoringDesc& desc,
                                         const ShaderToolDescriptor& dxc_tool);

} // namespace mirakana
