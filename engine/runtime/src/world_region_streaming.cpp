// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/world_region_streaming.hpp"

#include <algorithm>
#include <limits>
#include <string_view>
#include <utility>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool is_valid_region_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) {
    return std::ranges::any_of(ids, [id](const std::string& candidate) { return candidate == id; });
}

[[nodiscard]] bool contains_mount_id(const std::vector<RuntimeResidentPackageMountIdV2>& ids,
                                     RuntimeResidentPackageMountIdV2 id) {
    return std::ranges::any_of(ids, [id](RuntimeResidentPackageMountIdV2 candidate) { return candidate == id; });
}

[[nodiscard]] const RuntimeWorldRegionPackageDesc*
find_region(const std::vector<RuntimeWorldRegionPackageDesc>& regions, std::string_view id) {
    const auto found = std::ranges::find_if(
        regions, [id](const RuntimeWorldRegionPackageDesc& region) { return region.region_id == id; });
    return found == regions.end() ? nullptr : &(*found);
}

[[nodiscard]] bool has_mount_id(const RuntimeResidentPackageMountSetV2& mount_set, RuntimeResidentPackageMountIdV2 id) {
    return std::ranges::any_of(mount_set.mounts(),
                               [id](const RuntimeResidentPackageMountRecordV2& mount) { return mount.id == id; });
}

void add_diagnostic(RuntimeWorldRegionStreamingPlan& plan, RuntimeWorldRegionStreamingDiagnosticCode code,
                    std::string region_id, std::string message) {
    plan.diagnostics.push_back(RuntimeWorldRegionStreamingDiagnostic{
        .code = code,
        .region_id = std::move(region_id),
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimeWorldRegionStreamingSafePointResult& result, RuntimeWorldRegionStreamingDiagnosticCode code,
                    std::string region_id, std::string message) {
    result.diagnostics.push_back(RuntimeWorldRegionStreamingDiagnostic{
        .code = code,
        .region_id = std::move(region_id),
        .message = std::move(message),
    });
}

template <typename Result>
void add_navigation_diagnostic(Result& result, RuntimeWorldRegionNavigationDiagnosticCode code, std::string region_id,
                               std::string message) {
    result.diagnostics.push_back(RuntimeWorldRegionNavigationDiagnostic{
        .code = code,
        .region_id = std::move(region_id),
        .message = std::move(message),
    });
}

void validate_region_catalog(RuntimeWorldRegionStreamingPlan& plan,
                             const std::vector<RuntimeWorldRegionPackageDesc>& regions) {
    std::vector<std::string> seen;
    std::vector<RuntimeResidentPackageMountIdV2> seen_mount_ids;
    for (const auto& region : regions) {
        if (!is_valid_region_id(region.region_id)) {
            add_diagnostic(plan, RuntimeWorldRegionStreamingDiagnosticCode::invalid_region_id, region.region_id,
                           "world region id must be non-empty and path-safe");
        }
        if (contains_id(seen, region.region_id)) {
            add_diagnostic(plan, RuntimeWorldRegionStreamingDiagnosticCode::duplicate_region, region.region_id,
                           "world region catalog contains a duplicate region id");
        } else {
            seen.push_back(region.region_id);
        }
        if (!region.candidate.package_index_path.ends_with(".geindex")) {
            add_diagnostic(plan, RuntimeWorldRegionStreamingDiagnosticCode::invalid_package_index_path,
                           region.region_id, "world region package candidate must reference a .geindex package");
        }
        if (region.mount_id.value == 0U) {
            add_diagnostic(plan, RuntimeWorldRegionStreamingDiagnosticCode::invalid_mount_id, region.region_id,
                           "world region resident mount ids must be non-zero");
        } else if (contains_mount_id(seen_mount_ids, region.mount_id)) {
            add_diagnostic(plan, RuntimeWorldRegionStreamingDiagnosticCode::duplicate_mount_id, region.region_id,
                           "world region catalog contains a duplicate resident mount id");
        } else {
            seen_mount_ids.push_back(region.mount_id);
        }
    }
}

void validate_navigation_region_catalog(RuntimeWorldRegionNavigationRefReviewResult& result,
                                        const std::vector<RuntimeWorldRegionPackageDesc>& regions) {
    std::vector<std::string> seen_regions;
    std::vector<std::string> seen_package_refs;
    std::vector<RuntimeResidentPackageMountIdV2> seen_mount_ids;
    for (const auto& region : regions) {
        if (!is_valid_region_id(region.region_id)) {
            add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::invalid_region_id,
                                      region.region_id, "world region navigation catalog id must be path-safe");
        }
        if (contains_id(seen_regions, region.region_id)) {
            add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::duplicate_region,
                                      region.region_id, "world region navigation catalog contains a duplicate id");
        } else {
            seen_regions.push_back(region.region_id);
        }
        if (!region.candidate.package_index_path.ends_with(".geindex")) {
            add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::invalid_package_index_path,
                                      region.region_id, "world region navigation catalog must reference .geindex rows");
        }
        if (contains_id(seen_package_refs, region.candidate.package_index_path)) {
            add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::duplicate_region_ref,
                                      region.region_id,
                                      "world region navigation catalog contains a duplicate package index ref");
        } else {
            seen_package_refs.push_back(region.candidate.package_index_path);
        }
        if (region.mount_id.value == 0U) {
            add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::invalid_mount_id,
                                      region.region_id, "world region navigation mount id must be non-zero");
        } else if (contains_mount_id(seen_mount_ids, region.mount_id)) {
            add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::duplicate_mount_id,
                                      region.region_id,
                                      "world region navigation catalog contains a duplicate resident mount id");
        } else {
            seen_mount_ids.push_back(region.mount_id);
        }
    }
}

