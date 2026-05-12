# Editor Scene Prefab Instance Refresh Multi-Target Batch v1 (2026-05-11)

**Plan ID:** `editor-scene-prefab-instance-refresh-multi-target-batch-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)  

## Goal

Add a **reviewed multi-target** prefab instance refresh path: operators select multiple linked prefab instance roots, inspect an aggregated batch plan, and apply all refreshes in **one** undoable snapshot while preserving fail-closed validation (no automatic merge, no chained nested propagation).

## Context

- Single-root `plan_scene_prefab_instance_refresh` / `make_scene_prefab_instance_refresh_action` remain; batch APIs compose them.
- Sequential apply on disjoint subtrees requires **node id remapping** across applies because `apply_scene_prefab_instance_refresh` rebuilds out-of-subtree nodes with new ids.
- Editor shell previously had only single-selection refresh; batch selection uses **reviewed checkboxes** on refresh-eligible prefab source-link rows.

## Constraints

- Reject batch plans when: empty target list, duplicate instance roots, or **strict ancestor/descendant** relationships among selected roots (fail-closed).
- Each per-target sub-plan keeps existing `ScenePrefabInstanceRefreshPlan` semantics and retained row id stability rules.
- No chained automatic nested prefab propagation beyond existing per-instance refresh behavior.
- New retained MK_ui root id `scene_prefab_instance_refresh_batch` and nested ids must be registered in `tools/check-ai-integration.ps1` when required by the contract checker.

## Done when

- `plan_scene_prefab_instance_refresh_batch`, `apply_scene_prefab_instance_refresh_batch`, `make_scene_prefab_instance_refresh_batch_action`, and `make_scene_prefab_instance_refresh_batch_ui_model` are implemented and covered by `MK_editor_core_tests`.
- `ScenePrefabInstanceRefreshResult` exposes `source_to_result_node_id` for successful applies to support deterministic batch chaining.
- `MK_editor` exposes batch review UI (checkbox selection + batch review/apply) without claiming broad editor productization.
- `engine/agent/manifest.json`, plan registry, and stream ledger reference this slice; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the development host.

## Validation evidence

| Check | Result |
| --- | --- |
| `MK_editor_core_tests` | Multi-target plan/apply + serialized batch UI contract |
| `tools/validate.ps1` | Run at slice close |
