# Navigation Agent Controller Foundation v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a dependency-free navigation agent controller state contract on top of `mirakana_navigation` pathfinding/path-following so generated games can tick a single agent along an explicit point path without owning controller bookkeeping, scene components, physics bodies, middleware, platform APIs, or editor code.

**Architecture:** Keep this slice in `mirakana_navigation` as C++23 value-type APIs under `mirakana::`. The controller owns only explicit navigation state: position, path points, target cursor, speed config, per-tick delta, and deterministic result/status. It may compose the existing path-following and arrive-steering helpers, but it must not own scene nodes, physics bodies, behavior trees, dynamic replanning, navmesh data, crowd avoidance, or editor visualization.

**Tech Stack:** C++23, `mirakana_navigation`, existing `mirakana_navigation_tests`, no new third-party dependencies.

---

## Context

- `mirakana_navigation` now provides deterministic grid A* pathfinding plus Path Following / Steering Foundation v0 for point-path conversion, explicit target cursor advancement, and single-agent arrive steering.
- Gameplay still has to write repeated state glue for movement status, path replacement, delta-time max-step calculation, and replay-safe per-agent ticking.
- A first-party value-type controller contract is host-independent and directly supports generated games while keeping scene/physics integration as later adapter work.

## Constraints

- Do not add renderer, RHI, scene, runtime, physics, platform, editor, SDL3, Dear ImGui, navmesh middleware, or third-party dependencies.
- Do not implement navmesh assets, dynamic obstacle replanning, path smoothing splines, crowd avoidance, behavior trees, AI decisions, scene component ownership, physics body movement, or editor visualization in this slice.
- Keep public API in namespace `mirakana::`.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_navigation` exposes a value-type navigation agent config/state/update request/result/status contract.
- [x] A per-tick helper advances agent state deterministically from explicit path points, speed config, and delta seconds.
- [x] The helper reports status, desired velocity, step delta, step distance, remaining distance, target cursor, and reached-destination state without scene/physics ownership.
- [x] Tests cover idle/no-path behavior, path ticking, max-speed/delta clamping, destination reach, explicit path replacement, deterministic replay, and invalid inputs.
- [x] Docs, roadmap, gap analysis, manifest, skills, and Codex/Claude subagent guidance describe this as Navigation Agent Controller Foundation v0, not navmesh, dynamic replanning, crowd avoidance, behavior trees, scene/physics integration, or editor visualization.
- [x] Focused navigation tests, API boundary, schema/agent/format checks, `tidy-check`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Navigation Agent Controller Tests

**Files:**
- Modify: `tests/unit/navigation_tests.cpp`
- Inspect: `engine/navigation/include/mirakana/navigation/path_following.hpp`
- Inspect: `engine/navigation/src/path_following.cpp`

- [x] Add tests for idle/no-path state returning no movement and stable status.
- [x] Add tests for ticking an agent along a multi-point path with `delta_seconds * max_speed` max-step behavior.
- [x] Add tests for destination reach, target cursor persistence, desired velocity, step delta, remaining distance, and deterministic replay.
- [x] Add tests for explicit path replacement resetting the target cursor and continuing from the current position.
- [x] Add tests for invalid config/state/update inputs, including non-finite position/path values, empty config, non-positive delta, non-positive speed, negative arrival radius, and overflow-prone step calculation.
- [x] Verify the new tests fail for missing APIs before implementation.

### Task 2: Navigation Agent Controller Contracts

**Files:**
- Create: `engine/navigation/include/mirakana/navigation/navigation_agent.hpp`
- Create: `engine/navigation/src/navigation_agent.cpp`
- Modify: `engine/navigation/CMakeLists.txt`

- [x] Add value types for navigation agent config, state, update request, update status, and update result.
- [x] Implement path replacement helper that stores the explicit point path, target cursor, and status without owning scene/physics state.
- [x] Implement per-tick advancement using existing path-following semantics and derived desired velocity/step distance.
- [x] Validate all finite inputs and computed outputs, including `delta_seconds * max_speed` overflow.
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
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] Mark the navigation agent controller honestly as implemented only after tests and validation pass.
- [x] Keep navmesh assets, dynamic obstacle replanning, crowd avoidance, behavior trees, AI decisions, scene/physics integration, and editor visualization as follow-up work.

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

- RED test evidence: focused navigation build failed because `mirakana/navigation/navigation_agent.hpp` was missing; later regression tests failed on detour slowing, reached-tick desired velocity, invalid path replacement, and invalid status/state invariants before fixes.
- Focused navigation validation: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_navigation_tests"` passed after implementing `navigation_agent.hpp/.cpp` and fixing reviewer findings.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: clang-tidy config PASS, diagnostic-only blocker because `out/build/dev/compile_commands.json` is missing for the Visual Studio generator.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Existing diagnostic-only host gates remain: Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy analysis compile database unavailable.
