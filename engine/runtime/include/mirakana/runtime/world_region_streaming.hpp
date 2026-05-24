// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeWorldRegionStreamingActionKind : std::uint8_t {
    keep_resident = 0,
    load_region,
    unload_region,
};

enum class RuntimeWorldRegionStreamingPlanStatus : std::uint8_t {
    planned = 0,
    no_changes,
    invalid_request,
    budget_exceeded,
};

enum class RuntimeWorldRegionStreamingDiagnosticCode : std::uint8_t {
    invalid_region_id = 0,
    duplicate_region,
    invalid_package_index_path,
    duplicate_active_region,
    missing_active_region,
    missing_desired_region,
    duplicate_desired_region,
    missing_protected_region,
    duplicate_protected_region,
    protected_active_region_unload_requested,
    invalid_mount_id,
    duplicate_mount_id,
    resident_region_count_exceeded,
    resident_content_budget_exceeded,
    resident_asset_record_budget_exceeded,
    missing_package_candidate,
    package_streaming_execution_failed,
};

struct RuntimeWorldRegionPackageDesc {
    std::string region_id;
    RuntimePackageIndexDiscoveryCandidateV2 candidate;
    RuntimeResidentPackageMountIdV2 mount_id;
    std::uint64_t estimated_resident_bytes{0};
    std::size_t estimated_asset_records{0};
    std::vector<AssetId> required_preload_assets;
    std::vector<AssetKind> resident_resource_kinds;
};

struct RuntimeWorldRegionStreamingPlanRequest {
    std::vector<RuntimeWorldRegionPackageDesc> regions;
    std::vector<std::string> active_region_ids;
    std::vector<std::string> desired_region_ids;
    std::vector<std::string> protected_region_ids;
    RuntimeResourceResidencyBudgetV2 budget;
    std::size_t max_resident_regions{0};
};

struct RuntimeWorldRegionStreamingDiagnostic {
    RuntimeWorldRegionStreamingDiagnosticCode code{RuntimeWorldRegionStreamingDiagnosticCode::invalid_region_id};
    std::string region_id;
    std::string message;
};

struct RuntimeWorldRegionStreamingPlanRow {
    RuntimeWorldRegionStreamingActionKind action{RuntimeWorldRegionStreamingActionKind::keep_resident};
    std::string region_id;
    RuntimeResidentPackageMountIdV2 mount_id;
    std::string package_index_path;
    std::string content_root;
    std::uint64_t estimated_resident_bytes{0};
    std::size_t estimated_asset_records{0};
    std::size_t required_preload_asset_count{0};
    std::size_t resident_resource_kind_count{0};
    bool protected_region{false};
};

