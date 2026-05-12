# Editor Prefab Variant Missing Node Cleanup v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an explicit reviewed cleanup path for prefab variant overrides that target nodes no longer present in the base prefab.

**Architecture:** Extend the existing prefab variant conflict review/resolution model instead of adding a new prefab merge system. `mirakana_scene` keeps strict valid-variant deserialization while adding `deserialize_prefab_variant_definition_for_review` for editor repair review; missing-node cleanup removes the stale override row through the same deterministic `resolve_prefab_variant_conflict` and variant-scoped `UndoStack` action path already used for redundant and duplicate overrides. Node remapping, nested prefab propagation, instance links, package work, and automatic merge/rebase remain out of scope.

**Tech Stack:** C++23, `mirakana_scene` `PrefabVariantDefinition`, `mirakana_editor_core` `PrefabVariantAuthoringDocument`, `mirakana_ui`, `mirakana_editor` Dear ImGui adapter, `mirakana_editor_core_tests`, existing AI/static validation checks.

---

## Goal

Implement `editor-prefab-variant-missing-node-cleanup-v1` as the next narrow editor productization slice:

- add `deserialize_prefab_variant_definition_for_review` so editor tooling can parse structurally valid stale variants without weakening strict `deserialize_prefab_variant_definition`;
- let `PrefabVariantAuthoringDocument` hold review-cleanup-repairable stale variants while continuing to reject malformed payloads, invalid names, invalid transforms, invalid component overrides, and invalid base prefabs;
- mark `missing_node` conflict rows as resolvable by removing the stale override;
- keep cleanup explicit and reviewed, never automatic;
- expose stable resolution labels and retained `prefab_variant_conflicts` `mirakana_ui` rows;
- allow visible `mirakana_editor` to apply the cleanup only through the existing reviewed `UndoStack` action path;
- update docs, manifest, skills, plan registry, master plan, and static checks so the ready claim says missing-node cleanup, not nested prefab repair or merge/rebase UX.

## Context

- Editor Prefab Variant Conflict Review v1 already reports missing-node rows as blocking.
- Editor Prefab Variant Reviewed Resolution v1 already provides reviewed cleanup metadata and actions for redundant overrides and later duplicate override rows.
- Stale missing-node overrides are a safe cleanup case because removing the override preserves the current base prefab structure instead of guessing a remap.
- The master plan still keeps nested prefab propagation, node remapping, merge/rebase automation, instance link tracking, and broad prefab conflict UX as follow-up work.

## Constraints

- Keep `editor/core` GUI-independent and free of Dear ImGui, SDL3, renderer/RHI, native handles, OS APIs, package scripts, validation execution, and arbitrary shell.
- Do not mutate files from the model.
- Do not apply cleanup automatically.
- Do not invent node remap heuristics, nested prefab propagation, base-prefab rebase, instance links, or automatic merge/rebase/resolution UX.
- Do not relax strict valid-variant validation, composition, or serialization. Raw stale variants can be parsed only through `deserialize_prefab_variant_definition_for_review`, reviewed through diagnostics, cleaned through `resolve_prefab_variant_conflict`, and saved only after the resulting variant is valid.

## Done When

- Unit tests prove a raw missing-node conflict row exposes a reviewed cleanup resolution and retained UI labels.
- Unit tests prove `resolve_prefab_variant_conflict` removes a stale missing-node override and reports the result valid when no other blocking conflicts remain.
- Unit tests prove `load_prefab_variant_authoring_document` can load a review-cleanup-repairable stale variant, the undoable action removes the missing-node row, save writes a valid variant, and undo restores the stale in-memory review state.
- Docs, manifest, skills, plan registry, master plan, and `tools/check-ai-integration.ps1` distinguish missing-node cleanup from nested prefab repair or automatic merge/rebase UX.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Files

- Modify: `editor/core/include/mirakana/editor/prefab_variant_authoring.hpp`
- Modify: `editor/core/src/prefab_variant_authoring.cpp`
- Modify: `engine/scene/include/mirakana/scene/prefab_overrides.hpp`
- Modify: `engine/scene/src/prefab_overrides.cpp`
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

### Task 1: RED Tests For Missing-Node Cleanup

