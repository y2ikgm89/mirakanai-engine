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

bool validate_projected_resident_catalog_hints(const RuntimePackageStreamingExecutionDesc& desc,
                                               const RuntimeResourceCatalogV2& catalog,
                                               RuntimePackageStreamingExecutionResult& result,
                                               std::uint32_t resident_package_count) {
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
        if (!find_runtime_resource_v2(catalog, asset).has_value()) {
            add_diagnostic(result, "preload-asset-missing",
                           "projected resident catalog does not contain required preload asset id " +
                               std::to_string(asset.value));
            valid = false;
        }
    }

    if (!desc.resident_resource_kinds.empty()) {
        for (const auto& record : catalog.records()) {
            if (!contains_kind(desc.resident_resource_kinds, record.kind)) {
                add_diagnostic(result, "resident-resource-kind-disallowed",
                               "projected resident catalog contains resident resource kind " +
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

void set_resident_replace_failure(RuntimePackageStreamingExecutionResult& result,
                                  RuntimeResidentPackageMountIdV2 mount_id,
                                  RuntimeResidentPackageReplaceCommitStatusV2 status, std::string code,
                                  std::string message) {
    result.resident_replace = RuntimeResidentPackageReplaceCommitResultV2{
        .status = status,
        .diagnostic =
            RuntimeResidentPackageMountDiagnosticV2{
                .mount = mount_id,
                .code = code,
                .message = message,
            },
        .candidate_catalog_build = {},
        .catalog_refresh = {},
        .previous_mount_generation = result.resident_mount_generation,
        .mount_generation = result.resident_mount_generation,
        .mounted_package_count = result.resident_package_count,
        .invoked_candidate_catalog_build = false,
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

bool validate_resident_replace_mount_id(const RuntimeResidentPackageMountSetV2& mount_set,
                                        RuntimeResidentPackageMountIdV2 mount_id,
                                        RuntimePackageStreamingExecutionResult& result) {
    if (mount_id.value == 0) {
        set_resident_replace_failure(result, mount_id, RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id,
                                     "invalid-mount-id",
                                     "resident package streaming replacement mount id must be non-zero");
        return false;
    }
    if (!contains_mount_id(mount_set, mount_id)) {
        set_resident_replace_failure(result, mount_id, RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id,
                                     "missing-mount-id",
                                     "resident package streaming replacement mount id is not mounted");
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

void add_candidate_catalog_build_diagnostics(RuntimePackageStreamingExecutionResult& result,
                                             const RuntimeResourceCatalogBuildResultV2& catalog_build) {
    for (const auto& diagnostic : catalog_build.diagnostics) {
        add_diagnostic(result, "catalog-build-failed", diagnostic.diagnostic);
    }
}

void add_candidate_load_diagnostics(RuntimePackageStreamingExecutionResult& result,
                                    const RuntimePackageCandidateLoadResultV2& candidate_load) {
    if (candidate_load.diagnostics.empty()) {
        add_diagnostic(result, "package-load-failed", "runtime package candidate load failed");
        return;
    }

    for (const auto& diagnostic : candidate_load.diagnostics) {
        std::string message = diagnostic.message;
        if (!diagnostic.path.empty()) {
            message += " at ";
            message += diagnostic.path;
        }
        add_diagnostic(result, diagnostic.code, std::move(message));
    }
}

void add_reviewed_evictions_mount_diagnostics(
    RuntimePackageStreamingExecutionResult& result,
    const RuntimePackageCandidateResidentMountReviewedEvictionsResultV2& reviewed_mount) {
    for (const auto& diagnostic : reviewed_mount.diagnostics) {
        std::string message = diagnostic.message;
        if (!diagnostic.path.empty()) {
            message += " at ";
            message += diagnostic.path;
        }
        add_diagnostic(result, diagnostic.code, std::move(message));
    }
}

[[nodiscard]] RuntimePackageStreamingExecutionStatus
map_reviewed_evictions_mount_status(RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2 status) noexcept {
    switch (status) {
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::mounted:
        return RuntimePackageStreamingExecutionStatus::committed;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::invalid_mount_id:
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::duplicate_mount_id:
        return RuntimePackageStreamingExecutionStatus::resident_mount_failed;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::candidate_load_failed:
        return RuntimePackageStreamingExecutionStatus::package_load_failed;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::invalid_eviction_candidate_mount_id:
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::duplicate_eviction_candidate_mount_id:
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::missing_eviction_candidate_mount_id:
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::protected_eviction_candidate_mount_id:
        return RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::budget_failed:
        return RuntimePackageStreamingExecutionStatus::over_budget_intent;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::catalog_refresh_failed:
        return RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed;
    }
    return RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed;
}

void copy_reviewed_evictions_mount_result(
    RuntimePackageStreamingExecutionResult& result,
    const RuntimePackageCandidateResidentMountReviewedEvictionsResultV2& reviewed_mount) {
    result.candidate_load = reviewed_mount.candidate_load;
    result.resident_mount = reviewed_mount.resident_mount;
    result.eviction_plan = reviewed_mount.eviction_plan;
    result.resident_catalog_refresh = reviewed_mount.catalog_refresh;
    result.estimated_resident_bytes = reviewed_mount.projected_resident_bytes;
    result.resident_mount_generation = reviewed_mount.mount_generation;
    result.resident_package_count = static_cast<std::uint32_t>(reviewed_mount.mounted_package_count);
    result.evicted_mount_count = reviewed_mount.evicted_mount_count;
    result.invoked_eviction_plan = reviewed_mount.invoked_eviction_plan;
    add_reviewed_evictions_mount_diagnostics(result, reviewed_mount);
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

[[nodiscard]] RuntimePackageIndexDiscoveryCandidateV2
make_package_streaming_candidate(const RuntimePackageStreamingExecutionDesc& desc) {
    return RuntimePackageIndexDiscoveryCandidateV2{
        .package_index_path = desc.package_index_path,
        .content_root = desc.content_root,
        .label = desc.target_id,
    };
}

} // namespace

bool RuntimePackageStreamingExecutionResult::succeeded() const noexcept {
    return status == RuntimePackageStreamingExecutionStatus::committed && committed;
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
    result.committed = true;
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
    result.committed = true;
    return result;
}

RuntimePackageStreamingExecutionResult execute_selected_runtime_package_streaming_candidate_resident_mount_safe_point(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc) {
    RuntimePackageStreamingExecutionResult result;
    result.target_id = desc.target_id;
    result.package_index_path = desc.package_index_path;
    result.runtime_scene_validation_target_id = desc.runtime_scene_validation_target_id;
    result.resident_budget_bytes = desc.resident_budget_bytes;
    result.resident_mount_generation = mount_set.generation();
    result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());

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

    if (!validate_resident_mount_id(mount_set, mount_id, result)) {
        result.status = RuntimePackageStreamingExecutionStatus::resident_mount_failed;
        return result;
    }

    result.candidate_load = load_runtime_package_candidate_v2(filesystem, make_package_streaming_candidate(desc));
    if (!result.candidate_load.succeeded()) {
        result.status = RuntimePackageStreamingExecutionStatus::package_load_failed;
        add_candidate_load_diagnostics(result, result.candidate_load);
        return result;
    }

    const auto projected_resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size() + 1U);
    if (!validate_residency_hints(desc, result.candidate_load.loaded_package.package, result,
                                  projected_resident_package_count)) {
        result.status = RuntimePackageStreamingExecutionStatus::residency_hint_failed;
        return result;
    }

    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    result.resident_mount = projected_mount_set.mount(RuntimeResidentPackageMountRecordV2{
        .id = mount_id,
        .label = desc.target_id,
        .package = result.candidate_load.loaded_package.package,
    });
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
    RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    result.resident_catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, overlay, budget);
    result.estimated_resident_bytes = result.resident_catalog_refresh.budget_execution.estimated_resident_content_bytes;
    result.resident_package_count = static_cast<std::uint32_t>(projected_mount_set.mounts().size());
    if (!result.resident_catalog_refresh.succeeded()) {
        if (result.resident_catalog_refresh.status == RuntimeResidentCatalogCacheStatusV2::budget_failed) {
            result.status = RuntimePackageStreamingExecutionStatus::over_budget_intent;
            add_budget_diagnostics(result, result.resident_catalog_refresh.budget_execution);
        } else {
            result.status = RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed;
            add_catalog_refresh_diagnostics(result, result.resident_catalog_refresh);
        }
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);
    result.resident_mount_generation = mount_set.generation();
    result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());
    result.status = RuntimePackageStreamingExecutionStatus::committed;
    result.committed = true;
    return result;
}

RuntimePackageStreamingExecutionResult
execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc,
    std::vector<RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order,
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids) {
    RuntimePackageStreamingExecutionResult result;
    result.target_id = desc.target_id;
    result.package_index_path = desc.package_index_path;
    result.runtime_scene_validation_target_id = desc.runtime_scene_validation_target_id;
    result.resident_budget_bytes = desc.resident_budget_bytes;
    result.resident_mount_generation = mount_set.generation();
    result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());

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

    const RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = desc.resident_budget_bytes,
        .max_resident_asset_records = {},
    };
    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    const auto reviewed_mount = commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
        filesystem, projected_mount_set, projected_catalog_cache,
        RuntimePackageCandidateResidentMountReviewedEvictionsDescV2{
            .candidate = make_package_streaming_candidate(desc),
            .mount_id = mount_id,
            .overlay = overlay,
            .budget = budget,
            .eviction_candidate_unmount_order = std::move(eviction_candidate_unmount_order),
            .protected_mount_ids = std::move(protected_mount_ids),
        });
    copy_reviewed_evictions_mount_result(result, reviewed_mount);

    if (!reviewed_mount.succeeded()) {
        result.status = map_reviewed_evictions_mount_status(reviewed_mount.status);
        result.committed = false;
        result.resident_mount_generation = mount_set.generation();
        if (result.status == RuntimePackageStreamingExecutionStatus::resident_mount_failed ||
            result.status == RuntimePackageStreamingExecutionStatus::package_load_failed) {
            result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());
        }
        return result;
    }

    if (!validate_residency_hints(desc, reviewed_mount.candidate_load.loaded_package.package, result,
                                  static_cast<std::uint32_t>(reviewed_mount.mounted_package_count))) {
        result.status = RuntimePackageStreamingExecutionStatus::residency_hint_failed;
        result.committed = false;
        result.resident_mount_generation = mount_set.generation();
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);
    result.resident_mount_generation = mount_set.generation();
    result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());
    result.status = RuntimePackageStreamingExecutionStatus::committed;
    result.committed = true;
    return result;
}

