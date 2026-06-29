// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

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
    std::string status_label;
    std::string diagnostic;
    bool can_register{false};
    bool can_import{false};
    bool blocked_by_legal{false};
};

struct EditorAssetImportReviewRequest {
    std::string asset_root{"assets"};
    std::string imported_output_root{"assets/imported"};
    std::string source_registry_path{"source/assets/package.geassets"};
    std::string source_registry_content;
    std::vector<EditorAssetImportCandidateInput> sources;
};

struct EditorAssetImportReviewModel {
    std::vector<EditorAssetImportCandidateRow> rows;
    std::vector<SourceAssetRegistrationRequest> registration_requests;
    AssetImportPlan import_plan;
    std::vector<std::string> diagnostics;
    bool ready{false};
};

[[nodiscard]] AssetImportActionKind editor_asset_import_action_kind_for_asset_kind(AssetKind kind) noexcept;
[[nodiscard]] AssetKind editor_asset_import_asset_kind_for_source_path(std::string_view path) noexcept;
[[nodiscard]] std::string editor_asset_import_source_format_for_path(std::string_view path);
[[nodiscard]] std::string editor_asset_import_output_path_for_source_path(std::string_view output_root,
                                                                          std::string_view source_path, AssetKind kind);
[[nodiscard]] EditorAssetImportReviewModel
review_editor_asset_import_candidates(const EditorAssetImportReviewRequest& request);

} // namespace mirakana::editor
