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

void add_discovery_diagnostic(RuntimePackageIndexDiscoveryResultV2& result, std::string path, std::string code,
                              std::string message) {
    result.diagnostics.push_back(RuntimePackageIndexDiscoveryDiagnosticV2{
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_load_diagnostic(RuntimePackageCandidateLoadResultV2& result, AssetId asset, std::string path,
                                   std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageCandidateLoadDiagnosticV2{
        .asset = asset,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_load_diagnostic(RuntimePackageCandidateLoadResultV2& result, std::string path, std::string code,
                                   std::string message) {
    add_candidate_load_diagnostic(result, AssetId{}, std::move(path), std::move(code), std::move(message));
}

void add_candidate_resident_mount_diagnostic(RuntimePackageCandidateResidentMountResultV2& result,
                                             RuntimePackageCandidateResidentMountDiagnosticPhaseV2 phase, AssetId asset,
                                             RuntimeResidentPackageMountIdV2 mount, std::string path, std::string code,
                                             std::string message) {
    result.diagnostics.push_back(RuntimePackageCandidateResidentMountDiagnosticV2{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_resident_mount_diagnostic(RuntimePackageCandidateResidentMountResultV2& result,
                                             RuntimePackageCandidateResidentMountDiagnosticPhaseV2 phase,
                                             RuntimeResidentPackageMountIdV2 mount, std::string code,
                                             std::string message) {
    add_candidate_resident_mount_diagnostic(result, phase, AssetId{}, mount, {}, std::move(code), std::move(message));
}

void add_candidate_resident_mount_reviewed_evictions_diagnostic(
    RuntimePackageCandidateResidentMountReviewedEvictionsResultV2& result,
    RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2 phase, AssetId asset,
    RuntimeResidentPackageMountIdV2 mount, std::string path, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticV2{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_resident_mount_reviewed_evictions_diagnostic(
    RuntimePackageCandidateResidentMountReviewedEvictionsResultV2& result,
    RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2 phase, RuntimeResidentPackageMountIdV2 mount,
    std::string code, std::string message) {
    add_candidate_resident_mount_reviewed_evictions_diagnostic(result, phase, AssetId{}, mount, {}, std::move(code),
                                                               std::move(message));
}

void add_candidate_resident_replace_diagnostic(RuntimePackageCandidateResidentReplaceResultV2& result,
                                               RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2 phase,
                                               AssetId asset, RuntimeResidentPackageMountIdV2 mount, std::string path,
                                               std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageCandidateResidentReplaceDiagnosticV2{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_resident_replace_diagnostic(RuntimePackageCandidateResidentReplaceResultV2& result,
                                               RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2 phase,
                                               RuntimeResidentPackageMountIdV2 mount, std::string code,
                                               std::string message) {
    add_candidate_resident_replace_diagnostic(result, phase, AssetId{}, mount, {}, std::move(code), std::move(message));
}

void add_candidate_resident_replace_reviewed_evictions_diagnostic(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResultV2& result,
    RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2 phase, AssetId asset,
    RuntimeResidentPackageMountIdV2 mount, std::string path, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticV2{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_candidate_resident_replace_reviewed_evictions_diagnostic(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResultV2& result,
    RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2 phase,
    RuntimeResidentPackageMountIdV2 mount, std::string code, std::string message) {
    add_candidate_resident_replace_reviewed_evictions_diagnostic(result, phase, AssetId{}, mount, {}, std::move(code),
                                                                 std::move(message));
}

void add_discovery_resident_commit_diagnostic(RuntimePackageDiscoveryResidentCommitResultV2& result,
                                              RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2 phase,
                                              RuntimeResidentPackageMountIdV2 mount, std::string path, std::string code,
                                              std::string message) {
    result.diagnostics.push_back(RuntimePackageDiscoveryResidentCommitDiagnosticV2{
        .phase = phase,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_discovery_resident_commit_diagnostic(RuntimePackageDiscoveryResidentCommitResultV2& result,
                                              RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2 phase,
                                              RuntimeResidentPackageMountIdV2 mount, std::string code,
                                              std::string message) {
    add_discovery_resident_commit_diagnostic(result, phase, mount, {}, std::move(code), std::move(message));
}

void add_discovery_diagnostics(RuntimePackageDiscoveryResidentCommitResultV2& result,
                               const RuntimePackageIndexDiscoveryResultV2& discovery,
                               RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2 phase) {
    for (const auto& diagnostic : discovery.diagnostics) {
        add_discovery_resident_commit_diagnostic(result, phase, {}, diagnostic.path, diagnostic.code,
                                                 diagnostic.message);
    }
}

[[nodiscard]] RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2
resident_operation_phase(RuntimePackageDiscoveryResidentCommitModeV2 mode) noexcept {
    return mode == RuntimePackageDiscoveryResidentCommitModeV2::replace
               ? RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_replace
               : RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_mount;
}

[[nodiscard]] RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2
map_mount_diagnostic_phase(RuntimePackageCandidateResidentMountDiagnosticPhaseV2 phase) noexcept {
    switch (phase) {
    case RuntimePackageCandidateResidentMountDiagnosticPhaseV2::resident_mount:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_mount;
    case RuntimePackageCandidateResidentMountDiagnosticPhaseV2::candidate_load:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::candidate_load;
    case RuntimePackageCandidateResidentMountDiagnosticPhaseV2::resident_budget:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_budget;
    case RuntimePackageCandidateResidentMountDiagnosticPhaseV2::catalog_refresh:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::catalog_refresh;
    }
    return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_commit;
}

[[nodiscard]] RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2
map_replace_diagnostic_phase(RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2 phase) noexcept {
    switch (phase) {
    case RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::resident_replace:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_replace;
    case RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::candidate_load:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::candidate_load;
    case RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::candidate_catalog:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_commit;
    case RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::resident_budget:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_budget;
    case RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::catalog_refresh:
        return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::catalog_refresh;
    }
    return RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_commit;
}

void add_mount_commit_diagnostics(RuntimePackageDiscoveryResidentCommitResultV2& result,
                                  const RuntimePackageCandidateResidentMountResultV2& mount_result) {
    for (const auto& diagnostic : mount_result.diagnostics) {
        add_discovery_resident_commit_diagnostic(result, map_mount_diagnostic_phase(diagnostic.phase), diagnostic.mount,
                                                 diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

void add_replace_commit_diagnostics(RuntimePackageDiscoveryResidentCommitResultV2& result,
                                    const RuntimePackageCandidateResidentReplaceResultV2& replace_result) {
    for (const auto& diagnostic : replace_result.diagnostics) {
        add_discovery_resident_commit_diagnostic(result, map_replace_diagnostic_phase(diagnostic.phase),
                                                 diagnostic.mount, diagnostic.path, diagnostic.code,
                                                 diagnostic.message);
    }
}

void add_discovery_resident_mount_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentMountReviewedEvictionsResultV2& result,
    RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2 phase, AssetId asset,
    RuntimeResidentPackageMountIdV2 mount, std::string path, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticV2{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_discovery_resident_mount_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentMountReviewedEvictionsResultV2& result,
    RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2 phase, RuntimeResidentPackageMountIdV2 mount,
    std::string path, std::string code, std::string message) {
    add_discovery_resident_mount_reviewed_evictions_diagnostic(result, phase, AssetId{}, mount, std::move(path),
                                                               std::move(code), std::move(message));
}

void add_discovery_resident_mount_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentMountReviewedEvictionsResultV2& result,
    RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2 phase, RuntimeResidentPackageMountIdV2 mount,
    std::string code, std::string message) {
    add_discovery_resident_mount_reviewed_evictions_diagnostic(result, phase, mount, {}, std::move(code),
                                                               std::move(message));
}

void add_discovery_diagnostics(RuntimePackageDiscoveryResidentMountReviewedEvictionsResultV2& result,
                               const RuntimePackageIndexDiscoveryResultV2& discovery,
                               RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2 phase) {
    for (const auto& diagnostic : discovery.diagnostics) {
        add_discovery_resident_mount_reviewed_evictions_diagnostic(result, phase, {}, diagnostic.path, diagnostic.code,
                                                                   diagnostic.message);
    }
}

[[nodiscard]] RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2
map_reviewed_evictions_mount_diagnostic_phase(
    RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2 phase) noexcept {
    switch (phase) {
    case RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_mount:
        return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_mount;
    case RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::candidate_load:
        return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::candidate_load;
    case RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::eviction_plan:
        return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::eviction_plan;
    case RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_budget:
        return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_budget;
    case RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::catalog_refresh:
        return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::catalog_refresh;
    }
    return RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_commit;
}

void add_reviewed_evictions_mount_commit_diagnostics(
    RuntimePackageDiscoveryResidentMountReviewedEvictionsResultV2& result,
    const RuntimePackageCandidateResidentMountReviewedEvictionsResultV2& mount_result) {
    for (const auto& diagnostic : mount_result.diagnostics) {
        add_discovery_resident_mount_reviewed_evictions_diagnostic(
            result, map_reviewed_evictions_mount_diagnostic_phase(diagnostic.phase), diagnostic.asset, diagnostic.mount,
            diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

void add_discovery_resident_replace_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2& result,
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2 phase, AssetId asset,
    RuntimeResidentPackageMountIdV2 mount, std::string path, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticV2{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .path = std::move(path),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_discovery_resident_replace_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2& result,
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2 phase,
    RuntimeResidentPackageMountIdV2 mount, std::string path, std::string code, std::string message) {
    add_discovery_resident_replace_reviewed_evictions_diagnostic(result, phase, AssetId{}, mount, std::move(path),
                                                                 std::move(code), std::move(message));
}

void add_discovery_resident_replace_reviewed_evictions_diagnostic(
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2& result,
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2 phase,
    RuntimeResidentPackageMountIdV2 mount, std::string code, std::string message) {
    add_discovery_resident_replace_reviewed_evictions_diagnostic(result, phase, mount, {}, std::move(code),
                                                                 std::move(message));
}

void add_discovery_diagnostics(RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2& result,
                               const RuntimePackageIndexDiscoveryResultV2& discovery,
                               RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2 phase) {
    for (const auto& diagnostic : discovery.diagnostics) {
        add_discovery_resident_replace_reviewed_evictions_diagnostic(result, phase, {}, diagnostic.path,
                                                                     diagnostic.code, diagnostic.message);
    }
}

[[nodiscard]] RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2
map_reviewed_evictions_replace_diagnostic_phase(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2 phase) noexcept {
    switch (phase) {
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_replace:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_replace;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::candidate_load:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::candidate_load;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::candidate_catalog:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::candidate_catalog;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::eviction_plan:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::eviction_plan;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_budget:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_budget;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::catalog_refresh:
        return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::catalog_refresh;
    }
    return RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_commit;
}

void add_reviewed_evictions_replace_commit_diagnostics(
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2& result,
    const RuntimePackageCandidateResidentReplaceReviewedEvictionsResultV2& replace_result) {
    for (const auto& diagnostic : replace_result.diagnostics) {
        add_discovery_resident_replace_reviewed_evictions_diagnostic(
            result, map_reviewed_evictions_replace_diagnostic_phase(diagnostic.phase), diagnostic.asset,
            diagnostic.mount, diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_evictions_candidate_load_diagnostics(
    RuntimePackageCandidateResidentMountReviewedEvictionsResultV2& result,
    const RuntimePackageCandidateLoadResultV2& load_result, RuntimeResidentPackageMountIdV2 mount) {
    for (const auto& diagnostic : load_result.diagnostics) {
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::candidate_load,
            diagnostic.asset, mount, diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_evictions_eviction_plan_diagnostics(
    RuntimePackageCandidateResidentMountReviewedEvictionsResultV2& result,
    const RuntimeResidentPackageEvictionPlanResultV2& eviction_plan) {
    for (const auto& diagnostic : eviction_plan.diagnostics) {
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::eviction_plan,
            diagnostic.mount, diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_evictions_budget_diagnostics(RuntimePackageCandidateResidentMountReviewedEvictionsResultV2& result,
                                               const RuntimeResourceResidencyBudgetExecutionResultV2& budget_execution,
                                               RuntimeResidentPackageMountIdV2 mount) {
    for (const auto& diagnostic : budget_execution.diagnostics) {
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_budget, mount,
            diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_evictions_catalog_diagnostics(RuntimePackageCandidateResidentMountReviewedEvictionsResultV2& result,
                                                const RuntimeResourceCatalogBuildResultV2& catalog_build,
                                                RuntimeResidentPackageMountIdV2 mount) {
    for (const auto& diagnostic : catalog_build.diagnostics) {
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::catalog_refresh,
            diagnostic.asset, mount, {}, "catalog-build-failed", diagnostic.diagnostic);
    }
}

[[nodiscard]] RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2
map_eviction_plan_status(RuntimeResidentPackageEvictionPlanStatusV2 status) noexcept {
    switch (status) {
    case RuntimeResidentPackageEvictionPlanStatusV2::invalid_candidate_mount_id:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::invalid_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::duplicate_candidate_mount_id:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::duplicate_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::missing_candidate_mount_id:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::missing_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::protected_candidate_mount_id:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::protected_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::catalog_build_failed:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::catalog_refresh_failed;
    case RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable:
    case RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required:
    case RuntimeResidentPackageEvictionPlanStatusV2::planned:
        return RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::budget_failed;
    }
    return RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::budget_failed;
}

void add_replace_reviewed_evictions_candidate_load_diagnostics(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResultV2& result,
    const RuntimePackageCandidateLoadResultV2& load_result, RuntimeResidentPackageMountIdV2 mount) {
    for (const auto& diagnostic : load_result.diagnostics) {
        add_candidate_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::candidate_load,
            diagnostic.asset, mount, diagnostic.path, diagnostic.code, diagnostic.message);
    }
}

void add_replace_reviewed_evictions_eviction_plan_diagnostics(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResultV2& result,
    const RuntimeResidentPackageEvictionPlanResultV2& eviction_plan) {
    for (const auto& diagnostic : eviction_plan.diagnostics) {
        add_candidate_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::eviction_plan,
            diagnostic.mount, diagnostic.code, diagnostic.message);
    }
}

void add_replace_reviewed_evictions_budget_diagnostics(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResultV2& result,
    const RuntimeResourceResidencyBudgetExecutionResultV2& budget_execution, RuntimeResidentPackageMountIdV2 mount) {
    for (const auto& diagnostic : budget_execution.diagnostics) {
        add_candidate_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_budget, mount,
            diagnostic.code, diagnostic.message);
    }
}

void add_replace_reviewed_evictions_catalog_diagnostics(
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResultV2& result,
    RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2 phase,
    const RuntimeResourceCatalogBuildResultV2& catalog_build, RuntimeResidentPackageMountIdV2 mount,
    std::string_view code) {
    for (const auto& diagnostic : catalog_build.diagnostics) {
        add_candidate_resident_replace_reviewed_evictions_diagnostic(result, phase, diagnostic.asset, mount, {},
                                                                     std::string{code}, diagnostic.diagnostic);
    }
}

[[nodiscard]] RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2
map_replace_reviewed_evictions_plan_status(RuntimeResidentPackageEvictionPlanStatusV2 status) noexcept {
    switch (status) {
    case RuntimeResidentPackageEvictionPlanStatusV2::invalid_candidate_mount_id:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::invalid_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::duplicate_candidate_mount_id:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::duplicate_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::missing_candidate_mount_id:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::missing_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::protected_candidate_mount_id:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::protected_eviction_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::catalog_build_failed:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::catalog_refresh_failed;
    case RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable:
    case RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required:
    case RuntimeResidentPackageEvictionPlanStatusV2::planned:
        return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::budget_failed;
    }
    return RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::budget_failed;
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

[[nodiscard]] bool contains_mount_id(const std::vector<RuntimeResidentPackageMountIdV2>& ids,
                                     RuntimeResidentPackageMountIdV2 id) noexcept {
    for (const auto candidate : ids) {
        if (candidate == id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_mount_id(const RuntimeResidentPackageMountSetV2& mount_set,
                                RuntimeResidentPackageMountIdV2 id) noexcept {
    for (const auto& mounted : mount_set.mounts()) {
        if (mounted.id == id) {
            return true;
        }
    }
    return false;
}

void add_eviction_diagnostic(RuntimeResidentPackageEvictionPlanResultV2& result, RuntimeResidentPackageMountIdV2 mount,
                             std::string code, std::string message) {
    result.diagnostics.push_back(RuntimeResidentPackageEvictionPlanDiagnosticV2{
        .mount = mount,
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_reviewed_eviction_commit_diagnostic(RuntimeResidentPackageReviewedEvictionCommitResultV2& result,
                                             RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhaseV2 phase,
                                             AssetId asset, RuntimeResidentPackageMountIdV2 mount, std::string code,
                                             std::string message) {
    result.diagnostics.push_back(RuntimeResidentPackageReviewedEvictionCommitDiagnosticV2{
        .phase = phase,
        .asset = asset,
        .mount = mount,
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_reviewed_eviction_commit_plan_diagnostics(RuntimeResidentPackageReviewedEvictionCommitResultV2& result,
                                                   const RuntimeResidentPackageEvictionPlanResultV2& eviction_plan) {
    for (const auto& diagnostic : eviction_plan.diagnostics) {
        add_reviewed_eviction_commit_diagnostic(
            result, RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhaseV2::eviction_plan, {}, diagnostic.mount,
            diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_eviction_commit_budget_diagnostics(
    RuntimeResidentPackageReviewedEvictionCommitResultV2& result,
    const RuntimeResourceResidencyBudgetExecutionResultV2& budget_execution) {
    for (const auto& diagnostic : budget_execution.diagnostics) {
        add_reviewed_eviction_commit_diagnostic(
            result, RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhaseV2::resident_budget, {}, {},
            diagnostic.code, diagnostic.message);
    }
}

void add_reviewed_eviction_commit_catalog_diagnostics(RuntimeResidentPackageReviewedEvictionCommitResultV2& result,
                                                      const RuntimeResourceCatalogBuildResultV2& catalog_build) {
    for (const auto& diagnostic : catalog_build.diagnostics) {
        add_reviewed_eviction_commit_diagnostic(
            result, RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhaseV2::catalog_refresh, diagnostic.asset,
            {}, "catalog-build-failed", diagnostic.diagnostic);
    }
}

[[nodiscard]] RuntimeResidentPackageReviewedEvictionCommitStatusV2
map_reviewed_eviction_commit_plan_status(RuntimeResidentPackageEvictionPlanStatusV2 status) noexcept {
    switch (status) {
    case RuntimeResidentPackageEvictionPlanStatusV2::invalid_candidate_mount_id:
        return RuntimeResidentPackageReviewedEvictionCommitStatusV2::invalid_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::duplicate_candidate_mount_id:
        return RuntimeResidentPackageReviewedEvictionCommitStatusV2::duplicate_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::missing_candidate_mount_id:
        return RuntimeResidentPackageReviewedEvictionCommitStatusV2::missing_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::protected_candidate_mount_id:
        return RuntimeResidentPackageReviewedEvictionCommitStatusV2::protected_candidate_mount_id;
    case RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable:
        return RuntimeResidentPackageReviewedEvictionCommitStatusV2::budget_failed;
    case RuntimeResidentPackageEvictionPlanStatusV2::catalog_build_failed:
        return RuntimeResidentPackageReviewedEvictionCommitStatusV2::catalog_refresh_failed;
    case RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required:
        return RuntimeResidentPackageReviewedEvictionCommitStatusV2::no_eviction_required;
    case RuntimeResidentPackageEvictionPlanStatusV2::planned:
        return RuntimeResidentPackageReviewedEvictionCommitStatusV2::committed;
    }
    return RuntimeResidentPackageReviewedEvictionCommitStatusV2::budget_failed;
}

} // namespace

bool RuntimePackageIndexDiscoveryResultV2::succeeded() const noexcept {
    return status == RuntimePackageIndexDiscoveryStatusV2::discovered ||
           status == RuntimePackageIndexDiscoveryStatusV2::no_candidates;
}

bool RuntimePackageCandidateLoadResultV2::succeeded() const noexcept {
    return status == RuntimePackageCandidateLoadStatusV2::loaded && loaded_package.succeeded();
}

RuntimePackageIndexDiscoveryResultV2
discover_runtime_package_indexes_v2(const IFileSystem& filesystem, const RuntimePackageIndexDiscoveryDescV2& desc) {
    RuntimePackageIndexDiscoveryResultV2 result;
    result.root = trim_trailing_slashes(desc.root);

    if (!valid_relative_vfs_path(result.root)) {
        result.status = RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor;
        add_discovery_diagnostic(result, result.root, "invalid-root",
                                 "runtime package discovery root must be a non-empty relative VFS directory");
        return result;
    }

    const auto content_root = trim_trailing_slashes(desc.content_root);
    if (!content_root.empty() && !valid_relative_vfs_path(content_root)) {
        result.status = RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor;
        add_discovery_diagnostic(result, content_root, "invalid-content-root",
                                 "runtime package discovery content root must be a relative VFS directory");
        return result;
    }

    try {
        if (!filesystem.is_directory(result.root)) {
            result.status = RuntimePackageIndexDiscoveryStatusV2::missing_root;
            add_discovery_diagnostic(result, result.root, "missing-root",
                                     "runtime package discovery root is not a directory");
            return result;
        }
    } catch (const std::exception& exception) {
        result.status = RuntimePackageIndexDiscoveryStatusV2::scan_failed;
        add_discovery_diagnostic(result, result.root, "scan-failed", exception.what());
        return result;
    }

    std::vector<std::string> files;
    try {
        files = filesystem.list_files(result.root);
    } catch (const std::exception& exception) {
        result.status = RuntimePackageIndexDiscoveryStatusV2::scan_failed;
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
        result.candidates.push_back(RuntimePackageIndexDiscoveryCandidateV2{
            .package_index_path = file,
            .content_root = content_root,
            .label = label,
        });
    }

    std::ranges::sort(result.candidates, [](const RuntimePackageIndexDiscoveryCandidateV2& lhs,
                                            const RuntimePackageIndexDiscoveryCandidateV2& rhs) {
        return lhs.package_index_path < rhs.package_index_path;
    });
    const auto duplicate_begin =
        std::ranges::unique(result.candidates, {}, &RuntimePackageIndexDiscoveryCandidateV2::package_index_path)
            .begin();
    result.candidates.erase(duplicate_begin, result.candidates.end());
    result.status = result.candidates.empty() ? RuntimePackageIndexDiscoveryStatusV2::no_candidates
                                              : RuntimePackageIndexDiscoveryStatusV2::discovered;
    return result;
}

RuntimePackageCandidateLoadResultV2
load_runtime_package_candidate_v2(IFileSystem& filesystem, const RuntimePackageIndexDiscoveryCandidateV2& candidate) {
    RuntimePackageCandidateLoadResultV2 result;
    result.candidate = candidate;

    if (!valid_relative_vfs_path(candidate.package_index_path) ||
        !ends_with_package_index_extension(candidate.package_index_path)) {
        result.status = RuntimePackageCandidateLoadStatusV2::invalid_candidate;
        add_candidate_load_diagnostic(result, candidate.package_index_path, "invalid-package-index-path",
                                      "runtime package load candidate must reference a relative .geindex file path");
        return result;
    }
    if (!candidate.content_root.empty() && !valid_relative_vfs_path(candidate.content_root)) {
        result.status = RuntimePackageCandidateLoadStatusV2::invalid_candidate;
        add_candidate_load_diagnostic(result, candidate.content_root, "invalid-content-root",
                                      "runtime package load candidate content root must be a relative VFS directory");
        return result;
    }
    if (!valid_relative_vfs_path(candidate.label)) {
        result.status = RuntimePackageCandidateLoadStatusV2::invalid_candidate;
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
        result.status = RuntimePackageCandidateLoadStatusV2::package_load_failed;
        add_candidate_load_diagnostic(result, candidate.package_index_path, "package-index-invalid", exception.what());
        return result;
    } catch (const std::exception& exception) {
        result.status = RuntimePackageCandidateLoadStatusV2::read_failed;
        add_candidate_load_diagnostic(result, candidate.package_index_path, "package-read-failed", exception.what());
        return result;
    }

    if (!result.loaded_package.succeeded()) {
        result.status = RuntimePackageCandidateLoadStatusV2::package_load_failed;
        for (const auto& failure : result.loaded_package.failures) {
            add_candidate_load_diagnostic(result, failure.asset, failure.path, "package-load-failed",
                                          failure.diagnostic);
        }
        return result;
    }

    result.status = RuntimePackageCandidateLoadStatusV2::loaded;
    result.loaded_record_count = result.loaded_package.package.records().size();
    result.estimated_resident_bytes = estimate_runtime_asset_package_resident_bytes(result.loaded_package.package);
    return result;
}

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

bool RuntimePackageCandidateResidentMountResultV2::succeeded() const noexcept {
    return status == RuntimePackageCandidateResidentMountStatusV2::mounted && committed;
}

RuntimePackageCandidateResidentMountResultV2
commit_runtime_package_candidate_resident_mount_v2(IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set,
                                                   RuntimeResidentCatalogCacheV2& catalog_cache,
                                                   const RuntimePackageCandidateResidentMountDescV2& desc) {
    RuntimePackageCandidateResidentMountResultV2 result;
    result.candidate = desc.candidate;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageCandidateResidentMountStatusV2::invalid_mount_id;
        result.resident_mount = mount_result(desc.mount_id, RuntimeResidentPackageMountStatusV2::invalid_mount_id,
                                             "invalid-mount-id", "resident package mount ids must be non-zero");
        add_candidate_resident_mount_diagnostic(
            result, RuntimePackageCandidateResidentMountDiagnosticPhaseV2::resident_mount, desc.mount_id,
            result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }
    if (has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageCandidateResidentMountStatusV2::duplicate_mount_id;
        result.resident_mount = mount_result(desc.mount_id, RuntimeResidentPackageMountStatusV2::duplicate_mount_id,
                                             "duplicate-mount-id", "resident package mount id is already mounted");
        add_candidate_resident_mount_diagnostic(
            result, RuntimePackageCandidateResidentMountDiagnosticPhaseV2::resident_mount, desc.mount_id,
            result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }

    result.invoked_candidate_load = true;
    result.candidate_load = load_runtime_package_candidate_v2(filesystem, desc.candidate);
    result.package_desc = result.candidate_load.package_desc;
    result.loaded_record_count = result.candidate_load.loaded_record_count;
    result.loaded_resident_bytes = result.candidate_load.estimated_resident_bytes;
    if (!result.candidate_load.succeeded()) {
        result.status = RuntimePackageCandidateResidentMountStatusV2::candidate_load_failed;
        for (const auto& diagnostic : result.candidate_load.diagnostics) {
            add_candidate_resident_mount_diagnostic(
                result, RuntimePackageCandidateResidentMountDiagnosticPhaseV2::candidate_load, diagnostic.asset,
                desc.mount_id, diagnostic.path, diagnostic.code, diagnostic.message);
        }
        return result;
    }

    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    result.resident_mount = projected_mount_set.mount(RuntimeResidentPackageMountRecordV2{
        .id = desc.mount_id,
        .label = desc.candidate.label,
        .package = result.candidate_load.loaded_package.package,
    });
    if (!result.resident_mount.succeeded()) {
        result.status = result.resident_mount.status == RuntimeResidentPackageMountStatusV2::duplicate_mount_id
                            ? RuntimePackageCandidateResidentMountStatusV2::duplicate_mount_id
                            : RuntimePackageCandidateResidentMountStatusV2::invalid_mount_id;
        add_candidate_resident_mount_diagnostic(
            result, RuntimePackageCandidateResidentMountDiagnosticPhaseV2::resident_mount, desc.mount_id,
            result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }

    RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    result.invoked_catalog_refresh = true;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, desc.overlay, desc.budget);
    result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
    if (!result.catalog_refresh.succeeded()) {
        if (result.catalog_refresh.status == RuntimeResidentCatalogCacheStatusV2::budget_failed) {
            result.status = RuntimePackageCandidateResidentMountStatusV2::budget_failed;
            for (const auto& diagnostic : result.catalog_refresh.budget_execution.diagnostics) {
                add_candidate_resident_mount_diagnostic(
                    result, RuntimePackageCandidateResidentMountDiagnosticPhaseV2::resident_budget, desc.mount_id,
                    diagnostic.code, diagnostic.message);
            }
        } else {
            result.status = RuntimePackageCandidateResidentMountStatusV2::catalog_refresh_failed;
            for (const auto& diagnostic : result.catalog_refresh.catalog_build.diagnostics) {
                add_candidate_resident_mount_diagnostic(
                    result, RuntimePackageCandidateResidentMountDiagnosticPhaseV2::catalog_refresh, diagnostic.asset,
                    desc.mount_id, {}, "catalog-build-failed", diagnostic.diagnostic);
            }
        }
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.status = RuntimePackageCandidateResidentMountStatusV2::mounted;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    result.committed = true;
    return result;
}

bool RuntimePackageCandidateResidentReplaceResultV2::succeeded() const noexcept {
    return status == RuntimePackageCandidateResidentReplaceStatusV2::replaced && committed;
}

RuntimePackageCandidateResidentReplaceResultV2 commit_runtime_package_candidate_resident_replace_v2(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    const RuntimePackageCandidateResidentReplaceDescV2& desc) {
    RuntimePackageCandidateResidentReplaceResultV2 result;
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
        result.status = RuntimePackageCandidateResidentReplaceStatusV2::invalid_mount_id;
        result.resident_replace.status = RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id;
        result.resident_replace.diagnostic.code = "invalid-mount-id";
        result.resident_replace.diagnostic.message = "resident package mount ids must be non-zero";
        add_candidate_resident_replace_diagnostic(
            result, RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::resident_replace, desc.mount_id,
            result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
        return result;
    }
    if (!has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageCandidateResidentReplaceStatusV2::missing_mount_id;
        result.resident_replace.status = RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id;
        result.resident_replace.diagnostic.code = "missing-mount-id";
        result.resident_replace.diagnostic.message = "resident package mount id is not mounted";
        add_candidate_resident_replace_diagnostic(
            result, RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::resident_replace, desc.mount_id,
            result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
        return result;
    }

    result.invoked_candidate_load = true;
    result.candidate_load = load_runtime_package_candidate_v2(filesystem, desc.candidate);
    result.package_desc = result.candidate_load.package_desc;
    result.loaded_record_count = result.candidate_load.loaded_record_count;
    result.loaded_resident_bytes = result.candidate_load.estimated_resident_bytes;
    if (!result.candidate_load.succeeded()) {
        result.status = RuntimePackageCandidateResidentReplaceStatusV2::candidate_load_failed;
        for (const auto& diagnostic : result.candidate_load.diagnostics) {
            add_candidate_resident_replace_diagnostic(
                result, RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::candidate_load, diagnostic.asset,
                desc.mount_id, diagnostic.path, diagnostic.code, diagnostic.message);
        }
        return result;
    }

    result.invoked_resident_replace = true;
    result.resident_replace = commit_runtime_resident_package_replace_v2(mount_set, catalog_cache, desc.mount_id,
                                                                         result.candidate_load.loaded_package.package,
                                                                         desc.overlay, desc.budget);
    result.catalog_refresh = result.resident_replace.catalog_refresh;
    result.invoked_catalog_refresh =
        result.resident_replace.succeeded() ||
        result.resident_replace.status == RuntimeResidentPackageReplaceCommitStatusV2::budget_failed ||
        result.catalog_refresh.invoked_catalog_build || result.catalog_refresh.reused_cache;
    result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
    result.mount_generation = result.resident_replace.mount_generation;
    result.mounted_package_count = result.resident_replace.mounted_package_count;

    if (!result.resident_replace.succeeded()) {
        switch (result.resident_replace.status) {
        case RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id:
            result.status = RuntimePackageCandidateResidentReplaceStatusV2::invalid_mount_id;
            add_candidate_resident_replace_diagnostic(
                result, RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::resident_replace, desc.mount_id,
                result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id:
            result.status = RuntimePackageCandidateResidentReplaceStatusV2::missing_mount_id;
            add_candidate_resident_replace_diagnostic(
                result, RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::resident_replace, desc.mount_id,
                result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::budget_failed:
            result.status = RuntimePackageCandidateResidentReplaceStatusV2::budget_failed;
            for (const auto& diagnostic : result.catalog_refresh.budget_execution.diagnostics) {
                add_candidate_resident_replace_diagnostic(
                    result, RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::resident_budget, desc.mount_id,
                    diagnostic.code, diagnostic.message);
            }
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::catalog_build_failed:
            if (result.catalog_refresh.invoked_catalog_build) {
                result.status = RuntimePackageCandidateResidentReplaceStatusV2::catalog_refresh_failed;
                for (const auto& diagnostic : result.catalog_refresh.catalog_build.diagnostics) {
                    add_candidate_resident_replace_diagnostic(
                        result, RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::catalog_refresh,
                        diagnostic.asset, desc.mount_id, {}, "catalog-build-failed", diagnostic.diagnostic);
                }
            } else {
                result.status = RuntimePackageCandidateResidentReplaceStatusV2::resident_replace_failed;
                for (const auto& diagnostic : result.resident_replace.candidate_catalog_build.diagnostics) {
                    add_candidate_resident_replace_diagnostic(
                        result, RuntimePackageCandidateResidentReplaceDiagnosticPhaseV2::candidate_catalog,
                        diagnostic.asset, desc.mount_id, {}, "candidate-catalog-build-failed", diagnostic.diagnostic);
                }
            }
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::replaced:
            break;
        }
        return result;
    }

    result.status = RuntimePackageCandidateResidentReplaceStatusV2::replaced;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    result.committed = true;
    return result;
}

bool RuntimePackageCandidateResidentReplaceReviewedEvictionsResultV2::succeeded() const noexcept {
    return status == RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::replaced && committed;
}

RuntimePackageCandidateResidentReplaceReviewedEvictionsResultV2
commit_runtime_package_candidate_resident_replace_with_reviewed_evictions_v2(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    const RuntimePackageCandidateResidentReplaceReviewedEvictionsDescV2& desc) {
    RuntimePackageCandidateResidentReplaceReviewedEvictionsResultV2 result;
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
        result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::invalid_mount_id;
        result.resident_replace.status = RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id;
        result.resident_replace.diagnostic.code = "invalid-mount-id";
        result.resident_replace.diagnostic.message = "resident package mount ids must be non-zero";
        add_candidate_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_replace,
            desc.mount_id, result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
        return result;
    }
    if (!has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::missing_mount_id;
        result.resident_replace.status = RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id;
        result.resident_replace.diagnostic.code = "missing-mount-id";
        result.resident_replace.diagnostic.message = "resident package mount id is not mounted";
        add_candidate_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_replace,
            desc.mount_id, result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
        return result;
    }

    result.invoked_candidate_load = true;
    result.candidate_load = load_runtime_package_candidate_v2(filesystem, desc.candidate);
    result.package_desc = result.candidate_load.package_desc;
    result.loaded_record_count = result.candidate_load.loaded_record_count;
    result.loaded_resident_bytes = result.candidate_load.estimated_resident_bytes;
    if (!result.candidate_load.succeeded()) {
        result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::candidate_load_failed;
        add_replace_reviewed_evictions_candidate_load_diagnostics(result, result.candidate_load, desc.mount_id);
        return result;
    }

    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    result.invoked_resident_replace = true;
    result.resident_replace = commit_runtime_resident_package_replace_v2(
        projected_mount_set, projected_catalog_cache, desc.mount_id, result.candidate_load.loaded_package.package,
        desc.overlay, RuntimeResourceResidencyBudgetV2{});
    if (!result.resident_replace.succeeded()) {
        result.catalog_refresh = result.resident_replace.catalog_refresh;
        result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
        switch (result.resident_replace.status) {
        case RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id:
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::invalid_mount_id;
            add_candidate_resident_replace_reviewed_evictions_diagnostic(
                result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_replace,
                desc.mount_id, result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id:
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::missing_mount_id;
            add_candidate_resident_replace_reviewed_evictions_diagnostic(
                result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_replace,
                desc.mount_id, result.resident_replace.diagnostic.code, result.resident_replace.diagnostic.message);
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::budget_failed:
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::budget_failed;
            add_replace_reviewed_evictions_budget_diagnostics(result, result.catalog_refresh.budget_execution,
                                                              desc.mount_id);
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::catalog_build_failed:
            if (result.catalog_refresh.invoked_catalog_build) {
                result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::catalog_refresh_failed;
                add_replace_reviewed_evictions_catalog_diagnostics(
                    result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::catalog_refresh,
                    result.catalog_refresh.catalog_build, desc.mount_id, "catalog-build-failed");
            } else {
                result.status =
                    RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::resident_replace_failed;
                add_replace_reviewed_evictions_catalog_diagnostics(
                    result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::candidate_catalog,
                    result.resident_replace.candidate_catalog_build, desc.mount_id, "candidate-catalog-build-failed");
            }
            break;
        case RuntimeResidentPackageReplaceCommitStatusV2::replaced:
            break;
        }
        return result;
    }

    std::vector<RuntimeResidentPackageMountIdV2> protected_mounts = desc.protected_mount_ids;
    if (!contains_mount_id(protected_mounts, desc.mount_id)) {
        protected_mounts.push_back(desc.mount_id);
    }

    result.invoked_eviction_plan = true;
    result.eviction_plan = plan_runtime_resident_package_evictions_v2(
        projected_mount_set, RuntimeResidentPackageEvictionPlanDescV2{
                                 .target_budget = desc.budget,
                                 .overlay = desc.overlay,
                                 .candidate_unmount_order = desc.eviction_candidate_unmount_order,
                                 .protected_mount_ids = std::move(protected_mounts),
                             });
    if (!result.eviction_plan.succeeded()) {
        result.status = map_replace_reviewed_evictions_plan_status(result.eviction_plan.status);
        add_replace_reviewed_evictions_eviction_plan_diagnostics(result, result.eviction_plan);
        if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable) {
            add_replace_reviewed_evictions_budget_diagnostics(
                result, result.eviction_plan.projected_refresh.budget_execution, desc.mount_id);
        } else if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatusV2::catalog_build_failed) {
            add_replace_reviewed_evictions_catalog_diagnostics(
                result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::catalog_refresh,
                result.eviction_plan.projected_refresh.catalog_build, desc.mount_id, "catalog-build-failed");
        }
        result.projected_resident_bytes =
            result.eviction_plan.projected_refresh.budget_execution.estimated_resident_content_bytes;
        return result;
    }

    for (const auto& step : result.eviction_plan.steps) {
        const auto unmount = projected_mount_set.unmount(step.mount_id);
        if (!unmount.succeeded()) {
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::catalog_refresh_failed;
            add_candidate_resident_replace_reviewed_evictions_diagnostic(
                result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::eviction_plan,
                step.mount_id, unmount.diagnostic.code, unmount.diagnostic.message);
            return result;
        }
    }
    result.evicted_mount_count = result.eviction_plan.steps.size();

    result.invoked_catalog_refresh = true;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, desc.overlay, desc.budget);
    result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
    if (!result.catalog_refresh.succeeded()) {
        if (result.catalog_refresh.status == RuntimeResidentCatalogCacheStatusV2::budget_failed) {
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::budget_failed;
            add_replace_reviewed_evictions_budget_diagnostics(result, result.catalog_refresh.budget_execution,
                                                              desc.mount_id);
        } else {
            result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::catalog_refresh_failed;
            add_replace_reviewed_evictions_catalog_diagnostics(
                result, RuntimePackageCandidateResidentReplaceReviewedEvictionsDiagnosticPhaseV2::catalog_refresh,
                result.catalog_refresh.catalog_build, desc.mount_id, "catalog-build-failed");
        }
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.status = RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::replaced;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    result.committed = true;
    return result;
}

bool RuntimePackageDiscoveryResidentCommitResultV2::succeeded() const noexcept {
    return status == RuntimePackageDiscoveryResidentCommitStatusV2::committed && committed;
}

RuntimePackageDiscoveryResidentCommitResultV2
commit_runtime_package_discovery_resident_v2(IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set,
                                             RuntimeResidentCatalogCacheV2& catalog_cache,
                                             const RuntimePackageDiscoveryResidentCommitDescV2& desc) {
    RuntimePackageDiscoveryResidentCommitResultV2 result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    if (!valid_relative_vfs_path(desc.selected_package_index_path) ||
        !ends_with_package_index_extension(desc.selected_package_index_path)) {
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::invalid_descriptor;
        add_discovery_resident_commit_diagnostic(
            result, RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::descriptor, desc.mount_id,
            desc.selected_package_index_path, "invalid-selected-package-index-path",
            "runtime package discovery resident commit requires a relative selected .geindex path");
        return result;
    }

    switch (desc.mode) {
    case RuntimePackageDiscoveryResidentCommitModeV2::mount:
    case RuntimePackageDiscoveryResidentCommitModeV2::replace:
        break;
    default:
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::invalid_descriptor;
        add_discovery_resident_commit_diagnostic(
            result, RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::descriptor, desc.mount_id,
            "invalid-commit-mode", "runtime package discovery resident commit mode is not supported");
        return result;
    }

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::invalid_mount_id;
        add_discovery_resident_commit_diagnostic(result, resident_operation_phase(desc.mode), desc.mount_id,
                                                 "invalid-mount-id", "resident package mount ids must be non-zero");
        return result;
    }
    if (desc.mode == RuntimePackageDiscoveryResidentCommitModeV2::mount && has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::duplicate_mount_id;
        add_discovery_resident_commit_diagnostic(
            result, RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_mount, desc.mount_id,
            "duplicate-mount-id", "resident package mount id is already mounted");
        return result;
    }
    if (desc.mode == RuntimePackageDiscoveryResidentCommitModeV2::replace && !has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::missing_mount_id;
        add_discovery_resident_commit_diagnostic(
            result, RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_replace, desc.mount_id,
            "missing-mount-id", "resident package mount id is not mounted");
        return result;
    }

    result.invoked_discovery = true;
    result.discovery = discover_runtime_package_indexes_v2(filesystem, desc.discovery);
    if (!result.discovery.succeeded()) {
        const auto descriptor_failure =
            result.discovery.status == RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor;
        result.status = descriptor_failure ? RuntimePackageDiscoveryResidentCommitStatusV2::invalid_descriptor
                                           : RuntimePackageDiscoveryResidentCommitStatusV2::discovery_failed;
        add_discovery_diagnostics(result, result.discovery,
                                  descriptor_failure
                                      ? RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::descriptor
                                      : RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::discovery);
        return result;
    }

    const auto selected =
        std::ranges::find_if(result.discovery.candidates, [&desc](const RuntimePackageIndexDiscoveryCandidateV2& item) {
            return item.package_index_path == desc.selected_package_index_path;
        });
    if (selected == result.discovery.candidates.end()) {
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::candidate_not_found;
        add_discovery_resident_commit_diagnostic(
            result, RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::candidate_selection, desc.mount_id,
            desc.selected_package_index_path, "candidate-not-found",
            "selected runtime package index was not discovered under the reviewed root");
        return result;
    }
    result.selected_candidate = *selected;

    result.invoked_resident_commit = true;
    if (desc.mode == RuntimePackageDiscoveryResidentCommitModeV2::mount) {
        result.resident_mount =
            commit_runtime_package_candidate_resident_mount_v2(filesystem, mount_set, catalog_cache,
                                                               RuntimePackageCandidateResidentMountDescV2{
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
        case RuntimePackageCandidateResidentMountStatusV2::mounted:
            result.status = RuntimePackageDiscoveryResidentCommitStatusV2::committed;
            result.committed = true;
            break;
        case RuntimePackageCandidateResidentMountStatusV2::invalid_mount_id:
            result.status = RuntimePackageDiscoveryResidentCommitStatusV2::invalid_mount_id;
            break;
        case RuntimePackageCandidateResidentMountStatusV2::duplicate_mount_id:
            result.status = RuntimePackageDiscoveryResidentCommitStatusV2::duplicate_mount_id;
            break;
        case RuntimePackageCandidateResidentMountStatusV2::candidate_load_failed:
            result.status = RuntimePackageDiscoveryResidentCommitStatusV2::candidate_load_failed;
            break;
        case RuntimePackageCandidateResidentMountStatusV2::budget_failed:
            result.status = RuntimePackageDiscoveryResidentCommitStatusV2::budget_failed;
            break;
        case RuntimePackageCandidateResidentMountStatusV2::catalog_refresh_failed:
            result.status = RuntimePackageDiscoveryResidentCommitStatusV2::catalog_refresh_failed;
            break;
        }
        return result;
    }

    result.resident_replace =
        commit_runtime_package_candidate_resident_replace_v2(filesystem, mount_set, catalog_cache,
                                                             RuntimePackageCandidateResidentReplaceDescV2{
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
    case RuntimePackageCandidateResidentReplaceStatusV2::replaced:
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::committed;
        result.committed = true;
        break;
    case RuntimePackageCandidateResidentReplaceStatusV2::invalid_mount_id:
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::invalid_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceStatusV2::missing_mount_id:
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::missing_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceStatusV2::candidate_load_failed:
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::candidate_load_failed;
        break;
    case RuntimePackageCandidateResidentReplaceStatusV2::resident_replace_failed:
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::resident_commit_failed;
        break;
    case RuntimePackageCandidateResidentReplaceStatusV2::budget_failed:
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::budget_failed;
        break;
    case RuntimePackageCandidateResidentReplaceStatusV2::catalog_refresh_failed:
        result.status = RuntimePackageDiscoveryResidentCommitStatusV2::catalog_refresh_failed;
        break;
    }
    return result;
}

bool RuntimePackageDiscoveryResidentMountReviewedEvictionsResultV2::succeeded() const noexcept {
    return status == RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::committed && committed;
}

RuntimePackageDiscoveryResidentMountReviewedEvictionsResultV2
commit_runtime_package_discovery_resident_mount_with_reviewed_evictions_v2(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    const RuntimePackageDiscoveryResidentMountReviewedEvictionsDescV2& desc) {
    RuntimePackageDiscoveryResidentMountReviewedEvictionsResultV2 result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    if (!valid_relative_vfs_path(desc.selected_package_index_path) ||
        !ends_with_package_index_extension(desc.selected_package_index_path)) {
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::invalid_descriptor;
        add_discovery_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::descriptor, desc.mount_id,
            desc.selected_package_index_path, "invalid-selected-package-index-path",
            "runtime package discovery reviewed-eviction mount requires a relative selected .geindex path");
        return result;
    }

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::invalid_mount_id;
        add_discovery_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_mount,
            desc.mount_id, "invalid-mount-id", "resident package mount ids must be non-zero");
        return result;
    }
    if (has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::duplicate_mount_id;
        add_discovery_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_mount,
            desc.mount_id, "duplicate-mount-id", "resident package mount id is already mounted");
        return result;
    }

    result.invoked_discovery = true;
    result.discovery = discover_runtime_package_indexes_v2(filesystem, desc.discovery);
    if (!result.discovery.succeeded()) {
        const auto descriptor_failure =
            result.discovery.status == RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor;
        result.status = descriptor_failure
                            ? RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::invalid_descriptor
                            : RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::discovery_failed;
        add_discovery_diagnostics(
            result, result.discovery,
            descriptor_failure ? RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::descriptor
                               : RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::discovery);
        return result;
    }

    const auto selected =
        std::ranges::find_if(result.discovery.candidates, [&desc](const RuntimePackageIndexDiscoveryCandidateV2& item) {
            return item.package_index_path == desc.selected_package_index_path;
        });
    if (selected == result.discovery.candidates.end()) {
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::candidate_not_found;
        add_discovery_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentMountReviewedEvictionsDiagnosticPhaseV2::candidate_selection,
            desc.mount_id, desc.selected_package_index_path, "candidate-not-found",
            "selected runtime package index was not discovered under the reviewed root");
        return result;
    }
    result.selected_candidate = *selected;

    result.invoked_resident_commit = true;
    result.candidate_resident_mount = commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        RuntimePackageCandidateResidentMountReviewedEvictionsDescV2{
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
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::mounted:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::committed;
        result.committed = true;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::invalid_mount_id:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::invalid_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::duplicate_mount_id:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::duplicate_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::candidate_load_failed:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::candidate_load_failed;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::invalid_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::invalid_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::duplicate_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::duplicate_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::missing_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::missing_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::protected_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::protected_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::budget_failed:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::budget_failed;
        break;
    case RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::catalog_refresh_failed:
        result.status = RuntimePackageDiscoveryResidentMountReviewedEvictionsStatusV2::catalog_refresh_failed;
        break;
    }
    return result;
}

bool RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2::succeeded() const noexcept {
    return status == RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::committed && committed;
}

RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2
commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    const RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDescV2& desc) {
    RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2 result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    if (!valid_relative_vfs_path(desc.selected_package_index_path) ||
        !ends_with_package_index_extension(desc.selected_package_index_path)) {
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::invalid_descriptor;
        add_discovery_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::descriptor, desc.mount_id,
            desc.selected_package_index_path, "invalid-selected-package-index-path",
            "runtime package discovery reviewed-eviction replacement requires a relative selected .geindex path");
        return result;
    }

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::invalid_mount_id;
        add_discovery_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_replace,
            desc.mount_id, "invalid-mount-id", "resident package mount ids must be non-zero");
        return result;
    }
    if (!has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::missing_mount_id;
        add_discovery_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::resident_replace,
            desc.mount_id, "missing-mount-id", "resident package mount id is not mounted");
        return result;
    }

    result.invoked_discovery = true;
    result.discovery = discover_runtime_package_indexes_v2(filesystem, desc.discovery);
    if (!result.discovery.succeeded()) {
        const auto descriptor_failure =
            result.discovery.status == RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor;
        result.status = descriptor_failure
                            ? RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::invalid_descriptor
                            : RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::discovery_failed;
        add_discovery_diagnostics(
            result, result.discovery,
            descriptor_failure ? RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::descriptor
                               : RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::discovery);
        return result;
    }

    const auto selected =
        std::ranges::find_if(result.discovery.candidates, [&desc](const RuntimePackageIndexDiscoveryCandidateV2& item) {
            return item.package_index_path == desc.selected_package_index_path;
        });
    if (selected == result.discovery.candidates.end()) {
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::candidate_not_found;
        add_discovery_resident_replace_reviewed_evictions_diagnostic(
            result, RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::candidate_selection,
            desc.mount_id, desc.selected_package_index_path, "candidate-not-found",
            "selected runtime package index was not discovered under the reviewed root");
        return result;
    }
    result.selected_candidate = *selected;

    result.invoked_resident_commit = true;
    result.candidate_resident_replace = commit_runtime_package_candidate_resident_replace_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        RuntimePackageCandidateResidentReplaceReviewedEvictionsDescV2{
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
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::replaced:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::committed;
        result.committed = true;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::invalid_mount_id:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::invalid_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::missing_mount_id:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::missing_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::candidate_load_failed:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::candidate_load_failed;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::resident_replace_failed:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::resident_replace_failed;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::invalid_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::invalid_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::duplicate_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::duplicate_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::missing_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::missing_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::protected_eviction_candidate_mount_id:
        result.status =
            RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::protected_eviction_candidate_mount_id;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::budget_failed:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::budget_failed;
        break;
    case RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::catalog_refresh_failed:
        result.status = RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::catalog_refresh_failed;
        break;
    }
    return result;
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

bool RuntimeResidentPackageEvictionPlanResultV2::succeeded() const noexcept {
    return status == RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required ||
           status == RuntimeResidentPackageEvictionPlanStatusV2::planned;
}

RuntimeResidentPackageEvictionPlanResultV2
plan_runtime_resident_package_evictions_v2(const RuntimeResidentPackageMountSetV2& mount_set,
                                           const RuntimeResidentPackageEvictionPlanDescV2& desc) {
    RuntimeResidentPackageEvictionPlanResultV2 result;
    result.mount_generation = mount_set.generation();
    result.previous_mount_count = mount_set.mounts().size();
    result.projected_mount_count = result.previous_mount_count;

    RuntimeResidentCatalogCacheV2 current_cache;
    result.current_refresh = current_cache.refresh(mount_set, desc.overlay, desc.target_budget);
    result.projected_refresh = result.current_refresh;
    if (result.current_refresh.succeeded()) {
        result.status = RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required;
        return result;
    }
    if (result.current_refresh.status == RuntimeResidentCatalogCacheStatusV2::catalog_build_failed) {
        result.status = RuntimeResidentPackageEvictionPlanStatusV2::catalog_build_failed;
        return result;
    }

    std::vector<RuntimeResidentPackageMountIdV2> seen_candidates;
    seen_candidates.reserve(desc.candidate_unmount_order.size());
    for (const auto candidate : desc.candidate_unmount_order) {
        if (candidate.value == 0) {
            result.status = RuntimeResidentPackageEvictionPlanStatusV2::invalid_candidate_mount_id;
            add_eviction_diagnostic(result, candidate, "invalid-candidate-mount-id",
                                    "resident eviction candidate mount ids must be non-zero");
            return result;
        }
        if (contains_mount_id(seen_candidates, candidate)) {
            result.status = RuntimeResidentPackageEvictionPlanStatusV2::duplicate_candidate_mount_id;
            add_eviction_diagnostic(result, candidate, "duplicate-candidate-mount-id",
                                    "resident eviction candidate mount id appears more than once");
            return result;
        }
        if (contains_mount_id(desc.protected_mount_ids, candidate)) {
            result.status = RuntimeResidentPackageEvictionPlanStatusV2::protected_candidate_mount_id;
            add_eviction_diagnostic(result, candidate, "protected-candidate-mount-id",
                                    "resident eviction candidate mount id is protected");
            return result;
        }
        if (!has_mount_id(mount_set, candidate)) {
            result.status = RuntimeResidentPackageEvictionPlanStatusV2::missing_candidate_mount_id;
            add_eviction_diagnostic(result, candidate, "missing-candidate-mount-id",
                                    "resident eviction candidate mount id is not mounted");
            return result;
        }
        seen_candidates.push_back(candidate);
    }

    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    RuntimeResidentCatalogCacheV2 projected_cache;
    for (const auto candidate : desc.candidate_unmount_order) {
        RuntimeResidentPackageEvictionPlanStepV2 step;
        step.mount_id = candidate;
        step.unmount = projected_mount_set.unmount(candidate);
        step.catalog_refresh = projected_cache.refresh(projected_mount_set, desc.overlay, desc.target_budget);
        result.projected_refresh = step.catalog_refresh;
        result.projected_mount_count = projected_mount_set.mounts().size();
        result.steps.push_back(std::move(step));

        if (result.projected_refresh.succeeded()) {
            result.status = RuntimeResidentPackageEvictionPlanStatusV2::planned;
            return result;
        }
        if (result.projected_refresh.status == RuntimeResidentCatalogCacheStatusV2::catalog_build_failed) {
            result.status = RuntimeResidentPackageEvictionPlanStatusV2::catalog_build_failed;
            return result;
        }
    }

    result.status = RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable;
    return result;
}

bool RuntimeResidentPackageReviewedEvictionCommitResultV2::succeeded() const noexcept {
    return (status == RuntimeResidentPackageReviewedEvictionCommitStatusV2::committed ||
            status == RuntimeResidentPackageReviewedEvictionCommitStatusV2::no_eviction_required) &&
           committed;
}

RuntimeResidentPackageReviewedEvictionCommitResultV2
commit_runtime_resident_package_reviewed_evictions_v2(RuntimeResidentPackageMountSetV2& mount_set,
                                                      RuntimeResidentCatalogCacheV2& catalog_cache,
                                                      const RuntimeResidentPackageReviewedEvictionCommitDescV2& desc) {
    RuntimeResidentPackageReviewedEvictionCommitResultV2 result;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    result.invoked_eviction_plan = true;
    result.eviction_plan = plan_runtime_resident_package_evictions_v2(
        mount_set, RuntimeResidentPackageEvictionPlanDescV2{
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
        if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable) {
            add_reviewed_eviction_commit_budget_diagnostics(result, result.catalog_refresh.budget_execution);
        } else if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatusV2::catalog_build_failed) {
            add_reviewed_eviction_commit_catalog_diagnostics(result, result.catalog_refresh.catalog_build);
        }
        return result;
    }

    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    for (const auto& step : result.eviction_plan.steps) {
        const auto unmount = projected_mount_set.unmount(step.mount_id);
        if (!unmount.succeeded()) {
            result.status = RuntimeResidentPackageReviewedEvictionCommitStatusV2::catalog_refresh_failed;
            add_reviewed_eviction_commit_diagnostic(
                result, RuntimeResidentPackageReviewedEvictionCommitDiagnosticPhaseV2::eviction_plan, {}, step.mount_id,
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
        if (result.catalog_refresh.status == RuntimeResidentCatalogCacheStatusV2::budget_failed) {
            result.status = RuntimeResidentPackageReviewedEvictionCommitStatusV2::budget_failed;
            add_reviewed_eviction_commit_budget_diagnostics(result, result.catalog_refresh.budget_execution);
        } else {
            result.status = RuntimeResidentPackageReviewedEvictionCommitStatusV2::catalog_refresh_failed;
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

bool RuntimePackageCandidateResidentMountReviewedEvictionsResultV2::succeeded() const noexcept {
    return status == RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::mounted && committed;
}

RuntimePackageCandidateResidentMountReviewedEvictionsResultV2
commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
    IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentCatalogCacheV2& catalog_cache,
    const RuntimePackageCandidateResidentMountReviewedEvictionsDescV2& desc) {
    RuntimePackageCandidateResidentMountReviewedEvictionsResultV2 result;
    result.candidate = desc.candidate;
    result.previous_mount_generation = mount_set.generation();
    result.mount_generation = result.previous_mount_generation;
    result.previous_mount_count = mount_set.mounts().size();
    result.mounted_package_count = result.previous_mount_count;

    if (desc.mount_id.value == 0) {
        result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::invalid_mount_id;
        result.resident_mount = mount_result(desc.mount_id, RuntimeResidentPackageMountStatusV2::invalid_mount_id,
                                             "invalid-mount-id", "resident package mount ids must be non-zero");
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_mount,
            desc.mount_id, result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }
    if (has_mount_id(mount_set, desc.mount_id)) {
        result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::duplicate_mount_id;
        result.resident_mount = mount_result(desc.mount_id, RuntimeResidentPackageMountStatusV2::duplicate_mount_id,
                                             "duplicate-mount-id", "resident package mount id is already mounted");
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_mount,
            desc.mount_id, result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }

    result.invoked_candidate_load = true;
    result.candidate_load = load_runtime_package_candidate_v2(filesystem, desc.candidate);
    result.package_desc = result.candidate_load.package_desc;
    result.loaded_record_count = result.candidate_load.loaded_record_count;
    result.loaded_resident_bytes = result.candidate_load.estimated_resident_bytes;
    if (!result.candidate_load.succeeded()) {
        result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::candidate_load_failed;
        add_reviewed_evictions_candidate_load_diagnostics(result, result.candidate_load, desc.mount_id);
        return result;
    }

    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    result.resident_mount = projected_mount_set.mount(RuntimeResidentPackageMountRecordV2{
        .id = desc.mount_id,
        .label = desc.candidate.label,
        .package = result.candidate_load.loaded_package.package,
    });
    if (!result.resident_mount.succeeded()) {
        result.status = result.resident_mount.status == RuntimeResidentPackageMountStatusV2::duplicate_mount_id
                            ? RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::duplicate_mount_id
                            : RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::invalid_mount_id;
        add_candidate_resident_mount_reviewed_evictions_diagnostic(
            result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::resident_mount,
            desc.mount_id, result.resident_mount.diagnostic.code, result.resident_mount.diagnostic.message);
        return result;
    }

    std::vector<RuntimeResidentPackageMountIdV2> protected_mounts = desc.protected_mount_ids;
    if (!contains_mount_id(protected_mounts, desc.mount_id)) {
        protected_mounts.push_back(desc.mount_id);
    }

    result.invoked_eviction_plan = true;
    result.eviction_plan = plan_runtime_resident_package_evictions_v2(
        projected_mount_set, RuntimeResidentPackageEvictionPlanDescV2{
                                 .target_budget = desc.budget,
                                 .overlay = desc.overlay,
                                 .candidate_unmount_order = desc.eviction_candidate_unmount_order,
                                 .protected_mount_ids = std::move(protected_mounts),
                             });
    if (!result.eviction_plan.succeeded()) {
        result.status = map_eviction_plan_status(result.eviction_plan.status);
        add_reviewed_evictions_eviction_plan_diagnostics(result, result.eviction_plan);
        if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable) {
            add_reviewed_evictions_budget_diagnostics(result, result.eviction_plan.projected_refresh.budget_execution,
                                                      desc.mount_id);
        } else if (result.eviction_plan.status == RuntimeResidentPackageEvictionPlanStatusV2::catalog_build_failed) {
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
            result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::catalog_refresh_failed;
            add_candidate_resident_mount_reviewed_evictions_diagnostic(
                result, RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::eviction_plan,
                step.mount_id, unmount.diagnostic.code, unmount.diagnostic.message);
            return result;
        }
    }
    result.evicted_mount_count = result.eviction_plan.steps.size();

    RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    result.invoked_catalog_refresh = true;
    result.catalog_refresh = projected_catalog_cache.refresh(projected_mount_set, desc.overlay, desc.budget);
    result.projected_resident_bytes = result.catalog_refresh.budget_execution.estimated_resident_content_bytes;
    if (!result.catalog_refresh.succeeded()) {
        if (result.catalog_refresh.status == RuntimeResidentCatalogCacheStatusV2::budget_failed) {
            result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::budget_failed;
            add_reviewed_evictions_budget_diagnostics(result, result.catalog_refresh.budget_execution, desc.mount_id);
        } else {
            result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::catalog_refresh_failed;
            add_reviewed_evictions_catalog_diagnostics(result, result.catalog_refresh.catalog_build, desc.mount_id);
        }
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);

    result.status = RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::mounted;
    result.mount_generation = mount_set.generation();
    result.mounted_package_count = mount_set.mounts().size();
    result.committed = true;
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
