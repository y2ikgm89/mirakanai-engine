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

} // namespace mirakana::runtime
