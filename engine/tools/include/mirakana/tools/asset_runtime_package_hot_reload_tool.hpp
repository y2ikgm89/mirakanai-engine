// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/tools/asset_import_tool.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class AssetRuntimePackageHotReloadReplacementStatus : std::uint8_t {
    recook_failed,
    runtime_replacement_failed,
    committed,
};

enum class AssetRuntimePackageHotReloadReplacementDiagnosticPhase : std::uint8_t {
    recook_execution,
    runtime_replacement,
};

struct AssetRuntimePackageHotReloadRuntimeReplacementDesc {
    std::vector<runtime::RuntimePackageIndexDiscoveryCandidateV2> candidates;
    runtime::RuntimePackageIndexDiscoveryDescV2 discovery;
    std::string selected_package_index_path;
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    std::vector<runtime::RuntimeResidentPackageMountIdV2> reviewed_existing_mount_ids;
    runtime::RuntimePackageMountOverlay overlay{runtime::RuntimePackageMountOverlay::last_mount_wins};
    runtime::RuntimeResourceResidencyBudgetV2 budget;
    std::vector<runtime::RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order;
    std::vector<runtime::RuntimeResidentPackageMountIdV2> protected_mount_ids;
};

struct AssetRuntimePackageHotReloadReplacementDesc {
    AssetImportPlan import_plan;
    AssetImportExecutionOptions import_options;
    std::vector<AssetHotReloadRecookRequest> recook_requests;
    AssetRuntimePackageHotReloadRuntimeReplacementDesc runtime_replacement;
};

struct AssetRuntimePackageHotReloadReplacementDiagnostic {
    AssetRuntimePackageHotReloadReplacementDiagnosticPhase phase{
        AssetRuntimePackageHotReloadReplacementDiagnosticPhase::recook_execution};
    AssetId asset;
    runtime::RuntimeResidentPackageMountIdV2 mount;
    std::string path;
    std::string code;
    std::string message;
};

struct AssetRuntimePackageHotReloadReplacementResult {
    AssetRuntimePackageHotReloadReplacementStatus status{AssetRuntimePackageHotReloadReplacementStatus::recook_failed};
    AssetRuntimeRecookExecutionResult recook;
    runtime::RuntimePackageHotReloadRecookReplacementResultV2 runtime_replacement;
    std::vector<AssetHotReloadApplyResult> committed_apply_results;
    std::vector<AssetRuntimePackageHotReloadReplacementDiagnostic> diagnostics;
    bool invoked_file_watch{false};
    bool invoked_recook{false};
    bool invoked_runtime_replacement{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Runs asset recook execution, feeds the helper-owned staged recook rows into the reviewed runtime package replacement
/// safe point, and commits only the staged assets that matched the selected runtime package after the runtime resident
/// package commit succeeds. This helper does not watch files, infer replacement targets, choose evictions, run package
/// scripts, or touch renderer/RHI/native handles.
[[nodiscard]] AssetRuntimePackageHotReloadReplacementResult
execute_asset_runtime_package_hot_reload_replacement_safe_point(
    IFileSystem& filesystem, AssetRuntimeReplacementState& replacements,
    runtime::RuntimeResidentPackageMountSetV2& mount_set, runtime::RuntimeResidentCatalogCacheV2& catalog_cache,
    const AssetRuntimePackageHotReloadReplacementDesc& desc);

} // namespace mirakana