RuntimePackageStreamingExecutionResult execute_selected_runtime_package_streaming_resident_replace_safe_point(
    RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc, RuntimeAssetPackageLoadResult loaded_package) {
    RuntimePackageStreamingExecutionResult result;
    result.target_id = desc.target_id;
    result.package_index_path = desc.package_index_path;
    result.runtime_scene_validation_target_id = desc.runtime_scene_validation_target_id;
    result.resident_budget_bytes = desc.resident_budget_bytes;
    result.resident_mount_generation = mount_set.generation();
    result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());

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

    if (!validate_resident_replace_mount_id(mount_set, mount_id, result)) {
        result.status = RuntimePackageStreamingExecutionStatus::resident_replace_failed;
        return result;
    }

    const auto projected_resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());
    if (!validate_residency_hints(desc, loaded_package.package, result, projected_resident_package_count)) {
        result.status = RuntimePackageStreamingExecutionStatus::residency_hint_failed;
        return result;
    }

    const RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = desc.resident_budget_bytes,
        .max_resident_asset_records = {},
    };
    result.resident_replace = commit_runtime_resident_package_replace_v2(
        mount_set, catalog_cache, mount_id, std::move(loaded_package.package), overlay, budget);
    result.resident_catalog_refresh = result.resident_replace.catalog_refresh;
    result.resident_mount_generation = result.resident_replace.mount_generation;
    result.resident_package_count = static_cast<std::uint32_t>(result.resident_replace.mounted_package_count);
    result.estimated_resident_bytes =
        result.resident_replace.catalog_refresh.budget_execution.estimated_resident_content_bytes;

    if (!result.resident_replace.succeeded()) {
        switch (result.resident_replace.status) {
        case RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id:
        case RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id:
            result.status = RuntimePackageStreamingExecutionStatus::resident_replace_failed;
            if (!result.resident_replace.diagnostic.code.empty()) {
                add_diagnostic(result, result.resident_replace.diagnostic.code,
                               result.resident_replace.diagnostic.message);
            }
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::budget_failed:
            result.status = RuntimePackageStreamingExecutionStatus::over_budget_intent;
            add_budget_diagnostics(result, result.resident_replace.catalog_refresh.budget_execution);
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::catalog_build_failed:
            result.status = RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed;
            if (result.resident_replace.invoked_candidate_catalog_build &&
                !result.resident_replace.candidate_catalog_build.succeeded()) {
                add_candidate_catalog_build_diagnostics(result, result.resident_replace.candidate_catalog_build);
            } else {
                add_catalog_refresh_diagnostics(result, result.resident_replace.catalog_refresh);
            }
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::replaced:
            result.status = RuntimePackageStreamingExecutionStatus::committed;
            result.committed = true;
            break;
        }
        return result;
    }

    result.status = RuntimePackageStreamingExecutionStatus::committed;
    result.committed = true;
    return result;
}

