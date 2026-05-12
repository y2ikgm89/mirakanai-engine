# Editor Prefab Variant Node Retarget Review v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement `editor-prefab-variant-node-retarget-review-v1`: add a reviewed, undoable prefab-variant missing-node retarget path when a stale override carries a stable source-node name that uniquely resolves to a current base-prefab node.

**Architecture:** Extend the first-party `PrefabNodeOverride::source_node_name` contract with an optional hint emitted by editor authoring actions and preserved as `override.N.source_node_name` in `GameEngine.PrefabVariant.v1` text. Keep composition and strict validation index-based; the editor conflict-review model may offer a single explicit `Retarget override to node N` resolution only when the hint is unique and the retarget cannot create a duplicate override.

**Tech Stack:** C++23, `mirakana_scene` prefab variant IO/validation, `mirakana_editor_core` `PrefabVariantAuthoringDocument`, retained `mirakana_ui` rows, `mirakana_editor_core_tests`, `mirakana_core_tests`, existing AI/static validation checks.

---

## Goal

Close one narrow editor-productization gap for source-node retarget without broad automatic merge/rebase/resolution UX: an operator can load a stale prefab variant, review a missing-node override that records its original source node name, apply an explicit retarget to the uniquely matching current base node, undo it, and save only after the variant is valid.

## Context

- `editor-prefab-variant-missing-node-cleanup-v1` added review-mode loading plus `Remove missing-node override`.
- Current override identity is still node-index plus override-kind; there is no stable node hint in `PrefabNodeOverride`.
- `PrefabDefinition::nodes` are 1-based in override references and each node already has a valid name.
- `mirakana_editor` already renders a generic `Apply` button for any `PrefabVariantConflictRow` with `resolution_available`.

## Constraints

- Keep strict `deserialize_prefab_variant_definition` and `compose_prefab_variant` deterministic and validation-backed.
- Do not automatically retarget; the model only exposes reviewed resolution metadata and `resolve_prefab_variant_conflict` mutates on explicit resolution id.
- Retarget only when `source_node_name` is non-empty, valid, exactly matches one current base-prefab node, and the target node/kind pair does not already have an override.
- If the source-node hint is absent, ambiguous, missing, or would create a duplicate override, keep the existing safe remove resolution.
- Do not add nested prefab propagation, automatic merge/rebase UX, runtime execution, package scripts, validation recipes, renderer/RHI uploads, native handles, package streaming, or broad editor productization claims.
- Update Codex and Claude editor skills equivalently if behavior guidance changes.

## Done When

- `PrefabNodeOverride` can serialize/deserialize an optional `source_node_name` hint.
- Editor-created name/transform/component overrides populate the hint from the current base prefab node name.
- Missing-node conflict rows expose retarget resolution metadata for unique safe hints and retained UI ids for resolution kind/target.
- `resolve_prefab_variant_conflict` and `make_prefab_variant_conflict_resolution_action` support undoable retarget and still support cleanup.
- Focused tests cover RED/GREEN for serialization, safe retarget, undo, and ambiguous fallback.
- Docs, master plan, plan registry, skills, manifest, and static checks describe the narrow claim.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Tests

- [x] Add `mirakana_core_tests` coverage for optional `override.N.source_node_name` serialization/deserialization.
- [x] Add `mirakana_editor_core_tests` coverage for reviewed missing-node retarget from a unique `source_node_name`.
- [x] Add `mirakana_editor_core_tests` coverage that ambiguous source-node names do not retarget and still expose cleanup.
- [x] Run `cmake --build --preset dev --target mirakana_core_tests mirakana_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R "mirakana_core_tests|mirakana_editor_core_tests"` and record the expected RED failures.

### Task 2: Scene Contract

- [x] Add optional `source_node_name` to `PrefabNodeOverride` without breaking existing aggregate initializer order for `name`, `transform`, and `components`.
- [x] Validate non-empty source-node hints with the same text safety rules as prefab node names.
- [x] Parse and serialize `override.N.source_node_name` as an optional per-override field.
- [x] Keep composition index-based and unchanged.
- [x] Run focused `mirakana_core_tests` until green.

### Task 3: Editor Retarget Resolution

- [x] Add explicit conflict resolution kind metadata for `none`, `remove_override`, and `retarget_override`.
- [x] Populate editor-authored override hints in name, transform, and component override actions.
- [x] Offer retarget metadata only for unique safe `source_node_name` matches.
- [x] Apply retarget by changing only `node_index` and refreshing `source_node_name` to the target base node name.
- [x] Preserve remove cleanup for stale rows without a safe retarget.
- [x] Expose retained UI labels for resolution kind and target node.
- [x] Run focused `mirakana_editor_core_tests` until green.

### Task 4: Docs, Manifest, And Static Checks

- [x] Update `docs/editor.md`, `docs/current-capabilities.md`, `docs/ai-game-development.md`, `docs/testing.md`, and `docs/architecture.md`.
- [x] Update the master plan and plan registry.
- [x] Update `engine/agent/manifest.json`.
- [x] Update `.agents/skills/editor-change/SKILL.md` and `.claude/skills/gameengine-editor/SKILL.md`.
- [x] Update `tools/check-ai-integration.ps1` for the new contract/test names.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Pass/expected fail | `cmake --build --preset dev --target mirakana_core_tests mirakana_editor_core_tests` passed; `ctest --preset dev --output-on-failure -R "mirakana_core_tests\|mirakana_editor_core_tests"` failed on the new source-node hint tests before implementation because `override.N.source_node_name` was unknown. |
| Focused `mirakana_core_tests` | Pass | `ctest --preset dev --output-on-failure -R "mirakana_core_tests\|mirakana_editor_core_tests"` passed after implementation; includes source-node hint round-trip. |
| Focused `mirakana_editor_core_tests` | Pass | `ctest --preset dev --output-on-failure -R "mirakana_core_tests\|mirakana_editor_core_tests"` passed after implementation; includes unique retarget, undo, and ambiguous fallback. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Failed once on C++ line wrapping; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied clang-format; rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Public API boundary check accepted the `PrefabNodeOverride` and editor conflict-row surface changes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Static AI integration checks cover the new source-node hint, retarget metadata, docs, manifest, and skill guidance. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | Audit kept `unsupported_gaps=11` and `editor-productization` as `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Pass | Desktop GUI preset build succeeded and ran 46/46 tests. |
| `git diff --check` | Pass | No whitespace errors; Git reported line-ending warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full repository validation passed; Windows host gates for Metal/Apple lanes remained diagnostic/host-gated as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Dev preset build completed successfully after validation. |
| Slice-closing commit | Pass | Commit created after final validation/build, staging only the node-retarget slice files. |
