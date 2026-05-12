# Navigation Production Path v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `navigation-production-path-v1`  
**Status:** Complete.  
**Goal:** Add a first-party navigation production path planner that composes validated grid pathfinding, optional smoothing, point-path mapping, and `NavigationAgentState` creation for generated games without claiming navmesh, crowd simulation, scene/physics integration, or middleware.

**Architecture:** Keep the planner in `mirakana_navigation` because it is a dependency-free value contract over existing grid/path/agent APIs. Generated games and headless samples consume the planner through public `mirakana::` headers only.

**Tech Stack:** C++23, `mirakana_navigation`, existing `dev` preset.

---

## Context

- Master plan row: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md` lists `navigation-production-path-v1` under runtime-system minimums.
- `mirakana_navigation` already has deterministic cardinal grid pathfinding, path validation/replan, conservative line-of-sight smoothing, grid-to-point path mapping, path following, arrive steering, local avoidance, and value-type navigation agent movement.
- `sample_ai_navigation` currently composes those APIs manually; the production path slice should give generated games a smaller reviewed surface for the common "grid coord to moving agent" route setup.

## Constraints

- Do not add navmesh assets, polygon mesh navigation, funnel/string-pull smoothing, RVO/ORCA, full crowd simulation, scene ownership, physics ownership, async tasks, editor visualization, middleware, or new dependencies.
- Keep `mirakana_navigation` standard-library only and independent from scene, physics, renderer, platform, and editor modules.
- Keep public API names in `mirakana::`, use value types, and preserve deterministic behavior.
- Use TDD before production code.

## Done When

- `mirakana_navigation` exposes `NavigationGridAgentPathRequest`, `NavigationGridAgentPathPlan`, status/diagnostic enums, and `plan_navigation_grid_agent_path`.
- The planner validates adjacency and point mapping, maps pathfinding failures, optionally smooths successful grid paths, builds point paths, creates a moving `NavigationAgentState`, and records raw/smoothed point counts and cost evidence.
- Unit tests prove smoothed planning, unsmoothed planning, invalid mapping, unsupported adjacency, blocked/no-path pathfinding, and generated-game friendly agent-state output.
- `sample_ai_navigation` uses the new planner instead of manually stitching pathfinding/smoothing/point-mapping/agent-state creation.
- Docs, plan registry, master plan, manifest, AI game-development guidance, and static checks describe the new boundary and keep navmesh/crowd/scene/physics/editor claims unsupported.
- Focused navigation tests, sample smoke through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, API boundary check, format check, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before committing.

## Files

- Add: `engine/navigation/include/mirakana/navigation/navigation_path_planner.hpp`
- Add: `engine/navigation/src/navigation_path_planner.cpp`
- Modify: `engine/navigation/CMakeLists.txt`
- Modify: `tests/unit/navigation_tests.cpp`
- Modify: `games/sample_ai_navigation/main.cpp`
- Modify: `games/sample_ai_navigation/game.agent.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/architecture.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

## Tasks

### Task 1: Red Tests

- [x] Add `navigation_tests.cpp` tests for smoothed planning, unsmoothed planning, invalid mapping, unsupported adjacency, blocked endpoint, no path, and resulting `NavigationAgentState`.
- [x] Update `sample_ai_navigation` to consume `plan_navigation_grid_agent_path`.
- [x] Run focused navigation build and record the expected compile failure before the new APIs exist.

### Task 2: Navigation Production Path Contract

- [x] Add public planner request/result/status/diagnostic rows in `navigation_path_planner.hpp`.
- [x] Implement `plan_navigation_grid_agent_path` by composing existing pathfinding, smoothing, point mapping, and agent-state APIs.
- [x] Register the new source file in `engine/navigation/CMakeLists.txt`.

### Task 3: Documentation And Contract Checks

- [x] Update current-truth docs, AI guidance, game-development skills, manifest, master plan, and registry.
- [x] Update static checks to assert the new navigation planner API names and unsupported boundaries.

### Task 4: Validation And Commit

- [x] Run focused navigation build and CTest.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and format if needed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `feat: add navigation production path planner`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests sample_ai_navigation` | Expected RED failure | Failed before implementation because `mirakana/navigation/navigation_path_planner.hpp` did not exist. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_navigation_tests sample_ai_navigation; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_navigation_tests|sample_ai_navigation"` | Passed | Focused navigation tests and sample CTest passed after implementation and formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | JSON contracts accept `mirakana_navigation` `implemented-production-path-planner`, new public header, planner API, and unsupported navigation boundaries. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI guidance, specs, skills, Codex/Claude gameplay-builder agent prompts, manifest, and docs contain planner API names plus navmesh/crowd/scene/physics/editor boundaries. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Known unsupported gaps remain recorded; no navigation production-ready overclaim was added. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | New public navigation header stays within public API boundary rules. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Required `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` once for `navigation_path_planner.cpp`, then format check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | Timed out outside validate | Full all-repository tidy emitted existing large warning output and timed out at 600s; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` later ran the changed-file tidy lane and reported `tidy-check: ok (1 files)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed; host-gated Apple/Metal diagnostics remained diagnostic-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit precondition build completed after validation. |

## Status

**Status:** Complete.
