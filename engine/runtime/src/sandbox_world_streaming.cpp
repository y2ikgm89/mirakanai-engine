// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/sandbox_world_streaming.hpp"

#include <algorithm>
#include <limits>
#include <string_view>
#include <utility>

namespace mirakana::runtime {
namespace {

struct SelectedChunk {
    const RuntimeSandboxChunkRow* chunk{nullptr};
    std::string source_id;
    RuntimeSandboxWorldStreamingTargetState target_state{RuntimeSandboxWorldStreamingTargetState::loaded};
    std::int32_t priority{0};
    bool dirty_pinned{false};
    std::uint32_t source_index{0U};
};

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) {
    return std::ranges::any_of(ids, [id](const std::string& candidate) { return candidate == id; });
}

template <typename T> void push_unique(std::vector<T>& values, T value) {
    if (std::ranges::find(values, value) == values.end()) {
        values.push_back(std::move(value));
    }
}

void add_diagnostic(RuntimeSandboxWorldStreamingPlan& plan, std::string_view world_id,
                    RuntimeSandboxWorldStreamingDiagnosticCode code, std::string chunk_id, std::string source_id,
                    RuntimeAddressableAssetId address_id, std::string message, std::uint32_t source_index = 0U) {
    plan.diagnostics.push_back(RuntimeSandboxWorldStreamingDiagnostic{
        .code = code,
        .world_id = std::string{world_id},
        .chunk_id = std::move(chunk_id),
        .source_id = std::move(source_id),
        .address_id = std::move(address_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void add_result_diagnostic(RuntimeSandboxWorldStreamingSafePointResult& result,
                           RuntimeSandboxWorldStreamingDiagnosticCode code, std::string message) {
    result.diagnostics.push_back(RuntimeSandboxWorldStreamingDiagnostic{
        .code = code,
        .message = std::move(message),
    });
}

[[nodiscard]] const RuntimeSandboxChunkRow* find_chunk(const RuntimeSandboxWorld& world, std::string_view chunk_id) {
    const auto found =
        std::ranges::find_if(world.chunks, [chunk_id](const auto& row) { return row.chunk_id == chunk_id; });
    return found == world.chunks.end() ? nullptr : &*found;
}

[[nodiscard]] const RuntimeWorldRegionPackageDesc*
find_region(const std::vector<RuntimeWorldRegionPackageDesc>& regions, std::string_view region_id) {
    const auto found =
        std::ranges::find_if(regions, [region_id](const auto& row) { return row.region_id == region_id; });
    return found == regions.end() ? nullptr : &*found;
}

[[nodiscard]] const RuntimeAddressableAssetRow* find_address(const std::vector<RuntimeAddressableAssetRow>& addresses,
                                                             std::string_view address_id) {
    const auto found =
        std::ranges::find_if(addresses, [address_id](const auto& row) { return row.address_id.value == address_id; });
    return found == addresses.end() ? nullptr : &*found;
}

[[nodiscard]] bool contains_dependency_kind(const std::vector<RuntimeSandboxWorldAddressableDependencyRow>& rows,
                                            std::string_view chunk_id,
                                            RuntimeSandboxWorldAddressableDependencyKind kind) {
    return std::ranges::any_of(rows, [chunk_id, kind](const auto& row) {
        return row.required && row.chunk_id == chunk_id && row.kind == kind;
    });
}

[[nodiscard]] bool contains_dependency_row(const std::vector<RuntimeSandboxWorldAddressableDependencyRow>& rows,
                                           const RuntimeSandboxWorldAddressableDependencyRow& candidate) {
    return std::ranges::any_of(rows, [&candidate](const auto& row) {
        return row.chunk_id == candidate.chunk_id && row.kind == candidate.kind &&
               row.address_id.value == candidate.address_id.value;
    });
}

[[nodiscard]] bool source_range_valid(const RuntimeSandboxWorldStreamingSourceRow& source) {
    if (!is_valid_id(source.source_id)) {
        return false;
    }
    if (source.range_kind == RuntimeSandboxWorldStreamingRangeKind::radius) {
        return source.radius > 0U;
    }
    return source.max_coord_exclusive.x > source.min_coord.x && source.max_coord_exclusive.y > source.min_coord.y;
}

[[nodiscard]] bool chunk_intersects_rectangle(const RuntimeSandboxChunkRow& chunk,
                                              const RuntimeSandboxWorldStreamingSourceRow& source) {
    const auto chunk_max_x = chunk.origin_x + static_cast<std::int32_t>(chunk.size_x);
    const auto chunk_max_y = chunk.origin_y + static_cast<std::int32_t>(chunk.size_y);
    return chunk.origin_x < source.max_coord_exclusive.x && chunk_max_x > source.min_coord.x &&
           chunk.origin_y < source.max_coord_exclusive.y && chunk_max_y > source.min_coord.y;
}

[[nodiscard]] std::int64_t square(std::int64_t value) noexcept {
    return value * value;
}

[[nodiscard]] bool chunk_intersects_radius(const RuntimeSandboxChunkRow& chunk,
                                           const RuntimeSandboxWorldStreamingSourceRow& source) {
    const auto chunk_max_x = chunk.origin_x + static_cast<std::int32_t>(chunk.size_x) - 1;
    const auto chunk_max_y = chunk.origin_y + static_cast<std::int32_t>(chunk.size_y) - 1;
    const auto closest_x = std::clamp(source.position.x, chunk.origin_x, chunk_max_x);
    const auto closest_y = std::clamp(source.position.y, chunk.origin_y, chunk_max_y);
    const auto radius = static_cast<std::int64_t>(source.radius);
    const auto distance_squared = square(static_cast<std::int64_t>(source.position.x) - closest_x) +
                                  square(static_cast<std::int64_t>(source.position.y) - closest_y);
    return distance_squared <= square(radius);
}

[[nodiscard]] bool source_selects_chunk(const RuntimeSandboxChunkRow& chunk,
                                        const RuntimeSandboxWorldStreamingSourceRow& source) {
    return source.range_kind == RuntimeSandboxWorldStreamingRangeKind::rectangle
               ? chunk_intersects_rectangle(chunk, source)
               : chunk_intersects_radius(chunk, source);
}

[[nodiscard]] bool target_state_preferred(RuntimeSandboxWorldStreamingTargetState lhs,
                                          RuntimeSandboxWorldStreamingTargetState rhs) {
    return static_cast<std::uint8_t>(lhs) > static_cast<std::uint8_t>(rhs);
}

void upsert_selected_chunk(std::vector<SelectedChunk>& chunks, const RuntimeSandboxChunkRow& chunk,
                           const RuntimeSandboxWorldStreamingSourceRow& source) {
    auto found = std::ranges::find_if(chunks, [&chunk](const auto& row) { return row.chunk == &chunk; });
    if (found == chunks.end()) {
        chunks.push_back(SelectedChunk{
            .chunk = &chunk,
            .source_id = source.source_id,
            .target_state = source.target_state,
            .priority = source.priority,
            .dirty_pinned = false,
            .source_index = source.source_index,
        });
        return;
    }
    if (source.priority > found->priority ||
        (source.priority == found->priority && target_state_preferred(source.target_state, found->target_state)) ||
        (source.priority == found->priority && source.target_state == found->target_state &&
         source.source_index < found->source_index)) {
        found->source_id = source.source_id;
        found->target_state = source.target_state;
        found->priority = source.priority;
        found->source_index = source.source_index;
    }
}

void pin_dirty_chunks(RuntimeSandboxWorldStreamingPlan& plan, const RuntimeSandboxWorldStreamingPlanRequest& request,
                      std::vector<SelectedChunk>& selected_chunks) {
    std::vector<std::string> seen_dirty_chunks;
    for (const auto& chunk_id : request.dirty_chunk_ids) {
        if (!is_valid_id(chunk_id)) {
            add_diagnostic(plan, request.world.world_id, RuntimeSandboxWorldStreamingDiagnosticCode::unknown_chunk,
                           chunk_id, {}, {}, "dirty sandbox chunk id must be non-empty and path-safe");
            continue;
        }
        if (contains_id(seen_dirty_chunks, chunk_id)) {
            add_diagnostic(plan, request.world.world_id,
                           RuntimeSandboxWorldStreamingDiagnosticCode::duplicate_dirty_chunk, chunk_id, {}, {},
                           "dirty sandbox chunk ids must be unique");
            continue;
        }
        seen_dirty_chunks.push_back(chunk_id);
        const auto* chunk = find_chunk(request.world, chunk_id);
        if (chunk == nullptr) {
            add_diagnostic(plan, request.world.world_id, RuntimeSandboxWorldStreamingDiagnosticCode::unknown_chunk,
                           chunk_id, {}, {}, "dirty sandbox chunk id must reference the current runtime world");
            continue;
        }
        if (!chunk->resident) {
            continue;
        }
        auto found = std::ranges::find_if(selected_chunks, [chunk](const auto& row) { return row.chunk == chunk; });
        if (found == selected_chunks.end()) {
            selected_chunks.push_back(SelectedChunk{
                .chunk = chunk,
                .source_id = "dirty-pin",
                .target_state = RuntimeSandboxWorldStreamingTargetState::loaded,
                .priority = std::numeric_limits<std::int32_t>::max(),
                .dirty_pinned = true,
                .source_index = chunk->source_index,
            });
        } else {
            found->dirty_pinned = true;
        }
    }
}

[[nodiscard]] bool is_dirty_chunk(const RuntimeSandboxWorldStreamingPlanRequest& request, std::string_view chunk_id) {
    return contains_id(request.dirty_chunk_ids, chunk_id);
}

[[nodiscard]] RuntimeWorldRegionStreamingActionKind chunk_action(const RuntimeSandboxChunkRow& chunk,
                                                                 bool desired) noexcept {
    if (chunk.resident && desired) {
        return RuntimeWorldRegionStreamingActionKind::keep_resident;
    }
    if (!chunk.resident && desired) {
        return RuntimeWorldRegionStreamingActionKind::load_region;
    }
    return RuntimeWorldRegionStreamingActionKind::unload_region;
}

void append_chunk_rows(RuntimeSandboxWorldStreamingPlan& plan, const RuntimeSandboxWorldStreamingPlanRequest& request,
                       const std::vector<SelectedChunk>& selected_chunks) {
    for (const auto& selected : selected_chunks) {
        if (selected.chunk == nullptr) {
            continue;
        }
        const auto* region = find_region(request.region_packages, selected.chunk->region_id);
        plan.chunk_rows.push_back(RuntimeSandboxWorldStreamingChunkPlanRow{
            .chunk_id = selected.chunk->chunk_id,
            .region_id = selected.chunk->region_id,
            .source_id = selected.source_id,
            .target_state = selected.target_state,
            .world_action = chunk_action(*selected.chunk, true),
            .priority = selected.priority,
            .resident_before = selected.chunk->resident,
            .dirty_pinned = selected.dirty_pinned,
            .estimated_resident_bytes = region == nullptr ? 0U : region->estimated_resident_bytes,
            .source_index = selected.source_index,
        });
    }
    std::ranges::sort(plan.chunk_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.chunk_id != rhs.chunk_id) {
            return lhs.chunk_id < rhs.chunk_id;
        }
        return lhs.source_index < rhs.source_index;
    });
    plan.selected_chunk_count = plan.chunk_rows.size();
    plan.dirty_pinned_chunk_count = static_cast<std::size_t>(
        std::ranges::count_if(plan.chunk_rows, [](const auto& row) { return row.dirty_pinned; }));
}

void append_dependency_count(RuntimeSandboxWorldStreamingPlan& plan,
                             RuntimeSandboxWorldAddressableDependencyKind kind) noexcept {
    switch (kind) {
    case RuntimeSandboxWorldAddressableDependencyKind::tile_atlas:
        ++plan.tile_atlas_dependency_count;
        break;
    case RuntimeSandboxWorldAddressableDependencyKind::biome_material:
        ++plan.biome_material_dependency_count;
        break;
    case RuntimeSandboxWorldAddressableDependencyKind::audio:
        ++plan.audio_dependency_count;
        break;
    case RuntimeSandboxWorldAddressableDependencyKind::prefab_object:
        ++plan.prefab_object_dependency_count;
        break;
    }
}

[[nodiscard]] bool chunk_has_dependency_rows(const RuntimeSandboxWorldStreamingPlanRequest& request,
                                             std::string_view chunk_id) {
    return std::ranges::any_of(request.addressable_dependencies,
                               [chunk_id](const auto& row) { return row.chunk_id == chunk_id; });
}

RuntimeAddressableContentStreamingRequest
append_addressable_rows(RuntimeSandboxWorldStreamingPlan& plan, const RuntimeSandboxWorldStreamingPlanRequest& request,
                        const std::vector<SelectedChunk>& selected_chunks) {
    RuntimeAddressableContentStreamingRequest addressable_request = request.addressable_content;
    std::vector<RuntimeSandboxWorldAddressableDependencyRow> seen_dependencies;

    for (const auto& selected : selected_chunks) {
        if (selected.chunk == nullptr) {
            continue;
        }
        if (chunk_has_dependency_rows(request, selected.chunk->chunk_id)) {
            for (const auto required_kind : request.required_dependency_kinds) {
                if (!contains_dependency_kind(request.addressable_dependencies, selected.chunk->chunk_id,
                                              required_kind)) {
                    add_diagnostic(plan, request.world.world_id,
                                   RuntimeSandboxWorldStreamingDiagnosticCode::missing_addressable_dependency,
                                   selected.chunk->chunk_id, selected.source_id, {},
                                   "selected sandbox chunk is missing a required addressable dependency kind",
                                   selected.source_index);
                }
            }
        }
        for (const auto& dependency : request.addressable_dependencies) {
            if (dependency.chunk_id != selected.chunk->chunk_id) {
                continue;
            }
            if (contains_dependency_row(seen_dependencies, dependency)) {
                add_diagnostic(plan, request.world.world_id,
                               RuntimeSandboxWorldStreamingDiagnosticCode::duplicate_dependency, dependency.chunk_id,
                               selected.source_id, dependency.address_id,
                               "sandbox chunk addressable dependencies must be unique", dependency.source_index);
                continue;
            }
            seen_dependencies.push_back(dependency);
            if (dependency.address_id.value.empty() || dependency.asset.value == 0U ||
                find_address(addressable_request.addressable_assets, dependency.address_id.value) == nullptr) {
                add_diagnostic(plan, request.world.world_id,
                               RuntimeSandboxWorldStreamingDiagnosticCode::missing_addressable_dependency,
                               dependency.chunk_id, selected.source_id, dependency.address_id,
                               "sandbox chunk dependency must reference a reviewed addressable asset row",
                               dependency.source_index);
                continue;
            }

            plan.addressable_dependency_rows.push_back(RuntimeSandboxWorldAddressableDependencyPlanRow{
                .chunk_id = dependency.chunk_id,
                .kind = dependency.kind,
                .address_id = dependency.address_id,
                .asset = dependency.asset,
                .selected = true,
                .source_index = dependency.source_index,
            });
            append_dependency_count(plan, dependency.kind);
            addressable_request.load_requests.push_back(RuntimeAddressableLoadRequest{
                .address_id = dependency.address_id,
                .include_dependencies = true,
                .source_index = dependency.source_index,
            });
        }
    }

    std::ranges::sort(plan.addressable_dependency_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.chunk_id != rhs.chunk_id) {
            return lhs.chunk_id < rhs.chunk_id;
        }
        if (lhs.kind != rhs.kind) {
            return static_cast<std::uint8_t>(lhs.kind) < static_cast<std::uint8_t>(rhs.kind);
        }
        return lhs.address_id.value < rhs.address_id.value;
    });
    return addressable_request;
}

void map_world_region_diagnostics(RuntimeSandboxWorldStreamingPlan& plan, std::string_view world_id) {
    for (const auto& diagnostic : plan.world_region_plan.diagnostics) {
        switch (diagnostic.code) {
        case RuntimeWorldRegionStreamingDiagnosticCode::resident_region_count_exceeded:
            add_diagnostic(plan, world_id, RuntimeSandboxWorldStreamingDiagnosticCode::resident_chunk_count_exceeded,
                           {}, {}, {}, diagnostic.message);
            break;
        case RuntimeWorldRegionStreamingDiagnosticCode::resident_content_budget_exceeded:
            add_diagnostic(plan, world_id, RuntimeSandboxWorldStreamingDiagnosticCode::resident_payload_budget_exceeded,
                           {}, {}, {}, diagnostic.message);
            break;
        default:
            add_diagnostic(plan, world_id, RuntimeSandboxWorldStreamingDiagnosticCode::world_region_plan_failed,
                           diagnostic.region_id, {}, {}, diagnostic.message);
            break;
        }
    }
}

void map_addressable_status(RuntimeSandboxWorldStreamingPlan& plan, std::string_view world_id) {
    if (plan.addressable_plan.status == RuntimeAddressableContentStreamingStatus::budget_limited) {
        add_diagnostic(plan, world_id, RuntimeSandboxWorldStreamingDiagnosticCode::resident_payload_budget_exceeded, {},
                       {}, {}, "sandbox addressable dependency resident budget was exceeded");
    } else if (plan.addressable_plan.status == RuntimeAddressableContentStreamingStatus::invalid_request) {
        add_diagnostic(plan, world_id, RuntimeSandboxWorldStreamingDiagnosticCode::addressable_plan_failed, {}, {}, {},
                       "sandbox addressable dependency plan failed");
    }
}

[[nodiscard]] bool should_plan_addressables(const RuntimeAddressableContentStreamingRequest& request) {
    return !request.stream_id.empty() || !request.addressable_assets.empty() || !request.package.empty() ||
           !request.resident_assets.empty() || !request.load_requests.empty() || !request.release_requests.empty();
}

} // namespace

