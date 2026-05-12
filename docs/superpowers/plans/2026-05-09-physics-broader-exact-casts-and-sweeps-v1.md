# Physics Broader Exact Casts And Sweeps Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party exact 3D shape sweep query surface for the accepted AABB, sphere, and vertical capsule primitives while keeping existing conservative `PhysicsWorld3D::shape_sweep` behavior explicitly separate.

**Architecture:** Keep the work inside dependency-free `MK_physics` and public `mirakana::` value types. Add shared query-shape/filter/result rows, migrate existing exact-sphere behavior onto the new exact sweep path, and prove false-positive rejection beyond bounds-based sweeps with focused unit tests before widening docs or readiness claims.

**Tech Stack:** C++23, `MK_physics`, `MK_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.

---

**Plan ID:** `physics-broader-exact-casts-and-sweeps-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Previous Slice:** [2026-05-09-physics-scene-package-collision-authoring-v1.md](2026-05-09-physics-scene-package-collision-authoring-v1.md)

## Context

- The master plan's Physics 1.0 order selects this slice immediately after `physics-scene-package-collision-authoring-v1`.
- Current 3D physics primitives are `PhysicsShape3DKind::aabb`, `PhysicsShape3DKind::sphere`, and `PhysicsShape3DKind::capsule`.
- Current `PhysicsWorld3D::shape_sweep` is documented as a conservative bounds-based query and returns `std::optional`, so it must remain distinct from the exact diagnostic API.
- Current exact query coverage is limited to `PhysicsWorld3D::exact_sphere_cast`, which already proves exact moving-sphere queries against AABB, sphere, and vertical capsule targets.
- The prior package-collision slice makes query targets package-visible, but it does not broaden exact query behavior.

## Constraints

- Do not rename or change the semantics of `PhysicsWorld3D::shape_sweep`.
- Do not change `move_physics_character_controller_3d` to use exact sweeps in this slice; controller/dynamic policy remains a later child plan.
- Keep AABB axis-aligned and capsule orientation vertical/Y-axis.
- Do not add Jolt, middleware, native backend handles, mesh casts, convex hull casts, CCD, joints, or benchmarks here.
- Do not keep `PhysicsExactSphereCast3DDesc`, `PhysicsExactSphereCast3DResult`, or `PhysicsWorld3D::exact_sphere_cast` solely for backward compatibility. Keep them only if they remain first-class convenience API; otherwise replace all call sites/tests/docs with `exact_shape_sweep` and remove the old API in this same slice.

## Done When

- `PhysicsWorld3D` exposes an exact shape sweep API with status/diagnostic result rows for AABB, sphere, and vertical capsule query shapes.
- Existing exact-sphere behavior is preserved through the shared exact sweep path. If `exact_sphere_cast` remains, it delegates to `exact_shape_sweep`; if it is removed, all call sites/tests/docs are migrated in the same slice.
- Tests prove exact sweeps reject conservative bounds false positives, report initial overlaps, respect filters, choose nearest hits, and break ties deterministically.
- Docs, manifest, skills, and master-plan ledger say exact 3D AABB/sphere/capsule sweeps are implemented while CCD, joints, controller dynamic policy, Jolt/native backends, mesh/convex casts, and broad physics readiness remain unsupported.
- Focused build/test commands, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/tool blocker.

## File Structure

- Modify `engine/physics/include/mirakana/physics/physics3d.hpp`: add shared exact query shape/filter/result declarations and `PhysicsWorld3D::exact_shape_sweep`.
- Modify `engine/physics/src/physics3d.cpp`: add validation and exact sweep implementation for AABB, sphere, and vertical capsule query primitives against AABB, sphere, and vertical capsule targets.
- Modify `tests/unit/core_tests.cpp`: add RED/GREEN coverage beside the existing 3D exact sphere cast tests.
- Modify docs and guidance after behavior is green: `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/testing.md`, `docs/superpowers/plans/README.md`, this master plan, `engine/agent/manifest.json`, `.agents/skills/gameengine-game-development/SKILL.md`, and `.claude/skills/gameengine-game-development/SKILL.md`.

