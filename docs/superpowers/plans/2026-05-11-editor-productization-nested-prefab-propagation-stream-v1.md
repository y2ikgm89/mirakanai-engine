# Editor Productization Nested Prefab Propagation Stream v1 (2026-05-11)

**Plan ID:** `editor-productization-nested-prefab-propagation-stream-v1`  
**Gap:** `editor-productization` (primary) / `scene-component-prefab-schema-v2` (follow-on authoring)  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Status:** Active program stream (planning ledger; not a single implementation slice)  

## Goal

Sequence work toward **nested prefab propagation and merge-style UX** under reviewed, undoable editor actions—without automatic silent merges or fuzzy matching.

## Context

- Completed slices: prefab instance refresh review, local child / stale node / nested **keep** rows (`keep_nested_prefab_instance`), explicit `unsupported_nested_prefab_instance` blockers.
- [2026-05-11-editor-scene-prefab-instance-refresh-nested-summary-counters-v1.md](2026-05-11-editor-scene-prefab-instance-refresh-nested-summary-counters-v1.md) adds unsupported nested **summary** visibility.

## Constraints

- Every mutation remains behind `UndoableAction` and retained MK_ui review rows with stable ids for `tools/check-ai-integration.ps1` needles when ids change.
- Propagation graphs must remain deterministic and fail-closed on ambiguous source identity.

## Done when (stream exit)

- Chained refresh or propagation **plan preview** covers descendant linked prefab roots with explicit operator policy rows.
- Manifest `editor-productization` / `scene-component-prefab-schema-v2` `requiredBeforeReadyClaim` rows shrink only with matching tests and package/editor evidence.
- Each engineering slice closes with its own dated child plan + `validate.ps1` evidence.

## Next suggested child slices

