# Editor Scene Nested Prefab Propagation Undo Redo Test v1 (2026-05-11)

**Plan ID:** `editor-scene-nested-prefab-propagation-undo-redo-test-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)  
**Status:** Completed (test evidence only; does not narrow manifest `requiredBeforeReadyClaim` rows)

## Goal

Prove that **reviewed nested prefab propagation** applied through `make_scene_prefab_instance_refresh_action` participates in the editor **undo/redo** stack the same way as other scene authoring snapshot actions: one undo restores the pre-refresh scene (including nested weapon subtree and player root refresh additions), and one redo reapplies the combined result.

## Context

- [2026-05-11-editor-scene-nested-prefab-propagation-apply-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-apply-v1.md) implements chained apply and documents single-snapshot undo for the batch entrypoint; single-root refresh uses `make_scene_prefab_instance_refresh_action` in `scene_authoring.cpp`.
- Prior `MK_editor_core_tests` coverage asserted propagation outcomes but did not exercise `UndoStack::undo` / `redo` after propagation.
- Companion (single-target batch entrypoint): [2026-05-11-editor-scene-nested-prefab-propagation-batch-undo-redo-test-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-batch-undo-redo-test-v1.md).

## Constraints

- No new MK_ui row ids or `check-ai-integration` needles unless the test introduces new literals in tracked surfaces (this slice does not).
- Does not claim fuzzy merge/rebase propagation UX, multi-target batch propagation, or manifest gap promotion.

## Done when

- `tests/unit/editor_core_tests.cpp` test `editor scene prefab instance refresh review can apply nested prefab propagation chain` uses `history.execute(make_scene_prefab_instance_refresh_action(...))` and asserts presence/absence of **Shield** (player prefab) and **Grip** (nested weapon prefab) across **execute → undo → redo**.

## Validation evidence

| Command | Result |
| --- | --- |
| `cmake --build --preset dev --target MK_editor_core_tests` | Pass |
| `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass |
