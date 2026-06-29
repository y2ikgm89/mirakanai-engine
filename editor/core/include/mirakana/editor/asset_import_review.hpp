// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_presets.hpp"
#include "mirakana/editor/asset_browser_production.hpp"
#include "mirakana/tools/source_asset_registration_tool.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorAssetImportCandidateStatus : std::uint8_t {
    ready,
    blocked,
};

struct EditorAssetImportCandidateInput {
    std::string source_path;
    EditorAssetBrowserLegalProvenanceRow provenance;
    bool source_exists{false};
};

struct EditorAssetImportCandidateRow {
    std::string id;
    AssetKeyV2 asset_key;
    AssetId asset;
    AssetKind asset_kind{AssetKind::unknown};
    AssetImportActionKind action_kind{AssetImportActionKind::unknown};
    std::string source_path;
    std::string source_format;
    std::string imported_path;
    std::vector<std::string> preset_metadata;
    std::string status_label;
    std::string diagnostic;
    bool can_register{false};
    bool can_import{false};
    bool blocked_by_legal{false};
    bool blocked_by_preset{false};
};

struct EditorAssetImportReviewRequest {
    std::string asset_root{"assets"};
    std::string imported_output_root{"assets/imported"};
    std::string source_registry_path{"source/assets/package.geassets"};
    std::string source_registry_content;
    AssetImportPresetsDocumentV1 import_presets;
    std::vector<EditorAssetImportCandidateInput> sources;
};

struct EditorAssetImportReviewModel {
    std::vector<EditorAssetImportCandidateRow> rows;
    std::vector<SourceAssetRegistrationRequest> registration_requests;
    AssetImportPlan import_plan;
    std::vector<std::string> diagnostics;
    bool ready{false};
};

struct EditorAssetRecookContentHashRow {
    AssetKeyV2 asset_key;
    std::string source_content_hash;
    std::string output_content_hash;
};

struct EditorAssetImportReviewedActionRow {
    std::string id;
    AssetKeyV2 asset_key;
    AssetId asset;
    AssetImportActionKind action_kind{AssetImportActionKind::unknown};
    std::string source_path;
    std::string output_path;
    std::string source_content_hash;
    std::string output_content_hash;
    std::string status_label;
    std::string diagnostic;
    bool selected{false};
    bool dependency_expanded{false};
    bool stale{false};
    bool can_reimport{false};
    bool can_recook{false};
};

struct EditorAssetImportReviewedActionPlan {
    std::string command_id;
    std::vector<EditorAssetImportReviewedActionRow> rows;
    AssetImportPlan import_plan;
    std::vector<AssetHotReloadRecookRequest> recook_requests;
    std::vector<AssetHotReloadApplyResult> staged_results;
    std::vector<AssetId> safe_point_assets;
    std::vector<std::string> diagnostics;
    bool ready{false};
    bool requires_user_confirmation{true};
    bool mutates_project_files{true};
    bool executes_import_tools{false};
    bool runtime_stage_pending{false};
    bool commits_runtime_replacements{false};
    bool parses_runtime_sources{false};
};

struct EditorAssetReimportReviewRequest {
    SourceAssetRegistryDocumentV1 source_registry;
    AssetImportPlan import_plan;
    std::vector<AssetKeyV2> selected_asset_keys;
    bool include_dependency_closure{false};
    std::uint64_t previous_revision{1};
    std::uint64_t current_revision{2};
    std::uint64_t ready_tick{0};
};

struct EditorAssetRecookReviewRequest {
    AssetImportPlan import_plan;
    std::vector<AssetHotReloadRecookRequest> ready_recook_requests;
    std::vector<EditorAssetRecookContentHashRow> content_hash_rows;
};

struct EditorAssetHotReloadStageReviewRequest {
    std::vector<AssetHotReloadApplyResult> staged_results;
    std::vector<AssetKeyV2> selected_asset_keys;
    bool commit_at_safe_point{false};
};

[[nodiscard]] AssetImportActionKind editor_asset_import_action_kind_for_asset_kind(AssetKind kind) noexcept;
[[nodiscard]] AssetKind editor_asset_import_asset_kind_for_source_path(std::string_view path) noexcept;
[[nodiscard]] std::string editor_asset_import_source_format_for_path(std::string_view path);
[[nodiscard]] std::string editor_asset_import_output_path_for_source_path(std::string_view output_root,
                                                                          std::string_view source_path, AssetKind kind);
[[nodiscard]] EditorAssetImportReviewModel
review_editor_asset_import_candidates(const EditorAssetImportReviewRequest& request);
[[nodiscard]] EditorAssetImportReviewedActionPlan
review_editor_asset_reimport_request(const EditorAssetReimportReviewRequest& request);
[[nodiscard]] EditorAssetImportReviewedActionPlan
review_editor_asset_recook_request(const EditorAssetRecookReviewRequest& request);
[[nodiscard]] EditorAssetImportReviewedActionPlan
review_editor_asset_hot_reload_stage_request(const EditorAssetHotReloadStageReviewRequest& request);

} // namespace mirakana::editor
