// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/renderer/renderer.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class SpriteBatchKind : std::uint8_t { solid_color, textured };

enum class SpriteBatchDiagnosticCode : std::uint8_t {
    none,
    missing_texture_atlas,
    invalid_uv_rect,
    unsupported_reordering_policy,
    untextured_sprite_disallowed,
};

struct SpriteBatchDiagnostic {
    SpriteBatchDiagnosticCode code{SpriteBatchDiagnosticCode::none};
    std::uint64_t sprite_index{0};
};

struct SpriteBatchRange {
    SpriteBatchKind kind{SpriteBatchKind::solid_color};
    std::uint64_t first_sprite{0};
    std::uint64_t sprite_count{0};
    AssetId atlas_page;
};

struct SpriteBatchPlanOptions {
    bool allow_sprite_reordering{false};
    bool require_atlas_backed_sprites{false};
};

struct SpriteBatchPlanDesc {
    std::span<const SpriteCommand> sprites;
    SpriteBatchPlanOptions options;
};

struct SpriteBatchPlan {
    std::vector<SpriteBatchRange> batches;
    std::vector<SpriteBatchDiagnostic> diagnostics;
    std::uint64_t sprite_count{0};
    std::uint64_t textured_sprite_count{0};
    std::uint64_t draw_count{0};
    std::uint64_t texture_bind_count{0};
    std::uint64_t atlas_backed_batch_count{0};
    std::uint64_t repeated_atlas_batch_count{0};
    std::uint64_t repeated_atlas_sprite_count{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

enum class SpriteBatchBudgetLane : std::uint8_t { world, ui, effects };

enum class SpriteBatchBudgetProfileStatus : std::uint8_t {
    ready,
    invalid_request,
    diagnostics,
    budget_exceeded,
};

enum class SpriteBatchBudgetDiagnosticCode : std::uint8_t {
    none,
    missing_plan,
    invalid_budget,
    plan_diagnostics,
    sprite_budget_exceeded,
    draw_budget_exceeded,
    texture_bind_budget_exceeded,
};

struct SpriteBatchBudgetDesc {
    std::uint64_t max_sprites{0};
    std::uint64_t max_draws{0};
    std::uint64_t max_texture_binds{0};
};

struct SpriteBatchBudgetLanePlanDesc {
    SpriteBatchBudgetLane lane{SpriteBatchBudgetLane::world};
    const SpriteBatchPlan* plan{nullptr};
    SpriteBatchBudgetDesc budget;
};

struct SpriteBatchBudgetRow {
    SpriteBatchBudgetLane lane{SpriteBatchBudgetLane::world};
    std::uint64_t sprite_count{0};
    std::uint64_t draw_count{0};
    std::uint64_t texture_bind_count{0};
    std::uint64_t max_sprites{0};
    std::uint64_t max_draws{0};
    std::uint64_t max_texture_binds{0};
    bool within_budget{false};
};

struct SpriteBatchBudgetDiagnostic {
    SpriteBatchBudgetDiagnosticCode code{SpriteBatchBudgetDiagnosticCode::none};
    SpriteBatchBudgetLane lane{SpriteBatchBudgetLane::world};
};

struct SpriteBatchBudgetProfile {
    SpriteBatchBudgetProfileStatus status{SpriteBatchBudgetProfileStatus::invalid_request};
    std::vector<SpriteBatchBudgetRow> rows;
    std::vector<SpriteBatchBudgetDiagnostic> diagnostics;
    std::uint64_t total_sprites{0};
    std::uint64_t total_draws{0};
    std::uint64_t total_texture_binds{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == SpriteBatchBudgetProfileStatus::ready && diagnostics.empty();
    }
};

enum class SpriteBatchProductionWorkloadKind : std::uint8_t {
    custom,
    dense_arena_512,
    dense_arena_4096,
    projectile_storm_12000,
};

enum class SpriteBatchProductionThroughputStatus : std::uint8_t {
    ready,
    invalid_request,
    diagnostics,
    budget_exceeded,
};

enum class SpriteBatchProductionTimingStatus : std::uint8_t {
    host_gated,
    measured,
};

enum class SpriteBatchProductionThroughputDiagnosticCode : std::uint8_t {
    none,
    missing_workload_rows,
    missing_workload_id,
    invalid_budget,
    logical_sprite_count_mismatch,
    visible_sprite_budget_exceeded,
    draw_row_budget_exceeded,
    instance_row_budget_exceeded,
    upload_byte_budget_exceeded,
    atlas_page_budget_exceeded,
    material_lane_overflow,
    cpu_frame_budget_exceeded,
    missing_retained_timing_artifact,
};

struct SpriteBatchProductionDrawIntentRow {
    std::string sprite_id;
    std::uint64_t stable_order{0};
    std::int32_t sorting_layer{0};
    std::uint32_t material_lane{0};
    AssetId atlas_page;
    std::uint64_t upload_bytes{0};
};

struct SpriteBatchProductionBudgetDesc {
    std::uint64_t max_visible_sprites{0};
    std::uint64_t max_draw_rows{0};
    std::uint64_t max_instance_rows{0};
    std::uint64_t max_upload_bytes{0};
    std::uint64_t max_atlas_pages{0};
    std::uint64_t max_material_lanes{0};
    std::uint64_t max_cpu_frame_time_us{0};
};

struct SpriteBatchProductionMeasurementDesc {
    bool host_measurement_available{false};
    std::uint64_t cpu_frame_time_us{0};
    std::string retained_trace_artifact_path;
    std::string retained_profile_artifact_path;
    std::uint64_t retained_artifact_hash{0};
};

struct SpriteBatchProductionWorkloadDesc {
    std::string workload_id;
    SpriteBatchProductionWorkloadKind kind{SpriteBatchProductionWorkloadKind::custom};
    std::span<const SpriteBatchProductionDrawIntentRow> draw_intents;
    std::uint64_t logical_sprite_rows{0};
    std::uint64_t culled_sprite_rows{0};
    SpriteBatchProductionBudgetDesc budgets;
    SpriteBatchProductionMeasurementDesc measurement;
};

struct SpriteBatchProductionThroughputDesc {
    std::span<const SpriteBatchProductionWorkloadDesc> workloads;
};

struct SpriteBatchProductionDrawGroupRow {
    std::int32_t sorting_layer{0};
    std::uint32_t material_lane{0};
    AssetId atlas_page;
    std::uint64_t first_stable_order{0};
    std::uint64_t last_stable_order{0};
    std::uint64_t instance_count{0};
    std::uint64_t upload_bytes{0};
};

struct SpriteBatchProductionWorkloadRow {
    std::string workload_id;
    SpriteBatchProductionWorkloadKind kind{SpriteBatchProductionWorkloadKind::custom};
    std::vector<SpriteBatchProductionDrawGroupRow> draw_groups;
    std::uint64_t logical_sprite_rows{0};
    std::uint64_t visible_sprite_rows{0};
    std::uint64_t culled_sprite_rows{0};
    std::uint64_t draw_rows{0};
    std::uint64_t instance_rows{0};
    std::uint64_t upload_bytes{0};
    std::uint64_t atlas_page_count{0};
    std::uint64_t material_lane_count{0};
    SpriteBatchProductionTimingStatus timing_status{SpriteBatchProductionTimingStatus::host_gated};
    std::uint64_t cpu_frame_time_us{0};
    std::string retained_trace_artifact_path;
    std::string retained_profile_artifact_path;
    std::uint64_t retained_artifact_hash{0};
    bool over_budget{false};
};

struct SpriteBatchProductionThroughputDiagnostic {
    SpriteBatchProductionThroughputDiagnosticCode code{SpriteBatchProductionThroughputDiagnosticCode::none};
    std::string workload_id;
};

struct SpriteBatchProductionThroughputPlan {
    SpriteBatchProductionThroughputStatus status{SpriteBatchProductionThroughputStatus::invalid_request};
    std::vector<SpriteBatchProductionWorkloadRow> rows;
    std::vector<SpriteBatchProductionThroughputDiagnostic> diagnostics;
    std::uint64_t total_draw_rows{0};
    std::uint64_t total_instance_rows{0};
    std::uint64_t total_upload_bytes{0};
    std::uint64_t measured_workload_rows{0};
    std::uint64_t host_gated_workload_rows{0};
    bool claimed_broad_optimization{false};
    bool claimed_cross_backend_parity{false};
    bool claimed_metal_readiness{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == SpriteBatchProductionThroughputStatus::ready && diagnostics.empty();
    }
};

[[nodiscard]] SpriteBatchPlan plan_sprite_batches(std::span<const SpriteCommand> sprites);
[[nodiscard]] SpriteBatchPlan plan_sprite_batches(const SpriteBatchPlanDesc& desc);
[[nodiscard]] SpriteBatchBudgetProfile
plan_sprite_batch_budget_profile(std::span<const SpriteBatchBudgetLanePlanDesc> lanes);
[[nodiscard]] SpriteBatchProductionThroughputPlan
plan_sprite_batch_production_throughput(const SpriteBatchProductionThroughputDesc& desc);

} // namespace mirakana
