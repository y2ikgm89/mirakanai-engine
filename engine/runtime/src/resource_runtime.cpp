// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/resource_runtime.hpp"

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/runtime/asset_runtime.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

[[nodiscard]] std::uint32_t next_generation(std::uint32_t generation) noexcept {
    if (generation == std::numeric_limits<std::uint32_t>::max()) {
        return 1U;
    }
    return generation + 1U;
}

[[nodiscard]] RuntimeResidentPackageMountResultV2 mount_result(RuntimeResidentPackageMountIdV2 id,
                                                               RuntimeResidentPackageMountStatusV2 status,
                                                               std::string code, std::string message) {
    return RuntimeResidentPackageMountResultV2{
        .status = status,
        .diagnostic =
            RuntimeResidentPackageMountDiagnosticV2{
                .mount = id,
                .code = std::move(code),
                .message = std::move(message),
            },
    };
}

[[nodiscard]] bool same_budget(const RuntimeResourceResidencyBudgetV2& lhs,
                               const RuntimeResourceResidencyBudgetV2& rhs) noexcept {
    return lhs.max_resident_content_bytes == rhs.max_resident_content_bytes &&
           lhs.max_resident_asset_records == rhs.max_resident_asset_records;
}

[[nodiscard]] std::vector<RuntimeAssetPackage>
resident_packages_from_mount_set(const RuntimeResidentPackageMountSetV2& mount_set) {
    std::vector<RuntimeAssetPackage> packages;
    packages.reserve(mount_set.mounts().size());
    for (const auto& mount : mount_set.mounts()) {
        packages.push_back(mount.package);
    }
    return packages;
}

} // namespace

const std::vector<RuntimeResourceRecordV2>& RuntimeResourceCatalogV2::records() const noexcept {
    return records_;
}

std::uint32_t RuntimeResourceCatalogV2::generation() const noexcept {
    return generation_;
}

RuntimeResourceCatalogBuildResultV2 build_runtime_resource_catalog_v2(RuntimeResourceCatalogV2& catalog,
                                                                      const RuntimeAssetPackage& package) {
    RuntimeResourceCatalogBuildResultV2 result;
    std::unordered_set<AssetId, AssetIdHash> assets;
    for (const auto& record : package.records()) {
        if (!assets.insert(record.asset).second) {
            result.diagnostics.push_back(RuntimeResourceCatalogBuildDiagnosticV2{
                .asset = record.asset,
                .diagnostic = "duplicate runtime asset record",
            });
        }
    }
    if (!result.succeeded()) {
        return result;
    }

    const auto generation = next_generation(catalog.generation_);
    std::vector<RuntimeResourceRecordV2> records;
    records.reserve(package.records().size());
    for (const auto& record : package.records()) {
        records.push_back(RuntimeResourceRecordV2{
            .handle = RuntimeResourceHandleV2{.index = static_cast<std::uint32_t>(records.size() + 1U),
                                              .generation = generation},
            .asset = record.asset,
            .kind = record.kind,
            .package_handle = record.handle,
            .path = record.path,
            .content_hash = record.content_hash,
            .source_revision = record.source_revision,
        });
    }

    catalog.records_ = std::move(records);
    catalog.generation_ = generation;
    return result;
}

RuntimeResourceCatalogBuildResultV2
build_runtime_resource_catalog_v2_from_resident_mounts(RuntimeResourceCatalogV2& catalog,
                                                       const std::vector<RuntimeAssetPackage>& mounts,
                                                       RuntimePackageMountOverlay overlay) {
    auto merged = merge_runtime_asset_packages_overlay(mounts, overlay);
    return build_runtime_resource_catalog_v2(catalog, merged);
}

bool RuntimeResidentPackageMountResultV2::succeeded() const noexcept {
    return status == RuntimeResidentPackageMountStatusV2::mounted ||
           status == RuntimeResidentPackageMountStatusV2::unmounted;
}

const std::vector<RuntimeResidentPackageMountRecordV2>& RuntimeResidentPackageMountSetV2::mounts() const noexcept {
    return mounts_;
}

