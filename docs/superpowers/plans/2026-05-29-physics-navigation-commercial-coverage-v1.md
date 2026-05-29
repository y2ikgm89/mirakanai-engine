# Physics Navigation Commercial Coverage v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand the physics/navigation production breadth review gates so commercial optional-adapter evidence is explicit, host-gated, dependency-recorded, and native-handle-free before any broader Jolt/Recast/Detour readiness claim.

**Architecture:** Keep gameplay-facing APIs first-party and value-only in `MK_physics` and `MK_navigation`. This slice does not execute Jolt, Recast, Detour, native jobs, navmesh baking, vehicle simulation, runtime source import, or package mutation; it adds stricter review evidence fields and fail-closed diagnostics around optional adapter lifecycle, host validation recipes, dependency/legal records, and official-source coverage.

**Tech Stack:** C++23, CMake/CTest, `MK_physics`, `MK_navigation`, Context7 `/jrouwe/joltphysics`, Context7 `/websites/recastnav`, PowerShell validation tools, composed engine agent manifest.

---

**Plan ID:** `physics-navigation-commercial-coverage-v1`

**Status:** Active.

**Date:** 2026-05-29

## Context

- Current selection gate: `engine/agent/manifest.json.aiOperableProductionLoop.recommendedNextPlan.id = next-production-gap-selection` before this plan.
- Official-source refresh:
  - Jolt Context7 library: `/jrouwe/joltphysics`
  - Recast Navigation Context7 library: `/websites/recastnav`
  - Jolt evidence to model: allocator/factory/type registration, temp allocator, job system, collision layers, narrow-phase ray/cast queries, constraints, `CharacterVirtual`, and vehicle constraints.
  - Recast/Detour evidence to model: Recast build config/heightfield/contours/poly mesh, Detour navmesh query/path corridor, DetourTileCache dynamic obstacles, and DetourCrowd local avoidance.
- Clean-break constraint: no backward-compatibility shims, no SDL3, no public Jolt/Recast/Detour handles, and no broad middleware parity claim without host/tool/dependency/legal/package evidence.

## Done When

- `PhysicsProductionBreadthEvidenceRow` and `NavigationProductionBreadthEvidenceRow` require explicit optional-adapter boundary, host validation recipe, and lifecycle review evidence for optional backend rows.
- Missing adapter boundary, missing host validation recipe, and missing adapter lifecycle proof fail closed with deterministic diagnostics.
- Existing complete first-party rows remain source compatible inside this task after all designated initializers are updated in declaration order.
- Tests prove RED/GREEN for both physics and navigation optional-adapter evidence gates.
- Plan registry, production backlog/projection docs, manifest fragments, composed manifest, and static AI checks match the implemented contract.
- Focused physics/navigation test target and `tools/validate.ps1` pass before commit/PR, or an exact host/tool blocker is recorded.

## Task 1: Select This Plan

**Files:**
- Create: `docs/superpowers/plans/2026-05-29-physics-navigation-commercial-coverage-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`

- [x] **Step 1: Add the dated plan file**

Write this plan with the Goal, Context, Constraints, Done When, tasks, and validation evidence sections.

- [x] **Step 2: Update the registry**

Set Current Active Work to include this plan as the active selected slice and keep the completed renderer/runtime UI/platform/importer rows closed.

- [x] **Step 3: Update the production backlog and projection chapters**

Add `physics-navigation-commercial-coverage-v1` as the selected production gameplay slice. The projection chapter must say this slice is active and must not reopen Engine 1.0 or claim broad Jolt/Recast/Detour parity.

- [x] **Step 4: Update and compose the manifest**

Set `currentActivePlan` and `recommendedNextPlan` to this plan in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, then run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Expected: `engine/agent/manifest.json` is regenerated from fragments.

## Task 2: Add RED Tests For Optional Adapter Evidence Gates

**Files:**
- Modify: `tests/unit/physics_navigation_production_breadth_tests.cpp`

- [x] **Step 1: Add a failing physics test**

Add a test named `physics production breadth review requires optional adapter boundary evidence` that creates an optional native adapter row with dependency/legal and host-gated evidence but leaves `adapter_boundary_id`, `host_validation_recipe_id`, and `adapter_lifecycle_reviewed` empty/false.

Expected diagnostics:

```cpp
MK_REQUIRE(review.diagnostics[0] == mirakana::PhysicsProductionBreadthDiagnostic::missing_adapter_boundary);
MK_REQUIRE(review.diagnostics[1] == mirakana::PhysicsProductionBreadthDiagnostic::missing_host_validation_recipe);
MK_REQUIRE(review.diagnostics[2] == mirakana::PhysicsProductionBreadthDiagnostic::missing_adapter_lifecycle_review);
MK_REQUIRE(review.diagnostics[3] == mirakana::PhysicsProductionBreadthDiagnostic::missing_required_feature);
```

- [x] **Step 2: Add a failing navigation test**

Add a test named `navigation production breadth review requires recast detour adapter boundary evidence` with the same pattern for `NavigationProductionBreadthProof::optional_recast_detour_adapter`.

Expected diagnostics:

