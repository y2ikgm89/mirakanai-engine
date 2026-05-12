// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/package_streaming.hpp"

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

void add_diagnostic(RuntimePackageStreamingExecutionResult& result, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageStreamingExecutionDiagnostic{
        .code = std::move(code),
        .message = std::move(message),
    });
}

bool validate_descriptor(const RuntimePackageStreamingExecutionDesc& desc,
                         RuntimePackageStreamingExecutionResult& result) {
    if (desc.target_id.empty()) {
        add_diagnostic(result, "invalid-target-id", "package streaming target id must be non-empty");
        return false;
    }
    if (desc.package_index_path.empty() || !desc.package_index_path.ends_with(".geindex")) {
        add_diagnostic(result, "invalid-package-index-path",
                       "package streaming target must reference a .geindex package");
        return false;
    }
    if (desc.runtime_scene_validation_target_id.empty()) {
        add_diagnostic(result, "invalid-runtime-scene-validation-target",
                       "package streaming target must reference runtime scene validation evidence");
        return false;
    }
    if (desc.mode != RuntimePackageStreamingExecutionMode::host_gated_safe_point) {
        add_diagnostic(result, "host-gated-safe-point-mode-required",
                       "selected package streaming execution requires host_gated_safe_point mode");
        return false;
    }
    if (!desc.safe_point_required) {
        add_diagnostic(result, "safe-point-required", "selected package streaming execution requires a safe point");
        return false;
    }
    if (desc.resident_budget_bytes == 0) {
        add_diagnostic(result, "resident-budget-required", "resident budget intent must be a positive byte count");
        return false;
    }
    return true;
}

bool contains_kind(const std::vector<AssetKind>& kinds, AssetKind kind) {
    return std::ranges::find(kinds, kind) != kinds.end();
}

bool validate_residency_hints(const RuntimePackageStreamingExecutionDesc& desc, const RuntimeAssetPackage& package,
                              RuntimePackageStreamingExecutionResult& result, std::uint32_t resident_package_count) {
    result.required_preload_asset_count = static_cast<std::uint32_t>(desc.required_preload_assets.size());
    result.resident_resource_kind_count = static_cast<std::uint32_t>(desc.resident_resource_kinds.size());
    result.resident_package_count = resident_package_count;

    bool valid = true;
    if (desc.max_resident_packages > 0 && result.resident_package_count > desc.max_resident_packages) {
        add_diagnostic(result, "max-resident-packages-exceeded",
                       "selected package streaming execution exceeds the resident package count hint");
        valid = false;
    }

    for (const auto asset : desc.required_preload_assets) {
        if (package.find(asset) == nullptr) {
            add_diagnostic(result, "preload-asset-missing",
                           "loaded package does not contain required preload asset id " + std::to_string(asset.value));
            valid = false;
        }
    }

    if (!desc.resident_resource_kinds.empty()) {
        for (const auto& record : package.records()) {
            if (!contains_kind(desc.resident_resource_kinds, record.kind)) {
                add_diagnostic(result, "resident-resource-kind-disallowed",
                               "loaded package contains resident resource kind " +
                                   std::to_string(static_cast<int>(record.kind)) + " at " + record.path);
                valid = false;
            }
        }
    }

    return valid;
}

bool contains_mount_id(const RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentPackageMountIdV2 mount_id) {
    return std::ranges::any_of(mount_set.mounts(), [mount_id](const RuntimeResidentPackageMountRecordV2& mounted) {
        return mounted.id == mount_id;
    });
}

void set_resident_mount_failure(RuntimePackageStreamingExecutionResult& result,
                                RuntimeResidentPackageMountIdV2 mount_id, RuntimeResidentPackageMountStatusV2 status,
                                std::string code, std::string message) {
    result.resident_mount = RuntimeResidentPackageMountResultV2{
        .status = status,
        .diagnostic =
            RuntimeResidentPackageMountDiagnosticV2{
                .mount = mount_id,
                .code = code,
                .message = message,
            },
    };
    add_diagnostic(result, std::move(code), std::move(message));
}