## Task 1: Public Exact Shape Sweep Contract

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`

- [x] **Step 1: Add RED tests for the new public API**

Add tests near the existing `3d physics exact sphere cast` block:

```cpp
MK_TEST("3d physics exact shape sweep sphere matches exact sphere cast") {
    mirakana::PhysicsWorld3D world;
    const auto target = world.create_body(mirakana::PhysicsBody3DDesc{
        mirakana::Vec3{4.0F, 0.0F, 0.0F},
        mirakana::Vec3{},
        0.0F,
        0.0F,
        false,
        mirakana::Vec3{0.5F, 0.5F, 0.5F},
        true,
        mirakana::PhysicsShape3DKind::sphere,
        0.5F,
    });

    const auto sphere = world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        mirakana::Vec3{1.0F, 0.0F, 0.0F},
        8.0F,
        0.5F,
    });
    const auto generic = world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        mirakana::Vec3{1.0F, 0.0F, 0.0F},
        8.0F,
        mirakana::PhysicsShape3DDesc::sphere(0.5F),
    });

    MK_REQUIRE(sphere.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(generic.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(generic.hit.has_value());
    MK_REQUIRE(generic.hit->body == target);
    MK_REQUIRE(std::abs(generic.hit->distance - sphere.hit->distance) < 0.0001F);
    MK_REQUIRE(generic.hit->normal == sphere.hit->normal);
}
```

- [x] **Step 2: Run the focused build and confirm RED**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
```

Expected: FAIL to compile on missing `PhysicsShape3DDesc`, `PhysicsExactShapeSweep3DDesc`, `PhysicsExactShapeSweep3DStatus`, and `PhysicsWorld3D::exact_shape_sweep`.

- [x] **Step 3: Add public declarations**

Add declarations in `physics3d.hpp` near the current sweep/query rows:

```cpp
class PhysicsShape3DDesc {
  public:
    constexpr PhysicsShape3DDesc() noexcept = default;
    [[nodiscard]] static constexpr PhysicsShape3DDesc aabb(Vec3 half_extents) noexcept;
    [[nodiscard]] static constexpr PhysicsShape3DDesc sphere(float radius) noexcept;
    [[nodiscard]] static constexpr PhysicsShape3DDesc capsule(float radius, float half_height) noexcept;
    [[nodiscard]] constexpr PhysicsShape3DKind kind() const noexcept;
    [[nodiscard]] constexpr Vec3 half_extents() const noexcept;
    [[nodiscard]] constexpr float radius() const noexcept;
    [[nodiscard]] constexpr float half_height() const noexcept;
};

struct PhysicsQueryFilter3D {
    std::uint32_t collision_mask{0xFFFF'FFFFU};
    PhysicsBody3DId ignored_body{};
    bool include_triggers{true};
};

enum class PhysicsExactShapeSweep3DStatus { hit, no_hit, invalid_request };

enum class PhysicsExactShapeSweep3DDiagnostic { none, invalid_request };

struct PhysicsExactShapeSweep3DDesc {
    Vec3 origin{0.0F, 0.0F, 0.0F};
    Vec3 direction{1.0F, 0.0F, 0.0F};
    float max_distance{1.0F};
    PhysicsShape3DDesc shape{};
    PhysicsQueryFilter3D filter{};
};

struct PhysicsExactShapeSweep3DHit {
    PhysicsBody3DId body;
    Vec3 position{0.0F, 0.0F, 0.0F};
    Vec3 normal{1.0F, 0.0F, 0.0F};
    float distance{0.0F};
    bool initial_overlap{false};
};

struct PhysicsExactShapeSweep3DResult {
    PhysicsExactShapeSweep3DStatus status{PhysicsExactShapeSweep3DStatus::invalid_request};
    PhysicsExactShapeSweep3DDiagnostic diagnostic{PhysicsExactShapeSweep3DDiagnostic::none};
    std::optional<PhysicsExactShapeSweep3DHit> hit;
};
```

Add this member:

```cpp
[[nodiscard]] PhysicsExactShapeSweep3DResult exact_shape_sweep(PhysicsExactShapeSweep3DDesc desc) const;
```

- [x] **Step 4: Build and confirm the API compiles after a minimal stub**

Add a temporary implementation that returns `invalid_request`, then run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: build succeeds and the new test fails at runtime because the stub does not hit.

## Task 2: Exact Sphere Sweep Delegation

**Files:**
- Modify: `engine/physics/src/physics3d.cpp`
- Modify: `tests/unit/core_tests.cpp`

- [x] **Step 1: Implement exact sphere query through the shared API**

Implement `PhysicsWorld3D::exact_shape_sweep` for `PhysicsShape3DKind::sphere` by reusing the existing exact sphere candidate logic:

```cpp
// Pseudocode shape, keep local helper names consistent with existing file style.
if (desc.shape.kind() == PhysicsShape3DKind::sphere) {
    // validate desc.shape.radius(), direction, max_distance, and finite origin
    // iterate bodies with the same filters as exact_sphere_cast
    // call exact_sphere_cast_body(body, desc.origin, normalized_direction, desc.max_distance, desc.shape.radius())
    // return hit/no_hit/invalid_request with the shared result type
}
```

Keep tie-breaking stable by replacing the closest candidate only when distance is smaller, or when distance is equal within existing epsilon and the candidate body id is lower.

- [x] **Step 2: Update `exact_sphere_cast` to delegate or share candidate conversion**

Return equivalent `PhysicsExactSphereCast3DResult` values from `exact_sphere_cast` after invoking the shared sphere path, or keep the old loop if the conversion would make the implementation less clear. The public behavior must remain unchanged.

- [x] **Step 3: Run existing and new exact sphere tests**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: PASS for all existing exact sphere cast tests plus the new generic sphere sweep test.

## Task 3: Exact AABB Sweep

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/src/physics3d.cpp`

- [x] **Step 1: Add RED coverage for moving AABB**

Add tests that prove:

- moving AABB hits AABB at the expected distance;
- moving AABB against a sphere rejects a conservative-bounds false positive;
- moving AABB against a capsule reports the nearest deterministic hit;
- invalid AABB half extents return `invalid_request`.

Use explicit numeric expectations with `std::abs(value - expected) < 0.0001F`.

- [x] **Step 2: Implement exact AABB sweep against supported targets**

Implement exact moving-AABB behavior with first-party math:

- AABB vs AABB: swept slab over Minkowski-expanded target extents.
- AABB vs sphere: clamp swept AABB candidate through the exact sphere contact helper or reduce to interval plus closest-point validation at candidate time.
- AABB vs vertical capsule: combine AABB-vs-expanded vertical segment/capsule side and cap checks; reject conservative bounds false positives.

If a helper would become large, keep it private in `physics3d.cpp`; do not add a new module.

- [x] **Step 3: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: PASS for all exact sphere and AABB sweep tests.

## Task 4: Exact Capsule Sweep

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/src/physics3d.cpp`

- [x] **Step 1: Add RED coverage for moving vertical capsule**

Add tests that prove:

- moving capsule hits AABB side at a deterministic distance and normal;
- moving capsule hits sphere with rounded-side behavior;
- moving capsule hits capsule side and cap cases;
- initial overlap returns `initial_overlap=true`;
- invalid capsule radius or half height returns `invalid_request`.

- [x] **Step 2: Implement exact capsule sweep**

Implement vertical capsule sweeps using the existing segment/capsule helper concepts:

- Treat the moving capsule as a swept vertical segment plus radius.
- Reuse the existing exact sphere/cylinder/cap helper style where possible.
- Keep normals from target to query consistent with existing `exact_sphere_cast` hit normals.

- [x] **Step 3: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: PASS for exact sphere, AABB, and capsule sweep tests.

## Task 5: Filters, Diagnostics, And Conservative API Boundary

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/src/physics3d.cpp`

- [x] **Step 1: Add RED coverage for filters and diagnostics**

Add tests covering:

- `PhysicsQueryFilter3D::collision_mask`;
- `PhysicsQueryFilter3D::ignored_body`;
- `PhysicsQueryFilter3D::include_triggers=false`;
- disabled bodies are skipped;
- zero direction, non-finite origin/direction/max distance, and invalid shape fields return `invalid_request`;
- equal-distance ties select the lower body id deterministically.

- [x] **Step 2: Centralize exact sweep validation**

Add private validation helpers in `physics3d.cpp` for:

```cpp
bool is_valid_shape3d_desc(const PhysicsShape3DDesc& shape) noexcept;
bool is_valid_exact_shape_sweep_desc(const PhysicsExactShapeSweep3DDesc& desc) noexcept;
```

Use existing finite/positive checks and shape-specific validation behavior. Keep diagnostics deterministic; do not throw.

- [x] **Step 3: Preserve conservative sweep tests**

Run:

```powershell
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: existing conservative `shape_sweep` tests still pass unchanged, proving exact sweep is additive and not a behavior rename.

## Task 6: Docs, Manifest, Skills, And Plan Sync

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] **Step 1: Update docs after behavior is green**

