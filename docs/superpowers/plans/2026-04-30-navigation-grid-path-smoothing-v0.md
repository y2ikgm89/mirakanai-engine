# Navigation Grid Path Smoothing v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a dependency-free grid path smoothing helper so generated games can reduce cardinal A* waypoint noise before point-path conversion and agent movement.

**Architecture:** Keep the slice inside `mirakana_navigation` as C++23 value-type APIs under `mirakana::`. The helper validates an existing cardinal grid path with `validate_navigation_grid_path`, then greedily keeps the farthest later waypoint reachable by deterministic grid line-of-sight through in-bounds walkable cells. It does not own scene, physics, navmesh, crowd simulation, editor visualization, or background jobs.

**Tech Stack:** C++23, `mirakana_navigation`, CTest, no new third-party dependencies.

---

## Context

- `mirakana_navigation` already owns deterministic cardinal grid A*, remaining-path validation/replan, grid-path-to-point conversion, path following, arrive steering, value-type agent movement, and explicit-neighbor local avoidance.
- Generated games currently move through every cardinal A* cell center, which is correct but visibly stair-stepped for open areas.
- This slice adds a conservative smoothing pass between `replan_navigation_grid_path` / `find_navigation_path` and `build_navigation_point_path_result`.

## Constraints

- Keep `mirakana_navigation` standard-library-only and independent from scene, physics, renderer, RHI, editor, platform, SDL3, Dear ImGui, native handles, and third-party middleware.
- Reuse existing grid path validation semantics instead of accepting malformed source paths.
- Preserve start and goal waypoints.
- Only remove intermediate points when the straight segment between retained waypoints has deterministic line-of-sight through in-bounds walkable grid cells.
- Do not implement navmesh assets, funnel/string-pulling over polygon portals, spline/Bezier movement, path following changes, local avoidance redesign, full crowd simulation, RVO/ORCA, scene/physics obstacle queries, editor visualization, or async/background navigation work.

## Done When

- [x] `mirakana_navigation` exposes `mirakana/navigation/path_smoothing.hpp`.
- [x] `smooth_navigation_grid_path` validates source paths through `validate_navigation_grid_path` and forwards deterministic failure diagnostics.
- [x] The helper returns unchanged single-point and already-direct paths.
- [x] The helper removes unnecessary corners through clear walkable space while preserving start and goal.
- [x] The helper refuses smoothing across blocked or out-of-bounds line-of-sight cells.
- [x] The helper rejects unsupported adjacency and malformed non-cardinal source paths deterministically.
- [x] Tests prove direct pass-through, corner removal, obstacle preservation, malformed path rejection, unsupported adjacency, and deterministic replay.
- [x] Docs, roadmap/gap analysis, manifest, Codex/Claude game-development skills, and gameplay-builder agents describe this as grid path smoothing only, not navmesh/crowd/scene/physics integration.
- [x] Focused validation, public API boundary, schema/agent/format checks, tidy diagnostics, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Path Smoothing Tests

**Files:**
- Modify: `tests/unit/navigation_tests.cpp`

- [x] Add `#include "mirakana/navigation/path_smoothing.hpp"` before the production header exists.
- [x] Add `navigation grid path smoothing preserves direct paths`.
  - Build a `3x1` grid and path `{(0,0), (1,0), (2,0)}`.
  - Expect `success`, validation `valid`, smoothed path `{(0,0), (2,0)}`, and one removed point.
- [x] Add `navigation grid path smoothing removes clear corners`.
  - Build a `3x3` walkable grid and path `{(0,0), (0,1), (1,1), (2,1), (2,2)}`.
  - Expect `success`, start `(0,0)`, goal `(2,2)`, fewer points than the source, and deterministic replay equality.
- [x] Add `navigation grid path smoothing preserves corners around blocked line of sight`.
  - Build a `3x3` grid with `(1,1)` blocked and a valid route around it.
  - Expect no direct `{(0,0), (2,2)}` shortcut and a retained intermediate corner.
- [x] Add `navigation grid path smoothing forwards invalid source diagnostics`.
  - Use a non-cardinal source path `{(0,0), (1,1)}`.
  - Expect `invalid_source_path` plus validation `non_cardinal_step`.
- [x] Add `navigation grid path smoothing rejects unsupported adjacency`.
  - Pass `static_cast<NavigationAdjacency>(99)`.
  - Expect `unsupported_adjacency`.
- [x] Run focused build and confirm RED.

