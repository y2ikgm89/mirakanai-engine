// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/sprite_collision_hitbox.hpp"

#include <cmath>
#include <set>
#include <string>
#include <utility>

namespace mirakana::runtime {
namespace {

struct SpriteCollisionAabb {
    float min_x{0.0F};
    float min_y{0.0F};
    float max_x{0.0F};
    float max_y{0.0F};
};

[[nodiscard]] bool finite(const float value) noexcept {
    return std::isfinite(value);
}

void append_diagnostic(std::vector<RuntimeSpriteCollisionDiagnostic>& diagnostics,
                       RuntimeSpriteCollisionDiagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] const RuntimeSpriteCollisionFramePoseDesc*
find_frame_pose(const std::vector<RuntimeSpriteCollisionFramePoseDesc>& poses,
                const RuntimeSpriteCollisionBoxDesc& box) noexcept {
    for (const auto& pose : poses) {
        if (pose.frame_id == box.frame_id && pose.entity_id == box.entity_id) {
            return &pose;
        }
    }
    return nullptr;
}

[[nodiscard]] bool valid_box_bounds(const RuntimeSpriteCollisionBoxDesc& box) noexcept {
    return finite(box.center_x) && finite(box.center_y) && finite(box.half_width) && finite(box.half_height) &&
           box.half_width > 0.0F && box.half_height > 0.0F;
}

void validate_duplicate_box_ids(const std::vector<RuntimeSpriteCollisionBoxDesc>& boxes,
                                std::vector<RuntimeSpriteCollisionDiagnostic>& diagnostics) {
    std::set<std::string> seen;
    for (const auto& box : boxes) {
        if (box.id.empty()) {
            continue;
        }
        if (!seen.insert(box.id).second) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::duplicate_box_id,
                                               .box_id = box.id,
                                               .other_box_id = {},
                                               .frame_id = box.frame_id,
                                               .entity_id = box.entity_id,
                                               .source_index = box.source_index,
                                           });
        }
    }
}

void validate_duplicate_frame_poses(const std::vector<RuntimeSpriteCollisionFramePoseDesc>& poses,
                                    std::vector<RuntimeSpriteCollisionDiagnostic>& diagnostics) {
    std::set<std::pair<std::string, std::string>> seen;
    for (const auto& pose : poses) {
        if (pose.frame_id.empty() || pose.entity_id.empty()) {
            continue;
        }
        const auto key = std::make_pair(pose.frame_id, pose.entity_id);
        if (!seen.insert(key).second) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::duplicate_frame_pose,
                                               .box_id = {},
                                               .other_box_id = {},
                                               .frame_id = pose.frame_id,
                                               .entity_id = pose.entity_id,
                                               .source_index = pose.source_index,
                                           });
        }
    }
}

void validate_frame_pose_rows(const std::vector<RuntimeSpriteCollisionFramePoseDesc>& poses,
                              std::vector<RuntimeSpriteCollisionDiagnostic>& diagnostics) {
    validate_duplicate_frame_poses(poses, diagnostics);
    for (const auto& pose : poses) {
        if (pose.frame_id.empty()) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::invalid_frame_id,
                                               .box_id = {},
                                               .other_box_id = {},
                                               .frame_id = pose.frame_id,
                                               .entity_id = pose.entity_id,
                                               .source_index = pose.source_index,
                                           });
        }
        if (pose.entity_id.empty()) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::invalid_entity_id,
                                               .box_id = {},
                                               .other_box_id = {},
                                               .frame_id = pose.frame_id,
                                               .entity_id = pose.entity_id,
                                               .source_index = pose.source_index,
                                           });
        }
        if (!finite(pose.world_x) || !finite(pose.world_y)) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::invalid_frame_pose,
                                               .box_id = {},
                                               .other_box_id = {},
                                               .frame_id = pose.frame_id,
                                               .entity_id = pose.entity_id,
                                               .source_index = pose.source_index,
                                           });
        }
    }
}

void validate_box_rows(const RuntimeSpriteCollisionHitboxRequest& request,
                       std::vector<RuntimeSpriteCollisionDiagnostic>& diagnostics) {
    validate_duplicate_box_ids(request.boxes, diagnostics);
    validate_frame_pose_rows(request.frame_poses, diagnostics);

    for (const auto& box : request.boxes) {
        if (box.id.empty()) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::invalid_box_id,
                                               .box_id = box.id,
                                               .other_box_id = {},
                                               .frame_id = box.frame_id,
                                               .entity_id = box.entity_id,
                                               .source_index = box.source_index,
                                           });
        }
        if (box.frame_id.empty()) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::invalid_frame_id,
                                               .box_id = box.id,
                                               .other_box_id = {},
                                               .frame_id = box.frame_id,
                                               .entity_id = box.entity_id,
                                               .source_index = box.source_index,
                                           });
        }
        if (box.entity_id.empty()) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::invalid_entity_id,
                                               .box_id = box.id,
                                               .other_box_id = {},
                                               .frame_id = box.frame_id,
                                               .entity_id = box.entity_id,
                                               .source_index = box.source_index,
                                           });
        }
        if (!valid_box_bounds(box)) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::invalid_box_bounds,
                                               .box_id = box.id,
                                               .other_box_id = {},
                                               .frame_id = box.frame_id,
                                               .entity_id = box.entity_id,
                                               .source_index = box.source_index,
                                           });
        }
        if (box.layer == 0U || box.mask == 0U) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::invalid_layer_mask,
                                               .box_id = box.id,
                                               .other_box_id = {},
                                               .frame_id = box.frame_id,
                                               .entity_id = box.entity_id,
                                               .source_index = box.source_index,
                                           });
        }
        if (!box.frame_id.empty() && !box.entity_id.empty() && find_frame_pose(request.frame_poses, box) == nullptr) {
            append_diagnostic(diagnostics, RuntimeSpriteCollisionDiagnostic{
                                               .code = RuntimeSpriteCollisionDiagnosticCode::missing_frame_pose,
                                               .box_id = box.id,
                                               .other_box_id = {},
                                               .frame_id = box.frame_id,
                                               .entity_id = box.entity_id,
                                               .source_index = box.source_index,
                                           });
        }
    }
}

