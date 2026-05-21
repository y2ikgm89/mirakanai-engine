# Post-1.0 Capability Program v1 (2026-05-22)

**Plan ID:** `post-1-0-capability-program-v1`
**Status:** Active.
**Milestone type:** Post-1.0 / 1.x phase-gated capability program.
**Current phase:** Phase 1 - `physics-constraints-and-joints-v1`.

## Goal

Select and execute the next coherent post-1.0 / 1.x engine capability program after the 1.0 closeout backlog reached `unsupportedProductionGaps = []`.

This plan keeps the production-completion pointer on one active dated milestone while covering the remaining 2D/3D, physics, navigation, AI, and high-freedom game-creation candidate streams that were previously listed as post-1.0 / 1.x follow-ups.

## Context

- The 1.0 Windows-default closeout surface remains zero-gap: `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps = []`.
- The previous selection-gate state intentionally pointed `currentActivePlan` back to the production-completion master plan until a concrete roadmap decision selected the next developer-owned capability.
- The selected decision is to keep the post-1.0 / 1.x candidate backlog active as a single phase-gated milestone, starting with `physics-constraints-and-joints-v1`.
- Existing completed foundations remain evidence, not active work: `physics-collision-query-v1`, `physics-joints-foundation-v1`, `engine-advanced-physics-controller-v1`, `engine-navmesh-crowd-v1`, `ai-perception-services-v1`, `gameplay-simulation-orchestration-v1`, `engine-world-region-streaming-v1`, `engine-entity-scale-and-culling-v1`, and `ai-gameplay-authoring-tools-v1`.

## Constraints

- Do not re-open 1.0 readiness rows. New work in this plan is post-1.0 / 1.x unless a later plan explicitly promotes a phase into a release-ready definition.
- Keep `unsupportedProductionGaps = []`; selected post-1.0 work must not be represented as an unsupported 1.0 blocker.
- Use first-party value contracts in engine modules. Middleware such as Jolt, PhysX, Recast/Detour, Havok, or scripting runtimes requires a separate optional-adapter plan with dependency, legal, manifest, schema, and validation updates before adoption.
- Use tests first for production behavior/API changes when the local environment can run them.
- Keep public APIs free of backend, platform, editor, renderer, SDL3, and middleware-native handles.
- Update agent-operable surfaces only when durable behavior, validation recipes, manifest claims, or command contracts change.

## Candidate Map

| Phase | Capability row | Plan action | Done boundary |
| --- | --- | --- | --- |
| 0 | `post-1-0-capability-program-v1` | Select this milestone, update registry/master-plan/matrix/advanced-track/current-capabilities/manifest pointers. | Focused docs/manifest checks pass and `currentActivePlan` points here. |
| 1 | `physics-constraints-and-joints-v1` | Extend first-party `MK_physics` joint/constraint evidence beyond the completed distance-joint foundation. | Deterministic constraint rows, invalid-input diagnostics, package-visible counters, and focused/full validation evidence. |
| 2 | `physics-vehicles-and-kinematics-v1` | Add kinematic body and simple vehicle policy foundations for small games. | Fixed-step movement policy, interaction diagnostics, package-visible evidence, and explicit non-goals for AAA vehicle physics. |
| 3 | `navigation-hierarchical-world-v1` | Add region/portal navigation organization over existing navmesh/crowd foundations. | Region/portal assets, deterministic missing-region/cache diagnostics, safe streaming ownership, and package evidence. |
| 4 | `world-streaming-and-large-scenes-v1` plus `simulation-persistence-v1` | Connect larger-world runtime evidence with persistence and save/resume contracts. | Region/package/save-state evidence, migration diagnostics, and no open-world or binary compatibility overclaim. |
| 5 | AI game-creation follow-ons | Select from `ai-game-design-spec-v1`, `ai-game-generation-orchestrator-v1`, `ai-validation-remediation-recipes-v1`, and `ai-engine-capability-handoff-v1` after the engine phases above expose stable contracts. | Generated-game workflows produce reviewed game-owned files and hand off missing engine features instead of mutating engine internals. |

## Phase 0 - Plan Surface Sync

### Work

- [x] Create this active milestone plan.
- [x] Update the plan registry current active work row.
- [x] Update the production-completion master-plan verdict.
- [x] Update the 2D/3D matrix so completed foundations are not described as active 1.0 gaps.
- [x] Update the gameplay physics/navigation/AI advanced track with the selected Phase 1 stream and completed lower-level foundations.
- [x] Update current-capabilities active-work text.
- [x] Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, compose `engine/agent/manifest.json`, and keep `unsupportedProductionGaps = []`.

### Done When

- `currentActivePlan` points to this plan.
- `recommendedNextPlan.id = physics-constraints-and-joints-v1`.
- Related planning surfaces agree that Phase 1 is active and later post-1.0 candidates remain sequenced follow-ups.
- Focused docs/manifest validation passes.

## Phase 1 - Physics Constraints and Joints v1

### Goal

Advance the existing distance-joint foundation into a broader first-party constraints and joints surface suitable for post-1.0 gameplay systems.

### Planned Scope

- Preserve existing `physics-joints-foundation-v1` behavior as compatibility within the same greenfield task, without adding deprecated aliases.
- Add deterministic value rows for at least fixed and linear-axis-style 3D constraints, while retaining distance-joint evidence.
- Add explicit diagnostics for invalid bodies, invalid axes/anchors/limits, impossible limits, exhausted constraint budgets, and unsupported constraint kinds.
- Add stable solve ordering and bounded iteration policy evidence.
- Add package-visible counters for selected generated 3D gameplay-system evidence.
- Document non-goals: ragdoll authoring, vehicle suspension, soft bodies, cloth, destructible physics, and middleware-native handles.