1. **Completed (dry-run counters):** [2026-05-11-editor-nested-prefab-propagation-candidate-dry-run-v1.md](2026-05-11-editor-nested-prefab-propagation-candidate-dry-run-v1.md) — `descendant_linked_prefab_instance_root_count`, `distinct_nested_prefab_asset_count`, retained summary rows; no chained apply.
2. **Completed (reviewed multi-target batch):** [2026-05-11-editor-scene-prefab-instance-refresh-multi-target-batch-v1.md](2026-05-11-editor-scene-prefab-instance-refresh-multi-target-batch-v1.md) — explicit batch plan/apply over multiple linked prefab roots, retained `scene_prefab_instance_refresh_batch` rows, MK_editor batch review controls; no chained propagation or automatic merge.
3. **Completed (nested variant alignment):** [2026-05-11-editor-scene-prefab-nested-variant-alignment-v1.md](2026-05-11-editor-scene-prefab-nested-variant-alignment-v1.md) — `prefab_variant_conflict_resolution_kind_label`, retained `scene_prefab_instance_refresh.nested_prefab_variant_alignment.*` and per-row `nested_variant_alignment.resolution_kind` for nested refresh rows; no chained propagation.
4. **Completed (source node variant alignment):** [2026-05-11-editor-scene-prefab-source-node-variant-alignment-v1.md](2026-05-11-editor-scene-prefab-source-node-variant-alignment-v1.md) — `preserve_node` / `add_source_node` / `remove_stale_node` → per-row `source_node_variant_alignment.resolution_kind`, retained `ge.editor.scene_prefab_source_node_variant_alignment.v1`, single and batch contract roots; no chained propagation.
5. **Completed (propagation plan preview rows):** [2026-05-11-editor-scene-nested-prefab-propagation-plan-preview-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-plan-preview-v1.md) — deterministic `nested_prefab_propagation_preview` list + retained `propagation_preview` MK_ui contract rows + `preview_only_no_chained_refresh` operator policy; no chained apply.
6. **Completed (local / stale variant alignment):** [2026-05-11-editor-scene-prefab-local-stale-variant-alignment-v1.md](2026-05-11-editor-scene-prefab-local-stale-variant-alignment-v1.md) — `local_child_variant_alignment` / `stale_source_variant_alignment` retained roots + per-row `resolution_kind` for applicable refresh rows; nested alignment ids unchanged.
7. **Chained propagation apply (opt-in):** [2026-05-11-editor-scene-nested-prefab-propagation-apply-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-apply-v1.md) — reviewed apply of `plan_scene_prefab_instance_refresh` to `nested_prefab_propagation_preview` roots after root refresh; fail-closed planning; disjoint multi-target batch pairing with propagation is implemented in [2026-05-12-editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1.md); inner applies do not recurse propagation.
8. **Completed (undo/redo on single-root propagation action):** [2026-05-11-editor-scene-nested-prefab-propagation-undo-redo-test-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-undo-redo-test-v1.md) — `MK_editor_core_tests` exercises `UndoStack::undo` / `redo` after `make_scene_prefab_instance_refresh_action` with `apply_reviewed_nested_prefab_propagation` (Shield + Grip appear, undo removes both, redo restores); does not change manifest `requiredBeforeReadyClaim`.
9. **Completed (undo/redo on single-target batch propagation action):** [2026-05-11-editor-scene-nested-prefab-propagation-batch-undo-redo-test-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-batch-undo-redo-test-v1.md) — same scenario through `make_scene_prefab_instance_refresh_batch_action` with one `ScenePrefabInstanceRefreshBatchTargetInput`.
10. **Completed (multi-target batch propagation apply + undo/redo):** [2026-05-12-editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1.md) — two disjoint player roots with nested weapon; `plan_scene_prefab_instance_refresh_batch` `can_apply` with `apply_reviewed_nested_prefab_propagation`; `MK_editor_core_tests` batch action Shield + Grip counts under both hierarchies with undo/redo.
11. **Completed (triple-disjoint batch propagation apply + undo/redo):** [2026-05-12-editor-scene-nested-prefab-propagation-batch-triple-disjoint-apply-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-triple-disjoint-apply-v1.md) — three disjoint player roots with nested weapon; `ordered_targets.size() == 3`; `MK_editor_core_tests` `editor scene prefab instance refresh batch triple disjoint can apply nested prefab propagation` with undo/redo.
12. **Completed (blocked batch policy label):** [2026-05-12-editor-scene-nested-prefab-propagation-batch-blocked-policy-label-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-blocked-policy-label-v1.md) — blocked batch plans still mirror `apply_reviewed_nested_prefab_propagation` in retained UI rows while empty, duplicate, and ancestor/descendant target sets remain fail-closed.
13. **Completed (fail-closed loader edge coverage):** [2026-05-12-editor-scene-nested-prefab-propagation-fail-closed-edge-coverage-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-fail-closed-edge-coverage-v1.md) — missing, unresolved, and mismatched nested prefab loaders keep reviewed nested propagation batch apply blocked and leave the live scene unchanged.
14. **Completed (two-level batch propagation):** [2026-05-12-editor-scene-nested-prefab-propagation-two-level-batch-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-two-level-batch-v1.md) — disjoint batch roots with `player -> weapon -> gem` nested prefab chains add Shield / Grip / Sparkle through reviewed propagation, preserve the logically selected nested `gem.prefab` root, and retain undo/redo expectations.
15. **Completed (selected nested source-node remap):** [2026-05-12-editor-scene-nested-prefab-propagation-selected-added-node-remap-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-selected-added-node-remap-v1.md) — reviewed two-level batch propagation keeps selection on a remapped nested `weapon.prefab` `Blade` source node, preserving its local display name override across apply, undo, and redo.
16. **Completed (local-child policy during nested propagation):** [2026-05-12-editor-scene-nested-prefab-propagation-local-child-policy-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-local-child-policy-v1.md) — reviewed batch propagation carries `keep_local_children=true` into nested refreshes so a local child subtree under `weapon.prefab` stays local, selected, and undoable while Shield / Grip source additions apply.
17. **Completed (loader drift atomicity):** [2026-05-12-editor-scene-nested-prefab-propagation-loader-drift-atomicity-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-loader-drift-atomicity-v1.md) — a stateful nested loader that succeeds during validation and returns `std::nullopt` during later apply yields an empty undoable action and leaves the live scene, selection, and source-node counts unchanged.
18. **Completed (multi-target late loader drift atomicity):** [2026-05-12-editor-scene-nested-prefab-propagation-multi-target-late-loader-drift-atomicity-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-multi-target-late-loader-drift-atomicity-v1.md) — a two-target disjoint batch stays live-scene and undo-stack atomic when the later target's nested loader drifts after planning, first-target apply, and later-target validation already succeeded on the working clone.
