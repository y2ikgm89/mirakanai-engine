# Editor Prefab Instance Local Child Refresh Resolution Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend reviewed scene prefab instance refresh so an author can explicitly keep local child subtrees while refreshing a selected linked prefab root.

**Architecture:** Keep the merge decision in `mirakana_editor_core` as a reviewed, deterministic refresh policy and row model. The optional `mirakana_editor` Dear ImGui shell only exposes an explicit keep-local toggle and still routes all mutation through undoable `SceneAuthoringDocument` actions.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_editor`, first-party `mirakana_ui` retained models, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Plan ID:** `editor-prefab-instance-local-child-refresh-resolution-v1`

**Status:** Completed.

---

## Context

- Active master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Selected gap: `editor-productization`.
- Previous slice: `docs/superpowers/plans/2026-05-09-editor-scene-prefab-instance-refresh-review-v1.md`.
- Current refresh blocks unlinked or differently linked children because applying the refreshed source subtree would delete them without a reviewed merge decision.
- This slice adds an explicit local-child preservation policy. It does not add fuzzy matching, automatic merge/rebase, stale-node keep-as-local resolution, nested prefab propagation, package script execution, validation recipe execution, renderer/RHI uploads, native handles, or runtime prefab semantics.

## Files

- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
  - Add `ScenePrefabInstanceRefreshPolicy`.
  - Add `keep_local_child` row kind.
  - Add plan/result counters for kept local child subtrees.
  - Thread the policy through plan/apply/action APIs.
- Modify: `editor/core/src/scene_authoring.cpp`
  - Detect local child subtree roots deterministically.
  - Keep local subtrees only when the reviewed policy enables it and their refreshed parent anchor remains present.
  - Rebuild preserved local subtrees with original names, transforms, components, source-link metadata, and parenting.
  - Preserve selected-node remapping for kept local nodes.
  - Surface retained `scene_prefab_instance_refresh` UI rows and summary labels for kept local children.
- Modify: `tests/unit/editor_core_tests.cpp`
  - Add failing coverage for reviewed local child preservation.
  - Keep existing blocker coverage for the default policy.
- Modify: `editor/src/main.cpp`
  - Add a visible `Keep Local Children` control for selected prefab refresh.
  - Pass `ScenePrefabInstanceRefreshPolicy` into the plan and undoable action.
- Modify: `docs/editor.md`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`
  - Document the narrowed capability and explicit unsupported boundaries.
- Modify: `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`
  - Keep Codex and Claude editor guidance aligned.
- Modify: `engine/agent/manifest.json`
  - Update current editor/source-link capability text and active plan pointer.
- Modify: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`
  - Add sentinel checks for the new reviewed local-child refresh policy.
- Modify: `docs/superpowers/plans/README.md`
  - Track this plan as the active slice while work is in progress, then as latest completed evidence at closeout.
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update the current verdict and selected-gap evidence after validation.

## Done When

- Default refresh still blocks unsupported local children.
- With `ScenePrefabInstanceRefreshPolicy::keep_local_children == true`, local child subtrees under retained refreshed source nodes are preserved and remain undoable.
- The plan/result/UI model exposes kept-local counts and `keep_local_child` rows.
- The visible editor has an explicit keep-local control and still does not execute package scripts, validation recipes, or renderer/RHI work from editor core.
- Focused `mirakana_editor_core_tests` pass.
- GUI build passes because `editor/src/main.cpp` changes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `git diff --check -- <touched files>`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Tasks

### Task 1: Add failing editor-core coverage

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add a test named `editor scene prefab instance refresh review can keep local child subtrees`.
- [x] Build `mirakana_editor_core_tests` and verify the test fails because `ScenePrefabInstanceRefreshPolicy`, `keep_local_child`, and kept-local counters do not exist yet.

Run:

```powershell
cmake --build --preset dev --target mirakana_editor_core_tests
```

Expected before implementation: compile failure for the new refresh policy and row kind.

### Task 2: Implement the reviewed refresh policy

**Files:**
- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Modify: `editor/core/src/scene_authoring.cpp`

- [x] Add `ScenePrefabInstanceRefreshPolicy` with `bool keep_local_children{false}`.
- [x] Add `ScenePrefabInstanceRefreshRowKind::keep_local_child`.
- [x] Add `keep_local_child_count` to `ScenePrefabInstanceRefreshPlan`.
- [x] Add `kept_local_child_count` to `ScenePrefabInstanceRefreshResult`.
- [x] Thread the policy through `plan_scene_prefab_instance_refresh`, `apply_scene_prefab_instance_refresh`, and `make_scene_prefab_instance_refresh_action`.
- [x] Keep default behavior blocking local children.
- [x] When policy is enabled, preserve local child subtrees under refreshed source parents and expose warning rows instead of blockers.

### Task 3: Wire the visible editor shell

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add a `Keep Local Children` checkbox near `Refresh Prefab Instance`.
- [x] Pass `ScenePrefabInstanceRefreshPolicy{.keep_local_children = scene_prefab_refresh_keep_local_children_}` to the refresh plan and undoable action.
- [x] Keep refresh disabled unless the selected node is a linked prefab root with a safe prefab path.
- [x] Stage warning-capable retained refresh rows and require `Apply Reviewed Prefab Refresh` before visible mutation.

### Task 4: Synchronize docs, skills, manifest, and static checks

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

- [x] Document local-child preservation as explicit reviewed refresh policy.
- [x] Keep unsupported claims explicit: no automatic nested propagation, no fuzzy merge/rebase, no package/validation execution, no renderer/RHI/native handles.
- [x] Add sentinels for `ScenePrefabInstanceRefreshPolicy`, `keep_local_child`, retained UI labels, visible shell checkbox, docs, skills, and manifest text.

### Task 5: Validate and close the slice

**Files:**
- Modify: this plan file.

- [x] Run focused editor-core build and tests.
- [x] Run GUI build.
- [x] Run relevant static checks.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact validation evidence in this plan.
- [x] Set this plan status to `Completed`, move `currentActivePlan` back to the master plan, and update the plan registry latest completed slice.

## Validation Evidence

- RED: `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation on missing `mirakana::editor::ScenePrefabInstanceRefreshPolicy`, `keep_local_child`, and kept-local counters.
- Focused editor-core build: `cmake --build --preset dev --target mirakana_editor_core_tests` passed.
- Focused editor-core tests: `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed.
- GUI build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` passed, including 46/46 desktop-gui CTest tests.
- Static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Slice-closing validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. The production readiness audit still reports 11 unsupported gaps; `editor-productization` remains `partly-ready`.
- Host gates observed during validation: Metal shader tools are missing on this Windows host, and Apple host evidence remains macOS/Xcode gated; both are diagnostic-only for this slice.

## Status

Completed.