[[nodiscard]] bool collision_filters_match(const RuntimeSpriteCollisionBoxDesc& hitbox,
                                           const RuntimeSpriteCollisionBoxDesc& hurtbox) noexcept {
    return (hitbox.mask & hurtbox.layer) != 0U && (hurtbox.mask & hitbox.layer) != 0U;
}

[[nodiscard]] SpriteCollisionAabb make_aabb(const RuntimeSpriteCollisionBoxDesc& box,
                                            const RuntimeSpriteCollisionFramePoseDesc& pose) noexcept {
    const auto center_x = pose.world_x + box.center_x;
    const auto center_y = pose.world_y + box.center_y;
    return SpriteCollisionAabb{
        .min_x = center_x - box.half_width,
        .min_y = center_y - box.half_height,
        .max_x = center_x + box.half_width,
        .max_y = center_y + box.half_height,
    };
}

[[nodiscard]] bool overlaps(const SpriteCollisionAabb& first, const SpriteCollisionAabb& second) noexcept {
    return first.min_x <= second.max_x && first.max_x >= second.min_x && first.min_y <= second.max_y &&
           first.max_y >= second.min_y;
}

[[nodiscard]] RuntimeGameplayInteractionEvent make_gameplay_event(const RuntimeSpriteCollisionBoxDesc& hitbox,
                                                                  const RuntimeSpriteCollisionBoxDesc& hurtbox,
                                                                  std::string event_id) {
    return RuntimeGameplayInteractionEvent{
        .id = std::move(event_id),
        .kind = hitbox.gameplay_kind,
        .source_entity_id = hitbox.entity_id,
        .target_entity_id = hurtbox.entity_id,
        .pickup_id = {},
        .objective_id = {},
        .feedback_id = hitbox.gameplay_feedback_id,
        .amount = hitbox.gameplay_amount,
    };
}

[[nodiscard]] std::string make_hit_event_id(const RuntimeSpriteCollisionBoxDesc& hitbox,
                                            const RuntimeSpriteCollisionBoxDesc& hurtbox) {
    return std::to_string(hitbox.id.size()) + ":" + hitbox.id + "|" + std::to_string(hurtbox.id.size()) + ":" +
           hurtbox.id;
}

} // namespace

RuntimeSpriteCollisionHitboxPlan
plan_runtime_sprite_collision_hitboxes(const RuntimeSpriteCollisionHitboxRequest& request) {
    RuntimeSpriteCollisionHitboxPlan plan;
    validate_box_rows(request, plan.diagnostics);
    if (!plan.diagnostics.empty()) {
        plan.succeeded = false;
        return plan;
    }

    for (const auto& hitbox : request.boxes) {
        if (hitbox.kind != RuntimeSpriteCollisionBoxKind::hitbox) {
            continue;
        }
        const auto* hitbox_pose = find_frame_pose(request.frame_poses, hitbox);
        if (hitbox_pose == nullptr || !hitbox_pose->active) {
            continue;
        }
        const auto hitbox_aabb = make_aabb(hitbox, *hitbox_pose);
        for (const auto& hurtbox : request.boxes) {
            if (hurtbox.kind != RuntimeSpriteCollisionBoxKind::hurtbox || !collision_filters_match(hitbox, hurtbox)) {
                continue;
            }
            const auto* hurtbox_pose = find_frame_pose(request.frame_poses, hurtbox);
            if (hurtbox_pose == nullptr || !hurtbox_pose->active) {
                continue;
            }
            if (!overlaps(hitbox_aabb, make_aabb(hurtbox, *hurtbox_pose))) {
                continue;
            }
            if (plan.rows.size() >= request.max_hit_rows) {
                plan.succeeded = false;
                plan.rows.clear();
                plan.gameplay_events.clear();
                append_diagnostic(plan.diagnostics,
                                  RuntimeSpriteCollisionDiagnostic{
                                      .code = RuntimeSpriteCollisionDiagnosticCode::hit_budget_exceeded,
                                      .box_id = hitbox.id,
                                      .other_box_id = hurtbox.id,
                                      .frame_id = hitbox.frame_id,
                                      .entity_id = hitbox.entity_id,
                                      .source_index = hitbox.source_index,
                                  });
                return plan;
            }

            const auto event_id = make_hit_event_id(hitbox, hurtbox);
            plan.rows.push_back(RuntimeSpriteCollisionHitRow{
                .hitbox_id = hitbox.id,
                .hurtbox_id = hurtbox.id,
                .hitbox_frame_id = hitbox.frame_id,
                .hurtbox_frame_id = hurtbox.frame_id,
                .source_entity_id = hitbox.entity_id,
                .target_entity_id = hurtbox.entity_id,
                .gameplay_event_id = event_id,
                .hitbox_layer = hitbox.layer,
                .hurtbox_layer = hurtbox.layer,
                .hitbox_source_index = hitbox.source_index,
                .hurtbox_source_index = hurtbox.source_index,
            });
            plan.gameplay_events.push_back(make_gameplay_event(hitbox, hurtbox, event_id));
        }
    }

    return plan;
}

} // namespace mirakana::runtime