- [x] Add `editor prefab variant missing node cleanup resolves stale raw variants` to `tests/unit/editor_core_tests.cpp`.
- [ ] Build a raw `PrefabVariantDefinition` with one base node and one override targeting a missing second node.
- [ ] Assert `make_prefab_variant_conflict_review_model(variant)` reports `blocked`, `can_compose == false`, and a `missing_node` row with `resolution_available == true`, `resolution_label == "Remove missing-node override"`, and `resolution_diagnostic == "remove the stale override because the target node is absent from the base prefab"`.
- [ ] Assert `make_prefab_variant_conflict_review_ui_model(model)` exposes `prefab_variant_conflicts.rows.node.2.name.resolution` and `prefab_variant_conflicts.rows.node.2.name.resolution_diagnostic`.
- [ ] Assert `resolve_prefab_variant_conflict(variant, row.resolution_id)` applies, removes the stale override, and reports `valid_after_apply == true`.
- [x] Assert an unavailable or empty resolution id leaves `applied == false` with a diagnostic.
- [x] Add `editor prefab variant authoring loads stale variants for reviewed missing node cleanup`.
- [x] Assert `load_prefab_variant_authoring_document` can load review-mode stale text, apply the cleanup through `make_prefab_variant_conflict_resolution_action`, save a valid variant, and undo back to stale in-memory diagnostics.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests`; confirm RED fails before implementation because missing-node rows are not resolvable and stale variant load is rejected.

### Task 2: Editor-Core Missing-Node Resolution

- [x] Add `deserialize_prefab_variant_definition_for_review` to parse structurally valid variant text without changing strict `deserialize_prefab_variant_definition`.
- [x] Let `PrefabVariantAuthoringDocument::from_variant` and `replace_variant` accept repairable invalid rows for missing-node and duplicate-override cleanup while rejecting invalid base/name/kind/name/transform/component payloads.
- [x] Update `has_safe_remove_resolution` to include `PrefabVariantConflictKind::missing_node`.
- [x] Add the missing-node label in `resolution_label_for`: `Remove missing-node override`.
- [x] Add the missing-node diagnostic in `resolution_diagnostic_for`: `remove the stale override because the target node is absent from the base prefab`.
- [x] Keep `component_family_replacement`, `invalid_override`, and `clean` rows non-resolvable.
- [x] Keep `resolve_prefab_variant_conflict` behavior index-based and deterministic; no node remap or nested prefab inference.
- [x] Run focused build/test until `mirakana_editor_core_tests` passes.

### Task 3: Visible Editor And Static Contract

- [x] Confirm existing visible `mirakana_editor` conflict table renders the reviewed `Apply` button for any `resolution_available` row without adding missing-node-specific GUI branching.
- [x] Update `tools/check-ai-integration.ps1` to require missing-node cleanup tests, labels, diagnostics, retained UI ids, docs, manifest, and skill guidance.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Skills, Registry

- [x] Document Editor Prefab Variant Missing Node Cleanup v1 as explicit cleanup for stale overrides whose target node is absent.
- [x] Keep non-ready claims explicit: no node remapping, nested prefab propagation, instance links, automatic merge/rebase/resolution UX, package scripts, validation recipes, dynamic runtime-host Play-In-Editor, renderer/RHI uploads, native handles, package streaming, or broad editor productization.
- [x] Update `engine/agent/manifest.json` current editor guidance and `unsupportedProductionGaps.editor-productization.notes`.
- [x] Update Codex and Claude editor skills with equivalent behavior.
- [x] Update the master plan and plan registry.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS (expected fail) | `cmake --build --preset dev --target mirakana_editor_core_tests` built the RED tests, then `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` failed on `missing->resolution_available` and stale variant load throwing `prefab variant definition is invalid`. |
| Focused `mirakana_editor_core_tests` | PASS | `cmake --build --preset dev --target mirakana_editor_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed after adding review-mode parsing and missing-node cleanup. |
| Focused `mirakana_core_tests` | PASS | `cmake --build --preset dev --target mirakana_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_core_tests` passed after adding the public review-mode parser. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial check found clang-format issues in `tests/unit/editor_core_tests.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting and rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check accepted `deserialize_prefab_variant_definition_for_review` and the editor-core repairable authoring changes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok` after static checks were updated for review-mode parsing, missing-node resolution labels, docs, manifest, and skills. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; unsupported gap count remains 11 and editor-productization remains partly-ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Desktop GUI lane built and ran 46/46 tests after stale variant loading and missing-node cleanup. |
| `git diff --check` | PASS | No whitespace errors; Git reported line-ending conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Repository validation completed with `validate: ok`; CTest reported 29/29 tests passed, host-gated Apple/Metal diagnostics remained non-blocking, and production-readiness unsupported gap count remained 11. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev preset configure and MSBuild build completed successfully after the missing-node cleanup implementation and docs/static updates. |
| Slice-closing commit | PASS | Slice changes are ready for a focused commit after final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`, with unrelated dirty files left unstaged. |