bool validate_resident_mount_id(const RuntimeResidentPackageMountSetV2& mount_set,
                                RuntimeResidentPackageMountIdV2 mount_id,
                                RuntimePackageStreamingExecutionResult& result) {
    if (mount_id.value == 0) {
        set_resident_mount_failure(result, mount_id, RuntimeResidentPackageMountStatusV2::invalid_mount_id,
                                   "invalid-mount-id", "resident package streaming mount id must be non-zero");
        return false;
    }
    if (contains_mount_id(mount_set, mount_id)) {
        set_resident_mount_failure(result, mount_id, RuntimeResidentPackageMountStatusV2::duplicate_mount_id,
                                   "duplicate-mount-id", "resident package streaming mount id is already mounted");
        return false;
    }
    return true;
}

[[nodiscard]] std::vector<RuntimeAssetPackage>
project_resident_packages(const RuntimeResidentPackageMountSetV2& mount_set,
                          const RuntimeAssetPackage& loaded_package) {
    std::vector<RuntimeAssetPackage> projected;
    projected.reserve(mount_set.mounts().size() + 1U);
    for (const auto& mount : mount_set.mounts()) {
        projected.push_back(mount.package);
    }
    projected.push_back(loaded_package);
    return projected;
}

[[nodiscard]] RuntimeResourceResidencyBudgetExecutionResultV2
evaluate_projected_resident_budget(const RuntimeResidentPackageMountSetV2& mount_set,
                                   const RuntimeAssetPackage& loaded_package, RuntimePackageMountOverlay overlay,
                                   const RuntimePackageStreamingExecutionDesc& desc) {
    const auto projected_packages = project_resident_packages(mount_set, loaded_package);
    const RuntimeAssetPackage projected_view = merge_runtime_asset_packages_overlay(projected_packages, overlay);
    const RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = desc.resident_budget_bytes,
        .max_resident_asset_records = {},
    };
    return evaluate_runtime_resource_residency_budget(projected_view, budget);
}

void add_budget_diagnostics(RuntimePackageStreamingExecutionResult& result,
                            const RuntimeResourceResidencyBudgetExecutionResultV2& budget_execution) {
    add_diagnostic(result, "resident-budget-intent-exceeded",
                   "projected resident package content bytes exceed the selected resident budget intent");
    for (const auto& diagnostic : budget_execution.diagnostics) {
        add_diagnostic(result, diagnostic.code, diagnostic.message);
    }
}

void add_catalog_refresh_diagnostics(RuntimePackageStreamingExecutionResult& result,
                                     const RuntimeResidentCatalogCacheRefreshResultV2& refresh) {
    for (const auto& diagnostic : refresh.budget_execution.diagnostics) {
        add_diagnostic(result, diagnostic.code, diagnostic.message);
    }
    for (const auto& diagnostic : refresh.catalog_build.diagnostics) {
        add_diagnostic(result, "catalog-build-failed", diagnostic.diagnostic);
    }
}

void set_resident_catalog_build_failure(RuntimePackageStreamingExecutionResult& result,
                                        RuntimeResourceCatalogBuildResultV2 catalog_build,
                                        std::size_t projected_package_count, std::uint32_t mount_generation,
                                        std::uint32_t catalog_generation) {
    result.resident_catalog_refresh = RuntimeResidentCatalogCacheRefreshResultV2{
        .status = RuntimeResidentCatalogCacheStatusV2::catalog_build_failed,
        .budget_execution = {},
        .catalog_build = std::move(catalog_build),
        .mounted_package_count = projected_package_count,
        .mount_generation = mount_generation,
        .previous_catalog_generation = catalog_generation,
        .catalog_generation = catalog_generation,
        .invoked_catalog_build = true,
        .reused_cache = false,
    };
    add_catalog_refresh_diagnostics(result, result.resident_catalog_refresh);
}

