// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/asset_pipeline.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] AssetKind asset_kind_from_import_action(AssetImportActionKind kind) noexcept {
    switch (kind) {
    case AssetImportActionKind::texture:
        return AssetKind::texture;
    case AssetImportActionKind::mesh:
        return AssetKind::mesh;
    case AssetImportActionKind::morph_mesh_cpu:
        return AssetKind::morph_mesh_cpu;
    case AssetImportActionKind::animation_float_clip:
        return AssetKind::animation_float_clip;
    case AssetImportActionKind::animation_quaternion_clip:
        return AssetKind::animation_quaternion_clip;
    case AssetImportActionKind::material:
        return AssetKind::material;
    case AssetImportActionKind::scene:
        return AssetKind::scene;
    case AssetImportActionKind::audio:
        return AssetKind::audio;
    case AssetImportActionKind::unknown:
        break;
    }
    return AssetKind::unknown;
}

[[nodiscard]] bool is_valid_imported_asset_record(const AssetRecord& record) noexcept {
    return record.id.value != 0 && record.kind != AssetKind::unknown && !record.path.empty();
}

[[nodiscard]] EditorAssetThumbnailKind thumbnail_kind_from_import_action(AssetImportActionKind kind) noexcept {
    switch (kind) {
    case AssetImportActionKind::texture:
        return EditorAssetThumbnailKind::texture;
    case AssetImportActionKind::mesh:
        return EditorAssetThumbnailKind::mesh;
    case AssetImportActionKind::morph_mesh_cpu:
        return EditorAssetThumbnailKind::mesh;
    case AssetImportActionKind::animation_float_clip:
        return EditorAssetThumbnailKind::mesh;
    case AssetImportActionKind::animation_quaternion_clip:
        return EditorAssetThumbnailKind::mesh;
    case AssetImportActionKind::material:
        return EditorAssetThumbnailKind::material;
    case AssetImportActionKind::scene:
        return EditorAssetThumbnailKind::scene;
    case AssetImportActionKind::audio:
        return EditorAssetThumbnailKind::audio;
    case AssetImportActionKind::unknown:
        break;
    }
    return EditorAssetThumbnailKind::unknown;
}

