# Physics Contact Manifold Stability Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Improve 3D narrowphase contact rows and solver inputs so accepted first-party primitives produce deterministic, persistent, warm-start-safe contact data for stable stacking/sliding.

**Architecture:** Keep the work inside dependency-free `MK_physics` value types and `PhysicsWorld3D`. Add explicit manifold/contact-point rows beside the existing broadphase/query APIs, derive legacy `contacts()` rows from the same deterministic manifold path, and make `resolve_contacts` consume the shared manifold ordering without adding middleware, native handles, or a new backend.

**Tech Stack:** C++23, `MK_physics`, `MK_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.

---

**Plan ID:** `physics-contact-manifold-stability-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Previous Slice:** [2026-05-09-physics-broader-exact-casts-and-sweeps-v1.md](2026-05-09-physics-broader-exact-casts-and-sweeps-v1.md)

## Context

- `physics-broader-exact-casts-and-sweeps-v1` completed exact 3D AABB/sphere/vertical-capsule query shapes through `PhysicsWorld3D::exact_shape_sweep`.
- Current 3D contacts expose one `PhysicsContact3D` row with `first`, `second`, `normal`, and `penetration_depth`.
- `PhysicsWorld3D::contacts()` is deterministic but narrow: it does not expose contact point rows, persistent feature keys, or solver-ready ordering.
- `PhysicsWorld3D::resolve_contacts` currently loops over `contacts()` each iteration and applies a single normal impulse/correction per contact row.
- This slice should improve the first-party solver surface, not add Jolt, CCD, joints, dynamic character push policy, oriented boxes, mesh/convex casts, or performance benchmarks.

## Constraints

- Keep `engine/core` and `engine/physics` independent from OS, GPU, editor, asset formats, and middleware.
- Do not add third-party dependencies.
- Keep supported 3D shapes limited to AABB, sphere, and vertical/Y-axis capsule.
- Do not change `PhysicsWorld3D::shape_sweep`, `PhysicsWorld3D::exact_shape_sweep`, or `move_physics_character_controller_3d` semantics in this slice.
- Do not claim full warm starting. Add deterministic, stable row keys and `warm_start_eligible` metadata that a later solver can safely consume.
- It is acceptable to change public C++ API shape because the project has no backward-compatibility requirement, but keep `contacts()` as a first-class flattened view if it remains useful and tested.

## Done When

- `PhysicsWorld3D` exposes deterministic 3D contact manifolds with stable point rows for AABB/sphere/vertical-capsule contacts.
- `PhysicsContact3D` rows are derived from the same manifold path so old and new contact views cannot drift.
- Contact manifolds are sorted by stable body ids and contact points are sorted by stable feature ids.
- Solver iterations consume the same ordered manifold data and keep existing inverse-mass correction, slop, restitution, trigger filtering, and replay behavior green.
- Tests prove deterministic ordering, feature-key persistence under small movement, stable stacked-body correction, sliding tangential velocity preservation, trigger exclusion, and invalid solver config behavior.
- Docs, manifest, skills, registry, and this master-plan ledger say contact manifold stability is implemented while CCD, joints, dynamic controller policy, Jolt/native backends, mesh/convex casts, physics benchmarks, and broad physics readiness remain unsupported.
- Focused build/test commands, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/tool blocker.

## File Structure

- Modify `engine/physics/include/mirakana/physics/physics3d.hpp`: add `PhysicsContactPoint3D`, `PhysicsContactManifold3D`, and `PhysicsWorld3D::contact_manifolds`.
- Modify `engine/physics/src/physics3d.cpp`: replace private single-contact generation with deterministic manifold generation and make `contacts()` flatten the same rows.
- Modify `tests/unit/core_tests.cpp`: add RED/GREEN coverage beside the existing 3D contact and iterative solver tests.
- Modify docs and guidance after behavior is green: `docs/architecture.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/testing.md`, `docs/superpowers/plans/README.md`, this master plan, `engine/agent/manifest.json`, `.agents/skills/gameengine-game-development/SKILL.md`, and `.claude/skills/gameengine-game-development/SKILL.md`.

## Task 1: Public Manifold Contract

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`

- [x] **Step 1: Add RED tests for public contact manifold rows**

Add a test near the current `3d physics world reports deterministic aabb contacts` block:

```cpp
MK_TEST("3d physics contact manifolds expose deterministic solver rows") {
    mirakana::PhysicsWorld3D world;
    const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        1.0F,
        0.0F,
        true,
        mirakana::Vec3{1.0F, 1.0F, 1.0F},
    });
    const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
        mirakana::Vec3{1.5F, 0.0F, 0.0F},
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        1.0F,
        0.0F,
        true,
        mirakana::Vec3{1.0F, 1.0F, 1.0F},
    });

    const auto manifolds = world.contact_manifolds();

    MK_REQUIRE(manifolds.size() == 1);
    MK_REQUIRE(manifolds[0].first == first);
    MK_REQUIRE(manifolds[0].second == second);
    MK_REQUIRE(manifolds[0].normal == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(manifolds[0].points.size() == 1);
    MK_REQUIRE(std::abs(manifolds[0].points[0].penetration_depth - 0.5F) < 0.0001F);
    MK_REQUIRE(manifolds[0].points[0].warm_start_eligible);
    MK_REQUIRE(manifolds[0].points[0].feature_id != 0U);
}
```

- [x] **Step 2: Run the focused build and confirm RED**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
```

