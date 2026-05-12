# Behavior Tree Blackboard Conditions v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add dependency-free typed blackboard values and behavior-tree condition bindings so generated games can drive `mirakana_ai` conditions from explicit world-state snapshots instead of hand-building every condition leaf result.

**Architecture:** Keep the slice inside `mirakana_ai` as standard-library-only C++23 value APIs under `mirakana::`. The behavior tree evaluator remains deterministic and memoryless: callers provide blackboard entries, condition bindings, and action leaf results for each tick; `mirakana_ai` reads typed values for condition nodes and never owns scene, physics, navigation, runtime, editor, platform, async tasks, perception, or middleware state.

**Tech Stack:** C++23, `mirakana_ai`, `sample_ai_navigation`, CTest, no new third-party dependencies.

---

## Context

- `mirakana_ai` currently provides Behavior Tree Foundation v0: selector/sequence/action/condition traversal with externally supplied leaf statuses.
- `sample_ai_navigation` proves `mirakana_ai` + `mirakana_navigation` composition, but it still assembles condition statuses manually each frame.
- A typed blackboard snapshot is the smallest host-feasible AI production slice that improves generated-game ergonomics without crossing engine module boundaries.

## Constraints

- Keep `engine/ai` standard-library-only and independent from navigation, physics, scene, runtime, editor, platform, SDL3, Dear ImGui, native handles, scripting, and third-party middleware.
- Do not implement behavior-tree memory, decorators, services, async tasks, perception, utility AI, editor graph authoring, scene/physics/navigation engine integration, or a world-state service.
- Support only deterministic typed values for v0: bool, signed integer, finite double, and string.
- Keep condition comparison policy explicit: exact equality for bool/integer/double/string, ordered comparisons only for integer and finite double values, no epsilon or fuzzy string matching.
- Preserve existing action leaf behavior and existing manual condition leaf fallback when no blackboard condition binding is supplied.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_ai` exposes typed blackboard value rows, a deterministic `BehaviorTreeBlackboard` helper, blackboard condition bindings, and an evaluation context overload.
- [x] Condition nodes with blackboard bindings evaluate success/failure from caller-supplied blackboard entries.
- [x] Action nodes continue to use game-supplied `BehaviorTreeLeafResult` rows.
- [x] Existing manual condition leaf evaluation still works when a condition node has no blackboard binding.
- [x] Diagnostics cover duplicate blackboard keys, invalid keys/values, duplicate condition bindings, invalid condition bindings, missing keys, type mismatch, and unsupported comparison operators.
- [x] Tests prove typed storage, condition success/failure, action fallback, missing/type mismatch diagnostics, duplicate/invalid descriptors, deterministic replay, and existing behavior-tree semantics.
- [x] `sample_ai_navigation` drives its `has_path` / `needs_move` conditions through a blackboard snapshot and still reaches the deterministic destination.
- [x] Docs, roadmap/gap analysis, manifest, Codex/Claude game-development skills, gameplay-builder agents, and AI checks describe Blackboard Conditions v0 honestly without claiming async tasks, decorators/services, perception, utility AI, scene/physics/navigation integration, middleware, scripting, or editor graph authoring.
- [x] Focused validation, public API boundary, schema/agent/format checks, tidy diagnostics, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Blackboard Tests

**Files:**
- Modify: `tests/unit/ai_tests.cpp`

- [x] Add tests that include and use the new blackboard API before implementation exists.
- [x] Cover deterministic typed storage and replacement through `BehaviorTreeBlackboard`.
- [x] Cover condition nodes reading bool, signed integer, finite double, and string values from an evaluation context.
- [x] Cover missing blackboard key and type mismatch diagnostics.
- [x] Cover duplicate blackboard keys, duplicate condition bindings, invalid condition binding target, invalid comparison operator, and non-finite double rejection.
- [x] Run focused build and confirm RED.

Expected RED command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_ai_tests
```

Expected failure: compiler errors for missing blackboard value, condition, context, or helper APIs.

### Task 2: Blackboard API And Evaluator

**Files:**
- Modify: `engine/ai/include/mirakana/ai/behavior_tree.hpp`
- Modify: `engine/ai/src/behavior_tree.cpp`

- [x] Add `BehaviorTreeBlackboardValueKind`, `BehaviorTreeBlackboardValue`, and named factory helpers for bool, signed integer, finite double, and string values.
- [x] Add `BehaviorTreeBlackboardEntry` and `BehaviorTreeBlackboard` with deterministic insertion order, replacement on duplicate `set`, `find`, and `entries`.
- [x] Add `BehaviorTreeBlackboardComparison` and `BehaviorTreeBlackboardCondition`.
- [x] Add `BehaviorTreeEvaluationContext` and an `evaluate_behavior_tree` overload that accepts it.
- [x] Validate blackboard entry keys/values and condition bindings before traversal.
- [x] Evaluate condition nodes from blackboard bindings when present; otherwise use existing leaf-result fallback.
- [x] Preserve existing overloads by forwarding them through an empty-blackboard context.

### Task 3: GREEN Focused AI Tests

**Files:**
- Modify: `tests/unit/ai_tests.cpp`

- [x] Run focused build and CTest until `mirakana_ai_tests` passes.
- [x] Keep existing behavior-tree foundation tests passing.
- [x] Add edge coverage if condition comparison or diagnostic priority is ambiguous.

Focused command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_ai_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_ai_tests"
```

### Task 4: Sample AI Navigation Proof

**Files:**
- Modify: `games/sample_ai_navigation/main.cpp`
- Modify: `games/sample_ai_navigation/README.md`
- Modify: `games/sample_ai_navigation/game.agent.json`

- [x] Build a per-frame `BehaviorTreeBlackboard` with path readiness and agent movement state.
- [x] Bind `kHasPathNode` and `kNeedsMoveNode` to blackboard conditions.
- [x] Keep `kMoveActionNode` as a game-owned action leaf result.
- [x] Preserve deterministic output, final position, smoothed waypoint count, and frame count.
- [x] Update sample docs and manifest to call this a headless blackboard-condition proof.
- [x] Run `cmake --build --preset dev --target sample_ai_navigation` and `ctest --preset dev --output-on-failure -R "sample_ai_navigation"`.

### Task 5: Docs, Manifest, Skills, Agents, Checks

**Files:**
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`
- Modify: `tools/check-ai-integration.ps1`

- [x] Mark this plan active while implementation is in progress.
- [x] Update docs and manifest to list blackboard conditions as implemented while keeping async/decorator/service/perception/editor graph work as follow-up.
- [x] Update Codex and Claude guidance consistently.
- [x] Add static checks for the new blackboard APIs and stale "no blackboard" guidance.

### Task 6: Validation And Review

**Files:**
- Review all modified files.

- [x] Run focused validation:

```powershell
cmake --build --preset dev --target mirakana_ai_tests sample_ai_navigation
ctest --preset dev --output-on-failure -R "^(mirakana_ai_tests|sample_ai_navigation)$"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1
```

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Request `cpp-reviewer` for implementation review.
- [x] Fix any Critical/Important findings and rerun relevant validation.