std::uint32_t RuntimeResidentPackageMountSetV2::generation() const noexcept {
    return generation_;
}

RuntimeResidentPackageMountResultV2
RuntimeResidentPackageMountSetV2::mount(RuntimeResidentPackageMountRecordV2 record) {
    if (record.id.value == 0) {
        return mount_result(record.id, RuntimeResidentPackageMountStatusV2::invalid_mount_id, "invalid-mount-id",
                            "resident package mount ids must be non-zero");
    }
    for (const auto& mounted : mounts_) {
        if (mounted.id == record.id) {
            return mount_result(record.id, RuntimeResidentPackageMountStatusV2::duplicate_mount_id,
                                "duplicate-mount-id", "resident package mount id is already mounted");
        }
    }

    const auto id = record.id;
    mounts_.push_back(std::move(record));
    generation_ = next_generation(generation_);
    return mount_result(id, RuntimeResidentPackageMountStatusV2::mounted, {}, {});
}

RuntimeResidentPackageMountResultV2 RuntimeResidentPackageMountSetV2::unmount(RuntimeResidentPackageMountIdV2 id) {
    if (id.value == 0) {
        return mount_result(id, RuntimeResidentPackageMountStatusV2::invalid_mount_id, "invalid-mount-id",
                            "resident package mount ids must be non-zero");
    }
    for (auto it = mounts_.begin(); it != mounts_.end(); ++it) {
        if (it->id == id) {
            mounts_.erase(it);
            generation_ = next_generation(generation_);
            return mount_result(id, RuntimeResidentPackageMountStatusV2::unmounted, {}, {});
        }
    }
    return mount_result(id, RuntimeResidentPackageMountStatusV2::missing_mount_id, "missing-mount-id",
                        "resident package mount id is not mounted");
}

bool RuntimeResidentPackageMountCatalogBuildResultV2::succeeded() const noexcept {
    return catalog_build.succeeded();
}

RuntimeResidentPackageMountCatalogBuildResultV2
build_runtime_resource_catalog_v2_from_resident_mount_set(RuntimeResourceCatalogV2& catalog,
                                                          const RuntimeResidentPackageMountSetV2& mount_set,
                                                          RuntimePackageMountOverlay overlay) {
    auto packages = resident_packages_from_mount_set(mount_set);

    RuntimeResidentPackageMountCatalogBuildResultV2 result;
    result.mounted_package_count = packages.size();
    result.mount_generation = mount_set.generation();
    result.catalog_build = build_runtime_resource_catalog_v2_from_resident_mounts(catalog, packages, overlay);
    return result;
}

RuntimeResourceResidencyBudgetExecutionResultV2
evaluate_runtime_resource_residency_budget(const RuntimeAssetPackage& resident_package_view,
                                           const RuntimeResourceResidencyBudgetV2& budget) {
    RuntimeResourceResidencyBudgetExecutionResultV2 result;
    result.estimated_resident_content_bytes = estimate_runtime_asset_package_resident_bytes(resident_package_view);
    result.resident_asset_record_count = resident_package_view.records().size();
    result.within_budget = true;

    if (budget.max_resident_content_bytes.has_value() &&
        result.estimated_resident_content_bytes > *budget.max_resident_content_bytes) {
        result.within_budget = false;
        result.diagnostics.push_back(RuntimeResourceResidencyBudgetDiagnosticV2{
            .code = "resident-content-bytes-exceed-budget",
            .message = "runtime package content byte estimate exceeds max_resident_content_bytes",
        });
    }
    if (budget.max_resident_asset_records.has_value() &&
        result.resident_asset_record_count > *budget.max_resident_asset_records) {
        result.within_budget = false;
        result.diagnostics.push_back(RuntimeResourceResidencyBudgetDiagnosticV2{
            .code = "resident-asset-record-count-exceeds-budget",
            .message = "runtime package asset record count exceeds max_resident_asset_records",
        });
    }
    return result;
}

