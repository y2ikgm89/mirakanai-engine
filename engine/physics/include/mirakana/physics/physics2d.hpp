// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"
#include "mirakana/physics/collision_query.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
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

struct PhysicsRaycastBatch2DDesc {
    std::vector<PhysicsRaycast2DDesc> queries;
    // The default is intentionally unbounded; set a positive cap to fail closed on unexpected query counts.
    std::size_t max_queries{std::numeric_limits<std::size_t>::max()};
};

struct PhysicsRaycastBatch2DRow {
    std::size_t source_index{0};
    PhysicsCollisionQueryRowStatus status{PhysicsCollisionQueryRowStatus::invalid_request};
    PhysicsCollisionQueryRowDiagnostic diagnostic{PhysicsCollisionQueryRowDiagnostic::none};
    std::optional<PhysicsRaycastHit2D> hit;
};

struct PhysicsRaycastBatch2DResult {
    PhysicsCollisionQueryBatchStatus status{PhysicsCollisionQueryBatchStatus::invalid_request};
    PhysicsCollisionQueryBatchDiagnostic diagnostic{PhysicsCollisionQueryBatchDiagnostic::none};
    std::vector<PhysicsRaycastBatch2DRow> rows;
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

struct PhysicsShapeSweepBatch2DDesc {
    std::vector<PhysicsShapeSweep2DDesc> queries;
    // The default is intentionally unbounded; set a positive cap to fail closed on unexpected query counts.
    std::size_t max_queries{std::numeric_limits<std::size_t>::max()};
};

struct PhysicsShapeSweepBatch2DRow {
    std::size_t source_index{0};
    PhysicsCollisionQueryRowStatus status{PhysicsCollisionQueryRowStatus::invalid_request};
    PhysicsCollisionQueryRowDiagnostic diagnostic{PhysicsCollisionQueryRowDiagnostic::none};
    std::optional<PhysicsShapeSweepHit2D> hit;
};

struct PhysicsShapeSweepBatch2DResult {
    PhysicsCollisionQueryBatchStatus status{PhysicsCollisionQueryBatchStatus::invalid_request};
    PhysicsCollisionQueryBatchDiagnostic diagnostic{PhysicsCollisionQueryBatchDiagnostic::none};
    std::vector<PhysicsShapeSweepBatch2DRow> rows;
};

enum class Physics2DSimulateStepStatus : std::uint8_t { simulated, invalid_request };

enum class Physics2DRuntimeDiagnostic : std::uint8_t {
    none,
    invalid_request,
    invalid_delta_seconds,
    invalid_config,
    row_budget_exceeded,
    missing_body,
    invalid_joint,
    static_pair,
    disabled_joint,
    iteration_limit,
};

enum class Physics2DJointKind : std::uint8_t { distance, hinge, prismatic, spring };

enum class Physics2DAreaTriggerEventKind : std::uint8_t { enter, stay, exit };

struct Physics2DKinematicContactResolutionRequest {
    PhysicsBody2DId body;
    Vec2 attempted_displacement{.x = 0.0F, .y = 0.0F};
    float skin_width{0.0F};
    std::uint32_t max_iterations{1U};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    bool include_triggers{false};
};

struct Physics2DJointDesc {
    Physics2DJointKind kind{Physics2DJointKind::distance};
    PhysicsBody2DId first;
    PhysicsBody2DId second;
    Vec2 local_anchor_first{.x = 0.0F, .y = 0.0F};
    Vec2 local_anchor_second{.x = 0.0F, .y = 0.0F};
    float target_distance{0.0F};
    Vec2 axis{.x = 1.0F, .y = 0.0F};
    float minimum_translation{0.0F};
    float maximum_translation{0.0F};
    float stiffness{1.0F};
    bool enabled{true};
};

struct Physics2DContinuousCollisionRequest {
    float delta_seconds{0.0F};
    float skin_width{0.0F};
    bool include_triggers{false};
    std::size_t max_time_of_impact_rows{std::numeric_limits<std::size_t>::max()};
    std::size_t max_kinematic_contact_rows{std::numeric_limits<std::size_t>::max()};
    std::size_t max_joint_rows{std::numeric_limits<std::size_t>::max()};
    std::size_t max_trigger_event_rows{std::numeric_limits<std::size_t>::max()};
    std::vector<Physics2DKinematicContactResolutionRequest> kinematic_requests;
    std::vector<Physics2DJointDesc> joints;
    std::vector<PhysicsTriggerOverlap2D> previous_trigger_overlaps;
};

struct Physics2DTimeOfImpactRow {
    std::size_t source_index{0};
    PhysicsBody2DId body;
    Vec2 previous_position{.x = 0.0F, .y = 0.0F};
    Vec2 attempted_displacement{.x = 0.0F, .y = 0.0F};
    Vec2 applied_displacement{.x = 0.0F, .y = 0.0F};
    Vec2 remaining_displacement{.x = 0.0F, .y = 0.0F};
    PhysicsBody2DId hit_body;
    Vec2 normal{.x = 1.0F, .y = 0.0F};
    float distance{0.0F};
    float time_of_impact{0.0F};
    bool initial_overlap{false};
    bool hit{false};
    Physics2DRuntimeDiagnostic diagnostic{Physics2DRuntimeDiagnostic::none};
};

struct Physics2DKinematicContactResolutionRow {
    std::size_t source_index{0};
    PhysicsBody2DId body;
    PhysicsBody2DId hit_body;
    Vec2 attempted_displacement{.x = 0.0F, .y = 0.0F};
    Vec2 applied_displacement{.x = 0.0F, .y = 0.0F};
    Vec2 remaining_displacement{.x = 0.0F, .y = 0.0F};
    Vec2 normal{.x = 1.0F, .y = 0.0F};
    bool initial_overlap{false};
    Physics2DRuntimeDiagnostic diagnostic{Physics2DRuntimeDiagnostic::none};
};

struct Physics2DJointRow {
    std::size_t source_index{0};
    Physics2DJointKind kind{Physics2DJointKind::distance};
    PhysicsBody2DId first;
    PhysicsBody2DId second;
    float previous_distance{0.0F};
    float target_distance{0.0F};
    float residual_distance{0.0F};
    Vec2 first_correction{.x = 0.0F, .y = 0.0F};
    Vec2 second_correction{.x = 0.0F, .y = 0.0F};
    bool axis_limit_clamped{false};
    Physics2DRuntimeDiagnostic diagnostic{Physics2DRuntimeDiagnostic::none};
};

struct Physics2DAreaTriggerEventRow {
    std::size_t source_index{0};
    Physics2DAreaTriggerEventKind kind{Physics2DAreaTriggerEventKind::enter};
    PhysicsBody2DId trigger_body;
    PhysicsBody2DId other_body;
};

struct Physics2DSimulateStepResult {
    Physics2DSimulateStepStatus status{Physics2DSimulateStepStatus::invalid_request};
    Physics2DRuntimeDiagnostic diagnostic{Physics2DRuntimeDiagnostic::none};
    std::vector<Physics2DTimeOfImpactRow> time_of_impact_rows;
    std::vector<Physics2DKinematicContactResolutionRow> kinematic_contact_rows;
    std::vector<Physics2DJointRow> joint_rows;
    std::vector<Physics2DAreaTriggerEventRow> trigger_event_rows;
    std::size_t native_handle_exposure_count{0};
    std::size_t middleware_dispatch_count{0};
};

[[nodiscard]] bool is_valid_physics_body_desc(const PhysicsBody2DDesc& desc) noexcept;

class PhysicsWorld2D {
  public:
    explicit PhysicsWorld2D(PhysicsWorld2DConfig config = {});