Document the exact claim:

```text
PhysicsWorld3D::exact_shape_sweep supports exact AABB, sphere, and vertical capsule query primitives against current AABB, sphere, and vertical capsule targets with filters, nearest-hit, deterministic tie, initial-overlap, and invalid-request diagnostics.
```

Keep these unsupported:

```text
CCD, joints, controller dynamic push/step policy, Jolt/native backends, oriented boxes, mesh/convex casts, physics benchmarks, and broad physics readiness.
```

- [x] **Step 2: Move manifest and registry pointers on completion**

When implementation is complete, mark this plan `Completed`, move the registry active slice to the next master-plan child, and update `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` consistently.

- [x] **Step 3: Run static checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

Expected: PASS.

## Task 7: Full Verification And Closeout

**Files:**
- Modify: `docs/superpowers/plans/2026-05-09-physics-broader-exact-casts-and-sweeps-v1.md`

- [x] **Step 1: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: PASS.

- [x] **Step 2: Run repository checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
```

Expected: PASS, or record the exact host/tool blocker.

- [x] **Step 3: Record validation evidence**

Append a validation evidence table with command, result, and relevant test names. Then update:

```markdown
**Status:** Completed.
```

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_core_tests --config Debug` | Passed | Focused C++ build succeeded after exact shape sweep implementation, factory API cleanup, epsilon tie handling, and added invalid diagnostics tests. |
| `ctest --preset dev -R MK_core_tests --output-on-failure` | Passed | `MK_core_tests` passed, including exact AABB/sphere/vertical-capsule sweep tests, conservative false-positive rejection, rounded corners, diagonal paths, nearest/disabled behavior, capsule cap hits, initial overlaps, filters, deterministic ties, and invalid diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public API boundary accepted `PhysicsShape3DDesc`, `PhysicsQueryFilter3D`, `PhysicsExactShapeSweep3D*`, and `PhysicsWorld3D::exact_shape_sweep`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied clang-format first; the follow-up format check passed. |
| `git diff --check` | Passed | Whitespace/conflict-marker check returned exit code 0. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Plan registry, active child pointer, and manifest JSON contracts are synchronized with `physics-contact-manifold-stability-v1` as the next active child. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration guidance accepts exact shape sweeps as implemented and keeps contact manifold stability, CCD, joints, dynamic policy, and Jolt/native backends unsupported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Audit still reports 11 non-ready unsupported gaps; no broad physics readiness promotion was made. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed; Metal/Apple lanes remain diagnostic-only host gates on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Repository build completed after validation. |

## Self-Review

- Spec coverage: this plan covers the second Physics 1.0 child only: broader exact 3D primitive query/sweep coverage.
- Exclusions: conservative sweep behavior, controller policy, joints, CCD, benchmark gates, editor UX, Jolt/native backends, oriented primitives, mesh/convex casts, and broad physics readiness are intentionally deferred.
- Placeholder scan: no task contains deferred placeholder markers; every behavior task names the target files and expected verification commands.
- Type consistency: public names use `PhysicsShape3DDesc`, `PhysicsQueryFilter3D`, `PhysicsExactShapeSweep3D*`, and `PhysicsWorld3D::exact_shape_sweep` consistently.