bool validate_loaded_package_catalog_before_mount(RuntimePackageStreamingExecutionResult& result,
                                                  const RuntimeAssetPackage& loaded_package,
                                                  std::size_t projected_package_count,
                                                  const RuntimeResidentPackageMountSetV2& mount_set,
                                                  const RuntimeResidentCatalogCacheV2& catalog_cache) {
    RuntimeResourceCatalogV2 scratch;
    auto catalog_build = build_runtime_resource_catalog_v2(scratch, loaded_package);
    if (catalog_build.succeeded()) {
        return true;
    }

    set_resident_catalog_build_failure(result, std::move(catalog_build), projected_package_count,
                                       mount_set.generation(), catalog_cache.catalog().generation());
    return false;
}

} // namespace

bool RuntimePackageStreamingExecutionResult::succeeded() const noexcept {
    return status == RuntimePackageStreamingExecutionStatus::committed;
}

RuntimePackageStreamingExecutionResult execute_selected_runtime_package_streaming_safe_point(
    RuntimeAssetPackageStore& store, RuntimeResourceCatalogV2& catalog,
    const RuntimePackageStreamingExecutionDesc& desc, RuntimeAssetPackageLoadResult loaded_package) {
    RuntimePackageStreamingExecutionResult result;
    result.target_id = desc.target_id;
    result.package_index_path = desc.package_index_path;
    result.runtime_scene_validation_target_id = desc.runtime_scene_validation_target_id;
    result.resident_budget_bytes = desc.resident_budget_bytes;

    if (!validate_descriptor(desc, result)) {
        result.status = RuntimePackageStreamingExecutionStatus::invalid_descriptor;
        return result;
    }

    if (!desc.runtime_scene_validation_succeeded) {
        result.status = RuntimePackageStreamingExecutionStatus::validation_preflight_required;
        add_diagnostic(result, "runtime-scene-validation-required",
                       "validate-runtime-scene-package must succeed before selected package streaming execution");
        return result;
    }

    if (!loaded_package.succeeded()) {
        result.status = RuntimePackageStreamingExecutionStatus::package_load_failed;
        if (loaded_package.failures.empty()) {
            add_diagnostic(result, "package-load-failed", "runtime package load failed");
        }
        for (const auto& failure : loaded_package.failures) {
            std::string message = failure.diagnostic;
            if (!failure.path.empty()) {
                message += " at ";
                message += failure.path;
            }
            add_diagnostic(result, "package-load-failed", std::move(message));
        }
        return result;
    }

    if (!validate_residency_hints(desc, loaded_package.package, result, 1)) {
        result.status = RuntimePackageStreamingExecutionStatus::residency_hint_failed;
        return result;
    }

    result.estimated_resident_bytes = estimate_runtime_asset_package_resident_bytes(loaded_package.package);
    if (result.estimated_resident_bytes > result.resident_budget_bytes) {
        result.status = RuntimePackageStreamingExecutionStatus::over_budget_intent;
        add_diagnostic(result, "resident-budget-intent-exceeded",
                       "loaded package content bytes exceed the selected resident budget intent");
        return result;
    }

    if (!store.stage_if_loaded(std::move(loaded_package))) {
        result.status = RuntimePackageStreamingExecutionStatus::package_load_failed;
        add_diagnostic(result, "package-load-failed", "runtime package load result could not be staged");
        return result;
    }

    result.replacement = commit_runtime_package_safe_point_replacement(store, catalog);
    if (!result.replacement.succeeded()) {
        result.status = RuntimePackageStreamingExecutionStatus::safe_point_replacement_failed;
        for (const auto& diagnostic : result.replacement.diagnostics) {
            add_diagnostic(result, "catalog-build-failed", diagnostic.diagnostic);
        }
        return result;
    }

    result.status = RuntimePackageStreamingExecutionStatus::committed;
    return result;
}

