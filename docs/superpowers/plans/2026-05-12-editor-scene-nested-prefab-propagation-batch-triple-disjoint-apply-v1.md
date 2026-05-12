# Editor Scene Nested Prefab Propagation Batch Triple-Disjoint Apply v1 (2026-05-12)

**Plan ID:** `editor-scene-nested-prefab-propagation-batch-triple-disjoint-apply-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)

## Goal

Extend `MK_editor_core_tests` so `plan_scene_prefab_instance_refresh_batch` / `make_scene_prefab_instance_refresh_batch_action` with `apply_reviewed_nested_prefab_propagation` stays correct when **three** disjoint linked prefab instance roots are refreshed in one batch, including undo/redo.

## Context

- Two-root disjoint batch coverage exists in [2026-05-12-editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1.md).
- Batch apply already remaps instance roots across ordered steps; a third root exercises `source_to_result_node_id` / ordering over a longer chain without changing operator policy.

## Constraints

- Keep reviewed operator policy only; no automatic merge/rebase or fuzzy nested propagation UX.
- Do not narrow manifest `requiredBeforeReadyClaim` without a separate governance slice.

## Done when

- `MK_editor_core_tests` builds a scene with three disjoint `player.prefab` hierarchies each nesting `weapon.prefab`, runs a single batch refresh with `apply_reviewed_nested_prefab_propagation`, asserts three `Grip` and three `Shield` prefab-sourced nodes after apply, and asserts undo/redo restores counts to zero then back to three.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the implementation host.

## Validation evidence

| Step | Command | Expected | Result (2026-05-12) |
| --- | --- | --- | --- |
| Editor core | `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | Pass | Pass |
| Repository | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Pass |
