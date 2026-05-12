# Editor Prefab Variant Batch Resolution Review v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed batch-resolution path for prefab variant conflicts so the editor can apply all currently safe cleanup/retarget rows as one undoable action.

**Architecture:** Keep the conflict review contract in `editor/core` and keep Dear ImGui as a thin shell adapter. The batch path repeatedly applies the same reviewed single-row resolution metadata already exposed by `PrefabVariantConflictReviewModel`; it does not introduce automatic merge/rebase behavior, nested prefab propagation, package scripts, validation recipe execution, renderer/RHI work, native handles, or file mutation outside the existing authoring document action path.

**Tech Stack:** C++23, `MK_editor_core`, `MK_ui` retained models, Dear ImGui shell adapter, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Provide a narrow editor-productization improvement over the existing prefab variant review lane:

- Report an explicit `Apply All Reviewed Resolutions` batch plan when conflict rows have reviewed safe resolutions.
- Apply all currently available safe remove/retarget resolutions in deterministic review order.
- Wrap the batch apply as one `UndoStack` action for the visible editor.
- Surface retained `prefab_variant_conflicts.batch_resolution` ids for AI/editor diagnostics.

## Context

- `Editor Prefab Variant Reviewed Resolution v1`, `Missing Node Cleanup v1`, and `Node Retarget Review v1` already provide reviewed single-row resolutions.
- The visible editor currently renders one `Apply` button per row.
- The master plan still lists nested prefab propagation/merge resolution UX as an editor-productization gap. This slice only improves reviewed cleanup ergonomics; it must not claim nested propagation or automatic merge/rebase.

## Constraints

- Keep `editor/core` GUI-independent.
- Reuse `resolve_prefab_variant_conflict` semantics; do not create a separate unsafe merge path.
- Recompute the conflict model after each applied row so duplicate row ids and override indices cannot go stale.
- Batch action is valid only when the resulting variant is valid after apply.
- Update Codex and Claude editor guidance together.

## Done When

- A RED `MK_editor_core_tests` case proves the batch API and retained ids are missing before implementation.
- The batch API applies duplicate cleanup, missing-node source retarget, and redundant cleanup as one result.
- `make_prefab_variant_conflict_batch_resolution_action` produces one undoable action.
- `MK_editor` shows an `Apply All Reviewed` control over the existing Prefab Variant Authoring conflict review.
- Docs, master plan, plan registry, manifest, and static AI/editor checks describe the boundary truthfully.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor prefab variant batch resolution applies reviewed rows with one undo")` covering:
  - duplicate override cleanup,
  - missing-node retarget through unique `source_node_name`,
  - redundant override cleanup,
  - retained `prefab_variant_conflicts.batch_resolution.*` ids,
  - one undo restoring the original stale variant.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Confirm the build fails because `PrefabVariantConflictBatchResolutionPlan`, `resolve_prefab_variant_conflicts`, and `make_prefab_variant_conflict_batch_resolution_action` do not exist.

### Task 2: Editor-Core Batch Contract

**Files:**
- Modify: `editor/core/include/mirakana/editor/prefab_variant_authoring.hpp`
- Modify: `editor/core/src/prefab_variant_authoring.cpp`

- [x] Add `PrefabVariantConflictBatchResolutionPlan` and `PrefabVariantConflictBatchResolutionResult`.
- [x] Add `PrefabVariantConflictReviewModel::batch_resolution`.
- [x] Implement `make_prefab_variant_conflict_batch_resolution_plan`.
- [x] Implement `resolve_prefab_variant_conflicts` by recomputing the review model after every applied row.
- [x] Implement `make_prefab_variant_conflict_batch_resolution_action`.
- [x] Add retained UI ids under `prefab_variant_conflicts.batch_resolution`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Visible Editor Adapter

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add an `Apply All Reviewed` button before the conflict table.
- [x] Route it through `make_prefab_variant_conflict_batch_resolution_action`.
- [x] Keep per-row `Apply` buttons unchanged.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Guidance, Static Checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

- [x] Record that batch resolution is reviewed, explicit, and undoable.
- [x] Keep nested prefab propagation, automatic merge/rebase, package scripts, validation recipes, renderer/RHI uploads, native handles, and broad editor readiness unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Pass (expected failure) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation because `PrefabVariantConflictBatchResolutionPlan`, `resolve_prefab_variant_conflicts`, and `make_prefab_variant_conflict_batch_resolution_action` were missing. |
| Focused `MK_editor_core_tests` | Pass | `cmake --build --preset dev --target MK_editor_core_tests`; `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed 1/1. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Pass | Built `MK_editor` and passed 46/46 desktop-gui tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-readiness-audit-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied clang-format to `prefab_variant_authoring.cpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | `public-api-boundary-check: ok`. |
| `git diff --check` | Pass | No whitespace errors; Git reported only expected LF-to-CRLF working-copy warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Passed full validation; Metal/Apple lanes remain diagnostic/host-gated as expected on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Built the `dev` preset successfully. |
| Slice-closing commit | Pending |  |
