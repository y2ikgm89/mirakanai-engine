// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace mirakana {

struct PhysicsBody2DId {
    std::uint32_t value{0};

    friend constexpr bool operator==(PhysicsBody2DId lhs, PhysicsBody2DId rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

inline constexpr PhysicsBody2DId null_physics_body_2d{};

enum class PhysicsShape2DKind : std::uint8_t { aabb, circle };

struct PhysicsWorld2DConfig {
    Vec2 gravity{.x = 0.0F, .y = -9.80665F};
};

struct PhysicsBody2DDesc {
    Vec2 position{.x = 0.0F, .y = 0.0F};
    Vec2 velocity{.x = 0.0F, .y = 0.0F};
    float mass{1.0F};
    float linear_damping{0.0F};
    bool dynamic{true};
    Vec2 half_extents{.x = 0.5F, .y = 0.5F};
    bool collision_enabled{true};
    PhysicsShape2DKind shape{PhysicsShape2DKind::aabb};
    float radius{0.5F};
    std::uint32_t collision_layer{1U};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    bool trigger{false};
};

struct PhysicsBody2D {
    PhysicsBody2DId id;
    Vec2 position{.x = 0.0F, .y = 0.0F};
    Vec2 velocity{.x = 0.0F, .y = 0.0F};
    Vec2 accumulated_force{.x = 0.0F, .y = 0.0F};
    float inverse_mass{1.0F};
    float linear_damping{0.0F};
    bool dynamic{true};
    Vec2 half_extents{.x = 0.5F, .y = 0.5F};
    bool collision_enabled{true};
    PhysicsShape2DKind shape{PhysicsShape2DKind::aabb};
    float radius{0.5F};
    std::uint32_t collision_layer{1U};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    bool trigger{false};
};

struct PhysicsBroadphasePair2D {
    PhysicsBody2DId first;
    PhysicsBody2DId second;
};

struct PhysicsContact2D {
    PhysicsBody2DId first;
    PhysicsBody2DId second;
    Vec2 normal{.x = 1.0F, .y = 0.0F};
    float penetration_depth{0.0F};
};

struct PhysicsContactSolver2DConfig {
    float restitution{0.0F};
    std::uint32_t iterations{1U};
};

struct PhysicsRaycast2DDesc {
    Vec2 origin{.x = 0.0F, .y = 0.0F};
    Vec2 direction{.x = 1.0F, .y = 0.0F};
    float max_distance{1.0F};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
};

struct PhysicsRaycastHit2D {
    PhysicsBody2DId body;
    Vec2 point{.x = 0.0F, .y = 0.0F};
    Vec2 normal{.x = 1.0F, .y = 0.0F};
    float distance{0.0F};
};

struct PhysicsTriggerOverlap2D {
    // Sorted by body id; first/second do not imply trigger/other roles.
    PhysicsBody2DId first;
    PhysicsBody2DId second;
};

// Conservative v0 query: sweeps the query shape's collision bounds, not an exact primitive.
struct PhysicsShapeSweep2DDesc {
    Vec2 origin{.x = 0.0F, .y = 0.0F};
    Vec2 direction{.x = 1.0F, .y = 0.0F};
    float max_distance{1.0F};
    PhysicsShape2DKind shape{PhysicsShape2DKind::aabb};
    Vec2 half_extents{.x = 0.5F, .y = 0.5F};
    float radius{0.5F};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    PhysicsBody2DId ignored_body{};
    bool include_triggers{true};
};

struct PhysicsShapeSweepHit2D {
    PhysicsBody2DId body;
    Vec2 position{.x = 0.0F, .y = 0.0F};
    Vec2 normal{.x = 1.0F, .y = 0.0F};
    float distance{0.0F};
    bool initial_overlap{false};
};

[[nodiscard]] bool is_valid_physics_body_desc(const PhysicsBody2DDesc& desc) noexcept;

class PhysicsWorld2D {
  public:
    explicit PhysicsWorld2D(PhysicsWorld2DConfig config = {});

    [[nodiscard]] PhysicsBody2DId create_body(const PhysicsBody2DDesc& desc);
    [[nodiscard]] PhysicsBody2D* find_body(PhysicsBody2DId id) noexcept;
    [[nodiscard]] const PhysicsBody2D* find_body(PhysicsBody2DId id) const noexcept;
    [[nodiscard]] const std::vector<PhysicsBody2D>& bodies() const noexcept;
    [[nodiscard]] std::vector<PhysicsBroadphasePair2D> broadphase_pairs() const;
    [[nodiscard]] std::vector<PhysicsContact2D> contacts() const;
    [[nodiscard]] std::vector<PhysicsTriggerOverlap2D> trigger_overlaps() const;
    [[nodiscard]] std::optional<PhysicsRaycastHit2D> raycast(PhysicsRaycast2DDesc desc) const;
    [[nodiscard]] std::optional<PhysicsShapeSweepHit2D> shape_sweep(PhysicsShapeSweep2DDesc desc) const;

    void apply_force(PhysicsBody2DId id, Vec2 force);
    void step(float delta_seconds);
    void resolve_contacts(float restitution = 0.0F);
    void resolve_contacts(PhysicsContactSolver2DConfig config);

  private:
    PhysicsWorld2DConfig config_;
    std::vector<PhysicsBody2D> bodies_;
};

} // namespace mirakana
