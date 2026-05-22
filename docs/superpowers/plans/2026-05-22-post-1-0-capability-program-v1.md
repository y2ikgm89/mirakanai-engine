# Post-1.0 Capability Program v1 (2026-05-22)

**Plan ID:** `post-1-0-capability-program-v1`
**Status:** Completed; closed at Phase 2.
**Milestone type:** Post-1.0 / 1.x phase-gated capability program.
**Closed phase:** Phase 2 - `physics-vehicles-and-kinematics-v1`.

## Goal

Select and execute the next coherent post-1.0 / 1.x engine capability program after the 1.0 closeout backlog reached `unsupportedProductionGaps = []`.

This plan kept the production-completion pointer on one active dated milestone through the Phase 1 and Phase 2 physics checkpoints. It is now closed at that coherent boundary; later navigation, large-world, persistence, and AI game-creation candidate streams return to the master backlog / selection pool.

## Context

- The 1.0 Windows-default closeout surface remains zero-gap: `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps = []`.
- The previous selection-gate state intentionally pointed `currentActivePlan` back to the production-completion master plan until a concrete roadmap decision selected the next developer-owned capability.
- The selected decision kept the post-1.0 / 1.x candidate backlog active as a single phase-gated milestone through `physics-constraints-and-joints-v1` and `physics-vehicles-and-kinematics-v1`; closeout returns later candidates to backlog selection.
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
| 0 | `post-1-0-capability-program-v1` | Select this milestone, update registry/master-plan/matrix/advanced-track/current-capabilities/manifest pointers. | Focused docs/manifest checks passed while this plan was active. |
| 1 | `physics-constraints-and-joints-v1` | Extend first-party `MK_physics` joint/constraint evidence beyond the completed distance-joint foundation. | Deterministic constraint rows, invalid-input diagnostics, package-visible counters, and focused/full validation evidence. |
| 2 | `physics-vehicles-and-kinematics-v1` | Add kinematic body and simple vehicle policy foundations for small games. | Fixed-step movement policy, interaction diagnostics, package-visible evidence, and explicit non-goals for AAA vehicle physics. |
| 3 | `navigation-hierarchical-world-v1` | Returned to backlog after Phase 2 closeout. | Select through a new dated plan before implementation. |
| 4 | `world-streaming-and-large-scenes-v1` plus `simulation-persistence-v1` | Returned to backlog after Phase 2 closeout. | Select through a new dated plan before implementation. |
| 5 | AI game-creation follow-ons | Returned to backlog after Phase 2 closeout. | Select through a new dated plan before implementation. |

## Phase 0 - Plan Surface Sync

### Work

- [x] Create this active milestone plan.
- [x] Update the plan registry current active work row.
- [x] Update the production-completion master-plan verdict.
- [x] Update the 2D/3D matrix so completed foundations are not described as active 1.0 gaps.
- [x] Update the gameplay physics/navigation/AI advanced track with the selected post-1.0 stream and completed lower-level foundations.
- [x] Update current-capabilities active-work text.
- [x] Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, compose `engine/agent/manifest.json`, and keep `unsupportedProductionGaps = []`.

### Done When

- `currentActivePlan` pointed to this plan during the implementation phase.
- `recommendedNextPlan.id` pointed to `physics-vehicles-and-kinematics-v1` during the implementation phase.
- Related planning surfaces agreed that Phase 2 was active during implementation; closeout returns `currentActivePlan` to the production-completion master plan with `recommendedNextPlan.id = next-production-gap-selection`.
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
- [x] Add explicit `PhysicsConstraintSolve3DConfig::max_rows` row-count budget with `row_budget_exceeded` diagnostics, default-unbounded behavior, and no world mutation on budget failure.
- [x] Add package-visible selected constraint counters to `sample_generated_desktop_runtime_3d_package`.
- [x] Update docs, skills, manifest fragments/composed manifest, and static AI-contract guards.
- [x] Decide whether Phase 1 needs an explicit row-count budget API before moving to Phase 2; Phase 1 now has explicit `max_rows` evidence, while unsupported constraint kinds remain a non-goal until an extensible constraint input surface exists.

### Done When

