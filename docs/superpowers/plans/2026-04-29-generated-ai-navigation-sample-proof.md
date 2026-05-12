# Generated AI Navigation Sample Proof Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a source-tree sample game that proves AI-generated gameplay can combine the new `mirakana_ai` behavior tree evaluator with `mirakana_navigation` grid pathfinding, path following, and navigation agent movement through public `mirakana::` APIs only.

**Architecture:** Create `games/sample_ai_navigation` as a headless `mirakana::GameApp` sample. The sample owns game-specific behavior tree leaf decisions, builds a deterministic grid path, converts it to points, drives a `NavigationAgentState`, and reports a stable smoke result. It must not add engine API, renderer, RHI, scene, physics, editor, platform, SDL3, Dear ImGui, middleware, scripting, or third-party dependencies.

**Tech Stack:** C++23, `mirakana_ai`, `mirakana_navigation`, `mirakana_core`, first-party sample executable, game manifest/docs sync, no new dependencies.

---

## Context

- `mirakana_ai` Behavior Tree Foundation v0 is implemented as memoryless selector/sequence/action/condition evaluation with game-supplied leaf statuses.
- `mirakana_navigation` already provides deterministic grid pathfinding, point-path conversion, explicit path following, arrive steering, and value-type navigation agent movement ticks.
- AI-generated games need a concrete example that composes these systems without inventing game-local behavior tree or navigation controller frameworks.

## Constraints

- Keep all new gameplay code under `games/sample_ai_navigation`.
- Use only public `mirakana::` headers.
- Do not add new engine APIs or dependencies unless validation proves a real gap.
- Do not imply navmesh assets, crowd avoidance, dynamic replanning, blackboards, async tasks, perception, scene/physics integration, renderer presentation, or editor visualization are implemented.
- Register the sample as a default source-tree game with `mirakana_add_game`, not as a desktop runtime package target.

## Done When

- [x] `sample_ai_navigation` builds and runs under the default source-tree CTest lane.
- [x] The sample uses `mirakana::evaluate_behavior_tree` to choose movement only when a path is available and the agent has not reached the destination.
- [x] The sample uses `mirakana::NavigationGrid`, `mirakana::find_navigation_path`, `mirakana::build_navigation_point_path_result`, `mirakana::make_navigation_agent_state`, `mirakana::replace_navigation_agent_path`, and `mirakana::update_navigation_agent`.
- [x] The sample exits with deterministic final position/status/frame counters and non-zero failure on regression.
- [x] `game.agent.json`, README, generated-game docs/specs, roadmap/testing/manifest, skills, and Codex/Claude gameplay guidance describe this as a sample proof only.
- [x] Focused sample build/CTest, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Sample Registration

**Files:**
- Modify: `games/CMakeLists.txt`

- [x] Register `sample_ai_navigation` with `mirakana_add_game`.
- [x] Verify the build fails before the sample source and manifest/docs exist.

### Task 2: Sample Game, Manifest, And README

**Files:**
- Create: `games/sample_ai_navigation/main.cpp`
- Create: `games/sample_ai_navigation/README.md`
- Create: `games/sample_ai_navigation/game.agent.json`
- Modify: `games/CMakeLists.txt`

- [x] Implement a deterministic headless `mirakana::GameApp` that builds a grid path and point path at startup.
- [x] Evaluate a behavior tree each tick using explicit leaf statuses such as `has_path`, `needs_move`, and `move_action`.
- [x] Update a `NavigationAgentState` until `reached_destination`.
- [x] Print a stable smoke line and return failure if behavior tree status, navigation status, frame count, or final position drift.
- [x] Keep the manifest aligned with source-tree-default packaging and public module usage.

### Task 3: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] Record `sample_ai_navigation` as a proof sample, not a new engine capability beyond `mirakana_ai` and `mirakana_navigation`.
- [x] Keep all missing advanced AI/navigation systems listed as follow-up work.

### Task 4: Verification

- [x] Run focused sample build.
- [x] Run focused sample CTest.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED: after registering `sample_ai_navigation` before creating the source, `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --preset dev; Invoke-CheckedCommand $tools.CMake --build --preset dev --target sample_ai_navigation` failed as expected because `sample_ai_navigation/main.cpp` did not exist.
- Focused sample validation: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --preset dev; Invoke-CheckedCommand $tools.CMake --build --preset dev --target sample_ai_navigation; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "^sample_ai_navigation$"` passed.
- Agent/schema/format validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed. One sandboxed parallel `schema-check` run hit a transient `bunsh: Operation not permitted` path access issue; the standalone rerun passed.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including `sample_ai_navigation` in the default CTest lane. Existing diagnostic-only host gates remain: Metal `metal`/`metallib` missing, Apple packaging requires macOS/Xcode, Android release signing is not configured, Android device smoke is not connected, and strict clang-tidy analysis lacks a compile database for the Visual Studio generator.
