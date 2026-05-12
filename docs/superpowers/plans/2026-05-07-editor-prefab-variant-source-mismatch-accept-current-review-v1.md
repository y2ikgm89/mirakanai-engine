# Editor Prefab Variant Source Mismatch Accept Current Review v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed accept-current-node path for existing-node prefab variant source mismatches when the old source-node name no longer maps uniquely to another base node.

**Architecture:** Keep strict `MK_scene` prefab variant validation and composition index-based. Extend only the GUI-independent `MK_editor_core` conflict resolution metadata so an author can explicitly rebase a stale `source_node_name` hint to the current indexed base node through the same undoable single-row and batch resolution paths.

**Tech Stack:** C++23, `MK_scene`, `MK_editor_core`, `MK_ui` retained models, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the next narrow prefab variant merge/rebase UX gap:

- Keep `source_node_mismatch` blocking review rows from the previous slice.
- When a mismatch row cannot safely retarget to a unique different source node, offer `Accept current node N`.
- Applying that reviewed resolution updates only `PrefabNodeOverride::source_node_name` to the current target base node name.
- Reuse existing `resolve_prefab_variant_conflict`, `resolve_prefab_variant_conflicts`, retained `prefab_variant_conflicts` ids, and variant-scoped `UndoStack` actions.

## Context

- `Editor Prefab Variant Source Mismatch Retarget Review v1` handles an existing-node override whose `source_node_name` points to a unique different current base node.
- A base prefab can also rename or delete the old source name while keeping the numeric node index correct. In that case retarget is unavailable, but the author still needs an explicit reviewed way to accept the current indexed node and refresh the hint.
- This is a rebase hint repair only; it does not change strict runtime composition semantics.

## Constraints

- Do not change `validate_prefab_variant_definition`, `compose_prefab_variant`, or serialized format semantics.
- Do not offer accept-current for missing-node rows.
- Prefer retarget when a unique different `source_node_name` match exists; accept-current is the fallback for existing-node source mismatches without a safe retarget.
- Do not infer fuzzy matches, hierarchy moves, nested prefab propagation, automatic merge/rebase, package scripts, validation recipes, renderer/RHI uploads, native handles, or broad editor readiness.
- Update Codex and Claude editor guidance together.

## Done When

- RED `MK_editor_core_tests` proves a source mismatch with no unique retarget has no accept-current resolution yet.
- The review model reports `source_node_mismatch`, blocking status, retained UI ids, and reviewed `Accept current node N` metadata.
- `resolve_prefab_variant_conflict` and `resolve_prefab_variant_conflicts` update only `source_node_name` through existing undoable paths.
- Docs, master plan, registry, manifest, skills, and static checks record the boundary truthfully.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor prefab variant source mismatch accepts current indexed node hints")`.
- [x] Build a raw variant with base node 1 named `Enemy`, base node 2 named `Camera`, and a transform override targeting node 1 with `source_node_name=OldEnemy`.
- [x] Assert the review model is blocked, row `node.1.transform` has conflict `source_node_mismatch`, and row resolution metadata is:
  - `resolution_id == "accept_current.node.1.transform"`,
  - `resolution_label == "Accept current node 1"`,
  - `resolution_kind == PrefabVariantConflictResolutionKind::accept_current_node`,
  - `resolution_target_node_index == 1`,
  - `resolution_target_node_name == "Enemy"`,
  - retained `prefab_variant_conflicts.rows.node.1.transform.resolution_kind` and `.resolution_target_node` ids exist.
- [x] Assert single-row resolution preserves `node_index == 1`, updates `source_node_name == "Enemy"`, and returns a valid variant.
- [x] Assert batch resolution applies the same accept-current row and reports one applied resolution id.
- [x] Assert the undoable action updates the hint and undo restores `OldEnemy`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`; confirm the build fails before implementation because `accept_current_node` does not exist.

### Task 2: Editor-Core Accept-Current Resolution

**Files:**
- Modify: `editor/core/include/mirakana/editor/prefab_variant_authoring.hpp`
- Modify: `editor/core/src/prefab_variant_authoring.cpp`

- [x] Add `PrefabVariantConflictResolutionKind::accept_current_node`.
- [x] Return `accept_current_node` from the retained `resolution_kind` label helper.
- [x] Add `accept_current_resolution_id_for(row_id)` that produces `accept_current.<row_id>`.
- [x] In `make_resolution_metadata`, keep the existing retarget metadata for safe unique source matches. If conflict is `source_node_mismatch` and retarget is unavailable, return accept-current metadata for the current target node.
- [x] Use label `Accept current node N` and diagnostic `update the source node hint to current node <Name>`.
- [x] In `resolve_prefab_variant_conflict`, handle `accept_current_node` by updating only `override.source_node_name` to the current base node name.
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

- [x] Record that accept-current is reviewed, explicit, undoable, batch-compatible, and updates only `source_node_name`.
- [x] Keep strict composition index-based and keep nested propagation, fuzzy matching, automatic merge/rebase, package scripts, validation recipes, renderer/RHI uploads, native handles, and broad editor readiness unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Pass (expected failure) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation because `accept_current_node` was not declared. |
| Focused `MK_editor_core_tests` | Pass | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after the editor-core implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Static AI integration checks accepted the updated docs, manifest, skills, plan registry, implementation symbols, and focused accept-current test coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | Unsupported production gaps remain explicit; `editor-productization` stays `partly-ready` with accept-current limited to reviewed hint repair. |
| Focused `MK_editor_core_tests` after format | Pass | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Formatting check passed after applying `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Public API boundary check passed for the enum extension and editor-core/docs updates. |
| `git diff --check` | Pass | Whitespace check passed; Git reported only expected local CRLF conversion warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full validation passed; Metal/Apple diagnostics remain host-gated as expected on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Default dev build completed after validation. |
| Slice-closing commit | Pass | Commit stages only accept-current implementation, focused plan, docs, manifest, skills, tests, and static checks; unrelated dirty files remain unstaged. |
