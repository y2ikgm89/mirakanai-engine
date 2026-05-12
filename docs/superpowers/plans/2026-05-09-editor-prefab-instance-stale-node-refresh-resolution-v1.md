# Editor Prefab Instance Stale Node Refresh Resolution Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend reviewed scene prefab instance refresh so an author can explicitly keep stale source-node subtrees as local author-owned scene nodes.

**Architecture:** Keep stale-node merge decisions in `mirakana_editor_core` as deterministic refresh-policy rows and undoable scene replacement. The optional `mirakana_editor` Dear ImGui shell only exposes an explicit reviewed toggle and still routes mutation through `SceneAuthoringDocument` actions.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_editor`, first-party `mirakana_ui` retained models, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Plan ID:** `editor-prefab-instance-stale-node-refresh-resolution-v1`

**Status:** Completed.

---

## Context

- Active master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Selected gap: `editor-productization`.
- Previous slice: `docs/superpowers/plans/2026-05-09-editor-prefab-instance-local-child-refresh-resolution-v1.md`.
- Current selected-root refresh can preserve local child subtrees through `keep_local_children`, but stale source nodes removed from the refreshed source prefab are still only reviewed removal rows.
- This slice adds an explicit stale-node keep-as-local policy. It does not add fuzzy matching, automatic merge/rebase, nested prefab propagation, runtime prefab semantics, package script execution, validation recipe execution, renderer/RHI uploads, package streaming, or native handles.

## Files

- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
  - Add a stale-source-node keep policy flag.
  - Add a stale keep-as-local row kind.
  - Add plan/result counters for kept stale source-node subtrees.
- Modify: `editor/core/src/scene_authoring.cpp`
  - Detect stale source-node subtree roots deterministically.
  - Keep stale source-node subtrees only when explicitly reviewed and their nearest retained refreshed source parent remains present.
  - Rebuild kept stale subtrees as local author-owned nodes by clearing same-source prefab links.
  - Preserve selection remapping and retained UI rows.
- Modify: `tests/unit/editor_core_tests.cpp`
  - Add failing coverage for reviewed stale source-node preservation.
  - Keep default removal behavior covered.
- Modify: `editor/src/main.cpp`
  - Add a visible `Keep Stale Source Nodes` control.
  - Pass the expanded refresh policy into plan and undoable apply paths.
- Modify: `docs/editor.md`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`
  - Document the narrowed capability and unsupported boundaries.
- Modify: `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`
  - Keep Codex and Claude editor guidance aligned.
- Modify: `engine/agent/manifest.json`
  - Update the active plan pointer and editor-productization capability text.
- Modify: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`
  - Add sentinel checks for the reviewed stale keep-as-local policy.
- Modify: `docs/superpowers/plans/README.md`
  - Track this plan as the active slice, then latest completed evidence at closeout.
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update current verdict and selected-gap evidence after validation.

## Done When

- Default refresh still reports stale source nodes as reviewed removal rows and removes them on apply.
- With the explicit stale keep-as-local policy enabled, stale source-node subtrees under retained refreshed parents are preserved as local scene subtrees.
- Kept stale source nodes from the refreshed prefab identity have their `prefab_source` cleared so they become author-owned local nodes.
- The plan/result/UI model exposes kept-stale counts and retained `keep_stale_source_node_as_local` rows.
- The visible editor exposes an explicit stale-node keep toggle and still requires `Apply Reviewed Prefab Refresh`.
- Focused `mirakana_editor_core_tests` pass.
- GUI build passes because `editor/src/main.cpp` changes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `git diff --check -- <touched files>`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Tasks

### Task 1: Add failing editor-core coverage

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add a test named `editor scene prefab instance refresh review can keep stale source node subtrees as local`.
- [x] Assert the default policy still reports `remove_stale_node`.
- [x] Assert the explicit policy reports `keep_stale_source_node_as_local`, preserves stale descendants, clears same-source `prefab_source`, preserves selected-node remapping, and remains undoable.
- [x] Build `mirakana_editor_core_tests` and verify the test fails before implementation.

Run:

```powershell
cmake --build --preset dev --target mirakana_editor_core_tests
```

Expected before implementation: compile failure for the new policy flag, row kind, and kept-stale counters.

### Task 2: Implement stale source-node keep-as-local planning

**Files:**
- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Modify: `editor/core/src/scene_authoring.cpp`

- [x] Add `ScenePrefabInstanceRefreshPolicy::keep_stale_source_nodes_as_local`.
- [x] Add `ScenePrefabInstanceRefreshRowKind::keep_stale_source_node_as_local`.
- [x] Add `keep_stale_source_node_count` to `ScenePrefabInstanceRefreshPlan`.
- [x] Add `kept_stale_source_node_count` to `ScenePrefabInstanceRefreshResult`.
- [x] Keep default stale rows as `remove_stale_node`.
- [x] When the policy is enabled, emit warning rows for stale source-node roots that can be reparented under a retained refreshed source parent.
- [x] Block stale subtree preservation when a stale subtree contains a descendant whose source name still exists in the refreshed prefab, because that would duplicate or split refreshed source ownership.

### Task 3: Apply stale source-node preservation

**Files:**
- Modify: `editor/core/src/scene_authoring.cpp`

- [x] Build a preserved-node set from explicit local children and explicit stale source-node subtrees.
- [x] Copy preserved stale source nodes after refreshed source nodes, preserving names, transforms, components, and child relationships.
- [x] Clear `prefab_source` on copied stale same-source nodes so the kept subtree becomes local author-owned content.
- [x] Reparent kept stale roots under the corresponding refreshed parent.
- [x] Preserve selected-node remapping for kept stale nodes and their local descendants.

### Task 4: Wire retained UI and visible shell

**Files:**
- Modify: `editor/core/src/scene_authoring.cpp`
- Modify: `editor/src/main.cpp`

- [x] Add retained labels for stale keep-as-local counts and policy state.
- [x] Add a visible `Keep Stale Source Nodes` checkbox next to `Keep Local Children`.
- [x] Pass both refresh policy flags into refresh planning and reviewed apply.
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

- [x] Document stale source-node preservation as an explicit reviewed refresh policy.
- [x] Keep unsupported claims explicit: no automatic nested propagation, fuzzy matching, automatic merge/rebase, package/validation execution, renderer/RHI uploads, native handles, or package streaming.
- [x] Add sentinels for the new row kind, policy flag, counters, UI labels, visible shell checkbox, docs, skills, and manifest text.

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

- RED: after adding `editor scene prefab instance refresh review can keep stale source node subtrees as local`, `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation on the missing `keep_stale_source_node_count`, `keep_stale_source_nodes_as_local`, `keep_stale_source_node_as_local`, and `kept_stale_source_node_count` API.
- Focused GREEN: `cmake --build --preset dev --target mirakana_editor_core_tests` passed after implementation.
- Focused GREEN: `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed.
- GUI GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` passed, including 46/46 desktop GUI tests.
- Static GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `git diff --check -- <touched files>` passed. `git diff --check` reported only CRLF normalization warnings.
- Slice validation GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 29/29 CTest tests passing. `production-readiness-audit-check` still reports `unsupported_gaps=11`, with `editor-productization` still `partly-ready`; this slice narrows that gap but does not close it.
- Host-gated diagnostics retained: Metal shader tooling is missing on this Windows host (`metal` / `metallib`), Apple packaging and Apple host evidence remain host-gated, and those lanes are diagnostic-only in `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Status

Completed.