RuntimeResidentMountCatalogBudgetBundleResult build_runtime_resource_catalog_v2_from_resident_mounts_with_budget(
    RuntimeResourceCatalogV2& catalog, const std::vector<RuntimeAssetPackage>& mounts,
    RuntimePackageMountOverlay overlay, const RuntimeResourceResidencyBudgetV2& budget) {
    RuntimeResidentMountCatalogBudgetBundleResult out;
    auto merged = merge_runtime_asset_packages_overlay(mounts, overlay);
    out.budget_execution = evaluate_runtime_resource_residency_budget(merged, budget);
    if (!out.budget_execution.within_budget) {
        return out;
    }
    out.catalog_build = build_runtime_resource_catalog_v2(catalog, merged);
    out.invoked_catalog_build = true;
    return out;
}

bool RuntimeResidentCatalogCacheRefreshResultV2::succeeded() const noexcept {
    return status == RuntimeResidentCatalogCacheStatusV2::rebuilt ||
           status == RuntimeResidentCatalogCacheStatusV2::cache_hit;
}

const RuntimeResourceCatalogV2& RuntimeResidentCatalogCacheV2::catalog() const noexcept {
    return catalog_;
}

bool RuntimeResidentCatalogCacheV2::has_value() const noexcept {
    return has_cache_;
}

std::uint32_t RuntimeResidentCatalogCacheV2::cached_mount_generation() const noexcept {
    return cached_mount_generation_;
}

RuntimeResidentCatalogCacheRefreshResultV2
RuntimeResidentCatalogCacheV2::refresh(const RuntimeResidentPackageMountSetV2& mount_set,
                                       RuntimePackageMountOverlay overlay,
                                       const RuntimeResourceResidencyBudgetV2& budget) {
    RuntimeResidentCatalogCacheRefreshResultV2 result;
    result.mounted_package_count = mount_set.mounts().size();
    result.mount_generation = mount_set.generation();
    result.previous_catalog_generation = catalog_.generation();
    result.catalog_generation = catalog_.generation();

    if (has_cache_ && cached_mount_generation_ == mount_set.generation() && cached_overlay_ == overlay &&
        same_budget(cached_budget_, budget)) {
        result.status = RuntimeResidentCatalogCacheStatusV2::cache_hit;
        result.budget_execution = cached_budget_execution_;
        result.reused_cache = true;
        return result;
    }

    auto packages = resident_packages_from_mount_set(mount_set);
    auto merged = merge_runtime_asset_packages_overlay(packages, overlay);
    result.budget_execution = evaluate_runtime_resource_residency_budget(merged, budget);
    if (!result.budget_execution.within_budget) {
        result.status = RuntimeResidentCatalogCacheStatusV2::budget_failed;
        return result;
    }

    RuntimeResourceCatalogV2 replacement = catalog_;
    result.catalog_build = build_runtime_resource_catalog_v2(replacement, merged);
    result.invoked_catalog_build = true;
    if (!result.catalog_build.succeeded()) {
        result.status = RuntimeResidentCatalogCacheStatusV2::catalog_build_failed;
        return result;
    }

    catalog_ = std::move(replacement);
    cached_budget_execution_ = result.budget_execution;
    cached_budget_ = budget;
    cached_overlay_ = overlay;
    cached_mount_generation_ = mount_set.generation();
    has_cache_ = true;

    result.status = RuntimeResidentCatalogCacheStatusV2::rebuilt;
    result.catalog_generation = catalog_.generation();
    return result;
}

void RuntimeResidentCatalogCacheV2::clear() {
    catalog_ = RuntimeResourceCatalogV2{};
    cached_budget_execution_ = RuntimeResourceResidencyBudgetExecutionResultV2{};
    cached_budget_ = RuntimeResourceResidencyBudgetV2{};
    cached_overlay_ = RuntimePackageMountOverlay::first_mount_wins;
    cached_mount_generation_ = 0;
    has_cache_ = false;
}

bool RuntimeResidentPackageUnmountCommitResultV2::succeeded() const noexcept {
    return status == RuntimeResidentPackageUnmountCommitStatusV2::unmounted;
}