Expected RED command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests
```

Expected failure: compiler cannot open `mirakana/navigation/path_smoothing.hpp`.

### Task 2: Path Smoothing API And Implementation

**Files:**
- Create: `engine/navigation/include/mirakana/navigation/path_smoothing.hpp`
- Create: `engine/navigation/src/path_smoothing.cpp`
- Modify: `engine/navigation/CMakeLists.txt`

- [x] Define `NavigationGridPathSmoothingStatus` with `success`, `invalid_source_path`, and `unsupported_adjacency`.
- [x] Define `NavigationGridPathSmoothingRequest` with `start`, `goal`, `path`, and `adjacency`.
- [x] Define `NavigationGridPathSmoothingResult` with `status`, `validation`, `path`, and `removed_point_count`.
- [x] Implement `smooth_navigation_grid_path(const NavigationGrid&, const NavigationGridPathSmoothingRequest&)`.
- [x] Implement deterministic line-of-sight with an integer grid-cell traversal between two cell centers:
  - Always check the start and end cells.
  - Reject any traversed cell that is out of bounds or not walkable.
  - On exact corner crossings, conservatively check both adjacent cells before advancing.
- [x] Greedily smooth by keeping the current retained point, scanning backward from the goal to find the farthest reachable next source point, and repeating until the goal is kept.
- [x] Register `src/path_smoothing.cpp` in `engine/navigation/CMakeLists.txt`.

### Task 3: Green Navigation Tests

**Files:**
- Modify: `tests/unit/navigation_tests.cpp`

- [x] Run focused navigation build and CTest until all new tests pass.
- [x] Keep existing grid pathfinding, replan, local avoidance, path-following, steering, and agent tests passing.
- [x] Add edge coverage if direct, obstacle, or deterministic replay behavior is ambiguous.

Focused command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_navigation_tests"
```

### Task 4: Optional Sample Proof

**Files:**
- Modify: `games/sample_ai_navigation/main.cpp`
- Modify: `games/sample_ai_navigation/README.md`

- [x] Include `mirakana/navigation/path_smoothing.hpp`.
- [x] Smooth the successful grid path before `build_navigation_point_path_result`.
- [x] Add one deterministic output field for the smoothed waypoint count.
- [x] Preserve the sample's existing headless behavior-tree/navigation proof and expected final position.
- [x] Run `cmake --build --preset dev --target sample_ai_navigation` and `ctest --preset dev --output-on-failure -R "sample_ai_navigation"`.

### Task 5: Docs And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`
- Modify: `tools/check-ai-integration.ps1`

- [x] Register this plan as the active slice before implementation.
- [x] After validation, move the plan to completed with concise evidence.
- [x] Mark grid path smoothing implemented only after validation.
- [x] Keep navmesh assets, spline/funnel smoothing, full crowd simulation, RVO/ORCA, scene/physics integration, background jobs, and editor visualization listed as follow-up work.
- [x] Fix the stale `Host-Feasible Candidate Plans` note in the plan registry.
- [x] Update gap analysis immediate-next text from local avoidance to path smoothing while keeping local avoidance listed as implemented.
- [x] Update the roadmap `games/` summary so `sample_desktop_runtime_game` mentions the current fixed PCF shadow smoke fields rather than only `framegraph_passes=2`.

### Task 6: Verification

- [x] Run focused navigation build and CTest.
- [x] Run optional `sample_ai_navigation` focused build/CTest if Task 4 is implemented.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Request read-only `cpp-reviewer` follow-up.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED focused build before implementation: failed because `mirakana/navigation/path_smoothing.hpp` did not exist.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: PASS.
- Focused build/CTest: `mirakana_navigation_tests` and `sample_ai_navigation` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS after restoring existing game-development guidance phrases required by `tools/check-ai-integration.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS; diagnostic-only blocker remains `out\build\dev\compile_commands.json` missing for the Visual Studio dev preset.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, 22/22 tests PASS.
- `cpp-reviewer`: two P2 findings fixed. Tests now lock exact clear-corner and blocked-line-of-sight output paths and cover single-point/two-point unchanged paths. Gap analysis, plan registry, and manifest completed-slice guidance no longer describe path smoothing as active or omit it from completed navigation guidance.
- Focused reviewer-fix validation: `mirakana_navigation_tests` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` diagnostic-only, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with 22/22 tests.
- Known host-gated diagnostics remain: Metal `metal`/`metallib` missing, Apple packaging requires macOS/Xcode (`xcodebuild`/`xcrun`), Android release signing not configured, Android device smoke not connected, strict clang-tidy compile database unavailable for the current Visual Studio generator.
