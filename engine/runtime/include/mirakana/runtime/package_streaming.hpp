// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimePackageStreamingExecutionMode : std::uint8_t {
    planning_only = 0,
    host_gated_safe_point,
};

enum class RuntimePackageStreamingExecutionStatus : std::uint8_t {
    invalid_descriptor = 0,
    validation_preflight_required,
    package_load_failed,
    residency_hint_failed,
    over_budget_intent,
    safe_point_replacement_failed,
    committed,
    resident_mount_failed,
    resident_replace_failed,
    resident_catalog_refresh_failed,
};

struct RuntimePackageStreamingExecutionDiagnostic {
    std::string code;
    std::string message;
};

struct RuntimePackageStreamingExecutionDesc {
    std::string target_id;
    std::string package_index_path;
    std::string content_root;
    std::string runtime_scene_validation_target_id;
    RuntimePackageStreamingExecutionMode mode{RuntimePackageStreamingExecutionMode::planning_only};
    std::uint64_t resident_budget_bytes{0};
    bool safe_point_required{true};
    bool runtime_scene_validation_succeeded{false};
    std::vector<AssetId> required_preload_assets;
    std::vector<AssetKind> resident_resource_kinds;
    std::uint32_t max_resident_packages{0};
};

struct RuntimePackageStreamingExecutionResult {
    RuntimePackageStreamingExecutionStatus status{RuntimePackageStreamingExecutionStatus::invalid_descriptor};
    std::string target_id;
    std::string package_index_path;
    std::string runtime_scene_validation_target_id;
    std::uint64_t estimated_resident_bytes{0};
    std::uint64_t resident_budget_bytes{0};
    std::uint32_t required_preload_asset_count{0};
    std::uint32_t resident_resource_kind_count{0};
    std::uint32_t resident_package_count{0};
    std::uint32_t resident_mount_generation{0};
    RuntimePackageSafePointReplacementResult replacement;
    RuntimeResidentPackageMountResultV2 resident_mount;
    RuntimeResidentPackageReplaceCommitResultV2 resident_replace;
    RuntimeResidentCatalogCacheRefreshResultV2 resident_catalog_refresh;
    std::vector<RuntimePackageStreamingExecutionDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimePackageStreamingExecutionResult execute_selected_runtime_package_streaming_safe_point(
    RuntimeAssetPackageStore& store, RuntimeResourceCatalogV2& catalog,
    const RuntimePackageStreamingExecutionDesc& desc, RuntimeAssetPackageLoadResult loaded_package);

[[nodiscard]] RuntimePackageStreamingExecutionResult execute_selected_runtime_package_streaming_safe_point(
    RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc, RuntimeAssetPackageLoadResult loaded_package);

[[nodiscard]] RuntimePackageStreamingExecutionResult
execute_selected_runtime_package_streaming_resident_replace_safe_point(RuntimeResidentPackageMountSetV2& mount_set,
                                                                       RuntimeResidentCatalogCacheV2& catalog_cache,
                                                                       RuntimeResidentPackageMountIdV2 mount_id,
                                                                       RuntimePackageMountOverlay overlay,
                                                                       const RuntimePackageStreamingExecutionDesc& desc,
                                                                       RuntimeAssetPackageLoadResult loaded_package);

} // namespace mirakana::runtime