- A RED test demonstrates the missing constraint behavior.
- Focused `MK_physics` build/test passes after implementation.
- Public/API docs and agent surfaces are updated only where durable contracts changed.
- Full `tools/validate.ps1` passes before publication of the C++/runtime slice, or a concrete host/toolchain blocker is recorded.

## Phase 2 - Physics Vehicles and Kinematics v1

### Goal

Build simple kinematic and vehicle foundations on top of the Phase 1 constraint surface and existing advanced-controller policy.

### Implementation Slices

- [x] Add RED tests for deterministic kinematic fixed-step movement and package-visible simple vehicle evidence.
- [x] Add first-party `MK_physics` value contracts for value-only kinematic motion planning without middleware/native handles.
- [x] Add package-visible selected generated 3D counters for kinematic motion and public simple vehicle policy evidence.
- [x] Add RED tests for public `PhysicsSimpleVehicle3D*` value contracts and invalid wheel/filter diagnostics.
- [x] Add first-party `plan_physics_simple_vehicle_3d` policy rows that compose value-only kinematic motion with deterministic wheel probes without persistent vehicle bodies or drivetrain simulation.
- [x] Update generated 3D gameplay-systems package evidence to report `gameplay_systems_vehicle_status`, `gameplay_systems_vehicle_diagnostic`, `gameplay_systems_vehicle_wheel_rows`, grounded-wheel count, wheel-probe hit count, and final x.
- [x] Update docs, manifest fragments/composed manifest, templates, and static guard needles where durable contracts change.

### Done When

- Kinematic movement policy is deterministic under fixed timestep.
- Simple vehicle policy remains first-party and value-only; broad vehicle dynamics, persistent vehicle simulation, tire models, suspension tuning suites, and middleware vehicle modules are not claimed.
- Generated package evidence exposes selected counters.

## Deferred Backlog References

The following phases were not implemented in this plan. They remain post-1.0 / 1.x developer-owned backlog rows and require a new dated plan before work starts. Canonical backlog status lives in [04-developer-owned-engine-capability-backlog.md](../master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md).

| Deferred area | Canonical backlog row ids |
| --- | --- |
| Navigation hierarchical world | `navigation-hierarchical-world-v1` |
| Large world and persistence | `world-streaming-and-large-scenes-v1`, `simulation-persistence-v1` |
| AI game-creation follow-ons | `ai-game-design-spec-v1`, `ai-game-generation-orchestrator-v1`, `ai-placeholder-asset-pipeline-v1`, `ai-generated-game-playtest-loop-v1`, `ai-validation-remediation-recipes-v1`, `ai-generated-game-quality-rubric-v1`, `ai-engine-capability-handoff-v1` |
## Validation Evidence

