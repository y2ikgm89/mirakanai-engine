// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/asset_pipeline.hpp"
#include "mirakana/editor/content_browser.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorContentBrowserImportPanelStatus : std::uint8_t { empty, ready, attention };

struct EditorContentBrowserAssetRow {
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string id;
    std::string kind_label;
    std::string asset_key_label;
    std::string identity_source_path;
    std::string identity_status_label;
    std::string path;
    std::string display_name;
    std::string directory;
    bool identity_backed{false};
    bool selected{false};
};

struct EditorContentBrowserImportQueueRow {
    AssetId asset;
    AssetImportActionKind kind{AssetImportActionKind::unknown};
    std::string id;
    std::string kind_label;
    std::string status_label;
    std::string source_path;
    std::string output_path;
    std::string diagnostic;
    bool failed{false};
};

struct EditorContentBrowserHotReloadSummaryRow {
    std::string id;
    std::string label;
    std::size_t count{0};
    bool attention{false};
};

struct EditorContentBrowserImportOpenDialogRow {
    std::string id;
    std::string label;
    std::string value;
};

struct EditorContentBrowserImportOpenDialogModel {
    std::string status_label{"Asset import open dialog idle"};
    std::vector<std::string> selected_paths;
    std::vector<std::string> diagnostics;
    std::vector<EditorContentBrowserImportOpenDialogRow> rows;
    bool accepted{false};
};

enum class EditorContentBrowserImportExternalSourceCopyStatus : std::uint8_t { idle, ready, copied, blocked, failed };

struct EditorContentBrowserImportExternalSourceCopyInput {
    std::string source_path;
    std::string target_project_path;
    std::string diagnostic;
    bool source_exists{false};
    bool target_exists{false};
    bool copied{false};
    bool copy_failed{false};
};

struct EditorContentBrowserImportExternalSourceCopyRow {
    std::string id;
    std::string source_path;
    std::string target_project_path;
    std::string status_label;
    std::string diagnostic;
    bool can_copy{false};
    bool copied{false};
    bool blocked{false};
    bool failed{false};
};

struct EditorContentBrowserImportExternalSourceCopyModel {
    EditorContentBrowserImportExternalSourceCopyStatus status{EditorContentBrowserImportExternalSourceCopyStatus::idle};
    std::string status_label{"External import source copy idle"};
    std::vector<EditorContentBrowserImportExternalSourceCopyRow> rows;
    std::vector<std::string> target_project_paths;
    std::vector<std::string> diagnostics;
    std::size_t copy_count{0};
    bool can_copy{false};
    bool copied{false};
    bool blocked{false};
};

struct EditorContentBrowserImportPanelModel {
    EditorContentBrowserImportPanelStatus status{EditorContentBrowserImportPanelStatus::empty};
    std::string status_label;
    std::string text_filter;
    std::string kind_filter_label;
    std::size_t total_asset_count{0};
    std::size_t visible_asset_count{0};
    bool has_selected_asset{false};
    EditorContentBrowserAssetRow selected_asset;
    std::vector<EditorContentBrowserAssetRow> assets;
    std::vector<EditorContentBrowserImportQueueRow> import_queue;
    EditorAssetPipelinePanelModel pipeline;
    std::vector<EditorContentBrowserHotReloadSummaryRow> hot_reload_summary_rows;
    bool has_import_failures{false};
    bool has_hot_reload_failures{false};
    bool mutates{false};
    bool executes{false};
};

[[nodiscard]] std::string_view
editor_content_browser_import_panel_status_label(EditorContentBrowserImportPanelStatus status) noexcept;
[[nodiscard]] EditorContentBrowserImportPanelModel
make_editor_content_browser_import_panel_model(const ContentBrowserState& browser, const AssetPipelineState& pipeline,
                                               const AssetImportPlan& import_plan,
                                               const std::vector<MaterialDefinition>& preview_materials = {});
[[nodiscard]] mirakana::FileDialogRequest
make_content_browser_import_open_dialog_request(std::string_view default_location = "assets");
[[nodiscard]] EditorContentBrowserImportOpenDialogModel
make_content_browser_import_open_dialog_model(const mirakana::FileDialogResult& result);
[[nodiscard]] EditorContentBrowserImportExternalSourceCopyModel make_content_browser_import_external_source_copy_model(
    std::span<const EditorContentBrowserImportExternalSourceCopyInput> inputs);
[[nodiscard]] EditorContentBrowserImportExternalSourceCopyModel make_content_browser_import_external_source_copy_model(
    std::initializer_list<EditorContentBrowserImportExternalSourceCopyInput> inputs);
[[nodiscard]] mirakana::ui::UiDocument
make_content_browser_import_panel_ui_model(const EditorContentBrowserImportPanelModel& model);
[[nodiscard]] mirakana::ui::UiDocument
make_content_browser_import_open_dialog_ui_model(const EditorContentBrowserImportOpenDialogModel& model);
[[nodiscard]] mirakana::ui::UiDocument make_content_browser_import_external_source_copy_ui_model(
    const EditorContentBrowserImportExternalSourceCopyModel& model);

} // namespace mirakana::editor
