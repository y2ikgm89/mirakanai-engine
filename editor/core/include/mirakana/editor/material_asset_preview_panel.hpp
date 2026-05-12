// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/editor/asset_pipeline.hpp"
#include "mirakana/editor/viewport_shader_artifacts.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorMaterialAssetPreviewPanelStatus { empty, ready, attention };

struct EditorMaterialAssetPreviewTexturePayloadRow {
    std::string id;
    std::string slot_label;
    AssetId texture;
    std::string artifact_path;
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::size_t byte_count{0};
    std::string diagnostic;
    bool ready{false};
};

struct EditorMaterialAssetPreviewShaderRow {
    std::string id;
    std::string label;
    AssetId shader;
    ShaderCompileTarget target{ShaderCompileTarget::unknown};
    std::string target_label;
    std::string vertex_status_label;
    std::string fragment_status_label;
    std::string status_label;
    bool vertex_ready{false};
    bool fragment_ready{false};
    bool ready{false};
    bool required{false};
};

/// Deterministic display-parity checklist rows for MK_ui
/// (`material_asset_preview.gpu.execution.parity_checklist.rows.*`).
struct EditorMaterialGpuPreviewDisplayParityChecklistRow {
    std::string id;
    /// `complete`, `pending`, or `not-applicable`.
    std::string status_label;
    std::string detail_label;
};

struct EditorMaterialGpuPreviewExecutionSnapshot {
    EditorMaterialGpuPreviewStatus status{EditorMaterialGpuPreviewStatus::unknown};
    std::string diagnostic;
    std::string backend_label;
    std::string display_path_label;
    std::uint64_t frames_rendered{0};
};

struct EditorMaterialAssetPreviewPanelModel {
    EditorMaterialAssetPreviewPanelStatus status{EditorMaterialAssetPreviewPanelStatus::empty};
    std::string status_label;
    AssetId material;
    std::string material_id;
    std::string material_path;
    std::string material_name;
    EditorMaterialPreview preview;
    EditorMaterialGpuPreviewPlan gpu_plan;
    std::string gpu_status_label;
    std::vector<EditorMaterialAssetPreviewTexturePayloadRow> texture_payload_rows;
    std::vector<EditorMaterialAssetPreviewShaderRow> shader_rows;
    std::string required_shader_row_id;
    std::string gpu_execution_status_label;
    std::string gpu_execution_diagnostic;
    std::string gpu_execution_backend_label;
    std::string gpu_execution_display_path_label;
    std::uint64_t gpu_execution_frames_rendered{0};
    /// `complete`, `pending`, or `not-applicable` (Vulkan-scoped visible refresh evidence from snapshots only).
    std::string gpu_execution_vulkan_visible_refresh_evidence;
    /// `complete`, `pending`, or `not-applicable` (Metal-scoped visible refresh evidence from snapshots only).
    std::string gpu_execution_metal_visible_refresh_evidence;
    std::vector<EditorMaterialGpuPreviewDisplayParityChecklistRow> gpu_display_parity_checklist_rows;
    bool has_material_record{false};
    bool material_preview_ready{false};
    bool gpu_payload_ready{false};
    bool required_shader_ready{false};
    bool gpu_execution_host_owned{true};
    bool gpu_execution_ready{false};
    bool gpu_execution_rendered{false};
    bool mutates{false};
    bool executes{false};
};

[[nodiscard]] std::string_view
editor_material_asset_preview_panel_status_label(EditorMaterialAssetPreviewPanelStatus status) noexcept;
[[nodiscard]] EditorMaterialAssetPreviewPanelModel
make_editor_material_asset_preview_panel_model(IFileSystem& filesystem, const AssetRegistry& registry, AssetId material,
                                               const ViewportShaderArtifactState& shader_artifacts);
void apply_editor_material_gpu_preview_execution_snapshot(EditorMaterialAssetPreviewPanelModel& model,
                                                          const EditorMaterialGpuPreviewExecutionSnapshot& snapshot);
/// Host-owned Vulkan material-preview refresh evidence derived only from `snapshot` (no RHI work).
[[nodiscard]] std::string_view editor_material_gpu_preview_vulkan_visible_refresh_evidence(
    const EditorMaterialGpuPreviewExecutionSnapshot& snapshot) noexcept;
/// Host-owned Metal material-preview refresh evidence derived only from `snapshot` (no RHI work).
[[nodiscard]] std::string_view editor_material_gpu_preview_metal_visible_refresh_evidence(
    const EditorMaterialGpuPreviewExecutionSnapshot& snapshot) noexcept;
[[nodiscard]] mirakana::ui::UiDocument
make_material_asset_preview_panel_ui_model(const EditorMaterialAssetPreviewPanelModel& model);

} // namespace mirakana::editor