void validate_id_list(RuntimeWorldRegionStreamingPlan& plan, const std::vector<std::string>& ids,
                      const std::vector<RuntimeWorldRegionPackageDesc>& regions,
                      RuntimeWorldRegionStreamingDiagnosticCode duplicate_code,
                      RuntimeWorldRegionStreamingDiagnosticCode missing_code) {
    std::vector<std::string> seen;
    for (const auto& id : ids) {
        if (!is_valid_region_id(id)) {
            add_diagnostic(plan, RuntimeWorldRegionStreamingDiagnosticCode::invalid_region_id, id,
                           "world region request id must be non-empty and path-safe");
        }
        if (find_region(regions, id) == nullptr) {
            add_diagnostic(plan, missing_code, id, "world region request references a missing catalog region");
        }
        if (contains_id(seen, id)) {
            add_diagnostic(plan, duplicate_code, id, "world region request contains a duplicate region id");
        } else {
            seen.push_back(id);
        }
    }
}

[[nodiscard]] bool is_protected(const RuntimeWorldRegionStreamingPlanRequest& request, std::string_view id) {
    return contains_id(request.protected_region_ids, id);
}

void validate_navigation_route(RuntimeWorldRegionNavigationRefReviewResult& result,
                               const RuntimeWorldRegionNavigationRefReviewRequest& request) {
    if (request.route_region_ids.empty()) {
        add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::empty_route, {},
                                  "world region navigation route must contain at least one region");
        return;
    }

    std::vector<std::string> seen_route_regions;
    for (const auto& id : request.route_region_ids) {
        if (!is_valid_region_id(id)) {
            add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::invalid_region_id, id,
                                      "world region navigation route id must be path-safe");
        }
        if (find_region(request.regions, id) == nullptr) {
            add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::missing_region_package, id,
                                      "world region navigation route references a missing package region");
        }
        if (contains_id(seen_route_regions, id)) {
            add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::duplicate_route_region, id,
                                      "world region navigation route contains a duplicate region id");
        } else {
            seen_route_regions.push_back(id);
        }
    }
}

void append_navigation_ref_rows(RuntimeWorldRegionNavigationRefReviewResult& result,
                                const RuntimeResidentPackageMountSetV2& mount_set,
                                const RuntimeWorldRegionNavigationRefReviewRequest& request) {
    for (const auto& id : request.route_region_ids) {
        const auto* region = find_region(request.regions, id);
        if (region == nullptr) {
            continue;
        }
        const auto resident = has_mount_id(mount_set, region->mount_id);
        result.rows.push_back(RuntimeWorldRegionNavigationRefRow{
            .region_id = region->region_id,
            .mount_id = region->mount_id,
            .package_index_path = region->candidate.package_index_path,
            .content_root = region->candidate.content_root,
            .resident = resident,
        });
        if (resident) {
            ++result.resident_region_count;
        } else {
            ++result.missing_resident_region_count;
            add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::route_region_not_resident,
                                      region->region_id,
                                      "world region navigation route references a package that is not resident");
        }
    }
}

void copy_navigation_ref_review(RuntimeWorldRegionNavigationPathCacheReviewResult& out,
                                const RuntimeWorldRegionNavigationRefReviewResult& review) {
    out.status = review.status;
    out.diagnostics = review.diagnostics;
    out.rows = review.rows;
    out.resident_region_count = review.resident_region_count;
    out.missing_resident_region_count = review.missing_resident_region_count;
    out.current_mount_generation = review.current_mount_generation;
}

void validate_navigation_path_cache_route_shape(RuntimeWorldRegionNavigationPathCacheReviewResult& result,
                                                const RuntimeWorldRegionNavigationPathCacheReviewRequest& request) {
    const auto expected_portal_count =
        request.route_region_ids.empty() ? 0U : static_cast<std::size_t>(request.route_region_ids.size() - 1U);
    if (request.route_portal_ids.size() != expected_portal_count) {
        add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::route_portal_count_mismatch, {},
                                  "world region navigation path cache route portals must match region transitions");
    }
}

