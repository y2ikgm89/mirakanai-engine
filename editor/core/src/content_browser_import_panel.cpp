// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/content_browser_import_panel.hpp"

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/editor/asset_pipeline.hpp"
#include "mirakana/editor/content_browser.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/ui/ui.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string asset_id_string(AssetId asset) {
    return std::to_string(asset.value);
}

[[nodiscard]] std::string sanitize_text(std::string_view value) {
    if (value.empty()) {
        return "-";
    }

    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        if (character == '\n' || character == '\r') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text;
}

[[nodiscard]] std::string import_action_kind_label(AssetImportActionKind kind) {
    switch (kind) {
    case AssetImportActionKind::texture:
        return "Texture";
    case AssetImportActionKind::mesh:
        return "Mesh";
    case AssetImportActionKind::morph_mesh_cpu:
        return "Morph Mesh (CPU)";
    case AssetImportActionKind::animation_float_clip:
        return "Animation Float Clip";
    case AssetImportActionKind::animation_quaternion_clip:
        return "Animation Quaternion Clip";
    case AssetImportActionKind::material:
        return "Material";
    case AssetImportActionKind::scene:
        return "Scene";
    case AssetImportActionKind::audio:
        return "Audio";
    case AssetImportActionKind::unknown:
        break;
    }
    return "Unknown";
}

[[nodiscard]] bool contains_line_separator(std::string_view value) noexcept {
    return value.find('\n') != std::string_view::npos || value.find('\r') != std::string_view::npos;
}

void require_safe_field(std::string_view field, std::string_view value) {
    if (value.empty() || contains_line_separator(value) || value.find('=') != std::string_view::npos) {
        throw std::invalid_argument(std::string("content browser import ui field is invalid: ") + std::string(field));
    }
}

void require_safe_optional_field(std::string_view field, std::string_view value) {
    if (contains_line_separator(value) || value.find('=') != std::string_view::npos) {
        throw std::invalid_argument(std::string("content browser import ui field is invalid: ") + std::string(field));
    }
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("content browser import ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::ElementDesc make_root(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_child(std::string id, mirakana::ui::ElementId parent,
                                                   mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.parent = std::move(parent);
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    require_safe_optional_field("label", label);
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

void append_asset_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                       const std::vector<EditorContentBrowserAssetRow>& rows) {
    mirakana::ui::ElementDesc list =
        make_child("content_browser_import.assets", root, mirakana::ui::SemanticRole::list);
    list.text = make_text("Assets");
    add_or_throw(document, std::move(list));
    const mirakana::ui::ElementId list_id{"content_browser_import.assets"};

    for (const auto& row : rows) {
        require_safe_field("asset.id", row.id);
        require_safe_field("asset.path", row.path);
        require_safe_field("asset.kind_label", row.kind_label);
        require_safe_optional_field("asset.asset_key", row.asset_key_label);
        require_safe_optional_field("asset.identity_source_path", row.identity_source_path);
        require_safe_field("asset.identity_status", row.identity_status_label);

        const auto prefix = "content_browser_import.assets." + row.id;
        mirakana::ui::ElementDesc item = make_child(prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.display_name.empty() ? row.path : row.display_name);
        item.enabled = true;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{prefix};
        append_label(document, item_id, prefix + ".path", row.path);
        append_label(document, item_id, prefix + ".kind", row.kind_label);
        append_label(document, item_id, prefix + ".asset_key", row.asset_key_label.empty() ? "-" : row.asset_key_label);
        append_label(document, item_id, prefix + ".identity_source_path",
                     row.identity_source_path.empty() ? "-" : row.identity_source_path);
        append_label(document, item_id, prefix + ".identity_status", row.identity_status_label);
        append_label(document, item_id, prefix + ".selected", row.selected ? "selected" : "not_selected");
    }
}

void append_import_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                        const std::vector<EditorContentBrowserImportQueueRow>& rows) {
    mirakana::ui::ElementDesc list =
        make_child("content_browser_import.imports", root, mirakana::ui::SemanticRole::list);
    list.text = make_text("Import Queue");
    add_or_throw(document, std::move(list));
    const mirakana::ui::ElementId list_id{"content_browser_import.imports"};

    for (const auto& row : rows) {
        require_safe_field("import.id", row.id);
        require_safe_field("import.output_path", row.output_path);
        require_safe_field("import.status_label", row.status_label);
        require_safe_optional_field("import.source_path", row.source_path);
        require_safe_optional_field("import.diagnostic", row.diagnostic);

        const auto prefix = "content_browser_import.imports." + row.id;
        mirakana::ui::ElementDesc item = make_child(prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.output_path);
        item.enabled = !row.failed;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{prefix};
        append_label(document, item_id, prefix + ".status", row.status_label);
        append_label(document, item_id, prefix + ".kind", row.kind_label);
        append_label(document, item_id, prefix + ".source", row.source_path);
        append_label(document, item_id, prefix + ".output", row.output_path);
        append_label(document, item_id, prefix + ".diagnostic", row.diagnostic.empty() ? "-" : row.diagnostic);
    }
}

