# Editor Scene Nested Prefab Propagation Plan Preview v1 (2026-05-11)

**Plan ID:** `editor-scene-nested-prefab-propagation-plan-preview-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)  

## Goal

Expose a **deterministic, preview-only** ordering of descendant linked prefab instance roots (nested prefab propagation candidates) on `ScenePrefabInstanceRefreshPlan`, with retained `MK_ui` contract rows for operators and agents—without chained refresh apply, automatic merge, or expanded ready claims.

## Context

- [2026-05-11-editor-nested-prefab-propagation-candidate-dry-run-v1.md](2026-05-11-editor-nested-prefab-propagation-candidate-dry-run-v1.md) adds aggregate counters only.
- Nested prefab stream exit still requires explicit operator-facing preview rows before propagation execution exists.

## Constraints

- Preview list order matches existing scene node iteration (same pass as former counter-only logic).
- Every new retained row id remains stable for `tools/check-ai-integration.ps1` needles.
- No undoable action or apply path for chained nested refresh in this slice.

## Done when

- `SceneNestedPrefabPropagationPreviewRow` + `nested_prefab_propagation_preview` populated whenever descendant linked prefab roots exist.
- Retained labels: `ge.editor.scene_nested_prefab_propagation_preview.v1`, `propagation_preview.operator_policy` = `preview_only_no_chained_refresh`, per-row `propagation_preview.rows.<order>.*`.
- `make_scene_prefab_instance_refresh_ui_model` and per-target `make_scene_prefab_instance_refresh_batch_ui_model` emit the preview block when non-empty.
- `MK_editor_core_tests` cover plan payload and serialized UI ids for nested prefab refresh scenarios.

## Validation evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
