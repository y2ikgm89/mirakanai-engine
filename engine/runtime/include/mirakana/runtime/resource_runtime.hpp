// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_hot_reload.hpp"
#include "mirakana/runtime/asset_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

enum class RuntimePackageIndexDiscoveryStatus : std::uint8_t {
    discovered = 0,
    no_candidates,
    invalid_descriptor,
    missing_root,
    scan_failed,
};

struct RuntimePackageIndexDiscoveryDesc {
    std::string root;
    std::string content_root;
};

struct RuntimePackageIndexDiscoveryCandidate {
    std::string package_index_path;
    std::string content_root;
    std::string label;
};

struct RuntimePackageIndexDiscoveryDiagnostic {
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageIndexDiscoveryResult {
    RuntimePackageIndexDiscoveryStatus status{RuntimePackageIndexDiscoveryStatus::invalid_descriptor};
    std::string root;
    std::vector<RuntimePackageIndexDiscoveryCandidate> candidates;
    std::vector<RuntimePackageIndexDiscoveryDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

enum class RuntimePackageCandidateLoadStatus : std::uint8_t {
    loaded = 0,
    invalid_candidate,
    package_load_failed,
    read_failed,
};

struct RuntimePackageCandidateLoadDiagnostic {
    AssetId asset;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageCandidateLoadResult {
    RuntimePackageCandidateLoadStatus status{RuntimePackageCandidateLoadStatus::invalid_candidate};
    RuntimePackageIndexDiscoveryCandidate candidate;
    RuntimeAssetPackageDesc package_desc;
    RuntimeAssetPackageLoadResult loaded_package;
    std::vector<RuntimePackageCandidateLoadDiagnostic> diagnostics;
    std::size_t loaded_record_count{0};
    std::uint64_t estimated_resident_bytes{0};
    bool invoked_load{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Discovers reviewed cooked package index candidates under a caller-owned VFS root.
/// This helper does not read index contents, load packages, mount resident packages, refresh catalogs, stream in the
/// background, or touch renderer/RHI/native handles. `content_root` is caller-provided and may be empty; discovery does
/// not infer package payload roots from index locations.
[[nodiscard]] RuntimePackageIndexDiscoveryResult
discover_runtime_package_indexes(const IFileSystem& filesystem, const RuntimePackageIndexDiscoveryDesc& desc);

/// Loads one reviewed package index discovery candidate into a typed package load result.
/// This helper validates the selected candidate again, converts loader failures/exceptions into diagnostics, and does
/// not stage, mount, refresh resident catalogs, stream in the background, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageCandidateLoadResult
load_runtime_package_candidate(IFileSystem& filesystem, const RuntimePackageIndexDiscoveryCandidate& candidate);

enum class RuntimePackageHotReloadCandidateReviewStatus : std::uint8_t {
    review_ready = 0,
    no_changes,
    no_candidates,
    no_matches,
};

enum class RuntimePackageHotReloadCandidateReviewMatchKind : std::uint8_t {
    package_index = 0,
    content,
};

struct RuntimePackageHotReloadCandidateReviewDesc {
    std::vector<std::string> changed_paths;
    std::vector<RuntimePackageIndexDiscoveryCandidate> candidates;
};

struct RuntimePackageHotReloadCandidateReviewChange {
    std::string path;
    RuntimePackageHotReloadCandidateReviewMatchKind kind{
        RuntimePackageHotReloadCandidateReviewMatchKind::package_index};
};

struct RuntimePackageHotReloadCandidateReviewRow {
    RuntimePackageIndexDiscoveryCandidate candidate;
    std::vector<RuntimePackageHotReloadCandidateReviewChange> matched_changes;
};

struct RuntimePackageHotReloadCandidateReviewDiagnostic {
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageHotReloadCandidateReviewResult {
    RuntimePackageHotReloadCandidateReviewStatus status{RuntimePackageHotReloadCandidateReviewStatus::no_matches};
    std::vector<RuntimePackageHotReloadCandidateReviewRow> rows;
    std::vector<RuntimePackageHotReloadCandidateReviewDiagnostic> diagnostics;
    std::size_t changed_path_count{0};
    std::size_t matched_changed_path_count{0};
    std::size_t invalid_changed_path_count{0};
    std::size_t unmatched_changed_path_count{0};
    std::size_t review_candidate_count{0};
    bool invoked_package_load{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans exact package-index candidates a host should review after already-reviewed package/index/content path changes.
/// This pure planner consumes caller-provided changed relative VFS paths and already-discovered candidate rows only; it
/// does not read files, load packages, infer content roots, mutate resident state, watch files, recook assets, stream
/// in the background, apply eviction policy, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageHotReloadCandidateReviewResult
plan_runtime_package_hot_reload_candidate_review(const RuntimePackageHotReloadCandidateReviewDesc& desc);

enum class RuntimePackageHotReloadRecookChangeReviewStatus : std::uint8_t {
    review_ready = 0,
    no_recook_changes,
    invalid_recook_apply_result,
    failed_recook_apply_result,
    candidate_review_failed,
};

enum class RuntimePackageHotReloadRecookChangeReviewDiagnosticPhase : std::uint8_t {
    recook_apply_result = 0,
    candidate_review,
};

struct RuntimePackageHotReloadRecookChangeReviewDesc {
    std::vector<AssetHotReloadApplyResult> recook_apply_results;
    std::vector<RuntimePackageIndexDiscoveryCandidate> candidates;
};

struct RuntimePackageHotReloadRecookChangeReviewDiagnostic {
    RuntimePackageHotReloadRecookChangeReviewDiagnosticPhase phase{
        RuntimePackageHotReloadRecookChangeReviewDiagnosticPhase::recook_apply_result};
    AssetId asset;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageHotReloadRecookChangeReviewResult {
    RuntimePackageHotReloadRecookChangeReviewStatus status{
        RuntimePackageHotReloadRecookChangeReviewStatus::no_recook_changes};
    RuntimePackageHotReloadCandidateReviewDesc candidate_review_desc;
    RuntimePackageHotReloadCandidateReviewResult candidate_review;
    std::vector<RuntimePackageHotReloadRecookChangeReviewDiagnostic> diagnostics;
    std::size_t recook_apply_result_count{0};
    std::size_t accepted_recook_change_count{0};
    std::size_t staged_recook_change_count{0};
    std::size_t applied_recook_change_count{0};
    std::size_t failed_recook_apply_result_count{0};
    std::size_t invalid_recook_apply_result_count{0};
    bool invoked_candidate_review{false};
    bool invoked_file_watch{false};
    bool invoked_recook{false};
    bool invoked_package_load{false};
    bool invoked_resident_commit{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Converts reviewed recook apply-result rows into hot-reload candidate-review rows.
/// `AssetHotReloadApplyResult::path` is treated as a caller-reviewed runtime package/index/content relative VFS path,
/// not as a source-asset path. This pure planner validates apply rows, delegates exact package-index/content matching
/// to `plan_runtime_package_hot_reload_candidate_review`, and does not watch files, run recook, read packages,
/// mutate resident state, stream in the background, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageHotReloadRecookChangeReviewResult
plan_runtime_package_hot_reload_recook_change_review(const RuntimePackageHotReloadRecookChangeReviewDesc& desc);

struct RuntimeResourceHandle {
    std::uint32_t index{0};
    std::uint32_t generation{0};

    friend bool operator==(RuntimeResourceHandle lhs, RuntimeResourceHandle rhs) noexcept {
        return lhs.index == rhs.index && lhs.generation == rhs.generation;
    }
};

struct RuntimeResourceRecord {
    RuntimeResourceHandle handle;
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    RuntimeAssetHandle package_handle;
    std::string path;
    std::uint64_t content_hash{0};
    std::uint64_t source_revision{0};
};

struct RuntimeResourceCatalogBuildDiagnostic {
    AssetId asset;
    std::string diagnostic;
};

struct RuntimeResourceCatalogBuildResult {
    std::vector<RuntimeResourceCatalogBuildDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

class RuntimeResourceCatalog {
  public:
    [[nodiscard]] const std::vector<RuntimeResourceRecord>& records() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;

  private:
    friend RuntimeResourceCatalogBuildResult build_runtime_resource_catalog(RuntimeResourceCatalog& catalog,
                                                                            const RuntimeAssetPackage& package);

    std::vector<RuntimeResourceRecord> records_;
    std::uint32_t generation_{0};
};

[[nodiscard]] RuntimeResourceCatalogBuildResult build_runtime_resource_catalog(RuntimeResourceCatalog& catalog,
                                                                               const RuntimeAssetPackage& package);

/// Rebuilds `catalog` from an ordered list of resident runtime packages using explicit overlay semantics.
/// This is the narrow "multi-mount resident view" entrypoint: it does not load filesystem data, stage safe-point
/// replacements, or integrate streaming or RHI teardown.
[[nodiscard]] RuntimeResourceCatalogBuildResult
build_runtime_resource_catalog_from_resident_mounts(RuntimeResourceCatalog& catalog,
                                                    const std::vector<RuntimeAssetPackage>& mounts,
                                                    RuntimePackageMountOverlay overlay);

struct RuntimeResidentPackageMountId {
    std::uint32_t value{0};

    friend bool operator==(RuntimeResidentPackageMountId lhs, RuntimeResidentPackageMountId rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

struct RuntimeResidentPackageMountRecord {
    RuntimeResidentPackageMountId id;
    std::string label;
    RuntimeAssetPackage package;
};

enum class RuntimeResidentPackageMountStatus : std::uint8_t {
    mounted = 0,
    unmounted,
    invalid_mount_id,
    duplicate_mount_id,
    missing_mount_id,
};

struct RuntimeResidentPackageMountDiagnostic {
    RuntimeResidentPackageMountId mount;
    std::string code;
    std::string message;
};

struct RuntimeResidentPackageMountResult {
    RuntimeResidentPackageMountStatus status{RuntimeResidentPackageMountStatus::invalid_mount_id};
    RuntimeResidentPackageMountDiagnostic diagnostic;

    [[nodiscard]] bool succeeded() const noexcept;
};

class RuntimeResidentPackageMountSet {
  public:
    [[nodiscard]] const std::vector<RuntimeResidentPackageMountRecord>& mounts() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;

    [[nodiscard]] RuntimeResidentPackageMountResult mount(RuntimeResidentPackageMountRecord record);
    [[nodiscard]] RuntimeResidentPackageMountResult unmount(RuntimeResidentPackageMountId id);

  private:
    friend struct RuntimeResidentPackageMountSetReplaceAccess;

    std::vector<RuntimeResidentPackageMountRecord> mounts_;
    std::uint32_t generation_{0};
};

struct RuntimeResidentPackageMountCatalogBuildResult {
    RuntimeResourceCatalogBuildResult catalog_build;
    std::size_t mounted_package_count{0};
    std::uint32_t mount_generation{0};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Rebuilds `catalog` from the explicit resident package mount set. The mount set owns already-loaded package values;
/// this helper does not load from disk, stream in the background, evict resources, or touch renderer/RHI ownership.
[[nodiscard]] RuntimeResidentPackageMountCatalogBuildResult
build_runtime_resource_catalog_from_resident_mount_set(RuntimeResourceCatalog& catalog,
                                                       const RuntimeResidentPackageMountSet& mount_set,
                                                       RuntimePackageMountOverlay overlay);

struct RuntimeResourceResidencyBudgetDiagnostic {
    std::string code;
    std::string message;
};

/// Explicit optional caps for resident runtime package views. Unset fields impose no limit on that axis.
struct RuntimeResourceResidencyBudget {
    std::optional<std::uint64_t> max_resident_content_bytes;
    std::optional<std::size_t> max_resident_asset_records;
};

struct RuntimeResourceResidencyBudgetExecutionResult {
    bool within_budget{true};
    std::uint64_t estimated_resident_content_bytes{0};
    std::size_t resident_asset_record_count{0};
    std::vector<RuntimeResourceResidencyBudgetDiagnostic> diagnostics;
};

/// Evaluates merged or single-package resident view against optional byte and record caps (no eviction).
[[nodiscard]] RuntimeResourceResidencyBudgetExecutionResult
evaluate_runtime_resource_residency_budget(const RuntimeAssetPackage& resident_package_view,
                                           const RuntimeResourceResidencyBudget& budget);

struct RuntimeResidentMountCatalogBudgetBundleResult {
    RuntimeResourceResidencyBudgetExecutionResult budget_execution{};
    RuntimeResourceCatalogBuildResult catalog_build{};
    bool invoked_catalog_build{false};

    [[nodiscard]] bool ok() const noexcept {
        return budget_execution.within_budget && invoked_catalog_build && catalog_build.succeeded();
    }
};

/// Merges mounts, enforces the residency budget on the merged view, then rebuilds `catalog` only when the budget
/// passes. On budget failure, `catalog` is left unchanged and `invoked_catalog_build` is false.
[[nodiscard]] RuntimeResidentMountCatalogBudgetBundleResult
build_runtime_resource_catalog_from_resident_mounts_with_budget(RuntimeResourceCatalog& catalog,
                                                                const std::vector<RuntimeAssetPackage>& mounts,
                                                                RuntimePackageMountOverlay overlay,
                                                                const RuntimeResourceResidencyBudget& budget);

enum class RuntimeResidentCatalogCacheStatus : std::uint8_t {
    rebuilt = 0,
    cache_hit,
    budget_failed,
    catalog_build_failed,
};

struct RuntimeResidentCatalogCacheRefreshResult {
    RuntimeResidentCatalogCacheStatus status{RuntimeResidentCatalogCacheStatus::catalog_build_failed};
    RuntimeResourceResidencyBudgetExecutionResult budget_execution{};
    RuntimeResourceCatalogBuildResult catalog_build{};
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
class RuntimeResidentCatalogCache {
  public:
    [[nodiscard]] const RuntimeResourceCatalog& catalog() const noexcept;
    [[nodiscard]] bool has_value() const noexcept;
    [[nodiscard]] std::uint32_t cached_mount_generation() const noexcept;

    [[nodiscard]] RuntimeResidentCatalogCacheRefreshResult refresh(const RuntimeResidentPackageMountSet& mount_set,
                                                                   RuntimePackageMountOverlay overlay,
                                                                   const RuntimeResourceResidencyBudget& budget);

    void clear();

  private:
    RuntimeResourceCatalog catalog_;
    RuntimeResourceResidencyBudgetExecutionResult cached_budget_execution_{};
    RuntimeResourceResidencyBudget cached_budget_{};
    RuntimePackageMountOverlay cached_overlay_{RuntimePackageMountOverlay::first_mount_wins};
    std::uint32_t cached_mount_generation_{0};
    bool has_cache_{false};
};

enum class RuntimePackageCandidateResidentMountStatus : std::uint8_t {
    mounted = 0,
    invalid_mount_id,
    duplicate_mount_id,
    candidate_load_failed,
    budget_failed,
    catalog_refresh_failed,
};

enum class RuntimePackageCandidateResidentMountDiagnosticPhase : std::uint8_t {
    resident_mount = 0,
    candidate_load,
    resident_budget,
    catalog_refresh,
};

struct RuntimePackageCandidateResidentMountDesc {
    RuntimePackageIndexDiscoveryCandidate candidate;
    RuntimeResidentPackageMountId mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudget budget{};
};

struct RuntimePackageCandidateResidentMountDiagnostic {
    RuntimePackageCandidateResidentMountDiagnosticPhase phase{
        RuntimePackageCandidateResidentMountDiagnosticPhase::candidate_load};
    AssetId asset;
    RuntimeResidentPackageMountId mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageCandidateResidentMountResult {
    RuntimePackageCandidateResidentMountStatus status{
        RuntimePackageCandidateResidentMountStatus::candidate_load_failed};
    RuntimePackageIndexDiscoveryCandidate candidate;
    RuntimeAssetPackageDesc package_desc;
    RuntimePackageCandidateLoadResult candidate_load;
    RuntimeResidentPackageMountResult resident_mount;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
    std::vector<RuntimePackageCandidateResidentMountDiagnostic> diagnostics;
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
[[nodiscard]] RuntimePackageCandidateResidentMountResult
commit_runtime_package_candidate_resident_mount(IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set,
                                                RuntimeResidentCatalogCache& catalog_cache,
                                                const RuntimePackageCandidateResidentMountDesc& desc);

enum class RuntimeResidentPackageUnmountCommitStatus : std::uint8_t {
    unmounted = 0,
    invalid_mount_id,
    missing_mount_id,
    budget_failed,
    catalog_build_failed,
};

struct RuntimeResidentPackageUnmountCommitResult {
    RuntimeResidentPackageUnmountCommitStatus status{RuntimeResidentPackageUnmountCommitStatus::missing_mount_id};
    RuntimeResidentPackageMountResult unmount;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Removes one explicit resident package mount and refreshes the resident catalog cache as one safe-point-style
/// operation. Budget and catalog preflight run on the projected remaining mount set before live mutation.
[[nodiscard]] RuntimeResidentPackageUnmountCommitResult commit_runtime_resident_package_unmount(
    RuntimeResidentPackageMountSet& mount_set, RuntimeResidentCatalogCache& catalog_cache,
    RuntimeResidentPackageMountId id, RuntimePackageMountOverlay overlay, const RuntimeResourceResidencyBudget& budget);

enum class RuntimeResidentPackageEvictionPlanStatus : std::uint8_t {
    no_eviction_required = 0,
    planned,
    invalid_candidate_mount_id,
    duplicate_candidate_mount_id,
    missing_candidate_mount_id,
    protected_candidate_mount_id,
    budget_unreachable,
    catalog_build_failed,
};

struct RuntimeResidentPackageEvictionPlanDiagnostic {
    RuntimeResidentPackageMountId mount;
    std::string code;
    std::string message;
};

struct RuntimeResidentPackageEvictionPlanDesc {
    RuntimeResourceResidencyBudget target_budget;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    std::vector<RuntimeResidentPackageMountId> candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountId> protected_mount_ids;
};

struct RuntimeResidentPackageEvictionPlanStep {
    RuntimeResidentPackageMountId mount_id;
    RuntimeResidentPackageMountResult unmount;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
};

struct RuntimeResidentPackageEvictionPlanResult {
    RuntimeResidentPackageEvictionPlanStatus status{RuntimeResidentPackageEvictionPlanStatus::budget_unreachable};
    RuntimeResidentCatalogCacheRefreshResult current_refresh;
    RuntimeResidentCatalogCacheRefreshResult projected_refresh;
    std::vector<RuntimeResidentPackageEvictionPlanStep> steps;
    std::vector<RuntimeResidentPackageEvictionPlanDiagnostic> diagnostics;
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t projected_mount_count{0};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans reviewed resident package removals from an explicit candidate order until the projected resident view fits
/// the target budget. This is a pure planning helper: it does not mutate the live mount set, load packages, discover
/// files, infer LRU policy, enforce GPU budgets, or touch renderer/RHI resources.
[[nodiscard]] RuntimeResidentPackageEvictionPlanResult
plan_runtime_resident_package_evictions(const RuntimeResidentPackageMountSet& mount_set,
                                        const RuntimeResidentPackageEvictionPlanDesc& desc);

enum class RuntimeResidentPackageReviewedEvictionCommitStatus : std::uint8_t {
    committed = 0,
    no_eviction_required,
    invalid_candidate_mount_id,
    duplicate_candidate_mount_id,
    missing_candidate_mount_id,
    protected_candidate_mount_id,
    budget_failed,
    catalog_refresh_failed,
};

enum class RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhase : std::uint8_t {
    eviction_plan = 0,
    resident_budget,
    catalog_refresh,
};

struct RuntimeResidentPackageReviewedEvictionCommitDesc {
    RuntimeResourceResidencyBudget target_budget;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    std::vector<RuntimeResidentPackageMountId> candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountId> protected_mount_ids;
};

struct RuntimeResidentPackageReviewedEvictionCommitDiagnostic {
    RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhase phase{
        RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhase::eviction_plan};
    AssetId asset;
    RuntimeResidentPackageMountId mount;
    std::string code;
    std::string message;
};

struct RuntimeResidentPackageReviewedEvictionCommitResult {
    RuntimeResidentPackageReviewedEvictionCommitStatus status{
        RuntimeResidentPackageReviewedEvictionCommitStatus::budget_failed};
    RuntimeResidentPackageEvictionPlanResult eviction_plan;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
    std::vector<RuntimeResidentPackageReviewedEvictionCommitDiagnostic> diagnostics;
    std::uint64_t projected_resident_bytes{0};
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};
    std::size_t evicted_mount_count{0};
    bool invoked_eviction_plan{false};
    bool invoked_catalog_refresh{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Applies only caller-reviewed resident eviction candidates to a projected mount/cache view and commits the final
/// state atomically. No packages are loaded or discovered, no eviction policy is inferred, and renderer/RHI/native
/// handles are outside this host-independent safe point.
[[nodiscard]] RuntimeResidentPackageReviewedEvictionCommitResult
commit_runtime_resident_package_reviewed_evictions(RuntimeResidentPackageMountSet& mount_set,
                                                   RuntimeResidentCatalogCache& catalog_cache,
                                                   const RuntimeResidentPackageReviewedEvictionCommitDesc& desc);

enum class RuntimePackageCandidateResidentMountReviewedEvictionsStatus : std::uint8_t {
    mounted = 0,
    invalid_mount_id,
    duplicate_mount_id,
    candidate_load_failed,
    invalid_eviction_candidate_mount_id,
    duplicate_eviction_candidate_mount_id,
    missing_eviction_candidate_mount_id,
    protected_eviction_candidate_mount_id,
    budget_failed,
    catalog_refresh_failed,
};

enum class RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase : std::uint8_t {
    resident_mount = 0,
    candidate_load,
    eviction_plan,
    resident_budget,
    catalog_refresh,
};

struct RuntimePackageCandidateResidentMountReviewedEvictionsDesc {
    RuntimePackageIndexDiscoveryCandidate candidate;
    RuntimeResidentPackageMountId mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudget budget{};
    std::vector<RuntimeResidentPackageMountId> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountId> protected_mount_ids;
};

struct RuntimePackageCandidateResidentMountReviewedEvictionsDiagnostic {
    RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase phase{
        RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::candidate_load};
    AssetId asset;
    RuntimeResidentPackageMountId mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageCandidateResidentMountReviewedEvictionsResult {
    RuntimePackageCandidateResidentMountReviewedEvictionsStatus status{
        RuntimePackageCandidateResidentMountReviewedEvictionsStatus::candidate_load_failed};
    RuntimePackageIndexDiscoveryCandidate candidate;
    RuntimeAssetPackageDesc package_desc;
    RuntimePackageCandidateLoadResult candidate_load;
    RuntimeResidentPackageEvictionPlanResult eviction_plan;
    RuntimeResidentPackageMountResult resident_mount;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
    std::vector<RuntimePackageCandidateResidentMountReviewedEvictionsDiagnostic> diagnostics;
    std::size_t loaded_record_count{0};
    std::uint64_t loaded_resident_bytes{0};
    std::uint64_t projected_resident_bytes{0};
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};
    std::size_t evicted_mount_count{0};
    bool invoked_candidate_load{false};
    bool invoked_eviction_plan{false};
    bool invoked_catalog_refresh{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Loads one reviewed package-index candidate, mounts it into a projected resident view, applies only caller-reviewed
/// eviction candidates when the projected view exceeds the target budget, and commits the final mount/cache view
/// atomically. This helper does not infer eviction policy, stream in the background, hot reload, upload/stage renderer
/// resources, enforce GPU budgets, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageCandidateResidentMountReviewedEvictionsResult
commit_runtime_package_candidate_resident_mount_with_reviewed_evictions(
    IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set, RuntimeResidentCatalogCache& catalog_cache,
    const RuntimePackageCandidateResidentMountReviewedEvictionsDesc& desc);

enum class RuntimeResidentPackageReplaceCommitStatus : std::uint8_t {
    replaced = 0,
    invalid_mount_id,
    missing_mount_id,
    budget_failed,
    catalog_build_failed,
};

struct RuntimeResidentPackageReplaceCommitResult {
    RuntimeResidentPackageReplaceCommitStatus status{RuntimeResidentPackageReplaceCommitStatus::missing_mount_id};
    RuntimeResidentPackageMountDiagnostic diagnostic;
    RuntimeResourceCatalogBuildResult candidate_catalog_build;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t mounted_package_count{0};
    bool invoked_candidate_catalog_build{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Replaces one existing explicit resident package mount while preserving its mount slot/order, then refreshes the
/// resident catalog cache as one safe-point-style operation. Candidate package validation, budget checks, and catalog
/// refresh run on projected state before live mutation.
[[nodiscard]] RuntimeResidentPackageReplaceCommitResult
commit_runtime_resident_package_replace(RuntimeResidentPackageMountSet& mount_set,
                                        RuntimeResidentCatalogCache& catalog_cache, RuntimeResidentPackageMountId id,
                                        RuntimeAssetPackage replacement_package, RuntimePackageMountOverlay overlay,
                                        const RuntimeResourceResidencyBudget& budget);

enum class RuntimePackageCandidateResidentReplaceStatus : std::uint8_t {
    replaced = 0,
    invalid_mount_id,
    missing_mount_id,
    candidate_load_failed,
    resident_replace_failed,
    budget_failed,
    catalog_refresh_failed,
};

enum class RuntimePackageCandidateResidentReplaceDiagnosticPhase : std::uint8_t {
    resident_replace = 0,
    candidate_load,
    candidate_catalog,
    resident_budget,
    catalog_refresh,
};

struct RuntimePackageCandidateResidentReplaceDesc {
    RuntimePackageIndexDiscoveryCandidate candidate;
    RuntimeResidentPackageMountId mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudget budget{};
};

struct RuntimePackageCandidateResidentReplaceDiagnostic {
    RuntimePackageCandidateResidentReplaceDiagnosticPhase phase{
        RuntimePackageCandidateResidentReplaceDiagnosticPhase::candidate_load};
    AssetId asset;
    RuntimeResidentPackageMountId mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageCandidateResidentReplaceResult {
    RuntimePackageCandidateResidentReplaceStatus status{
        RuntimePackageCandidateResidentReplaceStatus::candidate_load_failed};
    RuntimePackageIndexDiscoveryCandidate candidate;
    RuntimeAssetPackageDesc package_desc;
    RuntimePackageCandidateLoadResult candidate_load;
    RuntimeResidentPackageReplaceCommitResult resident_replace;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
    std::vector<RuntimePackageCandidateResidentReplaceDiagnostic> diagnostics;
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
[[nodiscard]] RuntimePackageCandidateResidentReplaceResult
commit_runtime_package_candidate_resident_replace(IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set,
                                                  RuntimeResidentCatalogCache& catalog_cache,
                                                  const RuntimePackageCandidateResidentReplaceDesc& desc);

enum class RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus : std::uint8_t {
    replaced = 0,
    invalid_mount_id,
    missing_mount_id,
    candidate_load_failed,
    resident_replace_failed,
    invalid_eviction_candidate_mount_id,
    duplicate_eviction_candidate_mount_id,
    missing_eviction_candidate_mount_id,
    protected_eviction_candidate_mount_id,
    budget_failed,
    catalog_refresh_failed,
};

enum class RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase : std::uint8_t {
    resident_replace = 0,
    candidate_load,
    candidate_catalog,
    eviction_plan,
    resident_budget,
    catalog_refresh,
};

struct RuntimePackageCandidateResidentReplaceReviewedEvictionsDesc {
    RuntimePackageIndexDiscoveryCandidate candidate;
    RuntimeResidentPackageMountId mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudget budget{};
    std::vector<RuntimeResidentPackageMountId> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountId> protected_mount_ids;
};

struct RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnostic {
    RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase phase{
        RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_load};
    AssetId asset;
    RuntimeResidentPackageMountId mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageCandidateResidentReplaceReviewedEvictionsResult {
    RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus status{
        RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::candidate_load_failed};
    RuntimePackageIndexDiscoveryCandidate candidate;
    RuntimeAssetPackageDesc package_desc;
    RuntimePackageCandidateLoadResult candidate_load;
    RuntimeResidentPackageReplaceCommitResult resident_replace;
    RuntimeResidentPackageEvictionPlanResult eviction_plan;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
    std::vector<RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnostic> diagnostics;
    std::size_t loaded_record_count{0};
    std::uint64_t loaded_resident_bytes{0};
    std::uint64_t projected_resident_bytes{0};
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};
    std::size_t evicted_mount_count{0};
    bool invoked_candidate_load{false};
    bool invoked_resident_replace{false};
    bool invoked_eviction_plan{false};
    bool invoked_catalog_refresh{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Loads one reviewed package-index candidate, replaces one existing explicit resident mount in a projected view,
/// applies only caller-reviewed eviction candidates when the projected view exceeds the target budget, and commits the
/// final mount/cache view atomically. The replacement mount id is always protected from eviction. This helper does not
/// discover packages, infer eviction policy, stream in the background, hot reload, upload/stage renderer resources,
/// enforce GPU budgets, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageCandidateResidentReplaceReviewedEvictionsResult
commit_runtime_package_candidate_resident_replace_with_reviewed_evictions(
    IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set, RuntimeResidentCatalogCache& catalog_cache,
    const RuntimePackageCandidateResidentReplaceReviewedEvictionsDesc& desc);

enum class RuntimePackageDiscoveryResidentCommitMode : std::uint8_t {
    mount = 0,
    replace,
};

enum class RuntimePackageDiscoveryResidentCommitStatus : std::uint8_t {
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

enum class RuntimePackageDiscoveryResidentCommitDiagnosticPhase : std::uint8_t {
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

struct RuntimePackageDiscoveryResidentCommitDesc {
    RuntimePackageIndexDiscoveryDesc discovery;
    std::string selected_package_index_path;
    RuntimePackageDiscoveryResidentCommitMode mode{RuntimePackageDiscoveryResidentCommitMode::mount};
    RuntimeResidentPackageMountId mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudget budget{};
};

struct RuntimePackageDiscoveryResidentCommitDiagnostic {
    RuntimePackageDiscoveryResidentCommitDiagnosticPhase phase{
        RuntimePackageDiscoveryResidentCommitDiagnosticPhase::candidate_selection};
    RuntimeResidentPackageMountId mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageDiscoveryResidentCommitResult {
    RuntimePackageDiscoveryResidentCommitStatus status{RuntimePackageDiscoveryResidentCommitStatus::invalid_descriptor};
    RuntimePackageIndexDiscoveryResult discovery;
    RuntimePackageIndexDiscoveryCandidate selected_candidate;
    RuntimePackageCandidateResidentMountResult resident_mount;
    RuntimePackageCandidateResidentReplaceResult resident_replace;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
    std::vector<RuntimePackageDiscoveryResidentCommitDiagnostic> diagnostics;
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
[[nodiscard]] RuntimePackageDiscoveryResidentCommitResult
commit_runtime_package_discovery_resident(IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set,
                                          RuntimeResidentCatalogCache& catalog_cache,
                                          const RuntimePackageDiscoveryResidentCommitDesc& desc);

enum class RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus : std::uint8_t {
    committed = 0,
    invalid_descriptor,
    invalid_mount_id,
    duplicate_mount_id,
    discovery_failed,
    candidate_not_found,
    candidate_load_failed,
    invalid_eviction_candidate_mount_id,
    duplicate_eviction_candidate_mount_id,
    missing_eviction_candidate_mount_id,
    protected_eviction_candidate_mount_id,
    budget_failed,
    catalog_refresh_failed,
};

enum class RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase : std::uint8_t {
    descriptor = 0,
    resident_mount,
    discovery,
    candidate_selection,
    candidate_load,
    eviction_plan,
    resident_budget,
    catalog_refresh,
    resident_commit,
};

struct RuntimePackageDiscoveryResidentMountReviewedEvictionsDesc {
    RuntimePackageIndexDiscoveryDesc discovery;
    std::string selected_package_index_path;
    RuntimeResidentPackageMountId mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudget budget{};
    std::vector<RuntimeResidentPackageMountId> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountId> protected_mount_ids;
};

struct RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnostic {
    RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase phase{
        RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::candidate_selection};
    AssetId asset;
    RuntimeResidentPackageMountId mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageDiscoveryResidentMountReviewedEvictionsResult {
    RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus status{
        RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::invalid_descriptor};
    RuntimePackageIndexDiscoveryResult discovery;
    RuntimePackageIndexDiscoveryCandidate selected_candidate;
    RuntimePackageCandidateResidentMountReviewedEvictionsResult candidate_resident_mount;
    RuntimeResidentPackageEvictionPlanResult eviction_plan;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
    std::vector<RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnostic> diagnostics;
    std::size_t loaded_record_count{0};
    std::uint64_t loaded_resident_bytes{0};
    std::uint64_t projected_resident_bytes{0};
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};
    std::size_t evicted_mount_count{0};
    bool invoked_discovery{false};
    bool invoked_resident_commit{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Discovers reviewed package-index candidates from a caller-owned VFS root, selects one exact package index path,
/// then delegates to the reviewed eviction-assisted candidate resident mount helper. Descriptor and mount-id preflight
/// happens before discovery/package reads, and live mount/cache state changes only after the delegated copy-then-commit
/// path succeeds. This helper does not infer content roots, stream in the background, choose eviction policy, hot
/// reload, upload/stage renderer resources, enforce GPU budgets, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageDiscoveryResidentMountReviewedEvictionsResult
commit_runtime_package_discovery_resident_mount_with_reviewed_evictions(
    IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set, RuntimeResidentCatalogCache& catalog_cache,
    const RuntimePackageDiscoveryResidentMountReviewedEvictionsDesc& desc);

enum class RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus : std::uint8_t {
    committed = 0,
    invalid_descriptor,
    invalid_mount_id,
    missing_mount_id,
    discovery_failed,
    candidate_not_found,
    candidate_load_failed,
    resident_replace_failed,
    invalid_eviction_candidate_mount_id,
    duplicate_eviction_candidate_mount_id,
    missing_eviction_candidate_mount_id,
    protected_eviction_candidate_mount_id,
    budget_failed,
    catalog_refresh_failed,
};

enum class RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase : std::uint8_t {
    descriptor = 0,
    resident_replace,
    discovery,
    candidate_selection,
    candidate_load,
    candidate_catalog,
    eviction_plan,
    resident_budget,
    catalog_refresh,
    resident_commit,
};

struct RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDesc {
    RuntimePackageIndexDiscoveryDesc discovery;
    std::string selected_package_index_path;
    RuntimeResidentPackageMountId mount_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudget budget{};
    std::vector<RuntimeResidentPackageMountId> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountId> protected_mount_ids;
};

struct RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnostic {
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase phase{
        RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_selection};
    AssetId asset;
    RuntimeResidentPackageMountId mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult {
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus status{
        RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::invalid_descriptor};
    RuntimePackageIndexDiscoveryResult discovery;
    RuntimePackageIndexDiscoveryCandidate selected_candidate;
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResult candidate_resident_replace;
    RuntimeResidentPackageEvictionPlanResult eviction_plan;
    RuntimeResidentCatalogCacheRefreshResult catalog_refresh;
    std::vector<RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnostic> diagnostics;
    std::size_t loaded_record_count{0};
    std::uint64_t loaded_resident_bytes{0};
    std::uint64_t projected_resident_bytes{0};
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};
    std::size_t evicted_mount_count{0};
    bool invoked_discovery{false};
    bool invoked_resident_commit{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Discovers reviewed package-index candidates from a caller-owned VFS root, selects one exact package index path,
/// then delegates to the reviewed eviction-assisted candidate resident replacement helper. Selected-path and existing
/// mount-id preflight happens before discovery/package reads, and live mount/cache state changes only after the
/// delegated copy-then-commit path succeeds. This helper does not infer content roots, stream in the background, choose
/// eviction policy, hot reload, upload/stage renderer resources, enforce GPU budgets, or touch renderer/RHI/native
/// handles.
[[nodiscard]] RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult
commit_runtime_package_discovery_resident_replace_with_reviewed_evictions(
    IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set, RuntimeResidentCatalogCache& catalog_cache,
    const RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDesc& desc);

enum class RuntimePackageHotReloadReplacementIntentReviewStatus : std::uint8_t {
    review_ready = 0,
    invalid_candidate,
    missing_matched_change,
    invalid_descriptor,
    invalid_overlay,
    invalid_mount_id,
    missing_mount_id,
    invalid_eviction_candidate_mount_id,
    duplicate_eviction_candidate_mount_id,
    missing_eviction_candidate_mount_id,
    protected_eviction_candidate_mount_id,
};

enum class RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase : std::uint8_t {
    candidate = 0,
    descriptor,
    resident_replace,
    eviction_plan,
};

struct RuntimePackageHotReloadReplacementIntentReviewDesc {
    RuntimePackageHotReloadCandidateReviewRow selected_candidate;
    RuntimePackageIndexDiscoveryDesc discovery;
    RuntimeResidentPackageMountId mount_id;
    std::vector<RuntimeResidentPackageMountId> reviewed_existing_mount_ids;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudget budget{};
    std::vector<RuntimeResidentPackageMountId> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountId> protected_mount_ids;
};

struct RuntimePackageHotReloadReplacementIntentReviewDiagnostic {
    RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase phase{
        RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::candidate};
    RuntimeResidentPackageMountId mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageHotReloadReplacementIntentReviewResult {
    RuntimePackageHotReloadReplacementIntentReviewStatus status{
        RuntimePackageHotReloadReplacementIntentReviewStatus::invalid_candidate};
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDesc replacement_desc;
    std::vector<RuntimePackageHotReloadReplacementIntentReviewDiagnostic> diagnostics;
    std::size_t matched_change_count{0};
    std::size_t eviction_candidate_count{0};
    std::size_t protected_mount_count{0};
    bool invoked_discovery{false};
    bool invoked_package_load{false};
    bool invoked_resident_commit{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Converts one reviewed hot-reload candidate row into an explicit resident replacement descriptor.
/// This pure planner validates the selected candidate, caller-reviewed existing mount ids, discovery descriptor, and
/// reviewed eviction ids only. It does not scan discovery roots, read package indexes, load packages, mutate resident
/// state, watch files, recook assets, stream in the background, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimePackageHotReloadReplacementIntentReviewResult
plan_runtime_package_hot_reload_replacement_intent_review(
    const RuntimePackageHotReloadReplacementIntentReviewDesc& desc);

enum class RuntimePackageHotReloadRecookReplacementStatus : std::uint8_t {
    committed = 0,
    recook_change_review_failed,
    candidate_not_found,
    replacement_intent_review_failed,
    replacement_commit_failed,
};

enum class RuntimePackageHotReloadRecookReplacementDiagnosticPhase : std::uint8_t {
    recook_change_review = 0,
    candidate_selection,
    replacement_intent_review,
    resident_replace,
    discovery,
    candidate_load,
    candidate_catalog,
    eviction_plan,
    resident_budget,
    catalog_refresh,
    resident_commit,
};

struct RuntimePackageHotReloadRecookReplacementDesc {
    std::vector<AssetHotReloadApplyResult> recook_apply_results;
    std::vector<RuntimePackageIndexDiscoveryCandidate> candidates;
    RuntimePackageIndexDiscoveryDesc discovery;
    std::string selected_package_index_path;
    RuntimeResidentPackageMountId mount_id;
    std::vector<RuntimeResidentPackageMountId> reviewed_existing_mount_ids;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudget budget{};
    std::vector<RuntimeResidentPackageMountId> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountId> protected_mount_ids;
};

struct RuntimePackageHotReloadRecookReplacementDiagnostic {
    RuntimePackageHotReloadRecookReplacementDiagnosticPhase phase{
        RuntimePackageHotReloadRecookReplacementDiagnosticPhase::recook_change_review};
    AssetId asset;
    RuntimeResidentPackageMountId mount;
    std::string path;
    std::string code;
    std::string message;
};

struct RuntimePackageHotReloadRecookReplacementResult {
    RuntimePackageHotReloadRecookReplacementStatus status{
        RuntimePackageHotReloadRecookReplacementStatus::recook_change_review_failed};
    RuntimePackageHotReloadRecookChangeReviewResult recook_change_review;
    RuntimePackageHotReloadCandidateReviewRow selected_candidate;
    RuntimePackageHotReloadReplacementIntentReviewResult replacement_intent_review;
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult replacement_commit;
    std::vector<RuntimePackageHotReloadRecookReplacementDiagnostic> diagnostics;
    std::size_t selected_candidate_count{0};
    std::size_t loaded_record_count{0};
    std::uint64_t loaded_resident_bytes{0};
    std::uint64_t projected_resident_bytes{0};
    std::uint32_t previous_mount_generation{0};
    std::uint32_t mount_generation{0};
    std::size_t previous_mount_count{0};
    std::size_t mounted_package_count{0};
    std::size_t evicted_mount_count{0};
    bool invoked_candidate_review{false};
    bool invoked_replacement_intent_review{false};
    bool invoked_file_watch{false};
    bool invoked_recook{false};
    bool invoked_package_load{false};
    bool invoked_resident_commit{false};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Converts already-reviewed recook apply rows into one reviewed safe-point resident replacement commit.
/// The helper composes the pure recook change review, the pure replacement-intent review, and the existing
/// discovery-backed resident replacement commit. It does not watch files, run recook, infer roots, choose eviction
/// policy, stream in the background, upload/stage renderer resources, enforce GPU budgets, or touch renderer/RHI/native
/// handles.
[[nodiscard]] RuntimePackageHotReloadRecookReplacementResult
commit_runtime_package_hot_reload_recook_replacement(IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set,
                                                     RuntimeResidentCatalogCache& catalog_cache,
                                                     const RuntimePackageHotReloadRecookReplacementDesc& desc);

[[nodiscard]] std::optional<RuntimeResourceHandle> find_runtime_resource(const RuntimeResourceCatalog& catalog,
                                                                         AssetId asset) noexcept;
[[nodiscard]] bool is_runtime_resource_handle_live(const RuntimeResourceCatalog& catalog,
                                                   RuntimeResourceHandle handle) noexcept;
[[nodiscard]] const RuntimeResourceRecord* runtime_resource_record(const RuntimeResourceCatalog& catalog,
                                                                   RuntimeResourceHandle handle) noexcept;

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
    std::vector<RuntimeResourceCatalogBuildDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimePackageSafePointReplacementResult
commit_runtime_package_safe_point_replacement(RuntimeAssetPackageStore& store, RuntimeResourceCatalog& catalog);

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
commit_runtime_package_safe_point_unload(RuntimeAssetPackageStore& store, RuntimeResourceCatalog& catalog);

} // namespace mirakana::runtime