void append_diagnostic_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                            const std::vector<EditorAssetImportDiagnosticItem>& rows) {
    mirakana::ui::ElementDesc list =
        make_child("content_browser_import.diagnostics", root, mirakana::ui::SemanticRole::list);
    list.text = make_text("Import Diagnostics");
    add_or_throw(document, std::move(list));
    const mirakana::ui::ElementId list_id{"content_browser_import.diagnostics"};

    for (const auto& row : rows) {
        const auto id = asset_id_string(row.asset);
        require_safe_field("diagnostic.output_path", row.output_path);
        require_safe_field("diagnostic.message", row.diagnostic);

        const auto prefix = "content_browser_import.diagnostics." + id;
        mirakana::ui::ElementDesc item = make_child(prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.output_path);
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{prefix};
        append_label(document, item_id, prefix + ".message", row.diagnostic);
    }
}

void append_dependency_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                            const std::vector<EditorAssetDependencyItem>& rows) {
    mirakana::ui::ElementDesc list =
        make_child("content_browser_import.dependencies", root, mirakana::ui::SemanticRole::list);
    list.text = make_text("Import Dependencies");
    add_or_throw(document, std::move(list));
    const mirakana::ui::ElementId list_id{"content_browser_import.dependencies"};

    std::size_t index = 1;
    for (const auto& row : rows) {
        require_safe_field("dependency.path", row.path);
        const auto prefix = "content_browser_import.dependencies." + std::to_string(index);
        mirakana::ui::ElementDesc item = make_child(prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.path);
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{prefix};
        append_label(document, item_id, prefix + ".path", row.path);
        ++index;
    }
}

void append_thumbnail_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                           const std::vector<EditorAssetThumbnailRequest>& rows) {
    mirakana::ui::ElementDesc list =
        make_child("content_browser_import.thumbnails", root, mirakana::ui::SemanticRole::list);
    list.text = make_text("Thumbnail Requests");
    add_or_throw(document, std::move(list));
    const mirakana::ui::ElementId list_id{"content_browser_import.thumbnails"};

    std::size_t index = 1;
    for (const auto& row : rows) {
        require_safe_field("thumbnail.output_path", row.output_path);
        const auto prefix = "content_browser_import.thumbnails." + std::to_string(index);
        mirakana::ui::ElementDesc item = make_child(prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label.empty() ? std::string(editor_asset_thumbnail_kind_label(row.kind)) : row.label);
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{prefix};
        append_label(document, item_id, prefix + ".path", row.output_path);
        ++index;
    }
}

