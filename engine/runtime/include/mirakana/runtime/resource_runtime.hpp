// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/asset_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mirakana::runtime {

struct RuntimeResourceHandleV2 {
    std::uint32_t index{0};
    std::uint32_t generation{0};

    friend bool operator==(RuntimeResourceHandleV2 lhs, RuntimeResourceHandleV2 rhs) noexcept {
        return lhs.index == rhs.index && lhs.generation == rhs.generation;
    }
};

struct RuntimeResourceRecordV2 {
    RuntimeResourceHandleV2 handle;
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    RuntimeAssetHandle package_handle;
    std::string path;
    std::uint64_t content_hash{0};
    std::uint64_t source_revision{0};
};

struct RuntimeResourceCatalogBuildDiagnosticV2 {
    AssetId asset;
    std::string diagnostic;
};

struct RuntimeResourceCatalogBuildResultV2 {
    std::vector<RuntimeResourceCatalogBuildDiagnosticV2> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

class RuntimeResourceCatalogV2 {
  public:
    [[nodiscard]] const std::vector<RuntimeResourceRecordV2>& records() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;

  private:
    friend RuntimeResourceCatalogBuildResultV2 build_runtime_resource_catalog_v2(RuntimeResourceCatalogV2& catalog,
                                                                                 const RuntimeAssetPackage& package);

    std::vector<RuntimeResourceRecordV2> records_;
    std::uint32_t generation_{0};
};

[[nodiscard]] RuntimeResourceCatalogBuildResultV2 build_runtime_resource_catalog_v2(RuntimeResourceCatalogV2& catalog,
                                                                                    const RuntimeAssetPackage& package);

/// Rebuilds `catalog` from an ordered list of resident runtime packages using explicit overlay semantics.
/// This is the narrow "multi-mount resident view" entrypoint: it does not load filesystem data, stage safe-point
/// replacements, or integrate streaming or RHI teardown.
[[nodiscard]] RuntimeResourceCatalogBuildResultV2
build_runtime_resource_catalog_v2_from_resident_mounts(RuntimeResourceCatalogV2& catalog,
                                                       const std::vector<RuntimeAssetPackage>& mounts,
                                                       RuntimePackageMountOverlay overlay);

struct RuntimeResidentPackageMountIdV2 {
    std::uint32_t value{0};

