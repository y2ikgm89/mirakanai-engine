# Editor Prefab Variant Source Mismatch Retarget Review v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed retarget path when a prefab variant override still targets an existing node index but its recorded `source_node_name` now points to a different current base-prefab node.

**Architecture:** Keep strict `GameEngine.PrefabVariant.v1` composition index-based and unchanged in `MK_scene`. Extend the GUI-independent `MK_editor_core` conflict review model so source-name/index disagreement becomes an explicit reviewed conflict, with mutation only through existing variant-scoped `UndoStack` actions and batch resolution.

**Tech Stack:** C++23, `MK_scene`, `MK_editor_core`, `MK_ui` retained models, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the next narrow merge/rebase UX gap for prefab variants without adding automatic nested prefab propagation:

- Detect an override whose `source_node_name` is non-empty and differs from the current base node at `node_index`.
- Offer `Retarget override to node N` only when that source name uniquely identifies a different current base-prefab node and the target node/kind pair has no override.
- Treat the mismatch as a blocking review conflict in editor-core review, even though strict `MK_scene` composition remains index-based.
- Reuse existing single-row and `Apply All Reviewed` batch resolution paths.

## Context

- `Editor Prefab Variant Node Retarget Review v1` resolves missing-node stale overrides when a unique `source_node_name` points to a current base node.
- `Editor Prefab Variant Batch Resolution Review v1` applies currently reviewed cleanup/retarget rows after recomputing the model per row.
- A base prefab can reorder or replace nodes while keeping an override's old numeric index valid; without review, the override may compose onto the wrong node.

## Constraints

- Do not change `validate_prefab_variant_definition`, `compose_prefab_variant`, or strict serialized format semantics.
- Do not automatically retarget or remove source-mismatch rows.
- Do not infer fuzzy matches, hierarchy moves, nested prefab propagation, or broad merge/rebase workflows.
- Keep visible `MK_editor` as a thin adapter over existing conflict rows; per-row `Apply` and `Apply All Reviewed` should work without new GUI branching.
- Update Codex and Claude editor guidance together.

## Done When

- RED `MK_editor_core_tests` proves an existing-node source mismatch is not currently classified or retargetable.
- The review model reports `source_node_mismatch`, blocking status, retained UI ids, and a reviewed `Retarget override to node N` resolution.
- `resolve_prefab_variant_conflict` and `resolve_prefab_variant_conflicts` retarget the mismatch through the existing undoable paths.
- Docs, master plan, plan registry, manifest, skills, and static checks record the boundary truthfully.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor prefab variant source mismatch retargets existing stale node hints")`.
- [x] Build a raw variant with base node 1 named `Enemy`, base node 2 named `Camera`, and a transform override targeting node 1 with `source_node_name=Camera`.
- [x] Assert the new expected model behavior:
  - `status == blocked`,
  - `can_compose == false`,
  - row `node.1.transform` has conflict `source_node_mismatch`,
  - row resolution id is `retarget.node.1.transform.to.2`,
  - retained `prefab_variant_conflicts.rows.node.1.transform.resolution_kind` and `.resolution_target_node` ids exist.
- [x] Assert single-row resolution retargets to node 2, preserves `source_node_name=Camera`, and produces a valid variant.
- [x] Assert batch resolution applies the same retarget and reports one applied resolution id.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`; confirm the build fails before implementation because `source_node_mismatch` does not exist or the row is still clean.

### Task 2: Editor-Core Conflict Contract

**Files:**
- Modify: `editor/core/include/mirakana/editor/prefab_variant_authoring.hpp`
- Modify: `editor/core/src/prefab_variant_authoring.cpp`

- [x] Add `PrefabVariantConflictKind::source_node_mismatch`.
- [x] Return label `source_node_mismatch` from `prefab_variant_conflict_kind_label`.
- [x] Classify an override as source mismatch when:
  - the target node exists,
  - `source_node_name` is non-empty,
  - `source_node_name` differs from the current target node name.
- [x] Treat source mismatch as blocking in `status_for_conflict`.
- [x] Set `can_compose` false when blocking review conflicts exist, while leaving strict `MK_scene` composition unchanged.
- [x] Add diagnostic text: `override source node hint does not match the current target node`.
- [x] Reuse retarget metadata for `source_node_mismatch` when the source name uniquely maps to a different base node and no target node/kind override exists.
- [x] Do not offer safe remove metadata for source mismatch.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Docs, Manifest, Guidance, Static Checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

- [x] Record that source mismatch retarget is reviewed, explicit, undoable, and batch-compatible.
- [x] Keep strict composition index-based and keep nested propagation, fuzzy matching, automatic merge/rebase, package scripts, validation recipes, renderer/RHI uploads, native handles, and broad editor readiness unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Pass (expected failure) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation because `source_node_mismatch` was not declared. |
| Focused `MK_editor_core_tests` | Pass | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after the editor-core implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Static AI integration checks cover `source_node_mismatch`, source mismatch docs/manifest/skills, and the new focused plan. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | Unsupported gap status counts remain stable with editor productization still partly-ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Failed before formatting on `tests/unit/editor_core_tests.cpp`; passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Public API boundary sentinels accept the new editor-core conflict enum value. |
| `git diff --check` | Pass | No whitespace errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full repository validation passed, including `MK_editor_core_tests` in the dev CTest preset. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Dev preset build completed after validation. |
| Slice-closing commit | Pending |  |