void append_action_row(RuntimeWorldRegionStreamingPlan& plan, RuntimeWorldRegionStreamingActionKind action,
                       const RuntimeWorldRegionPackageDesc& region, bool protected_region) {
    plan.rows.push_back(RuntimeWorldRegionStreamingPlanRow{
        .action = action,
        .region_id = region.region_id,
        .mount_id = region.mount_id,
        .package_index_path = region.candidate.package_index_path,
        .content_root = region.candidate.content_root,
        .estimated_resident_bytes = region.estimated_resident_bytes,
        .estimated_asset_records = region.estimated_asset_records,
        .required_preload_asset_count = region.required_preload_assets.size(),
        .resident_resource_kind_count = region.resident_resource_kinds.size(),
        .protected_region = protected_region,
    });
}

void sort_rows(RuntimeWorldRegionStreamingPlan& plan) {
    std::ranges::sort(plan.rows,
                      [](const RuntimeWorldRegionStreamingPlanRow& lhs, const RuntimeWorldRegionStreamingPlanRow& rhs) {
                          if (lhs.region_id != rhs.region_id) {
                              return lhs.region_id < rhs.region_id;
                          }
                          return static_cast<std::uint8_t>(lhs.action) < static_cast<std::uint8_t>(rhs.action);
                      });
}

void count_rows(RuntimeWorldRegionStreamingPlan& plan) {
    for (const auto& row : plan.rows) {
        switch (row.action) {
        case RuntimeWorldRegionStreamingActionKind::load_region:
            ++plan.load_count;
            break;
        case RuntimeWorldRegionStreamingActionKind::keep_resident:
            ++plan.keep_count;
            break;
        case RuntimeWorldRegionStreamingActionKind::unload_region:
            ++plan.unload_count;
            break;
        }
    }
}

[[nodiscard]] std::uint64_t streaming_budget_bytes(const RuntimeWorldRegionStreamingSafePointDesc& desc) {
    if (desc.budget.max_resident_content_bytes.has_value()) {
        return *desc.budget.max_resident_content_bytes;
    }
    return desc.plan.projected_resident_bytes > 0U ? desc.plan.projected_resident_bytes
                                                   : std::numeric_limits<std::uint64_t>::max();
}

[[nodiscard]] RuntimePackageStreamingExecutionDesc
make_streaming_desc(const RuntimeWorldRegionStreamingSafePointDesc& desc, const RuntimeWorldRegionPackageDesc& region) {
    return RuntimePackageStreamingExecutionDesc{
        .target_id = desc.target_id,
        .package_index_path = region.candidate.package_index_path,
        .content_root = region.candidate.content_root,
        .runtime_scene_validation_target_id = desc.runtime_scene_validation_target_id,
        .mode = RuntimePackageStreamingExecutionMode::host_gated_safe_point,
        .resident_budget_bytes = streaming_budget_bytes(desc),
        .safe_point_required = desc.safe_point_required,
        .runtime_scene_validation_succeeded = desc.runtime_scene_validation_succeeded,
        .required_preload_assets = region.required_preload_assets,
        .resident_resource_kinds = region.resident_resource_kinds,
        .max_resident_packages = desc.max_resident_packages,
    };
}

void append_protected_plan_mount_ids(std::vector<RuntimeResidentPackageMountIdV2>& protected_mount_ids,
                                     const RuntimeWorldRegionStreamingPlan& plan) {
    for (const auto& row : plan.rows) {
        if (row.protected_region && row.mount_id.value != 0U && !contains_mount_id(protected_mount_ids, row.mount_id)) {
            protected_mount_ids.push_back(row.mount_id);
        }
    }
}

void append_package_failure_diagnostic(RuntimeWorldRegionStreamingSafePointResult& result,
                                       const RuntimeWorldRegionStreamingSafePointRowResult& row) {
    std::string message = "world region package streaming safe point failed";
    if (!row.streaming.diagnostics.empty()) {
        message += ": ";
        message += row.streaming.diagnostics.front().code;
        if (!row.streaming.diagnostics.front().message.empty()) {
            message += " - ";
            message += row.streaming.diagnostics.front().message;
        }
    }
    add_diagnostic(result, RuntimeWorldRegionStreamingDiagnosticCode::package_streaming_execution_failed, row.region_id,
                   std::move(message));
}

void append_large_scene_diagnostic(RuntimeWorldStreamingLargeSceneReadinessReport& report,
                                   RuntimeWorldStreamingLargeSceneReadinessDiagnostic diagnostic) {
    if (diagnostic == RuntimeWorldStreamingLargeSceneReadinessDiagnostic::none) {
        return;
    }
    if (report.diagnostics.empty()) {
        report.diagnostic = diagnostic;
    }
    report.diagnostics.push_back(diagnostic);
}