Expected: FAIL to compile because `PhysicsWorld3D::contact_manifolds`, `PhysicsContactManifold3D`, and `PhysicsContactPoint3D` do not exist yet.

- [x] **Step 3: Add public value types**

Add declarations near `PhysicsContact3D`:

```cpp
struct PhysicsContactPoint3D {
    Vec3 position{0.0F, 0.0F, 0.0F};
    float penetration_depth{0.0F};
    std::uint32_t feature_id{0U};
    bool warm_start_eligible{false};
};

struct PhysicsContactManifold3D {
    PhysicsBody3DId first;
    PhysicsBody3DId second;
    Vec3 normal{1.0F, 0.0F, 0.0F};
    std::vector<PhysicsContactPoint3D> points;
};
```

Add this member:

```cpp
[[nodiscard]] std::vector<PhysicsContactManifold3D> contact_manifolds() const;
```

- [x] **Step 4: Build with a minimal stub and confirm runtime RED**

Implement `PhysicsWorld3D::contact_manifolds()` as an empty vector, then run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: build succeeds and the new test fails because the manifold list is empty.

## Task 2: Deterministic Manifold Generation

**Files:**
- Modify: `engine/physics/src/physics3d.cpp`
- Modify: `tests/unit/core_tests.cpp`

- [x] **Step 1: Implement one-point manifolds from current narrowphase**

Replace the private single-contact path with a helper that returns one deterministic manifold:

```cpp
[[nodiscard]] PhysicsContactManifold3D make_single_point_manifold(const PhysicsContact3D& contact,
                                                                  Vec3 point,
                                                                  std::uint32_t feature_id) {
    return PhysicsContactManifold3D{
        contact.first,
        contact.second,
        contact.normal,
        {PhysicsContactPoint3D{point, contact.penetration_depth, feature_id, feature_id != 0U}},
    };
}
```

Use shape-pair-specific feature ids:

```cpp
constexpr std::uint32_t aabb_aabb_x_feature = 0xA001U;
constexpr std::uint32_t aabb_aabb_y_feature = 0xA002U;
constexpr std::uint32_t aabb_aabb_z_feature = 0xA003U;
constexpr std::uint32_t sphere_sphere_feature = 0xB001U;
constexpr std::uint32_t sphere_aabb_feature = 0xB002U;
constexpr std::uint32_t capsule_sphere_feature = 0xC001U;
constexpr std::uint32_t capsule_aabb_feature = 0xC002U;
constexpr std::uint32_t capsule_capsule_feature = 0xC003U;
```

Derive `point` from the current closest/contact point used by each shape helper. For AABB/AABB, use the midpoint of the overlapping slab center on the contact plane.

- [x] **Step 2: Sort manifolds and points deterministically**

Before returning from `contact_manifolds()`:

```cpp
std::ranges::sort(result, [](const auto& lhs, const auto& rhs) {
    if (lhs.first.value != rhs.first.value) {
        return lhs.first.value < rhs.first.value;
    }
    return lhs.second.value < rhs.second.value;
});
for (auto& manifold : result) {
    std::ranges::sort(manifold.points, [](const auto& lhs, const auto& rhs) {
        return lhs.feature_id < rhs.feature_id;
    });
}
```

- [x] **Step 3: Make `contacts()` flatten manifolds**

Implement `contacts()` from `contact_manifolds()`:

```cpp
std::vector<PhysicsContact3D> PhysicsWorld3D::contacts() const {
    std::vector<PhysicsContact3D> result;
    for (const auto& manifold : contact_manifolds()) {
        for (const auto& point : manifold.points) {
            result.push_back(PhysicsContact3D{manifold.first, manifold.second, manifold.normal,
                                              point.penetration_depth});
        }
    }
    return result;
}
```

- [x] **Step 4: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: PASS for existing contact tests and the new manifold contract test.

