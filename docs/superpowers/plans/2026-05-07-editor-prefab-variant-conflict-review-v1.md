# Editor Prefab Variant Conflict Review v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a GUI-independent, read-only prefab variant conflict review model so editor/AI tooling can surface unsafe or surprising prefab override rows before save, instantiation, or future nested propagation work.

**Architecture:** Keep conflict detection in `mirakana_editor_core` over existing `mirakana_scene` prefab variant value types. Expose deterministic rows and retained `mirakana_ui` output, then let the visible Dear ImGui editor render those rows as an adapter without mutating variants, running package scripts, or implementing nested prefab propagation.

**Tech Stack:** C++23, `mirakana_scene` `PrefabVariantDefinition`, `mirakana_editor_core` `PrefabVariantAuthoringDocument`, `mirakana_ui`, `mirakana_editor` Dear ImGui adapter, existing static AI integration checks.

---

## Goal

Implement `nested-prefab-conflict-ux-v1` as a narrow conflict review surface:

- summarize whether a prefab variant can compose;
- list each override with stable ids, node index, override kind, base value, override value, status, conflict kind, and diagnostic;
- report blocking validation problems such as missing nodes or duplicate overrides;
- report non-blocking review warnings such as redundant overrides and component-family replacement;
- expose retained `mirakana_ui` ids under `prefab_variant_conflicts`;
- show the conflict review rows in the visible `mirakana_editor` Prefab Variant Authoring section.

## Context

- `mirakana_scene` already owns dependency-free `PrefabVariantDefinition`, validation, deterministic text IO, and composition.
- `mirakana_editor_core` already owns `PrefabVariantAuthoringDocument`, override rows, registry-backed diagnostics, save/load helpers, dirty state, and undo actions.
- The master plan still lists nested prefab propagation/merge resolution UX as an editor-productization follow-up.
- This slice handles conflict review only; it does not implement nested instance links, automatic merge/rebase/resolution UX, or propagation.

## Constraints

- Keep `editor/core` independent from Dear ImGui, SDL3, renderer/RHI, OS APIs, package scripts, validation execution, and arbitrary shell.
- Do not mutate prefab variants, prefab files, runtime manifests, package rows, scenes, or undo history while building conflict review models.
- Do not claim nested prefab propagation, merge/resolution UX, runtime package migration, dynamic game-module/runtime-host execution, renderer/RHI uploads, native handles, package streaming, or broad editor productization.
- Preserve existing v1 prefab variant behavior and deterministic ordering.

## Done When

- `mirakana_editor_core` exposes `PrefabVariantConflictStatus`, `PrefabVariantConflictKind`, `PrefabVariantConflictRow`, `PrefabVariantConflictReviewModel`, `make_prefab_variant_conflict_review_model`, and `make_prefab_variant_conflict_review_ui_model`.
- Unit tests prove clean rows, blocking missing-node/duplicate diagnostics, non-blocking redundant/component replacement warnings, and retained `mirakana_ui` ids.
- `mirakana_editor` renders conflict review rows in the Prefab Variant Authoring section without mutation or execution.
- Docs, manifest, skills, plan registry, master plan, and `tools/check-ai-integration.ps1` describe the ready surface and non-goals.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

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

### Task 1: RED Tests For Conflict Review Rows

- [x] Add `editor prefab variant conflict review reports blocking and warning rows` to `tests/unit/editor_core_tests.cpp`.
- [x] Assert a clean document returns `status_label == "ready"`, `can_compose == true`, `mutates == false`, `executes == false`, and retained ids under `prefab_variant_conflicts`.
- [x] Assert a raw invalid variant with a missing node and duplicate override paths returns blocking conflict rows and `can_compose == false`.
- [x] Assert redundant name overrides and component-family replacement return non-blocking warning rows.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and record the expected missing symbol failure.

### Task 2: Editor-Core Conflict Model

- [x] Add conflict status/kind enums, row/model structs, and label helpers to `prefab_variant_authoring.hpp`.
- [x] Implement deterministic formatting for name, transform, and component override/base values.
- [x] Implement `make_prefab_variant_conflict_review_model(const PrefabVariantDefinition&)` by combining `validate_prefab_variant_definition`, duplicate detection, override row review, and `compose_prefab_variant`.
- [x] Implement `make_prefab_variant_conflict_review_model(const PrefabVariantAuthoringDocument&)` as a thin overload.
- [x] Implement `make_prefab_variant_conflict_review_ui_model` with stable ids under `prefab_variant_conflicts`.
- [x] Run focused `mirakana_editor_core_tests`.

### Task 3: Visible Editor Wiring

- [x] Include the conflict model in the existing Prefab Variant Authoring section.
- [x] Render status, row counts, conflict/warning table, and diagnostics without calling save/load/instantiate or mutating the variant.
- [x] Keep `composed_prefab()` calls guarded by the conflict model so invalid variants cannot throw during rendering.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Static Checks

- [x] Document Editor Prefab Variant Conflict Review v1 as a read-only conflict review surface.
- [x] Keep non-ready claims explicit: no nested propagation, automatic merge/rebase/resolution UX, mutation, execution, package scripts, validation recipes, runtime host, renderer/RHI uploads, native handles, package streaming, or broad editor productization.
- [x] Add static checks for the new symbols, retained UI ids, visible editor wiring, docs, manifest, and Codex/Claude skill guidance.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Pass | `cmake --build --preset dev --target mirakana_editor_core_tests` failed as expected before implementation on missing `PrefabVariantConflictRow`, `PrefabVariantConflictReviewModel`, `PrefabVariantConflictStatus`, `PrefabVariantConflictKind`, `make_prefab_variant_conflict_review_model`, and `make_prefab_variant_conflict_review_ui_model`. |
| Focused `mirakana_editor_core_tests` | Pass | `cmake --build --preset dev --target mirakana_editor_core_tests` and `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed after adding the model and fixing component value formatting so same-family mesh asset changes are not misclassified as redundant. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Repository formatting checks passed after applying `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Public API boundary check accepted the editor-core prefab conflict review API additions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Static AI integration checks now require the conflict review symbols, retained `prefab_variant_conflicts` ids, visible editor wiring, manifest/docs coverage, and Codex/Claude skill guidance. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `editor-productization` remains `partly-ready`; nested prefab propagation/merge resolution UX remains unsupported while read-only conflict review is documented as complete. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Pass | `desktop-gui` build and 46/46 GUI tests passed. |
| `git diff --check` | Pass | No whitespace errors were reported; existing CRLF warnings were diagnostic only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | `validate: ok`; host-specific Metal and Apple checks remained diagnostic-only blockers on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Default repository build completed after validation. |
| Slice-closing commit | Pass | This commit closes the read-only prefab variant conflict review slice after validation/build gates. |
