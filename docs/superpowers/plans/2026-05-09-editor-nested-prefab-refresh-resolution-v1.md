# Editor Nested Prefab Refresh Resolution Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend reviewed scene prefab instance refresh so nested linked prefab instances can be explicitly preserved under refreshed parent prefab anchors.

**Architecture:** Keep nested prefab merge decisions in `mirakana_editor_core` as deterministic refresh-policy rows and undoable scene replacement. The optional `mirakana_editor` Dear ImGui shell exposes only explicit reviewed toggles and still routes mutation through `SceneAuthoringDocument` actions.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_editor`, first-party `mirakana_ui` retained models, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Plan ID:** `editor-nested-prefab-refresh-resolution-v1`

**Status:** Completed.

---

## Context

- Active master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Selected gap: `editor-productization`.
- Previous slice: `docs/superpowers/plans/2026-05-09-editor-dynamic-game-module-driver-load-v1.md`.
- Selected-root scene prefab refresh can preserve generic local child subtrees and stale source-node subtrees through explicit policies, but nested linked prefab instances are still treated as generic local/differently linked children.
- This slice adds explicit nested prefab preservation rows for reviewed parent-prefab refresh. It does not add automatic nested prefab refresh, fuzzy matching, automatic merge/rebase/resolution UX, runtime prefab instance semantics, package script execution, validation recipe execution, renderer/RHI uploads, package streaming, or native handles.

## Files

- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
  - Add `keep_nested_prefab_instances` refresh policy state.
  - Add nested prefab keep/block row kinds.
  - Add plan/result counters for reviewed nested prefab preservation.
- Modify: `editor/core/src/scene_authoring.cpp`
  - Detect nested prefab roots as valid differently linked `ScenePrefabSourceLink` rows anchored under retained refreshed parent source nodes.
  - Block nested roots by default with explicit retained diagnostics.
  - Preserve reviewed nested prefab subtrees only when `keep_nested_prefab_instances` is enabled.
  - Keep nested prefab source links intact while preserving selection remapping and retained UI rows.
- Modify: `tests/unit/editor_core_tests.cpp`
  - Add failing coverage for default blocked nested prefab roots and explicit reviewed nested preservation.
  - Keep local-child and stale-source refresh behavior covered.
- Modify: `editor/src/main.cpp`
  - Add a visible `Keep Nested Prefab Instances` control.
  - Pass the expanded refresh policy into plan and undoable apply paths.
- Modify: `docs/editor.md`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`
  - Document the narrowed capability and unsupported boundaries.
- Modify: `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`
  - Keep Codex and Claude editor guidance aligned.
- Modify: `engine/agent/manifest.json`
  - Update the active plan pointer and editor-productization capability text.
