// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/asset_runtime_package_hot_reload_tool.hpp"

#include <algorithm>
#include <exception>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(AssetRuntimePackageHotReloadReplacementResult& result,
                    AssetRuntimePackageHotReloadReplacementDiagnosticPhase phase, AssetId asset,
                    runtime::RuntimeResidentPackageMountIdV2 mount, std::string path, std::string code,
                    std::string message) {
    result.diagnostics.push_back(AssetRuntimePackageHotReloadReplacementDiagnostic{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_recook_diagnostics(AssetRuntimePackageHotReloadReplacementResult& result) {
    for (const auto& apply_result : result.recook.apply_results) {
        if (apply_result.kind != AssetHotReloadApplyResultKind::failed_rolled_back &&
            apply_result.kind != AssetHotReloadApplyResultKind::unknown) {
            continue;
        }
        add_diagnostic(result, AssetRuntimePackageHotReloadReplacementDiagnosticPhase::recook_execution,
                       apply_result.asset, {}, apply_result.path, "recook-failed",
                       apply_result.diagnostic.empty() ? "asset recook failed" : apply_result.diagnostic);
    }

    if (result.diagnostics.empty() && !result.recook.succeeded()) {
        add_diagnostic(result, AssetRuntimePackageHotReloadReplacementDiagnosticPhase::recook_execution, {}, {}, {},
                       "recook-failed", "asset runtime recook failed");
    }
}

void add_runtime_replacement_diagnostics(AssetRuntimePackageHotReloadReplacementResult& result) {
    for (const auto& diagnostic : result.runtime_replacement.diagnostics) {
        add_diagnostic(result, AssetRuntimePackageHotReloadReplacementDiagnosticPhase::runtime_replacement,
                       diagnostic.asset, diagnostic.mount, diagnostic.path, diagnostic.code, diagnostic.message);
    }

    if (result.diagnostics.empty() && !result.runtime_replacement.succeeded()) {
        add_diagnostic(result, AssetRuntimePackageHotReloadReplacementDiagnosticPhase::runtime_replacement, {}, {}, {},
                       "runtime-replacement-failed", "runtime package hot reload replacement failed");
    }
}

[[nodiscard]] runtime::RuntimePackageHotReloadRecookReplacementDescV2
make_runtime_replacement_desc(const AssetRuntimePackageHotReloadRuntimeReplacementDesc& desc,
                              std::vector<AssetHotReloadApplyResult> apply_results) {
    return runtime::RuntimePackageHotReloadRecookReplacementDescV2{
        .recook_apply_results = std::move(apply_results),
        .candidates = desc.candidates,
        .discovery = desc.discovery,
        .selected_package_index_path = desc.selected_package_index_path,
        .mount_id = desc.mount_id,
        .reviewed_existing_mount_ids = desc.reviewed_existing_mount_ids,
        .overlay = desc.overlay,
        .budget = desc.budget,
        .eviction_candidate_unmount_order = desc.eviction_candidate_unmount_order,
        .protected_mount_ids = desc.protected_mount_ids,
    };
}

[[nodiscard]] std::vector<AssetId>
selected_assets_for_runtime_candidate(const std::vector<AssetHotReloadApplyResult>& apply_results,
                                      const runtime::RuntimePackageHotReloadCandidateReviewRowV2& selected_candidate) {
    std::unordered_set<std::string> selected_paths;
    selected_paths.reserve(selected_candidate.matched_changes.size());
    for (const auto& change : selected_candidate.matched_changes) {
        selected_paths.insert(change.path);
    }

    std::vector<AssetId> assets;
    std::unordered_set<std::uint64_t> seen_assets;
    assets.reserve(apply_results.size());
    seen_assets.reserve(apply_results.size());
    for (const auto& apply_result : apply_results) {
        if (apply_result.kind != AssetHotReloadApplyResultKind::staged &&
            apply_result.kind != AssetHotReloadApplyResultKind::applied) {
            continue;
        }
        if (selected_paths.find(apply_result.path) == selected_paths.end()) {
            continue;
        }
        if (!seen_assets.insert(apply_result.asset.value).second) {
            continue;
        }
        assets.push_back(apply_result.asset);
    }

    std::ranges::sort(assets, [](AssetId lhs, AssetId rhs) { return lhs.value < rhs.value; });
    return assets;
}

} // namespace

bool AssetRuntimePackageHotReloadReplacementResult::succeeded() const noexcept {
    return status == AssetRuntimePackageHotReloadReplacementStatus::committed && recook.succeeded() &&
           runtime_replacement.succeeded() && committed;
}

AssetRuntimePackageHotReloadReplacementResult execute_asset_runtime_package_hot_reload_replacement_safe_point(
    IFileSystem& filesystem, AssetRuntimeReplacementState& replacements,
    runtime::RuntimeResidentPackageMountSetV2& mount_set, runtime::RuntimeResidentCatalogCacheV2& catalog_cache,
    const AssetRuntimePackageHotReloadReplacementDesc& desc) {
    AssetRuntimePackageHotReloadReplacementResult result;
    result.invoked_file_watch = false;
    result.invoked_recook = true;
    try {
        result.recook = execute_asset_runtime_recook(filesystem, desc.import_plan, replacements, desc.recook_requests,
                                                     desc.import_options);
    } catch (const std::exception& error) {
        result.status = AssetRuntimePackageHotReloadReplacementStatus::recook_failed;
        add_diagnostic(result, AssetRuntimePackageHotReloadReplacementDiagnosticPhase::recook_execution, {}, {}, {},
                       "recook-exception", error.what());
        return result;
    }

    if (!result.recook.succeeded()) {
        result.status = AssetRuntimePackageHotReloadReplacementStatus::recook_failed;
        add_recook_diagnostics(result);
        return result;
    }

    const auto review = runtime::plan_runtime_package_hot_reload_recook_change_review_v2(
        runtime::RuntimePackageHotReloadRecookChangeReviewDescV2{
            .recook_apply_results = result.recook.apply_results,
            .candidates = desc.runtime_replacement.candidates,
        });
    std::vector<AssetId> selected_assets;
    if (review.succeeded()) {
        const auto selected = std::ranges::find_if(
            review.candidate_review.rows, [&desc](const runtime::RuntimePackageHotReloadCandidateReviewRowV2& row) {
                return row.candidate.package_index_path == desc.runtime_replacement.selected_package_index_path;
            });
        if (selected != review.candidate_review.rows.end()) {
            selected_assets = selected_assets_for_runtime_candidate(result.recook.apply_results, *selected);
            if (selected_assets.empty()) {
                result.status = AssetRuntimePackageHotReloadReplacementStatus::runtime_replacement_failed;
                add_diagnostic(result, AssetRuntimePackageHotReloadReplacementDiagnosticPhase::runtime_replacement, {},
                               desc.runtime_replacement.mount_id, desc.runtime_replacement.selected_package_index_path,
                               "runtime-replacement-no-matched-recook-assets",
                               "selected runtime package has no staged recook assets to commit");
                return result;
            }
        }
    }

    auto runtime_replacement_desc =
        make_runtime_replacement_desc(desc.runtime_replacement, result.recook.apply_results);
    result.invoked_runtime_replacement = true;
    result.runtime_replacement = runtime::commit_runtime_package_hot_reload_recook_replacement_v2(
        filesystem, mount_set, catalog_cache, runtime_replacement_desc);

    if (!result.runtime_replacement.succeeded()) {
        result.status = AssetRuntimePackageHotReloadReplacementStatus::runtime_replacement_failed;
        add_runtime_replacement_diagnostics(result);
        return result;
    }

    result.committed_apply_results = replacements.commit_safe_point(selected_assets);
    result.status = AssetRuntimePackageHotReloadReplacementStatus::committed;
    result.committed = true;
    return result;
}

} // namespace mirakana