[[nodiscard]] std::size_t count_plan_diagnostics(const RuntimeWorldRegionStreamingPlan& plan,
                                                 RuntimeWorldRegionStreamingDiagnosticCode code) noexcept {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] bool failed_only_for_missing_regions(const RuntimeWorldRegionStreamingPlan& plan,
                                                   std::size_t missing_region_diagnostics) noexcept {
    return !plan.succeeded() && missing_region_diagnostics > 0U &&
           missing_region_diagnostics == plan.diagnostics.size();
}

void validate_safe_point_plan(RuntimeWorldRegionStreamingSafePointResult& result,
                              const RuntimeWorldRegionStreamingSafePointDesc& desc) {
    if (!desc.plan.succeeded()) {
        result.diagnostics.insert(result.diagnostics.end(), desc.plan.diagnostics.begin(), desc.plan.diagnostics.end());
        return;
    }

    for (const auto& row : desc.plan.rows) {
        const auto* region = find_region(desc.regions, row.region_id);
        if (region == nullptr) {
            add_diagnostic(result, RuntimeWorldRegionStreamingDiagnosticCode::missing_package_candidate, row.region_id,
                           "world region safe-point row references a missing package candidate");
            continue;
        }
        if (row.mount_id.value == 0U || region->mount_id.value == 0U || row.mount_id != region->mount_id) {
            add_diagnostic(result, RuntimeWorldRegionStreamingDiagnosticCode::invalid_mount_id, row.region_id,
                           "world region safe-point row mount id must match a non-zero package candidate mount id");
        }
    }
}

[[nodiscard]] bool row_execution_order_less(const RuntimeWorldRegionStreamingPlanRow* lhs,
                                            const RuntimeWorldRegionStreamingPlanRow* rhs) {
    const auto action_order = [](RuntimeWorldRegionStreamingActionKind action) {
        switch (action) {
        case RuntimeWorldRegionStreamingActionKind::unload_region:
            return 0U;
        case RuntimeWorldRegionStreamingActionKind::load_region:
            return 1U;
        case RuntimeWorldRegionStreamingActionKind::keep_resident:
            return 2U;
        }
        return 3U;
    };
    const auto lhs_order = action_order(lhs->action);
    const auto rhs_order = action_order(rhs->action);
    if (lhs_order != rhs_order) {
        return lhs_order < rhs_order;
    }
    return lhs->region_id < rhs->region_id;
}

[[nodiscard]] std::vector<const RuntimeWorldRegionStreamingPlanRow*>
make_execution_rows(const RuntimeWorldRegionStreamingPlan& plan) {
    std::vector<const RuntimeWorldRegionStreamingPlanRow*> rows;
    rows.reserve(plan.rows.size());
    for (const auto& row : plan.rows) {
        rows.push_back(&row);
    }
    std::ranges::sort(rows, row_execution_order_less);
    return rows;
}

} // namespace

bool RuntimeWorldRegionStreamingPlan::succeeded() const noexcept {
    return status == RuntimeWorldRegionStreamingPlanStatus::planned ||
           status == RuntimeWorldRegionStreamingPlanStatus::no_changes;
}

bool RuntimeWorldRegionStreamingSafePointResult::succeeded() const noexcept {
    return status == RuntimeWorldRegionStreamingSafePointStatus::completed ||
           status == RuntimeWorldRegionStreamingSafePointStatus::no_changes;
}

bool RuntimeWorldRegionNavigationRefReviewResult::succeeded() const noexcept {
    return status == RuntimeWorldRegionNavigationReviewStatus::ready;
}

bool RuntimeWorldRegionNavigationPathCacheReviewResult::succeeded() const noexcept {
    return status == RuntimeWorldRegionNavigationReviewStatus::ready;
}

