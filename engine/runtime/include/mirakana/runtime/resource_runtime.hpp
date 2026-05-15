// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/asset_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

enum class RuntimePackageIndexDiscoveryStatusV2 : std::uint8_t {
    discovered = 0,
    no_candidates,
    invalid_descriptor,
    missing_root,
    scan_failed,
};

struct RuntimePackageIndexDiscoveryDescV2 {
    std::string root;
    std::string content_root;
};

struct RuntimePackageIndexDiscoveryCandidateV2 {
    std::string package_index_path;
    std::string content_root;
    std::string label;
};

struct RuntimePackageIndexDiscoveryDiagnosticV2 {
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageIndexDiscoveryResultV2 {
    RuntimePackageIndexDiscoveryStatusV2 status{RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor};
    std::string root;
    std::vector<RuntimePackageIndexDiscoveryCandidateV2> candidates;
    std::vector<RuntimePackageIndexDiscoveryDiagnosticV2> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

enum class RuntimePackageCandidateLoadStatusV2 : std::uint8_t {
    loaded = 0,
    invalid_candidate,
    package_load_failed,
    read_failed,
};

struct RuntimePackageCandidateLoadDiagnosticV2 {
    AssetId asset;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageCandidateLoadResultV2 {
    RuntimePackageCandidateLoadStatusV2 status{RuntimePackageCandidateLoadStatusV2::invalid_candidate};
    RuntimePackageIndexDiscoveryCandidateV2 candidate;
    RuntimeAssetPackageDesc package_desc;
    RuntimeAssetPackageLoadResult loaded_package;
    std::vector<RuntimePackageCandidateLoadDiagnosticV2> diagnostics;
    std::size_t loaded_record_count{0};
    std::uint64_t estimated_resident_bytes{0};
    bool invoked_load{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Discovers reviewed cooked package index candidates under a caller-owned VFS root.
/// This helper does not read index contents, load packages, mount resident packages, refresh catalogs, stream in the
/// background, or touch renderer/RHI/native handles. `content_root` is caller-provided and may be empty; discovery does
/// not infer package payload roots from index locations.
[[nodiscard]] RuntimePackageIndexDiscoveryResultV2
discover_runtime_package_indexes_v2(const IFileSystem& filesystem, const RuntimePackageIndexDiscoveryDescV2& desc);

/// Loads one reviewed package index discovery candidate into a typed package load result.
/// This helper validates the selected candidate again, converts loader failures/exceptions into diagnostics, and does
/// not stage, mount, refresh resident catalogs, stream in the background, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageCandidateLoadResultV2
load_runtime_package_candidate_v2(IFileSystem& filesystem, const RuntimePackageIndexDiscoveryCandidateV2& candidate);

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

enum class RuntimePackageCandidateResidentMountStatusV2 : std::uint8_t {
    mounted = 0,
    invalid_mount_id,
    duplicate_mount_id,
    candidate_load_failed,
    budget_failed,
    catalog_refresh_failed,
};

enum class RuntimePackageCandidateResidentMountDiagnosticPhaseV2 : std::uint8_t {
    resident_mount = 0,
    candidate_load,
    resident_budget,
    catalog_refresh,
};

struct RuntimePackageCandidateResidentMountDescV2 {
    RuntimePackageIndexDiscoveryCandidateV2 candidate;
    RuntimeResidentPackageMountIdV2 mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudgetV2 budget{};
};

struct RuntimePackageCandidateResidentMountDiagnosticV2 {
    RuntimePackageCandidateResidentMountDiagnosticPhaseV2 phase{
        RuntimePackageCandidateResidentMountDiagnosticPhaseV2::candidate_load};
    AssetId asset;
    RuntimeResidentPackageMountIdV2 mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageCandidateResidentMountResultV2 {
    RuntimePackageCandidateResidentMountStatusV2 status{
        RuntimePackageCandidateResidentMountStatusV2::candidate_load_failed};
    RuntimePackageIndexDiscoveryCandidateV2 candidate;
    RuntimeAssetPackageDesc package_desc;
    RuntimePackageCandidateLoadResultV2 candidate_load;
    RuntimeResidentPackageMountResultV2 resident_mount;
    RuntimeResidentCatalogCacheRefreshResultV2 catalog_refresh;
    std::vector<RuntimePackageCandidateResidentMountDiagnosticV2> diagnostics;
    std::size_t loaded_record_count{0};
    std::uint64_t loaded_resident_bytes{0};
    std::uint64_t projected_resident_bytes{0};
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};
    bool invoked_candidate_load{false};
    bool invoked_catalog_refresh{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Loads one reviewed package-index candidate and commits it into an explicit resident mount/cache view.
/// Mount id preflight happens before filesystem reads, and live mount/cache state changes only after projected catalog
/// refresh succeeds. This helper does not use package-streaming descriptors, background workers, hot reload,
/// upload/staging, renderer/RHI ownership, or native handles.
[[nodiscard]] RuntimePackageCandidateResidentMountResultV2
commit_runtime_package_candidate_resident_mount_v2(IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set,
                                                   RuntimeResidentCatalogCacheV2& catalog_cache,
                                                   const RuntimePackageCandidateResidentMountDescV2& desc);

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

enum class RuntimePackageCandidateResidentReplaceStatusV2 : std::uint8_t {
    replaced = 0,
    invalid_mount_id,
    missing_mount_id,
    candidate_load_failed,
    resident_replace_failed,
    budget_failed,
    catalog_refresh_failed,
};

enum class RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2 : std::uint8_t {
    resident_replace = 0,
    candidate_load,
    candidate_catalog,
    resident_budget,
    catalog_refresh,
};

struct RuntimePackageCandidateResidentReplaceDescV2 {
    RuntimePackageIndexDiscoveryCandidateV2 candidate;
    RuntimeResidentPackageMountIdV2 mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudgetV2 budget{};
};

struct RuntimePackageCandidateResidentReplaceDiagnosticV2 {
    RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2 phase{
        RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::candidate_load};
    AssetId asset;
    RuntimeResidentPackageMountIdV2 mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageCandidateResidentReplaceResultV2 {
    RuntimePackageCandidateResidentReplaceStatusV2 status{
        RuntimePackageCandidateResidentReplaceStatusV2::candidate_load_failed};
    RuntimePackageIndexDiscoveryCandidateV2 candidate;
    RuntimeAssetPackageDesc package_desc;
    RuntimePackageCandidateLoadResultV2 candidate_load;
    RuntimeResidentPackageReplaceCommitResultV2 resident_replace;
    RuntimeResidentCatalogCacheRefreshResultV2 catalog_refresh;
    std::vector<RuntimePackageCandidateResidentReplaceDiagnosticV2> diagnostics;
    std::size_t loaded_record_count{0};
    std::uint64_t loaded_resident_bytes{0};
    std::uint64_t projected_resident_bytes{0};
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};
    bool invoked_candidate_load{false};
    bool invoked_resident_replace{false};
    bool invoked_catalog_refresh{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Loads one reviewed package-index candidate and replaces one existing explicit resident mount/cache view.
/// Mount id preflight happens before filesystem reads, and live mount/cache state changes only after the delegated
/// resident replacement path succeeds on projected state. This helper does not use package-streaming descriptors,
/// background workers, hot reload, upload/staging, renderer/RHI ownership, or native handles.
[[nodiscard]] RuntimePackageCandidateResidentReplaceResultV2 commit_runtime_package_candidate_resident_replace_v2(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    const RuntimePackageCandidateResidentReplaceDescV2& desc);

enum class RuntimePackageDiscoveryResidentCommitModeV2 : std::uint8_t {
    mount = 0,
    replace,
};

enum class RuntimePackageDiscoveryResidentCommitStatusV2 : std::uint8_t {
    committed = 0,
    invalid_descriptor,
    invalid_mount_id,
    duplicate_mount_id,
    missing_mount_id,
    discovery_failed,
    candidate_not_found,
    candidate_load_failed,
    budget_failed,
    catalog_refresh_failed,
    resident_commit_failed,
};

enum class RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2 : std::uint8_t {
    descriptor = 0,
    resident_mount,
    resident_replace,
    discovery,
    candidate_selection,
    candidate_load,
    resident_budget,
    catalog_refresh,
    resident_commit,
};

struct RuntimePackageDiscoveryResidentCommitDescV2 {
    RuntimePackageIndexDiscoveryDescV2 discovery;
    std::string selected_package_index_path;
    RuntimePackageDiscoveryResidentCommitModeV2 mode{RuntimePackageDiscoveryResidentCommitModeV2::mount};
    RuntimeResidentPackageMountIdV2 mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudgetV2 budget{};
};

struct RuntimePackageDiscoveryResidentCommitDiagnosticV2 {
    RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2 phase{
        RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::candidate_selection};
    RuntimeResidentPackageMountIdV2 mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageDiscoveryResidentCommitResultV2 {
    RuntimePackageDiscoveryResidentCommitStatusV2 status{
        RuntimePackageDiscoveryResidentCommitStatusV2::invalid_descriptor};
    RuntimePackageIndexDiscoveryResultV2 discovery;
    RuntimePackageIndexDiscoveryCandidateV2 selected_candidate;
    RuntimePackageCandidateResidentMountResultV2 resident_mount;
    RuntimePackageCandidateResidentReplaceResultV2 resident_replace;
    RuntimeResidentCatalogCacheRefreshResultV2 catalog_refresh;
    std::vector<RuntimePackageDiscoveryResidentCommitDiagnosticV2> diagnostics;
    std::size_t loaded_record_count{0};
    std::uint64_t loaded_resident_bytes{0};
    std::uint64_t projected_resident_bytes{0};
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};
    bool invoked_discovery{false};
    bool invoked_resident_commit{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Discovers reviewed package-index candidates from a caller-owned VFS root, selects one exact package index path,
/// then delegates to the explicit candidate resident mount or replace commit helper. Resident operation preflight
/// happens before discovery/package reads, and live mount/cache state changes only after the delegated copy-then-commit
/// path succeeds. This helper does not stream in the background, infer content roots, apply eviction policy, hot
/// reload, upload/stage renderer resources, enforce GPU budgets, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageDiscoveryResidentCommitResultV2
commit_runtime_package_discovery_resident_v2(IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set,
                                             RuntimeResidentCatalogCacheV2& catalog_cache,
                                             const RuntimePackageDiscoveryResidentCommitDescV2& desc);

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
