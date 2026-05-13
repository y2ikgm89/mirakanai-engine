// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_hot_reload.hpp"
#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/tools/asset_import_tool.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorAssetImportStatus : std::uint8_t { unknown, pending, imported, failed };

struct EditorAssetImportItem {
    AssetId asset;
    AssetImportActionKind kind{AssetImportActionKind::unknown};
    std::string source_path;
    std::string output_path;
    EditorAssetImportStatus status{EditorAssetImportStatus::unknown};
    std::string diagnostic;
};

struct EditorAssetImportUpdate {
    AssetId asset;
    bool imported{false};
    std::string diagnostic;
};

struct EditorAssetImportProgress {
    std::size_t total_count{0};
    std::size_t pending_count{0};
    std::size_t imported_count{0};
    std::size_t failed_count{0};
    std::size_t completed_count{0};
    float completion_ratio{0.0F};
};

struct EditorAssetDependencyItem {
    AssetId asset;
    AssetId dependency;
    AssetDependencyKind kind{AssetDependencyKind::unknown};
    std::string path;
};

enum class EditorAssetThumbnailKind : std::uint8_t { unknown, texture, mesh, material, scene, audio };

enum class EditorMaterialPreviewStatus : std::uint8_t {
    unknown,
    ready,
    warning,
    missing_asset,
    wrong_kind,
    missing_artifact,
    invalid_material,
};

enum class EditorMaterialPreviewTextureStatus : std::uint8_t { unknown, resolved, missing, wrong_kind };

enum class EditorMaterialGpuPreviewStatus : std::uint8_t {
    unknown,
    ready,
    material_unavailable,
    invalid_material_payload,
    texture_unavailable,
    missing_texture_artifact,
    invalid_texture_payload,
    rhi_unavailable,
    render_failed,
};

struct EditorAssetThumbnailRequest {
    AssetId asset;
    EditorAssetThumbnailKind kind{EditorAssetThumbnailKind::unknown};
    std::string source_path;
    std::string output_path;
    std::string label;
};

struct EditorMaterialPreviewTextureRow {
    MaterialTextureSlot slot{MaterialTextureSlot::unknown};
    AssetId texture;
    std::string artifact_path;
    EditorMaterialPreviewTextureStatus status{EditorMaterialPreviewTextureStatus::unknown};
    std::string diagnostic;
};

struct EditorMaterialPreview {
    AssetId material;
    std::string name;
    std::array<float, 4> base_color{1.0F, 1.0F, 1.0F, 1.0F};
    std::array<float, 3> emissive{0.0F, 0.0F, 0.0F};
    float metallic{0.0F};
    float roughness{1.0F};
    std::uint64_t material_uniform_bytes{0};
    bool double_sided{false};
    bool requires_alpha_test{false};
    bool requires_alpha_blending{false};
    std::vector<MaterialPipelineBinding> bindings;
    EditorMaterialPreviewStatus status{EditorMaterialPreviewStatus::unknown};
    std::string artifact_path;
    std::string diagnostic;
    std::vector<EditorMaterialPreviewTextureRow> texture_rows;
};

struct EditorMaterialGpuPreviewTexture {
    MaterialTextureSlot slot{MaterialTextureSlot::unknown};
    AssetId texture;
    std::string artifact_path;
    runtime::RuntimeTexturePayload payload;
    std::string diagnostic;
};

struct EditorMaterialGpuPreviewPlan {
    EditorMaterialPreview preview;
    runtime::RuntimeMaterialPayload material;
    std::vector<EditorMaterialGpuPreviewTexture> textures;
    EditorMaterialGpuPreviewStatus status{EditorMaterialGpuPreviewStatus::unknown};
    std::string diagnostic;

    [[nodiscard]] bool ready() const noexcept {
        return status == EditorMaterialGpuPreviewStatus::ready;
    }
};

struct EditorAssetImportDiagnosticItem {
    AssetId asset;
    AssetImportActionKind kind{AssetImportActionKind::unknown};
    std::string output_path;
    std::string diagnostic;
};

struct EditorAssetPipelinePanelModel {
    EditorAssetImportProgress progress;
    std::vector<EditorAssetDependencyItem> dependencies;
    std::vector<EditorAssetThumbnailRequest> thumbnail_requests;
    std::vector<EditorMaterialPreview> material_previews;
    std::vector<EditorAssetImportDiagnosticItem> diagnostics;
};

