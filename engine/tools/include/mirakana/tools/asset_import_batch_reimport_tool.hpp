// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_batch_reimport.hpp"
#include "mirakana/tools/asset_import_tool.hpp"

#include <string>
#include <vector>

namespace mirakana {

struct AssetImportBatchReimportExecutionResult {
    AssetImportBatchReimportPlan plan;
    AssetImportExecutionResult staged_import;
    std::vector<AssetImportedArtifact> staged_artifacts;
    std::vector<AssetImportedArtifact> applied_artifacts;
    std::vector<std::string> diagnostics;
    bool staged{false};
    bool applied{false};
    bool project_outputs_mutated{false};

    [[nodiscard]] bool succeeded() const noexcept {
        if (!diagnostics.empty()) {
            return false;
        }
        if (plan.apply_requested) {
            return staged && applied && project_outputs_mutated;
        }
        return plan.ready();
    }
};

[[nodiscard]] AssetImportBatchReimportExecutionResult
execute_asset_import_batch_reimport(IFileSystem& filesystem, const AssetImportBatchReimportDesc& desc);

[[nodiscard]] AssetImportBatchReimportExecutionResult
execute_asset_import_batch_reimport(IFileSystem& filesystem, const AssetImportBatchReimportDesc& desc,
                                    const AssetImportExecutionOptions& options);

} // namespace mirakana
