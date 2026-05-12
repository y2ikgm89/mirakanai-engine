# Editor Scene Prefab Local And Stale Variant Alignment v1 (2026-05-11)

**Plan ID:** `editor-scene-prefab-local-stale-variant-alignment-v1`  
**Date:** 2026-05-11  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)  

## Goal

Extend **scene prefab instance refresh** review tooling so **local child** and **stale source subtree** row kinds expose the same `PrefabVariantConflictResolutionKind` labels as Prefab Variant conflict rows and nested prefab refresh rows, via retained MK_ui contract roots and per-row fields—without changing default refresh applicability or implementing chained propagation.

## Context

- [2026-05-11-editor-scene-prefab-nested-variant-alignment-v1.md](2026-05-11-editor-scene-prefab-nested-variant-alignment-v1.md) defines nested-only `nested_prefab_variant_alignment` and per-row `nested_variant_alignment.resolution_kind`.
- Scene refresh already emits `keep_local_child`, `unsupported_local_child`, `keep_stale_source_node_as_local`, and `unsupported_stale_source_subtree` rows.

## Constraints

- Additive retained ids only for local/stale alignment; keep existing nested ids and `ge.editor.scene_prefab_nested_variant_alignment.v1` unchanged.
- Mapping: reviewed keep paths (`keep_local_child`, `keep_stale_source_node_as_local`, `keep_nested_prefab_instance`) → `accept_current_node`; blocked / unsupported rows → `none`.
- Stable element ids for `tools/check-ai-integration.ps1` needles.

## Done when

- `make_scene_prefab_instance_refresh_ui_model` and batch UI emit optional roots `scene_prefab_instance_refresh.local_child_variant_alignment.*` / `stale_source_variant_alignment.*` with contracts `ge.editor.scene_prefab_local_child_variant_alignment.v1` and `ge.editor.scene_prefab_stale_source_variant_alignment.v1` when the plan surfaces those row kinds.
- Per-row `local_child_variant_alignment.resolution_kind` and `stale_source_variant_alignment.resolution_kind` on applicable rows.
- `MK_editor_core_tests` asserts serialized ids on existing local-child and stale-keep fixtures.
- `engine/agent/manifest.json` `editor-productization` notes reference the new contract ids.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the author host or blockers are recorded.

## Validation evidence

| Check | Command / artifact | Result |
| --- | --- | --- |
| Unit tests | `ctest --preset dev --output-on-failure -R MK_editor_core_tests` | Pass (0.24s) |
| Repository validate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass |
