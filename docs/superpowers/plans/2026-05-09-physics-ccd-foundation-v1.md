# Physics CCD Foundation Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Add a narrow, deterministic 3D continuous-collision-detection path for accepted fast-moving first-party bodies without changing the default fixed-step replay contract.

**Architecture:** Keep CCD inside dependency-free `MK_physics` value types and `PhysicsWorld3D`. Reuse the exact AABB/sphere/vertical-capsule sweep path for dynamic-body motion, report explicit per-body CCD rows, and keep default `PhysicsWorld3D::step` discrete so existing replay behavior does not silently change.

**Tech Stack:** C++23, `MK_physics`, `MK_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.

---

**Plan ID:** `physics-ccd-foundation-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Previous Slice:** [2026-05-09-physics-contact-manifold-stability-v1.md](2026-05-09-physics-contact-manifold-stability-v1.md)

## Context

- `physics-broader-exact-casts-and-sweeps-v1` added exact AABB/sphere/vertical-capsule sweeps through `PhysicsWorld3D::exact_shape_sweep`.
- `physics-contact-manifold-stability-v1` added deterministic contact manifolds through `PhysicsContactPoint3D`, `PhysicsContactManifold3D`, and `PhysicsWorld3D::contact_manifolds`.
- The current `PhysicsWorld3D::step` integrates dynamic bodies discretely. Fast bodies can pass through thin static geometry when the whole displacement crosses a collider between frames.
- This slice should add a focused, explicit CCD integration entrypoint. It should not make default discrete stepping silently more expensive or change existing deterministic replay tests.

## Constraints

- Keep `engine/physics` independent from OS, GPU, editor, asset formats, and middleware.
- Do not add third-party dependencies or native backend handles.
- Limit first CCD support to dynamic 3D AABB/sphere/vertical-capsule bodies swept against currently enabled, non-trigger first-party AABB/sphere/capsule targets.
- Do not claim dynamic-vs-dynamic time-of-impact, rotational CCD, 2D CCD, oriented boxes, mesh/convex casts, joints, dynamic controller push/step policy, Jolt/native backends, or broad physics readiness.
- Preserve `PhysicsWorld3D::step(float)` behavior. CCD must be opt-in through a new explicit API.
- Use `PhysicsWorld3D::exact_shape_sweep` rather than reimplementing shape-pair sweep math.

## Done When

- `PhysicsWorld3D` exposes an explicit CCD step result with deterministic per-body rows.
- Fast dynamic AABB/sphere/capsule bodies stop at the accepted sweep hit instead of tunneling through static AABB/sphere/capsule targets.
- CCD rows report previous position, attempted displacement, applied displacement, remaining displacement, hit body, hit normal, hit distance, and whether CCD was applied.
- CCD ignores the moving body itself, skips disabled targets, excludes triggers by default, honors collision layer/mask filtering, and rejects invalid config values.
- Default `PhysicsWorld3D::step` remains discrete and existing replay tests stay green.
- Tests prove no-tunnel behavior, no-hit discrete equivalence, trigger exclusion, mask/ignored behavior, deterministic row ordering, and invalid-request diagnostics.
- Docs, manifest, skills, registry, and the master plan say CCD foundation is implemented while dynamic push/step policy, joints, optional Jolt/native backends, 2D CCD, dynamic-vs-dynamic TOI, rotational CCD, oriented boxes, mesh/convex casts, physics benchmarks, and broad physics readiness remain unsupported.
- Focused build/test commands, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/tool blocker.

## File Structure

- Modify `engine/physics/include/mirakana/physics/physics3d.hpp`: add CCD status/diagnostic enums, config/result value types, and the explicit `PhysicsWorld3D::step_continuous` API.
- Modify `engine/physics/src/physics3d.cpp`: implement validation, dynamic-body sweep conversion, CCD integration, and deterministic result ordering.
- Modify `tests/unit/core_tests.cpp`: add RED/GREEN CCD tests beside the 3D physics step, exact sweep, contact manifold, and solver tests.
- Modify docs/guidance after behavior is green: `docs/architecture.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/testing.md`, `docs/superpowers/plans/README.md`, this master plan, `engine/agent/manifest.json`, `.agents/skills/gameengine-game-development/SKILL.md`, and `.claude/skills/gameengine-game-development/SKILL.md`.

