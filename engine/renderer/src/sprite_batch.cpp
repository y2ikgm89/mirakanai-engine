// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/sprite_batch.hpp"

#include <algorithm>
#include <cmath>

namespace mirakana {
namespace {

struct SpriteBatchKey {
    SpriteBatchKind kind{SpriteBatchKind::solid_color};
    AssetId atlas_page;
};

[[nodiscard]] bool operator==(SpriteBatchKey lhs, SpriteBatchKey rhs) noexcept {
    return lhs.kind == rhs.kind && lhs.atlas_page == rhs.atlas_page;
}

[[nodiscard]] bool valid_uv_rect(SpriteUvRect rect) noexcept {
    return std::isfinite(rect.u0) && std::isfinite(rect.v0) && std::isfinite(rect.u1) && std::isfinite(rect.v1) &&
           rect.u0 >= 0.0F && rect.v0 >= 0.0F && rect.u1 <= 1.0F && rect.v1 <= 1.0F && rect.u0 < rect.u1 &&
           rect.v0 < rect.v1;
}

[[nodiscard]] SpriteBatchKey classify_sprite(SpriteBatchPlan& plan, const SpriteCommand& sprite,
                                             std::uint64_t sprite_index, SpriteBatchPlanOptions options) {
    if (!sprite.texture.enabled) {
        if (options.require_atlas_backed_sprites) {
            plan.diagnostics.push_back(SpriteBatchDiagnostic{
                .code = SpriteBatchDiagnosticCode::untextured_sprite_disallowed,
                .sprite_index = sprite_index,
            });
        }
        return {};
    }

    if (sprite.texture.atlas_page.value == 0) {
        plan.diagnostics.push_back(SpriteBatchDiagnostic{
            .code = SpriteBatchDiagnosticCode::missing_texture_atlas,
            .sprite_index = sprite_index,
        });
        return {};
    }

    if (!valid_uv_rect(sprite.texture.uv_rect)) {
        plan.diagnostics.push_back(SpriteBatchDiagnostic{
            .code = SpriteBatchDiagnosticCode::invalid_uv_rect,
            .sprite_index = sprite_index,
        });
        return {};
    }

    ++plan.textured_sprite_count;
    return SpriteBatchKey{.kind = SpriteBatchKind::textured, .atlas_page = sprite.texture.atlas_page};
}

void append_or_extend_batch(SpriteBatchPlan& plan, SpriteBatchKey key, std::uint64_t sprite_index) {
    if (!plan.batches.empty()) {
        const auto& last = plan.batches.back();
        const auto last_key = SpriteBatchKey{.kind = last.kind, .atlas_page = last.atlas_page};
        if (last_key == key) {
            ++plan.batches.back().sprite_count;
            return;
        }
    }

    plan.batches.push_back(SpriteBatchRange{
        .kind = key.kind,
        .first_sprite = sprite_index,
        .sprite_count = 1,
        .atlas_page = key.atlas_page,
    });
}

[[nodiscard]] bool budget_desc_valid(SpriteBatchBudgetDesc budget) noexcept {
    return budget.max_sprites != 0 && budget.max_draws != 0;
}

[[nodiscard]] bool production_budget_desc_valid(SpriteBatchProductionBudgetDesc budget) noexcept {
    return budget.max_visible_sprites != 0 && budget.max_draw_rows != 0 && budget.max_instance_rows != 0 &&
           budget.max_upload_bytes != 0 && budget.max_atlas_pages != 0 && budget.max_material_lanes != 0 &&
           budget.max_cpu_frame_time_us != 0;
}

void add_budget_diagnostic(SpriteBatchBudgetProfile& profile, SpriteBatchBudgetLane lane,
                           SpriteBatchBudgetDiagnosticCode code) {
    profile.diagnostics.push_back(SpriteBatchBudgetDiagnostic{
        .code = code,
        .lane = lane,
    });
}

void add_production_diagnostic(SpriteBatchProductionThroughputPlan& plan,
                               SpriteBatchProductionThroughputDiagnosticCode code, std::string workload_id) {
    plan.diagnostics.push_back(SpriteBatchProductionThroughputDiagnostic{
        .code = code,
        .workload_id = std::move(workload_id),
    });
}

[[nodiscard]] bool production_draw_intent_less(const SpriteBatchProductionDrawIntentRow& lhs,
                                               const SpriteBatchProductionDrawIntentRow& rhs) noexcept {
    if (lhs.sorting_layer != rhs.sorting_layer) {
        return lhs.sorting_layer < rhs.sorting_layer;
    }
    if (lhs.material_lane != rhs.material_lane) {
        return lhs.material_lane < rhs.material_lane;
    }
    if (lhs.atlas_page.value != rhs.atlas_page.value) {
        return lhs.atlas_page.value < rhs.atlas_page.value;
    }
    return lhs.stable_order < rhs.stable_order;
}

[[nodiscard]] bool same_production_draw_group(const SpriteBatchProductionDrawGroupRow& lhs,
                                              const SpriteBatchProductionDrawIntentRow& rhs) noexcept {
    return lhs.sorting_layer == rhs.sorting_layer && lhs.material_lane == rhs.material_lane &&
           lhs.atlas_page == rhs.atlas_page;
}

[[nodiscard]] bool contains_atlas_page(const std::vector<AssetId>& pages, AssetId page) {
    return std::ranges::any_of(pages, [page](AssetId candidate) { return candidate == page; });
}

[[nodiscard]] bool contains_material_lane(const std::vector<std::uint32_t>& lanes, std::uint32_t lane) {
    return std::ranges::any_of(lanes, [lane](std::uint32_t candidate) { return candidate == lane; });
}

[[nodiscard]] bool has_retained_timing_artifact(const SpriteBatchProductionMeasurementDesc& measurement) {
    return measurement.host_measurement_available && !measurement.retained_trace_artifact_path.empty() &&
           !measurement.retained_profile_artifact_path.empty() && measurement.retained_artifact_hash != 0U;
}

void append_production_draw_groups(SpriteBatchProductionWorkloadRow& row,
                                   std::span<const SpriteBatchProductionDrawIntentRow> sorted_intents) {
    for (const auto& intent : sorted_intents) {
        if (!row.draw_groups.empty()) {
            auto& last_group = row.draw_groups.back();
            if (same_production_draw_group(last_group, intent)) {
                last_group.last_stable_order = intent.stable_order;
                ++last_group.instance_count;
                last_group.upload_bytes += intent.upload_bytes;
                continue;
            }
        }

        row.draw_groups.push_back(SpriteBatchProductionDrawGroupRow{
            .sorting_layer = intent.sorting_layer,
            .material_lane = intent.material_lane,
            .atlas_page = intent.atlas_page,
            .first_stable_order = intent.stable_order,
            .last_stable_order = intent.stable_order,
            .instance_count = 1U,
            .upload_bytes = intent.upload_bytes,
        });
    }
}

[[nodiscard]] SpriteBatchProductionWorkloadRow
build_production_workload_row(const SpriteBatchProductionWorkloadDesc& workload) {
    std::vector<SpriteBatchProductionDrawIntentRow> sorted_intents{workload.draw_intents.begin(),
                                                                   workload.draw_intents.end()};
    std::stable_sort(sorted_intents.begin(), sorted_intents.end(), production_draw_intent_less);

    SpriteBatchProductionWorkloadRow row{
        .workload_id = workload.workload_id,
        .kind = workload.kind,
        .logical_sprite_rows = workload.logical_sprite_rows,
        .visible_sprite_rows = static_cast<std::uint64_t>(workload.draw_intents.size()),
        .culled_sprite_rows = workload.culled_sprite_rows,
        .instance_rows = static_cast<std::uint64_t>(workload.draw_intents.size()),
        .timing_status = has_retained_timing_artifact(workload.measurement)
                             ? SpriteBatchProductionTimingStatus::measured
                             : SpriteBatchProductionTimingStatus::host_gated,
        .cpu_frame_time_us = workload.measurement.cpu_frame_time_us,
        .retained_trace_artifact_path = workload.measurement.retained_trace_artifact_path,
        .retained_profile_artifact_path = workload.measurement.retained_profile_artifact_path,
        .retained_artifact_hash = workload.measurement.retained_artifact_hash,
    };

    std::vector<AssetId> atlas_pages;
    std::vector<std::uint32_t> material_lanes;
    for (const auto& intent : sorted_intents) {
        row.upload_bytes += intent.upload_bytes;
        if (!contains_atlas_page(atlas_pages, intent.atlas_page)) {
            atlas_pages.push_back(intent.atlas_page);
        }
        if (!contains_material_lane(material_lanes, intent.material_lane)) {
            material_lanes.push_back(intent.material_lane);
        }
    }

    append_production_draw_groups(row, sorted_intents);
    row.draw_rows = static_cast<std::uint64_t>(row.draw_groups.size());
    row.atlas_page_count = static_cast<std::uint64_t>(atlas_pages.size());
    row.material_lane_count = static_cast<std::uint64_t>(material_lanes.size());
    return row;
}

void validate_production_workload(SpriteBatchProductionThroughputPlan& plan,
                                  const SpriteBatchProductionWorkloadDesc& workload,
                                  const SpriteBatchProductionWorkloadRow& row, bool& has_invalid_request,
                                  bool& has_budget_exceeded, bool& has_diagnostics) {
    if (workload.workload_id.empty()) {
        has_invalid_request = true;
        add_production_diagnostic(plan, SpriteBatchProductionThroughputDiagnosticCode::missing_workload_id,
                                  workload.workload_id);
    }
    if (!production_budget_desc_valid(workload.budgets)) {
        has_invalid_request = true;
        add_production_diagnostic(plan, SpriteBatchProductionThroughputDiagnosticCode::invalid_budget,
                                  workload.workload_id);
    }
    if (row.logical_sprite_rows < row.visible_sprite_rows + row.culled_sprite_rows) {
        has_invalid_request = true;
        add_production_diagnostic(plan, SpriteBatchProductionThroughputDiagnosticCode::logical_sprite_count_mismatch,
                                  workload.workload_id);
    }

    auto add_budget_exceeded = [&](SpriteBatchProductionThroughputDiagnosticCode code) {
        has_budget_exceeded = true;
        add_production_diagnostic(plan, code, workload.workload_id);
    };

    if (row.visible_sprite_rows > workload.budgets.max_visible_sprites) {
        add_budget_exceeded(SpriteBatchProductionThroughputDiagnosticCode::visible_sprite_budget_exceeded);
    }
    if (row.draw_rows > workload.budgets.max_draw_rows) {
        add_budget_exceeded(SpriteBatchProductionThroughputDiagnosticCode::draw_row_budget_exceeded);
    }
    if (row.instance_rows > workload.budgets.max_instance_rows) {
        add_budget_exceeded(SpriteBatchProductionThroughputDiagnosticCode::instance_row_budget_exceeded);
    }
    if (row.upload_bytes > workload.budgets.max_upload_bytes) {
        add_budget_exceeded(SpriteBatchProductionThroughputDiagnosticCode::upload_byte_budget_exceeded);
    }
    if (row.atlas_page_count > workload.budgets.max_atlas_pages) {
        add_budget_exceeded(SpriteBatchProductionThroughputDiagnosticCode::atlas_page_budget_exceeded);
    }
    if (row.material_lane_count > workload.budgets.max_material_lanes) {
        add_budget_exceeded(SpriteBatchProductionThroughputDiagnosticCode::material_lane_overflow);
    }
    if (workload.measurement.host_measurement_available &&
        row.cpu_frame_time_us > workload.budgets.max_cpu_frame_time_us) {
        add_budget_exceeded(SpriteBatchProductionThroughputDiagnosticCode::cpu_frame_budget_exceeded);
    }
    if (workload.measurement.host_measurement_available && !has_retained_timing_artifact(workload.measurement)) {
        has_diagnostics = true;
        add_production_diagnostic(plan, SpriteBatchProductionThroughputDiagnosticCode::missing_retained_timing_artifact,
                                  workload.workload_id);
    }
}

} // namespace

SpriteBatchPlan plan_sprite_batches(std::span<const SpriteCommand> sprites) {
    return plan_sprite_batches(SpriteBatchPlanDesc{.sprites = sprites});
}

SpriteBatchPlan plan_sprite_batches(const SpriteBatchPlanDesc& desc) {
    SpriteBatchPlan plan;
    plan.sprite_count = static_cast<std::uint64_t>(desc.sprites.size());

    if (desc.options.allow_sprite_reordering) {
        plan.diagnostics.push_back(SpriteBatchDiagnostic{
            .code = SpriteBatchDiagnosticCode::unsupported_reordering_policy,
            .sprite_index = 0,
        });
        return plan;
    }

    std::uint64_t sprite_index = 0;
    for (const auto& sprite : desc.sprites) {
        const auto key = classify_sprite(plan, sprite, sprite_index, desc.options);
        append_or_extend_batch(plan, key, sprite_index);
        ++sprite_index;
    }

    plan.draw_count = static_cast<std::uint64_t>(plan.batches.size());
    for (const auto& batch : plan.batches) {
        if (batch.kind == SpriteBatchKind::textured) {
            ++plan.texture_bind_count;
            ++plan.atlas_backed_batch_count;
            if (batch.sprite_count > 1) {
                ++plan.repeated_atlas_batch_count;
                plan.repeated_atlas_sprite_count += batch.sprite_count;
            }
        }
    }

    return plan;
}

SpriteBatchBudgetProfile plan_sprite_batch_budget_profile(std::span<const SpriteBatchBudgetLanePlanDesc> lanes) {
    SpriteBatchBudgetProfile profile;
    profile.rows.reserve(lanes.size());
    if (lanes.empty()) {
        profile.status = SpriteBatchBudgetProfileStatus::invalid_request;
        return profile;
    }

    bool has_invalid_request = false;
    bool has_plan_diagnostics = false;
    bool has_budget_exceeded = false;
    for (const auto& lane : lanes) {
        if (lane.plan == nullptr) {
            has_invalid_request = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::missing_plan);
            continue;
        }
        if (!budget_desc_valid(lane.budget)) {
            has_invalid_request = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::invalid_budget);
            continue;
        }

