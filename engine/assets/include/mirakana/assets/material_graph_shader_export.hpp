// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/material.hpp"
#include "mirakana/assets/material_graph.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

/**
 * Canonical text format id for the Phase-4 shader-generation bridge document.
 * This slice keeps authored HLSL on disk (repository-relative paths) and uses `mirakana_tools` shader runners only.
 */
inline constexpr std::string_view material_graph_shader_export_format_id = "GameEngine.MaterialGraphShaderExport.v0";

/**
 * Declares how a validated `GameEngine.MaterialGraph.v1` (or future graph) maps to first-party HLSL entry points.
 * `hlsl_source_path` is repository-relative and must exist before compile planning.
 */
struct MaterialGraphShaderExportDesc {
    AssetId export_id{};
    std::string export_name;
    /** Optional reference to a `.materialgraph` text asset for operator traceability; may be empty. */
    std::string material_graph_path;
    /** Repository-relative path to merged HLSL (typically generated or hand-authored). */
    std::string hlsl_source_path;
    std::string vertex_entry;
    std::string fragment_entry;
};

enum class MaterialGraphShaderExportDiagnosticCode {
    unknown,
    invalid_format,
    duplicate_key,
    missing_key,
    invalid_token,
    unsafe_path,
};

struct MaterialGraphShaderExportDiagnostic {
    MaterialGraphShaderExportDiagnosticCode code{MaterialGraphShaderExportDiagnosticCode::unknown};
    std::string field;
    std::string message;
};

[[nodiscard]] bool operator==(const MaterialGraphShaderExportDesc& lhs,
                              const MaterialGraphShaderExportDesc& rhs) noexcept;

[[nodiscard]] std::string serialize_material_graph_shader_export(const MaterialGraphShaderExportDesc& desc);
[[nodiscard]] MaterialGraphShaderExportDesc deserialize_material_graph_shader_export(std::string_view text);
[[nodiscard]] std::vector<MaterialGraphShaderExportDiagnostic>
validate_material_graph_shader_export(const MaterialGraphShaderExportDesc& desc);

/**
 * Deterministic HLSL v0 stub synthesized from a validated material graph by lowering to `MaterialDefinition`.
 * Intended for reviewed compilation through `plan_material_graph_shader_pipeline`; not a production shading model.
 */
[[nodiscard]] std::string emit_material_graph_reviewed_hlsl_v0(const MaterialGraphDesc& graph);

} // namespace mirakana
