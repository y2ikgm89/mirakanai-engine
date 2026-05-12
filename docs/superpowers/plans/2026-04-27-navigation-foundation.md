# Navigation Foundation Implementation Plan (2026-04-27)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic first-party grid pathfinding foundation as `mirakana_navigation`.

**Architecture:** `mirakana_navigation` is a new standard-library-only engine module with public headers under `engine/navigation/include/mirakana/navigation`. It exposes explicit value types and a deterministic A* function while keeping navmesh, steering, crowd avoidance, and editor visualization for later slices.

**Tech Stack:** C++23, target-based CMake, CTest, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

### Task 1: Red Test For Navigation Contract

**Files:**
- Create: `tests/unit/navigation_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add tests for straight paths, obstacle detours, endpoint validation, unreachable goals, and cost-aware path selection.
- [x] Register `mirakana_navigation_tests` in CMake.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Expected: build fails because `mirakana/navigation/navigation_grid.hpp` is not implemented yet.

### Task 2: Implement mirakana_navigation

**Files:**
- Create: `engine/navigation/CMakeLists.txt`
- Create: `engine/navigation/include/mirakana/navigation/navigation_grid.hpp`
- Create: `engine/navigation/src/navigation_grid.cpp`
- Modify: `CMakeLists.txt`

- [x] Add the module target with scoped include directories and C++23 common options.
- [x] Implement deterministic grid validation and A* pathfinding.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Expected: `mirakana_navigation_tests` builds.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`.
- [x] Expected: navigation tests pass.

### Task 3: Sync Engine Guidance

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`

- [x] Record `mirakana_navigation` in the engine manifest.
- [x] Move grid pathfinding from missing to implemented foundation while keeping navmesh/crowd AI as future work.
- [x] Update generated-game guidance so games use `mirakana::find_navigation_path` for deterministic grid navigation.

### Task 4: Validation

**Files:**
- No source edits.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record any host/toolchain diagnostic blockers exactly.
