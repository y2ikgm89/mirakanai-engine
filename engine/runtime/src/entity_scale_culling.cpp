// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/entity_scale_culling.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

namespace mirakana::runtime {
namespace {

struct RuntimeEntityScaleCullingSelectedLod {
    std::uint32_t lod_index{0U};
    RuntimeEntityScaleCullingDrawIntentKind draw_intent{RuntimeEntityScaleCullingDrawIntentKind::none};
    std::uint32_t draw_cost{0U};
    std::uint32_t update_cost{0U};
    std::uint32_t update_interval_frames{0U};
};

[[nodiscard]] bool is_valid_entity_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) {
    return std::ranges::any_of(ids, [id](const std::string& candidate) { return candidate == id; });
}

[[nodiscard]] bool contains_lod_index(const std::vector<std::uint32_t>& indexes, std::uint32_t index) {
    return std::ranges::any_of(indexes, [index](std::uint32_t candidate) { return candidate == index; });
}

[[nodiscard]] bool is_valid_range(float min_value, float max_value) {
    return std::isfinite(min_value) && std::isfinite(max_value) && min_value <= max_value;
}

[[nodiscard]] bool is_valid_bounds(const RuntimeEntityScaleCullingBounds2D& bounds) {
    return is_valid_range(bounds.min_x, bounds.max_x) && is_valid_range(bounds.min_y, bounds.max_y);
}

[[nodiscard]] bool is_valid_bounds(const RuntimeEntityScaleCullingBounds3D& bounds) {
    return is_valid_range(bounds.min_x, bounds.max_x) && is_valid_range(bounds.min_y, bounds.max_y) &&
           is_valid_range(bounds.min_z, bounds.max_z);
}

[[nodiscard]] bool is_valid_entity_bounds(const RuntimeEntityScaleCullingEntityDesc& entity) {
    switch (entity.bounds_kind) {
    case RuntimeEntityScaleCullingBoundsKind::aabb_2d:
        return is_valid_bounds(entity.bounds_2d);
    case RuntimeEntityScaleCullingBoundsKind::aabb_3d:
        return is_valid_bounds(entity.bounds_3d);
    }
    return false;
}

[[nodiscard]] bool is_valid_view_bounds(const RuntimeEntityScaleCullingViewDesc& view) {
    switch (view.bounds_kind) {
    case RuntimeEntityScaleCullingBoundsKind::aabb_2d:
        return is_valid_bounds(view.bounds_2d);
    case RuntimeEntityScaleCullingBoundsKind::aabb_3d:
        return is_valid_bounds(view.bounds_3d);
    }
    return false;
}

[[nodiscard]] bool is_valid_draw_intent(RuntimeEntityScaleCullingDrawIntentKind intent) {
    switch (intent) {
    case RuntimeEntityScaleCullingDrawIntentKind::none:
    case RuntimeEntityScaleCullingDrawIntentKind::sprite_2d:
    case RuntimeEntityScaleCullingDrawIntentKind::mesh_3d:
    case RuntimeEntityScaleCullingDrawIntentKind::custom:
        return true;
    }
    return false;
}

[[nodiscard]] RuntimeEntityScaleCullingBounds2D project_to_2d(const RuntimeEntityScaleCullingEntityDesc& entity) {
    if (entity.bounds_kind == RuntimeEntityScaleCullingBoundsKind::aabb_2d) {
        return entity.bounds_2d;
    }
    return RuntimeEntityScaleCullingBounds2D{
        .min_x = entity.bounds_3d.min_x,
        .min_y = entity.bounds_3d.min_y,
        .max_x = entity.bounds_3d.max_x,
        .max_y = entity.bounds_3d.max_y,
    };
}

[[nodiscard]] RuntimeEntityScaleCullingBounds3D project_to_3d(const RuntimeEntityScaleCullingEntityDesc& entity) {
    if (entity.bounds_kind == RuntimeEntityScaleCullingBoundsKind::aabb_3d) {
        return entity.bounds_3d;
    }
    return RuntimeEntityScaleCullingBounds3D{
        .min_x = entity.bounds_2d.min_x,
        .min_y = entity.bounds_2d.min_y,
        .min_z = 0.0F,
        .max_x = entity.bounds_2d.max_x,
        .max_y = entity.bounds_2d.max_y,
        .max_z = 0.0F,
    };
}

[[nodiscard]] bool intersects(const RuntimeEntityScaleCullingBounds2D& lhs,
                              const RuntimeEntityScaleCullingBounds2D& rhs) {
    return lhs.min_x <= rhs.max_x && lhs.max_x >= rhs.min_x && lhs.min_y <= rhs.max_y && lhs.max_y >= rhs.min_y;
}

[[nodiscard]] bool intersects(const RuntimeEntityScaleCullingBounds3D& lhs,
                              const RuntimeEntityScaleCullingBounds3D& rhs) {
    return lhs.min_x <= rhs.max_x && lhs.max_x >= rhs.min_x && lhs.min_y <= rhs.max_y && lhs.max_y >= rhs.min_y &&
           lhs.min_z <= rhs.max_z && lhs.max_z >= rhs.min_z;
}

[[nodiscard]] bool intersects_view(const RuntimeEntityScaleCullingEntityDesc& entity,
                                   const RuntimeEntityScaleCullingViewDesc& view) {
    if (view.bounds_kind == RuntimeEntityScaleCullingBoundsKind::aabb_2d) {
        return intersects(project_to_2d(entity), view.bounds_2d);
    }
    return intersects(project_to_3d(entity), view.bounds_3d);
}

[[nodiscard]] float center_x(const RuntimeEntityScaleCullingBounds2D& bounds) {
    return (bounds.min_x + bounds.max_x) * 0.5F;
}

[[nodiscard]] float center_y(const RuntimeEntityScaleCullingBounds2D& bounds) {
    return (bounds.min_y + bounds.max_y) * 0.5F;
}

[[nodiscard]] float center_x(const RuntimeEntityScaleCullingBounds3D& bounds) {
    return (bounds.min_x + bounds.max_x) * 0.5F;
}

[[nodiscard]] float center_y(const RuntimeEntityScaleCullingBounds3D& bounds) {
    return (bounds.min_y + bounds.max_y) * 0.5F;
}

[[nodiscard]] float center_z(const RuntimeEntityScaleCullingBounds3D& bounds) {
    return (bounds.min_z + bounds.max_z) * 0.5F;
}

[[nodiscard]] float view_distance(const RuntimeEntityScaleCullingEntityDesc& entity,
                                  const RuntimeEntityScaleCullingViewDesc& view) {
    if (view.bounds_kind == RuntimeEntityScaleCullingBoundsKind::aabb_2d) {
        const auto entity_bounds = project_to_2d(entity);
        const auto dx = center_x(entity_bounds) - center_x(view.bounds_2d);
        const auto dy = center_y(entity_bounds) - center_y(view.bounds_2d);
        return std::sqrt((dx * dx) + (dy * dy));
    }

    const auto entity_bounds = project_to_3d(entity);
    const auto dx = center_x(entity_bounds) - center_x(view.bounds_3d);
    const auto dy = center_y(entity_bounds) - center_y(view.bounds_3d);
    const auto dz = center_z(entity_bounds) - center_z(view.bounds_3d);
    return std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

[[nodiscard]] RuntimeEntityScaleCullingDrawIntentKind
default_draw_intent(RuntimeEntityScaleCullingBoundsKind bounds_kind) {
    switch (bounds_kind) {
    case RuntimeEntityScaleCullingBoundsKind::aabb_2d:
        return RuntimeEntityScaleCullingDrawIntentKind::sprite_2d;
    case RuntimeEntityScaleCullingBoundsKind::aabb_3d:
        return RuntimeEntityScaleCullingDrawIntentKind::mesh_3d;
    }
    return RuntimeEntityScaleCullingDrawIntentKind::custom;
}

[[nodiscard]] bool lod_band_less(const RuntimeEntityScaleCullingLodBandDesc* lhs,
                                 const RuntimeEntityScaleCullingLodBandDesc* rhs) {
    if (lhs->max_view_distance != rhs->max_view_distance) {
        return lhs->max_view_distance < rhs->max_view_distance;
    }
    return lhs->lod_index < rhs->lod_index;
}

[[nodiscard]] RuntimeEntityScaleCullingSelectedLod select_lod(const RuntimeEntityScaleCullingEntityDesc& entity,
                                                              const RuntimeEntityScaleCullingViewDesc& view) {
    if (entity.lod_bands.empty()) {
        return RuntimeEntityScaleCullingSelectedLod{
            .lod_index = 0U,
            .draw_intent = default_draw_intent(entity.bounds_kind),
            .draw_cost = 1U,
            .update_cost = 1U,
            .update_interval_frames = 1U,
        };
    }

    std::vector<const RuntimeEntityScaleCullingLodBandDesc*> sorted_bands;
    sorted_bands.reserve(entity.lod_bands.size());
    for (const auto& band : entity.lod_bands) {
        sorted_bands.push_back(&band);
    }
    std::ranges::sort(sorted_bands, lod_band_less);

    const auto distance = view_distance(entity, view);
    const RuntimeEntityScaleCullingLodBandDesc* selected = sorted_bands.back();
    for (const auto* band : sorted_bands) {
        if (distance <= band->max_view_distance) {
            selected = band;
            break;
        }
    }
    return RuntimeEntityScaleCullingSelectedLod{
        .lod_index = selected->lod_index,
        .draw_intent = selected->draw_intent,
        .draw_cost = selected->draw_cost,
        .update_cost = selected->update_cost,
        .update_interval_frames = selected->update_interval_frames,
    };
}

void add_diagnostic(RuntimeEntityScaleCullingPlan& plan, RuntimeEntityScaleCullingDiagnosticCode code,
                    std::string entity_id, std::string message, std::uint32_t source_index = 0U) {
    plan.diagnostics.push_back(RuntimeEntityScaleCullingDiagnostic{
        .code = code,
        .entity_id = std::move(entity_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void validate_request(RuntimeEntityScaleCullingPlan& plan, const RuntimeEntityScaleCullingRequest& request) {
    if (!is_valid_view_bounds(request.view)) {
        add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::invalid_view_bounds, {},
                       "entity scale culling view bounds must be finite and ordered");
    }

    std::vector<std::string> seen;
    seen.reserve(request.entities.size());
    for (const auto& entity : request.entities) {
        if (!is_valid_entity_id(entity.entity_id)) {
            add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::missing_entity_id, entity.entity_id,
                           "entity scale culling entity id must be non-empty and path-safe", entity.source_index);
        } else if (contains_id(seen, entity.entity_id)) {
            add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::duplicate_entity_id, entity.entity_id,
                           "entity scale culling request contains a duplicate entity id", entity.source_index);
        } else {
            seen.push_back(entity.entity_id);
        }
        if (!is_valid_entity_bounds(entity)) {
            add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::invalid_entity_bounds, entity.entity_id,
                           "entity scale culling entity bounds must be finite and ordered", entity.source_index);
        }
        if (entity.layer_mask == 0U) {
            add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::zero_layer_mask, entity.entity_id,
                           "entity scale culling entity layer mask must be non-zero", entity.source_index);
        }

        std::vector<std::uint32_t> seen_lod_indexes;
        seen_lod_indexes.reserve(entity.lod_bands.size());
        for (const auto& band : entity.lod_bands) {
            const auto invalid_lod = !std::isfinite(band.max_view_distance) || band.max_view_distance < 0.0F ||
                                     band.draw_cost == 0U || band.update_cost == 0U ||
                                     band.update_interval_frames == 0U || !is_valid_draw_intent(band.draw_intent);
            if (invalid_lod) {
                add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::invalid_lod_band, entity.entity_id,
                               "entity scale culling LOD bands must have finite non-negative distance and non-zero "
                               "draw/update costs and interval",
                               entity.source_index);
                continue;
            }
            if (contains_lod_index(seen_lod_indexes, band.lod_index)) {
                add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::invalid_lod_band, entity.entity_id,
                               "entity scale culling LOD band indexes must be unique per entity", entity.source_index);
                continue;
            }
            seen_lod_indexes.push_back(band.lod_index);
        }
    }
}