```cpp
MK_REQUIRE(review.diagnostics[0] == mirakana::NavigationProductionBreadthDiagnostic::missing_adapter_boundary);
MK_REQUIRE(review.diagnostics[1] == mirakana::NavigationProductionBreadthDiagnostic::missing_host_validation_recipe);
MK_REQUIRE(review.diagnostics[2] == mirakana::NavigationProductionBreadthDiagnostic::missing_adapter_lifecycle_review);
MK_REQUIRE(review.diagnostics[3] == mirakana::NavigationProductionBreadthDiagnostic::missing_required_feature);
```

- [x] **Step 3: Verify RED**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_physics_navigation_production_breadth_tests
```

Expected: build fails because the new diagnostics and row fields do not exist yet.

## Task 3: Implement The Evidence Gate

**Files:**
- Modify: `engine/physics/include/mirakana/physics/physics_production_breadth.hpp`
- Modify: `engine/physics/src/physics_production_breadth.cpp`
- Modify: `engine/navigation/include/mirakana/navigation/navigation_production_breadth.hpp`
- Modify: `engine/navigation/src/navigation_production_breadth.cpp`
- Modify: `tests/unit/physics_navigation_production_breadth_tests.cpp`

- [x] **Step 1: Extend public value rows**

Add these fields after `package_counter_id` in both evidence row structs:

```cpp
std::string adapter_boundary_id;
std::string host_validation_recipe_id;
bool adapter_lifecycle_reviewed{false};
```

Add diagnostics to both diagnostic enums:

```cpp
missing_adapter_boundary,
missing_host_validation_recipe,
missing_adapter_lifecycle_review,
```

- [x] **Step 2: Validate optional adapter rows**

For `PhysicsProductionBreadthProof::optional_native_adapter` and `NavigationProductionBreadthProof::optional_recast_detour_adapter`, fail closed when any new field is absent. Hash the new strings and boolean plus existing safety booleans so the review hash changes when adapter evidence changes.

- [x] **Step 3: Update existing tests**

Update helper row designated initializers in declaration order. Positive optional-adapter tests must set:

```cpp
.adapter_boundary_id = "MK_physics_jolt",
.host_validation_recipe_id = "validate-physics-jolt",
.adapter_lifecycle_reviewed = true,
```

and:

```cpp
.adapter_boundary_id = "RecastDetourPrivateAdapter",
.host_validation_recipe_id = "validate-navigation-recast-detour",
.adapter_lifecycle_reviewed = true,
```

- [x] **Step 4: Verify GREEN**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_physics_navigation_production_breadth_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "physics_navigation_production_breadth"
```

Expected: target builds and all physics/navigation production breadth tests pass.

## Task 4: Sync Docs, Manifest, And Static Checks

**Files:**
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify: `tools/check-ai-integration-101-physics-navigation-production-breadth.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Generate: `engine/agent/manifest.json`

- [x] **Step 1: Update agent-facing capability text**

Mention `adapter_boundary_id`, `host_validation_recipe_id`, and `adapter_lifecycle_reviewed` where the production breadth gates are described. Keep all native handles and broad middleware parity claims explicitly unsupported.

- [x] **Step 2: Update static guard needles**

Add needles for the three new fields and the three new diagnostics in `tools/check-ai-integration-101-physics-navigation-production-breadth.ps1`.

- [x] **Step 3: Compose manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Expected: composed manifest contains the updated physics/navigation gate contract.

## Task 5: Validate, Commit, Publish, And Close

**Files:**
- All task-owned files above.

- [x] **Step 1: Run focused validation**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

- [x] **Step 2: Run full validation**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] **Step 3: Create validated commit and PR**

Commit only task-owned files. Push `codex/physics-navigation-commercial-coverage-v1`, create a focused PR with local validation evidence, wait for hosted checks, use `gh pr merge --auto --merge --match-head-commit <headRefOid>` after checks are clean, and remove the merged worktree with `tools/remove-merged-worktree.ps1`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| Context7 `/jrouwe/joltphysics` | Pass | Official Jolt initialization/query/character/vehicle docs refreshed before design. |
| Context7 `/websites/recastnav` | Pass | Official Recast/Detour module/build/query/tile-cache/crowd docs refreshed before design. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_physics_navigation_production_breadth_tests` | RED | Failed before implementation because `missing_adapter_boundary`, `missing_host_validation_recipe`, and `missing_adapter_lifecycle_review` diagnostics did not exist. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_physics_navigation_production_breadth_tests` | Pass | Target built after value-row/diagnostic implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "physics_navigation_production_breadth"` | Pass | Focused production breadth tests passed after optional adapter diagnostic expectations included the existing `missing_required_feature` aggregate diagnostic. |
| `git diff --check` | Pass | No whitespace errors in the task-owned diff. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Text and clang-format checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | Manifest compose and JSON contract checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | AI integration checks passed with the selected physics/navigation plan and new adapter-evidence needles. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Public API boundary check passed after the physics/navigation value-row additions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_physics_navigation_production_breadth_tests` | Pass | Focused target rebuilt after final static-check edits. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "physics_navigation_production_breadth"` | Pass | 1/1 focused CTest passed after final static-check edits. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full validation passed: 19 static checks, build, tidy smoke, and 79/79 CTest tests passed; Apple/Metal remained diagnostic-only host-gated on this Windows host. |
