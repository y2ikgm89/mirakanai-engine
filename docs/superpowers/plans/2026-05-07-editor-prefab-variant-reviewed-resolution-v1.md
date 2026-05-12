# Editor Prefab Variant Reviewed Resolution v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an explicit reviewed cleanup path for safe prefab variant conflict rows, starting with redundant overrides and later duplicate override rows.

**Architecture:** Extend the existing `mirakana_editor_core` prefab variant conflict review rows with deterministic resolution metadata and a narrow undoable apply action. Keep mutation behind an explicit `UndoStack` action in the optional editor adapter; `editor/core` still performs no process execution, package work, manifest edits, renderer/RHI work, or nested prefab propagation.

**Tech Stack:** C++23, `mirakana_scene` `PrefabVariantDefinition`, `mirakana_editor_core` `PrefabVariantAuthoringDocument`, `mirakana_ui`, `mirakana_editor` Dear ImGui adapter, `mirakana_editor_core_tests`, existing AI/static validation checks.

---

## Goal

Implement `editor-prefab-variant-reviewed-resolution-v1` as a narrow follow-up to conflict review:

- mark redundant override rows as resolvable by removing the override;
- mark later duplicate override rows as resolvable by removing the duplicate row while preserving the first reviewed override for that node/kind pair;
- keep missing-node, invalid override, component-family replacement, and clean rows non-resolvable;
- expose stable resolution ids and retained `prefab_variant_conflicts` `mirakana_ui` labels;
- add a GUI-independent `resolve_prefab_variant_conflict` helper plus an undoable `make_prefab_variant_conflict_resolution_action`;
- show explicit `Apply` controls in visible `mirakana_editor` only when the reviewed row has a safe resolution.

## Context

- Editor Prefab Variant Conflict Review v1 already reports blocking duplicate/missing-node rows and non-blocking redundant/component-family warnings.
- `PrefabVariantAuthoringDocument` currently keeps loaded documents valid; the visible editor therefore mostly exercises redundant cleanup today, while raw `PrefabVariantDefinition` resolution can support future stale/merge import lanes.
- The master plan still lists nested prefab propagation/merge resolution UX as a production gap. This slice only closes the safe reviewed cleanup subset and keeps automatic/broad merge UX unsupported.

## Constraints

- Keep `editor/core` GUI-independent and free of Dear ImGui, SDL3, renderer/RHI, native handles, OS APIs, package scripts, validation execution, and arbitrary shell.
- Do not persist resolution acknowledgement state or mutate files from the model.
- Do not automatically apply resolutions; visible editor users must click a reviewed row action.
- Do not resolve missing-node rows, component-family replacement semantics, nested prefab propagation, instance links, base-prefab rebase, or broad merge/rebase workflows.
- Keep prefab variant text serialization deterministic and do not relax public `mirakana_scene` validation in this slice.

## Done When

- Unit tests prove redundant cleanup removes the override through an undoable action and undo restores it.
- Unit tests prove later duplicate raw rows expose a safe resolution id and retained UI labels without resolving missing-node rows.
- `mirakana_editor` renders a reviewed `Apply` button only for rows with `resolution_available`.
- Docs, manifest, skills, plan registry, master plan, and `tools/check-ai-integration.ps1` distinguish reviewed cleanup from unsupported automatic/nested resolution UX.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Files

