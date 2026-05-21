// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeEntityScaleCullingBoundsKind : std::uint8_t {
    aabb_2d = 0,
    aabb_3d,
};

enum class RuntimeEntityScaleCullingDecision : std::uint8_t {
    visible = 0,
    culled_layer_mask,
    culled_outside_view,
    culled_disabled,
};

enum class RuntimeEntityScaleCullingUpdateBucket : std::uint8_t {
    background = 0,
    normal,
    priority,
};

enum class RuntimeEntityScaleCullingPlanStatus : std::uint8_t {
    planned = 0,
    no_entities,
    invalid_request,
    budget_exceeded,
};

enum class RuntimeEntityScaleCullingDiagnosticCode : std::uint8_t {
    missing_entity_id = 0,
    duplicate_entity_id,
    invalid_entity_bounds,
    zero_layer_mask,
    invalid_view_bounds,
    visible_entity_budget_exceeded,
};

struct RuntimeEntityScaleCullingBounds2D {
    float min_x{0.0F};
    float min_y{0.0F};
    float max_x{0.0F};
    float max_y{0.0F};
};

struct RuntimeEntityScaleCullingBounds3D {
    float min_x{0.0F};
    float min_y{0.0F};
    float min_z{0.0F};
    float max_x{0.0F};
    float max_y{0.0F};
    float max_z{0.0F};
};

struct RuntimeEntityScaleCullingEntityDesc {
    std::string entity_id;
    RuntimeEntityScaleCullingBoundsKind bounds_kind{RuntimeEntityScaleCullingBoundsKind::aabb_2d};
    RuntimeEntityScaleCullingBounds2D bounds_2d;
    RuntimeEntityScaleCullingBounds3D bounds_3d;
    std::uint32_t layer_mask{0U};
    RuntimeEntityScaleCullingUpdateBucket update_bucket{RuntimeEntityScaleCullingUpdateBucket::normal};
    bool enabled{true};
    std::uint32_t source_index{0U};
};

struct RuntimeEntityScaleCullingViewDesc {
    RuntimeEntityScaleCullingBoundsKind bounds_kind{RuntimeEntityScaleCullingBoundsKind::aabb_2d};
    RuntimeEntityScaleCullingBounds2D bounds_2d;
    RuntimeEntityScaleCullingBounds3D bounds_3d;
    std::uint32_t layer_mask{0xFFFF'FFFFU};
    std::size_t max_visible_entities{0U};
};

struct RuntimeEntityScaleCullingRequest {
    std::vector<RuntimeEntityScaleCullingEntityDesc> entities;
    RuntimeEntityScaleCullingViewDesc view;
};

struct RuntimeEntityScaleCullingDiagnostic {
    RuntimeEntityScaleCullingDiagnosticCode code{RuntimeEntityScaleCullingDiagnosticCode::missing_entity_id};
    std::string entity_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeEntityScaleCullingRow {
    std::string entity_id;
    RuntimeEntityScaleCullingDecision decision{RuntimeEntityScaleCullingDecision::visible};
    RuntimeEntityScaleCullingUpdateBucket update_bucket{RuntimeEntityScaleCullingUpdateBucket::normal};
    RuntimeEntityScaleCullingBoundsKind bounds_kind{RuntimeEntityScaleCullingBoundsKind::aabb_2d};
    std::uint32_t layer_mask{0U};
    std::uint32_t source_index{0U};
    bool visible{false};
};

struct RuntimeEntityScaleCullingPlan {
    RuntimeEntityScaleCullingPlanStatus status{RuntimeEntityScaleCullingPlanStatus::invalid_request};
    std::vector<RuntimeEntityScaleCullingDiagnostic> diagnostics;
    std::vector<RuntimeEntityScaleCullingRow> rows;
    std::size_t projected_visible_count{0U};
    std::size_t projected_culled_count{0U};
    std::size_t projected_disabled_count{0U};
    std::size_t projected_layer_culled_count{0U};
    std::size_t projected_outside_view_culled_count{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans deterministic, value-only entity visibility/update bucket rows for generated gameplay systems.
/// This helper does not mutate scene nodes or components, own renderer/RHI resources, perform GPU/occlusion culling,
/// upload buffers, launch background workers, read packages, or touch native handles.
[[nodiscard]] RuntimeEntityScaleCullingPlan
plan_runtime_entity_scale_culling(const RuntimeEntityScaleCullingRequest& request);

} // namespace mirakana::runtime
