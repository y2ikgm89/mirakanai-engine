// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/addressable_content_streaming.hpp"
#include "mirakana/runtime/sandbox_world_runtime.hpp"
#include "mirakana/runtime/world_region_streaming.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeSandboxWorldStreamingRangeKind : std::uint8_t {
    radius = 0,
    rectangle,
};

enum class RuntimeSandboxWorldStreamingTargetState : std::uint8_t {
    loaded = 0,
    active,
};

enum class RuntimeSandboxWorldStreamingStatus : std::uint8_t {
    ready = 0,
    no_changes,
    invalid_request,
    budget_exceeded,
};

enum class RuntimeSandboxWorldStreamingSafePointStatus : std::uint8_t {
    invalid_plan = 0,
    no_changes,
    failed,
    completed,
};

enum class RuntimeSandboxWorldAddressableDependencyKind : std::uint8_t {
    tile_atlas = 0,
    biome_material,
    audio,
    prefab_object,
};

enum class RuntimeSandboxWorldStreamingDiagnosticCode : std::uint8_t {
    missing_world_id = 0,
    invalid_source,
    duplicate_source,
    invalid_range,
    unknown_chunk,
    duplicate_dirty_chunk,
    duplicate_dependency,
    missing_addressable_dependency,
    missing_region_package,
    resident_chunk_count_exceeded,
    resident_payload_budget_exceeded,
    row_budget_exceeded,
    unsupported_automatic_lru_eviction,
    world_region_plan_failed,
    addressable_plan_failed,
    package_streaming_execution_failed,
};

struct RuntimeSandboxWorldStreamingSourceRow {
    std::string source_id;
    RuntimeSandboxCellCoord position;
    RuntimeSandboxWorldStreamingRangeKind range_kind{RuntimeSandboxWorldStreamingRangeKind::radius};
    std::uint32_t radius{0U};
    RuntimeSandboxCellCoord min_coord;
    RuntimeSandboxCellCoord max_coord_exclusive;
    RuntimeSandboxWorldStreamingTargetState target_state{RuntimeSandboxWorldStreamingTargetState::loaded};
    std::int32_t priority{0};
    bool player_source{false};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldStreamingSourceRow&) const = default;
};

struct RuntimeSandboxWorldAddressableDependencyRow {
    std::string chunk_id;
    RuntimeSandboxWorldAddressableDependencyKind kind{RuntimeSandboxWorldAddressableDependencyKind::tile_atlas};
    RuntimeAddressableAssetId address_id;
    AssetId asset;
    bool required{true};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldAddressableDependencyRow&) const = default;
};

struct RuntimeSandboxWorldStreamingDiagnostic {
    RuntimeSandboxWorldStreamingDiagnosticCode code{RuntimeSandboxWorldStreamingDiagnosticCode::missing_world_id};
    std::string world_id;
    std::string chunk_id;
    std::string source_id;
    RuntimeAddressableAssetId address_id;
    std::string message;
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldStreamingDiagnostic&) const = default;
};

struct RuntimeSandboxWorldStreamingChunkPlanRow {
    std::string chunk_id;
    std::string region_id;
    std::string source_id;
    RuntimeSandboxWorldStreamingTargetState target_state{RuntimeSandboxWorldStreamingTargetState::loaded};
    RuntimeWorldRegionStreamingActionKind world_action{RuntimeWorldRegionStreamingActionKind::keep_resident};
    std::int32_t priority{0};
    bool resident_before{false};
    bool dirty_pinned{false};
    std::uint64_t estimated_resident_bytes{0U};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldStreamingChunkPlanRow&) const = default;
};

struct RuntimeSandboxWorldAddressableDependencyPlanRow {
    std::string chunk_id;
    RuntimeSandboxWorldAddressableDependencyKind kind{RuntimeSandboxWorldAddressableDependencyKind::tile_atlas};
    RuntimeAddressableAssetId address_id;
    AssetId asset;
    bool selected{false};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxWorldAddressableDependencyPlanRow&) const = default;
};

