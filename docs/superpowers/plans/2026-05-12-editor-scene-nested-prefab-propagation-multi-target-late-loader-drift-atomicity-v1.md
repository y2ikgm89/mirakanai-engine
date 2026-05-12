# Editor Scene Nested Prefab Propagation Multi Target Late Loader Drift Atomicity v1 (2026-05-12)

**Plan ID:** `editor-scene-nested-prefab-propagation-multi-target-late-loader-drift-atomicity-v1`  
**Status:** Completed  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `editor-productization`

## Goal

Prove that reviewed multi-target nested prefab propagation remains atomic when a later batch target's nested prefab loader drifts after earlier targets have already succeeded on the internal working scene clone.

## Context

- Single-target loader drift is covered by `editor-scene-nested-prefab-propagation-loader-drift-atomicity-v1`.
- Multi-target propagation currently applies ordered disjoint targets against a working document clone before creating an undoable snapshot action.
- A stateful loader can let batch planning, the first target apply, and the later target validation pass before returning `std::nullopt` during the later target nested apply.

## Constraints

- Keep this slice test-only unless the regression exposes an implementation failure.
- Do not apply partial refreshed roots to the live scene when a later batch target fails.
- Keep mutation behind `UndoableAction`; do not add automatic recovery, fuzzy matching, or merge/rebase UX.

## Done When

- `MK_editor_core_tests` covers a two-target disjoint batch where the nested loader fails only during the second target's nested apply.
- `make_scene_prefab_instance_refresh_batch_action` returns an empty action / `UndoStack::execute` fails.
- The live scene node count, source-node counts, parent links, selected node, undo count, and redo count remain unchanged.
- Validation evidence is recorded for focused tests and the repository slice gate.

## Validation Evidence

- Pass: `cmake --build --preset dev --target MK_editor_core_tests`.
- Pass: `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.
- Pass: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- Pass: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- Pass: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- Pass: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Next Candidate After Validation

- Return to `recommendedNextPlan.id=next-production-gap-selection` and pick the next `editor-productization` wedge from the manifest-selected order.