void append_material_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                          const std::vector<EditorMaterialPreview>& rows) {
    mirakana::ui::ElementDesc list =
        make_child("content_browser_import.materials", root, mirakana::ui::SemanticRole::list);
    list.text = make_text("Material Previews");
    add_or_throw(document, std::move(list));
    const mirakana::ui::ElementId list_id{"content_browser_import.materials"};

    for (const auto& row : rows) {
        const auto id = asset_id_string(row.material);
        const auto label = row.name.empty() ? row.artifact_path : row.name;
        require_safe_optional_field("material.label", label);
        const auto prefix = "content_browser_import.materials." + id;
        mirakana::ui::ElementDesc item = make_child(prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(label.empty() ? id : label);
        item.enabled =
            row.status == EditorMaterialPreviewStatus::ready || row.status == EditorMaterialPreviewStatus::warning;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{prefix};
        append_label(document, item_id, prefix + ".status",
                     std::string(editor_material_preview_status_label(row.status)));
        append_label(document, item_id, prefix + ".uniform_bytes", std::to_string(row.material_uniform_bytes));
    }
}

void append_hot_reload_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                            const std::vector<EditorContentBrowserHotReloadSummaryRow>& rows) {
    mirakana::ui::ElementDesc list =
        make_child("content_browser_import.hot_reload", root, mirakana::ui::SemanticRole::list);
    list.text = make_text("Hot Reload");
    add_or_throw(document, std::move(list));
    const mirakana::ui::ElementId list_id{"content_browser_import.hot_reload"};

    for (const auto& row : rows) {
        require_safe_field("hot_reload.id", row.id);
        require_safe_field("hot_reload.label", row.label);
        const auto prefix = "content_browser_import.hot_reload." + row.id;
        mirakana::ui::ElementDesc item = make_child(prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = !row.attention;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{prefix};
        append_label(document, item_id, prefix + ".count", std::to_string(row.count));
    }
}

[[nodiscard]] std::vector<EditorContentBrowserAssetRow> make_asset_rows(const ContentBrowserState& browser) {
    std::vector<EditorContentBrowserAssetRow> rows;
    const auto visible_items = browser.visible_items();
    rows.reserve(visible_items.size());
    const auto* selected = browser.selected_asset();
    for (const auto& item : visible_items) {
        rows.push_back(EditorContentBrowserAssetRow{
            .asset = item.id,
            .kind = item.kind,
            .id = asset_id_string(item.id),
            .kind_label = std::string(asset_kind_label(item.kind)),
            .asset_key_label = item.asset_key_label,
            .identity_source_path = item.identity_source_path,
            .identity_status_label = item.identity_backed ? "identity_v2" : "unannotated",
            .path = item.path,
            .display_name = item.display_name,
            .directory = item.directory,
            .identity_backed = item.identity_backed,
            .selected = selected != nullptr && selected->id == item.id,
        });
    }
    return rows;
}

[[nodiscard]] std::vector<EditorContentBrowserImportQueueRow> make_import_rows(const AssetPipelineState& pipeline) {
    std::vector<EditorContentBrowserImportQueueRow> rows;
    rows.reserve(pipeline.items().size());
    for (const auto& item : pipeline.items()) {
        rows.push_back(EditorContentBrowserImportQueueRow{
            .asset = item.asset,
            .kind = item.kind,
            .id = asset_id_string(item.asset),
            .kind_label = import_action_kind_label(item.kind),
            .status_label = std::string(editor_asset_import_status_label(item.status)),
            .source_path = item.source_path,
            .output_path = item.output_path,
            .diagnostic = item.diagnostic,
            .failed = item.status == EditorAssetImportStatus::failed,
        });
    }
    return rows;
}

[[nodiscard]] std::vector<EditorContentBrowserHotReloadSummaryRow>
make_hot_reload_summary_rows(const AssetPipelineState& pipeline) {
    return {
        EditorContentBrowserHotReloadSummaryRow{
            .id = "events", .label = "Changed", .count = pipeline.hot_reload_events().size(), .attention = false},
        EditorContentBrowserHotReloadSummaryRow{
            .id = "recook", .label = "Recook", .count = pipeline.recook_requests().size(), .attention = false},
        EditorContentBrowserHotReloadSummaryRow{
            .id = "applied", .label = "Applied", .count = pipeline.applied_hot_reload_count(), .attention = false},
        EditorContentBrowserHotReloadSummaryRow{.id = "failed",
                                                .label = "Failed",
                                                .count = pipeline.failed_hot_reload_count(),
                                                .attention = pipeline.failed_hot_reload_count() > 0},
    };
}

[[nodiscard]] bool is_supported_import_source_path(std::string_view path) noexcept {
    constexpr std::array<std::string_view, 11> extensions{
        ".texture", ".mesh", ".material", ".scene", ".audio_source", ".png", ".gltf", ".glb", ".wav", ".mp3", ".flac",
    };

    return std::ranges::any_of(extensions, [path](std::string_view extension) { return path.ends_with(extension); });
}

[[nodiscard]] std::string import_open_dialog_status(std::string_view state) {
    return "Asset import open dialog " + std::string(state);
}

[[nodiscard]] std::string external_source_copy_status_label(EditorContentBrowserImportExternalSourceCopyStatus status) {
    switch (status) {
    case EditorContentBrowserImportExternalSourceCopyStatus::idle:
        return "External import source copy idle";
    case EditorContentBrowserImportExternalSourceCopyStatus::ready:
        return "External import source copy ready";
    case EditorContentBrowserImportExternalSourceCopyStatus::copied:
        return "External import source copy copied";
    case EditorContentBrowserImportExternalSourceCopyStatus::blocked:
        return "External import source copy blocked";
    case EditorContentBrowserImportExternalSourceCopyStatus::failed:
        return "External import source copy failed";
    }
    return "External import source copy idle";
}

[[nodiscard]] bool is_drive_absolute_path(std::string_view path) noexcept {
    if (path.size() < 2 || path[1] != ':') {
        return false;
    }
    const char prefix = path[0];
    return (prefix >= 'A' && prefix <= 'Z') || (prefix >= 'a' && prefix <= 'z');
}

[[nodiscard]] bool is_absolute_like_path(std::string_view path) noexcept {
    return path.starts_with("/") || path.starts_with("\\") || is_drive_absolute_path(path);
}

[[nodiscard]] bool is_safe_project_relative_path(std::string_view path) noexcept {
    if (path.empty() || contains_line_separator(path) || path.find('=') != std::string_view::npos ||
        path.find(';') != std::string_view::npos || path.find('\\') != std::string_view::npos ||
        is_absolute_like_path(path)) {
        return false;
    }

    std::size_t segment_start = 0;
    while (segment_start <= path.size()) {
        const auto segment_end = path.find('/', segment_start);
        const auto segment = segment_end == std::string_view::npos
                                 ? path.substr(segment_start)
                                 : path.substr(segment_start, segment_end - segment_start);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_start = segment_end + 1;
    }
    return true;
}

[[nodiscard]] std::string
external_source_copy_row_status(const EditorContentBrowserImportExternalSourceCopyInput& input,
                                std::string& diagnostic) {
    if (input.copy_failed) {
        diagnostic = input.diagnostic.empty() ? "external import source copy failed" : input.diagnostic;
        return "Failed";
    }
    if (input.source_path.empty()) {
        diagnostic = "external import source copy source path is required";
        return "Blocked";
    }
    if (contains_line_separator(input.source_path) || input.source_path.find('=') != std::string_view::npos) {
        diagnostic = "external import source copy source path is invalid";
        return "Blocked";
    }
    if (!is_supported_import_source_path(input.source_path) ||
        !is_supported_import_source_path(input.target_project_path)) {
        diagnostic =
            "external import source copy paths must be first-party source document or supported codec source paths";
        return "Blocked";
    }
    if (!is_safe_project_relative_path(input.target_project_path)) {
        diagnostic = "external import source copy target must be a safe project-relative path";
        return "Blocked";
    }
    if (!input.source_exists) {
        diagnostic = "external import source copy source path does not exist";
        return "Blocked";
    }
    if (input.copied) {
        return "Copied";
    }
    if (input.target_exists) {
        diagnostic = "external import source copy target already exists";
        return "Blocked";
    }
    return "Ready to copy";
}

void append_external_source_copy_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                                      const std::vector<EditorContentBrowserImportExternalSourceCopyRow>& rows) {
    mirakana::ui::ElementDesc list =
        make_child("content_browser_import.external_copy.rows", root, mirakana::ui::SemanticRole::list);
    list.text = make_text("External Source Copy");
    add_or_throw(document, std::move(list));
    const mirakana::ui::ElementId list_id{"content_browser_import.external_copy.rows"};

    for (const auto& row : rows) {
        require_safe_field("external_copy.id", row.id);
        const auto prefix = "content_browser_import.external_copy.rows." + row.id;
        mirakana::ui::ElementDesc item = make_child(prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.status_label);
        item.enabled = row.can_copy || row.copied;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{prefix};
        append_label(document, item_id, prefix + ".status", row.status_label);
        append_label(document, item_id, prefix + ".source", row.source_path);
        append_label(document, item_id, prefix + ".target", row.target_project_path);
        if (!row.diagnostic.empty()) {
            append_label(document, item_id, prefix + ".diagnostic", row.diagnostic);
        }
    }
}

[[nodiscard]] std::string selected_filter_value(int selected_filter) {
    return selected_filter < 0 ? "-" : std::to_string(selected_filter);
}

void append_import_open_dialog_rows(EditorContentBrowserImportOpenDialogModel& model, std::size_t selected_count,
                                    int selected_filter) {
    model.rows = {
        EditorContentBrowserImportOpenDialogRow{.id = "status", .label = "Status", .value = model.status_label},
        EditorContentBrowserImportOpenDialogRow{
            .id = "selected_count", .label = "Selected count", .value = std::to_string(selected_count)},
        EditorContentBrowserImportOpenDialogRow{
            .id = "selected_filter", .label = "Selected filter", .value = selected_filter_value(selected_filter)},
    };

    std::size_t index = 1;
    for (const auto& path : model.selected_paths) {
        model.rows.push_back(EditorContentBrowserImportOpenDialogRow{
            .id = "path." + std::to_string(index),
            .label = "Path " + std::to_string(index),
            .value = sanitize_text(path),
        });
        ++index;
    }

    index = 1;
    for (const auto& diagnostic : model.diagnostics) {
        model.rows.push_back(EditorContentBrowserImportOpenDialogRow{
            .id = "diagnostic." + std::to_string(index),
            .label = "Diagnostic " + std::to_string(index),
            .value = sanitize_text(diagnostic),
        });
        ++index;
    }
}

