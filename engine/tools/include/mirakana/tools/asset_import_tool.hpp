// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <string>
#include <vector>

namespace mirakana {

struct AssetImportedArtifact {
    AssetId asset;
    AssetImportActionKind kind{AssetImportActionKind::unknown};
    std::string output_path;
};

struct AssetImportFailure {
    AssetId asset;
    AssetImportActionKind kind{AssetImportActionKind::unknown};
    std::string source_path;
    std::string output_path;
    std::string diagnostic;
};

struct AssetImportExecutionResult {
    std::vector<AssetImportedArtifact> imported;
    std::vector<AssetImportFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

class IExternalAssetImporter {
  public:
    virtual ~IExternalAssetImporter() = default;

    [[nodiscard]] virtual bool supports(const AssetImportAction& action) const noexcept = 0;
    [[nodiscard]] virtual std::string import_source_document(IFileSystem& filesystem,
                                                             const AssetImportAction& action) = 0;
};

struct AssetImportExecutionOptions {
    std::vector<IExternalAssetImporter*> external_importers;
};

struct AssetRuntimeRecookExecutionResult {
    AssetImportPlan recook_plan;
    AssetImportExecutionResult import_result;
    std::vector<AssetHotReloadApplyResult> apply_results;

    [[nodiscard]] bool succeeded() const noexcept {
        return import_result.succeeded();
    }
};

[[nodiscard]] AssetImportExecutionResult execute_asset_import_plan(IFileSystem& filesystem,
                                                                   const AssetImportPlan& plan);
[[nodiscard]] AssetImportExecutionResult execute_asset_import_plan(IFileSystem& filesystem, const AssetImportPlan& plan,
                                                                   const AssetImportExecutionOptions& options);
[[nodiscard]] AssetRuntimeRecookExecutionResult
execute_asset_runtime_recook(IFileSystem& filesystem, const AssetImportPlan& import_plan,
                             AssetRuntimeReplacementState& replacements,
                             const std::vector<AssetHotReloadRecookRequest>& requests);

} // namespace mirakana
