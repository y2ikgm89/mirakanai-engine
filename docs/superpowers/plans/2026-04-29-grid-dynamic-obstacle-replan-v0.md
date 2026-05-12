# Grid Dynamic Obstacle Replan v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a dependency-free grid navigation replan helper that lets generated games detect stale or blocked remaining paths and compute a deterministic replacement path when dynamic obstacles change a `NavigationGrid`.

**Architecture:** Keep the slice inside `mirakana_navigation` as C++23 value-type APIs under `mirakana::`. The API validates a remaining grid path against the current grid, reports the first deterministic failure, reuses a still-valid path, and otherwise calls the existing `find_navigation_path` from the current grid coordinate to the goal. It must not add scene, physics, renderer, editor, platform, middleware, threading, or third-party dependencies.

**Tech Stack:** C++23, `mirakana_navigation`, CTest, no new third-party dependencies.

---

## Context

- `mirakana_navigation` already owns deterministic cardinal-grid A* pathfinding, grid-path-to-point mapping, explicit path-follow advancement, arrive steering, and value-type navigation agent movement.
- Dynamic obstacle replanning is still documented as missing. Generated games currently must hand-roll stale path detection before calling `find_navigation_path` again.
- A small grid-path validator plus replan helper is host-independent and can be verified on this Windows host without SDK, hardware, signing, or dependency gates.

## Constraints

- Keep `mirakana_navigation` standard-library-only and independent from scene, physics, renderer, RHI, editor, platform, SDL3, Dear ImGui, and native handles.
- Keep the API explicit: no hidden ownership of agents, scene nodes, physics bodies, blackboards, navmesh assets, or background jobs.
- Support only the existing `NavigationAdjacency::cardinal4` policy in this slice. Report unsupported adjacency values deterministically.
- Do not implement navmesh assets, crowd avoidance, local avoidance, path smoothing, scene/physics integration, or editor visualization.

## Done When

- [x] `mirakana_navigation` exposes `mirakana/navigation/navigation_replan.hpp`.
- [x] `validate_navigation_grid_path` reports valid paths plus empty path, start mismatch, goal mismatch, out-of-bounds cell, blocked cell, non-cardinal step, unsupported adjacency, and cost overflow diagnostics.
- [x] `replan_navigation_grid_path` reuses a still-valid remaining path, replans when the path is stale or blocked, and forwards invalid endpoint, blocked endpoint, and no-path failures from `find_navigation_path`.
- [x] Focused navigation tests prove deterministic reuse, replan-around-new-obstacle, no-path, invalid endpoints, blocked endpoints, and validation diagnostics.
- [x] Docs, roadmap/gap analysis, manifest, Codex/Claude game-development skills, and gameplay-builder agents describe this as grid dynamic replan only, not navmesh/crowd/scene/physics integration.
- [x] Focused validation, public API boundary, schema/agent/format checks, tidy diagnostics, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Navigation Replan Tests

**Files:**
- Modify: `tests/unit/navigation_tests.cpp`

- [x] Add `#include "mirakana/navigation/navigation_replan.hpp"` before the production header exists.
- [x] Add tests for:
  - reusing a valid remaining path with the same total cost as the grid path.
  - replanning around a newly blocked cell in the old remaining path.
  - returning no-path when dynamic blockers seal the route.
  - invalid endpoint and blocked endpoint status forwarding.
  - validation diagnostics for empty path, start mismatch, goal mismatch, out-of-bounds, blocked cell, non-cardinal step, unsupported adjacency, and cost overflow.
- [x] Run the focused build and confirm RED.

Expected RED command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests
```

Expected failure: compiler cannot open `mirakana/navigation/navigation_replan.hpp`.

### Task 2: Navigation Replan API

**Files:**
- Create: `engine/navigation/include/mirakana/navigation/navigation_replan.hpp`
- Create: `engine/navigation/src/navigation_replan.cpp`
- Modify: `engine/navigation/CMakeLists.txt`

- [x] Define `NavigationGridPathValidationStatus`.
- [x] Define `NavigationGridPathValidationResult` with `status`, `failing_index`, `coord`, and `total_cost`.
- [x] Define `NavigationGridReplanStatus`.
- [x] Define `NavigationGridReplanRequest` with `current`, `goal`, `remaining_path`, `adjacency`, and `reuse_valid_path`.
- [x] Define `NavigationGridReplanResult` with `status`, `validation`, `path`, `reused_existing_path`, and `replanned`.
- [x] Implement `validate_navigation_grid_path`.
- [x] Implement `replan_navigation_grid_path` using `validate_navigation_grid_path` and `find_navigation_path`.
- [x] Keep all functions deterministic and non-throwing for valid `NavigationGrid` references.

### Task 3: Green Navigation Tests

**Files:**
- Modify: `tests/unit/navigation_tests.cpp`

- [x] Run focused navigation build and CTest until all new tests pass.
- [x] Keep existing pathfinding, path-following, steering, and agent tests passing.
- [x] Add edge coverage if the first implementation misses deterministic failure-index or total-cost behavior.

Focused command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_navigation_tests"
```

### Task 4: Docs And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] Register this plan as the active slice before implementation.
- [x] After validation, move the plan to completed with concise evidence.
- [x] Mark grid dynamic obstacle replanning implemented only after validation.
- [x] Keep navmesh assets, crowd avoidance, local avoidance, path smoothing, scene/physics integration, and editor visualization listed as follow-up work.

### Task 5: Verification

- [x] Run focused navigation build and CTest.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED evidence: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests` failed because `mirakana/navigation/navigation_replan.hpp` did not exist.
- Focused navigation validation: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_navigation_tests"` passed after adding `navigation_replan.hpp/.cpp`.
- cpp-reviewer follow-up: added `NavigationGridReplanResult::total_cost`, direct unsupported-adjacency replan coverage, `engine/navigation/include` API-boundary scanning, and `docs/architecture.md` navigation guidance plus static AI check coverage.
- Static validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` passed; tidy remains diagnostic-only because the Visual Studio dev preset does not emit `out\build\dev\compile_commands.json`.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 22/22 tests. Existing diagnostic-only blockers remain Metal `metal`/`metallib`, Apple macOS/Xcode/iOS signing/runtime, Android release signing/device smoke, and strict clang-tidy compile database availability.