- Modify: `editor/core/include/mirakana/editor/prefab_variant_authoring.hpp`
- Modify: `editor/core/src/prefab_variant_authoring.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

---

### Task 1: RED Tests For Reviewed Resolution

- [x] Add `editor prefab variant reviewed resolution removes redundant overrides with undo` to `tests/unit/editor_core_tests.cpp`.
- [x] Build a valid variant where a name override equals the base node name.
- [x] Assert `make_prefab_variant_conflict_review_model(document)` reports one warning row with `resolution_available == true`, a non-empty `resolution_id`, and `resolution_label == "Remove redundant override"`.
- [x] Assert `make_prefab_variant_conflict_review_ui_model(model)` exposes `prefab_variant_conflicts.rows.node.1.name.resolution`.
- [x] Execute `make_prefab_variant_conflict_resolution_action(document, row.resolution_id)` through `UndoStack`, assert the redundant override is removed, the new review is ready, and `undo()` restores the override.
- [x] Add `editor prefab variant reviewed resolution exposes duplicate cleanup and leaves missing nodes blocked`.
- [x] Build a raw invalid variant with a duplicate name override and a missing-node name override.
- [x] Assert the later duplicate row has `resolution_available == true`, a stable duplicate-specific id, and `resolution_label == "Remove duplicate override"`.
- [x] Assert the missing-node row has `resolution_available == false`.
- [x] Assert `resolve_prefab_variant_conflict(raw_variant, duplicate_row.resolution_id)` applies the duplicate cleanup while reporting that remaining missing-node diagnostics still keep the result invalid.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and confirm it fails on missing resolution fields/functions.

### Task 2: Editor-Core Resolution Model And Action

- [x] Add resolution fields to `PrefabVariantConflictRow`: `override_index`, `resolution_available`, `resolution_id`, `resolution_label`, and `resolution_diagnostic`.
- [x] Add `PrefabVariantConflictResolutionResult` and declarations for `resolve_prefab_variant_conflict` and `make_prefab_variant_conflict_resolution_action`.
- [x] Make duplicate row ids unique for later duplicate occurrences while preserving the existing `node.<index>.<kind>` ids for first/clean rows.
- [x] Populate resolution metadata for redundant overrides and later duplicate overrides only.
- [x] Implement `resolve_prefab_variant_conflict` by rebuilding the variant without the reviewed override index and reporting whether the result is valid after apply.
- [x] Implement `make_prefab_variant_conflict_resolution_action` as a snapshot action that is available only when the resolved document variant remains valid.
- [x] Extend `make_prefab_variant_conflict_review_ui_model` with retained `.resolution` and `.resolution_diagnostic` labels per row.
- [x] Run focused build/test until `mirakana_editor_core_tests` passes.

### Task 3: Visible Editor Wiring

- [x] Add a `Resolution` column to the Prefab Variant Conflict Rows table.
- [x] Render an `Apply` button with an id derived from `row.resolution_id` only when `row.resolution_available`.
- [x] Route the button through `execute_prefab_variant_authoring_action(make_prefab_variant_conflict_resolution_action(document, row.resolution_id))`.
- [x] Keep component-family replacement rows as review warnings without an apply control.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Skills, Static Checks

- [x] Document Editor Prefab Variant Reviewed Resolution v1 as explicit cleanup for redundant and duplicate override rows.
- [x] Keep non-ready claims explicit: no missing-node resolution, nested propagation, automatic merge/rebase/resolution UX, package scripts, validation recipes, dynamic runtime-host Play-In-Editor, renderer/RHI uploads, native handles, package streaming, or broad editor productization.
- [x] Update static checks for the new tests, APIs, retained ids, visible editor apply control, docs, manifest, and Codex/Claude skill guidance.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS (expected fail) | `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation on missing `resolution_available`, `resolution_id`, `resolution_label`, `resolve_prefab_variant_conflict`, and `make_prefab_variant_conflict_resolution_action`. |
| Focused `mirakana_editor_core_tests` | PASS | `cmake --build --preset dev --target mirakana_editor_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial check found one clang-format issue in `prefab_variant_authoring.hpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting and rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check accepted the editor-core reviewed resolution additions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Static AI integration checks now require the reviewed resolution symbols, retained resolution ids, visible editor apply control, docs, manifest, and Codex/Claude skill guidance. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; unsupported gap count remains 11 and editor-productization remains partly-ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Desktop GUI lane built and ran 46/46 tests after visible `Apply` wiring. |
| `git diff --check` | PASS | No whitespace errors; Git reported line-ending conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; Metal/Apple lanes remained diagnostic/host-gated on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default dev build completed after validation. |
| Slice-closing commit | Pending |  |
