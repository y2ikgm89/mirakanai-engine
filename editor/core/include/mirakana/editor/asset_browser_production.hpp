// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/editor/content_browser.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorAssetBrowserProductionStatus : std::uint8_t { empty, ready, attention };

struct EditorAssetBrowserSourcePulseRow {
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string row_id;
    std::string kind_label;
    std::string asset_key_label;
    std::string source_path;
    std::string imported_path;
    std::string display_name;
    std::string scope_label;
    std::string state_label;
    std::string import_status_label;
    std::string package_status_label;
    std::string provenance_status_label;
    std::string preview_status_label;
    std::string hot_reload_status_label;
    bool selected{false};
    bool identity_backed{false};
    bool source_visible{false};
    bool package_visible{false};
    bool blocked{false};
    bool host_gated{false};
};

struct EditorAssetBrowserProductionDesc {
    const ContentBrowserState* browser{nullptr};
    const AssetImportPlan* import_plan{nullptr};
    std::string project_root{"."};
    std::string asset_root{"assets"};
    std::string source_registry_path;
};

struct EditorAssetBrowserProductionModel {
    EditorAssetBrowserProductionStatus status{EditorAssetBrowserProductionStatus::empty};
    std::string status_label{"Asset browser empty"};
    std::string project_root;
    std::string asset_root;
    std::string source_registry_path;
    std::size_t total_row_count{0};
    std::size_t visible_row_count{0};
    std::vector<EditorAssetBrowserSourcePulseRow> rows;
    std::vector<std::string> diagnostics;
    bool mutates{false};
    bool executes{false};
    bool exposes_native_handles{false};
};

[[nodiscard]] std::string_view
editor_asset_browser_production_status_label(EditorAssetBrowserProductionStatus status) noexcept;
[[nodiscard]] EditorAssetBrowserProductionModel
make_editor_asset_browser_production_model(const EditorAssetBrowserProductionDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_asset_browser_production_ui_model(const EditorAssetBrowserProductionModel& model);

} // namespace mirakana::editor
