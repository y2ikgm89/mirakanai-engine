# Editor Scene Prefab Source Node Variant Alignment v1 (2026-05-11)

**Plan ID:** `editor-scene-prefab-source-node-variant-alignment-v1`  
**Date:** 2026-05-11  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)  

## Goal

Extend **scene prefab instance refresh** review tooling so **preserve**, **add**, and **remove** primary refresh row kinds expose `PrefabVariantConflictResolutionKind` labels through the same retained MK_ui pattern as nested, local-child, and stale alignment—without changing default refresh applicability or implementing chained propagation.

## Context

- [2026-05-11-editor-scene-prefab-nested-variant-alignment-v1.md](2026-05-11-editor-scene-prefab-nested-variant-alignment-v1.md) and [2026-05-11-editor-scene-prefab-local-stale-variant-alignment-v1.md](2026-05-11-editor-scene-prefab-local-stale-variant-alignment-v1.md) define per-row `*.variant_alignment.resolution_kind` for nested / local / stale kinds.
- Stream item 4 calls for **broader scene-prefab row kinds**; `preserve_node`, `add_source_node`, and `remove_stale_node` are the primary diff rows not yet covered.

## Constraints

- Additive retained ids only; no change to refresh planning rules.
- Mapping: `preserve_node` → `accept_current_node`; `add_source_node` → `retarget_override`; `remove_stale_node` → `remove_override`.
- Batch UI emits contract roots at `scene_prefab_instance_refresh_batch.source_node_variant_alignment.*` when any target plan surfaces those row kinds.
- Stable element ids for `tools/check-ai-integration.ps1` needles.

## Done when

- `make_scene_prefab_instance_refresh_ui_model` and `make_scene_prefab_instance_refresh_batch_ui_model` emit optional roots `scene_prefab_instance_refresh.source_node_variant_alignment.*` / `scene_prefab_instance_refresh_batch.source_node_variant_alignment.*` with contract `ge.editor.scene_prefab_source_node_variant_alignment.v1` when applicable.
- Per-row `source_node_variant_alignment.resolution_kind` on preserve/add/remove rows.
- `MK_editor_core_tests` asserts serialized ids and label text on the existing prefab refresh fixture.
- `engine/agent/manifest.json` `editor-productization` notes reference the new contract id (via fragments + compose).
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the author host or blockers are recorded.

## Validation evidence

| Check | Command / artifact | Result |
| --- | --- | --- |
| Unit tests | `ctest --preset dev --output-on-failure -R MK_editor_core_tests` | Record exit code |
| Repository validate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Record exit code |