[[nodiscard]] EditorContentBrowserImportPanelStatus determine_status(const ContentBrowserState& browser,
                                                                     const AssetPipelineState& pipeline) noexcept {
    if (pipeline.failed_count() > 0 || pipeline.failed_hot_reload_count() > 0) {
        return EditorContentBrowserImportPanelStatus::attention;
    }
    if (browser.item_count() == 0 && pipeline.item_count() == 0) {
        return EditorContentBrowserImportPanelStatus::empty;
    }
    return EditorContentBrowserImportPanelStatus::ready;
}

} // namespace

std::string_view
editor_content_browser_import_panel_status_label(EditorContentBrowserImportPanelStatus status) noexcept {
    switch (status) {
    case EditorContentBrowserImportPanelStatus::empty:
        return "empty";
    case EditorContentBrowserImportPanelStatus::ready:
        return "ready";
    case EditorContentBrowserImportPanelStatus::attention:
        return "attention";
    }
    return "empty";
}

EditorContentBrowserImportPanelModel
make_editor_content_browser_import_panel_model(const ContentBrowserState& browser, const AssetPipelineState& pipeline,
                                               const AssetImportPlan& import_plan,
                                               const std::vector<MaterialDefinition>& preview_materials) {
    EditorContentBrowserImportPanelModel model;
    model.status = determine_status(browser, pipeline);
    model.status_label = std::string(editor_content_browser_import_panel_status_label(model.status));
    model.text_filter = std::string(browser.text_filter());
    model.kind_filter_label = std::string(asset_kind_label(browser.kind_filter()));
    model.total_asset_count = browser.item_count();
    model.assets = make_asset_rows(browser);
    model.visible_asset_count = model.assets.size();
    if (const auto* selected = browser.selected_asset()) {
        model.has_selected_asset = true;
        model.selected_asset = EditorContentBrowserAssetRow{
            .asset = selected->id,
            .kind = selected->kind,
            .id = asset_id_string(selected->id),
            .kind_label = std::string(asset_kind_label(selected->kind)),
            .asset_key_label = selected->asset_key_label,
            .identity_source_path = selected->identity_source_path,
            .identity_status_label = selected->identity_backed ? "identity_v2" : "unannotated",
            .path = selected->path,
            .display_name = selected->display_name,
            .directory = selected->directory,
            .identity_backed = selected->identity_backed,
            .selected = true,
        };
    }
    model.import_queue = make_import_rows(pipeline);
    model.pipeline = make_editor_asset_pipeline_panel_model(pipeline, import_plan, preview_materials);
    model.hot_reload_summary_rows = make_hot_reload_summary_rows(pipeline);
    model.has_import_failures = pipeline.failed_count() > 0;
    model.has_hot_reload_failures = pipeline.failed_hot_reload_count() > 0;
    model.mutates = false;
    model.executes = false;
    return model;
}