bool RuntimeSandboxWorldStreamingPlan::succeeded() const noexcept {
    return status == RuntimeSandboxWorldStreamingStatus::ready ||
           status == RuntimeSandboxWorldStreamingStatus::no_changes;
}

bool RuntimeSandboxWorldStreamingSafePointResult::succeeded() const noexcept {
    return status == RuntimeSandboxWorldStreamingSafePointStatus::completed ||
           status == RuntimeSandboxWorldStreamingSafePointStatus::no_changes;
}

RuntimeSandboxWorldStreamingPlan
plan_runtime_sandbox_world_streaming(const RuntimeSandboxWorldStreamingPlanRequest& request) {
    RuntimeSandboxWorldStreamingPlan plan;
    plan.package_regions = request.region_packages;

    if (!is_valid_id(request.world.world_id)) {
        add_diagnostic(plan, request.world.world_id, RuntimeSandboxWorldStreamingDiagnosticCode::missing_world_id, {},
                       {}, {}, "sandbox streaming world id must be non-empty and path-safe");
    }
    if (request.allow_automatic_lru_eviction) {
        add_diagnostic(plan, request.world.world_id,
                       RuntimeSandboxWorldStreamingDiagnosticCode::unsupported_automatic_lru_eviction, {}, {}, {},
                       "sandbox streaming requires explicit eviction review; automatic LRU is unsupported");
    }
    if (request.row_budget > 0U &&
        request.sources.size() + request.world.chunks.size() + request.addressable_dependencies.size() >
            request.row_budget) {
        add_diagnostic(plan, request.world.world_id, RuntimeSandboxWorldStreamingDiagnosticCode::row_budget_exceeded,
                       {}, {}, {}, "sandbox streaming request exceeds the configured row budget");
    }

    std::vector<std::string> seen_sources;
    std::vector<SelectedChunk> selected_chunks;
    for (const auto& source : request.sources) {
        if (!source_range_valid(source)) {
            add_diagnostic(plan, request.world.world_id, RuntimeSandboxWorldStreamingDiagnosticCode::invalid_source, {},
                           source.source_id, {}, "sandbox streaming source requires a valid id and load range",
                           source.source_index);
            continue;
        }
        if (contains_id(seen_sources, source.source_id)) {
            add_diagnostic(plan, request.world.world_id, RuntimeSandboxWorldStreamingDiagnosticCode::duplicate_source,
                           {}, source.source_id, {}, "sandbox streaming source ids must be unique",
                           source.source_index);
            continue;
        }
        seen_sources.push_back(source.source_id);
        for (const auto& chunk : request.world.chunks) {
            if (source_selects_chunk(chunk, source)) {
                upsert_selected_chunk(selected_chunks, chunk, source);
            }
        }
    }
    pin_dirty_chunks(plan, request, selected_chunks);
    append_chunk_rows(plan, request, selected_chunks);

    std::vector<std::string> active_region_ids;
    std::vector<std::string> desired_region_ids;
    std::vector<std::string> protected_region_ids;
    for (const auto& chunk : request.world.chunks) {
        if (chunk.resident) {
            push_unique(active_region_ids, chunk.region_id);
        }
        if (chunk.resident && is_dirty_chunk(request, chunk.chunk_id)) {
            push_unique(protected_region_ids, chunk.region_id);
        }
    }
    for (const auto& selected : selected_chunks) {
        if (selected.chunk != nullptr) {
            push_unique(desired_region_ids, selected.chunk->region_id);
        }
    }

    plan.world_region_plan = plan_runtime_world_region_streaming(RuntimeWorldRegionStreamingPlanRequest{
        .regions = request.region_packages,
        .active_region_ids = std::move(active_region_ids),
        .desired_region_ids = std::move(desired_region_ids),
        .protected_region_ids = std::move(protected_region_ids),
        .budget = request.budget,
        .max_resident_regions = request.max_resident_chunks,
    });
    if (!plan.world_region_plan.succeeded()) {
        map_world_region_diagnostics(plan, request.world.world_id);
    }

    const auto addressable_request = append_addressable_rows(plan, request, selected_chunks);
    if (plan.diagnostics.empty() && should_plan_addressables(addressable_request)) {
        plan.addressable_plan = plan_runtime_addressable_content_streaming(addressable_request);
        if (!plan.addressable_plan.succeeded()) {
            map_addressable_status(plan, request.world.world_id);
        }
    } else if (!should_plan_addressables(addressable_request)) {
        plan.addressable_plan.status = RuntimeAddressableContentStreamingStatus::no_requests;
    }

    plan.eviction_review_required =
        plan.world_region_plan.status == RuntimeWorldRegionStreamingPlanStatus::budget_exceeded ||
        plan.addressable_plan.status == RuntimeAddressableContentStreamingStatus::budget_limited;

    if (std::ranges::any_of(plan.diagnostics, [](const auto& diagnostic) {
            return diagnostic.code == RuntimeSandboxWorldStreamingDiagnosticCode::resident_chunk_count_exceeded ||
                   diagnostic.code == RuntimeSandboxWorldStreamingDiagnosticCode::resident_payload_budget_exceeded;
        })) {
        plan.status = RuntimeSandboxWorldStreamingStatus::budget_exceeded;
    } else if (!plan.diagnostics.empty()) {
        plan.status = RuntimeSandboxWorldStreamingStatus::invalid_request;
    } else if (plan.chunk_rows.empty() && plan.addressable_dependency_rows.empty()) {
        plan.status = RuntimeSandboxWorldStreamingStatus::no_changes;
    } else {
        plan.status = RuntimeSandboxWorldStreamingStatus::ready;
    }
    return plan;
}