RuntimeWorldRegionStreamingPlan
plan_runtime_world_region_streaming(const RuntimeWorldRegionStreamingPlanRequest& request) {
    RuntimeWorldRegionStreamingPlan plan;

    validate_region_catalog(plan, request.regions);
    validate_id_list(plan, request.active_region_ids, request.regions,
                     RuntimeWorldRegionStreamingDiagnosticCode::duplicate_active_region,
                     RuntimeWorldRegionStreamingDiagnosticCode::missing_active_region);
    validate_id_list(plan, request.desired_region_ids, request.regions,
                     RuntimeWorldRegionStreamingDiagnosticCode::duplicate_desired_region,
                     RuntimeWorldRegionStreamingDiagnosticCode::missing_desired_region);
    validate_id_list(plan, request.protected_region_ids, request.regions,
                     RuntimeWorldRegionStreamingDiagnosticCode::duplicate_protected_region,
                     RuntimeWorldRegionStreamingDiagnosticCode::missing_protected_region);

    if (!plan.diagnostics.empty()) {
        plan.status = RuntimeWorldRegionStreamingPlanStatus::invalid_request;
        return plan;
    }

    for (const auto& active_id : request.active_region_ids) {
        if (is_protected(request, active_id) && !contains_id(request.desired_region_ids, active_id)) {
            add_diagnostic(plan, RuntimeWorldRegionStreamingDiagnosticCode::protected_active_region_unload_requested,
                           active_id, "protected active world region cannot be planned for unload");
        }
    }
    if (!plan.diagnostics.empty()) {
        plan.status = RuntimeWorldRegionStreamingPlanStatus::invalid_request;
        return plan;
    }

    for (const auto& desired_id : request.desired_region_ids) {
        const auto* region = find_region(request.regions, desired_id);
        if (region == nullptr) {
            continue;
        }
        plan.projected_resident_bytes += region->estimated_resident_bytes;
        plan.projected_resident_asset_records += region->estimated_asset_records;
        ++plan.projected_resident_region_count;
    }

    if (request.max_resident_regions > 0U && plan.projected_resident_region_count > request.max_resident_regions) {
        add_diagnostic(plan, RuntimeWorldRegionStreamingDiagnosticCode::resident_region_count_exceeded, {},
                       "projected world region count exceeds the configured resident region limit");
    }
    if (request.budget.max_resident_content_bytes.has_value() &&
        plan.projected_resident_bytes > *request.budget.max_resident_content_bytes) {
        add_diagnostic(plan, RuntimeWorldRegionStreamingDiagnosticCode::resident_content_budget_exceeded, {},
                       "projected world region resident bytes exceed the configured budget");
    }
    if (request.budget.max_resident_asset_records.has_value() &&
        plan.projected_resident_asset_records > *request.budget.max_resident_asset_records) {
        add_diagnostic(plan, RuntimeWorldRegionStreamingDiagnosticCode::resident_asset_record_budget_exceeded, {},
                       "projected world region asset records exceed the configured budget");
    }
    if (!plan.diagnostics.empty()) {
        plan.status = RuntimeWorldRegionStreamingPlanStatus::budget_exceeded;
        return plan;
    }

    for (const auto& desired_id : request.desired_region_ids) {
        const auto* region = find_region(request.regions, desired_id);
        if (region != nullptr && contains_id(request.active_region_ids, desired_id)) {
            append_action_row(plan, RuntimeWorldRegionStreamingActionKind::keep_resident, *region,
                              is_protected(request, desired_id));
        } else if (region != nullptr) {
            append_action_row(plan, RuntimeWorldRegionStreamingActionKind::load_region, *region,
                              is_protected(request, desired_id));
        }
    }
    for (const auto& active_id : request.active_region_ids) {
        const auto* region = find_region(request.regions, active_id);
        if (region != nullptr && !contains_id(request.desired_region_ids, active_id)) {
            append_action_row(plan, RuntimeWorldRegionStreamingActionKind::unload_region, *region,
                              is_protected(request, active_id));
        }
    }

    sort_rows(plan);
    count_rows(plan);
    plan.status = plan.rows.empty() ? RuntimeWorldRegionStreamingPlanStatus::no_changes
                                    : RuntimeWorldRegionStreamingPlanStatus::planned;
    return plan;
}

RuntimeWorldRegionNavigationRefReviewResult
review_runtime_world_region_navigation_refs(const RuntimeResidentPackageMountSetV2& mount_set,
                                            const RuntimeWorldRegionNavigationRefReviewRequest& request) {
    RuntimeWorldRegionNavigationRefReviewResult result;
    result.current_mount_generation = mount_set.generation();

    validate_navigation_region_catalog(result, request.regions);
    validate_navigation_route(result, request);
    if (!result.diagnostics.empty()) {
        result.status = RuntimeWorldRegionNavigationReviewStatus::invalid_request;
        return result;
    }

    append_navigation_ref_rows(result, mount_set, request);
    result.status = result.missing_resident_region_count > 0U ? RuntimeWorldRegionNavigationReviewStatus::not_resident
                                                              : RuntimeWorldRegionNavigationReviewStatus::ready;
    return result;
}