RuntimePackageStreamingExecutionResult execute_selected_runtime_package_streaming_candidate_resident_replace_safe_point(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc) {
    RuntimePackageStreamingExecutionResult result;
    result.target_id = desc.target_id;
    result.package_index_path = desc.package_index_path;
    result.runtime_scene_validation_target_id = desc.runtime_scene_validation_target_id;
    result.resident_budget_bytes = desc.resident_budget_bytes;
    result.resident_mount_generation = mount_set.generation();
    result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());

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

    if (!validate_resident_replace_mount_id(mount_set, mount_id, result)) {
        result.status = RuntimePackageStreamingExecutionStatus::resident_replace_failed;
        return result;
    }

    result.candidate_load = load_runtime_package_candidate_v2(filesystem, make_package_streaming_candidate(desc));
    if (!result.candidate_load.succeeded()) {
        result.status = RuntimePackageStreamingExecutionStatus::package_load_failed;
        add_candidate_load_diagnostics(result, result.candidate_load);
        return result;
    }

    const auto projected_resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());
    if (!validate_residency_hints(desc, result.candidate_load.loaded_package.package, result,
                                  projected_resident_package_count)) {
        result.status = RuntimePackageStreamingExecutionStatus::residency_hint_failed;
        return result;
    }

    result.estimated_resident_bytes = result.candidate_load.estimated_resident_bytes;
    const RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = desc.resident_budget_bytes,
        .max_resident_asset_records = {},
    };
    result.resident_replace = commit_runtime_resident_package_replace_v2(
        mount_set, catalog_cache, mount_id, result.candidate_load.loaded_package.package, overlay, budget);
    result.resident_catalog_refresh = result.resident_replace.catalog_refresh;
    result.resident_mount_generation = result.resident_replace.mount_generation;
    result.resident_package_count = static_cast<std::uint32_t>(result.resident_replace.mounted_package_count);
    result.estimated_resident_bytes =
        result.resident_replace.catalog_refresh.budget_execution.estimated_resident_content_bytes;

    if (!result.resident_replace.succeeded()) {
        switch (result.resident_replace.status) {
        case RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id:
        case RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id:
            result.status = RuntimePackageStreamingExecutionStatus::resident_replace_failed;
            if (!result.resident_replace.diagnostic.code.empty()) {
                add_diagnostic(result, result.resident_replace.diagnostic.code,
                               result.resident_replace.diagnostic.message);
            }
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::budget_failed:
            result.status = RuntimePackageStreamingExecutionStatus::over_budget_intent;
            add_budget_diagnostics(result, result.resident_replace.catalog_refresh.budget_execution);
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::catalog_build_failed:
            result.status = RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed;
            if (result.resident_replace.invoked_candidate_catalog_build &&
                !result.resident_replace.candidate_catalog_build.succeeded()) {
                add_candidate_catalog_build_diagnostics(result, result.resident_replace.candidate_catalog_build);
            } else {
                add_catalog_refresh_diagnostics(result, result.resident_replace.catalog_refresh);
            }
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::replaced:
            result.status = RuntimePackageStreamingExecutionStatus::committed;
            result.committed = true;
            break;
        }
        return result;
    }

    result.status = RuntimePackageStreamingExecutionStatus::committed;
    result.committed = true;
    return result;
}