RuntimeResidentPackageUnmountCommitResultV2
commit_runtime_resident_package_unmount_v2(RuntimeResidentPackageMountSetV2& mount_set,
                                           RuntimeResidentCatalogCacheV2& catalog_cache,
                                           RuntimeResidentPackageMountIdV2 id, RuntimePackageMountOverlay overlay,
                                           const RuntimeResourceResidencyBudgetV2& budget) {
    RuntimeResidentPackageUnmountCommitResultV2 result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    result.unmount = projected_mount_set.unmount(id);
    if (!result.unmount.succeeded()) {
        result.status = result.unmount.status == RuntimeResidentPackageMountStatusV2::invalid_mount_id
                            ? RuntimeResidentPackageUnmountCommitStatusV2::invalid_mount_id
                            : RuntimeResidentPackageUnmountCommitStatusV2::missing_mount_id;
        return result;
    }

    RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, overlay, budget);
    if (!result.catalog_refresh.succeeded()) {
        result.status = result.catalog_refresh.status == RuntimeResidentCatalogCacheStatusV2::budget_failed
                            ? RuntimeResidentPackageUnmountCommitStatusV2::budget_failed
                            : RuntimeResidentPackageUnmountCommitStatusV2::catalog_build_failed;
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.status = RuntimeResidentPackageUnmountCommitStatusV2::unmounted;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    return result;
}

struct RuntimeResidentPackageMountSetReplaceAccessV2 {
    [[nodiscard]] static bool replace(RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentPackageMountIdV2 id,
                                      RuntimeAssetPackage package) {
        for (auto& mount : mount_set.mounts_) {
            if (mount.id == id) {
                mount.package = std::move(package);
                mount_set.generation_ = next_generation(mount_set.generation_);
                return true;
            }
        }
        return false;
    }
};

bool RuntimeResidentPackageReplaceCommitResultV2::succeeded() const noexcept {
    return status == RuntimeResidentPackageReplaceCommitStatusV2::replaced;
}

RuntimeResidentPackageReplaceCommitResultV2 commit_runtime_resident_package_replace_v2(
    RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    RuntimeResidentPackageMountIdV2 id, RuntimeAssetPackage replacement_package, RuntimePackageMountOverlay overlay,
    const RuntimeResourceResidencyBudgetV2& budget) {
    RuntimeResidentPackageReplaceCommitResultV2 result;
    result.diagnostic.mount = id;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.mounted_package_count = mount_set.mounts().size();

    if (id.value == 0) {
        result.status = RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id;
        result.diagnostic.code = "invalid-mount-id";
        result.diagnostic.message = "resident package mount ids must be non-zero";
        return result;
    }

    bool found = false;
    for (const auto& mount : mount_set.mounts()) {
        if (mount.id == id) {
            found = true;
            break;
        }
    }
    if (!found) {
        result.status = RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id;
        result.diagnostic.code = "missing-mount-id";
        result.diagnostic.message = "resident package mount id is not mounted";
        return result;
    }

    RuntimeResourceCatalogV2 candidate_catalog;
    result.candidate_catalog_build = build_runtime_resource_catalog_v2(candidate_catalog, replacement_package);
    result.invoked_candidate_catalog_build = true;
    if (!result.candidate_catalog_build.succeeded()) {
        result.status = RuntimeResidentPackageReplaceCommitStatusV2::catalog_build_failed;
        return result;
    }

    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    const auto replaced =
        RuntimeResidentPackageMountSetReplaceAccessV2::replace(projected_mount_set, id, std::move(replacement_package));
    if (!replaced) {
        result.status = RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id;
        result.diagnostic.code = "missing-mount-id";
        result.diagnostic.message = "resident package mount id is not mounted";
        return result;
    }

    RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, overlay, budget);
    if (!result.catalog_refresh.succeeded()) {
        result.status = result.catalog_refresh.status == RuntimeResidentCatalogCacheStatusV2::budget_failed
                            ? RuntimeResidentPackageReplaceCommitStatusV2::budget_failed
                            : RuntimeResidentPackageReplaceCommitStatusV2::catalog_build_failed;
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.status = RuntimeResidentPackageReplaceCommitStatusV2::replaced;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    return result;
}