    friend bool operator==(RuntimeResidentPackageMountIdV2 lhs, RuntimeResidentPackageMountIdV2 rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

struct RuntimeResidentPackageMountRecordV2 {
    RuntimeResidentPackageMountIdV2 id;
    std::string label;
    RuntimeAssetPackage package;
};

enum class RuntimeResidentPackageMountStatusV2 : std::uint8_t {
    mounted = 0,
    unmounted,
    invalid_mount_id,
    duplicate_mount_id,
    missing_mount_id,
};

struct RuntimeResidentPackageMountDiagnosticV2 {
    RuntimeResidentPackageMountIdV2 mount;
    std::string code;
    std::string message;
};

struct RuntimeResidentPackageMountResultV2 {
    RuntimeResidentPackageMountStatusV2 status{RuntimeResidentPackageMountStatusV2::invalid_mount_id};
    RuntimeResidentPackageMountDiagnosticV2 diagnostic;

    [[nodiscard]] bool succeeded() const noexcept;
};

class RuntimeResidentPackageMountSetV2 {
  public:
    [[nodiscard]] const std::vector<RuntimeResidentPackageMountRecordV2>& mounts() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;

    [[nodiscard]] RuntimeResidentPackageMountResultV2 mount(RuntimeResidentPackageMountRecordV2 record);
    [[nodiscard]] RuntimeResidentPackageMountResultV2 unmount(RuntimeResidentPackageMountIdV2 id);

  private:
    friend struct RuntimeResidentPackageMountSetReplaceAccessV2;

    std::vector<RuntimeResidentPackageMountRecordV2> mounts_;
    std::uint32_t generation_{0};
};

struct RuntimeResidentPackageMountCatalogBuildResultV2 {
    RuntimeResourceCatalogBuildResultV2 catalog_build;
    std::size_t mounted_package_count{0};
    std::uint32_t mount_generation{0};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Rebuilds `catalog` from the explicit resident package mount set. The mount set owns already-loaded package values;
/// this helper does not load from disk, stream in the background, evict resources, or touch renderer/RHI ownership.
[[nodiscard]] RuntimeResidentPackageMountCatalogBuildResultV2
build_runtime_resource_catalog_v2_from_resident_mount_set(RuntimeResourceCatalogV2& catalog,
                                                          const RuntimeResidentPackageMountSetV2& mount_set,
                                                          RuntimePackageMountOverlay overlay);

struct RuntimeResourceResidencyBudgetDiagnosticV2 {
    std::string code;
    std::string message;
};

/// Explicit optional caps for resident runtime package views. Unset fields impose no limit on that axis.
struct RuntimeResourceResidencyBudgetV2 {
    std::optional<std::uint64_t> max_resident_content_bytes;
    std::optional<std::size_t> max_resident_asset_records;
};

struct RuntimeResourceResidencyBudgetExecutionResultV2 {
    bool within_budget{true};
    std::uint64_t estimated_resident_content_bytes{0};
    std::size_t resident_asset_record_count{0};
    std::vector<RuntimeResourceResidencyBudgetDiagnosticV2> diagnostics;
};

/// Evaluates merged or single-package resident view against optional byte and record caps (no eviction).
[[nodiscard]] RuntimeResourceResidencyBudgetExecutionResultV2
evaluate_runtime_resource_residency_budget(const RuntimeAssetPackage& resident_package_view,
                                           const RuntimeResourceResidencyBudgetV2& budget);

struct RuntimeResidentMountCatalogBudgetBundleResult {
    RuntimeResourceResidencyBudgetExecutionResultV2 budget_execution{};
    RuntimeResourceCatalogBuildResultV2 catalog_build{};
    bool invoked_catalog_build{false};

    [[nodiscard]] bool ok() const noexcept {
        return budget_execution.within_budget && invoked_catalog_build && catalog_build.succeeded();
    }
};

/// Merges mounts, enforces the residency budget on the merged view, then rebuilds `catalog` only when the budget
/// passes. On budget failure, `catalog` is left unchanged and `invoked_catalog_build` is false.
[[nodiscard]] RuntimeResidentMountCatalogBudgetBundleResult
build_runtime_resource_catalog_v2_from_resident_mounts_with_budget(RuntimeResourceCatalogV2& catalog,
                                                                   const std::vector<RuntimeAssetPackage>& mounts,
                                                                   RuntimePackageMountOverlay overlay,
                                                                   const RuntimeResourceResidencyBudgetV2& budget);

enum class RuntimeResidentCatalogCacheStatusV2 : std::uint8_t {
    rebuilt = 0,
    cache_hit,
    budget_failed,
    catalog_build_failed,
};

struct RuntimeResidentCatalogCacheRefreshResultV2 {
    RuntimeResidentCatalogCacheStatusV2 status{RuntimeResidentCatalogCacheStatusV2::catalog_build_failed};
    RuntimeResourceResidencyBudgetExecutionResultV2 budget_execution{};
    RuntimeResourceCatalogBuildResultV2 catalog_build{};
    std::size_t mounted_package_count{0};
    std::uint32_t mount_generation{0};
    std::uint32_t previous_catalog_generation{0};
    std::uint32_t catalog_generation{0};
    bool invoked_catalog_build{false};
    bool reused_cache{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Host-independent resident catalog cache for an explicit loaded-package mount set.
/// The cache rebuilds only when the mount generation, overlay policy, or residency budget changes. It does not load
/// packages, stream in the background, evict resources, enforce GPU budgets, or own renderer/RHI resources.
class RuntimeResidentCatalogCacheV2 {
  public:
    [[nodiscard]] const RuntimeResourceCatalogV2& catalog() const noexcept;
    [[nodiscard]] bool has_value() const noexcept;
    [[nodiscard]] std::uint32_t cached_mount_generation() const noexcept;

    [[nodiscard]] RuntimeResidentCatalogCacheRefreshResultV2 refresh(const RuntimeResidentPackageMountSetV2& mount_set,
                                                                     RuntimePackageMountOverlay overlay,
                                                                     const RuntimeResourceResidencyBudgetV2& budget);

    void clear();

  private:
    RuntimeResourceCatalogV2 catalog_;
    RuntimeResourceResidencyBudgetExecutionResultV2 cached_budget_execution_{};
    RuntimeResourceResidencyBudgetV2 cached_budget_{};
    RuntimePackageMountOverlay cached_overlay_{RuntimePackageMountOverlay::first_mount_wins};
    std::uint32_t cached_mount_generation_{0};
    bool has_cache_{false};
};

enum class RuntimeResidentPackageUnmountCommitStatusV2 : std::uint8_t {
    unmounted = 0,
    invalid_mount_id,
    missing_mount_id,
    budget_failed,
    catalog_build_failed,
};

struct RuntimeResidentPackageUnmountCommitResultV2 {
    RuntimeResidentPackageUnmountCommitStatusV2 status{RuntimeResidentPackageUnmountCommitStatusV2::missing_mount_id};
    RuntimeResidentPackageMountResultV2 unmount;
    RuntimeResidentCatalogCacheRefreshResultV2 catalog_refresh;
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Removes one explicit resident package mount and refreshes the resident catalog cache as one safe-point-style
/// operation. Budget and catalog preflight run on the projected remaining mount set before live mutation.
[[nodiscard]] RuntimeResidentPackageUnmountCommitResultV2
commit_runtime_resident_package_unmount_v2(RuntimeResidentPackageMountSetV2& mount_set,
                                           RuntimeResidentCatalogCacheV2& catalog_cache,
                                           RuntimeResidentPackageMountIdV2 id, RuntimePackageMountOverlay overlay,
                                           const RuntimeResourceResidencyBudgetV2& budget);

enum class RuntimeResidentPackageEvictionPlanStatusV2 : std::uint8_t {
    no_eviction_required = 0,
    planned,
    invalid_candidate_mount_id,
    duplicate_candidate_mount_id,
    missing_candidate_mount_id,
    protected_candidate_mount_id,
    budget_unreachable,
    catalog_build_failed,
};

struct RuntimeResidentPackageEvictionPlanDiagnosticV2 {
    RuntimeResidentPackageMountIdV2 mount;
    std::string code;
    std::string message;
};

struct RuntimeResidentPackageEvictionPlanDescV2 {
    RuntimeResourceResidencyBudgetV2 target_budget;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    std::vector<RuntimeResidentPackageMountIdV2> candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids;
};

struct RuntimeResidentPackageEvictionPlanStepV2 {
    RuntimeResidentPackageMountIdV2 mount_id;
    RuntimeResidentPackageMountResultV2 unmount;
    RuntimeResidentCatalogCacheRefreshResultV2 catalog_refresh;
};

struct RuntimeResidentPackageEvictionPlanResultV2 {
    RuntimeResidentPackageEvictionPlanStatusV2 status{RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable};
    RuntimeResidentCatalogCacheRefreshResultV2 current_refresh;
    RuntimeResidentCatalogCacheRefreshResultV2 projected_refresh;
    std::vector<RuntimeResidentPackageEvictionPlanStepV2> steps;
    std::vector<RuntimeResidentPackageEvictionPlanDiagnosticV2> diagnostics;
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t projected_mount_count{0};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans reviewed resident package removals from an explicit candidate order until the projected resident view fits
/// the target budget. This is a pure planning helper: it does not mutate the live mount set, load packages, discover
/// files, infer LRU policy, enforce GPU budgets, or touch renderer/RHI resources.
[[nodiscard]] RuntimeResidentPackageEvictionPlanResultV2
plan_runtime_resident_package_evictions_v2(const RuntimeResidentPackageMountSetV2& mount_set,
                                           const RuntimeResidentPackageEvictionPlanDescV2& desc);

enum class RuntimeResidentPackageReplaceCommitStatusV2 : std::uint8_t {
    replaced = 0,
    invalid_mount_id,
    missing_mount_id,
    budget_failed,
    catalog_build_failed,
};

struct RuntimeResidentPackageReplaceCommitResultV2 {
    RuntimeResidentPackageReplaceCommitStatusV2 status{RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id};
    RuntimeResidentPackageMountDiagnosticV2 diagnostic;
    RuntimeResourceCatalogBuildResultV2 candidate_catalog_build;
    RuntimeResidentCatalogCacheRefreshResultV2 catalog_refresh;
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t mounted_package_count{0};
    bool invoked_candidate_catalog_build{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Replaces one existing explicit resident package mount while preserving its mount slot/order, then refreshes the
/// resident catalog cache as one safe-point-style operation. Candidate package validation, budget checks, and catalog
/// refresh run on projected state before live mutation.
[[nodiscard]] RuntimeResidentPackageReplaceCommitResultV2 commit_runtime_resident_package_replace_v2(
    RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 id, RuntimeAssetPackage replacement_package, RuntimePackageMountOverlay overlay,
    const RuntimeResourceResidencyBudgetV2& budget);

[[nodiscard]] std::optional<RuntimeResourceHandleV2> find_runtime_resource_v2(const RuntimeResourceCatalogV2& catalog,
                                                                              AssetId asset) noexcept;
[[nodiscard]] bool is_runtime_resource_handle_live_v2(const RuntimeResourceCatalogV2& catalog,
                                                      RuntimeResourceHandleV2 handle) noexcept;
[[nodiscard]] const RuntimeResourceRecordV2* runtime_resource_record_v2(const RuntimeResourceCatalogV2& catalog,
                                                                        RuntimeResourceHandleV2 handle) noexcept;

enum class RuntimePackageSafePointReplacementStatus : std::uint8_t {
    no_pending_package = 0,
    catalog_build_failed,
    committed,
};

struct RuntimePackageSafePointReplacementResult {
    RuntimePackageSafePointReplacementStatus status{RuntimePackageSafePointReplacementStatus::no_pending_package};
    std::size_t previous_record_count{0};
    std::size_t committed_record_count{0};
    std::uint32_t previous_generation{0};
    std::uint32_t committed_generation{0};
    std::size_t stale_handle_count{0};
    std::vector<RuntimeResourceCatalogBuildDiagnosticV2> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimePackageSafePointReplacementResult
commit_runtime_package_safe_point_replacement(RuntimeAssetPackageStore& store, RuntimeResourceCatalogV2& catalog);

enum class RuntimePackageSafePointUnloadStatus : std::uint8_t {
    no_active_package = 0,
    unloaded,
};

struct RuntimePackageSafePointUnloadResult {
    RuntimePackageSafePointUnloadStatus status{RuntimePackageSafePointUnloadStatus::no_active_package};
    std::size_t previous_record_count{0};
    std::size_t committed_record_count{0};
    std::uint32_t previous_generation{0};
    std::uint32_t committed_generation{0};
    std::size_t stale_handle_count{0};
    bool discarded_pending_package{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimePackageSafePointUnloadResult
commit_runtime_package_safe_point_unload(RuntimeAssetPackageStore& store, RuntimeResourceCatalogV2& catalog);

} // namespace mirakana::runtime
