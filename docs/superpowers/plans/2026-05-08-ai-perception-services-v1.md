# AI Perception Services v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `ai-perception-services-v1`  
**Status:** Complete.  
**Goal:** Add a first-party deterministic AI perception service contract that turns explicit 2D agent/target rows into behavior-tree blackboard facts for generated games without claiming scene, physics, middleware, async services, or editor graph authoring.

**Architecture:** Keep perception in `mirakana_ai` as a standard-library-only value API beside behavior tree and blackboard support. Callers own scene/physics queries and provide explicit target rows; `mirakana_ai` computes deterministic sight/hearing facts and projects the best target into `BehaviorTreeBlackboard` entries.

**Tech Stack:** C++23, `mirakana_ai`, existing `dev` preset.

---

## Context

- Master plan row: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md` selected AI perception/services as a runtime-system minimum follow-up before this slice.
- `mirakana_ai` currently has memoryless behavior-tree evaluation and typed blackboard conditions only.
- `sample_ai_navigation` already proves blackboard-driven behavior tree plus navigation, but the game still authors perception-style facts by hand.

## Constraints

- Do not add scene, physics, renderer, platform, editor, scripting, middleware, background jobs, async service ownership, or new third-party dependencies.
- Keep `mirakana_ai` standard-library only and independent from `mirakana_scene`, `mirakana_physics`, `mirakana_navigation`, SDL3, Dear ImGui, and native handles.
- Keep the API deterministic: stable target ordering, explicit diagnostics, finite numeric validation, no hidden global state.
- Use TDD before production code.

## Done When

- `mirakana_ai` exposes `AiPerceptionAgent2D`, `AiPerceptionTarget2D`, `AiPerceptionSnapshot2D`, status/diagnostic enums, `build_ai_perception_snapshot_2d`, and `write_ai_perception_blackboard`.
- Snapshot building validates agent and target rows, rejects duplicate ids and non-finite ranges/positions, computes visible/audible target rows from range/FOV and target signal rows, and chooses a deterministic primary target.
- Blackboard projection writes caller-selected facts such as has-target, target id, distance, visible count, audible count, and target state through the existing `BehaviorTreeBlackboard` API.
- Unit tests prove visible target selection, audible fallback, deterministic tie-breaking, invalid diagnostics, duplicate rejection, blackboard projection, and behavior-tree condition consumption.
- `sample_ai_navigation` uses the new perception projection for its `has_path`/`needs_move` style blackboard setup without changing its headless runtime scope.
- Docs, plan registry, master plan, manifest, AI game-development guidance, and static checks describe the new boundary and keep scene/physics integration, async services, editor graph authoring, utility AI, scripting, and middleware unsupported.
- Focused AI tests and sample smoke, API boundary check, format check, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before committing.

## Files

- Add: `engine/ai/include/mirakana/ai/perception.hpp`
- Add: `engine/ai/src/perception.cpp`
- Modify: `engine/ai/CMakeLists.txt`
- Modify: `tests/unit/ai_tests.cpp`
- Modify: `games/sample_ai_navigation/main.cpp`
- Modify: `games/sample_ai_navigation/game.agent.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/architecture.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

## Tasks

### Task 1: Red Tests

- [x] Add `ai_tests.cpp` tests for visible target selection, audible fallback, deterministic tie-breaking, invalid diagnostics, duplicate ids, blackboard projection, and behavior-tree condition consumption.
- [x] Update `sample_ai_navigation` to use the new perception projection for blackboard facts.
- [x] Run focused AI/sample build and record the expected compile failure before the new APIs exist.

### Task 2: AI Perception Contract

- [x] Add public perception request/result/status/diagnostic rows in `perception.hpp`.
- [x] Implement `build_ai_perception_snapshot_2d` with finite validation, FOV/range classification, stable primary-target selection, and explicit diagnostics.
- [x] Implement `write_ai_perception_blackboard` using existing `BehaviorTreeBlackboard::set`.
- [x] Register the new source file in `engine/ai/CMakeLists.txt`.

### Task 3: Documentation And Contract Checks

- [x] Update current-truth docs, AI guidance, game-development skills, gameplay-builder prompts, manifest, master plan, gap analysis, and registry.
- [x] Update static checks to assert the new AI perception API names and unsupported boundaries.

### Task 4: Validation And Commit

- [x] Run focused AI build and CTest.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and format if needed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `feat: add ai perception services`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_ai_tests sample_ai_navigation` | Expected RED failure | Failed before implementation because `mirakana/ai/perception.hpp` did not exist. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_ai_tests sample_ai_navigation; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_ai_tests|sample_ai_navigation"` | Passed | Focused post-implementation AI tests and sample CTest passed after implementation; rerun passed after adding duplicate projection-key rejection. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | JSON contract check passed after manifest and sample manifest sync. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public header boundary verification passed with `perception.hpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Unsupported production gap audit remained green. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Static AI integration guidance passed after adding perception API names and returning `currentActivePlan` to the master plan. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Initial run found clang-format drift; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting and rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed with 29/29 CTest; Metal/Apple host blockers remained diagnostic-only and changed-file tidy reported `tidy-check: ok (1 files)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit precondition build completed after validation. |

## Status

**Status:** Complete.
