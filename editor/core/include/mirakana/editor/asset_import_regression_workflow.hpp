// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_batch_reimport.hpp"
#include "mirakana/assets/asset_import_preset_diff.hpp"
#include "mirakana/assets/asset_import_regression_corpus.hpp"
#include "mirakana/editor/asset_browser_production.hpp"
#include "mirakana/tools/asset_axis_unit_preview.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorAssetImportRegressionWorkflowStatus : std::uint8_t { empty, ready, blocked };

enum class EditorAssetImportRegressionWorkflowCommandKind : std::uint8_t {
    run_corpus,
    open_report,
    batch_reimport,
    preset_diff,
    axis_unit_preview,
};

struct EditorAssetImportRegressionWorkflowDesc {
    const AssetImportRegressionCorpusDocumentV1* corpus{nullptr};
    const AssetImportRegressionReportV1* latest_report{nullptr};
    const AssetImportBatchReimportPlan* batch_reimport{nullptr};
    const AssetImportPresetDiff* preset_diff{nullptr};
    std::vector<AssetAxisUnitPreview> axis_unit_previews;
    std::uint64_t generation{1U};
};

struct EditorAssetImportRegressionWorkflowModel {
    EditorAssetImportRegressionWorkflowStatus status{EditorAssetImportRegressionWorkflowStatus::empty};
    std::string status_label{"Asset import regression workflow empty"};
    std::uint64_t generation{1U};
    std::vector<EditorAssetBrowserRetainedCommandRow> command_rows;
    std::vector<EditorAssetBrowserImportWorkflowRow> rows;
    std::vector<std::string> diagnostics;
    std::uint64_t ready_row_count{0U};
    std::uint64_t blocked_row_count{0U};
    bool mutates_project_files{false};
    bool executes_import_tools{false};
    bool executes_package_scripts{false};
    bool executes_validation_recipes{false};
    bool exposes_native_handles{false};
};

[[nodiscard]] std::string_view
editor_asset_import_regression_workflow_status_label(EditorAssetImportRegressionWorkflowStatus status) noexcept;
[[nodiscard]] std::string_view
editor_asset_import_regression_workflow_command_id(EditorAssetImportRegressionWorkflowCommandKind kind) noexcept;
[[nodiscard]] EditorAssetImportRegressionWorkflowModel
make_editor_asset_import_regression_workflow_model(const EditorAssetImportRegressionWorkflowDesc& desc);
[[nodiscard]] EditorAssetBrowserRetainedUiDesc
make_editor_asset_import_regression_workflow_retained_ui_desc(const EditorAssetImportRegressionWorkflowModel& model);

} // namespace mirakana::editor