RuntimeSandboxWorldStreamingSafePointResult
execute_runtime_sandbox_world_streaming_safe_point(IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set,
                                                   RuntimeResidentCatalogCacheV2& catalog_cache,
                                                   const RuntimeSandboxWorldStreamingSafePointDesc& desc) {
    RuntimeSandboxWorldStreamingSafePointResult result;
    result.addressable_plan = desc.plan.addressable_plan;

    if (!desc.plan.succeeded() || !desc.plan.world_region_plan.succeeded() || !desc.plan.addressable_plan.succeeded()) {
        result.diagnostics = desc.plan.diagnostics;
        result.status = RuntimeSandboxWorldStreamingSafePointStatus::invalid_plan;
        return result;
    }

    const auto world_result = execute_runtime_world_region_streaming_safe_point(
        filesystem, mount_set, catalog_cache,
        RuntimeWorldRegionStreamingSafePointDesc{
            .plan = desc.plan.world_region_plan,
            .regions = desc.plan.package_regions,
            .target_id = desc.target_id,
            .runtime_scene_validation_target_id = desc.runtime_scene_validation_target_id,
            .overlay = desc.overlay,
            .budget = desc.budget,
            .max_resident_packages = desc.max_resident_packages,
            .safe_point_required = desc.safe_point_required,
            .runtime_scene_validation_succeeded = desc.runtime_scene_validation_succeeded,
            .eviction_candidate_unmount_order = desc.eviction_candidate_unmount_order,
            .protected_mount_ids = desc.protected_mount_ids,
        });
    result.world_region_result = world_result;
    result.invoked_package_io = world_result.load_count > 0U;
    result.committed = world_result.committed;

    switch (world_result.status) {
    case RuntimeWorldRegionStreamingSafePointStatus::completed:
        result.status = RuntimeSandboxWorldStreamingSafePointStatus::completed;
        break;
    case RuntimeWorldRegionStreamingSafePointStatus::no_changes:
        result.status = RuntimeSandboxWorldStreamingSafePointStatus::no_changes;
        break;
    case RuntimeWorldRegionStreamingSafePointStatus::failed:
        add_result_diagnostic(result, RuntimeSandboxWorldStreamingDiagnosticCode::package_streaming_execution_failed,
                              "sandbox world-region package streaming safe point failed");
        result.status = RuntimeSandboxWorldStreamingSafePointStatus::failed;
        break;
    case RuntimeWorldRegionStreamingSafePointStatus::invalid_plan:
        result.status = RuntimeSandboxWorldStreamingSafePointStatus::invalid_plan;
        for (const auto& diagnostic : world_result.diagnostics) {
            add_result_diagnostic(result, RuntimeSandboxWorldStreamingDiagnosticCode::world_region_plan_failed,
                                  diagnostic.message);
        }
        break;
    }

    return result;
}

} // namespace mirakana::runtime