## Task 3: Persistence And Replay Evidence

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/src/physics3d.cpp`

- [x] **Step 1: Add RED tests for stable feature ids**

Add a test that creates two worlds with the same AABB/AABB and capsule/sphere contacts, shifts the dynamic body by `0.01F` along a non-separating tangent, and checks:

```cpp
MK_REQUIRE(before[0].points[0].feature_id == after[0].points[0].feature_id);
MK_REQUIRE(before[0].points[0].warm_start_eligible);
MK_REQUIRE(after[0].points[0].warm_start_eligible);
```

- [x] **Step 2: Add RED tests for trigger and disabled exclusion**

Use one trigger and one disabled body overlapping a dynamic body, then require:

```cpp
MK_REQUIRE(world.contact_manifolds().empty());
MK_REQUIRE(world.contacts().empty());
```

- [x] **Step 3: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: PASS after manifold generation keeps feature ids stable and continues excluding trigger/disabled contacts.

## Task 4: Solver Consumption

**Files:**
- Modify: `engine/physics/src/physics3d.cpp`
- Modify: `tests/unit/core_tests.cpp`

- [x] **Step 1: Make `resolve_contacts` consume manifolds**

Replace `const auto pending_contacts = contacts();` with `const auto pending_manifolds = contact_manifolds();` and apply the existing inverse-mass correction/impulse logic per `PhysicsContactPoint3D` while using the manifold normal.

- [x] **Step 2: Preserve sliding tangent velocity**

Add a test with a dynamic box overlapping a static floor and moving on X:

```cpp
MK_REQUIRE(std::abs(world.find_body(box)->velocity.x - 2.0F) < 0.0001F);
MK_REQUIRE(std::abs(world.find_body(box)->velocity.y) < 0.0001F);
```

after `resolve_contacts(mirakana::PhysicsContactSolver3DConfig{0.0F, 8U, 0.85F, 0.001F});`.

- [x] **Step 3: Prove deterministic stacked replay**

Extend the current stacked-body replay test to compare `contact_manifolds()` counts, body ids, normals, feature ids, and `warm_start_eligible` flags across two identical worlds before and after solving.

- [x] **Step 4: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: PASS for existing solver tests and new manifold replay/sliding tests.

## Task 5: Docs, Manifest, Skills, And Static Checks

**Files:**
- Modify: `docs/architecture.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] **Step 1: Update capability language after behavior is green**

Document this exact claim:

```text
PhysicsWorld3D::contact_manifolds exposes deterministic contact manifolds with stable feature ids, warm-start-safe point rows, trigger/disabled exclusion, and solver consumption for current AABB/sphere/vertical-capsule contacts.
```

Keep these unsupported:

```text
CCD, joints, dynamic controller push/step policy, optional Jolt/native backends, oriented boxes, mesh/convex casts, physics benchmarks, and broad physics readiness.
```

- [x] **Step 2: Move active pointers on completion**

When implementation is complete, mark this plan `Completed`, update the registry latest completed row, and move `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to the next master-plan child or back to the master plan with `recommendedNextPlan.id=next-production-gap-selection`.

- [x] **Step 3: Run static checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

Expected: PASS.

## Task 6: Full Verification And Closeout

**Files:**
- Modify: `docs/superpowers/plans/2026-05-09-physics-contact-manifold-stability-v1.md`

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
| `cmake --build --preset dev --target MK_core_tests --config Debug` after adding the first RED test | Expected failure | `PhysicsWorld3D` did not expose `contact_manifolds` yet. |
| `cmake --build --preset dev --target MK_core_tests --config Debug && ctest --preset dev -R MK_core_tests --output-on-failure` after a minimal stub | Expected failure | New manifold test failed because the manifold list was empty. |
| `cmake --build --preset dev --target MK_core_tests --config Debug && ctest --preset dev -R MK_core_tests --output-on-failure` after generic point generation | Expected failure | Shape-pair closest-feature test caught the sphere/AABB contact point mismatch. |
| `cmake --build --preset dev --target MK_core_tests --config Debug; ctest --preset dev -R MK_core_tests --output-on-failure` | PASS | `MK_core_tests` passed, including deterministic manifold rows, closest feature point, feature-id persistence, trigger/disabled exclusion, solver tangent preservation, invalid solver config rejection, and manifold replay checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format: ok`; `format-check: ok`. |
| `git diff --check` | PASS | No whitespace errors; Git reported only existing line-ending conversion warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; `unsupported_gaps=11` remains honest non-ready evidence. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest reported 29/29 passing. Diagnostic-only Metal/Apple host gates remained blocked on this Windows host as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Debug build completed through engine libraries, tests, and sample executables. |

## Self-Review

- Spec coverage: this plan covers the third Physics 1.0 child only: contact manifold stability and solver-input rows.
- Exclusions: CCD, joints, character/dynamic policy, benchmark gates, editor UX, optional Jolt/native backend, oriented boxes, mesh/convex casts, and broad physics readiness are intentionally deferred.
- Placeholder scan: no task contains deferred placeholder markers; each task names files, commands, and expected outcomes.
- Type consistency: public names use `PhysicsContactPoint3D`, `PhysicsContactManifold3D`, and `PhysicsWorld3D::contact_manifolds` consistently.