        const auto& plan = *lane.plan;
        const bool within_budget = plan.sprite_count <= lane.budget.max_sprites &&
                                   plan.draw_count <= lane.budget.max_draws &&
                                   plan.texture_bind_count <= lane.budget.max_texture_binds && plan.diagnostics.empty();
        profile.rows.push_back(SpriteBatchBudgetRow{
            .lane = lane.lane,
            .sprite_count = plan.sprite_count,
            .draw_count = plan.draw_count,
            .texture_bind_count = plan.texture_bind_count,
            .max_sprites = lane.budget.max_sprites,
            .max_draws = lane.budget.max_draws,
            .max_texture_binds = lane.budget.max_texture_binds,
            .within_budget = within_budget,
        });
        profile.total_sprites += plan.sprite_count;
        profile.total_draws += plan.draw_count;
        profile.total_texture_binds += plan.texture_bind_count;

        if (!plan.diagnostics.empty()) {
            has_plan_diagnostics = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::plan_diagnostics);
        }
        if (plan.sprite_count > lane.budget.max_sprites) {
            has_budget_exceeded = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::sprite_budget_exceeded);
        }
        if (plan.draw_count > lane.budget.max_draws) {
            has_budget_exceeded = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::draw_budget_exceeded);
        }
        if (plan.texture_bind_count > lane.budget.max_texture_binds) {
            has_budget_exceeded = true;
            add_budget_diagnostic(profile, lane.lane, SpriteBatchBudgetDiagnosticCode::texture_bind_budget_exceeded);
        }
    }

    if (has_invalid_request) {
        profile.status = SpriteBatchBudgetProfileStatus::invalid_request;
    } else if (has_plan_diagnostics) {
        profile.status = SpriteBatchBudgetProfileStatus::diagnostics;
    } else if (has_budget_exceeded) {
        profile.status = SpriteBatchBudgetProfileStatus::budget_exceeded;
    } else {
        profile.status = SpriteBatchBudgetProfileStatus::ready;
    }
    return profile;
}

