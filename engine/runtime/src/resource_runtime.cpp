// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/resource_runtime.hpp"

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/runtime/asset_runtime.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

constexpr std::string_view package_index_extension = ".geindex";

[[nodiscard]] bool contains_control_character(std::string_view text) noexcept {
    return std::ranges::any_of(text, [](unsigned char character) { return character < 0x20U; });
}

[[nodiscard]] std::string trim_trailing_slashes(std::string_view path) {
    std::string normalized{path};
    while (!normalized.empty() && (normalized.back() == '/' || normalized.back() == '\\')) {
        normalized.pop_back();
    }
    return normalized;
}

[[nodiscard]] bool valid_relative_vfs_path(std::string_view path) noexcept {
    if (path.empty() || contains_control_character(path)) {
        return false;
    }
    if (path.front() == '/' || path.front() == '\\') {
        return false;
    }
    if (path.find('\\') != std::string_view::npos) {
        return false;
    }
    if (path.size() >= 2U && path[1] == ':') {
        return false;
    }

    std::size_t segment_begin = 0;
    while (segment_begin <= path.size()) {
        const auto segment_end = path.find('/', segment_begin);
        const auto segment = segment_end == std::string_view::npos
                                 ? path.substr(segment_begin)
                                 : path.substr(segment_begin, segment_end - segment_begin);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }

    return true;
}