class AssetPipelineState {
  public:
    void set_import_plan(const AssetImportPlan& plan);
    void apply_import_updates(const std::vector<EditorAssetImportUpdate>& updates);
    void apply_import_execution_result(const AssetImportExecutionResult& result);
    void apply_hot_reload_events(std::vector<AssetHotReloadEvent> events);
    void apply_recook_requests(std::vector<AssetHotReloadRecookRequest> requests);
    void apply_hot_reload_results(std::vector<AssetHotReloadApplyResult> results);
    void clear() noexcept;

    [[nodiscard]] const std::vector<EditorAssetImportItem>& items() const noexcept;
    [[nodiscard]] const std::vector<AssetHotReloadEvent>& hot_reload_events() const noexcept;
    [[nodiscard]] const std::vector<AssetHotReloadRecookRequest>& recook_requests() const noexcept;
    [[nodiscard]] const std::vector<AssetHotReloadApplyResult>& hot_reload_results() const noexcept;
    [[nodiscard]] std::size_t item_count() const noexcept;
    [[nodiscard]] std::size_t pending_count() const noexcept;
    [[nodiscard]] std::size_t imported_count() const noexcept;
    [[nodiscard]] std::size_t failed_count() const noexcept;
    [[nodiscard]] std::size_t applied_hot_reload_count() const noexcept;
    [[nodiscard]] std::size_t failed_hot_reload_count() const noexcept;

  private:
    [[nodiscard]] EditorAssetImportItem* find_item(AssetId asset) noexcept;
    [[nodiscard]] std::size_t count_status(EditorAssetImportStatus status) const noexcept;
    [[nodiscard]] std::size_t count_hot_reload_result(AssetHotReloadApplyResultKind kind) const noexcept;

    std::vector<EditorAssetImportItem> items_;
    std::vector<AssetHotReloadEvent> hot_reload_events_;
    std::vector<AssetHotReloadRecookRequest> recook_requests_;
    std::vector<AssetHotReloadApplyResult> hot_reload_results_;
};

[[nodiscard]] std::string_view editor_asset_import_status_label(EditorAssetImportStatus status) noexcept;
[[nodiscard]] std::string_view editor_asset_hot_reload_event_kind_label(AssetHotReloadEventKind kind) noexcept;
[[nodiscard]] std::string_view editor_asset_hot_reload_recook_reason_label(AssetHotReloadRecookReason reason) noexcept;
[[nodiscard]] std::string_view editor_asset_hot_reload_apply_result_label(AssetHotReloadApplyResultKind kind) noexcept;
[[nodiscard]] std::string_view editor_asset_thumbnail_kind_label(EditorAssetThumbnailKind kind) noexcept;
[[nodiscard]] std::string_view editor_material_preview_status_label(EditorMaterialPreviewStatus status) noexcept;
[[nodiscard]] std::string_view
editor_material_preview_texture_status_label(EditorMaterialPreviewTextureStatus status) noexcept;
[[nodiscard]] std::string_view editor_material_gpu_preview_status_label(EditorMaterialGpuPreviewStatus status) noexcept;
[[nodiscard]] std::string_view editor_material_texture_slot_label(MaterialTextureSlot slot) noexcept;
[[nodiscard]] EditorAssetImportProgress make_editor_asset_import_progress(const AssetPipelineState& state) noexcept;
[[nodiscard]] std::vector<EditorAssetDependencyItem> make_editor_asset_dependency_items(const AssetImportPlan& plan);
[[nodiscard]] std::vector<EditorAssetThumbnailRequest>
make_editor_asset_thumbnail_requests(const AssetImportPlan& plan);
[[nodiscard]] EditorMaterialPreview make_editor_material_preview(const MaterialDefinition& material);
[[nodiscard]] EditorMaterialPreview
make_editor_selected_material_preview(IFileSystem& filesystem, const AssetRegistry& registry, AssetId material);
[[nodiscard]] EditorMaterialGpuPreviewPlan
make_editor_material_gpu_preview_plan(IFileSystem& filesystem, const AssetRegistry& registry, AssetId material);
[[nodiscard]] std::vector<EditorAssetImportDiagnosticItem>
make_editor_asset_import_diagnostics(const AssetPipelineState& state);
[[nodiscard]] EditorAssetPipelinePanelModel
make_editor_asset_pipeline_panel_model(const AssetPipelineState& state, const AssetImportPlan& plan,
                                       const std::vector<MaterialDefinition>& preview_materials);
[[nodiscard]] std::vector<AssetRecord> make_imported_asset_records(const AssetImportExecutionResult& result);
std::size_t add_imported_asset_records(AssetRegistry& registry, const AssetImportExecutionResult& result);

} // namespace mirakana::editor
