// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_presets.hpp"
#include "mirakana/assets/asset_import_regression_corpus.hpp"
#include "mirakana/tools/asset_import_tool.hpp"

#include <cstdint>
#include <string>

namespace mirakana {

struct AssetImportRegressionRunnerOptions {
    std::string corpus_root;
    std::string output_root;
    AssetImportPresetsDocumentV1 project_presets;
    AssetImportExecutionOptions import_execution_options;
    bool require_large_corpus{false};
    bool write_cooked_outputs{false};
    bool compare_expected_hashes{true};
    bool collect_preview_rows{true};
    std::uint64_t row_budget{10000U};
};

[[nodiscard]] AssetImportRegressionReportV1
run_asset_import_regression_corpus(IFileSystem& filesystem, const AssetImportRegressionCorpusDocumentV1& corpus,
                                   const AssetImportRegressionRunnerOptions& options);

} // namespace mirakana
