# Behavior Tree Foundation v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a dependency-free behavior tree evaluation foundation so generated games can model simple selector/sequence/action/condition AI decisions without creating game-local tree evaluators or depending on scene, physics, navigation, editor, middleware, scripting, or platform APIs.

**Architecture:** Introduce `mirakana_ai` as a standard-library-only engine module with value-type behavior tree descriptions and a deterministic tick evaluator. Leaf actions/conditions remain game-owned and report explicit statuses into the evaluator; the engine owns tree traversal, validation, status propagation, diagnostics, and replay-friendly results only. Navigation agent movement, scene components, physics queries, blackboard storage, async jobs, and editor graph authoring remain follow-up slices.

**Tech Stack:** C++23, new `mirakana_ai` module, first-party unit tests, no new third-party dependencies.

---

## Context

- `mirakana_navigation` now has pathfinding, path following, arrive steering, and value-type navigation agent movement ticks.
- Production AI still lacks first-party decision-flow contracts comparable to a minimal behavior tree.
- A deterministic evaluator that consumes externally supplied leaf statuses is host-independent and avoids prematurely committing scene/physics/blackboard integration.

## Constraints

- Do not add renderer, RHI, scene, runtime, physics, navigation dependencies, platform, editor, SDL3, Dear ImGui, scripting, middleware, or third-party dependencies.
- Do not implement blackboard storage, service nodes, decorators beyond the minimal v0 set, async tasks, dynamic replanning, navmesh integration, perception, gameplay component ownership, visual graph authoring, or editor tooling in this slice.
- Keep public API in namespace `mirakana::`.
- New public headers require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- New CMake targets must be installed/exported consistently through existing package target lists.

## Done When

- [x] `mirakana_ai` exposes value-type behavior tree node/tree/leaf-result/tick-result contracts.
- [x] The evaluator supports deterministic `sequence`, `selector`, `action`, and `condition` nodes with stable child ordering.
- [x] Invalid trees report deterministic diagnostics for missing root, duplicate ids, missing child ids, invalid leaf results, missing leaf results, duplicate leaf results, empty composites, leaf children, and cycle/depth overflow protection.
- [x] Tests cover success/failure/running propagation, selector/sequence ordering, condition/action leaf results, invalid tree diagnostics, deterministic replay, and cycle rejection.
- [x] Docs, roadmap, gap analysis, manifest, skills, and Codex/Claude guidance describe this as Behavior Tree Foundation v0, not blackboards, async tasks, perception, scene/physics/navigation integration, or editor graph authoring.
- [x] Focused AI tests, API boundary, schema/agent/format checks, `tidy-check`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Behavior Tree Tests

**Files:**
- Create: `tests/unit/ai_tests.cpp`
- Inspect: `CMakeLists.txt`

- [x] Add tests for sequence success, first failure, first running, and visited-node ordering.
- [x] Add tests for selector first success, first running, all failure, and visited-node ordering.
- [x] Add tests for action/condition leaf statuses supplied externally by node id.
- [x] Add tests for missing root, duplicate ids, missing child ids, missing leaf status, invalid leaf status, duplicate leaf status, leaf children, empty composites, and cycle rejection.
- [x] Verify the new tests fail for missing APIs before implementation.

### Task 2: `mirakana_ai` Module And Evaluator

**Files:**
- Create: `engine/ai/include/mirakana/ai/behavior_tree.hpp`
- Create: `engine/ai/src/behavior_tree.cpp`
- Create: `engine/ai/CMakeLists.txt`
- Modify: `CMakeLists.txt`

- [x] Add `mirakana_ai` target with scoped public include paths and common target options.
- [x] Add value types for node ids, node kinds, node descriptions, tree descriptions, leaf statuses, diagnostics, and tick results.
- [x] Implement deterministic validation and traversal without callbacks or external dependencies.
- [x] Add `mirakana_ai_tests` CTest target linked only to `mirakana_ai`.

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

- [x] Mark behavior trees honestly as implemented only after tests and validation pass.
- [x] Keep blackboard storage, async tasks, perception, scene/physics/navigation integration, visual authoring, and editor graph tooling as follow-up work.

### Task 4: Verification

- [x] Run focused AI tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED test evidence: `cmake --build --preset dev --target mirakana_ai_tests` failed before implementation because `mirakana/ai/behavior_tree.hpp` did not exist. Review-driven RED tests later failed for missing `leaf_has_children` and `duplicate_leaf_result` diagnostics before those were implemented.
- Focused AI validation: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_ai_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_ai_tests"` passed.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. `tidy-check` remains diagnostic-only because `out/build/dev/compile_commands.json` is missing under the Visual Studio generator; shader/mobile checks report the known Metal/Apple/signing/device host gates.
