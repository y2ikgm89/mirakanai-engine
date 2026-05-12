# Editor Scene Prefab Nested Variant Alignment v1 (2026-05-11)

**Plan ID:** `editor-scene-prefab-nested-variant-alignment-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)  

## Goal

Align **nested prefab instance refresh** review tooling with **Prefab Variant conflict** row semantics by exposing shared `PrefabVariantConflictResolutionKind` labels on nested refresh rows and a retained contract root for agents/tests—without implementing chained nested propagation, automatic merge, or silent refresh.

## Context

- `ScenePrefabInstanceRefreshPlan` already emits `keep_nested_prefab_instance` and `unsupported_nested_prefab_instance` rows with deterministic diagnostics.
- Prefab Variant conflict UI exposes `prefab_variant_conflicts.rows.*.resolution_kind` via `resolution_kind_label` in editor-core.

## Constraints

- No change to default refresh applicability rules; only additive retained MK_ui fields.
- Nested alignment labels appear only for nested prefab refresh row kinds (`keep_nested_prefab_instance`, `unsupported_nested_prefab_instance`).
- Mapping: reviewed keep → `accept_current_node`; blocked / needs operator policy or anchors → `none`.
- Stable element ids for `tools/check-ai-integration.ps1` needles.

## Done when

- `prefab_variant_conflict_resolution_kind_label` is a public editor-core API used by scene prefab refresh UI generation.
- `make_scene_prefab_instance_refresh_ui_model` and batch nested plans emit `scene_prefab_instance_refresh.nested_prefab_variant_alignment.*` and per-row `nested_variant_alignment.resolution_kind` for nested rows.
- `MK_editor_core_tests` asserts serialized ids on a nested fixture.
- `engine/agent/manifest.json` `editor-productization` notes mention nested variant alignment contract ids.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the author host or blockers are recorded.

## Validation evidence

| Check | Command / artifact | Result |
| --- | --- | --- |
| Unit tests | `ctest --preset dev --output-on-failure -R MK_editor_core_tests` | Record exit code |
| Repository validate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Record exit code |
