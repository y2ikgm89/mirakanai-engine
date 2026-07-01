// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/asset_browser_production.hpp"
#include "mirakana/editor/ui_model.hpp"
#include "mirakana/tools/gltf_mesh_inspect.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::editor {

enum class EditorGltfMeshInspectSelectionStatus : std::uint8_t { empty, unsupported, ready, blocked };

/// Value-only Assets-selection input for glTF mesh review. Editor core must receive a prebuilt inspect report from the
/// host/tooling layer; this API does not read source files, run importers, mutate project files, package assets, or
/// expose native handles.
struct EditorGltfMeshInspectSelectionDesc {
    AssetId asset;
    std::string asset_key_label;
    std::string source_path;
    const mirakana::GltfMeshInspectReport* report{nullptr};
    bool selected{false};
    bool source_visible{false};
    bool mutates_project_files{false};
    bool executes_import_tools{false};
    bool executes_package_scripts{false};
    bool executes_validation_recipes{false};
    bool exposes_native_handles{false};
};

struct EditorGltfMeshInspectSelectionModel {
    EditorGltfMeshInspectSelectionStatus status{EditorGltfMeshInspectSelectionStatus::empty};
    std::string status_label{"glTF mesh inspect empty"};
    std::string row_id;
    AssetId asset;
    std::string asset_key_label;
    std::string source_path;
    std::vector<EditorPropertyRow> inspector_rows;
    std::vector<EditorAssetBrowserRetainedCommandRow> command_rows;
    std::vector<EditorAssetBrowserImportWorkflowRow> import_workflow_rows;
    std::vector<std::string> diagnostics;
    std::uint64_t primitive_count{0U};
    std::uint64_t warning_count{0U};
    bool inspect_supported{false};
    bool selected{false};
    bool source_visible{false};
    bool ready{false};
    bool blocked{false};
    bool mutates_project_files{false};
    bool executes_import_tools{false};
    bool executes_package_scripts{false};
    bool executes_validation_recipes{false};
    bool exposes_native_handles{false};
};

[[nodiscard]] std::string_view
editor_gltf_mesh_inspect_selection_status_label(EditorGltfMeshInspectSelectionStatus status) noexcept;

[[nodiscard]] EditorGltfMeshInspectSelectionModel
make_editor_gltf_mesh_inspect_selection_model(const EditorGltfMeshInspectSelectionDesc& desc);

[[nodiscard]] EditorAssetBrowserRetainedUiDesc
make_editor_gltf_mesh_inspect_selection_retained_ui_desc(const EditorGltfMeshInspectSelectionModel& model);

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