    [[nodiscard]] PhysicsBody2DId create_body(const PhysicsBody2DDesc& desc);
    [[nodiscard]] PhysicsWorld2DConfig config() const noexcept;
    [[nodiscard]] PhysicsBody2D* find_body(PhysicsBody2DId id) noexcept;
    [[nodiscard]] const PhysicsBody2D* find_body(PhysicsBody2DId id) const noexcept;
    [[nodiscard]] const std::vector<PhysicsBody2D>& bodies() const noexcept;
    [[nodiscard]] std::vector<PhysicsBroadphasePair2D> broadphase_pairs() const;
    [[nodiscard]] std::vector<PhysicsContact2D> contacts() const;
    [[nodiscard]] std::vector<PhysicsTriggerOverlap2D> trigger_overlaps() const;
    [[nodiscard]] std::optional<PhysicsRaycastHit2D> raycast(PhysicsRaycast2DDesc desc) const;
    [[nodiscard]] PhysicsRaycastBatch2DResult raycast_batch(const PhysicsRaycastBatch2DDesc& desc) const;
    [[nodiscard]] std::optional<PhysicsShapeSweepHit2D> shape_sweep(PhysicsShapeSweep2DDesc desc) const;
    [[nodiscard]] PhysicsShapeSweepBatch2DResult shape_sweep_batch(const PhysicsShapeSweepBatch2DDesc& desc) const;

    void apply_force(PhysicsBody2DId id, Vec2 force);
    void step(float delta_seconds);
    void resolve_contacts(float restitution = 0.0F);
    void resolve_contacts(PhysicsContactSolver2DConfig config);

  private:
    PhysicsWorld2DConfig config_;
    std::vector<PhysicsBody2D> bodies_;
};

[[nodiscard]] Physics2DSimulateStepResult simulate_physics2d_step(PhysicsWorld2D& world,
                                                                  const Physics2DContinuousCollisionRequest& request);

} // namespace mirakana
