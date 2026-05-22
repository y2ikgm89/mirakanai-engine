# Gameplay Simulation Orchestration v1 (2026-05-22)

**Plan ID:** `gameplay-simulation-orchestration-v1`
**Status:** Completed.
**Historical pointer note:** This completed plan was selected by `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` only while active; current pointer truth is the composed manifest and plan registry.

## Goal

Add first-party, value-only gameplay simulation orchestration rows so generated games can plan fixed-timestep simulation steps, ordered input-command playback, frame-budget clamping, and replay-ready diagnostics without coupling authoritative simulation to renderer frame pacing, networking I/O, editor runtime hosts, native handles, or game-specific rules.

## Context

- `gameplay-authoring-foundation-v1` is completed: runtime-scene gameplay binding and interaction rows are ready, and optional debug overlay rows were not selected inside that milestone.
- Engine Networking Foundation v1 is completed through selected package evidence and needs deterministic simulation prerequisites to remain value-only until future transport work.
- The post-1.0 gameplay advanced track lists `gameplay-simulation-orchestration-v1` as a recommended stream for fixed timestep, pause/step/replay hooks, deterministic input-command playback, rollback-ready diagnostics, and package handoff.
- Engine 1.0 remains zero-gap: `unsupportedProductionGaps = []`.

## Constraints

- Preserve `unsupportedProductionGaps = []`. Stop if this work requires reopening an Engine 1.0 blocker.
- Keep Phase 1 contracts backend-neutral, deterministic, dependency-free, and value-only.
- Do not add networking rollback, lockstep multiplayer, save-state binary compatibility policy, editor in-process runtime embedding, package mutation, renderer/RHI execution, platform frame pacing, native handles, background threads, or game-specific simulation rules.
- Start behavior/API changes with RED tests.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Close the stale networking active pointer after PR #171 and select this gameplay follow-up milestone while keeping historical evidence discoverable.

### Done When

- `docs/superpowers/plans/README.md`, the readiness ledger, roadmap/current-capabilities active-work prose, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` point at this plan as active.
- Engine Networking Foundation v1 is recorded as completed historical evidence with PR #171 / merge commit evidence where appropriate.
- Composed `engine/agent/manifest.json` keeps `unsupportedProductionGaps = []`.

## Phase 1: Fixed-Step Command Playback Planning

**Status:** Completed.

### Goal

Add the smallest `MK_runtime` public contract that plans fixed-timestep simulation step rows and deterministic input-command rows from caller-owned timing and command data.

### Planned API Shape

- `RuntimeSimulationOrchestrationRequest`: simulation id, next tick, fixed tick rate, accumulated microseconds, max steps per frame, and caller-owned input command rows.
- `RuntimeSimulationInputCommandDesc`: command id, action id, target tick, and source index.
- `RuntimeSimulationOrchestrationPlan`: status, diagnostics, fixed delta, available/planned step counts, consumed/remaining time, step rows, command rows, and `succeeded()`.
- `plan_runtime_simulation_orchestration`: fail-closed planner that never mutates gameplay state.

### Done When

- RED tests prove the API is missing before implementation.
- Focused tests prove deterministic fixed-step rows, command ordering, budget-limited frames, and fail-closed diagnostics for invalid ids, duplicate command ids, invalid tick rate, invalid step budget, past commands, and commands beyond the planned window.
- Focused `MK_runtime_simulation_orchestration_tests` build/test passes.
- Public API boundary, touched-file tidy, agent/static checks, docs, manifest fragments, and full `tools/validate.ps1` pass at the runtime/public-contract gate.

## Phase 2: Package Evidence and Closeout

**Status:** Completed.

### Goal

Promote the simulation orchestration contract into selected package-visible evidence only if the Phase 1 value contract needs a package smoke before closing the milestone.

### Done When

- A selected package or sample lane reports deterministic simulation orchestration counters for planned steps, command playback rows, budget-limited frames, and clean diagnostics, or this phase records why package promotion is unnecessary for the current narrow claim.
- Docs, manifest fragments, skills, and static checks are reconciled after the package decision.
- The milestone closes without reopening `unsupportedProductionGaps`.

## Validation Evidence

- Phase 0 pointer sync records Engine Networking Foundation v1 as completed by PR #171 / merge commit `987016f1`, selects this plan in the plan registry, readiness ledger, master-plan index, roadmap, current capabilities, and manifest fragments, and preserves `unsupportedProductionGaps = []`.
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_simulation_orchestration_tests` failed because `mirakana/runtime/simulation_orchestration.hpp` did not exist.
- Phase 1 focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_simulation_orchestration_tests`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure --tests-regex MK_runtime_simulation_orchestration_tests` passed after adding the value-only `MK_runtime` contract.
- Phase 1 focused/static checks passed: `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-production-readiness-audit.ps1` (`unsupported_gaps=0`), `tools/check-public-api-boundaries.ps1`, `tools/check-tidy.ps1 -Files engine/runtime/src/simulation_orchestration.cpp,tests/unit/runtime_simulation_orchestration_tests.cpp`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1`.
- Phase 1 runtime/public-contract gate passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed successfully with 73 tests passed and `unsupported_gaps=0`.
- Phase 2 RED: `out/build/dev/games/Debug/sample_2d_desktop_runtime_package/sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-simulation-orchestration` failed before implementation with `unknown argument: --require-simulation-orchestration`.
- Phase 2 focused GREEN: `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package`, source-tree `sample_2d_desktop_runtime_package --smoke --require-simulation-orchestration`, `tools/ctest.ps1 --preset dev --output-on-failure --tests-regex sample_2d_desktop_runtime_package_smoke`, and `tools/check-tidy.ps1 -Files games/sample_2d_desktop_runtime_package/main.cpp` passed after adding selected package counters.
- Phase 2 agent/static checks passed: `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, and `tools/check-production-readiness-audit.ps1` (`unsupported_gaps=0`).
- Phase 2 installed package evidence passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` completed with installed `sample_2d_desktop_runtime_package --require-simulation-orchestration` counters (`simulation_orchestration_status=planned`, `simulation_orchestration_planned_steps=3`, `simulation_orchestration_command_playback_rows=3`, `simulation_orchestration_budget_limited_status=budget_limited`, `simulation_orchestration_invalid_command_diagnostics=4`, `simulation_orchestration_diagnostics=0`) and `installed-desktop-runtime-validation: ok`.
- Phase 2 full validation gate passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed successfully with 73 tests passed and `production-readiness-audit: unsupported_gaps=0`.
- PR #172 closeout passed hosted `PR Gate`, `Windows MSVC`, `Linux CMake`, `Linux Coverage`, `Linux Clang ASan/UBSan`, `Full Repository Static Analysis (0-3)`, `CodeQL`, `iOS Simulator smoke`, and `macOS Metal CMake`, then merged as commit `eb6a34b5` while keeping `unsupportedProductionGaps = []`.