- Modify: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`
  - Add sentinel checks for nested prefab refresh review.
- Modify: `docs/superpowers/plans/README.md`
  - Track this plan as the active slice, then latest completed evidence at closeout.
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update current verdict and selected-gap evidence after validation.

## Done When

- Default selected-root refresh reports nested linked prefab roots as blocked reviewed rows instead of silently treating them as generic local children.
- With `keep_nested_prefab_instances` enabled, nested linked prefab subtrees under retained refreshed parent anchors are preserved with their nested `prefab_source` links intact.
- Generic local-child preservation still requires `keep_local_children`; stale source-node preservation still requires `keep_stale_source_nodes_as_local`.
- The plan/result/UI model exposes kept-nested counts, retained `keep_nested_prefab_instance` rows, and blocked `unsupported_nested_prefab_instance` rows.
- The visible editor exposes an explicit nested-prefab keep toggle and still requires `Apply Reviewed Prefab Refresh`.
- Focused `mirakana_editor_core_tests` pass.
- GUI build passes because `editor/src/main.cpp` changes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `git diff --check -- <touched files>`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Tasks

### Task 1: Add failing editor-core coverage

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add a test named `editor scene prefab instance refresh review can keep nested prefab instances`.
- [x] Assert the default policy reports `unsupported_nested_prefab_instance` for a nested linked prefab root anchored under a retained parent source node.
- [x] Assert `keep_nested_prefab_instances` reports `keep_nested_prefab_instance`, preserves the nested root and descendants with nested source links intact, preserves selected-node remapping, and remains undoable.
- [x] Build `mirakana_editor_core_tests` and verify the test fails before implementation.

Run:

```powershell
cmake --build --preset dev --target mirakana_editor_core_tests
```

Expected before implementation: compile failure for the new policy flag, row kinds, and nested counters.

### Task 2: Implement nested prefab refresh planning

**Files:**
- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Modify: `editor/core/src/scene_authoring.cpp`

- [x] Add `ScenePrefabInstanceRefreshPolicy::keep_nested_prefab_instances`.
- [x] Add `ScenePrefabInstanceRefreshRowKind::keep_nested_prefab_instance`.
- [x] Add `ScenePrefabInstanceRefreshRowKind::unsupported_nested_prefab_instance`.
- [x] Add `keep_nested_prefab_instance_count` to `ScenePrefabInstanceRefreshPlan`.
- [x] Add `kept_nested_prefab_instance_count` to `ScenePrefabInstanceRefreshResult`.
- [x] Detect nested prefab roots only when the node has a valid different prefab source identity, `source_node_index == 1`, and a retained refreshed parent source-node anchor.
- [x] Block unsafe nested preservation when the nested subtree contains parent-prefab source-link descendants.

### Task 3: Apply nested prefab preservation

**Files:**
- Modify: `editor/core/src/scene_authoring.cpp`

- [x] Build preserved subtrees from reviewed `keep_local_child`, `keep_stale_source_node_as_local`, and `keep_nested_prefab_instance` rows rather than broad policy flags.
- [x] Preserve nested prefab names, transforms, components, child relationships, and nested `prefab_source` links.
- [x] Reparent kept nested roots under the corresponding refreshed parent.
- [x] Preserve selected-node remapping for kept nested nodes and descendants.

### Task 4: Wire retained UI and visible shell

**Files:**
- Modify: `editor/core/src/scene_authoring.cpp`
- Modify: `editor/src/main.cpp`

- [x] Add retained labels for nested keep counts and policy state.
- [x] Add a visible `Keep Nested Prefab Instances` checkbox next to the existing refresh-policy controls.
- [x] Pass all refresh policy flags into planning and reviewed apply.
- [x] Keep visible mutation behind `Apply Reviewed Prefab Refresh`.

### Task 5: Synchronize docs, manifest, skills, and static checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/roadmap.md`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

- [x] Document nested prefab preservation as an explicit reviewed refresh policy.
- [x] Keep unsupported claims explicit: no automatic nested refresh, fuzzy matching, automatic merge/rebase, package/validation execution, renderer/RHI uploads, native handles, or package streaming.
- [x] Add sentinels for the new row kinds, policy flag, counters, UI labels, visible shell checkbox, docs, skills, and manifest text.

### Task 6: Validate and close the slice

**Files:**
- Modify: this plan file.

- [x] Run focused editor-core build and tests.
- [x] Run GUI build.
- [x] Run relevant static checks.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact validation evidence in this plan.
- [x] Set this plan status to `Completed`, move `currentActivePlan` back to the master plan, and update the plan registry latest completed slice.

## Validation Evidence

- RED: `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation because `keep_nested_prefab_instances`, `keep_nested_prefab_instance`, `unsupported_nested_prefab_instance`, `keep_nested_prefab_instance_count`, and `kept_nested_prefab_instance_count` were not implemented.
- GREEN: `cmake --build --preset dev --target mirakana_editor_core_tests` passed.
- GREEN: `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed: 1/1 test passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed: `json-contract-check: ok`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed: `ai-integration-check: ok`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` passed, including `mirakana_editor` and 46/46 GUI preset tests.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed: `public-api-boundary-check: ok`.
- GREEN: `git diff --check` passed after closeout metadata updates.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed: license, agent config, JSON contracts, validation recipe runner, installed SDK validation, release package artifacts, AI integration, production readiness audit, CI matrix, dependency policy, vcpkg environment, toolchain, C++ standard policy, coverage thresholds, public API boundary, tidy smoke, default build, and 29/29 default preset tests all passed. Diagnostic-only host gates remain explicit: Metal/Apple toolchain is unavailable on this Windows host.

## Status

Completed.
