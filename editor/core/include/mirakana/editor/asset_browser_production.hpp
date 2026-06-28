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
enum class EditorAssetBrowserQueryStatus : std::uint8_t { empty, ready, blocked };
enum class EditorAssetBrowserCommandKind : std::uint8_t {
    reload_source_registry,
    review_import_sources,
    copy_external_sources,
    execute_reviewed_import_plan,
    preview_cooked_package,
    stage_hot_reload_recook,
    inspect_selection,
    apply_package_registration,
};
enum class EditorAssetBrowserCommandMode : std::uint8_t { dry_run, apply };
enum class EditorAssetBrowserCommandStatus : std::uint8_t { ready, blocked, rejected_stale_generation };

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
    std::uint64_t generation{1};
};

struct EditorAssetBrowserProductionModel {
    EditorAssetBrowserProductionStatus status{EditorAssetBrowserProductionStatus::empty};
    std::string status_label{"Asset browser empty"};
    std::string project_root;
    std::string asset_root;
    std::string source_registry_path;
    std::uint64_t generation{1};
    std::size_t total_row_count{0};
    std::size_t visible_row_count{0};
    std::vector<EditorAssetBrowserSourcePulseRow> rows;
    std::vector<std::string> diagnostics;
    bool mutates{false};
    bool executes{false};
    bool exposes_native_handles{false};
};

struct EditorAssetBrowserQueryTokenRow {
    std::string id;
    std::string key;
    std::string value;
    std::string status_label;
    bool active{false};
    bool blocked{false};
};

struct EditorAssetBrowserQueryDesc {
    std::string query_text;
    std::vector<EditorAssetBrowserSourcePulseRow> rows;
};

struct EditorAssetBrowserQueryResult {
    EditorAssetBrowserQueryStatus status{EditorAssetBrowserQueryStatus::empty};
    std::string status_label{"Asset browser query empty"};
    std::string normalized_query;
    std::vector<EditorAssetBrowserQueryTokenRow> tokens;
    std::vector<EditorAssetBrowserSourcePulseRow> rows;
    std::vector<std::string> diagnostics;
};

struct EditorAssetBrowserCommandRequest {
    EditorAssetBrowserCommandKind kind{EditorAssetBrowserCommandKind::reload_source_registry};
    EditorAssetBrowserCommandMode mode{EditorAssetBrowserCommandMode::dry_run};
    std::uint64_t expected_generation{0};
    std::uint64_t current_generation{0};
    bool user_confirmed{false};
};

struct EditorAssetBrowserCommandPlan {
    std::string command_id;
    std::string label;
    EditorAssetBrowserCommandStatus status{EditorAssetBrowserCommandStatus::blocked};
    std::string status_label;
    std::uint64_t expected_generation{0};
    std::uint64_t current_generation{0};
    bool requires_user_confirmation{false};
    bool mutates_project_files{false};
    bool executes_import_tools{false};
    bool executes_package_scripts{false};
    bool executes_validation_recipes{false};
    bool exposes_native_handles{false};
    std::vector<std::string> report_rows;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view
editor_asset_browser_production_status_label(EditorAssetBrowserProductionStatus status) noexcept;
[[nodiscard]] std::string_view editor_asset_browser_command_id(EditorAssetBrowserCommandKind kind) noexcept;
[[nodiscard]] EditorAssetBrowserProductionModel
make_editor_asset_browser_production_model(const EditorAssetBrowserProductionDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_asset_browser_production_ui_model(const EditorAssetBrowserProductionModel& model);
[[nodiscard]] EditorAssetBrowserQueryResult plan_editor_asset_browser_query(const EditorAssetBrowserQueryDesc& desc);
[[nodiscard]] EditorAssetBrowserCommandPlan
plan_editor_asset_browser_command(const EditorAssetBrowserCommandRequest& request);

} // namespace mirakana::editor