struct RuntimeSandboxWorldStreamingPlanRequest {
    RuntimeSandboxWorld world;
    std::vector<RuntimeSandboxWorldStreamingSourceRow> sources;
    std::vector<std::string> dirty_chunk_ids;
    std::vector<RuntimeWorldRegionPackageDesc> region_packages;
    RuntimeAddressableContentStreamingRequest addressable_content;
    std::vector<RuntimeSandboxWorldAddressableDependencyRow> addressable_dependencies;
    RuntimeResourceResidencyBudgetV2 budget;
    std::size_t max_resident_chunks{0U};
    std::vector<RuntimeSandboxWorldAddressableDependencyKind> required_dependency_kinds;
    bool allow_automatic_lru_eviction{false};
    std::size_t row_budget{1024U};
};

struct RuntimeSandboxWorldStreamingPlan {
    RuntimeSandboxWorldStreamingStatus status{RuntimeSandboxWorldStreamingStatus::invalid_request};
    std::vector<RuntimeSandboxWorldStreamingDiagnostic> diagnostics;
    std::vector<RuntimeSandboxWorldStreamingChunkPlanRow> chunk_rows;
    std::vector<RuntimeSandboxWorldAddressableDependencyPlanRow> addressable_dependency_rows;
    std::vector<RuntimeWorldRegionPackageDesc> package_regions;
    RuntimeWorldRegionStreamingPlan world_region_plan;
    RuntimeAddressableContentStreamingPlan addressable_plan;
    std::size_t selected_chunk_count{0U};
    std::size_t dirty_pinned_chunk_count{0U};
    std::size_t tile_atlas_dependency_count{0U};
    std::size_t biome_material_dependency_count{0U};
    std::size_t audio_dependency_count{0U};
    std::size_t prefab_object_dependency_count{0U};
    bool eviction_review_required{false};
    bool committed{false};
    bool invoked_package_io{false};
    bool invoked_threading{false};
    bool invoked_native_handle{false};
    bool invoked_automatic_lru_eviction{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeSandboxWorldStreamingSafePointDesc {
    RuntimeSandboxWorldStreamingPlan plan;
    std::string target_id;
    std::string runtime_scene_validation_target_id;
    RuntimePackageMountOverlay overlay{RuntimePackageMountOverlay::last_mount_wins};
    RuntimeResourceResidencyBudgetV2 budget;
    std::uint32_t max_resident_packages{0U};
    bool safe_point_required{true};
    bool runtime_scene_validation_succeeded{false};
    std::vector<RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order;
    std::vector<RuntimeResidentPackageMountIdV2> protected_mount_ids;
};

struct RuntimeSandboxWorldStreamingSafePointResult {
    RuntimeSandboxWorldStreamingSafePointStatus status{RuntimeSandboxWorldStreamingSafePointStatus::invalid_plan};
    std::vector<RuntimeSandboxWorldStreamingDiagnostic> diagnostics;
    RuntimeWorldRegionStreamingSafePointResult world_region_result;
    RuntimeAddressableContentStreamingPlan addressable_plan;
    bool committed{false};
    bool invoked_package_io{false};
    bool invoked_async_execution{false};
    bool invoked_threading{false};
    bool invoked_native_handle{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans sandbox chunk streaming rows from explicit source ranges, dirty-chunk pins, reviewed world-region package
/// candidates, and addressable dependency rows. This value-only helper does not read packages, mutate resident mounts,
/// execute addressable work, start background streaming, upload renderer resources, or expose native handles.
[[nodiscard]] RuntimeSandboxWorldStreamingPlan
plan_runtime_sandbox_world_streaming(const RuntimeSandboxWorldStreamingPlanRequest& request);

/// Executes the reviewed world-region portion of a sandbox streaming plan at a host safe point. Addressable rows remain
/// value evidence in this phase. Live resident package state is updated only through the existing world-region
/// safe-point transaction, which runs on projected mount/catalog state before committing.
[[nodiscard]] RuntimeSandboxWorldStreamingSafePointResult
execute_runtime_sandbox_world_streaming_safe_point(IFileSystem& filesystem, RuntimeResidentPackageMountSetV2& mount_set,
                                                   RuntimeResidentCatalogCacheV2& catalog_cache,
                                                   const RuntimeSandboxWorldStreamingSafePointDesc& desc);

} // namespace mirakana::runtime