RuntimePackageStreamingExecutionResult execute_selected_runtime_package_streaming_resident_unmount_safe_point(
    RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 mount_id, RuntimePackageMountOverlay overlay,
    const RuntimePackageStreamingExecutionDesc& desc) {
    RuntimePackageStreamingExecutionResult result;
    result.target_id = desc.target_id;
    result.package_index_path = desc.package_index_path;
    result.runtime_scene_validation_target_id = desc.runtime_scene_validation_target_id;
    result.resident_budget_bytes = desc.resident_budget_bytes;
    result.resident_mount_generation = mount_set.generation();
    result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());

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

    const RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = desc.resident_budget_bytes,
        .max_resident_asset_records = {},
    };

    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    const auto projected_unmount = commit_runtime_resident_package_unmount_v2(
        projected_mount_set, projected_catalog_cache, mount_id, overlay, budget);
    result.resident_catalog_refresh = projected_unmount.catalog_refresh;
    result.resident_mount_generation = projected_unmount.mount_generation;
    result.resident_package_count = static_cast<std::uint32_t>(projected_unmount.mounted_package_count);
    result.estimated_resident_bytes =
        projected_unmount.catalog_refresh.budget_execution.estimated_resident_content_bytes;

    if (!projected_unmount.succeeded()) {
        result.resident_unmount = projected_unmount;
        switch (projected_unmount.status) {
        case RuntimeResidentPackageUnmountCommitStatusV2::invalid_mount_id:
        case RuntimeResidentPackageUnmountCommitStatusV2::missing_mount_id:
            result.status = RuntimePackageStreamingExecutionStatus::resident_unmount_failed;
            if (!projected_unmount.unmount.diagnostic.code.empty()) {
                add_diagnostic(result, projected_unmount.unmount.diagnostic.code,
                               projected_unmount.unmount.diagnostic.message);
            }
            break;
        case RuntimeResidentPackageUnmountCommitStatusV2::budget_failed:
            result.status = RuntimePackageStreamingExecutionStatus::over_budget_intent;
            add_budget_diagnostics(result, projected_unmount.catalog_refresh.budget_execution);
            break;
        case RuntimeResidentPackageUnmountCommitStatusV2::catalog_build_failed:
            result.status = RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed;
            add_catalog_refresh_diagnostics(result, projected_unmount.catalog_refresh);
            break;
        case RuntimeResidentPackageUnmountCommitStatusV2::unmounted:
            result.status = RuntimePackageStreamingExecutionStatus::committed;
            result.committed = true;
            break;
        }
        result.resident_mount_generation = mount_set.generation();
        result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());
        return result;
    }

    if (!validate_projected_resident_catalog_hints(desc, projected_catalog_cache.catalog(), result,
                                                   static_cast<std::uint32_t>(projected_mount_set.mounts().size()))) {
        result.status = RuntimePackageStreamingExecutionStatus::residency_hint_failed;
        result.resident_mount_generation = mount_set.generation();
        result.resident_package_count = static_cast<std::uint32_t>(mount_set.mounts().size());
        return result;
    }

    result.resident_unmount =
        commit_runtime_resident_package_unmount_v2(mount_set, catalog_cache, mount_id, overlay, budget);
    result.resident_catalog_refresh = result.resident_unmount.catalog_refresh;
    result.resident_mount_generation = result.resident_unmount.mount_generation;
    result.resident_package_count = static_cast<std::uint32_t>(result.resident_unmount.mounted_package_count);
    result.estimated_resident_bytes =
        result.resident_unmount.catalog_refresh.budget_execution.estimated_resident_content_bytes;

    if (!result.resident_unmount.succeeded()) {
        switch (result.resident_unmount.status) {
        case RuntimeResidentPackageUnmountCommitStatusV2::invalid_mount_id:
        case RuntimeResidentPackageUnmountCommitStatusV2::missing_mount_id:
            result.status = RuntimePackageStreamingExecutionStatus::resident_unmount_failed;
            if (!result.resident_unmount.unmount.diagnostic.code.empty()) {
                add_diagnostic(result, result.resident_unmount.unmount.diagnostic.code,
                               result.resident_unmount.unmount.diagnostic.message);
            }
            break;
        case RuntimeResidentPackageUnmountCommitStatusV2::budget_failed:
            result.status = RuntimePackageStreamingExecutionStatus::over_budget_intent;
            add_budget_diagnostics(result, result.resident_unmount.catalog_refresh.budget_execution);
            break;
        case RuntimeResidentPackageUnmountCommitStatusV2::catalog_build_failed:
            result.status = RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed;
            add_catalog_refresh_diagnostics(result, result.resident_unmount.catalog_refresh);
            break;
        case RuntimeResidentPackageUnmountCommitStatusV2::unmounted:
            result.status = RuntimePackageStreamingExecutionStatus::committed;
            result.committed = true;
            break;
        }
        return result;
    }

    result.status = RuntimePackageStreamingExecutionStatus::committed;
    result.committed = true;
    return result;
}

} // namespace mirakana::runtime
