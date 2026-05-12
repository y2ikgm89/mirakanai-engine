# Path Following Steering Foundation v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend `mirakana_navigation` with deterministic path-to-point conversion, path following, and single-agent arrive steering helpers so generated games can move along computed grid paths without owning scene, physics, renderer, middleware, or platform dependencies.

**Architecture:** Keep this slice in `mirakana_navigation` as C++23 value-type contracts beside the existing `NavigationGrid` and A* pathfinding. The API should remain standard-library-only under `mirakana::`, accept explicit point/path state, return deterministic status/diagnostics, and avoid controller ownership; navmesh assets, dynamic obstacle replanning, crowd avoidance, behavior trees, scene integration, physics integration, and editor visualization remain follow-up work.

**Tech Stack:** C++23, `mirakana_navigation`, existing `mirakana_navigation_tests`, no new third-party dependencies.

---

## Context

- `mirakana_navigation` already provides cardinal-grid A* pathfinding with stable tie-breaking, traversal costs, endpoint diagnostics, no-path reporting, total cost, and visited-node counts.
- Production gameplay still lacks a first-party way to turn a path into deterministic movement targets or steering output.
- A dependency-free path follower and arrive steering helper is host-independent and can be validated on the current Windows host.

## Constraints

- Do not add renderer, RHI, scene, runtime, physics, platform, editor, SDL3, Dear ImGui, navmesh middleware, or third-party dependencies.
- Do not implement navmesh assets, agent radius constraints, dynamic obstacle replanning, path smoothing splines, crowd avoidance, behavior trees, AI controller ownership, or editor visualization in this slice.
- Keep public API in namespace `mirakana::`.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_navigation` exposes point path mapping from `NavigationGridCoord` paths to deterministic 2D navigation points.
- [x] `mirakana_navigation` exposes a value-type path following request/result with target cursor, arrival radius, max step distance, advanced distance, remaining distance, and deterministic status.
- [x] `mirakana_navigation` exposes a single-agent arrive steering helper that returns desired velocity/speed and reached-target state without owning scene or physics state.
- [x] Tests cover grid-path conversion, waypoint advancement, overshoot across waypoints, arrival threshold snapping, invalid requests, deterministic replay, path replacement through explicit request state, and arrive steering edge cases.
- [x] Docs, roadmap, gap analysis, manifest, and Codex/Claude guidance describe this as Path Following / Steering Foundation v0, not navmesh, crowd avoidance, behavior trees, AI controllers, dynamic obstacle replanning, or scene/physics integration.
- [x] Focused navigation tests, API boundary, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Path Following / Steering Tests

**Files:**
- Modify: `tests/unit/navigation_tests.cpp`
- Inspect: `engine/navigation/include/mirakana/navigation/navigation_grid.hpp`
- Inspect: `engine/navigation/src/navigation_grid.cpp`

- [x] Add tests for converting a `NavigationGridCoord` path into point centers with origin and cell size mapping.
- [x] Add tests for following a path through waypoints with max-step clamping, overshoot across multiple segments, target cursor updates, remaining distance, and deterministic replay.
- [x] Add tests for arrival threshold snapping and explicit path replacement by passing a new request state.
- [x] Add tests for invalid empty paths, out-of-range target indices, non-finite positions, negative/zero step distances, and invalid steering values.
- [x] Add tests for single-agent arrive steering: far target clamps to max speed, near target slows down, and reached target reports zero velocity.
- [x] Verify the new tests fail for missing APIs before implementation.

### Task 2: Path Following / Steering Contracts

**Files:**
- Create: `engine/navigation/include/mirakana/navigation/path_following.hpp`
- Create: `engine/navigation/src/path_following.cpp`
- Modify: `engine/navigation/CMakeLists.txt`

- [x] Add value types for `NavigationPoint2`, grid-to-point mapping, path follow request/result/status, and arrive steering request/result/status.
- [x] Implement finite-input validation, path cursor advancement, overshoot handling, remaining-distance calculation, and deterministic output.
- [x] Implement arrive steering with max-speed clamping, slowing radius, arrival radius, and zero-velocity reached state.
- [x] Keep behavior deterministic and standard-library-only.

### Task 3: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/architecture.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Mark path following and arrive steering honestly as implemented only after tests and validation pass.
- [x] Keep navmesh assets, agent radius constraints, dynamic obstacle replanning, crowd avoidance, behavior trees, AI controller ownership, scene/physics integration, and editor visualization as follow-up work.

### Task 4: Verification

- [x] Run focused navigation tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED test evidence: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests` failed because `mirakana/navigation/path_following.hpp` and later `build_navigation_point_path_result` implementation were missing.
- Focused navigation validation: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_navigation_tests"` passed after implementing path following and fixing reviewer-identified max-step/overflow/epsilon issues.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: clang-tidy config PASS, diagnostic-only blocker because `out/build/dev/compile_commands.json` is missing for the Visual Studio generator.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Existing diagnostic-only host gates remain: Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy analysis compile database unavailable.