## Task 1: Public CCD Contract

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`

- [x] **Step 1: Add RED tests for a public CCD result row**

Add a test near the existing `3d physics integration applies gravity forces and damping` block:

```cpp
MK_TEST("3d physics continuous step reports fast body hit rows") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{0.0F, 0.0F, 0.0F}});
    const auto wall = world.create_body(mirakana::PhysicsBody3DDesc{
        mirakana::Vec3{5.0F, 0.0F, 0.0F},
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        0.0F,
        0.0F,
        false,
        mirakana::Vec3{0.05F, 2.0F, 2.0F},
    });
    const auto bullet = world.create_body(mirakana::PhysicsBody3DDesc{
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        mirakana::Vec3{20.0F, 0.0F, 0.0F},
        1.0F,
        0.0F,
        true,
        mirakana::Vec3{0.1F, 0.1F, 0.1F},
        true,
        mirakana::PhysicsShape3DKind::sphere,
        0.1F,
    });

    const auto result = world.step_continuous(0.5F);

    MK_REQUIRE(result.status == mirakana::PhysicsContinuousStep3DStatus::stepped);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsContinuousStep3DDiagnostic::none);
    MK_REQUIRE(result.rows.size() == 1);
    MK_REQUIRE(result.rows[0].body == bullet);
    MK_REQUIRE(result.rows[0].hit_body == wall);
    MK_REQUIRE(result.rows[0].ccd_applied);
    MK_REQUIRE(result.rows[0].hit.has_value());
    MK_REQUIRE(result.rows[0].hit->body == wall);
    MK_REQUIRE(result.rows[0].hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(world.find_body(bullet)->position.x < 5.0F);
}
```

- [x] **Step 2: Run the focused build and confirm RED**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
```

Expected: FAIL to compile because the CCD value types and `PhysicsWorld3D::step_continuous` do not exist yet.

- [x] **Step 3: Add public CCD value types**

Add declarations near the existing 3D step/query types:

```cpp
enum class PhysicsContinuousStep3DStatus { stepped, invalid_request };

enum class PhysicsContinuousStep3DDiagnostic { none, invalid_delta_seconds, invalid_config };

struct PhysicsContinuousStep3DConfig {
    float skin_width{0.001F};
    bool include_triggers{false};
};

struct PhysicsContinuousStep3DRow {
    PhysicsBody3DId body;
    Vec3 previous_position{0.0F, 0.0F, 0.0F};
    Vec3 attempted_displacement{0.0F, 0.0F, 0.0F};
    Vec3 applied_displacement{0.0F, 0.0F, 0.0F};
    Vec3 remaining_displacement{0.0F, 0.0F, 0.0F};
    PhysicsBody3DId hit_body{};
    std::optional<PhysicsExactShapeSweep3DHit> hit;
    bool ccd_applied{false};
};

struct PhysicsContinuousStep3DResult {
    PhysicsContinuousStep3DStatus status{PhysicsContinuousStep3DStatus::invalid_request};
    PhysicsContinuousStep3DDiagnostic diagnostic{PhysicsContinuousStep3DDiagnostic::none};
    std::vector<PhysicsContinuousStep3DRow> rows;
};
```

Add:

```cpp
[[nodiscard]] PhysicsContinuousStep3DResult step_continuous(
    float delta_seconds,
    PhysicsContinuousStep3DConfig config = {});
```

- [x] **Step 4: Build with a minimal invalid stub and confirm runtime RED**

Return `PhysicsContinuousStep3DResult{PhysicsContinuousStep3DStatus::stepped}` from `step_continuous`, then run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: build succeeds and the new test fails because no row or hit is reported.

## Task 2: Dynamic Body Sweep Integration

**Files:**
- Modify: `engine/physics/src/physics3d.cpp`
- Modify: `tests/unit/core_tests.cpp`

- [x] **Step 1: Convert dynamic body state into exact sweep requests**

Add a helper that maps `PhysicsBody3D` to `PhysicsShape3DDesc`:

```cpp
[[nodiscard]] PhysicsShape3DDesc shape_desc_for_body(const PhysicsBody3D& body) noexcept {
    switch (body.shape) {
    case PhysicsShape3DKind::sphere:
        return PhysicsShape3DDesc::sphere(body.radius);
    case PhysicsShape3DKind::capsule:
        return PhysicsShape3DDesc::capsule(body.radius, body.half_height);
    case PhysicsShape3DKind::aabb:
    default:
        return PhysicsShape3DDesc::aabb(body.half_extents);
    }
}
```

- [x] **Step 2: Implement one-iteration CCD movement**

For each dynamic, enabled, non-trigger body in body-id order:

1. Integrate velocity exactly as `step` does: gravity, accumulated force, damping.
2. Compute `attempted_displacement = velocity * delta_seconds`.
3. If the displacement length is zero, keep position and emit a row with `ccd_applied=false`.
4. Call `exact_shape_sweep` with:
   - `origin = previous_position`
   - normalized displacement direction
   - `max_distance = length(attempted_displacement)`
   - `shape = shape_desc_for_body(body)`
   - `filter.ignored_body = body.id`
   - `filter.include_triggers = config.include_triggers`
   - `filter.collision_mask = body.collision_mask`
5. On `hit`, move to `previous_position + direction * max(0, hit.distance - config.skin_width)`.
6. Remove only the velocity component into the hit normal: `velocity -= normal * min(0, dot(velocity, normal))`.
7. Clear accumulated force at the end of the row.

- [x] **Step 3: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: PASS for the public CCD row test and existing 3D step tests.

