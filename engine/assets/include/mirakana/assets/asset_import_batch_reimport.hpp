// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_import_regression_corpus.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class AssetImportBatchReimportRowStatus : std::uint8_t { skipped, ready, blocked };

struct AssetImportBatchReimportDesc {
    std::string run_id;
    std::string staging_root;
    AssetImportPlan import_plan;
    AssetImportRegressionReportV1 report;
    std::vector<std::string> selected_asset_ids;
    bool apply_requested{false};
    bool dry_run_acknowledged{false};
    std::uint64_t max_selected_assets{10000U};
};

struct AssetImportBatchReimportPlanRow {
    std::string asset_id;
    AssetId asset;
    AssetImportActionKind kind{AssetImportActionKind::unknown};
    std::string source_path;
    std::string output_path;
    std::string staging_path;
    std::string source_sha256;
    std::string preset_sha256;
    std::string importer_id;
    std::string expected_output_hash;
    AssetImportBatchReimportRowStatus status{AssetImportBatchReimportRowStatus::skipped};
    std::string diagnostic;
    bool selected{false};
    bool legal_blocked{false};
    bool output_validation_required{false};
};

struct AssetImportBatchReimportPlan {
    std::vector<AssetImportBatchReimportPlanRow> rows;
    std::vector<std::string> diagnostics;
    std::uint64_t selected_count{0U};
    std::uint64_t ready_row_count{0U};
    std::uint64_t blocked_row_count{0U};
    bool dry_run_required{false};
    bool apply_requested{false};
    bool dry_run_acknowledged{false};
    bool apply_allowed{false};
    bool ready_for_apply{false};
    bool all_or_nothing{true};
    bool mutates_project_outputs{false};

    [[nodiscard]] bool ready() const noexcept {
        return diagnostics.empty() && selected_count > 0U && blocked_row_count == 0U &&
               ready_row_count == selected_count;
    }
};

[[nodiscard]] AssetImportBatchReimportPlan plan_asset_import_batch_reimport(const AssetImportBatchReimportDesc& desc);

} // namespace mirakana
