# Navigation Agent Local Avoidance v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a dependency-free local avoidance helper so generated games can adjust single-agent desired velocities against nearby agents without owning a crowd simulation, scene, physics, renderer, editor, platform, or third-party dependency.

**Architecture:** Keep the slice inside `mirakana_navigation` as C++23 value-type APIs under `mirakana::`. The helper takes one agent row plus explicit neighbor rows, validates finite positions/radii/velocities, returns a deterministic adjusted velocity and diagnostics, and can be composed after path following or `update_navigation_agent` without mutating agent state.

**Tech Stack:** C++23, `mirakana_navigation`, CTest, no new third-party dependencies.

---

## Context

- `mirakana_navigation` now owns deterministic cardinal-grid pathfinding, grid dynamic replan helpers, grid-path-to-point mapping, explicit path-follow advancement, arrive steering, and value-type navigation agent movement.
- Production game agents need a first local avoidance contract before full navmesh/crowd systems. A value-only helper keeps the contract honest while avoiding Detour/RVO/ORCA, physics ownership, scene queries, and threading.
- This slice is host-independent and can be verified on the Windows dev preset through `mirakana_navigation_tests`.

## Constraints

- Keep `mirakana_navigation` standard-library-only and independent from scene, physics, renderer, RHI, editor, platform, SDL3, Dear ImGui, and native handles.
- Do not implement navmesh assets, RVO/ORCA, full crowd simulation, path smoothing, steering behaviors beyond local separation, scene/physics obstacle adapters, editor visualization, middleware, or background jobs.
- Keep the API explicit: callers provide current agent state, desired velocity, max speed, radius, and neighbor rows for the current tick.
- Determinism matters more than optimal avoidance quality in v0.

## Done When

- [x] `mirakana_navigation` exposes `mirakana/navigation/local_avoidance.hpp`.
- [x] `calculate_navigation_local_avoidance` validates invalid agent ids, non-finite positions/velocities, negative radius, negative max speed, invalid neighbor rows, and unsupported parameters deterministically.
- [x] The helper returns unchanged desired velocity when there are no relevant neighbors.
- [x] The helper adjusts velocity away from overlapping or near-future collision neighbors in deterministic neighbor order.
- [x] The helper clamps adjusted velocity to `max_speed` while preserving zero desired velocity behavior.
- [x] Focused navigation tests prove no-neighbor pass-through, overlap separation, predicted head-on adjustment, deterministic neighbor ordering, max-speed clamp, and invalid diagnostics.
- [x] Docs, roadmap/gap analysis, manifest, Codex/Claude game-development skills, and gameplay-builder agents describe this as local avoidance only, not navmesh/crowd/scene/physics integration.
- [x] Focused validation, public API boundary, schema/agent/format checks, tidy diagnostics, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Local Avoidance Tests

**Files:**
- Modify: `tests/unit/navigation_tests.cpp`

- [x] Add `#include "mirakana/navigation/local_avoidance.hpp"` before the production header exists.
- [x] Add tests for:
  - no-neighbor pass-through returns desired velocity unchanged.
  - overlapping neighbor pushes the adjusted velocity away from the neighbor.
  - head-on moving neighbor adjusts the velocity laterally or away before collision.
  - deterministic neighbor ordering produces stable replay.
  - adjusted velocity is clamped to `max_speed`.
  - invalid agent, invalid neighbor, invalid radius, invalid speed, and non-finite values report deterministic diagnostics.
- [x] Run the focused build and confirm RED.

Expected RED command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests
```

Expected failure: compiler cannot open `mirakana/navigation/local_avoidance.hpp`.

### Task 2: Local Avoidance API And Implementation

**Files:**
- Create: `engine/navigation/include/mirakana/navigation/local_avoidance.hpp`
- Create: `engine/navigation/src/local_avoidance.cpp`
- Modify: `engine/navigation/CMakeLists.txt`

- [x] Define `NavigationLocalAvoidanceAgentId` as `std::uint64_t`.
- [x] Define `NavigationLocalAvoidanceStatus` with `success` and `invalid_request`.
- [x] Define `NavigationLocalAvoidanceDiagnostic` with `none`, `invalid_agent_id`, `invalid_position`, `invalid_desired_velocity`, `invalid_radius`, `invalid_max_speed`, `invalid_neighbor_id`, `invalid_neighbor_position`, `invalid_neighbor_velocity`, and `invalid_neighbor_radius`.
- [x] Define `NavigationLocalAvoidanceAgentDesc` with `id`, `position`, `desired_velocity`, `radius`, and `max_speed`.
- [x] Define `NavigationLocalAvoidanceNeighborDesc` with `id`, `position`, `velocity`, and `radius`.
- [x] Define `NavigationLocalAvoidanceRequest` with `agent`, `neighbors`, `separation_weight`, `prediction_time_seconds`, and `epsilon`.
- [x] Define `NavigationLocalAvoidanceResult` with `status`, `diagnostic`, `adjusted_velocity`, `applied_neighbor_count`, and `clamped_to_max_speed`.
- [x] Implement `calculate_navigation_local_avoidance` using deterministic separation accumulation over valid neighbor rows, skipping self and non-overlapping neighbors beyond prediction range.
- [x] Clamp final velocity magnitude to `max_speed` when needed.

### Task 3: Green Navigation Tests

**Files:**
- Modify: `tests/unit/navigation_tests.cpp`

- [x] Run focused navigation build and CTest until all new tests pass.
- [x] Keep existing grid pathfinding, replan, path-following, steering, and agent tests passing.
- [x] Add edge coverage if the first implementation misses deterministic diagnostics or clamping behavior.

Focused command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_navigation_tests"
```

### Task 4: Docs And Agent Sync

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
- [x] Mark local avoidance implemented only after validation.
- [x] Keep navmesh assets, full crowd simulation, RVO/ORCA, path smoothing, scene/physics integration, and editor visualization listed as follow-up work.

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

- RED: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests` failed before production header existed with missing `mirakana/navigation/local_avoidance.hpp`.
- Reviewer RED: focused build failed when tests referenced missing `duplicate_neighbor_id` and `calculation_overflow` diagnostics.
- Focused: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_navigation_tests"` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config passed; Visual Studio dev preset did not emit `out\build\dev\compile_commands.json`, so strict analysis remains diagnostic-only.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed, 22/22 tests passed.
- Known diagnostic-only host gates remain: Metal `metal` / `metallib` missing, Apple packaging requires macOS/Xcode with `xcodebuild` / `xcrun`, Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database missing for the Visual Studio generator.
