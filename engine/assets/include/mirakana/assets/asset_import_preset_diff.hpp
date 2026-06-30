// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_import_regression_corpus.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mirakana {

enum class AssetImportPresetDiffImpact : std::uint8_t { source_document, cooked_output, package_output };
enum class AssetImportPresetDiffRowStatus : std::uint8_t { unchanged, changed, review_blocked };

struct AssetImportPresetDiffAssetV1 {
    std::string asset_id;
    AssetKeyV2 asset_key;
    AssetId asset;
    AssetImportRegressionCorpusAssetKind corpus_kind{AssetImportRegressionCorpusAssetKind::gltf_mesh};
};

struct AssetImportPresetDiffFieldChange {
    std::string field;
    std::string before_value;
    std::string after_value;
    std::vector<AssetImportPresetDiffImpact> impacts;
};

struct AssetImportPresetDiffRow {
    std::string asset_id;
    AssetKeyV2 asset_key;
    AssetId asset;
    AssetImportRegressionCorpusAssetKind corpus_kind{AssetImportRegressionCorpusAssetKind::gltf_mesh};
    AssetImportActionKind action_kind{AssetImportActionKind::unknown};
    std::string source_path;
    std::string output_path;
    AssetImportPresetDiffRowStatus status{AssetImportPresetDiffRowStatus::unchanged};
    std::vector<AssetImportPresetDiffFieldChange> field_changes;
    std::vector<std::string> diagnostics;
    bool source_review_required{false};
    bool cooked_output_changes{false};
    bool package_output_changes{false};
    bool review_blocked{false};
};

struct AssetImportPresetDiffDesc {
    AssetImportPlan import_plan;
    std::vector<AssetImportPresetDiffAssetV1> assets;
    AssetImportPresetsDocumentV1 before;
    AssetImportPresetsDocumentV1 after;
    std::optional<AssetImportRegressionReportV1> latest_report;
    bool include_unchanged_rows{false};
};

struct AssetImportPresetDiff {
    std::vector<AssetImportPresetDiffRow> rows;
    std::vector<std::string> diagnostics;
    std::uint64_t affected_count{0U};
    std::uint64_t source_review_count{0U};
    std::uint64_t cooked_output_change_count{0U};
    std::uint64_t package_output_change_count{0U};
    std::uint64_t review_blocked_count{0U};
    bool executes_importers{false};

    [[nodiscard]] bool ready() const noexcept {
        return diagnostics.empty() && review_blocked_count == 0U;
    }
};

[[nodiscard]] AssetImportPresetDiff diff_asset_import_presets(const AssetImportPresetDiffDesc& desc);

} // namespace mirakana
