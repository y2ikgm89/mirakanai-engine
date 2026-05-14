// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace mirakana {

struct PhysicsBody3DId {
    std::uint32_t value{0};

    friend constexpr bool operator==(PhysicsBody3DId lhs, PhysicsBody3DId rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

inline constexpr PhysicsBody3DId null_physics_body_3d{};

enum class PhysicsShape3DKind : std::uint8_t { aabb, sphere, capsule };

struct PhysicsWorld3DConfig {
    Vec3 gravity{.x = 0.0F, .y = -9.80665F, .z = 0.0F};
};

struct PhysicsBody3DDesc {
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 velocity{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    float mass{1.0F};
    float linear_damping{0.0F};
    bool dynamic{true};
    Vec3 half_extents{.x = 0.5F, .y = 0.5F, .z = 0.5F};
    bool collision_enabled{true};
    PhysicsShape3DKind shape{PhysicsShape3DKind::aabb};
    float radius{0.5F};
    std::uint32_t collision_layer{1U};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    float half_height{0.5F};
    bool trigger{false};
};

struct PhysicsBody3D {
    PhysicsBody3DId id;
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 velocity{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 accumulated_force{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    float inverse_mass{1.0F};
    float linear_damping{0.0F};
    bool dynamic{true};
    Vec3 half_extents{.x = 0.5F, .y = 0.5F, .z = 0.5F};
    bool collision_enabled{true};
    PhysicsShape3DKind shape{PhysicsShape3DKind::aabb};
    float radius{0.5F};
    std::uint32_t collision_layer{1U};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    float half_height{0.5F};
    bool trigger{false};
};

struct PhysicsBroadphasePair3D {
    PhysicsBody3DId first;
    PhysicsBody3DId second;
};

struct PhysicsContact3D {
    PhysicsBody3DId first;
    PhysicsBody3DId second;
    Vec3 normal{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float penetration_depth{0.0F};
};

struct PhysicsContactPoint3D {
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    float penetration_depth{0.0F};
    std::uint32_t feature_id{0U};
    bool warm_start_eligible{false};
};

struct PhysicsContactManifold3D {
    PhysicsBody3DId first;
    PhysicsBody3DId second;
    Vec3 normal{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    std::vector<PhysicsContactPoint3D> points;
};

struct PhysicsContactSolver3DConfig {
    float restitution{0.0F};
    std::uint32_t iterations{1U};
    float position_correction_percent{1.0F};
    float penetration_slop{0.0F};
};

struct PhysicsRaycast3DDesc {
    Vec3 origin{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 direction{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float max_distance{1.0F};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
};

struct PhysicsRaycastHit3D {
    PhysicsBody3DId body;
    Vec3 point{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 normal{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float distance{0.0F};
};

struct PhysicsTriggerOverlap3D {
    // Sorted by body id; first/second do not imply trigger/other roles.
    PhysicsBody3DId first;
    PhysicsBody3DId second;
};

// Conservative v0 query: sweeps the query shape's collision bounds, not an exact primitive.
struct PhysicsShapeSweep3DDesc {
    Vec3 origin{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 direction{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float max_distance{1.0F};
    PhysicsShape3DKind shape{PhysicsShape3DKind::aabb};
    Vec3 half_extents{.x = 0.5F, .y = 0.5F, .z = 0.5F};
    float radius{0.5F};
    float half_height{0.5F};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    PhysicsBody3DId ignored_body{};
    bool include_triggers{true};
};

struct PhysicsShapeSweepHit3D {
    PhysicsBody3DId body;
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 normal{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float distance{0.0F};
    bool initial_overlap{false};
};

class PhysicsShape3DDesc {
  public:
    constexpr PhysicsShape3DDesc() noexcept = default;

    [[nodiscard]] static constexpr PhysicsShape3DDesc aabb(Vec3 half_extents) noexcept {
        return PhysicsShape3DDesc{PhysicsShape3DKind::aabb, half_extents, 0.5F, 0.5F};
    }

    [[nodiscard]] static constexpr PhysicsShape3DDesc sphere(float radius) noexcept {
        return PhysicsShape3DDesc{PhysicsShape3DKind::sphere, Vec3{.x = radius, .y = radius, .z = radius}, radius,
                                  radius};
    }

    [[nodiscard]] static constexpr PhysicsShape3DDesc capsule(float radius, float half_height) noexcept {
        return PhysicsShape3DDesc{PhysicsShape3DKind::capsule,
                                  Vec3{.x = radius, .y = half_height + radius, .z = radius}, radius, half_height};
    }

    [[nodiscard]] constexpr PhysicsShape3DKind kind() const noexcept {
        return kind_;
    }

    [[nodiscard]] constexpr Vec3 half_extents() const noexcept {
        return half_extents_;
    }

    [[nodiscard]] constexpr float radius() const noexcept {
        return radius_;
    }

    [[nodiscard]] constexpr float half_height() const noexcept {
        return half_height_;
    }

  private:
    constexpr PhysicsShape3DDesc(PhysicsShape3DKind kind, Vec3 half_extents, float radius, float half_height) noexcept
        : kind_(kind), half_extents_(half_extents), radius_(radius), half_height_(half_height) {}

    PhysicsShape3DKind kind_{PhysicsShape3DKind::aabb};
    Vec3 half_extents_{.x = 0.5F, .y = 0.5F, .z = 0.5F};
    float radius_{0.5F};
    float half_height_{0.5F};
};

struct PhysicsQueryFilter3D {
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    PhysicsBody3DId ignored_body{};
    bool include_triggers{true};
};

enum class PhysicsExactShapeSweep3DStatus : std::uint8_t { hit, no_hit, invalid_request };

enum class PhysicsExactShapeSweep3DDiagnostic : std::uint8_t { none, invalid_request };

struct PhysicsExactShapeSweep3DDesc {
    Vec3 origin{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 direction{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float max_distance{1.0F};
    PhysicsShape3DDesc shape;
    PhysicsQueryFilter3D filter{};
};

struct PhysicsExactShapeSweep3DHit {
    PhysicsBody3DId body;
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 normal{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float distance{0.0F};
    bool initial_overlap{false};
};

struct PhysicsExactShapeSweep3DResult {
    PhysicsExactShapeSweep3DStatus status{PhysicsExactShapeSweep3DStatus::invalid_request};
    PhysicsExactShapeSweep3DDiagnostic diagnostic{PhysicsExactShapeSweep3DDiagnostic::none};
    std::optional<PhysicsExactShapeSweep3DHit> hit;
};

enum class PhysicsContinuousStep3DStatus : std::uint8_t { stepped, invalid_request };

enum class PhysicsContinuousStep3DDiagnostic : std::uint8_t { none, invalid_delta_seconds, invalid_config };

struct PhysicsContinuousStep3DConfig {
    float skin_width{0.001F};
    // When true, trigger bodies are treated as blocking CCD targets rather than non-blocking sensor rows.
    bool include_triggers{false};
};

struct PhysicsContinuousStep3DRow {
    PhysicsBody3DId body;
    Vec3 previous_position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 attempted_displacement{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 applied_displacement{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 remaining_displacement{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    PhysicsBody3DId hit_body{};
    std::optional<PhysicsExactShapeSweep3DHit> hit;
    bool ccd_applied{false};
};

struct PhysicsContinuousStep3DResult {
    PhysicsContinuousStep3DStatus status{PhysicsContinuousStep3DStatus::invalid_request};
    PhysicsContinuousStep3DDiagnostic diagnostic{PhysicsContinuousStep3DDiagnostic::none};
    std::vector<PhysicsContinuousStep3DRow> rows;
};

enum class PhysicsExactSphereCast3DStatus : std::uint8_t { hit, no_hit, invalid_request };

enum class PhysicsExactSphereCast3DDiagnostic : std::uint8_t { none, invalid_request };

struct PhysicsExactSphereCast3DDesc {
    Vec3 origin{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 direction{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float max_distance{1.0F};
    float radius{0.5F};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    PhysicsBody3DId ignored_body{};
    bool include_triggers{true};
};

struct PhysicsExactSphereCast3DHit {
    PhysicsBody3DId body;
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 normal{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float distance{0.0F};
    bool initial_overlap{false};
};

struct PhysicsExactSphereCast3DResult {
    PhysicsExactSphereCast3DStatus status{PhysicsExactSphereCast3DStatus::invalid_request};
    PhysicsExactSphereCast3DDiagnostic diagnostic{PhysicsExactSphereCast3DDiagnostic::none};
    std::optional<PhysicsExactSphereCast3DHit> hit;
};

enum class PhysicsCharacterController3DStatus : std::uint8_t {
    moved,
    constrained,
    blocked,
    initial_overlap,
    invalid_request
};

enum class PhysicsCharacterController3DDiagnostic : std::uint8_t {
    none,
    invalid_request,
    initial_overlap,
    iteration_limit
};

struct PhysicsCharacterController3DDesc {
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 displacement{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    float radius{0.5F};
    float half_height{0.5F};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    bool include_triggers{false};
    float skin_width{0.01F};
    std::uint32_t max_iterations{4U};
    float grounded_normal_y{0.70710677F};
};

struct PhysicsCharacterController3DContact {
    PhysicsBody3DId body;
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 normal{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float distance{0.0F};
    bool initial_overlap{false};
    bool grounded{false};
};

struct PhysicsCharacterController3DResult {
    PhysicsCharacterController3DStatus status{PhysicsCharacterController3DStatus::invalid_request};
    PhysicsCharacterController3DDiagnostic diagnostic{PhysicsCharacterController3DDiagnostic::none};
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 applied_displacement{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 remaining_displacement{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    bool grounded{false};
    std::vector<PhysicsCharacterController3DContact> contacts;
};

enum class PhysicsCharacterDynamicPolicy3DStatus : std::uint8_t {
    moved,
    constrained,
    stepped,
    initial_overlap,
    invalid_request
};

enum class PhysicsCharacterDynamicPolicy3DDiagnostic : std::uint8_t {
    none,
    invalid_request,
    initial_overlap,
    step_blocked
};

enum class PhysicsCharacterDynamicPolicy3DRowKind : std::uint8_t {
    solid_contact,
    trigger_overlap,
    dynamic_push,
    step_up,
    ground_probe
};

struct PhysicsCharacterDynamicPolicy3DDesc {
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 displacement{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    float radius{0.5F};
    float half_height{0.5F};
    std::uint32_t character_layer{1U};
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    bool include_triggers{false};
    float skin_width{0.01F};
    float step_height{0.0F};
    float ground_probe_distance{0.05F};
    float grounded_normal_y{0.70710677F};
    float max_slope_normal_y{0.70710677F};
    float dynamic_push_distance{0.0F};
};

struct PhysicsCharacterDynamicPolicy3DRow {
    PhysicsCharacterDynamicPolicy3DRowKind kind{PhysicsCharacterDynamicPolicy3DRowKind::solid_contact};
    PhysicsBody3DId body;
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 normal{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    float distance{0.0F};
    bool initial_overlap{false};
    bool grounded{false};
    bool walkable_slope{false};
    Vec3 suggested_displacement{.x = 0.0F, .y = 0.0F, .z = 0.0F};
};

struct PhysicsCharacterDynamicPolicy3DResult {
    PhysicsCharacterDynamicPolicy3DStatus status{PhysicsCharacterDynamicPolicy3DStatus::invalid_request};
    PhysicsCharacterDynamicPolicy3DDiagnostic diagnostic{PhysicsCharacterDynamicPolicy3DDiagnostic::none};
    Vec3 position{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 applied_displacement{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 remaining_displacement{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    bool grounded{false};
    bool stepped{false};
    std::vector<PhysicsCharacterDynamicPolicy3DRow> rows;
};

enum class PhysicsJoint3DStatus : std::uint8_t { solved, invalid_request };

enum class PhysicsJoint3DDiagnostic : std::uint8_t {
    none,
    invalid_config,
    invalid_joint,
    missing_body,
    static_pair,
    disabled_joint,
};

struct PhysicsDistanceJoint3DDesc {
    PhysicsBody3DId first;
    PhysicsBody3DId second;
    Vec3 local_anchor_first{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 local_anchor_second{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    float rest_distance{0.0F};
    bool enabled{true};
};

struct PhysicsJointSolve3DConfig {
    std::uint32_t iterations{1U};
    float tolerance{0.0001F};
};

struct PhysicsJointSolve3DDesc {
    PhysicsJointSolve3DConfig config{};
    std::vector<PhysicsDistanceJoint3DDesc> distance_joints;
};

struct PhysicsJointSolve3DRow {
    std::size_t source_index{0};
    PhysicsBody3DId first;
    PhysicsBody3DId second;
    float previous_distance{0.0F};
    float target_distance{0.0F};
    float residual_distance{0.0F};
    Vec3 first_correction{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vec3 second_correction{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    PhysicsJoint3DDiagnostic diagnostic{PhysicsJoint3DDiagnostic::none};
};

struct PhysicsJointSolve3DResult {
    PhysicsJoint3DStatus status{PhysicsJoint3DStatus::invalid_request};
    PhysicsJoint3DDiagnostic diagnostic{PhysicsJoint3DDiagnostic::none};
    std::vector<PhysicsJointSolve3DRow> rows;
};

enum class PhysicsDeterminismGate3DStatus : std::uint8_t { passed, budget_exceeded, invalid_request };

enum class PhysicsDeterminismGate3DDiagnostic : std::uint8_t {
    none,
    invalid_config,
    bodies_exceeded,
    broadphase_pairs_exceeded,
    contacts_exceeded,
    contact_manifolds_exceeded,
    trigger_overlaps_exceeded,
    contact_points_exceeded,
};

struct PhysicsDeterminismGate3DConfig {
    std::uint64_t max_bodies{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t max_broadphase_pairs{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t max_contacts{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t max_contact_manifolds{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t max_trigger_overlaps{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t max_contact_points{std::numeric_limits<std::uint64_t>::max()};
};

struct PhysicsDeterminismGate3DCounts {
    std::uint64_t bodies{0};
    std::uint64_t broadphase_pairs{0};
    std::uint64_t contacts{0};
    std::uint64_t contact_manifolds{0};
    std::uint64_t trigger_overlaps{0};
    std::uint64_t contact_points{0};
};

struct PhysicsReplaySignature3D {
    std::uint64_t value{0};
    std::uint64_t body_count{0};
};

struct PhysicsDeterminismGate3DResult {
    PhysicsDeterminismGate3DStatus status{PhysicsDeterminismGate3DStatus::invalid_request};
    PhysicsDeterminismGate3DDiagnostic diagnostic{PhysicsDeterminismGate3DDiagnostic::none};
    PhysicsDeterminismGate3DCounts counts{};
    PhysicsReplaySignature3D replay_signature{};
};

enum class PhysicsAuthoredCollision3DBuildStatus : std::uint8_t {
    success,
    invalid_request,
    invalid_body,
    duplicate_name,
    unsupported_native_backend,
};

enum class PhysicsAuthoredCollision3DDiagnostic : std::uint8_t {
    none,
    invalid_world_gravity,
    invalid_body_name,
    invalid_body_desc,
    duplicate_body_name,
    native_backend_unsupported,
};

struct PhysicsAuthoredCollisionBody3DDesc {
    std::string name;
    PhysicsBody3DDesc body;
};

struct PhysicsAuthoredCollisionScene3DDesc {
    PhysicsWorld3DConfig world_config{};
    std::vector<PhysicsAuthoredCollisionBody3DDesc> bodies;
    bool require_native_backend{false};
};

struct PhysicsAuthoredCollisionBody3DRow {
    std::string name;
    PhysicsBody3DId body;
    std::size_t source_index{0};
};

[[nodiscard]] bool is_valid_physics_body_desc(const PhysicsBody3DDesc& desc) noexcept;

class PhysicsWorld3D {
  public:
    explicit PhysicsWorld3D(PhysicsWorld3DConfig config = {});

    [[nodiscard]] PhysicsBody3DId create_body(const PhysicsBody3DDesc& desc);
    [[nodiscard]] PhysicsBody3D* find_body(PhysicsBody3DId id) noexcept;
    [[nodiscard]] const PhysicsBody3D* find_body(PhysicsBody3DId id) const noexcept;
    [[nodiscard]] PhysicsWorld3DConfig config() const noexcept;
    [[nodiscard]] const std::vector<PhysicsBody3D>& bodies() const noexcept;
    [[nodiscard]] std::vector<PhysicsBroadphasePair3D> broadphase_pairs() const;
    [[nodiscard]] std::vector<PhysicsContact3D> contacts() const;
    [[nodiscard]] std::vector<PhysicsContactManifold3D> contact_manifolds() const;
    [[nodiscard]] std::vector<PhysicsTriggerOverlap3D> trigger_overlaps() const;
    [[nodiscard]] std::optional<PhysicsRaycastHit3D> raycast(PhysicsRaycast3DDesc desc) const;
    [[nodiscard]] std::optional<PhysicsShapeSweepHit3D> shape_sweep(PhysicsShapeSweep3DDesc desc) const;
    [[nodiscard]] PhysicsExactShapeSweep3DResult exact_shape_sweep(PhysicsExactShapeSweep3DDesc desc) const;
    [[nodiscard]] PhysicsExactSphereCast3DResult exact_sphere_cast(PhysicsExactSphereCast3DDesc desc) const;
    [[nodiscard]] PhysicsContinuousStep3DResult step_continuous(float delta_seconds,
                                                                PhysicsContinuousStep3DConfig config = {});

    void apply_force(PhysicsBody3DId id, Vec3 force);
    void step(float delta_seconds);
    void resolve_contacts(float restitution = 0.0F);
    void resolve_contacts(PhysicsContactSolver3DConfig config);

  private:
    PhysicsWorld3DConfig config_;
    std::vector<PhysicsBody3D> bodies_;
};

struct PhysicsAuthoredCollisionScene3DBuildResult {
    PhysicsAuthoredCollision3DBuildStatus status{PhysicsAuthoredCollision3DBuildStatus::invalid_request};
    PhysicsAuthoredCollision3DDiagnostic diagnostic{PhysicsAuthoredCollision3DDiagnostic::none};
    std::size_t body_index{0};
    PhysicsWorld3D world;
    std::vector<PhysicsAuthoredCollisionBody3DRow> bodies;
};

[[nodiscard]] PhysicsCharacterController3DResult
move_physics_character_controller_3d(const PhysicsWorld3D& world, const PhysicsCharacterController3DDesc& desc);

[[nodiscard]] PhysicsCharacterDynamicPolicy3DResult
evaluate_physics_character_dynamic_policy_3d(const PhysicsWorld3D& world,
                                             const PhysicsCharacterDynamicPolicy3DDesc& desc);

[[nodiscard]] PhysicsJointSolve3DResult solve_physics_joints_3d(PhysicsWorld3D& world,
                                                                const PhysicsJointSolve3DDesc& desc);

[[nodiscard]] PhysicsReplaySignature3D make_physics_replay_signature_3d(const PhysicsWorld3D& world);

[[nodiscard]] PhysicsDeterminismGate3DResult
evaluate_physics_determinism_gate_3d(const PhysicsWorld3D& world, const PhysicsDeterminismGate3DConfig& config = {});

[[nodiscard]] PhysicsAuthoredCollisionScene3DBuildResult
build_physics_world_3d_from_authored_collision_scene(const PhysicsAuthoredCollisionScene3DDesc& desc);

} // namespace mirakana
