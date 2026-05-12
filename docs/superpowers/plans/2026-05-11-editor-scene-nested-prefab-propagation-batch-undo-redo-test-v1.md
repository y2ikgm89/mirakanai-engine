# Editor Scene Nested Prefab Propagation Batch Undo Redo Test v1 (2026-05-11)

**Plan ID:** `editor-scene-nested-prefab-propagation-batch-undo-redo-test-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)  
**Status:** Completed (test evidence only; does not narrow manifest `requiredBeforeReadyClaim` rows)

## Goal

Prove that **reviewed nested prefab propagation** applied through `make_scene_prefab_instance_refresh_batch_action` with **exactly one** batch target participates in the editor **undo/redo** stack the same way as `make_scene_prefab_instance_refresh_action`, matching the single-root propagation chain test.

## Context

- [2026-05-11-editor-scene-nested-prefab-propagation-undo-redo-test-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-undo-redo-test-v1.md) covers the non-batch entrypoint only.
- `plan_scene_prefab_instance_refresh_batch` blocks `apply_reviewed_nested_prefab_propagation` when `ordered_targets.size() > 1`; single-target batch is the supported pairing.

## Constraints

- No new MK_ui row ids or `check-ai-integration` needles unless new literals appear on tracked surfaces (this slice does not add editor shell needles).
- Does not claim multi-target batch propagation, fuzzy merge/rebase propagation UX, or manifest gap promotion.

## Done when

- `tests/unit/editor_core_tests.cpp` contains a test that builds the same Shield + nested weapon + Grip scenario as the single-root propagation undo test, executes `make_scene_prefab_instance_refresh_batch_action` with one `ScenePrefabInstanceRefreshBatchTargetInput`, and asserts weapon Grip + player Shield presence across **execute → undo → redo**.

## Validation evidence

| Command | Result |
| --- | --- |
| `cmake --build --preset dev --target MK_editor_core_tests` | Pass |
| `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass |
