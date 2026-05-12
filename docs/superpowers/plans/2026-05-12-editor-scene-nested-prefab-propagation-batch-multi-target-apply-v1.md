# Editor Scene Nested Prefab Propagation Batch Multi-Target Apply v1 (2026-05-12)

**Plan ID:** `editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)

## Goal

Allow `plan_scene_prefab_instance_refresh_batch` / `apply_scene_prefab_instance_refresh_batch` to succeed when `apply_reviewed_nested_prefab_propagation` is true and **more than one disjoint instance root** is selected, matching the existing sequential apply + id-remapping model (each step replans on the working document).

## Context

- `apply_scene_prefab_instance_refresh` already runs root refresh plus nested propagation previews in order.
- `apply_scene_prefab_instance_refresh_batch` already remaps later `instance_root` ids through `source_to_result_node_id` (including `outside_remap` for disjoint roots).
- A post-`finalize_scene_prefab_refresh_batch_plan` guard rejected multi-target + propagation without executing apply.

## Constraints

- Keep `prefab_refresh_batch_has_instance_root_hierarchy_conflict` and duplicate-root rejection.
- Do not enable automatic merge/rebase or fuzzy nested propagation UX beyond reviewed operator policy.
- No manifest `requiredBeforeReadyClaim` narrowing without separate governance slice.

## Done when

- The artificial `ordered_targets.size() > 1` + propagation diagnostic is removed from `plan_scene_prefab_instance_refresh_batch`.
- `MK_editor_core_tests` covers two disjoint player instance roots, each with nested weapon, batch plan `can_apply` with propagation, and `make_scene_prefab_instance_refresh_batch_action` applies Shield + Grip under both hierarchies.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the implementation host.

## Validation evidence

| Step | Command | Expected | Result (2026-05-12) |
| --- | --- | --- | --- |
| Editor core | `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | Pass | Pass |
| Repository | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Pass |