void add_discovery_diagnostic(RuntimePackageIndexDiscoveryResult& result, std::string path, std::string code,
                              std::string message) {
    result.diagnostics.push_back(RuntimePackageIndexDiscoveryDiagnostic{
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_load_diagnostic(RuntimePackageCandidateLoadResult& result, AssetId asset, std::string path,
                                   std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageCandidateLoadDiagnostic{
        .asset = asset,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_load_diagnostic(RuntimePackageCandidateLoadResult& result, std::string path, std::string code,
                                   std::string message) {
    add_candidate_load_diagnostic(result, AssetId{}, std::move(path), std::move(code), std::move(message));
}

void add_candidate_resident_mount_diagnostic(RuntimePackageCandidateResidentMountResult& result,
                                             RuntimePackageCandidateResidentMountDiagnosticPhase phase, AssetId asset,
                                             RuntimeResidentPackageMountId mount, std::string path, std::string code,
                                             std::string message) {
    result.diagnostics.push_back(RuntimePackageCandidateResidentMountDiagnostic{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_resident_mount_diagnostic(RuntimePackageCandidateResidentMountResult& result,
                                             RuntimePackageCandidateResidentMountDiagnosticPhase phase,
                                             RuntimeResidentPackageMountId mount, std::string code,
                                             std::string message) {
    add_candidate_resident_mount_diagnostic(result, phase, AssetId{}, mount, {}, std::move(code), std::move(message));
}

void add_candidate_resident_mount_reviewed_evictions_diagnostic(
    RuntimePackageCandidateResidentMountReviewedEvictionsResult& result,
    RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase phase, AssetId asset,
    RuntimeResidentPackageMountId mount, std::string path, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageCandidateResidentMountReviewedEvictionsDiagnostic{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_resident_mount_reviewed_evictions_diagnostic(
    RuntimePackageCandidateResidentMountReviewedEvictionsResult& result,
    RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase phase, RuntimeResidentPackageMountId mount,
    std::string code, std::string message) {
    add_candidate_resident_mount_reviewed_evictions_diagnostic(result, phase, AssetId{}, mount, {}, std::move(code),
                                                               std::move(message));
}

void add_candidate_resident_replace_diagnostic(RuntimePackageCandidateResidentReplaceResult& result,
                                               RuntimePackageCandidateResidentReplaceDiagnosticPhase phase,
                                               AssetId asset, RuntimeResidentPackageMountId mount, std::string path,
                                               std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageCandidateResidentReplaceDiagnostic{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_resident_replace_diagnostic(RuntimePackageCandidateResidentReplaceResult& result,
                                               RuntimePackageCandidateResidentReplaceDiagnosticPhase phase,
                                               RuntimeResidentPackageMountId mount, std::string code,
                                               std::string message) {
    add_candidate_resident_replace_diagnostic(result, phase, AssetId{}, mount, {}, std::move(code), std::move(message));
}

void add_candidate_resident_replace_reviewed_evictions_diagnostic(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResult& result,
    RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase phase, AssetId asset,
    RuntimeResidentPackageMountId mount, std::string path, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnostic{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_resident_replace_reviewed_evictions_diagnostic(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResult& result,
    RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase phase, RuntimeResidentPackageMountId mount,
    std::string code, std::string message) {
    add_candidate_resident_replace_reviewed_evictions_diagnostic(result, phase, AssetId{}, mount, {}, std::move(code),
                                                                 std::move(message));
}

void add_discovery_resident_commit_diagnostic(RuntimePackageDiscoveryResidentCommitResult& result,
                                              RuntimePackageDiscoveryResidentCommitDiagnosticPhase phase,
                                              RuntimeResidentPackageMountId mount, std::string path, std::string code,
                                              std::string message) {
    result.diagnostics.push_back(RuntimePackageDiscoveryResidentCommitDiagnostic{
        .phase = phase,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_discovery_resident_commit_diagnostic(RuntimePackageDiscoveryResidentCommitResult& result,
                                              RuntimePackageDiscoveryResidentCommitDiagnosticPhase phase,
                                              RuntimeResidentPackageMountId mount, std::string code,
                                              std::string message) {
    add_discovery_resident_commit_diagnostic(result, phase, mount, {}, std::move(code), std::move(message));
}

void add_discovery_diagnostics(RuntimePackageDiscoveryResidentCommitResult& result,
                               const RuntimePackageIndexDiscoveryResult& discovery,
                               RuntimePackageDiscoveryResidentCommitDiagnosticPhase phase) {
    for (const auto& diagnostic : discovery.diagnostics) {
        add_discovery_resident_commit_diagnostic(result, phase, {}, diagnostic.path, diagnostic.code,
                                                 diagnostic.message);
    }
}

[[nodiscard]] RuntimePackageDiscoveryResidentCommitDiagnosticPhase
resident_operation_phase(RuntimePackageDiscoveryResidentCommitMode mode) noexcept {
    return mode == RuntimePackageDiscoveryResidentCommitMode::replace
               ? RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_replace
               : RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_mount;
}

[[nodiscard]] RuntimePackageDiscoveryResidentCommitDiagnosticPhase
map_mount_diagnostic_phase(RuntimePackageCandidateResidentMountDiagnosticPhase phase) noexcept {
    switch (phase) {
    case RuntimePackageCandidateResidentMountDiagnosticPhase::resident_mount:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_mount;
    case RuntimePackageCandidateResidentMountDiagnosticPhase::candidate_load:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::candidate_load;
    case RuntimePackageCandidateResidentMountDiagnosticPhase::resident_budget:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_budget;
    case RuntimePackageCandidateResidentMountDiagnosticPhase::catalog_refresh:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::catalog_refresh;
    }
    return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_commit;
}

[[nodiscard]] RuntimePackageDiscoveryResidentCommitDiagnosticPhase
map_replace_diagnostic_phase(RuntimePackageCandidateResidentReplaceDiagnosticPhase phase) noexcept {
    switch (phase) {
    case RuntimePackageCandidateResidentReplaceDiagnosticPhase::resident_replace:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_replace;
    case RuntimePackageCandidateResidentReplaceDiagnosticPhase::candidate_load:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::candidate_load;
    case RuntimePackageCandidateResidentReplaceDiagnosticPhase::candidate_catalog:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_commit;
    case RuntimePackageCandidateResidentReplaceDiagnosticPhase::resident_budget:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_budget;
    case RuntimePackageCandidateResidentReplaceDiagnosticPhase::catalog_refresh:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::catalog_refresh;
    }
    return RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_commit;
}

void add_mount_commit_diagnostics(RuntimePackageDiscoveryResidentCommitResult& result,
                                  const RuntimePackageCandidateResidentMountResult& mount_result) {
    for (const auto& diagnostic : mount_result.diagnostics) {
        add_discovery_resident_commit_diagnostic(result, map_mount_diagnostic_phase(diagnostic.phase), diagnostic.mount,
                                                 diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

void add_replace_commit_diagnostics(RuntimePackageDiscoveryResidentCommitResult& result,
                                    const RuntimePackageCandidateResidentReplaceResult& replace_result) {
    for (const auto& diagnostic : replace_result.diagnostics) {
        add_discovery_resident_commit_diagnostic(result, map_replace_diagnostic_phase(diagnostic.phase),
                                                 diagnostic.mount, diagnostic.path, diagnostic.code,
                                                 diagnostic.message);
    }
}

void add_discovery_resident_mount_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentMountReviewedEvictionsResult& result,
    RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase phase, AssetId asset,
    RuntimeResidentPackageMountId mount, std::string path, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnostic{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_discovery_resident_mount_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentMountReviewedEvictionsResult& result,
    RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase phase, RuntimeResidentPackageMountId mount,
    std::string path, std::string code, std::string message) {
    add_discovery_resident_mount_reviewed_evictions_diagnostic(result, phase, AssetId{}, mount, std::move(path),
                                                               std::move(code), std::move(message));
}

void add_discovery_resident_mount_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentMountReviewedEvictionsResult& result,
    RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase phase, RuntimeResidentPackageMountId mount,
    std::string code, std::string message) {
    add_discovery_resident_mount_reviewed_evictions_diagnostic(result, phase, mount, {}, std::move(code),
                                                               std::move(message));
}

void add_discovery_diagnostics(RuntimePackageDiscoveryResidentMountReviewedEvictionsResult& result,
                               const RuntimePackageIndexDiscoveryResult& discovery,
                               RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase phase) {
    for (const auto& diagnostic : discovery.diagnostics) {
        add_discovery_resident_mount_reviewed_evictions_diagnostic(result, phase, {}, diagnostic.path, diagnostic.code,
                                                                   diagnostic.message);
    }
}

[[nodiscard]] RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase
map_reviewed_evictions_mount_diagnostic_phase(
    RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase phase) noexcept {
    switch (phase) {
    case RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::resident_mount:
        return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::resident_mount;
    case RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::candidate_load:
        return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::candidate_load;
    case RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::eviction_plan:
        return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::eviction_plan;
    case RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::resident_budget:
        return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::resident_budget;
    case RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::catalog_refresh:
        return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::catalog_refresh;
    }
    return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::resident_commit;
}

void add_reviewed_evictions_mount_commit_diagnostics(
    RuntimePackageDiscoveryResidentMountReviewedEvictionsResult& result,
    const RuntimePackageCandidateResidentMountReviewedEvictionsResult& mount_result) {
    for (const auto& diagnostic : mount_result.diagnostics) {
        add_discovery_resident_mount_reviewed_evictions_diagnostic(
            result, map_reviewed_evictions_mount_diagnostic_phase(diagnostic.phase), diagnostic.asset, diagnostic.mount,
            diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

void add_discovery_resident_replace_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult& result,
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase phase, AssetId asset,
    RuntimeResidentPackageMountId mount, std::string path, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnostic{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_discovery_resident_replace_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult& result,
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase phase, RuntimeResidentPackageMountId mount,
    std::string path, std::string code, std::string message) {
    add_discovery_resident_replace_reviewed_evictions_diagnostic(result, phase, AssetId{}, mount, std::move(path),
                                                                 std::move(code), std::move(message));
}

void add_discovery_resident_replace_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult& result,
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase phase, RuntimeResidentPackageMountId mount,
    std::string code, std::string message) {
    add_discovery_resident_replace_reviewed_evictions_diagnostic(result, phase, mount, {}, std::move(code),
                                                                 std::move(message));
}

void add_discovery_diagnostics(RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult& result,
                               const RuntimePackageIndexDiscoveryResult& discovery,
                               RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase phase) {
    for (const auto& diagnostic : discovery.diagnostics) {
        add_discovery_resident_replace_reviewed_evictions_diagnostic(result, phase, {}, diagnostic.path,
                                                                     diagnostic.code, diagnostic.message);
    }
}

[[nodiscard]] RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase
map_reviewed_evictions_replace_diagnostic_phase(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase phase) noexcept {
    switch (phase) {
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::resident_replace:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::resident_replace;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_load:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_load;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_catalog:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_catalog;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::eviction_plan:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::eviction_plan;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::resident_budget:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::resident_budget;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::catalog_refresh:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::catalog_refresh;
    }
    return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::resident_commit;
}

void add_reviewed_evictions_replace_commit_diagnostics(
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult& result,
    const RuntimePackageCandidateResidentReplaceReviewedEvictionsResult& replace_result) {
    for (const auto& diagnostic : replace_result.diagnostics) {
        add_discovery_resident_replace_reviewed_evictions_diagnostic(
            result, map_reviewed_evictions_replace_diagnostic_phase(diagnostic.phase), diagnostic.asset,
            diagnostic.mount, diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_evictions_candidate_load_diagnostics(
    RuntimePackageCandidateResidentMountReviewedEvictionsResult& result,
    const RuntimePackageCandidateLoadResult& load_result, RuntimeResidentPackageMountId mount) {
    for (const auto& diagnostic : load_result.diagnostics) {
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::candidate_load,
            diagnostic.asset, mount, diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_evictions_eviction_plan_diagnostics(
    RuntimePackageCandidateResidentMountReviewedEvictionsResult& result,
    const RuntimeResidentPackageEvictionPlanResult& eviction_plan) {
    for (const auto& diagnostic : eviction_plan.diagnostics) {
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::eviction_plan,
            diagnostic.mount, diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_evictions_budget_diagnostics(RuntimePackageCandidateResidentMountReviewedEvictionsResult& result,
                                               const RuntimeResourceResidencyBudgetExecutionResult& budget_execution,
                                               RuntimeResidentPackageMountId mount) {
    for (const auto& diagnostic : budget_execution.diagnostics) {
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::resident_budget, mount,
            diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_evictions_catalog_diagnostics(RuntimePackageCandidateResidentMountReviewedEvictionsResult& result,
                                                const RuntimeResourceCatalogBuildResult& catalog_build,
                                                RuntimeResidentPackageMountId mount) {
    for (const auto& diagnostic : catalog_build.diagnostics) {
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::catalog_refresh,
            diagnostic.asset, mount, {}, "catalog-build-failed", diagnostic.diagnostic);
    }
}

[[nodiscard]] RuntimePackageCandidateResidentMountReviewedEvictionsStatus
map_eviction_plan_status(RuntimeResidentPackageEvictionPlanStatus status) noexcept {
    switch (status) {
    case RuntimeResidentPackageEvictionPlanStatus::invalid_candidate_mount_id:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatus::invalid_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::duplicate_candidate_mount_id:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatus::duplicate_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::missing_candidate_mount_id:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatus::missing_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::protected_candidate_mount_id:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatus::protected_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::catalog_build_failed:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatus::catalog_refresh_failed;
    case RuntimeResidentPackageEvictionPlanStatus::budget_unreachable:
    case RuntimeResidentPackageEvictionPlanStatus::no_eviction_required:
    case RuntimeResidentPackageEvictionPlanStatus::planned:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatus::budget_failed;
    }
    return RuntimePackageCandidateResidentMountReviewedEvictionsStatus::budget_failed;
}

void add_replace_reviewed_evictions_candidate_load_diagnostics(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResult& result,
    const RuntimePackageCandidateLoadResult& load_result, RuntimeResidentPackageMountId mount) {
    for (const auto& diagnostic : load_result.diagnostics) {
        add_candidate_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_load,
            diagnostic.asset, mount, diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

void add_replace_reviewed_evictions_eviction_plan_diagnostics(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResult& result,
    const RuntimeResidentPackageEvictionPlanResult& eviction_plan) {
    for (const auto& diagnostic : eviction_plan.diagnostics) {
        add_candidate_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::eviction_plan,
            diagnostic.mount, diagnostic.code, diagnostic.message);
    }
}

void add_replace_reviewed_evictions_budget_diagnostics(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResult& result,
    const RuntimeResourceResidencyBudgetExecutionResult& budget_execution, RuntimeResidentPackageMountId mount) {
    for (const auto& diagnostic : budget_execution.diagnostics) {
        add_candidate_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::resident_budget, mount,
            diagnostic.code, diagnostic.message);
    }
}

void add_replace_reviewed_evictions_catalog_diagnostics(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResult& result,
    RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase phase,
    const RuntimeResourceCatalogBuildResult& catalog_build, RuntimeResidentPackageMountId mount,
    std::string_view code) {
    for (const auto& diagnostic : catalog_build.diagnostics) {
        add_candidate_resident_replace_reviewed_evictions_diagnostic(result, phase, diagnostic.asset, mount, {},
                                                                     std::string{code}, diagnostic.diagnostic);
    }
}

[[nodiscard]] RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus
map_replace_reviewed_evictions_plan_status(RuntimeResidentPackageEvictionPlanStatus status) noexcept {
    switch (status) {
    case RuntimeResidentPackageEvictionPlanStatus::invalid_candidate_mount_id:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::invalid_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::duplicate_candidate_mount_id:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::duplicate_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::missing_candidate_mount_id:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::missing_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::protected_candidate_mount_id:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::protected_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::catalog_build_failed:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::catalog_refresh_failed;
    case RuntimeResidentPackageEvictionPlanStatus::budget_unreachable:
    case RuntimeResidentPackageEvictionPlanStatus::no_eviction_required:
    case RuntimeResidentPackageEvictionPlanStatus::planned:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::budget_failed;
    }
    return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::budget_failed;
}

[[nodiscard]] bool path_is_under_root(std::string_view path, std::string_view root) {
    if (path.size() <= root.size()) {
        return false;
    }
    if (!path.starts_with(root)) {
        return false;
    }
    const auto separator = path[root.size()];
    return separator == '/';
}

[[nodiscard]] bool ends_with_package_index_extension(std::string_view path) noexcept {
    return path.size() > package_index_extension.size() && path.ends_with(package_index_extension);
}

[[nodiscard]] bool
valid_hot_reload_candidate_review_candidate(const RuntimePackageIndexDiscoveryCandidate& candidate) noexcept {
    return valid_relative_vfs_path(candidate.package_index_path) &&
           ends_with_package_index_extension(candidate.package_index_path) &&
           (candidate.content_root.empty() || valid_relative_vfs_path(candidate.content_root)) &&
           valid_relative_vfs_path(candidate.label);
}

void add_hot_reload_candidate_review_diagnostic(RuntimePackageHotReloadCandidateReviewResult& result, std::string path,
                                                std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageHotReloadCandidateReviewDiagnostic{
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_hot_reload_recook_change_review_diagnostic(RuntimePackageHotReloadRecookChangeReviewResult& result,
                                                    RuntimePackageHotReloadRecookChangeReviewDiagnosticPhase phase,
                                                    AssetId asset, std::string path, std::string code,
                                                    std::string message) {
    result.diagnostics.push_back(RuntimePackageHotReloadRecookChangeReviewDiagnostic{
        .phase = phase,
        .asset = asset,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_hot_reload_replacement_intent_review_diagnostic(
    RuntimePackageHotReloadReplacementIntentReviewResult& result,
    RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase phase, RuntimeResidentPackageMountId mount,
    std::string path, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageHotReloadReplacementIntentReviewDiagnostic{
        .phase = phase,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_hot_reload_recook_replacement_diagnostic(RuntimePackageHotReloadRecookReplacementResult& result,
                                                  RuntimePackageHotReloadRecookReplacementDiagnosticPhase phase,
                                                  AssetId asset, RuntimeResidentPackageMountId mount, std::string path,
                                                  std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageHotReloadRecookReplacementDiagnostic{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

[[nodiscard]] RuntimePackageHotReloadRecookReplacementDiagnosticPhase
map_hot_reload_recook_replacement_commit_diagnostic_phase(
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase phase) noexcept {
    switch (phase) {
    case RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::descriptor:
        return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::replacement_intent_review;
    case RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::resident_replace:
        return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::resident_replace;
    case RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::discovery:
        return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::discovery;
    case RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_selection:
        return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::candidate_selection;
    case RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_load:
        return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::candidate_load;
    case RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_catalog:
        return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::candidate_catalog;
    case RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::eviction_plan:
        return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::eviction_plan;
    case RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::resident_budget:
        return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::resident_budget;
    case RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::catalog_refresh:
        return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::catalog_refresh;
    case RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::resident_commit:
        return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::resident_commit;
    }
    return RuntimePackageHotReloadRecookReplacementDiagnosticPhase::resident_commit;
}

void add_hot_reload_recook_replacement_commit_diagnostics(
    RuntimePackageHotReloadRecookReplacementResult& result,
    const RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult& commit_result) {
    for (const auto& diagnostic : commit_result.diagnostics) {
        add_hot_reload_recook_replacement_diagnostic(
            result, map_hot_reload_recook_replacement_commit_diagnostic_phase(diagnostic.phase), diagnostic.asset,
            diagnostic.mount, diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

[[nodiscard]] bool has_matched_hot_reload_change(const RuntimePackageHotReloadCandidateReviewRow& row,
                                                 std::string_view path,
                                                 RuntimePackageHotReloadCandidateReviewMatchKind kind) noexcept {
    return std::ranges::any_of(row.matched_changes,
                               [path, kind](const auto& change) { return change.path == path && change.kind == kind; });
}

void add_hot_reload_candidate_review_change(RuntimePackageHotReloadCandidateReviewRow& row, std::string_view path,
                                            RuntimePackageHotReloadCandidateReviewMatchKind kind) {
    if (has_matched_hot_reload_change(row, path, kind)) {
        return;
    }
    row.matched_changes.push_back(RuntimePackageHotReloadCandidateReviewChange{
        .path = std::string{path},
        .kind = kind,
    });
}

[[nodiscard]] bool
hot_reload_candidate_matches_changed_path(const RuntimePackageIndexDiscoveryCandidate& candidate,
                                          std::string_view changed_path,
                                          RuntimePackageHotReloadCandidateReviewMatchKind& match_kind) {
    if (!valid_hot_reload_candidate_review_candidate(candidate)) {
        return false;
    }
    if (ends_with_package_index_extension(changed_path)) {
        if (changed_path == candidate.package_index_path) {
            match_kind = RuntimePackageHotReloadCandidateReviewMatchKind::package_index;
            return true;
        }
        return false;
    }

    if (candidate.content_root.empty()) {
        return false;
    }
    if (path_is_under_root(changed_path, candidate.content_root)) {
        match_kind = RuntimePackageHotReloadCandidateReviewMatchKind::content;
        return true;
    }
    return false;
}

[[nodiscard]] bool has_valid_hot_reload_replacement_intent_match(const RuntimePackageHotReloadCandidateReviewRow& row) {
    for (const auto& change : row.matched_changes) {
        if (!valid_relative_vfs_path(change.path)) {
            continue;
        }

        RuntimePackageHotReloadCandidateReviewMatchKind actual_kind{
            RuntimePackageHotReloadCandidateReviewMatchKind::package_index};
        if (hot_reload_candidate_matches_changed_path(row.candidate, change.path, actual_kind) &&
            actual_kind == change.kind) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool valid_runtime_package_mount_overlay(RuntimePackageMountOverlay overlay) noexcept {
    return overlay == RuntimePackageMountOverlay::first_mount_wins ||
           overlay == RuntimePackageMountOverlay::last_mount_wins;
}

[[nodiscard]] std::string package_index_label(std::string_view path, std::string_view root) {
    const auto relative = path.substr(root.size() + 1U);
    return std::string{relative.substr(0, relative.size() - package_index_extension.size())};
}

[[nodiscard]] std::uint32_t next_generation(std::uint32_t generation) noexcept {
    if (generation == std::numeric_limits<std::uint32_t>::max()) {
        return 1U;
    }
    return generation + 1U;
}

[[nodiscard]] RuntimeResidentPackageMountResult mount_result(RuntimeResidentPackageMountId id,
                                                             RuntimeResidentPackageMountStatus status, std::string code,
                                                             std::string message) {
    return RuntimeResidentPackageMountResult{
        .status = status,
        .diagnostic =
            RuntimeResidentPackageMountDiagnostic{
                .mount = id,
                .code = std::move(code),
                .message = std::move(message),
            },
    };
}

[[nodiscard]] bool same_budget(const RuntimeResourceResidencyBudget& lhs,
                               const RuntimeResourceResidencyBudget& rhs) noexcept {
    return lhs.max_resident_content_bytes == rhs.max_resident_content_bytes &&
           lhs.max_resident_asset_records == rhs.max_resident_asset_records;
}

[[nodiscard]] std::vector<RuntimeAssetPackage>
resident_packages_from_mount_set(const RuntimeResidentPackageMountSet& mount_set) {
    std::vector<RuntimeAssetPackage> packages;
    packages.reserve(mount_set.mounts().size());
    for (const auto& mount : mount_set.mounts()) {
        packages.push_back(mount.package);
    }
    return packages;
}

[[nodiscard]] bool contains_mount_id(const std::vector<RuntimeResidentPackageMountId>& ids,
                                     RuntimeResidentPackageMountId id) noexcept {
    for (const auto candidate : ids) {
        if (candidate == id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_mount_id(const RuntimeResidentPackageMountSet& mount_set,
                                RuntimeResidentPackageMountId id) noexcept {
    for (const auto& mounted : mount_set.mounts()) {
        if (mounted.id == id) {
            return true;
        }
    }
    return false;
}

void add_eviction_diagnostic(RuntimeResidentPackageEvictionPlanResult& result, RuntimeResidentPackageMountId mount,
                             std::string code, std::string message) {
    result.diagnostics.push_back(RuntimeResidentPackageEvictionPlanDiagnostic{
        .mount = mount,
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_reviewed_eviction_commit_diagnostic(RuntimeResidentPackageReviewedEvictionCommitResult& result,
                                             RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhase phase,
                                             AssetId asset, RuntimeResidentPackageMountId mount, std::string code,
                                             std::string message) {
    result.diagnostics.push_back(RuntimeResidentPackageReviewedEvictionCommitDiagnostic{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_reviewed_eviction_commit_plan_diagnostics(RuntimeResidentPackageReviewedEvictionCommitResult& result,
                                                   const RuntimeResidentPackageEvictionPlanResult& eviction_plan) {
    for (const auto& diagnostic : eviction_plan.diagnostics) {
        add_reviewed_eviction_commit_diagnostic(
            result, RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhase::eviction_plan, {}, diagnostic.mount,
            diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_eviction_commit_budget_diagnostics(
    RuntimeResidentPackageReviewedEvictionCommitResult& result,
    const RuntimeResourceResidencyBudgetExecutionResult& budget_execution) {
    for (const auto& diagnostic : budget_execution.diagnostics) {
        add_reviewed_eviction_commit_diagnostic(
            result, RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhase::resident_budget, {}, {},
            diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_eviction_commit_catalog_diagnostics(RuntimeResidentPackageReviewedEvictionCommitResult& result,
                                                      const RuntimeResourceCatalogBuildResult& catalog_build) {
    for (const auto& diagnostic : catalog_build.diagnostics) {
        add_reviewed_eviction_commit_diagnostic(
            result, RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhase::catalog_refresh, diagnostic.asset, {},
            "catalog-build-failed", diagnostic.diagnostic);
    }
}

[[nodiscard]] RuntimeResidentPackageReviewedEvictionCommitStatus
map_reviewed_eviction_commit_plan_status(RuntimeResidentPackageEvictionPlanStatus status) noexcept {
    switch (status) {
    case RuntimeResidentPackageEvictionPlanStatus::invalid_candidate_mount_id:
        return RuntimeResidentPackageReviewedEvictionCommitStatus::invalid_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::duplicate_candidate_mount_id:
        return RuntimeResidentPackageReviewedEvictionCommitStatus::duplicate_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::missing_candidate_mount_id:
        return RuntimeResidentPackageReviewedEvictionCommitStatus::missing_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::protected_candidate_mount_id:
        return RuntimeResidentPackageReviewedEvictionCommitStatus::protected_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatus::budget_unreachable:
        return RuntimeResidentPackageReviewedEvictionCommitStatus::budget_failed;
    case RuntimeResidentPackageEvictionPlanStatus::catalog_build_failed:
        return RuntimeResidentPackageReviewedEvictionCommitStatus::catalog_refresh_failed;
    case RuntimeResidentPackageEvictionPlanStatus::no_eviction_required:
        return RuntimeResidentPackageReviewedEvictionCommitStatus::no_eviction_required;
    case RuntimeResidentPackageEvictionPlanStatus::planned:
        return RuntimeResidentPackageReviewedEvictionCommitStatus::committed;
    }
    return RuntimeResidentPackageReviewedEvictionCommitStatus::budget_failed;
}

} // namespace

bool RuntimePackageIndexDiscoveryResult::succeeded() const noexcept {
    return status == RuntimePackageIndexDiscoveryStatus::discovered ||
           status == RuntimePackageIndexDiscoveryStatus::no_candidates;
}

bool RuntimePackageCandidateLoadResult::succeeded() const noexcept {
    return status == RuntimePackageCandidateLoadStatus::loaded && loaded_package.succeeded();
}

bool RuntimePackageHotReloadCandidateReviewResult::succeeded() const noexcept {
    return status == RuntimePackageHotReloadCandidateReviewStatus::review_ready && !rows.empty();
}

RuntimePackageHotReloadCandidateReviewResult
plan_runtime_package_hot_reload_candidate_review(const RuntimePackageHotReloadCandidateReviewDesc& desc) {
    RuntimePackageHotReloadCandidateReviewResult result;
    result.changed_path_count = desc.changed_paths.size();

    if (desc.changed_paths.empty()) {
        result.status = RuntimePackageHotReloadCandidateReviewStatus::no_changes;
        return result;
    }

    std::vector<bool> valid_changed_paths(desc.changed_paths.size(), false);
    std::vector<bool> matched_changed_paths(desc.changed_paths.size(), false);
    for (std::size_t changed_path_index = 0; changed_path_index < desc.changed_paths.size(); ++changed_path_index) {
        valid_changed_paths[changed_path_index] = valid_relative_vfs_path(desc.changed_paths[changed_path_index]);
    }

    std::vector<std::string> reviewed_candidate_paths;
    reviewed_candidate_paths.reserve(desc.candidates.size());
    for (const auto& candidate : desc.candidates) {
        if (std::ranges::find(reviewed_candidate_paths, candidate.package_index_path) !=
            reviewed_candidate_paths.end()) {
            continue;
        }

        RuntimePackageHotReloadCandidateReviewRow row;
        row.candidate = candidate;
        for (std::size_t changed_path_index = 0; changed_path_index < desc.changed_paths.size(); ++changed_path_index) {
            if (!valid_changed_paths[changed_path_index]) {
                continue;
            }

            RuntimePackageHotReloadCandidateReviewMatchKind match_kind{
                RuntimePackageHotReloadCandidateReviewMatchKind::package_index};
            if (hot_reload_candidate_matches_changed_path(candidate, desc.changed_paths[changed_path_index],
                                                          match_kind)) {
                matched_changed_paths[changed_path_index] = true;
                add_hot_reload_candidate_review_change(row, desc.changed_paths[changed_path_index], match_kind);
            }
        }

        if (!row.matched_changes.empty()) {
            reviewed_candidate_paths.push_back(candidate.package_index_path);
            result.rows.push_back(std::move(row));
        }
    }

    for (std::size_t changed_path_index = 0; changed_path_index < desc.changed_paths.size(); ++changed_path_index) {
        if (!valid_changed_paths[changed_path_index]) {
            ++result.invalid_changed_path_count;
            add_hot_reload_candidate_review_diagnostic(
                result, desc.changed_paths[changed_path_index], "invalid-changed-path",
                "hot-reload changed paths must be non-empty relative VFS paths without backslashes or dot segments");
            continue;
        }
        if (matched_changed_paths[changed_path_index]) {
            ++result.matched_changed_path_count;
            continue;
        }

        ++result.unmatched_changed_path_count;
        add_hot_reload_candidate_review_diagnostic(
            result, desc.changed_paths[changed_path_index], "unmatched-changed-path",
            "hot-reload changed path did not match any reviewed runtime package index candidate");
    }

    result.review_candidate_count = result.rows.size();
    std::ranges::sort(result.rows, [](const RuntimePackageHotReloadCandidateReviewRow& lhs,
                                      const RuntimePackageHotReloadCandidateReviewRow& rhs) {
        return lhs.candidate.package_index_path < rhs.candidate.package_index_path;
    });
    if (!result.rows.empty()) {
        result.status = RuntimePackageHotReloadCandidateReviewStatus::review_ready;
    } else if (desc.candidates.empty()) {
        result.status = RuntimePackageHotReloadCandidateReviewStatus::no_candidates;
    } else {
        result.status = RuntimePackageHotReloadCandidateReviewStatus::no_matches;
    }

    return result;
}

bool RuntimePackageHotReloadRecookChangeReviewResult::succeeded() const noexcept {
    return status == RuntimePackageHotReloadRecookChangeReviewStatus::review_ready && candidate_review.succeeded();
}

RuntimePackageHotReloadRecookChangeReviewResult
plan_runtime_package_hot_reload_recook_change_review(const RuntimePackageHotReloadRecookChangeReviewDesc& desc) {
    RuntimePackageHotReloadRecookChangeReviewResult result;
    result.recook_apply_result_count = desc.recook_apply_results.size();

    if (desc.recook_apply_results.empty()) {
        result.status = RuntimePackageHotReloadRecookChangeReviewStatus::no_recook_changes;
        return result;
    }

    result.candidate_review_desc.candidates = desc.candidates;
    result.candidate_review_desc.changed_paths.reserve(desc.recook_apply_results.size());
    for (const auto& apply_result : desc.recook_apply_results) {
        const bool is_staged = apply_result.kind == AssetHotReloadApplyResultKind::staged;
        const bool is_applied = apply_result.kind == AssetHotReloadApplyResultKind::applied;
        const bool is_failed = apply_result.kind == AssetHotReloadApplyResultKind::failed_rolled_back;

        if (is_staged) {
            ++result.staged_recook_change_count;
        } else if (is_applied) {
            ++result.applied_recook_change_count;
        } else if (is_failed) {
            ++result.failed_recook_apply_result_count;
        }

        if (apply_result.asset.value == 0) {
            ++result.invalid_recook_apply_result_count;
            result.status = RuntimePackageHotReloadRecookChangeReviewStatus::invalid_recook_apply_result;
            add_hot_reload_recook_change_review_diagnostic(
                result, RuntimePackageHotReloadRecookChangeReviewDiagnosticPhase::recook_apply_result,
                apply_result.asset, apply_result.path, "invalid-recook-apply-result-asset",
                "recook apply-result rows must reference a non-zero asset id");
            return result;
        }
        if (!is_staged && !is_applied && !is_failed) {
            ++result.invalid_recook_apply_result_count;
            result.status = RuntimePackageHotReloadRecookChangeReviewStatus::invalid_recook_apply_result;
            add_hot_reload_recook_change_review_diagnostic(
                result, RuntimePackageHotReloadRecookChangeReviewDiagnosticPhase::recook_apply_result,
                apply_result.asset, apply_result.path, "invalid-recook-result-kind",
                "recook apply-result rows must be staged, applied, or failed_rolled_back");
            return result;
        }
        if (!valid_relative_vfs_path(apply_result.path)) {
            ++result.invalid_recook_apply_result_count;
            result.status = RuntimePackageHotReloadRecookChangeReviewStatus::invalid_recook_apply_result;
            add_hot_reload_recook_change_review_diagnostic(
                result, RuntimePackageHotReloadRecookChangeReviewDiagnosticPhase::recook_apply_result,
                apply_result.asset, apply_result.path, "invalid-recook-result-path",
                "recook apply-result paths must be caller-reviewed relative runtime VFS paths");
            return result;
        }
        if (is_applied && apply_result.active_revision == 0) {
            ++result.invalid_recook_apply_result_count;
            result.status = RuntimePackageHotReloadRecookChangeReviewStatus::invalid_recook_apply_result;
            add_hot_reload_recook_change_review_diagnostic(
                result, RuntimePackageHotReloadRecookChangeReviewDiagnosticPhase::recook_apply_result,
                apply_result.asset, apply_result.path, "invalid-recook-apply-result-revision",
                "applied recook apply-result rows must report a non-zero active revision");
            return result;
        }
        if (is_failed) {
            result.status = RuntimePackageHotReloadRecookChangeReviewStatus::failed_recook_apply_result;
            add_hot_reload_recook_change_review_diagnostic(
                result, RuntimePackageHotReloadRecookChangeReviewDiagnosticPhase::recook_apply_result,
                apply_result.asset, apply_result.path, "recook-failed",
                apply_result.diagnostic.empty() ? "recook apply-result row failed and was rolled back"
                                                : apply_result.diagnostic);
            return result;
        }

        result.candidate_review_desc.changed_paths.push_back(apply_result.path);
    }

    result.accepted_recook_change_count = result.candidate_review_desc.changed_paths.size();
    if (result.candidate_review_desc.changed_paths.empty()) {
        result.status = RuntimePackageHotReloadRecookChangeReviewStatus::no_recook_changes;
        return result;
    }

    result.invoked_candidate_review = true;
    result.candidate_review = plan_runtime_package_hot_reload_candidate_review(result.candidate_review_desc);
    if (!result.candidate_review.succeeded()) {
        result.status = RuntimePackageHotReloadRecookChangeReviewStatus::candidate_review_failed;
        for (const auto& diagnostic : result.candidate_review.diagnostics) {
            add_hot_reload_recook_change_review_diagnostic(
                result, RuntimePackageHotReloadRecookChangeReviewDiagnosticPhase::candidate_review, {}, diagnostic.path,
                diagnostic.code, diagnostic.message);
        }
        return result;
    }

    result.status = RuntimePackageHotReloadRecookChangeReviewStatus::review_ready;
    return result;
}

bool RuntimePackageHotReloadReplacementIntentReviewResult::succeeded() const noexcept {
    return status == RuntimePackageHotReloadReplacementIntentReviewStatus::review_ready;
}

RuntimePackageHotReloadReplacementIntentReviewResult plan_runtime_package_hot_reload_replacement_intent_review(
    const RuntimePackageHotReloadReplacementIntentReviewDesc& desc) {
    RuntimePackageHotReloadReplacementIntentReviewResult result;
    result.matched_change_count = desc.selected_candidate.matched_changes.size();
    result.eviction_candidate_count = desc.eviction_candidate_unmount_order.size();
    result.protected_mount_count = desc.protected_mount_ids.size();

    if (!valid_hot_reload_candidate_review_candidate(desc.selected_candidate.candidate)) {
        result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::invalid_candidate;
        add_hot_reload_replacement_intent_review_diagnostic(
            result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::candidate, desc.mount_id,
            desc.selected_candidate.candidate.package_index_path, "invalid-selected-candidate",
            "selected hot-reload candidate must be a reviewed relative .geindex candidate with valid roots");
        return result;
    }

    if (!has_valid_hot_reload_replacement_intent_match(desc.selected_candidate)) {
        result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::missing_matched_change;
        add_hot_reload_replacement_intent_review_diagnostic(
            result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::candidate, desc.mount_id,
            desc.selected_candidate.candidate.package_index_path, "missing-matched-change",
            "selected hot-reload candidate requires at least one reviewed matching changed path");
        return result;
    }

    const auto discovery_root = trim_trailing_slashes(desc.discovery.root);
    if (!valid_relative_vfs_path(discovery_root)) {
        result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::invalid_descriptor;
        add_hot_reload_replacement_intent_review_diagnostic(
            result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::descriptor, desc.mount_id,
            discovery_root, "invalid-discovery-root",
            "runtime package replacement intent discovery root must be a non-empty relative VFS directory");
        return result;
    }
    if (!path_is_under_root(desc.selected_candidate.candidate.package_index_path, discovery_root)) {
        result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::invalid_descriptor;
        add_hot_reload_replacement_intent_review_diagnostic(
            result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::descriptor, desc.mount_id,
            desc.selected_candidate.candidate.package_index_path, "candidate-outside-discovery-root",
            "selected hot-reload candidate package index must stay under the reviewed discovery root");
        return result;
    }

    const auto discovery_content_root = trim_trailing_slashes(desc.discovery.content_root);
    if (!discovery_content_root.empty() && !valid_relative_vfs_path(discovery_content_root)) {
        result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::invalid_descriptor;
        add_hot_reload_replacement_intent_review_diagnostic(
            result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::descriptor, desc.mount_id,
            discovery_content_root, "invalid-discovery-content-root",
            "runtime package replacement intent discovery content root must be empty or a relative VFS directory");
        return result;
    }
    const auto candidate_content_root = trim_trailing_slashes(desc.selected_candidate.candidate.content_root);
    if (candidate_content_root != discovery_content_root) {
        result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::invalid_descriptor;
        add_hot_reload_replacement_intent_review_diagnostic(
            result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::descriptor, desc.mount_id,
            desc.selected_candidate.candidate.content_root, "candidate-content-root-mismatch",
            "selected hot-reload candidate content root must match the reviewed discovery content root");
        return result;
    }

    if (!valid_runtime_package_mount_overlay(desc.overlay)) {
        result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::invalid_overlay;
        add_hot_reload_replacement_intent_review_diagnostic(
            result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::descriptor, desc.mount_id, {},
            "invalid-overlay", "runtime package replacement intent overlay must be a known mount overlay mode");
        return result;
    }

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::invalid_mount_id;
        add_hot_reload_replacement_intent_review_diagnostic(
            result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::resident_replace, desc.mount_id, {},
            "invalid-mount-id", "resident package mount ids must be non-zero");
        return result;
    }
    if (!contains_mount_id(desc.reviewed_existing_mount_ids, desc.mount_id)) {
        result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::missing_mount_id;
        add_hot_reload_replacement_intent_review_diagnostic(
            result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::resident_replace, desc.mount_id, {},
            "missing-mount-id", "resident package mount id is not present in the reviewed existing mount set");
        return result;
    }

    std::vector<RuntimeResidentPackageMountId> seen_eviction_candidates;
    seen_eviction_candidates.reserve(desc.eviction_candidate_unmount_order.size());
    for (const auto candidate : desc.eviction_candidate_unmount_order) {
        if (candidate.value == 0) {
            result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::invalid_eviction_candidate_mount_id;
            add_hot_reload_replacement_intent_review_diagnostic(
                result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::eviction_plan, candidate, {},
                "invalid-eviction-candidate-mount-id", "reviewed eviction candidate mount ids must be non-zero");
            return result;
        }
        if (contains_mount_id(seen_eviction_candidates, candidate)) {
            result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::duplicate_eviction_candidate_mount_id;
            add_hot_reload_replacement_intent_review_diagnostic(
                result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::eviction_plan, candidate, {},
                "duplicate-eviction-candidate-mount-id", "reviewed eviction candidate mount id appears more than once");
            return result;
        }
        if (!contains_mount_id(desc.reviewed_existing_mount_ids, candidate)) {
            result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::missing_eviction_candidate_mount_id;
            add_hot_reload_replacement_intent_review_diagnostic(
                result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::eviction_plan, candidate, {},
                "missing-eviction-candidate-mount-id",
                "reviewed eviction candidate mount id is not present in the reviewed existing mount set");
            return result;
        }
        if (candidate == desc.mount_id || contains_mount_id(desc.protected_mount_ids, candidate)) {
            result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::protected_eviction_candidate_mount_id;
            add_hot_reload_replacement_intent_review_diagnostic(
                result, RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhase::eviction_plan, candidate, {},
                "protected-eviction-candidate-mount-id",
                "reviewed eviction candidate mount id is protected from eviction");
            return result;
        }
        seen_eviction_candidates.push_back(candidate);
    }

    result.replacement_desc = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDesc{
        .discovery =
            RuntimePackageIndexDiscoveryDesc{
                .root = discovery_root,
                .content_root = discovery_content_root,
            },
        .selected_package_index_path = desc.selected_candidate.candidate.package_index_path,
        .mount_id = desc.mount_id,
        .overlay = desc.overlay,
        .budget = desc.budget,
        .eviction_candidate_unmount_order = desc.eviction_candidate_unmount_order,
        .protected_mount_ids = desc.protected_mount_ids,
    };
    result.status = RuntimePackageHotReloadReplacementIntentReviewStatus::review_ready;
    return result;
}

bool RuntimePackageHotReloadRecookReplacementResult::succeeded() const noexcept {
    return status == RuntimePackageHotReloadRecookReplacementStatus::committed && committed &&
           replacement_commit.succeeded();
}

RuntimePackageHotReloadRecookReplacementResult
commit_runtime_package_hot_reload_recook_replacement(IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set,
                                                     RuntimeResidentCatalogCache& catalog_cache,
                                                     const RuntimePackageHotReloadRecookReplacementDesc& desc) {
    RuntimePackageHotReloadRecookReplacementResult result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    result.recook_change_review =
        plan_runtime_package_hot_reload_recook_change_review(RuntimePackageHotReloadRecookChangeReviewDesc{
            .recook_apply_results = desc.recook_apply_results,
            .candidates = desc.candidates,
        });
    result.invoked_candidate_review = result.recook_change_review.invoked_candidate_review;
    if (!result.recook_change_review.succeeded()) {
        result.status = RuntimePackageHotReloadRecookReplacementStatus::recook_change_review_failed;
        for (const auto& diagnostic : result.recook_change_review.diagnostics) {
            add_hot_reload_recook_replacement_diagnostic(
                result, RuntimePackageHotReloadRecookReplacementDiagnosticPhase::recook_change_review, diagnostic.asset,
                {}, diagnostic.path, diagnostic.code, diagnostic.message);
        }
        return result;
    }

    const auto selected =
        std::ranges::find_if(result.recook_change_review.candidate_review.rows,
                             [&desc](const RuntimePackageHotReloadCandidateReviewRow& row) {
                                 return row.candidate.package_index_path == desc.selected_package_index_path;
                             });
    if (selected == result.recook_change_review.candidate_review.rows.end()) {
        result.status = RuntimePackageHotReloadRecookReplacementStatus::candidate_not_found;
        add_hot_reload_recook_replacement_diagnostic(
            result, RuntimePackageHotReloadRecookReplacementDiagnosticPhase::candidate_selection, {}, desc.mount_id,
            desc.selected_package_index_path, "candidate-not-found",
            "selected runtime package index was not present in reviewed recook hot-reload candidates");
        return result;
    }
    result.selected_candidate = *selected;
    result.selected_candidate_count = 1U;

    result.invoked_replacement_intent_review = true;
    result.replacement_intent_review =
        plan_runtime_package_hot_reload_replacement_intent_review(RuntimePackageHotReloadReplacementIntentReviewDesc{
            .selected_candidate = result.selected_candidate,
            .discovery = desc.discovery,
            .mount_id = desc.mount_id,
            .reviewed_existing_mount_ids = desc.reviewed_existing_mount_ids,
            .overlay = desc.overlay,
            .budget = desc.budget,
            .eviction_candidate_unmount_order = desc.eviction_candidate_unmount_order,
            .protected_mount_ids = desc.protected_mount_ids,
        });
    if (!result.replacement_intent_review.succeeded()) {
        result.status = RuntimePackageHotReloadRecookReplacementStatus::replacement_intent_review_failed;
        for (const auto& diagnostic : result.replacement_intent_review.diagnostics) {
            add_hot_reload_recook_replacement_diagnostic(
                result, RuntimePackageHotReloadRecookReplacementDiagnosticPhase::replacement_intent_review, {},
                diagnostic.mount, diagnostic.path, diagnostic.code, diagnostic.message);
        }
        return result;
    }

    result.invoked_resident_commit = true;
    result.replacement_commit = commit_runtime_package_discovery_resident_replace_with_reviewed_evictions(
        filesystem, mount_set, catalog_cache, result.replacement_intent_review.replacement_desc);
    result.loaded_record_count = result.replacement_commit.loaded_record_count;
    result.loaded_resident_bytes = result.replacement_commit.loaded_resident_bytes;
    result.projected_resident_bytes = result.replacement_commit.projected_resident_bytes;
    result.previous_mount_generation = result.replacement_commit.previous_mount_generation;
    result.mount_generation = result.replacement_commit.mount_generation;
    result.previous_mount_count = result.replacement_commit.previous_mount_count;
    result.mounted_package_count = result.replacement_commit.mounted_package_count;
    result.evicted_mount_count = result.replacement_commit.evicted_mount_count;
    result.invoked_package_load = result.replacement_commit.candidate_resident_replace.invoked_candidate_load;
    if (!result.replacement_commit.succeeded()) {
        result.status = RuntimePackageHotReloadRecookReplacementStatus::replacement_commit_failed;
        add_hot_reload_recook_replacement_commit_diagnostics(result, result.replacement_commit);
        if (result.diagnostics.empty()) {
            add_hot_reload_recook_replacement_diagnostic(
                result, RuntimePackageHotReloadRecookReplacementDiagnosticPhase::resident_replace, {}, desc.mount_id,
                desc.selected_package_index_path, "replacement-commit-failed",
                "reviewed recook hot-reload replacement failed before committing resident state");
        }
        return result;
    }

    result.status = RuntimePackageHotReloadRecookReplacementStatus::committed;
    result.committed = true;
    return result;
}

RuntimePackageIndexDiscoveryResult discover_runtime_package_indexes(const IFileSystem& filesystem,
                                                                    const RuntimePackageIndexDiscoveryDesc& desc) {
    RuntimePackageIndexDiscoveryResult result;
    result.root = trim_trailing_slashes(desc.root);

    if (!valid_relative_vfs_path(result.root)) {
        result.status = RuntimePackageIndexDiscoveryStatus::invalid_descriptor;
        add_discovery_diagnostic(result, result.root, "invalid-root",
                                 "runtime package discovery root must be a non-empty relative VFS directory");
        return result;
    }

    const auto content_root = trim_trailing_slashes(desc.content_root);
    if (!content_root.empty() && !valid_relative_vfs_path(content_root)) {
        result.status = RuntimePackageIndexDiscoveryStatus::invalid_descriptor;
        add_discovery_diagnostic(result, content_root, "invalid-content-root",
                                 "runtime package discovery content root must be a relative VFS directory");
        return result;
    }

    try {
        if (!filesystem.is_directory(result.root)) {
            result.status = RuntimePackageIndexDiscoveryStatus::missing_root;
            add_discovery_diagnostic(result, result.root, "missing-root",
                                     "runtime package discovery root is not a directory");
            return result;
        }
    } catch (const std::exception& exception) {
        result.status = RuntimePackageIndexDiscoveryStatus::scan_failed;
        add_discovery_diagnostic(result, result.root, "scan-failed", exception.what());
        return result;
    }

    std::vector<std::string> files;
    try {
        files = filesystem.list_files(result.root);
    } catch (const std::exception& exception) {
        result.status = RuntimePackageIndexDiscoveryStatus::scan_failed;
        add_discovery_diagnostic(result, result.root, "scan-failed", exception.what());
        return result;
    }

    for (const auto& file : files) {
        if (!path_is_under_root(file, result.root) || !ends_with_package_index_extension(file)) {
            continue;
        }
        const auto label = package_index_label(file, result.root);
        if (!valid_relative_vfs_path(file) || label.empty()) {
            add_discovery_diagnostic(result, file, "invalid-package-index-path",
                                     "runtime package index candidates must be relative .geindex file paths");
            continue;
        }
        result.candidates.push_back(RuntimePackageIndexDiscoveryCandidate{
            .package_index_path = file,
            .content_root = content_root,
            .label = label,
        });
    }

    std::ranges::sort(result.candidates, [](const RuntimePackageIndexDiscoveryCandidate& lhs,
                                            const RuntimePackageIndexDiscoveryCandidate& rhs) {
        return lhs.package_index_path < rhs.package_index_path;
    });
    const auto duplicate_begin =
        std::ranges::unique(result.candidates, {}, &RuntimePackageIndexDiscoveryCandidate::package_index_path).begin();
    result.candidates.erase(duplicate_begin, result.candidates.end());
    result.status = result.candidates.empty() ? RuntimePackageIndexDiscoveryStatus::no_candidates
                                              : RuntimePackageIndexDiscoveryStatus::discovered;
    return result;
}

RuntimePackageCandidateLoadResult
load_runtime_package_candidate(IFileSystem& filesystem, const RuntimePackageIndexDiscoveryCandidate& candidate) {
    RuntimePackageCandidateLoadResult result;
    result.candidate = candidate;

    if (!valid_relative_vfs_path(candidate.package_index_path) ||
        !ends_with_package_index_extension(candidate.package_index_path)) {
        result.status = RuntimePackageCandidateLoadStatus::invalid_candidate;
        add_candidate_load_diagnostic(result, candidate.package_index_path, "invalid-package-index-path",
                                      "runtime package load candidate must reference a relative .geindex file path");
        return result;
    }
    if (!candidate.content_root.empty() && !valid_relative_vfs_path(candidate.content_root)) {
        result.status = RuntimePackageCandidateLoadStatus::invalid_candidate;
        add_candidate_load_diagnostic(result, candidate.content_root, "invalid-content-root",
                                      "runtime package load candidate content root must be a relative VFS directory");
        return result;
    }
    if (!valid_relative_vfs_path(candidate.label)) {
        result.status = RuntimePackageCandidateLoadStatus::invalid_candidate;
        add_candidate_load_diagnostic(result, candidate.label, "invalid-label",
                                      "runtime package load candidate label must be a non-empty relative identifier");
        return result;
    }

    result.package_desc = RuntimeAssetPackageDesc{
        .index_path = candidate.package_index_path,
        .content_root = candidate.content_root,
    };

    try {
        result.invoked_load = true;
        result.loaded_package = load_runtime_asset_package(filesystem, result.package_desc);
    } catch (const std::invalid_argument& exception) {
        result.status = RuntimePackageCandidateLoadStatus::package_load_failed;
        add_candidate_load_diagnostic(result, candidate.package_index_path, "package-index-invalid", exception.what());
        return result;
    } catch (const std::exception& exception) {
        result.status = RuntimePackageCandidateLoadStatus::read_failed;
        add_candidate_load_diagnostic(result, candidate.package_index_path, "package-read-failed", exception.what());
        return result;
    }

    if (!result.loaded_package.succeeded()) {
        result.status = RuntimePackageCandidateLoadStatus::package_load_failed;
        for (const auto& failure : result.loaded_package.failures) {
            add_candidate_load_diagnostic(result, failure.asset, failure.path, "package-load-failed",
                                          failure.diagnostic);
        }
        return result;
    }

    result.status = RuntimePackageCandidateLoadStatus::loaded;
    result.loaded_record_count = result.loaded_package.package.records().size();
    result.estimated_resident_bytes = estimate_runtime_asset_package_resident_bytes(result.loaded_package.package);
    return result;
}

const std::vector<RuntimeResourceRecord>& RuntimeResourceCatalog::records() const noexcept {
    return records_;
}

std::uint32_t RuntimeResourceCatalog::generation() const noexcept {
    return generation_;
}

RuntimeResourceCatalogBuildResult build_runtime_resource_catalog(RuntimeResourceCatalog& catalog,
                                                                 const RuntimeAssetPackage& package) {
    RuntimeResourceCatalogBuildResult result;
    std::unordered_set<AssetId, AssetIdHash> assets;
    for (const auto& record : package.records()) {
        if (!assets.insert(record.asset).second) {
            result.diagnostics.push_back(RuntimeResourceCatalogBuildDiagnostic{
                .asset = record.asset,
                .diagnostic = "duplicate runtime asset record",
            });
        }
    }
    if (!result.succeeded()) {
        return result;
    }

    const auto generation = next_generation(catalog.generation_);
    std::vector<RuntimeResourceRecord> records;
    records.reserve(package.records().size());
    for (const auto& record : package.records()) {
        records.push_back(RuntimeResourceRecord{
            .handle = RuntimeResourceHandle{.index = static_cast<std::uint32_t>(records.size() + 1U),
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

RuntimeResourceCatalogBuildResult
build_runtime_resource_catalog_from_resident_mounts(RuntimeResourceCatalog& catalog,
                                                    const std::vector<RuntimeAssetPackage>& mounts,
                                                    RuntimePackageMountOverlay overlay) {
    auto merged = merge_runtime_asset_packages_overlay(mounts, overlay);
    return build_runtime_resource_catalog(catalog, merged);
}

bool RuntimeResidentPackageMountResult::succeeded() const noexcept {
    return status == RuntimeResidentPackageMountStatus::mounted ||
           status == RuntimeResidentPackageMountStatus::unmounted;
}

const std::vector<RuntimeResidentPackageMountRecord>& RuntimeResidentPackageMountSet::mounts() const noexcept {
    return mounts_;
}

std::uint32_t RuntimeResidentPackageMountSet::generation() const noexcept {
    return generation_;
}

RuntimeResidentPackageMountResult RuntimeResidentPackageMountSet::mount(RuntimeResidentPackageMountRecord record) {
    if (record.id.value == 0) {
        return mount_result(record.id, RuntimeResidentPackageMountStatus::invalid_mount_id, "invalid-mount-id",
                            "resident package mount ids must be non-zero");
    }
    for (const auto& mounted : mounts_) {
        if (mounted.id == record.id) {
            return mount_result(record.id, RuntimeResidentPackageMountStatus::duplicate_mount_id, "duplicate-mount-id",
                                "resident package mount id is already mounted");
        }
    }

    const auto id = record.id;
    mounts_.push_back(std::move(record));
    generation_ = next_generation(generation_);
    return mount_result(id, RuntimeResidentPackageMountStatus::mounted, {}, {});
}

RuntimeResidentPackageMountResult RuntimeResidentPackageMountSet::unmount(RuntimeResidentPackageMountId id) {
    if (id.value == 0) {
        return mount_result(id, RuntimeResidentPackageMountStatus::invalid_mount_id, "invalid-mount-id",
                            "resident package mount ids must be non-zero");
    }
    for (auto it = mounts_.begin(); it != mounts_.end(); ++it) {
        if (it->id == id) {
            mounts_.erase(it);
            generation_ = next_generation(generation_);
            return mount_result(id, RuntimeResidentPackageMountStatus::unmounted, {}, {});
        }
    }
    return mount_result(id, RuntimeResidentPackageMountStatus::missing_mount_id, "missing-mount-id",
                        "resident package mount id is not mounted");
}

bool RuntimeResidentPackageMountCatalogBuildResult::succeeded() const noexcept {
    return catalog_build.succeeded();
}

RuntimeResidentPackageMountCatalogBuildResult
build_runtime_resource_catalog_from_resident_mount_set(RuntimeResourceCatalog& catalog,
                                                       const RuntimeResidentPackageMountSet& mount_set,
                                                       RuntimePackageMountOverlay overlay) {
    auto packages = resident_packages_from_mount_set(mount_set);

    RuntimeResidentPackageMountCatalogBuildResult result;
    result.mounted_package_count = packages.size();
    result.mount_generation = mount_set.generation();
    result.catalog_build = build_runtime_resource_catalog_from_resident_mounts(catalog, packages, overlay);
    return result;
}

RuntimeResourceResidencyBudgetExecutionResult
evaluate_runtime_resource_residency_budget(const RuntimeAssetPackage& resident_package_view,
                                           const RuntimeResourceResidencyBudget& budget) {
    RuntimeResourceResidencyBudgetExecutionResult result;
    result.estimated_resident_content_bytes = estimate_runtime_asset_package_resident_bytes(resident_package_view);
    result.resident_asset_record_count = resident_package_view.records().size();
    result.within_budget = true;

    if (budget.max_resident_content_bytes.has_value() &&
        result.estimated_resident_content_bytes > *budget.max_resident_content_bytes) {
        result.within_budget = false;
        result.diagnostics.push_back(RuntimeResourceResidencyBudgetDiagnostic{
            .code = "resident-content-bytes-exceed-budget",
            .message = "runtime package content byte estimate exceeds max_resident_content_bytes",
        });
    }
    if (budget.max_resident_asset_records.has_value() &&
        result.resident_asset_record_count > *budget.max_resident_asset_records) {
        result.within_budget = false;
        result.diagnostics.push_back(RuntimeResourceResidencyBudgetDiagnostic{
            .code = "resident-asset-record-count-exceeds-budget",
            .message = "runtime package asset record count exceeds max_resident_asset_records",
        });
    }
    return result;
}

RuntimeResidentMountCatalogBudgetBundleResult build_runtime_resource_catalog_from_resident_mounts_with_budget(
    RuntimeResourceCatalog& catalog, const std::vector<RuntimeAssetPackage>& mounts, RuntimePackageMountOverlay overlay,
    const RuntimeResourceResidencyBudget& budget) {
    RuntimeResidentMountCatalogBudgetBundleResult out;
    auto merged = merge_runtime_asset_packages_overlay(mounts, overlay);
    out.budget_execution = evaluate_runtime_resource_residency_budget(merged, budget);
    if (!out.budget_execution.within_budget) {
        return out;
    }
    out.catalog_build = build_runtime_resource_catalog(catalog, merged);
    out.invoked_catalog_build = true;
    return out;
}

bool RuntimeResidentCatalogCacheRefreshResult::succeeded() const noexcept {
    return status == RuntimeResidentCatalogCacheStatus::rebuilt ||
           status == RuntimeResidentCatalogCacheStatus::cache_hit;
}

const RuntimeResourceCatalog& RuntimeResidentCatalogCache::catalog() const noexcept {
    return catalog_;
}

bool RuntimeResidentCatalogCache::has_value() const noexcept {
    return has_cache_;
}

std::uint32_t RuntimeResidentCatalogCache::cached_mount_generation() const noexcept {
    return cached_mount_generation_;
}

RuntimeResidentCatalogCacheRefreshResult
RuntimeResidentCatalogCache::refresh(const RuntimeResidentPackageMountSet& mount_set,
                                     RuntimePackageMountOverlay overlay, const RuntimeResourceResidencyBudget& budget) {
    RuntimeResidentCatalogCacheRefreshResult result;
    result.mounted_package_count = mount_set.mounts().size();
    result.mount_generation = mount_set.generation();
    result.previous_catalog_generation = catalog_.generation();
    result.catalog_generation = catalog_.generation();

    if (has_cache_ && cached_mount_generation_ == mount_set.generation() && cached_overlay_ == overlay &&
        same_budget(cached_budget_, budget)) {
        result.status = RuntimeResidentCatalogCacheStatus::cache_hit;
        result.budget_execution = cached_budget_execution_;
        result.reused_cache = true;
        return result;
    }

    auto packages = resident_packages_from_mount_set(mount_set);
    auto merged = merge_runtime_asset_packages_overlay(packages, overlay);
    result.budget_execution = evaluate_runtime_resource_residency_budget(merged, budget);
    if (!result.budget_execution.within_budget) {
        result.status = RuntimeResidentCatalogCacheStatus::budget_failed;
        return result;
    }

    RuntimeResourceCatalog replacement = catalog_;
    result.catalog_build = build_runtime_resource_catalog(replacement, merged);
    result.invoked_catalog_build = true;
    if (!result.catalog_build.succeeded()) {
        result.status = RuntimeResidentCatalogCacheStatus::catalog_build_failed;
        return result;
    }

    catalog_ = std::move(replacement);
    cached_budget_execution_ = result.budget_execution;
    cached_budget_ = budget;
    cached_overlay_ = overlay;
    cached_mount_generation_ = mount_set.generation();
    has_cache_ = true;

    result.status = RuntimeResidentCatalogCacheStatus::rebuilt;
    result.catalog_generation = catalog_.generation();
    return result;
}

void RuntimeResidentCatalogCache::clear() {
    catalog_ = RuntimeResourceCatalog{};
    cached_budget_execution_ = RuntimeResourceResidencyBudgetExecutionResult{};
    cached_budget_ = RuntimeResourceResidencyBudget{};
    cached_overlay_ = RuntimePackageMountOverlay::first_mount_wins;
    cached_mount_generation_ = 0;
    has_cache_ = false;
}

bool RuntimePackageCandidateResidentMountResult::succeeded() const noexcept {
    return status == RuntimePackageCandidateResidentMountStatus::mounted && committed;
}

RuntimePackageCandidateResidentMountResult
commit_runtime_package_candidate_resident_mount(IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set,
                                                RuntimeResidentCatalogCache& catalog_cache,
                                                const RuntimePackageCandidateResidentMountDesc& desc) {
    RuntimePackageCandidateResidentMountResult result;
    result.candidate = desc.candidate;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageCandidateResidentMountStatus::invalid_mount_id;
        result.resident_mount = mount_result(desc.mount_id, RuntimeResidentPackageMountStatus::invalid_mount_id,
                                             "invalid-mount-id", "resident package mount ids must be non-zero");
        add_candidate_resident_mount_diagnostic(
            result, RuntimePackageCandidateResidentMountDiagnosticPhase::resident_mount, desc.mount_id,
            result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }
    if (has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageCandidateResidentMountStatus::duplicate_mount_id;
        result.resident_mount = mount_result(desc.mount_id, RuntimeResidentPackageMountStatus::duplicate_mount_id,
                                             "duplicate-mount-id", "resident package mount id is already mounted");
        add_candidate_resident_mount_diagnostic(
            result, RuntimePackageCandidateResidentMountDiagnosticPhase::resident_mount, desc.mount_id,
            result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }

    result.invoked_candidate_load = true;
    result.candidate_load = load_runtime_package_candidate(filesystem, desc.candidate);
    result.package_desc = result.candidate_load.package_desc;
    result.loaded_record_count = result.candidate_load.loaded_record_count;
    result.loaded_resident_bytes = result.candidate_load.estimated_resident_bytes;
    if (!result.candidate_load.succeeded()) {
        result.status = RuntimePackageCandidateResidentMountStatus::candidate_load_failed;
        for (const auto& diagnostic : result.candidate_load.diagnostics) {
            add_candidate_resident_mount_diagnostic(
                result, RuntimePackageCandidateResidentMountDiagnosticPhase::candidate_load, diagnostic.asset,
                desc.mount_id, diagnostic.path, diagnostic.code, diagnostic.message);
        }
        return result;
    }

    RuntimeResidentPackageMountSet projected_mount_set = mount_set;
    result.resident_mount = projected_mount_set.mount(RuntimeResidentPackageMountRecord{
        .id = desc.mount_id,
        .label = desc.candidate.label,
        .package = result.candidate_load.loaded_package.package,
    });
    if (!result.resident_mount.succeeded()) {
        result.status = result.resident_mount.status == RuntimeResidentPackageMountStatus::duplicate_mount_id
                            ? RuntimePackageCandidateResidentMountStatus::duplicate_mount_id
                            : RuntimePackageCandidateResidentMountStatus::invalid_mount_id;
        add_candidate_resident_mount_diagnostic(
            result, RuntimePackageCandidateResidentMountDiagnosticPhase::resident_mount, desc.mount_id,
            result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }

    RuntimeResidentCatalogCache projected_catalog_cache = catalog_cache;
    result.invoked_catalog_refresh = true;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, desc.overlay, desc.budget);
    result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
    if (!result.catalog_refresh.succeeded()) {
        if (result.catalog_refresh.status == RuntimeResidentCatalogCacheStatus::budget_failed) {
            result.status = RuntimePackageCandidateResidentMountStatus::budget_failed;
            for (const auto& diagnostic : result.catalog_refresh.budget_execution.diagnostics) {
                add_candidate_resident_mount_diagnostic(
                    result, RuntimePackageCandidateResidentMountDiagnosticPhase::resident_budget, desc.mount_id,
                    diagnostic.code, diagnostic.message);
            }
        } else {
            result.status = RuntimePackageCandidateResidentMountStatus::catalog_refresh_failed;
            for (const auto& diagnostic : result.catalog_refresh.catalog_build.diagnostics) {
                add_candidate_resident_mount_diagnostic(
                    result, RuntimePackageCandidateResidentMountDiagnosticPhase::catalog_refresh, diagnostic.asset,
                    desc.mount_id, {}, "catalog-build-failed", diagnostic.diagnostic);
            }
        }
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.status = RuntimePackageCandidateResidentMountStatus::mounted;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    result.committed = true;
    return result;
}

bool RuntimePackageCandidateResidentReplaceResult::succeeded() const noexcept {
    return status == RuntimePackageCandidateResidentReplaceStatus::replaced && committed;
}

RuntimePackageCandidateResidentReplaceResult
commit_runtime_package_candidate_resident_replace(IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set,
                                                  RuntimeResidentCatalogCache& catalog_cache,
                                                  const RuntimePackageCandidateResidentReplaceDesc& desc) {
    RuntimePackageCandidateResidentReplaceResult result;
    result.candidate = desc.candidate;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;
    result.resident_replace.previous_mount_generation = result.previous_mount_generation;
    result.resident_replace.mount_generation = result.previous_mount_generation;
    result.resident_replace.mounted_package_count = result.previous_mount_count;
    result.resident_replace.diagnostic.mount = desc.mount_id;

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageCandidateResidentReplaceStatus::invalid_mount_id;
        result.resident_replace.status = RuntimeResidentPackageReplaceCommitStatus::invalid_mount_id;
        result.resident_replace.diagnostic.code = "invalid-mount-id";
        result.resident_replace.diagnostic.message = "resident package mount ids must be non-zero";
        add_candidate_resident_replace_diagnostic(
            result, RuntimePackageCandidateResidentReplaceDiagnosticPhase::resident_replace, desc.mount_id,
            result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
        return result;
    }
    if (!has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageCandidateResidentReplaceStatus::missing_mount_id;
        result.resident_replace.status = RuntimeResidentPackageReplaceCommitStatus::missing_mount_id;
        result.resident_replace.diagnostic.code = "missing-mount-id";
        result.resident_replace.diagnostic.message = "resident package mount id is not mounted";
        add_candidate_resident_replace_diagnostic(
            result, RuntimePackageCandidateResidentReplaceDiagnosticPhase::resident_replace, desc.mount_id,
            result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
        return result;
    }

    result.invoked_candidate_load = true;
    result.candidate_load = load_runtime_package_candidate(filesystem, desc.candidate);
    result.package_desc = result.candidate_load.package_desc;
    result.loaded_record_count = result.candidate_load.loaded_record_count;
    result.loaded_resident_bytes = result.candidate_load.estimated_resident_bytes;
    if (!result.candidate_load.succeeded()) {
        result.status = RuntimePackageCandidateResidentReplaceStatus::candidate_load_failed;
        for (const auto& diagnostic : result.candidate_load.diagnostics) {
            add_candidate_resident_replace_diagnostic(
                result, RuntimePackageCandidateResidentReplaceDiagnosticPhase::candidate_load, diagnostic.asset,
                desc.mount_id, diagnostic.path, diagnostic.code, diagnostic.message);
        }
        return result;
    }

    result.invoked_resident_replace = true;
    result.resident_replace = commit_runtime_resident_package_replace(mount_set, catalog_cache, desc.mount_id,
                                                                      result.candidate_load.loaded_package.package,
                                                                      desc.overlay, desc.budget);
    result.catalog_refresh = result.resident_replace.catalog_refresh;
    result.invoked_catalog_refresh =
        result.resident_replace.succeeded() ||
        result.resident_replace.status == RuntimeResidentPackageReplaceCommitStatus::budget_failed ||
        result.catalog_refresh.invoked_catalog_build || result.catalog_refresh.reused_cache;
    result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
    result.mount_generation = result.resident_replace.mount_generation;
    result.mounted_package_count = result.resident_replace.mounted_package_count;

    if (!result.resident_replace.succeeded()) {
        switch (result.resident_replace.status) {
        case RuntimeResidentPackageReplaceCommitStatus::invalid_mount_id:
            result.status = RuntimePackageCandidateResidentReplaceStatus::invalid_mount_id;
            add_candidate_resident_replace_diagnostic(
                result, RuntimePackageCandidateResidentReplaceDiagnosticPhase::resident_replace, desc.mount_id,
                result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
            break;
        case RuntimeResidentPackageReplaceCommitStatus::missing_mount_id:
            result.status = RuntimePackageCandidateResidentReplaceStatus::missing_mount_id;
            add_candidate_resident_replace_diagnostic(
                result, RuntimePackageCandidateResidentReplaceDiagnosticPhase::resident_replace, desc.mount_id,
                result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
            break;
        case RuntimeResidentPackageReplaceCommitStatus::budget_failed:
            result.status = RuntimePackageCandidateResidentReplaceStatus::budget_failed;
            for (const auto& diagnostic : result.catalog_refresh.budget_execution.diagnostics) {
                add_candidate_resident_replace_diagnostic(
                    result, RuntimePackageCandidateResidentReplaceDiagnosticPhase::resident_budget, desc.mount_id,
                    diagnostic.code, diagnostic.message);
            }
            break;
        case RuntimeResidentPackageReplaceCommitStatus::catalog_build_failed:
            if (result.catalog_refresh.invoked_catalog_build) {
                result.status = RuntimePackageCandidateResidentReplaceStatus::catalog_refresh_failed;
                for (const auto& diagnostic : result.catalog_refresh.catalog_build.diagnostics) {
                    add_candidate_resident_replace_diagnostic(
                        result, RuntimePackageCandidateResidentReplaceDiagnosticPhase::catalog_refresh,
                        diagnostic.asset, desc.mount_id, {}, "catalog-build-failed", diagnostic.diagnostic);
                }
            } else {
                result.status = RuntimePackageCandidateResidentReplaceStatus::resident_replace_failed;
                for (const auto& diagnostic : result.resident_replace.candidate_catalog_build.diagnostics) {
                    add_candidate_resident_replace_diagnostic(
                        result, RuntimePackageCandidateResidentReplaceDiagnosticPhase::candidate_catalog,
                        diagnostic.asset, desc.mount_id, {}, "candidate-catalog-build-failed", diagnostic.diagnostic);
                }
            }
            break;
        case RuntimeResidentPackageReplaceCommitStatus::replaced:
            break;
        }
        return result;
    }

    result.status = RuntimePackageCandidateResidentReplaceStatus::replaced;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    result.committed = true;
    return result;
}

bool RuntimePackageCandidateResidentReplaceReviewedEvictionsResult::succeeded() const noexcept {
    return status == RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::replaced && committed;
}

RuntimePackageCandidateResidentReplaceReviewedEvictionsResult
commit_runtime_package_candidate_resident_replace_with_reviewed_evictions(
    IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set, RuntimeResidentCatalogCache& catalog_cache,
    const RuntimePackageCandidateResidentReplaceReviewedEvictionsDesc& desc) {
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResult result;
    result.candidate = desc.candidate;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;
    result.resident_replace.previous_mount_generation = result.previous_mount_generation;
    result.resident_replace.mount_generation = result.previous_mount_generation;
    result.resident_replace.mounted_package_count = result.previous_mount_count;
    result.resident_replace.diagnostic.mount = desc.mount_id;

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::invalid_mount_id;
        result.resident_replace.status = RuntimeResidentPackageReplaceCommitStatus::invalid_mount_id;
        result.resident_replace.diagnostic.code = "invalid-mount-id";
        result.resident_replace.diagnostic.message = "resident package mount ids must be non-zero";
        add_candidate_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::resident_replace,
            desc.mount_id, result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
        return result;
    }
    if (!has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::missing_mount_id;
        result.resident_replace.status = RuntimeResidentPackageReplaceCommitStatus::missing_mount_id;
        result.resident_replace.diagnostic.code = "missing-mount-id";
        result.resident_replace.diagnostic.message = "resident package mount id is not mounted";
        add_candidate_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::resident_replace,
            desc.mount_id, result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
        return result;
    }

    result.invoked_candidate_load = true;
    result.candidate_load = load_runtime_package_candidate(filesystem, desc.candidate);
    result.package_desc = result.candidate_load.package_desc;
    result.loaded_record_count = result.candidate_load.loaded_record_count;
    result.loaded_resident_bytes = result.candidate_load.estimated_resident_bytes;
    if (!result.candidate_load.succeeded()) {
        result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::candidate_load_failed;
        add_replace_reviewed_evictions_candidate_load_diagnostics(result, result.candidate_load, desc.mount_id);
        return result;
    }

    RuntimeResidentPackageMountSet projected_mount_set = mount_set;
    RuntimeResidentCatalogCache projected_catalog_cache = catalog_cache;
    result.invoked_resident_replace = true;
    result.resident_replace = commit_runtime_resident_package_replace(
        projected_mount_set, projected_catalog_cache, desc.mount_id, result.candidate_load.loaded_package.package,
        desc.overlay, RuntimeResourceResidencyBudget{});
    if (!result.resident_replace.succeeded()) {
        result.catalog_refresh = result.resident_replace.catalog_refresh;
        result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
        switch (result.resident_replace.status) {
        case RuntimeResidentPackageReplaceCommitStatus::invalid_mount_id:
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::invalid_mount_id;
            add_candidate_resident_replace_reviewed_evictions_diagnostic(
                result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::resident_replace,
                desc.mount_id, result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
            break;
        case RuntimeResidentPackageReplaceCommitStatus::missing_mount_id:
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::missing_mount_id;
            add_candidate_resident_replace_reviewed_evictions_diagnostic(
                result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::resident_replace,
                desc.mount_id, result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
            break;
        case RuntimeResidentPackageReplaceCommitStatus::budget_failed:
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::budget_failed;
            add_replace_reviewed_evictions_budget_diagnostics(result, result.catalog_refresh.budget_execution,
                                                              desc.mount_id);
            break;
        case RuntimeResidentPackageReplaceCommitStatus::catalog_build_failed:
            if (result.catalog_refresh.invoked_catalog_build) {
                result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::catalog_refresh_failed;
                add_replace_reviewed_evictions_catalog_diagnostics(
                    result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::catalog_refresh,
                    result.catalog_refresh.catalog_build, desc.mount_id, "catalog-build-failed");
            } else {
                result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::resident_replace_failed;
                add_replace_reviewed_evictions_catalog_diagnostics(
                    result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_catalog,
                    result.resident_replace.candidate_catalog_build, desc.mount_id, "candidate-catalog-build-failed");
            }
            break;
        case RuntimeResidentPackageReplaceCommitStatus::replaced:
            break;
        }
        return result;
    }

    std::vector<RuntimeResidentPackageMountId> protected_mounts = desc.protected_mount_ids;
    if (!contains_mount_id(protected_mounts, desc.mount_id)) {
        protected_mounts.push_back(desc.mount_id);
    }

    result.invoked_eviction_plan = true;
    result.eviction_plan = plan_runtime_resident_package_evictions(
        projected_mount_set, RuntimeResidentPackageEvictionPlanDesc{
                                 .target_budget = desc.budget,
                                 .overlay = desc.overlay,
                                 .candidate_unmount_order = desc.eviction_candidate_unmount_order,
                                 .protected_mount_ids = std::move(protected_mounts),
                             });
    if (!result.eviction_plan.succeeded()) {
        result.status = map_replace_reviewed_evictions_plan_status(result.eviction_plan.status);
        add_replace_reviewed_evictions_eviction_plan_diagnostics(result, result.eviction_plan);
        if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatus::budget_unreachable) {
            add_replace_reviewed_evictions_budget_diagnostics(
                result, result.eviction_plan.projected_refresh.budget_execution, desc.mount_id);
        } else if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatus::catalog_build_failed) {
            add_replace_reviewed_evictions_catalog_diagnostics(
                result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::catalog_refresh,
                result.eviction_plan.projected_refresh.catalog_build, desc.mount_id, "catalog-build-failed");
        }
        result.projected_resident_bytes =
            result.eviction_plan.projected_refresh.budget_execution.estimated_resident_content_bytes;
        return result;
    }

    for (const auto& step : result.eviction_plan.steps) {
        const auto unmount = projected_mount_set.unmount(step.mount_id);
        if (!unmount.succeeded()) {
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::catalog_refresh_failed;
            add_candidate_resident_replace_reviewed_evictions_diagnostic(
                result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::eviction_plan,
                step.mount_id, unmount.diagnostic.code, unmount.diagnostic.message);
            return result;
        }
    }
    result.evicted_mount_count = result.eviction_plan.steps.size();

    result.invoked_catalog_refresh = true;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, desc.overlay, desc.budget);
    result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
    if (!result.catalog_refresh.succeeded()) {
        if (result.catalog_refresh.status == RuntimeResidentCatalogCacheStatus::budget_failed) {
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::budget_failed;
            add_replace_reviewed_evictions_budget_diagnostics(result, result.catalog_refresh.budget_execution,
                                                              desc.mount_id);
        } else {
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::catalog_refresh_failed;
            add_replace_reviewed_evictions_catalog_diagnostics(
                result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhase::catalog_refresh,
                result.catalog_refresh.catalog_build, desc.mount_id, "catalog-build-failed");
        }
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::replaced;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    result.committed = true;
    return result;
}

bool RuntimePackageDiscoveryResidentCommitResult::succeeded() const noexcept {
    return status == RuntimePackageDiscoveryResidentCommitStatus::committed && committed;
}

RuntimePackageDiscoveryResidentCommitResult
commit_runtime_package_discovery_resident(IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set,
                                          RuntimeResidentCatalogCache& catalog_cache,
                                          const RuntimePackageDiscoveryResidentCommitDesc& desc) {
    RuntimePackageDiscoveryResidentCommitResult result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    if (!valid_relative_vfs_path(desc.selected_package_index_path) ||
        !ends_with_package_index_extension(desc.selected_package_index_path)) {
        result.status = RuntimePackageDiscoveryResidentCommitStatus::invalid_descriptor;
        add_discovery_resident_commit_diagnostic(
            result, RuntimePackageDiscoveryResidentCommitDiagnosticPhase::descriptor, desc.mount_id,
            desc.selected_package_index_path, "invalid-selected-package-index-path",
            "runtime package discovery resident commit requires a relative selected .geindex path");
        return result;
    }

    switch (desc.mode) {
    case RuntimePackageDiscoveryResidentCommitMode::mount:
    case RuntimePackageDiscoveryResidentCommitMode::replace:
        break;
    default:
        result.status = RuntimePackageDiscoveryResidentCommitStatus::invalid_descriptor;
        add_discovery_resident_commit_diagnostic(
            result, RuntimePackageDiscoveryResidentCommitDiagnosticPhase::descriptor, desc.mount_id,
            "invalid-commit-mode", "runtime package discovery resident commit mode is not supported");
        return result;
    }

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageDiscoveryResidentCommitStatus::invalid_mount_id;
        add_discovery_resident_commit_diagnostic(result, resident_operation_phase(desc.mode), desc.mount_id,
                                                 "invalid-mount-id", "resident package mount ids must be non-zero");
        return result;
    }
    if (desc.mode == RuntimePackageDiscoveryResidentCommitMode::mount && has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageDiscoveryResidentCommitStatus::duplicate_mount_id;
        add_discovery_resident_commit_diagnostic(
            result, RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_mount, desc.mount_id,
            "duplicate-mount-id", "resident package mount id is already mounted");
        return result;
    }
    if (desc.mode == RuntimePackageDiscoveryResidentCommitMode::replace && !has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageDiscoveryResidentCommitStatus::missing_mount_id;
        add_discovery_resident_commit_diagnostic(
            result, RuntimePackageDiscoveryResidentCommitDiagnosticPhase::resident_replace, desc.mount_id,
            "missing-mount-id", "resident package mount id is not mounted");
        return result;
    }

    result.invoked_discovery = true;
    result.discovery = discover_runtime_package_indexes(filesystem, desc.discovery);
    if (!result.discovery.succeeded()) {
        const auto descriptor_failure =
            result.discovery.status == RuntimePackageIndexDiscoveryStatus::invalid_descriptor;
        result.status = descriptor_failure ? RuntimePackageDiscoveryResidentCommitStatus::invalid_descriptor
                                           : RuntimePackageDiscoveryResidentCommitStatus::discovery_failed;
        add_discovery_diagnostics(result, result.discovery,
                                  descriptor_failure ? RuntimePackageDiscoveryResidentCommitDiagnosticPhase::descriptor
                                                     : RuntimePackageDiscoveryResidentCommitDiagnosticPhase::discovery);
        return result;
    }

    const auto selected =
        std::ranges::find_if(result.discovery.candidates, [&desc](const RuntimePackageIndexDiscoveryCandidate& item) {
            return item.package_index_path == desc.selected_package_index_path;
        });
    if (selected == result.discovery.candidates.end()) {
        result.status = RuntimePackageDiscoveryResidentCommitStatus::candidate_not_found;
        add_discovery_resident_commit_diagnostic(
            result, RuntimePackageDiscoveryResidentCommitDiagnosticPhase::candidate_selection, desc.mount_id,
            desc.selected_package_index_path, "candidate-not-found",
            "selected runtime package index was not discovered under the reviewed root");
        return result;
    }
    result.selected_candidate = *selected;

    result.invoked_resident_commit = true;
    if (desc.mode == RuntimePackageDiscoveryResidentCommitMode::mount) {
        result.resident_mount =
            commit_runtime_package_candidate_resident_mount(filesystem, mount_set, catalog_cache,
                                                            RuntimePackageCandidateResidentMountDesc{
                                                                .candidate = result.selected_candidate,
                                                                .mount_id = desc.mount_id,
                                                                .overlay = desc.overlay,
                                                                .budget = desc.budget,
                                                            });
        result.catalog_refresh = result.resident_mount.catalog_refresh;
        result.loaded_record_count = result.resident_mount.loaded_record_count;
        result.loaded_resident_bytes = result.resident_mount.loaded_resident_bytes;
        result.projected_resident_bytes = result.resident_mount.projected_resident_bytes;
        result.mount_generation = result.resident_mount.mount_generation;
        result.mounted_package_count = result.resident_mount.mounted_package_count;
        add_mount_commit_diagnostics(result, result.resident_mount);

        switch (result.resident_mount.status) {
        case RuntimePackageCandidateResidentMountStatus::mounted:
            result.status = RuntimePackageDiscoveryResidentCommitStatus::committed;
            result.committed = true;
            break;
        case RuntimePackageCandidateResidentMountStatus::invalid_mount_id:
            result.status = RuntimePackageDiscoveryResidentCommitStatus::invalid_mount_id;
            break;
        case RuntimePackageCandidateResidentMountStatus::duplicate_mount_id:
            result.status = RuntimePackageDiscoveryResidentCommitStatus::duplicate_mount_id;
            break;
        case RuntimePackageCandidateResidentMountStatus::candidate_load_failed:
            result.status = RuntimePackageDiscoveryResidentCommitStatus::candidate_load_failed;
            break;
        case RuntimePackageCandidateResidentMountStatus::budget_failed:
            result.status = RuntimePackageDiscoveryResidentCommitStatus::budget_failed;
            break;
        case RuntimePackageCandidateResidentMountStatus::catalog_refresh_failed:
            result.status = RuntimePackageDiscoveryResidentCommitStatus::catalog_refresh_failed;
            break;
        }
        return result;
    }

    result.resident_replace =
        commit_runtime_package_candidate_resident_replace(filesystem, mount_set, catalog_cache,
                                                          RuntimePackageCandidateResidentReplaceDesc{
                                                              .candidate = result.selected_candidate,
                                                              .mount_id = desc.mount_id,
                                                              .overlay = desc.overlay,
                                                              .budget = desc.budget,
                                                          });
    result.catalog_refresh = result.resident_replace.catalog_refresh;
    result.loaded_record_count = result.resident_replace.loaded_record_count;
    result.loaded_resident_bytes = result.resident_replace.loaded_resident_bytes;
    result.projected_resident_bytes = result.resident_replace.projected_resident_bytes;
    result.mount_generation = result.resident_replace.mount_generation;
    result.mounted_package_count = result.resident_replace.mounted_package_count;
    add_replace_commit_diagnostics(result, result.resident_replace);

    switch (result.resident_replace.status) {
    case RuntimePackageCandidateResidentReplaceStatus::replaced:
        result.status = RuntimePackageDiscoveryResidentCommitStatus::committed;
        result.committed = true;
        break;
    case RuntimePackageCandidateResidentReplaceStatus::invalid_mount_id:
        result.status = RuntimePackageDiscoveryResidentCommitStatus::invalid_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceStatus::missing_mount_id:
        result.status = RuntimePackageDiscoveryResidentCommitStatus::missing_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceStatus::candidate_load_failed:
        result.status = RuntimePackageDiscoveryResidentCommitStatus::candidate_load_failed;
        break;
    case RuntimePackageCandidateResidentReplaceStatus::resident_replace_failed:
        result.status = RuntimePackageDiscoveryResidentCommitStatus::resident_commit_failed;
        break;
    case RuntimePackageCandidateResidentReplaceStatus::budget_failed:
        result.status = RuntimePackageDiscoveryResidentCommitStatus::budget_failed;
        break;
    case RuntimePackageCandidateResidentReplaceStatus::catalog_refresh_failed:
        result.status = RuntimePackageDiscoveryResidentCommitStatus::catalog_refresh_failed;
        break;
    }
    return result;
}

bool RuntimePackageDiscoveryResidentMountReviewedEvictionsResult::succeeded() const noexcept {
    return status == RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::committed && committed;
}

RuntimePackageDiscoveryResidentMountReviewedEvictionsResult
commit_runtime_package_discovery_resident_mount_with_reviewed_evictions(
    IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set, RuntimeResidentCatalogCache& catalog_cache,
    const RuntimePackageDiscoveryResidentMountReviewedEvictionsDesc& desc) {
    RuntimePackageDiscoveryResidentMountReviewedEvictionsResult result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    if (!valid_relative_vfs_path(desc.selected_package_index_path) ||
        !ends_with_package_index_extension(desc.selected_package_index_path)) {
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::invalid_descriptor;
        add_discovery_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::descriptor, desc.mount_id,
            desc.selected_package_index_path, "invalid-selected-package-index-path",
            "runtime package discovery reviewed-eviction mount requires a relative selected .geindex path");
        return result;
    }

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::invalid_mount_id;
        add_discovery_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::resident_mount, desc.mount_id,
            "invalid-mount-id", "resident package mount ids must be non-zero");
        return result;
    }
    if (has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::duplicate_mount_id;
        add_discovery_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::resident_mount, desc.mount_id,
            "duplicate-mount-id", "resident package mount id is already mounted");
        return result;
    }

    result.invoked_discovery = true;
    result.discovery = discover_runtime_package_indexes(filesystem, desc.discovery);
    if (!result.discovery.succeeded()) {
        const auto descriptor_failure =
            result.discovery.status == RuntimePackageIndexDiscoveryStatus::invalid_descriptor;
        result.status = descriptor_failure
                            ? RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::invalid_descriptor
                            : RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::discovery_failed;
        add_discovery_diagnostics(
            result, result.discovery,
            descriptor_failure ? RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::descriptor
                               : RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::discovery);
        return result;
    }

    const auto selected =
        std::ranges::find_if(result.discovery.candidates, [&desc](const RuntimePackageIndexDiscoveryCandidate& item) {
            return item.package_index_path == desc.selected_package_index_path;
        });
    if (selected == result.discovery.candidates.end()) {
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::candidate_not_found;
        add_discovery_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhase::candidate_selection,
            desc.mount_id, desc.selected_package_index_path, "candidate-not-found",
            "selected runtime package index was not discovered under the reviewed root");
        return result;
    }
    result.selected_candidate = *selected;

    result.invoked_resident_commit = true;
    result.candidate_resident_mount = commit_runtime_package_candidate_resident_mount_with_reviewed_evictions(
        filesystem, mount_set, catalog_cache,
        RuntimePackageCandidateResidentMountReviewedEvictionsDesc{
            .candidate = result.selected_candidate,
            .mount_id = desc.mount_id,
            .overlay = desc.overlay,
            .budget = desc.budget,
            .eviction_candidate_unmount_order = desc.eviction_candidate_unmount_order,
            .protected_mount_ids = desc.protected_mount_ids,
        });
    result.eviction_plan = result.candidate_resident_mount.eviction_plan;
    result.catalog_refresh = result.candidate_resident_mount.catalog_refresh;
    result.loaded_record_count = result.candidate_resident_mount.loaded_record_count;
    result.loaded_resident_bytes = result.candidate_resident_mount.loaded_resident_bytes;
    result.projected_resident_bytes = result.candidate_resident_mount.projected_resident_bytes;
    result.mount_generation = result.candidate_resident_mount.mount_generation;
    result.mounted_package_count = result.candidate_resident_mount.mounted_package_count;
    result.evicted_mount_count = result.candidate_resident_mount.evicted_mount_count;
    add_reviewed_evictions_mount_commit_diagnostics(result, result.candidate_resident_mount);

    switch (result.candidate_resident_mount.status) {
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatus::mounted:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::committed;
        result.committed = true;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatus::invalid_mount_id:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::invalid_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatus::duplicate_mount_id:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::duplicate_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatus::candidate_load_failed:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::candidate_load_failed;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatus::invalid_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::invalid_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatus::duplicate_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::duplicate_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatus::missing_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::missing_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatus::protected_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::protected_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatus::budget_failed:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::budget_failed;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatus::catalog_refresh_failed:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatus::catalog_refresh_failed;
        break;
    }
    return result;
}

bool RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult::succeeded() const noexcept {
    return status == RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::committed && committed;
}

RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult
commit_runtime_package_discovery_resident_replace_with_reviewed_evictions(
    IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set, RuntimeResidentCatalogCache& catalog_cache,
    const RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDesc& desc) {
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResult result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    if (!valid_relative_vfs_path(desc.selected_package_index_path) ||
        !ends_with_package_index_extension(desc.selected_package_index_path)) {
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::invalid_descriptor;
        add_discovery_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::descriptor, desc.mount_id,
            desc.selected_package_index_path, "invalid-selected-package-index-path",
            "runtime package discovery reviewed-eviction replacement requires a relative selected .geindex path");
        return result;
    }

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::invalid_mount_id;
        add_discovery_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::resident_replace,
            desc.mount_id, "invalid-mount-id", "resident package mount ids must be non-zero");
        return result;
    }
    if (!has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::missing_mount_id;
        add_discovery_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::resident_replace,
            desc.mount_id, "missing-mount-id", "resident package mount id is not mounted");
        return result;
    }

    result.invoked_discovery = true;
    result.discovery = discover_runtime_package_indexes(filesystem, desc.discovery);
    if (!result.discovery.succeeded()) {
        const auto descriptor_failure =
            result.discovery.status == RuntimePackageIndexDiscoveryStatus::invalid_descriptor;
        result.status = descriptor_failure
                            ? RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::invalid_descriptor
                            : RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::discovery_failed;
        add_discovery_diagnostics(
            result, result.discovery,
            descriptor_failure ? RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::descriptor
                               : RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::discovery);
        return result;
    }

    const auto selected =
        std::ranges::find_if(result.discovery.candidates, [&desc](const RuntimePackageIndexDiscoveryCandidate& item) {
            return item.package_index_path == desc.selected_package_index_path;
        });
    if (selected == result.discovery.candidates.end()) {
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::candidate_not_found;
        add_discovery_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhase::candidate_selection,
            desc.mount_id, desc.selected_package_index_path, "candidate-not-found",
            "selected runtime package index was not discovered under the reviewed root");
        return result;
    }
    result.selected_candidate = *selected;

    result.invoked_resident_commit = true;
    result.candidate_resident_replace = commit_runtime_package_candidate_resident_replace_with_reviewed_evictions(
        filesystem, mount_set, catalog_cache,
        RuntimePackageCandidateResidentReplaceReviewedEvictionsDesc{
            .candidate = result.selected_candidate,
            .mount_id = desc.mount_id,
            .overlay = desc.overlay,
            .budget = desc.budget,
            .eviction_candidate_unmount_order = desc.eviction_candidate_unmount_order,
            .protected_mount_ids = desc.protected_mount_ids,
        });
    result.eviction_plan = result.candidate_resident_replace.eviction_plan;
    result.catalog_refresh = result.candidate_resident_replace.catalog_refresh;
    result.loaded_record_count = result.candidate_resident_replace.loaded_record_count;
    result.loaded_resident_bytes = result.candidate_resident_replace.loaded_resident_bytes;
    result.projected_resident_bytes = result.candidate_resident_replace.projected_resident_bytes;
    result.mount_generation = result.candidate_resident_replace.mount_generation;
    result.mounted_package_count = result.candidate_resident_replace.mounted_package_count;
    result.evicted_mount_count = result.candidate_resident_replace.evicted_mount_count;
    add_reviewed_evictions_replace_commit_diagnostics(result, result.candidate_resident_replace);

    switch (result.candidate_resident_replace.status) {
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::replaced:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::committed;
        result.committed = true;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::invalid_mount_id:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::invalid_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::missing_mount_id:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::missing_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::candidate_load_failed:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::candidate_load_failed;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::resident_replace_failed:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::resident_replace_failed;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::invalid_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::invalid_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::duplicate_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::duplicate_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::missing_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::missing_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::protected_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::protected_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::budget_failed:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::budget_failed;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatus::catalog_refresh_failed:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatus::catalog_refresh_failed;
        break;
    }
    return result;
}

bool RuntimeResidentPackageUnmountCommitResult::succeeded() const noexcept {
    return status == RuntimeResidentPackageUnmountCommitStatus::unmounted;
}

RuntimeResidentPackageUnmountCommitResult
commit_runtime_resident_package_unmount(RuntimeResidentPackageMountSet& mount_set,
                                        RuntimeResidentCatalogCache& catalog_cache, RuntimeResidentPackageMountId id,
                                        RuntimePackageMountOverlay overlay,
                                        const RuntimeResourceResidencyBudget& budget) {
    RuntimeResidentPackageUnmountCommitResult result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    RuntimeResidentPackageMountSet projected_mount_set = mount_set;
    result.unmount = projected_mount_set.unmount(id);
    if (!result.unmount.succeeded()) {
        result.status = result.unmount.status == RuntimeResidentPackageMountStatus::invalid_mount_id
                            ? RuntimeResidentPackageUnmountCommitStatus::invalid_mount_id
                            : RuntimeResidentPackageUnmountCommitStatus::missing_mount_id;
        return result;
    }

    RuntimeResidentCatalogCache projected_catalog_cache = catalog_cache;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, overlay, budget);
    if (!result.catalog_refresh.succeeded()) {
        result.status = result.catalog_refresh.status == RuntimeResidentCatalogCacheStatus::budget_failed
                            ? RuntimeResidentPackageUnmountCommitStatus::budget_failed
                            : RuntimeResidentPackageUnmountCommitStatus::catalog_build_failed;
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.status = RuntimeResidentPackageUnmountCommitStatus::unmounted;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    return result;
}

bool RuntimeResidentPackageEvictionPlanResult::succeeded() const noexcept {
    return status == RuntimeResidentPackageEvictionPlanStatus::no_eviction_required ||
           status == RuntimeResidentPackageEvictionPlanStatus::planned;
}

RuntimeResidentPackageEvictionPlanResult
plan_runtime_resident_package_evictions(const RuntimeResidentPackageMountSet& mount_set,
                                        const RuntimeResidentPackageEvictionPlanDesc& desc) {
    RuntimeResidentPackageEvictionPlanResult result;
    result.mount_generation = mount_set.generation();
    result.previous_mount_count = mount_set.mounts().size();
    result.projected_mount_count = result.previous_mount_count;

    RuntimeResidentCatalogCache current_cache;
    result.current_refresh = current_cache.refresh(mount_set, desc.overlay, desc.target_budget);
    result.projected_refresh = result.current_refresh;
    if (result.current_refresh.succeeded()) {
        result.status = RuntimeResidentPackageEvictionPlanStatus::no_eviction_required;
        return result;
    }
    if (result.current_refresh.status == RuntimeResidentCatalogCacheStatus::catalog_build_failed) {
        result.status = RuntimeResidentPackageEvictionPlanStatus::catalog_build_failed;
        return result;
    }

    std::vector<RuntimeResidentPackageMountId> seen_candidates;
    seen_candidates.reserve(desc.candidate_unmount_order.size());
    for (const auto candidate : desc.candidate_unmount_order) {
        if (candidate.value == 0) {
            result.status = RuntimeResidentPackageEvictionPlanStatus::invalid_candidate_mount_id;
            add_eviction_diagnostic(result, candidate, "invalid-candidate-mount-id",
                                    "resident eviction candidate mount ids must be non-zero");
            return result;
        }
        if (contains_mount_id(seen_candidates, candidate)) {
            result.status = RuntimeResidentPackageEvictionPlanStatus::duplicate_candidate_mount_id;
            add_eviction_diagnostic(result, candidate, "duplicate-candidate-mount-id",
                                    "resident eviction candidate mount id appears more than once");
            return result;
        }
        if (contains_mount_id(desc.protected_mount_ids, candidate)) {
            result.status = RuntimeResidentPackageEvictionPlanStatus::protected_candidate_mount_id;
            add_eviction_diagnostic(result, candidate, "protected-candidate-mount-id",
                                    "resident eviction candidate mount id is protected");
            return result;
        }
        if (!has_mount_id(mount_set, candidate)) {
            result.status = RuntimeResidentPackageEvictionPlanStatus::missing_candidate_mount_id;
            add_eviction_diagnostic(result, candidate, "missing-candidate-mount-id",
                                    "resident eviction candidate mount id is not mounted");
            return result;
        }
        seen_candidates.push_back(candidate);
    }

    RuntimeResidentPackageMountSet projected_mount_set = mount_set;
    RuntimeResidentCatalogCache projected_cache;
    for (const auto candidate : desc.candidate_unmount_order) {
        RuntimeResidentPackageEvictionPlanStep step;
        step.mount_id = candidate;
        step.unmount = projected_mount_set.unmount(candidate);
        step.catalog_refresh = projected_cache.refresh(projected_mount_set, desc.overlay, desc.target_budget);
        result.projected_refresh = step.catalog_refresh;
        result.projected_mount_count = projected_mount_set.mounts().size();
        result.steps.push_back(std::move(step));

        if (result.projected_refresh.succeeded()) {
            result.status = RuntimeResidentPackageEvictionPlanStatus::planned;
            return result;
        }
        if (result.projected_refresh.status == RuntimeResidentCatalogCacheStatus::catalog_build_failed) {
            result.status = RuntimeResidentPackageEvictionPlanStatus::catalog_build_failed;
            return result;
        }
    }

    result.status = RuntimeResidentPackageEvictionPlanStatus::budget_unreachable;
    return result;
}

bool RuntimeResidentPackageReviewedEvictionCommitResult::succeeded() const noexcept {
    return (status == RuntimeResidentPackageReviewedEvictionCommitStatus::committed ||
            status == RuntimeResidentPackageReviewedEvictionCommitStatus::no_eviction_required) &&
           committed;
}

RuntimeResidentPackageReviewedEvictionCommitResult
commit_runtime_resident_package_reviewed_evictions(RuntimeResidentPackageMountSet& mount_set,
                                                   RuntimeResidentCatalogCache& catalog_cache,
                                                   const RuntimeResidentPackageReviewedEvictionCommitDesc& desc) {
    RuntimeResidentPackageReviewedEvictionCommitResult result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    result.invoked_eviction_plan = true;
    result.eviction_plan =
        plan_runtime_resident_package_evictions(mount_set, RuntimeResidentPackageEvictionPlanDesc{
                                                               .target_budget = desc.target_budget,
                                                               .overlay = desc.overlay,
                                                               .candidate_unmount_order = desc.candidate_unmount_order,
                                                               .protected_mount_ids = desc.protected_mount_ids,
                                                           });
    result.catalog_refresh = result.eviction_plan.projected_refresh;
    result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;

    if (!result.eviction_plan.succeeded()) {
        result.status = map_reviewed_eviction_commit_plan_status(result.eviction_plan.status);
        add_reviewed_eviction_commit_plan_diagnostics(result, result.eviction_plan);
        if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatus::budget_unreachable) {
            add_reviewed_eviction_commit_budget_diagnostics(result, result.catalog_refresh.budget_execution);
        } else if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatus::catalog_build_failed) {
            add_reviewed_eviction_commit_catalog_diagnostics(result, result.catalog_refresh.catalog_build);
        }
        return result;
    }

    RuntimeResidentPackageMountSet projected_mount_set = mount_set;
    RuntimeResidentCatalogCache projected_catalog_cache = catalog_cache;
    for (const auto& step : result.eviction_plan.steps) {
        const auto unmount = projected_mount_set.unmount(step.mount_id);
        if (!unmount.succeeded()) {
            result.status = RuntimeResidentPackageReviewedEvictionCommitStatus::catalog_refresh_failed;
            add_reviewed_eviction_commit_diagnostic(
                result, RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhase::eviction_plan, {}, step.mount_id,
                unmount.diagnostic.code, unmount.diagnostic.message);
            return result;
        }
    }

    result.invoked_catalog_refresh = true;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, desc.overlay, desc.target_budget);
    result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
    result.evicted_mount_count = result.eviction_plan.steps.size();
    result.mounted_package_count = projected_mount_set.mounts().size();
    result.mount_generation = projected_mount_set.generation();

    if (!result.catalog_refresh.succeeded()) {
        if (result.catalog_refresh.status == RuntimeResidentCatalogCacheStatus::budget_failed) {
            result.status = RuntimeResidentPackageReviewedEvictionCommitStatus::budget_failed;
            add_reviewed_eviction_commit_budget_diagnostics(result, result.catalog_refresh.budget_execution);
        } else {
            result.status = RuntimeResidentPackageReviewedEvictionCommitStatus::catalog_refresh_failed;
            add_reviewed_eviction_commit_catalog_diagnostics(result, result.catalog_refresh.catalog_build);
        }
        result.mount_generation = result.previous_mount_generation;
        result.mounted_package_count = result.previous_mount_count;
        result.evicted_mount_count = 0;
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);
    result.status = map_reviewed_eviction_commit_plan_status(result.eviction_plan.status);
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    result.committed = true;
    return result;
}

bool RuntimePackageCandidateResidentMountReviewedEvictionsResult::succeeded() const noexcept {
    return status == RuntimePackageCandidateResidentMountReviewedEvictionsStatus::mounted && committed;
}

RuntimePackageCandidateResidentMountReviewedEvictionsResult
commit_runtime_package_candidate_resident_mount_with_reviewed_evictions(
    IFileSystem& filesystem, RuntimeResidentPackageMountSet& mount_set, RuntimeResidentCatalogCache& catalog_cache,
    const RuntimePackageCandidateResidentMountReviewedEvictionsDesc& desc) {
    RuntimePackageCandidateResidentMountReviewedEvictionsResult result;
    result.candidate = desc.candidate;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatus::invalid_mount_id;
        result.resident_mount = mount_result(desc.mount_id, RuntimeResidentPackageMountStatus::invalid_mount_id,
                                             "invalid-mount-id", "resident package mount ids must be non-zero");
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::resident_mount, desc.mount_id,
            result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }
    if (has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatus::duplicate_mount_id;
        result.resident_mount = mount_result(desc.mount_id, RuntimeResidentPackageMountStatus::duplicate_mount_id,
                                             "duplicate-mount-id", "resident package mount id is already mounted");
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::resident_mount, desc.mount_id,
            result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }

    result.invoked_candidate_load = true;
    result.candidate_load = load_runtime_package_candidate(filesystem, desc.candidate);
    result.package_desc = result.candidate_load.package_desc;
    result.loaded_record_count = result.candidate_load.loaded_record_count;
    result.loaded_resident_bytes = result.candidate_load.estimated_resident_bytes;
    if (!result.candidate_load.succeeded()) {
        result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatus::candidate_load_failed;
        add_reviewed_evictions_candidate_load_diagnostics(result, result.candidate_load, desc.mount_id);
        return result;
    }

    RuntimeResidentPackageMountSet projected_mount_set = mount_set;
    result.resident_mount = projected_mount_set.mount(RuntimeResidentPackageMountRecord{
        .id = desc.mount_id,
        .label = desc.candidate.label,
        .package = result.candidate_load.loaded_package.package,
    });
    if (!result.resident_mount.succeeded()) {
        result.status = result.resident_mount.status == RuntimeResidentPackageMountStatus::duplicate_mount_id
                            ? RuntimePackageCandidateResidentMountReviewedEvictionsStatus::duplicate_mount_id
                            : RuntimePackageCandidateResidentMountReviewedEvictionsStatus::invalid_mount_id;
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::resident_mount, desc.mount_id,
            result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }

    std::vector<RuntimeResidentPackageMountId> protected_mounts = desc.protected_mount_ids;
    if (!contains_mount_id(protected_mounts, desc.mount_id)) {
        protected_mounts.push_back(desc.mount_id);
    }

    result.invoked_eviction_plan = true;
    result.eviction_plan = plan_runtime_resident_package_evictions(
        projected_mount_set, RuntimeResidentPackageEvictionPlanDesc{
                                 .target_budget = desc.budget,
                                 .overlay = desc.overlay,
                                 .candidate_unmount_order = desc.eviction_candidate_unmount_order,
                                 .protected_mount_ids = std::move(protected_mounts),
                             });
    if (!result.eviction_plan.succeeded()) {
        result.status = map_eviction_plan_status(result.eviction_plan.status);
        add_reviewed_evictions_eviction_plan_diagnostics(result, result.eviction_plan);
        if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatus::budget_unreachable) {
            add_reviewed_evictions_budget_diagnostics(result, result.eviction_plan.projected_refresh.budget_execution,
                                                      desc.mount_id);
        } else if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatus::catalog_build_failed) {
            add_reviewed_evictions_catalog_diagnostics(result, result.eviction_plan.projected_refresh.catalog_build,
                                                       desc.mount_id);
        }
        result.projected_resident_bytes =
            result.eviction_plan.projected_refresh.budget_execution.estimated_resident_content_bytes;
        return result;
    }

    for (const auto& step : result.eviction_plan.steps) {
        const auto unmount = projected_mount_set.unmount(step.mount_id);
        if (!unmount.succeeded()) {
            result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatus::catalog_refresh_failed;
            add_candidate_resident_mount_reviewed_evictions_diagnostic(
                result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhase::eviction_plan,
                step.mount_id, unmount.diagnostic.code, unmount.diagnostic.message);
            return result;
        }
    }
    result.evicted_mount_count = result.eviction_plan.steps.size();

    RuntimeResidentCatalogCache projected_catalog_cache = catalog_cache;
    result.invoked_catalog_refresh = true;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, desc.overlay, desc.budget);
    result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
    if (!result.catalog_refresh.succeeded()) {
        if (result.catalog_refresh.status == RuntimeResidentCatalogCacheStatus::budget_failed) {
            result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatus::budget_failed;
            add_reviewed_evictions_budget_diagnostics(result, result.catalog_refresh.budget_execution, desc.mount_id);
        } else {
            result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatus::catalog_refresh_failed;
            add_reviewed_evictions_catalog_diagnostics(result, result.catalog_refresh.catalog_build, desc.mount_id);
        }
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatus::mounted;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    result.committed = true;
    return result;
}

struct RuntimeResidentPackageMountSetReplaceAccess {
    [[nodiscard]] static bool replace(RuntimeResidentPackageMountSet& mount_set, RuntimeResidentPackageMountId id,
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

bool RuntimeResidentPackageReplaceCommitResult::succeeded() const noexcept {
    return status == RuntimeResidentPackageReplaceCommitStatus::replaced;
}

RuntimeResidentPackageReplaceCommitResult
commit_runtime_resident_package_replace(RuntimeResidentPackageMountSet& mount_set,
                                        RuntimeResidentCatalogCache& catalog_cache, RuntimeResidentPackageMountId id,
                                        RuntimeAssetPackage replacement_package, RuntimePackageMountOverlay overlay,
                                        const RuntimeResourceResidencyBudget& budget) {
    RuntimeResidentPackageReplaceCommitResult result;
    result.diagnostic.mount = id;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.mounted_package_count = mount_set.mounts().size();

    if (id.value == 0) {
        result.status = RuntimeResidentPackageReplaceCommitStatus::invalid_mount_id;
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
        result.status = RuntimeResidentPackageReplaceCommitStatus::missing_mount_id;
        result.diagnostic.code = "missing-mount-id";
        result.diagnostic.message = "resident package mount id is not mounted";
        return result;
    }

    RuntimeResourceCatalog candidate_catalog;
    result.candidate_catalog_build = build_runtime_resource_catalog(candidate_catalog, replacement_package);
    result.invoked_candidate_catalog_build = true;
    if (!result.candidate_catalog_build.succeeded()) {
        result.status = RuntimeResidentPackageReplaceCommitStatus::catalog_build_failed;
        return result;
    }

    RuntimeResidentPackageMountSet projected_mount_set = mount_set;
    const auto replaced =
        RuntimeResidentPackageMountSetReplaceAccess::replace(projected_mount_set, id, std::move(replacement_package));
    if (!replaced) {
        result.status = RuntimeResidentPackageReplaceCommitStatus::missing_mount_id;
        result.diagnostic.code = "missing-mount-id";
        result.diagnostic.message = "resident package mount id is not mounted";
        return result;
    }

    RuntimeResidentCatalogCache projected_catalog_cache = catalog_cache;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, overlay, budget);
    if (!result.catalog_refresh.succeeded()) {
        result.status = result.catalog_refresh.status == RuntimeResidentCatalogCacheStatus::budget_failed
                            ? RuntimeResidentPackageReplaceCommitStatus::budget_failed
                            : RuntimeResidentPackageReplaceCommitStatus::catalog_build_failed;
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.status = RuntimeResidentPackageReplaceCommitStatus::replaced;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    return result;
}

std::optional<RuntimeResourceHandle> find_runtime_resource(const RuntimeResourceCatalog& catalog,
                                                           AssetId asset) noexcept {
    for (const auto& record : catalog.records()) {
        if (record.asset == asset && is_runtime_resource_handle_live(catalog, record.handle)) {
            return record.handle;
        }
    }
    return std::nullopt;
}

bool is_runtime_resource_handle_live(const RuntimeResourceCatalog& catalog, RuntimeResourceHandle handle) noexcept {
    if (handle.index == 0 || handle.index > catalog.records().size()) {
        return false;
    }
    return catalog.records()[handle.index - 1U].handle.generation == handle.generation;
}

const RuntimeResourceRecord* runtime_resource_record(const RuntimeResourceCatalog& catalog,
                                                     RuntimeResourceHandle handle) noexcept {
    if (!is_runtime_resource_handle_live(catalog, handle)) {
        return nullptr;
    }
    return &catalog.records()[handle.index - 1U];
}

bool RuntimePackageSafePointReplacementResult::succeeded() const noexcept {
    return status == RuntimePackageSafePointReplacementStatus::committed;
}

RuntimePackageSafePointReplacementResult
commit_runtime_package_safe_point_replacement(RuntimeAssetPackageStore& store, RuntimeResourceCatalog& catalog) {
    RuntimePackageSafePointReplacementResult result;
    result.previous_record_count = catalog.records().size();
    result.previous_generation = catalog.generation();
    result.committed_generation = result.previous_generation;

    const auto* pending = store.pending();
    if (pending == nullptr) {
        return result;
    }

    RuntimeResourceCatalog replacement_catalog = catalog;
    auto catalog_build = build_runtime_resource_catalog(replacement_catalog, *pending);
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
                                                                             RuntimeResourceCatalog& catalog) {
    RuntimePackageSafePointUnloadResult result;
    result.previous_record_count = catalog.records().size();
    result.previous_generation = catalog.generation();
    result.committed_generation = result.previous_generation;

    if (store.active() == nullptr) {
        return result;
    }

    RuntimeResourceCatalog unloaded_catalog = catalog;
    const RuntimeAssetPackage empty_package;
    auto catalog_build = build_runtime_resource_catalog(unloaded_catalog, empty_package);
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