[[nodiscard]] std::vector<EditorMaterialPreviewTextureRow>
make_material_preview_texture_rows(const MaterialDefinition& material, const AssetRegistry* registry) {
    std::vector<EditorMaterialPreviewTextureRow> rows;
    rows.reserve(material.texture_bindings.size());
    for (const auto& binding : material.texture_bindings) {
        EditorMaterialPreviewTextureRow row;
        row.slot = binding.slot;
        row.texture = binding.texture;

        if (registry == nullptr) {
            rows.push_back(std::move(row));
            continue;
        }

        const auto* record = registry->find(binding.texture);
        if (record == nullptr) {
            row.status = EditorMaterialPreviewTextureStatus::missing;
            row.diagnostic = "texture asset is not registered";
        } else {
            row.artifact_path = record->path;
            if (record->kind == AssetKind::texture) {
                row.status = EditorMaterialPreviewTextureStatus::resolved;
            } else {
                row.status = EditorMaterialPreviewTextureStatus::wrong_kind;
                row.diagnostic = "registered asset is not a texture";
            }
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

[[nodiscard]] bool has_unresolved_texture_rows(const std::vector<EditorMaterialPreviewTextureRow>& rows) noexcept {
    return std::ranges::any_of(rows, [](const EditorMaterialPreviewTextureRow& row) {
        return row.status != EditorMaterialPreviewTextureStatus::resolved;
    });
}

[[nodiscard]] runtime::RuntimeAssetRecord make_preview_runtime_record(runtime::RuntimeAssetHandle handle, AssetId asset,
                                                                      AssetKind kind, std::string path,
                                                                      std::string content) {
    const auto content_hash = hash_asset_cooked_content(content);
    return runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = kind,
        .path = std::move(path),
        .content_hash = content_hash,
        .source_revision = 1,
        .dependencies = {},
        .content = std::move(content),
    };
}

void mark_gpu_preview_failure(EditorMaterialGpuPreviewPlan& plan, EditorMaterialGpuPreviewStatus status,
                              std::string diagnostic) {
    if (plan.status == EditorMaterialGpuPreviewStatus::ready ||
        plan.status == EditorMaterialGpuPreviewStatus::unknown) {
        plan.status = status;
        plan.diagnostic = std::move(diagnostic);
    }
}

} // namespace

void AssetPipelineState::set_import_plan(const AssetImportPlan& plan) {
    items_.clear();
    items_.reserve(plan.actions.size());
    for (const auto& action : plan.actions) {
        items_.push_back(EditorAssetImportItem{
            .asset = action.id,
            .kind = action.kind,
            .source_path = action.source_path,
            .output_path = action.output_path,
            .status = EditorAssetImportStatus::pending,
            .diagnostic = {},
        });
    }

    std::ranges::sort(items_, [](const EditorAssetImportItem& lhs, const EditorAssetImportItem& rhs) {
        if (lhs.output_path != rhs.output_path) {
            return lhs.output_path < rhs.output_path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
}

void AssetPipelineState::apply_import_updates(const std::vector<EditorAssetImportUpdate>& updates) {
    for (const auto& update : updates) {
        auto* item = find_item(update.asset);
        if (item == nullptr) {
            continue;
        }

        item->status = update.imported ? EditorAssetImportStatus::imported : EditorAssetImportStatus::failed;
        item->diagnostic = update.diagnostic;
    }
}

void AssetPipelineState::apply_import_execution_result(const AssetImportExecutionResult& result) {
    std::vector<EditorAssetImportUpdate> updates;
    updates.reserve(result.imported.size() + result.failures.size());
    for (const auto& imported : result.imported) {
        updates.push_back(EditorAssetImportUpdate{
            .asset = imported.asset,
            .imported = true,
            .diagnostic = {},
        });
    }
    for (const auto& failure : result.failures) {
        updates.push_back(EditorAssetImportUpdate{
            .asset = failure.asset,
            .imported = false,
            .diagnostic = failure.diagnostic,
        });
    }
    apply_import_updates(updates);
}

void AssetPipelineState::apply_hot_reload_events(std::vector<AssetHotReloadEvent> events) {
    std::ranges::sort(events, [](const AssetHotReloadEvent& lhs, const AssetHotReloadEvent& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
    hot_reload_events_ = std::move(events);
}

void AssetPipelineState::apply_recook_requests(std::vector<AssetHotReloadRecookRequest> requests) {
    std::ranges::sort(requests, [](const AssetHotReloadRecookRequest& lhs, const AssetHotReloadRecookRequest& rhs) {
        const auto lhs_dependency = lhs.asset != lhs.source_asset;
        const auto rhs_dependency = rhs.asset != rhs.source_asset;
        if (lhs.ready_tick != rhs.ready_tick) {
            return lhs.ready_tick < rhs.ready_tick;
        }
        if (lhs_dependency != rhs_dependency) {
            return lhs_dependency;
        }
        if (lhs.trigger_path != rhs.trigger_path) {
            return lhs.trigger_path < rhs.trigger_path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
    recook_requests_ = std::move(requests);
}

void AssetPipelineState::apply_hot_reload_results(std::vector<AssetHotReloadApplyResult> results) {
    std::ranges::sort(results, [](const AssetHotReloadApplyResult& lhs, const AssetHotReloadApplyResult& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
    hot_reload_results_ = std::move(results);
}

void AssetPipelineState::clear() noexcept {
    items_.clear();
    hot_reload_events_.clear();
    recook_requests_.clear();
    hot_reload_results_.clear();
}

const std::vector<EditorAssetImportItem>& AssetPipelineState::items() const noexcept {
    return items_;
}

const std::vector<AssetHotReloadEvent>& AssetPipelineState::hot_reload_events() const noexcept {
    return hot_reload_events_;
}

const std::vector<AssetHotReloadRecookRequest>& AssetPipelineState::recook_requests() const noexcept {
    return recook_requests_;
}

const std::vector<AssetHotReloadApplyResult>& AssetPipelineState::hot_reload_results() const noexcept {
    return hot_reload_results_;
}

std::size_t AssetPipelineState::item_count() const noexcept {
    return items_.size();
}

std::size_t AssetPipelineState::pending_count() const noexcept {
    return count_status(EditorAssetImportStatus::pending);
}

std::size_t AssetPipelineState::imported_count() const noexcept {
    return count_status(EditorAssetImportStatus::imported);
}

std::size_t AssetPipelineState::failed_count() const noexcept {
    return count_status(EditorAssetImportStatus::failed);
}

std::size_t AssetPipelineState::applied_hot_reload_count() const noexcept {
    return count_hot_reload_result(AssetHotReloadApplyResultKind::applied);
}

std::size_t AssetPipelineState::failed_hot_reload_count() const noexcept {
    return count_hot_reload_result(AssetHotReloadApplyResultKind::failed_rolled_back);
}

EditorAssetImportItem* AssetPipelineState::find_item(AssetId asset) noexcept {
    const auto it =
        std::ranges::find_if(items_, [asset](const EditorAssetImportItem& item) { return item.asset == asset; });
    return it == items_.end() ? nullptr : &*it;
}

std::size_t AssetPipelineState::count_status(EditorAssetImportStatus status) const noexcept {
    return static_cast<std::size_t>(
        std::ranges::count_if(items_, [status](const EditorAssetImportItem& item) { return item.status == status; }));
}

std::size_t AssetPipelineState::count_hot_reload_result(AssetHotReloadApplyResultKind kind) const noexcept {
    return static_cast<std::size_t>(std::ranges::count_if(
        hot_reload_results_, [kind](const AssetHotReloadApplyResult& result) { return result.kind == kind; }));
}

std::string_view editor_asset_import_status_label(EditorAssetImportStatus status) noexcept {
    switch (status) {
    case EditorAssetImportStatus::pending:
        return "Pending";
    case EditorAssetImportStatus::imported:
        return "Imported";
    case EditorAssetImportStatus::failed:
        return "Failed";
    case EditorAssetImportStatus::unknown:
        break;
    }
    return "Unknown";
}

std::string_view editor_asset_hot_reload_event_kind_label(AssetHotReloadEventKind kind) noexcept {
    switch (kind) {
    case AssetHotReloadEventKind::added:
        return "Added";
    case AssetHotReloadEventKind::modified:
        return "Modified";
    case AssetHotReloadEventKind::removed:
        return "Removed";
    case AssetHotReloadEventKind::unknown:
        break;
    }
    return "Unknown";
}

std::string_view editor_asset_hot_reload_recook_reason_label(AssetHotReloadRecookReason reason) noexcept {
    switch (reason) {
    case AssetHotReloadRecookReason::source_added:
        return "Source Added";
    case AssetHotReloadRecookReason::source_modified:
        return "Source Modified";
    case AssetHotReloadRecookReason::source_removed:
        return "Source Removed";
    case AssetHotReloadRecookReason::dependency_invalidated:
        return "Dependency Invalidated";
    case AssetHotReloadRecookReason::unknown:
        break;
    }
    return "Unknown";
}

std::string_view editor_asset_hot_reload_apply_result_label(AssetHotReloadApplyResultKind kind) noexcept {
    switch (kind) {
    case AssetHotReloadApplyResultKind::staged:
        return "Staged";
    case AssetHotReloadApplyResultKind::applied:
        return "Applied";
    case AssetHotReloadApplyResultKind::failed_rolled_back:
        return "Failed Rolled Back";
    case AssetHotReloadApplyResultKind::unknown:
        break;
    }
    return "Unknown";
}

std::string_view editor_asset_thumbnail_kind_label(EditorAssetThumbnailKind kind) noexcept {
    switch (kind) {
    case EditorAssetThumbnailKind::texture:
        return "Texture";
    case EditorAssetThumbnailKind::mesh:
        return "Mesh";
    case EditorAssetThumbnailKind::material:
        return "Material";
    case EditorAssetThumbnailKind::scene:
        return "Scene";
    case EditorAssetThumbnailKind::audio:
        return "Audio";
    case EditorAssetThumbnailKind::unknown:
        break;
    }
    return "Unknown";
}

std::string_view editor_material_preview_status_label(EditorMaterialPreviewStatus status) noexcept {
    switch (status) {
    case EditorMaterialPreviewStatus::ready:
        return "Ready";
    case EditorMaterialPreviewStatus::warning:
        return "Warning";
    case EditorMaterialPreviewStatus::missing_asset:
        return "Missing Asset";
    case EditorMaterialPreviewStatus::wrong_kind:
        return "Wrong Kind";
    case EditorMaterialPreviewStatus::missing_artifact:
        return "Missing Artifact";
    case EditorMaterialPreviewStatus::invalid_material:
        return "Invalid Material";
    case EditorMaterialPreviewStatus::unknown:
        break;
    }
    return "Unknown";
}

std::string_view editor_material_preview_texture_status_label(EditorMaterialPreviewTextureStatus status) noexcept {
    switch (status) {
    case EditorMaterialPreviewTextureStatus::resolved:
        return "Resolved";
    case EditorMaterialPreviewTextureStatus::missing:
        return "Missing";
    case EditorMaterialPreviewTextureStatus::wrong_kind:
        return "Wrong Kind";
    case EditorMaterialPreviewTextureStatus::unknown:
        break;
    }
    return "Unknown";
}

std::string_view editor_material_gpu_preview_status_label(EditorMaterialGpuPreviewStatus status) noexcept {
    switch (status) {
    case EditorMaterialGpuPreviewStatus::ready:
        return "Ready";
    case EditorMaterialGpuPreviewStatus::material_unavailable:
        return "Material Unavailable";
    case EditorMaterialGpuPreviewStatus::invalid_material_payload:
        return "Invalid Material Payload";
    case EditorMaterialGpuPreviewStatus::texture_unavailable:
        return "Texture Unavailable";
    case EditorMaterialGpuPreviewStatus::missing_texture_artifact:
        return "Missing Texture Artifact";
    case EditorMaterialGpuPreviewStatus::invalid_texture_payload:
        return "Invalid Texture Payload";
    case EditorMaterialGpuPreviewStatus::rhi_unavailable:
        return "RHI Unavailable";
    case EditorMaterialGpuPreviewStatus::render_failed:
        return "Render Failed";
    case EditorMaterialGpuPreviewStatus::unknown:
        break;
    }
    return "Unknown";
}

std::string_view editor_material_texture_slot_label(MaterialTextureSlot slot) noexcept {
    switch (slot) {
    case MaterialTextureSlot::base_color:
        return "Base Color";
    case MaterialTextureSlot::normal:
        return "Normal";
    case MaterialTextureSlot::metallic_roughness:
        return "Metallic Roughness";
    case MaterialTextureSlot::emissive:
        return "Emissive";
    case MaterialTextureSlot::occlusion:
        return "Occlusion";
    case MaterialTextureSlot::unknown:
        break;
    }
    return "Unknown";
}

EditorAssetImportProgress make_editor_asset_import_progress(const AssetPipelineState& state) noexcept {
    const auto total = state.item_count();
    const auto imported = state.imported_count();
    const auto failed = state.failed_count();
    const auto completed = imported + failed;
    return EditorAssetImportProgress{
        .total_count = total,
        .pending_count = state.pending_count(),
        .imported_count = imported,
        .failed_count = failed,
        .completed_count = completed,
        .completion_ratio = total == 0 ? 0.0F : static_cast<float>(completed) / static_cast<float>(total),
    };
}

std::vector<EditorAssetDependencyItem> make_editor_asset_dependency_items(const AssetImportPlan& plan) {
    std::vector<EditorAssetDependencyItem> items;
    items.reserve(plan.dependencies.size());
    for (const auto& edge : plan.dependencies) {
        if (!is_valid_asset_dependency_edge(edge)) {
            continue;
        }
        items.push_back(EditorAssetDependencyItem{
            .asset = edge.asset,
            .dependency = edge.dependency,
            .kind = edge.kind,
            .path = edge.path,
        });
    }
    std::ranges::sort(items, [](const EditorAssetDependencyItem& lhs, const EditorAssetDependencyItem& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        return lhs.dependency.value < rhs.dependency.value;
    });
    return items;
}

std::vector<EditorAssetThumbnailRequest> make_editor_asset_thumbnail_requests(const AssetImportPlan& plan) {
    std::vector<EditorAssetThumbnailRequest> requests;
    requests.reserve(plan.actions.size());
    for (const auto& action : plan.actions) {
        const auto kind = thumbnail_kind_from_import_action(action.kind);
        if (kind == EditorAssetThumbnailKind::unknown) {
            continue;
        }
        requests.push_back(EditorAssetThumbnailRequest{
            .asset = action.id,
            .kind = kind,
            .source_path = action.source_path,
            .output_path = action.output_path,
            .label = std::string(editor_asset_thumbnail_kind_label(kind)),
        });
    }
    std::ranges::sort(requests, [](const EditorAssetThumbnailRequest& lhs, const EditorAssetThumbnailRequest& rhs) {
        if (lhs.output_path != rhs.output_path) {
            return lhs.output_path < rhs.output_path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
    return requests;
}

EditorMaterialPreview make_editor_material_preview(const MaterialDefinition& material) {
    if (!is_valid_material_definition(material)) {
        throw std::invalid_argument("material definition is invalid for editor preview");
    }
    const auto bindings = build_material_pipeline_binding_metadata(material);
    return EditorMaterialPreview{
        .material = material.id,
        .name = material.name,
        .base_color = material.factors.base_color,
        .emissive = material.factors.emissive,
        .metallic = material.factors.metallic,
        .roughness = material.factors.roughness,
        .material_uniform_bytes = material_factor_uniform_size_bytes,
        .double_sided = bindings.double_sided,
        .requires_alpha_test = bindings.requires_alpha_test,
        .requires_alpha_blending = bindings.requires_alpha_blending,
        .bindings = bindings.bindings,
        .status = EditorMaterialPreviewStatus::ready,
        .artifact_path = {},
        .diagnostic = {},
        .texture_rows = make_material_preview_texture_rows(material, nullptr),
    };
}

EditorMaterialPreview make_editor_selected_material_preview(IFileSystem& filesystem, const AssetRegistry& registry,
                                                            AssetId material) {
    EditorMaterialPreview preview;
    preview.material = material;

    const auto* record = registry.find(material);
    if (record == nullptr) {
        preview.status = EditorMaterialPreviewStatus::missing_asset;
        preview.diagnostic = "selected material asset is not registered";
        return preview;
    }

    preview.artifact_path = record->path;
    preview.name = record->path;
    if (record->kind != AssetKind::material) {
        preview.status = EditorMaterialPreviewStatus::wrong_kind;
        preview.diagnostic = "selected asset is not a material";
        return preview;
    }

    if (!filesystem.exists(record->path)) {
        preview.status = EditorMaterialPreviewStatus::missing_artifact;
        preview.diagnostic = "material artifact is missing";
        return preview;
    }

    try {
        auto material_definition = deserialize_material_definition(filesystem.read_text(record->path));
        if (material_definition.id != material) {
            preview.status = EditorMaterialPreviewStatus::invalid_material;
            preview.diagnostic = "material artifact asset id does not match selection";
            return preview;
        }

        preview = make_editor_material_preview(material_definition);
        preview.artifact_path = record->path;
        preview.texture_rows = make_material_preview_texture_rows(material_definition, &registry);
        if (has_unresolved_texture_rows(preview.texture_rows)) {
            preview.status = EditorMaterialPreviewStatus::warning;
            preview.diagnostic = "material preview has unresolved texture dependencies";
        }
        return preview;
    } catch (const std::exception& error) {
        preview.status = EditorMaterialPreviewStatus::invalid_material;
        preview.diagnostic = std::string("material artifact is invalid: ") + error.what();
        return preview;
    }
}

EditorMaterialGpuPreviewPlan make_editor_material_gpu_preview_plan(IFileSystem& filesystem,
                                                                   const AssetRegistry& registry, AssetId material) {
    EditorMaterialGpuPreviewPlan plan;
    plan.preview = make_editor_selected_material_preview(filesystem, registry, material);

    if (plan.preview.status != EditorMaterialPreviewStatus::ready &&
        plan.preview.status != EditorMaterialPreviewStatus::warning) {
        plan.status = EditorMaterialGpuPreviewStatus::material_unavailable;
        plan.diagnostic = plan.preview.diagnostic;
        return plan;
    }

    const auto* record = registry.find(material);
    if (record == nullptr || record->kind != AssetKind::material || !filesystem.exists(record->path)) {
        plan.status = EditorMaterialGpuPreviewStatus::material_unavailable;
        plan.diagnostic =
            plan.preview.diagnostic.empty() ? "material artifact is unavailable" : plan.preview.diagnostic;
        return plan;
    }

    const auto material_content = filesystem.read_text(record->path);
    auto material_payload = runtime::runtime_material_payload(make_preview_runtime_record(
        runtime::RuntimeAssetHandle{1}, record->id, record->kind, record->path, material_content));
    if (!material_payload.succeeded()) {
        plan.status = EditorMaterialGpuPreviewStatus::invalid_material_payload;
        plan.diagnostic = material_payload.diagnostic;
        return plan;
    }

    plan.material = std::move(material_payload.payload);
    plan.status = EditorMaterialGpuPreviewStatus::ready;
    plan.textures.reserve(plan.preview.texture_rows.size());

    std::uint32_t next_handle = 2;
    for (const auto& row : plan.preview.texture_rows) {
        EditorMaterialGpuPreviewTexture texture;
        texture.slot = row.slot;
        texture.texture = row.texture;
        texture.artifact_path = row.artifact_path;

        if (row.status != EditorMaterialPreviewTextureStatus::resolved) {
            texture.diagnostic =
                row.diagnostic.empty() ? "material texture dependency is not resolved" : row.diagnostic;
            mark_gpu_preview_failure(plan, EditorMaterialGpuPreviewStatus::texture_unavailable,
                                     "material gpu preview texture dependency is unavailable");
            plan.textures.push_back(std::move(texture));
            continue;
        }

        if (!filesystem.exists(row.artifact_path)) {
            texture.diagnostic = "texture artifact is missing";
            mark_gpu_preview_failure(plan, EditorMaterialGpuPreviewStatus::missing_texture_artifact,
                                     "material gpu preview texture artifact is missing");
            plan.textures.push_back(std::move(texture));
            continue;
        }

        const auto texture_content = filesystem.read_text(row.artifact_path);
        auto texture_payload = runtime::runtime_texture_payload(
            make_preview_runtime_record(runtime::RuntimeAssetHandle{next_handle++}, row.texture, AssetKind::texture,
                                        row.artifact_path, texture_content));
        if (!texture_payload.succeeded()) {
            texture.diagnostic = texture_payload.diagnostic;
            mark_gpu_preview_failure(plan, EditorMaterialGpuPreviewStatus::invalid_texture_payload,
                                     "material gpu preview texture payload is invalid");
            plan.textures.push_back(std::move(texture));
            continue;
        }

        texture.payload = std::move(texture_payload.payload);
        plan.textures.push_back(std::move(texture));
    }

    return plan;
}

std::vector<EditorAssetImportDiagnosticItem> make_editor_asset_import_diagnostics(const AssetPipelineState& state) {
    std::vector<EditorAssetImportDiagnosticItem> diagnostics;
    for (const auto& item : state.items()) {
        if (item.status != EditorAssetImportStatus::failed || item.diagnostic.empty()) {
            continue;
        }
        diagnostics.push_back(EditorAssetImportDiagnosticItem{
            .asset = item.asset,
            .kind = item.kind,
            .output_path = item.output_path,
            .diagnostic = item.diagnostic,
        });
    }

    std::ranges::sort(diagnostics,
                      [](const EditorAssetImportDiagnosticItem& lhs, const EditorAssetImportDiagnosticItem& rhs) {
                          if (lhs.output_path != rhs.output_path) {
                              return lhs.output_path < rhs.output_path;
                          }
                          return lhs.asset.value < rhs.asset.value;
                      });
    return diagnostics;
}

EditorAssetPipelinePanelModel
make_editor_asset_pipeline_panel_model(const AssetPipelineState& state, const AssetImportPlan& plan,
                                       const std::vector<MaterialDefinition>& preview_materials) {
    EditorAssetPipelinePanelModel model;
    model.progress = make_editor_asset_import_progress(state);
    model.dependencies = make_editor_asset_dependency_items(plan);
    model.thumbnail_requests = make_editor_asset_thumbnail_requests(plan);
    model.diagnostics = make_editor_asset_import_diagnostics(state);

    model.material_previews.reserve(preview_materials.size());
    for (const auto& material : preview_materials) {
        if (!is_valid_material_definition(material)) {
            continue;
        }
        model.material_previews.push_back(make_editor_material_preview(material));
    }
    std::ranges::sort(model.material_previews, [](const EditorMaterialPreview& lhs, const EditorMaterialPreview& rhs) {
        if (lhs.name != rhs.name) {
            return lhs.name < rhs.name;
        }
        return lhs.material.value < rhs.material.value;
    });
    return model;
}

std::vector<AssetRecord> make_imported_asset_records(const AssetImportExecutionResult& result) {
    std::vector<AssetRecord> records;
    records.reserve(result.imported.size());
    for (const auto& imported : result.imported) {
        AssetRecord record{
            .id = imported.asset,
            .kind = asset_kind_from_import_action(imported.kind),
            .path = imported.output_path,
        };
        if (is_valid_imported_asset_record(record)) {
            records.push_back(std::move(record));
        }
    }

    std::ranges::sort(records, [](const AssetRecord& lhs, const AssetRecord& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.id.value < rhs.id.value;
    });
    return records;
}

std::size_t add_imported_asset_records(AssetRegistry& registry, const AssetImportExecutionResult& result) {
    std::size_t added = 0;
    for (auto record : make_imported_asset_records(result)) {
        if (registry.find(record.id) != nullptr) {
            continue;
        }
        if (registry.try_add(std::move(record))) {
            ++added;
        }
    }
    return added;
}

} // namespace mirakana::editor