std::optional<RuntimeResourceHandleV2> find_runtime_resource_v2(const RuntimeResourceCatalogV2& catalog,
                                                                AssetId asset) noexcept {
    for (const auto& record : catalog.records()) {
        if (record.asset == asset && is_runtime_resource_handle_live_v2(catalog, record.handle)) {
            return record.handle;
        }
    }
    return std::nullopt;
}

bool is_runtime_resource_handle_live_v2(const RuntimeResourceCatalogV2& catalog,
                                        RuntimeResourceHandleV2 handle) noexcept {
    if (handle.index == 0 || handle.index > catalog.records().size()) {
        return false;
    }
    return catalog.records()[handle.index - 1U].handle.generation == handle.generation;
}

const RuntimeResourceRecordV2* runtime_resource_record_v2(const RuntimeResourceCatalogV2& catalog,
                                                          RuntimeResourceHandleV2 handle) noexcept {
    if (!is_runtime_resource_handle_live_v2(catalog, handle)) {
        return nullptr;
    }
    return &catalog.records()[handle.index - 1U];
}

bool RuntimePackageSafePointReplacementResult::succeeded() const noexcept {
    return status == RuntimePackageSafePointReplacementStatus::committed;
}

RuntimePackageSafePointReplacementResult
commit_runtime_package_safe_point_replacement(RuntimeAssetPackageStore& store, RuntimeResourceCatalogV2& catalog) {
    RuntimePackageSafePointReplacementResult result;
    result.previous_record_count = catalog.records().size();
    result.previous_generation = catalog.generation();
    result.committed_generation = result.previous_generation;

    const auto* pending = store.pending();
    if (pending == nullptr) {
        return result;
    }

    RuntimeResourceCatalogV2 replacement_catalog = catalog;
    auto catalog_build = build_runtime_resource_catalog_v2(replacement_catalog, *pending);
    if (!catalog_build.succeeded()) {
        result.status = RuntimePackageSafePointReplacementStatus::catalog_build_failed;
        result.diagnostics = std::move(catalog_build.diagnostics);
        store.rollback_pending();
        return result;
    }

    if (!store.commit_safe_point()) {
        return result;
    }

    catalog = std::move(replacement_catalog);
    result.status = RuntimePackageSafePointReplacementStatus::committed;
    result.committed_record_count = catalog.records().size();
    result.committed_generation = catalog.generation();
    if (result.previous_generation != 0 && result.previous_generation != result.committed_generation) {
        result.stale_handle_count = result.previous_record_count;
    }
    return result;
}

bool RuntimePackageSafePointUnloadResult::succeeded() const noexcept {
    return status == RuntimePackageSafePointUnloadStatus::unloaded;
}

RuntimePackageSafePointUnloadResult commit_runtime_package_safe_point_unload(RuntimeAssetPackageStore& store,
                                                                             RuntimeResourceCatalogV2& catalog) {
    RuntimePackageSafePointUnloadResult result;
    result.previous_record_count = catalog.records().size();
    result.previous_generation = catalog.generation();
    result.committed_generation = result.previous_generation;

    if (store.active() == nullptr) {
        return result;
    }

    RuntimeResourceCatalogV2 unloaded_catalog = catalog;
    const RuntimeAssetPackage empty_package;
    auto catalog_build = build_runtime_resource_catalog_v2(unloaded_catalog, empty_package);
    if (!catalog_build.succeeded()) {
        return result;
    }

    result.discarded_pending_package = store.pending() != nullptr;
    if (!store.unload_safe_point()) {
        result.discarded_pending_package = false;
        return result;
    }

    catalog = std::move(unloaded_catalog);
    result.status = RuntimePackageSafePointUnloadStatus::unloaded;
    result.committed_record_count = catalog.records().size();
    result.committed_generation = catalog.generation();
    if (result.previous_generation != 0 && result.previous_generation != result.committed_generation) {
        result.stale_handle_count = result.previous_record_count;
    }
    return result;
}

} // namespace mirakana::runtime
