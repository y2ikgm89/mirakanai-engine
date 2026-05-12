// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/material_graph_shader_export.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/tools/shader_compile_action.hpp"
#include "mirakana/tools/shader_toolchain.hpp"

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

} // namespace mirakana