mirakana::FileDialogRequest make_content_browser_import_open_dialog_request(std::string_view default_location) {
    mirakana::FileDialogRequest request;
    request.kind = mirakana::FileDialogKind::open_file;
    request.title = "Import Assets";
    request.filters = {
        mirakana::FileDialogFilter{.name = "Texture Source", .pattern = "texture"},
        mirakana::FileDialogFilter{.name = "Mesh Source", .pattern = "mesh"},
        mirakana::FileDialogFilter{.name = "Material Source", .pattern = "material"},
        mirakana::FileDialogFilter{.name = "Scene Source", .pattern = "scene"},
        mirakana::FileDialogFilter{.name = "Audio Source", .pattern = "audio_source"},
        mirakana::FileDialogFilter{.name = "PNG Texture Source", .pattern = "png"},
        mirakana::FileDialogFilter{.name = "glTF Mesh Source", .pattern = "gltf;glb"},
        mirakana::FileDialogFilter{.name = "Common Audio Source", .pattern = "wav;mp3;flac"},
    };
    request.default_location = std::string(default_location);
    request.allow_many = true;
    request.accept_label = "Import";
    request.cancel_label = "Cancel";
    return request;
}

EditorContentBrowserImportOpenDialogModel
make_content_browser_import_open_dialog_model(const mirakana::FileDialogResult& result) {
    EditorContentBrowserImportOpenDialogModel model;
    model.status_label = import_open_dialog_status("idle");

    if (auto diagnostic = mirakana::validate_file_dialog_result(result); !diagnostic.empty()) {
        model.status_label = import_open_dialog_status("blocked");
        model.diagnostics.push_back(std::move(diagnostic));
        append_import_open_dialog_rows(model, result.paths.size(), result.selected_filter);
        return model;
    }

    switch (result.status) {
    case mirakana::FileDialogStatus::canceled:
        model.status_label = import_open_dialog_status("canceled");
        break;
    case mirakana::FileDialogStatus::failed:
        model.status_label = import_open_dialog_status("failed");
        model.diagnostics.push_back(result.error);
        break;
    case mirakana::FileDialogStatus::accepted:
        for (const auto& path : result.paths) {
            if (!is_supported_import_source_path(path)) {
                model.status_label = import_open_dialog_status("blocked");
                model.diagnostics.emplace_back(
                    "asset import open dialog selections must be first-party source document "
                    "or supported codec source paths");
                append_import_open_dialog_rows(model, result.paths.size(), result.selected_filter);
                return model;
            }
        }

        model.selected_paths = result.paths;
        model.accepted = true;
        model.status_label = import_open_dialog_status("accepted");
        break;
    }

    append_import_open_dialog_rows(model, result.paths.size(), result.selected_filter);
    return model;
}

