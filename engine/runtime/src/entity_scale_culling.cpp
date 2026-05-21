// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/entity_scale_culling.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool is_valid_entity_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) {
    return std::ranges::any_of(ids, [id](const std::string& candidate) { return candidate == id; });
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
                RuntimeEntityScaleCullingDecision decision) {
    const auto visible = decision == RuntimeEntityScaleCullingDecision::visible;
    plan.rows.push_back(RuntimeEntityScaleCullingRow{
        .entity_id = entity.entity_id,
        .decision = decision,
        .update_bucket = entity.update_bucket,
        .bounds_kind = entity.bounds_kind,
        .layer_mask = entity.layer_mask,
        .source_index = entity.source_index,
        .visible = visible,
    });

    if (visible) {
        ++plan.projected_visible_count;
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
        append_row(plan, entity, decide_entity(entity, request.view));
    }
    sort_rows(plan);

    if (request.view.max_visible_entities > 0U && plan.projected_visible_count > request.view.max_visible_entities) {
        plan.rows.clear();
        add_diagnostic(plan, RuntimeEntityScaleCullingDiagnosticCode::visible_entity_budget_exceeded, {},
                       "projected visible entity count exceeds the configured visible entity budget");
        plan.status = RuntimeEntityScaleCullingPlanStatus::budget_exceeded;
        return plan;
    }

    plan.status = plan.rows.empty() ? RuntimeEntityScaleCullingPlanStatus::no_entities
                                    : RuntimeEntityScaleCullingPlanStatus::planned;
    return plan;
}

} // namespace mirakana::runtime