RuntimeWorldRegionNavigationPathCacheReviewResult
review_runtime_world_region_navigation_path_cache(const RuntimeResidentPackageMountSetV2& mount_set,
                                                  const RuntimeResidentCatalogCacheV2& catalog_cache,
                                                  const RuntimeWorldRegionNavigationPathCacheReviewRequest& request) {
    const auto ref_review = review_runtime_world_region_navigation_refs(
        mount_set, RuntimeWorldRegionNavigationRefReviewRequest{.regions = request.regions,
                                                                .route_region_ids = request.route_region_ids});
    RuntimeWorldRegionNavigationPathCacheReviewResult result;
    copy_navigation_ref_review(result, ref_review);
    result.current_catalog_generation = catalog_cache.catalog().generation();
    if (result.status != RuntimeWorldRegionNavigationReviewStatus::ready) {
        result.cache_ready = false;
        return result;
    }

    validate_navigation_path_cache_route_shape(result, request);
    if (!result.diagnostics.empty()) {
        result.status = RuntimeWorldRegionNavigationReviewStatus::invalid_request;
        result.cache_ready = false;
        return result;
    }

    if (request.cache.region_path != request.route_region_ids) {
        add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::path_cache_region_path_mismatch,
                                  {},
                                  "world region navigation path cache region path does not match the reviewed route");
    }
    if (request.cache.portal_path != request.route_portal_ids) {
        add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::path_cache_portal_path_mismatch,
                                  {},
                                  "world region navigation path cache portal path does not match the reviewed route");
    }
    if (request.cache.mount_generation != mount_set.generation()) {
        add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::mount_generation_mismatch, {},
                                  "world region navigation path cache mount generation is stale");
    }
    if (request.cache.catalog_generation != catalog_cache.catalog().generation()) {
        add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::catalog_generation_mismatch, {},
                                  "world region navigation path cache catalog generation is stale");
    }
    if (!catalog_cache.has_value()) {
        add_navigation_diagnostic(result, RuntimeWorldRegionNavigationDiagnosticCode::catalog_cache_not_ready, {},
                                  "world region navigation path cache requires a refreshed resident catalog cache");
    } else if (catalog_cache.cached_mount_generation() != mount_set.generation()) {
        add_navigation_diagnostic(
            result, RuntimeWorldRegionNavigationDiagnosticCode::catalog_cache_not_ready, {},
            "world region navigation path cache resident catalog does not match the current mount generation");
    }

    if (!result.diagnostics.empty()) {
        result.status = RuntimeWorldRegionNavigationReviewStatus::stale;
        result.cache_ready = false;
        return result;
    }

    result.status = RuntimeWorldRegionNavigationReviewStatus::ready;
    result.cache_ready = true;
    return result;
}

RuntimeWorldRegionStreamingSafePointResult
execute_runtime_world_region_streaming_safe_point(IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set,
                                                  RuntimeResidentCatalogCacheV2& catalog_cache,
                                                  const RuntimeWorldRegionStreamingSafePointDesc& desc) {
    RuntimeWorldRegionStreamingSafePointResult result;

    validate_safe_point_plan(result, desc);
    if (!result.diagnostics.empty()) {
        result.status = RuntimeWorldRegionStreamingSafePointStatus::invalid_plan;
        return result;
    }

    if (desc.plan.status == RuntimeWorldRegionStreamingPlanStatus::no_changes || desc.plan.rows.empty()) {
        result.status = RuntimeWorldRegionStreamingSafePointStatus::no_changes;
        return result;
    }

    RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set;
    RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache;
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids = desc.protected_mount_ids;
    append_protected_plan_mount_ids(protected_mount_ids, desc.plan);

    for (const auto* row : make_execution_rows(desc.plan)) {
        RuntimeWorldRegionStreamingSafePointRowResult row_result;
        row_result.action = row->action;
        row_result.region_id = row->region_id;
        row_result.mount_id = row->mount_id;
        const auto* region = find_region(desc.regions, row->region_id);
        switch (row->action) {
        case RuntimeWorldRegionStreamingActionKind::keep_resident:
            ++result.keep_count;
            result.rows.push_back(std::move(row_result));
            break;
        case RuntimeWorldRegionStreamingActionKind::load_region:
            row_result.streaming =
                has_mount_id(projected_mount_set, row->mount_id)
                    ? execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
                          filesystem, projected_mount_set, projected_catalog_cache, row->mount_id, desc.overlay,
                          make_streaming_desc(desc, *region), desc.eviction_candidate_unmount_order,
                          protected_mount_ids)
                    : execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
                          filesystem, projected_mount_set, projected_catalog_cache, row->mount_id, desc.overlay,
                          make_streaming_desc(desc, *region), desc.eviction_candidate_unmount_order,
                          protected_mount_ids);
            row_result.committed = row_result.streaming.committed;
            ++result.load_count;
            if (row_result.streaming.committed) {
                ++result.committed_count;
            }
            if (!row_result.streaming.succeeded()) {
                append_package_failure_diagnostic(result, row_result);
                result.rows.push_back(std::move(row_result));
                result.status = RuntimeWorldRegionStreamingSafePointStatus::failed;
                return result;
            }
            result.rows.push_back(std::move(row_result));
            break;
        case RuntimeWorldRegionStreamingActionKind::unload_region:
            auto unload_desc = make_streaming_desc(desc, *region);
            unload_desc.required_preload_assets.clear();
            unload_desc.resident_resource_kinds.clear();
            row_result.streaming = execute_selected_runtime_package_streaming_resident_unmount_safe_point(
                projected_mount_set, projected_catalog_cache, row->mount_id, desc.overlay, unload_desc);
            row_result.committed = row_result.streaming.committed;
            ++result.unload_count;
            if (row_result.streaming.committed) {
                ++result.committed_count;
            }
            if (!row_result.streaming.succeeded()) {
                append_package_failure_diagnostic(result, row_result);
                result.rows.push_back(std::move(row_result));
                result.status = RuntimeWorldRegionStreamingSafePointStatus::failed;
                return result;
            }
            result.rows.push_back(std::move(row_result));
            break;
        }
    }

    const auto final_refresh = projected_catalog_cache.refresh(projected_mount_set, desc.overlay, desc.budget);
    if (!final_refresh.succeeded()) {
        const auto budget_code =
            final_refresh.budget_execution.estimated_resident_content_bytes > streaming_budget_bytes(desc)
                ? RuntimeWorldRegionStreamingDiagnosticCode::resident_content_budget_exceeded
                : RuntimeWorldRegionStreamingDiagnosticCode::resident_asset_record_budget_exceeded;
        add_diagnostic(result, budget_code, {}, "world region safe-point final resident catalog budget failed");
        result.status = RuntimeWorldRegionStreamingSafePointStatus::failed;
        return result;
    }

    mount_set = std::move(projected_mount_set);
    catalog_cache = std::move(projected_catalog_cache);
    result.committed = result.committed_count > 0U;
    result.status = RuntimeWorldRegionStreamingSafePointStatus::completed;
    return result;
}