[[nodiscard]] RuntimeEntityScaleCullingDecision decide_entity(const RuntimeEntityScaleCullingEntityDesc& entity,
                                                              const RuntimeEntityScaleCullingViewDesc& view) {
    if (!entity.enabled) {
        return RuntimeEntityScaleCullingDecision::culled_disabled;
    }
    if ((entity.layer_mask & view.layer_mask) == 0U) {
        return RuntimeEntityScaleCullingDecision::culled_layer_mask;
    }
    if (!intersects_view(entity, view)) {
        return RuntimeEntityScaleCullingDecision::culled_outside_view;
    }
    return RuntimeEntityScaleCullingDecision::visible;
}

void append_row(RuntimeEntityScaleCullingPlan& plan, const RuntimeEntityScaleCullingEntityDesc& entity,
                const RuntimeEntityScaleCullingViewDesc& view, RuntimeEntityScaleCullingDecision decision) {
    const auto visible = decision == RuntimeEntityScaleCullingDecision::visible;
    const auto lod = visible ? select_lod(entity, view) : RuntimeEntityScaleCullingSelectedLod{};
    plan.rows.push_back(RuntimeEntityScaleCullingRow{
        .entity_id = entity.entity_id,
        .decision = decision,
        .update_bucket = entity.update_bucket,
        .bounds_kind = entity.bounds_kind,
        .layer_mask = entity.layer_mask,
        .source_index = entity.source_index,
        .lod_index = lod.lod_index,
        .draw_intent = lod.draw_intent,
        .projected_draw_cost = lod.draw_cost,
        .projected_update_cost = lod.update_cost,
        .update_interval_frames = lod.update_interval_frames,
        .budget_protected = entity.budget_protected,
        .visible = visible,
    });

    if (visible) {
        ++plan.projected_visible_count;
        plan.projected_draw_cost += lod.draw_cost;
        plan.projected_update_cost += lod.update_cost;
        if (entity.budget_protected) {
            ++plan.projected_protected_visible_count;
        }
        return;
    }

    ++plan.projected_culled_count;
    switch (decision) {
    case RuntimeEntityScaleCullingDecision::visible:
        break;
    case RuntimeEntityScaleCullingDecision::culled_layer_mask:
        ++plan.projected_layer_culled_count;
        break;
    case RuntimeEntityScaleCullingDecision::culled_outside_view:
        ++plan.projected_outside_view_culled_count;
        break;
    case RuntimeEntityScaleCullingDecision::culled_disabled:
        ++plan.projected_disabled_count;
        break;
    }
}