struct RuntimeWorldRegionStreamingPlan {
    RuntimeWorldRegionStreamingPlanStatus status{RuntimeWorldRegionStreamingPlanStatus::invalid_request};
    std::vector<RuntimeWorldRegionStreamingDiagnostic> diagnostics;
    std::vector<RuntimeWorldRegionStreamingPlanRow> rows;
    std::size_t projected_resident_region_count{0};
    std::uint64_t projected_resident_bytes{0};
    std::size_t projected_resident_asset_records{0};
    std::size_t load_count{0};
    std::size_t keep_count{0};
    std::size_t unload_count{0};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans deterministic world-region load/unload intent rows over caller-reviewed package candidates.
/// This value-only helper does not read files, load packages, mutate resident mounts, refresh catalogs, stream in the
/// background, own renderer/RHI resources, or touch native handles.
[[nodiscard]] RuntimeWorldRegionStreamingPlan
plan_runtime_world_region_streaming(const RuntimeWorldRegionStreamingPlanRequest& request);

enum class RuntimeWorldRegionNavigationReviewStatus : std::uint8_t {
    ready = 0,
    invalid_request,
    not_resident,
    stale,
};

enum class RuntimeWorldRegionNavigationDiagnosticCode : std::uint8_t {
    invalid_region_id = 0,
    duplicate_region,
    duplicate_route_region,
    missing_region_package,
    invalid_package_index_path,
    invalid_mount_id,
    duplicate_mount_id,
    duplicate_region_ref,
    empty_route,
    route_portal_count_mismatch,
    route_region_not_resident,
    path_cache_region_path_mismatch,
    path_cache_portal_path_mismatch,
    mount_generation_mismatch,
    catalog_generation_mismatch,
    catalog_cache_not_ready,
};

struct RuntimeWorldRegionNavigationDiagnostic {
    RuntimeWorldRegionNavigationDiagnosticCode code{RuntimeWorldRegionNavigationDiagnosticCode::invalid_region_id};
    std::string region_id;
    std::string message;
};

struct RuntimeWorldRegionNavigationRefReviewRequest {
    std::vector<RuntimeWorldRegionPackageDesc> regions;
    std::vector<std::string> route_region_ids;
};

struct RuntimeWorldRegionNavigationRefRow {
    std::string region_id;
    RuntimeResidentPackageMountIdV2 mount_id;
    std::string package_index_path;
    std::string content_root;
    bool resident{false};
};

struct RuntimeWorldRegionNavigationRefReviewResult {
    RuntimeWorldRegionNavigationReviewStatus status{RuntimeWorldRegionNavigationReviewStatus::invalid_request};
    std::vector<RuntimeWorldRegionNavigationDiagnostic> diagnostics;
    std::vector<RuntimeWorldRegionNavigationRefRow> rows;
    std::size_t resident_region_count{0};
    std::size_t missing_resident_region_count{0};
    std::uint32_t current_mount_generation{0};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews a hierarchical navigation route against caller-reviewed world-region package references and current resident
/// mounts. This value-only helper does not read package files, execute streaming safe points, mutate resident state,
/// refresh catalogs, bake navigation data, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimeWorldRegionNavigationRefReviewResult
review_runtime_world_region_navigation_refs(const RuntimeResidentPackageMountSetV2& mount_set,
                                            const RuntimeWorldRegionNavigationRefReviewRequest& request);

struct RuntimeWorldRegionNavigationPathCacheEntry {
    std::vector<std::string> region_path;
    std::vector<std::string> portal_path;
    std::uint32_t mount_generation{0};
    std::uint32_t catalog_generation{0};
};

struct RuntimeWorldRegionNavigationPathCacheReviewRequest {
    std::vector<RuntimeWorldRegionPackageDesc> regions;
    std::vector<std::string> route_region_ids;
    std::vector<std::string> route_portal_ids;
    RuntimeWorldRegionNavigationPathCacheEntry cache;
};

struct RuntimeWorldRegionNavigationPathCacheReviewResult {
    RuntimeWorldRegionNavigationReviewStatus status{RuntimeWorldRegionNavigationReviewStatus::invalid_request};
    std::vector<RuntimeWorldRegionNavigationDiagnostic> diagnostics;
    std::vector<RuntimeWorldRegionNavigationRefRow> rows;
    std::size_t resident_region_count{0};
    std::size_t missing_resident_region_count{0};
    std::uint32_t current_mount_generation{0};
    std::uint32_t current_catalog_generation{0};
    bool cache_ready{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews whether a retained hierarchical navigation path-cache row still matches the current route and resident
/// package/catalog generations. The coarse generation key is conservative: it may invalidate too often, but it never
/// reuses stale package-backed navigation rows as ready.
[[nodiscard]] RuntimeWorldRegionNavigationPathCacheReviewResult
review_runtime_world_region_navigation_path_cache(const RuntimeResidentPackageMountSetV2& mount_set,
                                                  const RuntimeResidentCatalogCacheV2& catalog_cache,
                                                  const RuntimeWorldRegionNavigationPathCacheReviewRequest& request);

enum class RuntimeWorldRegionStreamingSafePointStatus : std::uint8_t {
    invalid_plan = 0,
    no_changes,
    failed,
    completed,
};

struct RuntimeWorldRegionStreamingSafePointDesc {
    RuntimeWorldRegionStreamingPlan plan;
    std::vector<RuntimeWorldRegionPackageDesc> regions;
    std::string target_id;
    std::string runtime_scene_validation_target_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudgetV2 budget;
    std::uint32_t max_resident_packages{0};
    bool safe_point_required{true};
    bool runtime_scene_validation_succeeded{false};
    std::vector<RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids;
};

struct RuntimeWorldRegionStreamingSafePointRowResult {
    RuntimeWorldRegionStreamingActionKind action{RuntimeWorldRegionStreamingActionKind::keep_resident};
    std::string region_id;
    RuntimeResidentPackageMountIdV2 mount_id;
    RuntimePackageStreamingExecutionResult streaming;
    bool committed{false};
};

struct RuntimeWorldRegionStreamingSafePointResult {
    RuntimeWorldRegionStreamingSafePointStatus status{RuntimeWorldRegionStreamingSafePointStatus::invalid_plan};
    std::vector<RuntimeWorldRegionStreamingDiagnostic> diagnostics;
    std::vector<RuntimeWorldRegionStreamingSafePointRowResult> rows;
    std::size_t load_count{0};
    std::size_t keep_count{0};
    std::size_t unload_count{0};
    std::size_t committed_count{0};
    bool committed{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Executes reviewed world-region load/unload plan rows through the existing package streaming safe-point primitives.
/// The live mount set and catalog cache are updated only after every row succeeds on a projected view. The helper does
/// not discover packages, infer eviction policy, stream in the background, upload renderer resources, or touch native
/// handles.
[[nodiscard]] RuntimeWorldRegionStreamingSafePointResult
execute_runtime_world_region_streaming_safe_point(IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set,
                                                  RuntimeResidentCatalogCacheV2& catalog_cache,
                                                  const RuntimeWorldRegionStreamingSafePointDesc& desc);

enum class RuntimeWorldStreamingLargeSceneReadinessStatus : std::uint8_t {
    ready = 0,
    diagnostics,
    invalid_evidence,
};

enum class RuntimeWorldStreamingLargeSceneReadinessDiagnostic : std::uint8_t {
    none = 0,
    invalid_streaming_plan,
    streaming_safe_point_failed,
    insufficient_plan_rows,
    insufficient_load_rows,
    insufficient_keep_rows,
    insufficient_unload_rows,
    insufficient_safe_point_rows,
    insufficient_committed_rows,
    missing_reviewed_package_adoption,
    missing_region_diagnostic_absent,
    safe_point_diagnostics_present,
    projected_region_budget_exceeded,
    projected_byte_budget_exceeded,
    navigation_refs_not_ready,
    navigation_path_cache_not_ready,
};

struct RuntimeWorldStreamingLargeSceneReadinessConfig {
    bool require_missing_region_diagnostic{false};
    bool require_navigation_refs_ready{false};
    bool require_navigation_path_cache_ready{false};
    std::size_t min_plan_rows{1U};
    std::size_t min_load_rows{1U};
    std::size_t min_keep_rows{0U};
    std::size_t min_unload_rows{0U};
    std::size_t min_safe_point_rows{1U};
    std::size_t min_committed_rows{1U};
    std::size_t min_reviewed_package_adoptions{1U};
    std::size_t max_safe_point_diagnostics{0U};
    std::size_t max_projected_resident_regions{std::numeric_limits<std::size_t>::max()};
    std::uint64_t max_projected_resident_bytes{std::numeric_limits<std::uint64_t>::max()};
};

struct RuntimeWorldStreamingLargeSceneReadinessRequest {
    std::span<const RuntimeWorldRegionStreamingPlan> streaming_plans;
    std::span<const RuntimeWorldRegionStreamingSafePointResult> safe_points;
    const RuntimeWorldRegionStreamingPlan* missing_region_probe{nullptr};
    const RuntimeWorldRegionNavigationRefReviewResult* navigation_refs{nullptr};
    const RuntimeWorldRegionNavigationPathCacheReviewResult* navigation_path_cache{nullptr};
};

struct RuntimeWorldStreamingLargeSceneReadinessReport {
    RuntimeWorldStreamingLargeSceneReadinessStatus status{
        RuntimeWorldStreamingLargeSceneReadinessStatus::invalid_evidence};
    RuntimeWorldStreamingLargeSceneReadinessDiagnostic diagnostic{
        RuntimeWorldStreamingLargeSceneReadinessDiagnostic::none};
    std::vector<RuntimeWorldStreamingLargeSceneReadinessDiagnostic> diagnostics;
    RuntimeWorldRegionStreamingPlanStatus first_plan_status{RuntimeWorldRegionStreamingPlanStatus::invalid_request};
    RuntimeWorldRegionStreamingSafePointStatus first_safe_point_status{
        RuntimeWorldRegionStreamingSafePointStatus::invalid_plan};
    RuntimeWorldRegionNavigationReviewStatus navigation_refs_status{
        RuntimeWorldRegionNavigationReviewStatus::invalid_request};
    RuntimeWorldRegionNavigationReviewStatus navigation_path_cache_status{
        RuntimeWorldRegionNavigationReviewStatus::invalid_request};
    std::size_t plan_rows{0U};
    std::size_t load_rows{0U};
    std::size_t keep_rows{0U};
    std::size_t unload_rows{0U};
    std::size_t safe_point_rows{0U};
    std::size_t committed_rows{0U};
    std::size_t reviewed_package_adoptions{0U};
    std::size_t projected_resident_regions{0U};
    std::uint64_t projected_resident_bytes{0U};
    std::size_t max_projected_resident_regions{0U};
    std::uint64_t max_projected_resident_bytes{0U};
    std::size_t missing_region_diagnostics{0U};
    std::size_t safe_point_diagnostics{0U};
    std::size_t navigation_resident_regions{0U};
    std::size_t navigation_missing_resident_regions{0U};
    bool navigation_path_cache_ready{false};
};

/// Summarizes reviewed world-region streaming, package safe-point, missing-region, and optional navigation-cache
/// evidence for a large-scene foundation claim. This helper only consumes caller-supplied value evidence; it does not
/// read packages, execute streaming, refresh catalogs, start background jobs, or touch renderer/RHI/native handles.
[[nodiscard]] RuntimeWorldStreamingLargeSceneReadinessReport evaluate_runtime_world_streaming_large_scene_readiness(
    const RuntimeWorldStreamingLargeSceneReadinessRequest& request,
    const RuntimeWorldStreamingLargeSceneReadinessConfig& config = {});

} // namespace mirakana::runtime