RuntimeWorldStreamingLargeSceneReadinessReport
evaluate_runtime_world_streaming_large_scene_readiness(const RuntimeWorldStreamingLargeSceneReadinessRequest& request,
                                                       const RuntimeWorldStreamingLargeSceneReadinessConfig& config) {
    RuntimeWorldStreamingLargeSceneReadinessReport report{
        .status = RuntimeWorldStreamingLargeSceneReadinessStatus::diagnostics,
        .diagnostic = RuntimeWorldStreamingLargeSceneReadinessDiagnostic::none,
        .diagnostics = {},
        .first_plan_status = request.streaming_plans.empty() ? RuntimeWorldRegionStreamingPlanStatus::invalid_request
                                                             : request.streaming_plans.front().status,
        .first_safe_point_status = request.safe_points.empty()
                                       ? RuntimeWorldRegionStreamingSafePointStatus::invalid_plan
                                       : request.safe_points.front().status,
        .navigation_refs_status = request.navigation_refs == nullptr
                                      ? RuntimeWorldRegionNavigationReviewStatus::invalid_request
                                      : request.navigation_refs->status,
        .navigation_path_cache_status = request.navigation_path_cache == nullptr
                                            ? RuntimeWorldRegionNavigationReviewStatus::invalid_request
                                            : request.navigation_path_cache->status,
        .plan_rows = 0U,
        .load_rows = 0U,
        .keep_rows = 0U,
        .unload_rows = 0U,
        .safe_point_rows = 0U,
        .committed_rows = 0U,
        .reviewed_package_adoptions = 0U,
        .projected_resident_regions = 0U,
        .projected_resident_bytes = 0U,
        .max_projected_resident_regions = config.max_projected_resident_regions,
        .max_projected_resident_bytes = config.max_projected_resident_bytes,
        .missing_region_diagnostics = 0U,
        .safe_point_diagnostics = 0U,
        .navigation_resident_regions =
            request.navigation_refs == nullptr ? 0U : request.navigation_refs->resident_region_count,
        .navigation_missing_resident_regions =
            request.navigation_refs == nullptr ? 0U : request.navigation_refs->missing_resident_region_count,
        .navigation_path_cache_ready =
            request.navigation_path_cache != nullptr && request.navigation_path_cache->cache_ready,
    };

    if (request.streaming_plans.empty()) {
        append_large_scene_diagnostic(report,
                                      RuntimeWorldStreamingLargeSceneReadinessDiagnostic::invalid_streaming_plan);
    }

    for (const auto& plan : request.streaming_plans) {
        if (!plan.succeeded()) {
            append_large_scene_diagnostic(report,
                                          RuntimeWorldStreamingLargeSceneReadinessDiagnostic::invalid_streaming_plan);
        }
        report.plan_rows += plan.rows.size();
        report.load_rows += plan.load_count;
        report.keep_rows += plan.keep_count;
        report.unload_rows += plan.unload_count;
        report.projected_resident_regions =
            std::max(report.projected_resident_regions, plan.projected_resident_region_count);
        report.projected_resident_bytes = std::max(report.projected_resident_bytes, plan.projected_resident_bytes);
        report.missing_region_diagnostics +=
            count_plan_diagnostics(plan, RuntimeWorldRegionStreamingDiagnosticCode::missing_desired_region);
    }

    if (request.missing_region_probe != nullptr) {
        const auto missing_region_diagnostics = count_plan_diagnostics(
            *request.missing_region_probe, RuntimeWorldRegionStreamingDiagnosticCode::missing_desired_region);
        report.missing_region_diagnostics += missing_region_diagnostics;
        if (!request.missing_region_probe->succeeded() &&
            !failed_only_for_missing_regions(*request.missing_region_probe, missing_region_diagnostics)) {
            append_large_scene_diagnostic(report,
                                          RuntimeWorldStreamingLargeSceneReadinessDiagnostic::invalid_streaming_plan);
        }
    }

    for (const auto& safe_point : request.safe_points) {
        if (!safe_point.succeeded()) {
            append_large_scene_diagnostic(
                report, RuntimeWorldStreamingLargeSceneReadinessDiagnostic::streaming_safe_point_failed);
        }
        report.safe_point_rows += safe_point.rows.size();
        report.committed_rows += safe_point.committed_count;
        report.safe_point_diagnostics += safe_point.diagnostics.size();
        for (const auto& row : safe_point.rows) {
            if (row.action == RuntimeWorldRegionStreamingActionKind::load_region && row.committed) {
                ++report.reviewed_package_adoptions;
            }
        }
    }

    if (report.plan_rows < config.min_plan_rows) {
        append_large_scene_diagnostic(report,
                                      RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_plan_rows);
    }
    if (report.load_rows < config.min_load_rows) {
        append_large_scene_diagnostic(report,
                                      RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_load_rows);
    }
    if (report.keep_rows < config.min_keep_rows) {
        append_large_scene_diagnostic(report,
                                      RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_keep_rows);
    }
    if (report.unload_rows < config.min_unload_rows) {
        append_large_scene_diagnostic(report,
                                      RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_unload_rows);
    }
    if (report.safe_point_rows < config.min_safe_point_rows) {
        append_large_scene_diagnostic(report,
                                      RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_safe_point_rows);
    }
    if (report.committed_rows < config.min_committed_rows) {
        append_large_scene_diagnostic(report,
                                      RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_committed_rows);
    }
    if (report.reviewed_package_adoptions < config.min_reviewed_package_adoptions) {
        append_large_scene_diagnostic(
            report, RuntimeWorldStreamingLargeSceneReadinessDiagnostic::missing_reviewed_package_adoption);
    }
    if (config.require_missing_region_diagnostic && report.missing_region_diagnostics == 0U) {
        append_large_scene_diagnostic(
            report, RuntimeWorldStreamingLargeSceneReadinessDiagnostic::missing_region_diagnostic_absent);
    }
    if (report.safe_point_diagnostics > config.max_safe_point_diagnostics) {
        append_large_scene_diagnostic(
            report, RuntimeWorldStreamingLargeSceneReadinessDiagnostic::safe_point_diagnostics_present);
    }
    if (report.projected_resident_regions > config.max_projected_resident_regions) {
        append_large_scene_diagnostic(
            report, RuntimeWorldStreamingLargeSceneReadinessDiagnostic::projected_region_budget_exceeded);
    }
    if (report.projected_resident_bytes > config.max_projected_resident_bytes) {
        append_large_scene_diagnostic(
            report, RuntimeWorldStreamingLargeSceneReadinessDiagnostic::projected_byte_budget_exceeded);
    }
    if (config.require_navigation_refs_ready &&
        (request.navigation_refs == nullptr || !request.navigation_refs->succeeded())) {
        append_large_scene_diagnostic(report,
                                      RuntimeWorldStreamingLargeSceneReadinessDiagnostic::navigation_refs_not_ready);
    }
    if (config.require_navigation_path_cache_ready &&
        (request.navigation_path_cache == nullptr || !request.navigation_path_cache->succeeded() ||
         !request.navigation_path_cache->cache_ready)) {
        append_large_scene_diagnostic(
            report, RuntimeWorldStreamingLargeSceneReadinessDiagnostic::navigation_path_cache_not_ready);
    }

    if (report.diagnostics.empty()) {
        report.status = RuntimeWorldStreamingLargeSceneReadinessStatus::ready;
    } else if (report.diagnostic == RuntimeWorldStreamingLargeSceneReadinessDiagnostic::invalid_streaming_plan) {
        report.status = RuntimeWorldStreamingLargeSceneReadinessStatus::invalid_evidence;
    } else {
        report.status = RuntimeWorldStreamingLargeSceneReadinessStatus::diagnostics;
    }
    return report;
}

} // namespace mirakana::runtime