EditorContentBrowserImportExternalSourceCopyModel make_content_browser_import_external_source_copy_model(
    std::span<const EditorContentBrowserImportExternalSourceCopyInput> inputs) {
    EditorContentBrowserImportExternalSourceCopyModel model;
    model.copy_count = inputs.size();

    bool any_failed = false;
    bool all_copied = !inputs.empty();
    bool all_ready = !inputs.empty();

    std::size_t index = 1;
    for (const auto& input : inputs) {
        std::string diagnostic;
        const auto status_label = external_source_copy_row_status(input, diagnostic);

        EditorContentBrowserImportExternalSourceCopyRow row;
        row.id = std::to_string(index);
        row.source_path = sanitize_text(input.source_path);
        row.target_project_path = sanitize_text(input.target_project_path);
        row.status_label = status_label;
        row.diagnostic = sanitize_text(diagnostic);
        row.can_copy = status_label == "Ready to copy";
        row.copied = status_label == "Copied";
        row.blocked = status_label == "Blocked";
        row.failed = status_label == "Failed";

        if (!diagnostic.empty()) {
            model.diagnostics.push_back(diagnostic);
        }
        if (!row.blocked && !row.failed) {
            model.target_project_paths.push_back(input.target_project_path);
        }

        any_failed = any_failed || row.failed;
        all_copied = all_copied && row.copied;
        all_ready = all_ready && row.can_copy;

        model.rows.push_back(std::move(row));
        ++index;
    }

    if (inputs.empty()) {
        model.status = EditorContentBrowserImportExternalSourceCopyStatus::idle;
    } else if (any_failed) {
        model.status = EditorContentBrowserImportExternalSourceCopyStatus::failed;
    } else if (all_copied) {
        model.status = EditorContentBrowserImportExternalSourceCopyStatus::copied;
    } else if (all_ready) {
        model.status = EditorContentBrowserImportExternalSourceCopyStatus::ready;
    } else {
        model.status = EditorContentBrowserImportExternalSourceCopyStatus::blocked;
    }

    model.status_label = external_source_copy_status_label(model.status);
    model.can_copy = model.status == EditorContentBrowserImportExternalSourceCopyStatus::ready;
    model.copied = model.status == EditorContentBrowserImportExternalSourceCopyStatus::copied;
    model.blocked = model.status == EditorContentBrowserImportExternalSourceCopyStatus::blocked;
    return model;
}

EditorContentBrowserImportExternalSourceCopyModel make_content_browser_import_external_source_copy_model(
    std::initializer_list<EditorContentBrowserImportExternalSourceCopyInput> inputs) {
    return make_content_browser_import_external_source_copy_model(
        std::span<const EditorContentBrowserImportExternalSourceCopyInput>{inputs.begin(), inputs.size()});
}

