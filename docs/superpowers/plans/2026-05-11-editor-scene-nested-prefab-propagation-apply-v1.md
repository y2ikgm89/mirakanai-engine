# Editor Scene Nested Prefab Propagation Apply v1 (2026-05-11)

**Plan ID:** `editor-scene-nested-prefab-propagation-apply-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)  
**Status:** Implementation plan  

## Goal

Add an **opt-in, reviewed** path to apply `plan_scene_prefab_instance_refresh` to **descendant linked prefab instance roots** listed in `nested_prefab_propagation_preview`, **after** a successful root refresh, in deterministic `preview_order`, with **one undo** for the combined operation when using the batch action entrypoint.

## Context

- [2026-05-11-editor-scene-nested-prefab-propagation-plan-preview-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-plan-preview-v1.md) surfaces preview rows and `preview_only_no_chained_refresh` when propagation apply is off.
- Root refresh already builds `SceneNestedPrefabPropagationPreviewRow` via `collect_nested_prefab_propagation_preview`.
- `apply_scene_prefab_instance_refresh_batch` remaps later targets through `source_to_result_node_id`; nested propagation reuses the same id-remapping idea for preview roots across chained applies.

## Constraints

- **Fail-closed:** If any nested prefab cannot be loaded, any nested plan is blocked, or id remapping loses a preview root, the **whole** operation is blocked at plan time (and apply must not partially mutate the live document).
- **No automatic recursion:** Inner nested applies use the same `ScenePrefabInstanceRefreshPolicy` with `apply_reviewed_nested_prefab_propagation = false` so only the operator-selected root run chains one level from the captured preview list.
- **Multi-target batch:** Nested propagation apply is **not** supported together with multiple batch targets in one reviewed apply (plan blocks with an explicit diagnostic).
- **Undo:** `make_scene_prefab_instance_refresh_batch_action` continues to snapshot once for the final scene state.
- **Stable MK_ui ids:** New retained rows for the opt-in policy; extend `tools/check-ai-integration.ps1` needles when literals change.

## Done when

- `ScenePrefabInstanceRefreshPolicy` carries `apply_reviewed_nested_prefab_propagation` plus a prefab loader callback used for plan simulation and apply.
- `plan_scene_prefab_instance_refresh` / `plan_scene_prefab_instance_refresh_batch` gate `can_apply` using the same simulation as apply.
- `MK_editor` exposes a checkbox wired into the reviewed policy; propagation preview UI shows `reviewed_chained_prefab_refresh_after_root` when enabled.
- `MK_editor_core_tests` cover a minimal nested load + chained apply (weapon receives a new prefab node after player root refresh).

## Validation evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
- Focused: `ctest --preset dev --output-on-failure -R editor_core` (or project equivalent for `MK_editor_core_tests`)
- Follow-up test slice: [2026-05-11-editor-scene-nested-prefab-propagation-undo-redo-test-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-undo-redo-test-v1.md) (`UndoStack::undo` / `redo` after `make_scene_prefab_instance_refresh_action` with nested propagation).
