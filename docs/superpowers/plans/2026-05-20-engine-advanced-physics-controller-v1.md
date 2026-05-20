# Engine Advanced Physics Controller v1 (2026-05-20)

**Plan ID:** `engine-advanced-physics-controller-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is developer-owned gameplay/physics capability work, not a reopened Engine 1.0 production gap.

## Goal

Extend first-party physics controller primitives so generated movement-heavy games can combine character movement, shape sweeps, contact/trigger interpretation, simple constraints, moving-platform handoff, and deterministic replay diagnostics without exposing middleware or native physics handles.

## Context

- `engine-navmesh-crowd-v1` completed deterministic navmesh/crowd query rows and selected generated 3D package-visible counters.
- Existing `MK_physics` work already covers 2D/3D worlds, triggers, broadphase contacts, exact primitive casts/sweeps, contact manifolds, 3D CCD rows, character controller movement, character/dynamic interaction policy rows, distance-joint solving, collision query batches, and selected package evidence.
- The developer-owned capability backlog lists `engine-advanced-physics-controller-v1` as the next gameplay-family enabler for movement-heavy, action, platforming, vehicle, and physical-world games.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep optional middleware, public native physics handles, full vehicle physics, ragdolls, broad solver benchmarks, and game-specific movement feel out of scope unless a later dated optional-adapter plan accepts them.
- Keep game-specific controller tuning, abilities, damage rules, and encounter logic in game-owned code/data.
- Reuse existing `MK_physics` body/query/contact/controller primitives where possible; use clean-break changes if existing contracts block deterministic generated-game rows.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the next active developer-owned gameplay/physics capability after `engine-navmesh-crowd-v1` closeout.

### Done When

- `docs/superpowers/plans/README.md`, the readiness ledger, and the production master-plan index list this plan as the active developer-owned milestone.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Controller Movement Contract

**Status:** Completed.

### Goal

Define the smallest reusable generated-game controller contract over existing shape sweep, contact manifold, trigger, dynamic interaction, and constraint primitives, with deterministic rows and explicit diagnostics.

### Done When

- RED tests or static guards fail first for missing advanced controller movement behavior.
- `MK_physics` exposes value-only request/result/diagnostic rows for generated-game controller planning.
- Focused tests prove deterministic movement rows, wall/ground/trigger handling, moving-platform handoff intent, constraint participation, invalid-request diagnostics, and stable replay counters.

## Phase 2: Package Movement Evidence

**Status:** Completed.

### Goal

Adopt the controller movement contract in selected package paths so smokes can report deterministic advanced movement counters.

### Done When

- A committed sample or generated package path reports package-visible advanced movement counters through existing validation recipes.
- Docs, manifest fragments, schemas/static checks, and agent surfaces are updated for durable workflow or AI-operable contract changes.
- Full `tools/validate.ps1` passes at the coherent phase gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 pointer sync: this plan was selected after `engine-navmesh-crowd-v1` completed deterministic navmesh/crowd rows and package-visible `gameplay_systems_navigation_crowd_*` counters, while `unsupportedProductionGaps = []` stayed empty.
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` failed before implementation because `PhysicsAdvancedController3DDesc`, `PhysicsMovingPlatform3DDesc`, `PhysicsAdvancedController3DStatus`, `PhysicsAdvancedController3DDiagnostic`, and `plan_physics_advanced_controller_3d` were missing.
- Phase 1 focused validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp,tests/unit/core_tests.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Phase 1 gate validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on Windows; expected diagnostic-only host gates remain for Metal/iOS tooling on this Windows host, and `unsupportedProductionGaps = []` remained empty.
- Phase 2 RED: the installed `sample_generated_desktop_runtime_3d_package` gameplay-systems smoke failed after `tools/validate-installed-desktop-runtime.ps1` required `gameplay_systems_advanced_controller_*` counters and before the generated package emitted them.
- Phase 2 focused validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_generated_desktop_runtime_3d_package`; `out/build/dev/games/Debug/sample_generated_desktop_runtime_3d_package/sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-gameplay-systems`; `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` with the selected gameplay-systems smoke args; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files games/sample_generated_desktop_runtime_3d_package/main.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`; and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Phase 2 gate validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on Windows with `validate: ok`; `production-readiness-audit` reported `unsupported_gaps=0`.
