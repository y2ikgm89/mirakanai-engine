// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/ui_model.hpp"
#include "mirakana/tools/gltf_mesh_inspect.hpp"

#include <string>
#include <vector>

namespace mirakana::editor {

/// Maps `mirakana::GltfMeshInspectReport` into deterministic inspector rows for editor shells that render `mirakana_ui`
/// documents. This is a read-only catalog surface: it does not cook assets, mutate scenes, or execute importers.
[[nodiscard]] std::vector<EditorPropertyRow>
gltf_mesh_inspect_report_to_inspector_rows(const mirakana::GltfMeshInspectReport& report);

/// Builds a retained-mode `UiDocument` for AI/editor diagnostics: root `gltf_mesh_inspect`, contract label
/// `ge.editor.gltf_mesh_inspect.v1`, and per-row `gltf_mesh_inspect.rows.<row.id>.caption` / `.text` aligned with
/// `gltf_mesh_inspect_report_to_inspector_rows`.
[[nodiscard]] mirakana::ui::UiDocument make_gltf_mesh_inspect_ui_model(const mirakana::GltfMeshInspectReport& report);

/// True when `path` ends with `.gltf` or `.glb` using ASCII case-insensitive suffix rules (for Assets-driven inspect).
[[nodiscard]] bool editor_asset_path_supports_gltf_mesh_inspect(std::string_view path) noexcept;

} // namespace mirakana::editor