RuntimePackageStreamingExecutionResult execute_selected_runtime_package_streaming_safe_point(
    RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc, RuntimeAssetPackageLoadResult loaded_package) {
    RuntimePackageStreamingExecutionResult result;
    result.target_id = desc.target_id;
    result.package_index_path = desc.package_index_path;
    result.runtime_scene_validation_target_id = desc.runtime_scene_validation_target_id;
    result.resident_budget_bytes = desc.resident_budget_bytes;
    result.resident_mount_generation = mount_set.generation();

    if (!validate_descriptor(desc, result)) {
        result.status = RuntimePackageStreamingExecutionStatus::invalid_descriptor;
        return result;
    }

    if (!desc.runtime_scene_validation_succeeded) {
        result.status = RuntimePackageStreamingExecutionStatus::validation_preflight_required;
        add_diagnostic(result, "runtime-scene-validation-required",
                       "validate-runtime-scene-package must succeed before selected package streaming execution");
        return result;
    }

    if (!loaded_package.succeeded()) {
        result.status = RuntimePackageStreamingExecutionStatus::package_load_failed;
        if (loaded_package.failures.empty()) {
            add_diagnostic(result, "package-load-failed", "runtime package load failed");
        }
        for (const auto& failure : loaded_package.failures) {
            std::string message = failure.diagnostic;
            if (!failure.path.empty()) {
                message += " at ";
                message += failure.path;
            }
            add_diagnostic(result, "package-load-failed", std::move(message));
        }
        return result;
    }

    if (!validate_resident_mount_id(mount_set, mount_id, result)) {
        result.status = RuntimePackageStreamingExecutionStatus::resident_mount_failed;
        return result;
    }

    const auto projected_resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size() + 1U);
    if (!validate_residency_hints(desc, loaded_package.package, result, projected_resident_package_count)) {
        result.status = RuntimePackageStreamingExecutionStatus::residency_hint_failed;
        return result;
    }

    const auto budget_execution = evaluate_projected_resident_budget(mount_set, loaded_package.package, overlay, desc);
    result.estimated_resident_bytes = budget_execution.estimated_resident_content_bytes;
    if (!budget_execution.within_budget) {
        result.status = RuntimePackageStreamingExecutionStatus::over_budget_intent;
        add_budget_diagnostics(result, budget_execution);
        return result;
    }

    if (!validate_loaded_package_catalog_before_mount(result, loaded_package.package, projected_resident_package_count,
                                                      mount_set, catalog_cache)) {
        result.status = RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed;
        return result;
    }

    result.resident_mount = mount_set.mount(RuntimeResidentPackageMountRecordV2{
        .id = mount_id,
        .label = desc.target_id,
        .package = std::move(loaded_package.package),
    });
    result.resident_mount_generation = mount_set.generation();
    if (!result.resident_mount.succeeded()) {
        result.status = RuntimePackageStreamingExecutionStatus::resident_mount_failed;
        if (!result.resident_mount.diagnostic.code.empty()) {
            add_diagnostic(result, result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        }
        return result;
    }

    const RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = desc.resident_budget_bytes,
        .max_resident_asset_records = {},
    };
    result.resident_catalog_refresh = catalog_cache.refresh(mount_set, overlay, budget);
    if (!result.resident_catalog_refresh.succeeded()) {
        result.status = RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed;
        add_catalog_refresh_diagnostics(result, result.resident_catalog_refresh);
        const auto rollback = mount_set.unmount(mount_id);
        if (!rollback.succeeded() && !rollback.diagnostic.code.empty()) {
            add_diagnostic(result, rollback.diagnostic.code, rollback.diagnostic.message);
        }
        result.resident_mount_generation = mount_set.generation();
        return result;
    }

    result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());
    result.resident_mount_generation = mount_set.generation();
    result.status = RuntimePackageStreamingExecutionStatus::committed;
    return result;
}

} // namespace mirakana::runtime