void sort_rows(RuntimeEntityScaleCullingPlan& plan) {
    std::ranges::sort(plan.rows, [](const RuntimeEntityScaleCullingRow& lhs, const RuntimeEntityScaleCullingRow& rhs) {
        if (lhs.entity_id != rhs.entity_id) {
            return lhs.entity_id < rhs.entity_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

} // namespace

bool RuntimeEntityScaleCullingPlan::succeeded() const noexcept {
    return status == RuntimeEntityScaleCullingPlanStatus::planned ||
           status == RuntimeEntityScaleCullingPlanStatus::no_entities;
}

RuntimeEntityScaleCullingPlan plan_runtime_entity_scale_culling(const RuntimeEntityScaleCullingRequest& request) {
    RuntimeEntityScaleCullingPlan plan;

    validate_request(plan, request);
    if (!plan.diagnostics.empty()) {
        plan.status = RuntimeEntityScaleCullingPlanStatus::invalid_request;
        return plan;
    }

    plan.rows.reserve(request.entities.size());
    for (const auto& entity : request.entities) {
        append_row(plan, entity, request.view, decide_entity(entity, request.view));
    }
    sort_rows(plan);

    if (request.view.max_visible_entities > 0U && plan.projected_visible_count > request.view.max_visible_entities) {
        add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::visible_entity_budget_exceeded, {},
                       "projected visible entity count exceeds the configured visible entity budget");
    }
    if (request.view.max_projected_draw_cost > 0U && plan.projected_draw_cost > request.view.max_projected_draw_cost) {
        add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::draw_budget_exceeded, {},
                       "projected entity draw cost exceeds the configured draw cost budget");
    }
    if (request.view.max_projected_update_cost > 0U &&
        plan.projected_update_cost > request.view.max_projected_update_cost) {
        add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::update_budget_exceeded, {},
                       "projected entity update cost exceeds the configured update cost budget");
    }
    if (!plan.diagnostics.empty()) {
        plan.rows.clear();
        plan.status = RuntimeEntityScaleCullingPlanStatus::budget_exceeded;
        return plan;
    }

    plan.status = plan.rows.empty() ? RuntimeEntityScaleCullingPlanStatus::no_entities
                                    : RuntimeEntityScaleCullingPlanStatus::planned;
    return plan;
}

} // namespace mirakana::runtime