SpriteBatchProductionThroughputPlan
plan_sprite_batch_production_throughput(const SpriteBatchProductionThroughputDesc& desc) {
    SpriteBatchProductionThroughputPlan plan;
    if (desc.workloads.empty()) {
        plan.status = SpriteBatchProductionThroughputStatus::invalid_request;
        add_production_diagnostic(plan, SpriteBatchProductionThroughputDiagnosticCode::missing_workload_rows, {});
        return plan;
    }

    plan.rows.reserve(desc.workloads.size());
    bool has_invalid_request = false;
    bool has_budget_exceeded = false;
    bool has_diagnostics = false;
    for (const auto& workload : desc.workloads) {
        auto row = build_production_workload_row(workload);
        validate_production_workload(plan, workload, row, has_invalid_request, has_budget_exceeded, has_diagnostics);
        row.over_budget = row.visible_sprite_rows > workload.budgets.max_visible_sprites ||
                          row.draw_rows > workload.budgets.max_draw_rows ||
                          row.instance_rows > workload.budgets.max_instance_rows ||
                          row.upload_bytes > workload.budgets.max_upload_bytes ||
                          row.atlas_page_count > workload.budgets.max_atlas_pages ||
                          row.material_lane_count > workload.budgets.max_material_lanes ||
                          (workload.measurement.host_measurement_available &&
                           row.cpu_frame_time_us > workload.budgets.max_cpu_frame_time_us);

        plan.total_draw_rows += row.draw_rows;
        plan.total_instance_rows += row.instance_rows;
        plan.total_upload_bytes += row.upload_bytes;
        if (row.timing_status == SpriteBatchProductionTimingStatus::measured) {
            ++plan.measured_workload_rows;
        } else {
            ++plan.host_gated_workload_rows;
        }
        plan.rows.push_back(std::move(row));
    }

    if (has_invalid_request) {
        plan.status = SpriteBatchProductionThroughputStatus::invalid_request;
    } else if (has_budget_exceeded) {
        plan.status = SpriteBatchProductionThroughputStatus::budget_exceeded;
    } else if (has_diagnostics) {
        plan.status = SpriteBatchProductionThroughputStatus::diagnostics;
    } else {
        plan.status = SpriteBatchProductionThroughputStatus::ready;
    }
    return plan;
}

} // namespace mirakana