| Gate | Command | Evidence |
| --- | --- | --- |
| Phase 0 docs/manifest sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS on 2026-05-22 in this branch; `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| Phase 0 agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS on 2026-05-22 in this branch; `ai-integration-check: ok`. |
| Phase 0 agent surfaces | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS on 2026-05-22 in this branch; `agent-config-check: ok`. |
| Phase 1 RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` before implementation | Expected failure on 2026-05-22: missing `solve_physics_constraints_3d` and `PhysicsConstraint*` public types. |
| Phase 1 focused physics build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` | PASS on 2026-05-22 after implementation. |
| Phase 1 focused physics test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests"` | PASS on 2026-05-22; `MK_core_tests` passed. |
| Phase 1 row-budget RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` before row-budget implementation | Expected failure on 2026-05-22: missing `PhysicsConstraintSolve3DConfig::max_rows` and `PhysicsConstraint3DDiagnostic::row_budget_exceeded`. |
| Phase 1 row-budget focused physics test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests"` | PASS on 2026-05-22 after row-budget implementation and review follow-up; `MK_core_tests` passed, including `requested_rows == max_rows` acceptance and explicit aggregate `max_rows` call sites. |
| Phase 1 package build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_generated_desktop_runtime_3d_package` | PASS on 2026-05-22. |
| Phase 1 package smoke | `out\build\dev\games\Debug\sample_generated_desktop_runtime_3d_package\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-gameplay-systems` | PASS on 2026-05-22; reports `gameplay_systems_physics_constraints_status=solved`, `gameplay_systems_physics_constraints_rows=2`, fixed rows `1`, linear-axis rows `1`, and axis-limit-clamped `1`. |
| Phase 2 RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` before implementation | Expected failure on 2026-05-22: missing `PhysicsKinematicMotion3D*` public types and `plan_physics_kinematic_motion_3d`. |
| Phase 2 focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests sample_generated_desktop_runtime_3d_package` | PASS on 2026-05-22 after implementation. |
| Phase 2 focused test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` | PASS on 2026-05-22; kinematic motion tests cover deterministic slide rows, trigger overlap rows, invalid requests, initial-overlap diagnostics, and no caller-world mutation. |
| Phase 2 package smoke | `out\build\dev\games\Debug\sample_generated_desktop_runtime_3d_package\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-gameplay-systems` | PASS on 2026-05-22; reports `gameplay_systems_kinematic_motion_status=constrained`, `gameplay_systems_kinematic_motion_rows=2`, `gameplay_systems_vehicle_status=grounded`, `gameplay_systems_vehicle_diagnostic=none`, `gameplay_systems_vehicle_wheel_rows=4`, `gameplay_systems_vehicle_grounded_wheels=4`, and `gameplay_systems_vehicle_wheel_probe_hits=4`. |
| Phase 2 constraint-review RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` before constraint diagnostic hardening | Expected failure on 2026-05-22: exact linear-axis boundary was flagged as clamped and static-pair constraint rows did not retain kind-specific deltas. |
| Phase 2 constraint-review focused test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` | PASS on 2026-05-22 after constraint diagnostic hardening. |
| Phase 2 simple vehicle RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` before implementation | Expected failure on 2026-05-22: missing `PhysicsSimpleVehicle3D*` public types and `plan_physics_simple_vehicle_3d`. |
| Phase 2 simple vehicle focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests sample_generated_desktop_runtime_3d_package` | PASS on 2026-05-22 after implementation. |
| Phase 2 simple vehicle focused test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` | PASS on 2026-05-22; simple vehicle tests cover deterministic kinematic composition, wheel rows, invalid wheel/filter diagnostics, and no caller-world mutation. |
| Phase 2 simple vehicle package smoke | `out\build\dev\games\Debug\sample_generated_desktop_runtime_3d_package\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-gameplay-systems` | PASS on 2026-05-22; reports `gameplay_systems_vehicle_status=grounded`, `gameplay_systems_vehicle_diagnostic=none`, `gameplay_systems_vehicle_wheel_rows=4`, grounded wheels `4`, wheel-probe hits `4`, and final x `1`. |
| Phase 2 public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS on 2026-05-22. |
| Phase 2 format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `git diff --check` | PASS on 2026-05-22 after `tools/format.ps1`. |
| Phase 2 closeout pointer sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS on 2026-05-22; `currentActivePlan` returns to the production-completion master plan, `recommendedNextPlan.id = next-production-gap-selection`, `unsupportedProductionGaps = []`, `agent-manifest-compose: ok`, and `json-contract-check: ok`. |
| Phase 2 agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS on 2026-05-22 after simple vehicle and closeout pointer sync; `ai-integration-check: ok`. |
| Phase 2 agent surfaces | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS on 2026-05-22; `agent-config-check: ok`. |
| Phase 2 full gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticJobs 1` | PASS on 2026-05-22; all 73 CTest tests passed. Diagnostic-only host gates still report missing Apple/macOS and Metal host tooling on this Windows host. |
| Phase 1 public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS on 2026-05-22. |
| Phase 1 format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` plus `git diff --check` | PASS on 2026-05-22 after `tools/format.ps1` normalized C++ formatting. |
| Phase 1 docs/manifest sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS on 2026-05-22; `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| Phase 1 agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS on 2026-05-22; `ai-integration-check: ok`. |
| Phase 1 agent surfaces | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS on 2026-05-22; `agent-config-check: ok`. |
| Phase 1 full gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS on 2026-05-22; all 73 CTest tests passed. Diagnostic-only host gates still report missing Apple/macOS and Metal host tooling on this Windows host. |