## Task 3: Filtering, Triggers, And Invalid Config

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/src/physics3d.cpp`

- [x] **Step 1: Add RED tests for no-hit equivalence and filtering**

Add one test that creates a fast body with no collider in its path, calls both `step(0.25F)` and `step_continuous(0.25F)` on equivalent worlds, and requires matching final positions and velocities.

Add another test that places a trigger and a masked-out static wall in the path and requires `step_continuous` to ignore them by default while still reporting deterministic rows.

- [x] **Step 2: Add RED tests for invalid request diagnostics**

Check these invalid inputs:

```cpp
MK_REQUIRE(world.step_continuous(-0.01F).diagnostic ==
           mirakana::PhysicsContinuousStep3DDiagnostic::invalid_delta_seconds);
MK_REQUIRE(world.step_continuous(0.016F, mirakana::PhysicsContinuousStep3DConfig{-1.0F}).diagnostic ==
           mirakana::PhysicsContinuousStep3DDiagnostic::invalid_config);
```

- [x] **Step 3: Implement validation and filter behavior**

Return `invalid_request` without mutating bodies when:

- `delta_seconds` is non-finite or negative.
- `skin_width` is non-finite or negative.

Preserve body positions, velocities, and accumulated forces on invalid requests.

- [x] **Step 4: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: PASS for no-hit equivalence, trigger/mask behavior, and invalid diagnostics.

## Task 4: Determinism And Narrow Scope Evidence

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/physics/src/physics3d.cpp`

- [x] **Step 1: Add RED tests for deterministic row ordering**

Create two worlds with the same three dynamic bodies inserted in the same order. Call `step_continuous(0.25F)` and compare:

```cpp
MK_REQUIRE(first.rows.size() == second.rows.size());
for (std::size_t index = 0; index < first.rows.size(); ++index) {
    MK_REQUIRE(first.rows[index].body == second.rows[index].body);
    MK_REQUIRE(first.rows[index].hit_body == second.rows[index].hit_body);
    MK_REQUIRE(first.rows[index].ccd_applied == second.rows[index].ccd_applied);
    MK_REQUIRE(first.rows[index].applied_displacement == second.rows[index].applied_displacement);
}
```

- [x] **Step 2: Add RED tests that default `step` remains discrete**

Use the same fast-body/wall setup and verify `step(0.5F)` still moves the body past the wall while `step_continuous(0.5F)` stops it before the wall. This protects backward behavioral honesty: CCD is opt-in, not a silent default-step change.

- [x] **Step 3: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests --config Debug
ctest --preset dev -R MK_core_tests --output-on-failure
```

Expected: PASS for deterministic rows and default-step scope tests.

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
PhysicsWorld3D::step_continuous exposes opt-in deterministic 3D CCD rows for fast dynamic AABB/sphere/vertical-capsule bodies swept against current non-trigger AABB/sphere/capsule targets, while PhysicsWorld3D::step remains discrete.
```

Keep these unsupported:

```text
dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, joints, dynamic controller push/step policy, optional Jolt/native backends, oriented boxes, mesh/convex casts, physics benchmarks, and broad physics readiness.
```

- [x] **Step 2: Move active pointers on completion**

When implementation is complete, mark this plan `Completed`, update the registry latest completed row, and move `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to the next master-plan child: `physics-character-dynamic-interaction-policy-v1`.

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
- Modify: `docs/superpowers/plans/2026-05-09-physics-ccd-foundation-v1.md`

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

## Self-Review

- Spec coverage: this plan covers only the next Physics 1.0 child: opt-in CCD foundation for accepted fast-moving 3D body cases.
- Exclusions: dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, joints, character/dynamic policy, benchmark gates, optional Jolt/native backend, oriented boxes, mesh/convex casts, and broad physics readiness are intentionally deferred.
- Placeholder scan: no task contains deferred placeholder markers; each task names files, commands, expected outcomes, and concrete API/test shapes.
- Type consistency: public names use `PhysicsContinuousStep3D*` and `PhysicsWorld3D::step_continuous` consistently.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_core_tests --config Debug` | PASS | Focused CCD/test target build. |
| `ctest --preset dev -R MK_core_tests --output-on-failure` | PASS | `MK_core_tests` passed, including continuous-step no-tunnel, no-hit equivalence, trigger/mask filtering, invalid diagnostics, tangent preservation, shape coverage, deterministic rows, and default-step discrete preservation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public physics API boundary accepted. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting check accepted. |
| `git diff --check` | PASS | Whitespace check accepted; Git reported expected LF-to-CRLF working-copy warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest/schema contract sync accepted with `physics-character-dynamic-interaction-policy-v1` as the only active child plan. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent guidance and plan pointer sync accepted after adding `PhysicsWorld3D::step_continuous` guidance. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Governance audit still reports 11 known unsupported gaps; CCD completion does not promote broad 1.0 readiness. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | TIMEOUT | Standalone full tidy run timed out after 15 minutes on `editor/core/src/scene_authoring.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` later ran the default tidy smoke successfully. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed, including build, 29/29 CTest tests, and default `tidy-check: ok (1 files)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default build completed. |