### First Implementation Slice

- [x] Add RED tests for fixed/linear-axis constraint rows and invalid linear-axis axis/limit mutation safety.
- [x] Add `PhysicsConstraint3DStatus`, `PhysicsConstraint3DDiagnostic`, `PhysicsConstraint3DKind`, `PhysicsFixedConstraint3DDesc`, `PhysicsLinearAxisConstraint3DDesc`, `PhysicsConstraintSolve3DConfig`, `PhysicsConstraintSolve3DDesc`, `PhysicsConstraintSolve3DRow`, `PhysicsConstraintSolve3DResult`, and `solve_physics_constraints_3d`.
- [x] Add deterministic row order across distance, fixed, and linear-axis vectors; invalid-request validation happens before mutation.
- [x] Add package-visible selected constraint counters to `sample_generated_desktop_runtime_3d_package`.
- [x] Update docs, skills, manifest fragments/composed manifest, and static AI-contract guards.
- [ ] Decide whether Phase 1 needs an explicit row-count budget API before moving to Phase 2; the first slice has no extensible unsupported-kind input and no persistent constraint assets.

### Done When

- A RED test demonstrates the missing constraint behavior.
- Focused `MK_physics` build/test passes after implementation.
- Public/API docs and agent surfaces are updated only where durable contracts changed.
- Full `tools/validate.ps1` passes before publication of the C++/runtime slice, or a concrete host/toolchain blocker is recorded.

## Phase 2 - Physics Vehicles and Kinematics v1

### Goal

Build simple kinematic and vehicle foundations on top of the Phase 1 constraint surface and existing advanced-controller policy.

### Done When

- Kinematic movement policy is deterministic under fixed timestep.
- Vehicle rows remain simple and first-party; no tire model, suspension tuning suite, or middleware vehicle module is claimed.
- Generated package evidence exposes selected counters.

## Phase 3 - Navigation Hierarchical World v1

### Goal

Add region/portal navigation organization for larger scenes using existing navigation and world-region foundations.

### Done When

- Region graph and portal rows validate deterministically.
- Missing or unloaded navigation regions fail with explicit diagnostics.
- Cache invalidation and path handoff evidence remain safe-point/package-scoped.

## Phase 4 - Large World and Persistence v1

### Goal

Connect large-scene streaming evidence with persistence, save/resume validation, and migration diagnostics.

### Done When

- Region/chunk identity, entity persistence, and save-slot rows validate through package evidence.
- Version mismatch and corrupted-state diagnostics are deterministic.
- No cloud save, multiplayer replication, or binary compatibility promise is made without a release-policy plan.

## Phase 5 - AI Game-Creation Follow-Ons

### Goal

Use the stable engine contracts from earlier phases to strengthen generated-game workflows without letting game-creation agents edit engine internals.

### Candidate Rows

- `ai-game-design-spec-v1`
- `ai-game-generation-orchestrator-v1`
- `ai-placeholder-asset-pipeline-v1`
- `ai-generated-game-playtest-loop-v1`
- `ai-validation-remediation-recipes-v1`
- `ai-generated-game-quality-rubric-v1`
- `ai-engine-capability-handoff-v1`

### Done When

- Generated-game plans produce reviewed game-owned files and durable handoff rows for missing reusable engine capabilities.
- Manifest recipes and validation commands distinguish supported generation claims from planned follow-ups.

## Validation Evidence

| Gate | Command | Evidence |
| --- | --- | --- |
| Phase 0 docs/manifest sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS on 2026-05-22 in this branch; `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| Phase 0 agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS on 2026-05-22 in this branch; `ai-integration-check: ok`. |
| Phase 0 agent surfaces | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS on 2026-05-22 in this branch; `agent-config-check: ok`. |
| Phase 1 RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` before implementation | Expected failure on 2026-05-22: missing `solve_physics_constraints_3d` and `PhysicsConstraint*` public types. |
| Phase 1 focused physics build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` | PASS on 2026-05-22 after implementation. |
| Phase 1 focused physics test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests"` | PASS on 2026-05-22; `MK_core_tests` passed. |
| Phase 1 package build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_generated_desktop_runtime_3d_package` | PASS on 2026-05-22. |
| Phase 1 package smoke | `out\build\dev\games\Debug\sample_generated_desktop_runtime_3d_package\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-gameplay-systems` | PASS on 2026-05-22; reports `gameplay_systems_physics_constraints_status=solved`, `gameplay_systems_physics_constraints_rows=2`, fixed rows `1`, linear-axis rows `1`, and axis-limit-clamped `1`. |
| Phase 1 public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS on 2026-05-22. |
| Phase 1 format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` plus `git diff --check` | PASS on 2026-05-22 after `tools/format.ps1` normalized C++ formatting. |
| Phase 1 docs/manifest sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS on 2026-05-22; `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| Phase 1 agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS on 2026-05-22; `ai-integration-check: ok`. |
| Phase 1 agent surfaces | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS on 2026-05-22; `agent-config-check: ok`. |
| Phase 1 full gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS on 2026-05-22; all 73 CTest tests passed. Diagnostic-only host gates still report missing Apple/macOS and Metal host tooling on this Windows host. |