mirakana::ui::UiDocument make_content_browser_import_panel_ui_model(const EditorContentBrowserImportPanelModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("content_browser_import", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"content_browser_import"};

    append_label(document, root, "content_browser_import.status", model.status_label);
    append_label(document, root, "content_browser_import.filter", model.text_filter.empty() ? "-" : model.text_filter);
    append_label(document, root, "content_browser_import.kind", model.kind_filter_label);
    append_label(document, root, "content_browser_import.total_assets", std::to_string(model.total_asset_count));
    append_label(document, root, "content_browser_import.visible_assets", std::to_string(model.visible_asset_count));
    append_label(document, root, "content_browser_import.import_total",
                 std::to_string(model.pipeline.progress.total_count));
    append_label(document, root, "content_browser_import.import_failed",
                 std::to_string(model.pipeline.progress.failed_count));

    append_asset_rows(document, root, model.assets);
    if (model.has_selected_asset) {
        mirakana::ui::ElementDesc selection =
            make_child("content_browser_import.selection", root, mirakana::ui::SemanticRole::panel);
        selection.text = make_text("Selection");
        add_or_throw(document, std::move(selection));
        const mirakana::ui::ElementId selection_id{"content_browser_import.selection"};
        append_label(document, selection_id, "content_browser_import.selection.path", model.selected_asset.path);
        append_label(document, selection_id, "content_browser_import.selection.kind", model.selected_asset.kind_label);
        append_label(document, selection_id, "content_browser_import.selection.asset_key",
                     model.selected_asset.asset_key_label.empty() ? "-" : model.selected_asset.asset_key_label);
        append_label(document, selection_id, "content_browser_import.selection.identity_source_path",
                     model.selected_asset.identity_source_path.empty() ? "-"
                                                                       : model.selected_asset.identity_source_path);
        append_label(document, selection_id, "content_browser_import.selection.identity_status",
                     model.selected_asset.identity_status_label);
    }
    append_import_rows(document, root, model.import_queue);
    append_diagnostic_rows(document, root, model.pipeline.diagnostics);
    append_dependency_rows(document, root, model.pipeline.dependencies);
    append_thumbnail_rows(document, root, model.pipeline.thumbnail_requests);
    append_material_rows(document, root, model.pipeline.material_previews);
    append_hot_reload_rows(document, root, model.hot_reload_summary_rows);

    return document;
}

mirakana::ui::UiDocument
make_content_browser_import_open_dialog_ui_model(const EditorContentBrowserImportOpenDialogModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("content_browser_import.open_dialog", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"content_browser_import.open_dialog"};

    append_label(document, root, "content_browser_import.open_dialog.status", model.status_label);
    append_label(document, root, "content_browser_import.open_dialog.selected_count",
                 std::to_string(model.selected_paths.size()));

    std::size_t path_index = 1;
    for (const auto& path : model.selected_paths) {
        append_label(document, root, "content_browser_import.open_dialog.paths." + std::to_string(path_index), path);
        ++path_index;
    }

    std::size_t diagnostic_index = 1;
    for (const auto& diagnostic : model.diagnostics) {
        append_label(document, root,
                     "content_browser_import.open_dialog.diagnostics." + std::to_string(diagnostic_index), diagnostic);
        ++diagnostic_index;
    }

    return document;
}

mirakana::ui::UiDocument make_content_browser_import_external_source_copy_ui_model(
    const EditorContentBrowserImportExternalSourceCopyModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("content_browser_import.external_copy", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"content_browser_import.external_copy"};

    append_label(document, root, "content_browser_import.external_copy.status", model.status_label);
    append_label(document, root, "content_browser_import.external_copy.copy_count", std::to_string(model.copy_count));

    append_external_source_copy_rows(document, root, model.rows);

    std::size_t diagnostic_index = 1;
    for (const auto& diagnostic : model.diagnostics) {
        append_label(document, root,
                     "content_browser_import.external_copy.diagnostics." + std::to_string(diagnostic_index),
                     diagnostic);
        ++diagnostic_index;
    }

    return document;
}

} // namespace mirakana::editor
