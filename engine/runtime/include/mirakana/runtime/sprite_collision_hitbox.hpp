// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/gameplay_interaction.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeSpriteCollisionBoxKind : std::uint8_t {
    hitbox,
    hurtbox,
};

struct RuntimeSpriteCollisionBoxDesc {
    std::string id;
    std::string frame_id;
    std::string entity_id;
    RuntimeSpriteCollisionBoxKind kind{RuntimeSpriteCollisionBoxKind::hitbox};
    float center_x{0.0F};
    float center_y{0.0F};
    float half_width{0.5F};
    float half_height{0.5F};
    std::uint32_t layer{1U};
    std::uint32_t mask{0xFFFF'FFFFU};
    RuntimeGameplayInteractionKind gameplay_kind{RuntimeGameplayInteractionKind::damage};
    int gameplay_amount{1};
    std::string gameplay_feedback_id;
    std::size_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSpriteCollisionBoxDesc&) const = default;
};

struct RuntimeSpriteCollisionFramePoseDesc {
    std::string frame_id;
    std::string entity_id;
    float world_x{0.0F};
    float world_y{0.0F};
    bool active{true};
    std::size_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSpriteCollisionFramePoseDesc&) const = default;
};

struct RuntimeSpriteCollisionHitboxRequest {
    std::vector<RuntimeSpriteCollisionBoxDesc> boxes;
    std::vector<RuntimeSpriteCollisionFramePoseDesc> frame_poses;
    std::size_t max_hit_rows{std::numeric_limits<std::size_t>::max()};

    [[nodiscard]] bool operator==(const RuntimeSpriteCollisionHitboxRequest&) const = default;
};

struct RuntimeSpriteCollisionHitRow {
    std::string hitbox_id;
    std::string hurtbox_id;
    std::string hitbox_frame_id;
    std::string hurtbox_frame_id;
    std::string source_entity_id;
    std::string target_entity_id;
    std::string gameplay_event_id;
    std::uint32_t hitbox_layer{0U};
    std::uint32_t hurtbox_layer{0U};
    std::size_t hitbox_source_index{0U};
    std::size_t hurtbox_source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSpriteCollisionHitRow&) const = default;
};

enum class RuntimeSpriteCollisionDiagnosticCode : std::uint8_t {
    none,
    invalid_box_id,
    duplicate_box_id,
    invalid_frame_id,
    invalid_entity_id,
    invalid_box_bounds,
    invalid_layer_mask,
    missing_frame_pose,
    invalid_frame_pose,
    duplicate_frame_pose,
    hit_budget_exceeded,
};

struct RuntimeSpriteCollisionDiagnostic {
    RuntimeSpriteCollisionDiagnosticCode code{RuntimeSpriteCollisionDiagnosticCode::none};
    std::string box_id;
    std::string other_box_id;
    std::string frame_id;
    std::string entity_id;
    std::size_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSpriteCollisionDiagnostic&) const = default;
};

struct RuntimeSpriteCollisionHitboxPlan {
    bool succeeded{true};
    std::vector<RuntimeSpriteCollisionHitRow> rows;
    std::vector<RuntimeGameplayInteractionEvent> gameplay_events;
    std::vector<RuntimeSpriteCollisionDiagnostic> diagnostics;

    [[nodiscard]] bool operator==(const RuntimeSpriteCollisionHitboxPlan&) const = default;
};

[[nodiscard]] RuntimeSpriteCollisionHitboxPlan
plan_runtime_sprite_collision_hitboxes(const RuntimeSpriteCollisionHitboxRequest& request);

} // namespace mirakana::runtime
