# Editor Scene Nested Prefab Propagation Fail-Closed Edge Coverage v1 (2026-05-12)

**Plan ID:** `editor-scene-nested-prefab-propagation-fail-closed-edge-coverage-v1`  
**Status:** Completed  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `editor-productization`

## Goal

Prove that reviewed nested prefab propagation batch apply fails closed when nested prefab documents cannot be loaded or cannot be replanned safely, without leaving partial scene mutations or undo history entries.

## Context

- Disjoint multi-target nested prefab propagation apply is implemented.
- The previous blocked-policy-label slice keeps retained policy rows truthful even for blocked batch plans.
- `validate_nested_prefab_propagation_apply` already has loader-missing, loader-`nullopt`, and nested-plan-blocked fail-closed branches; this slice pins them to the batch action boundary.

## Constraints

- Keep `editor/core` GUI-independent.
- Do not add automatic nested prefab merge/rebase behavior.
- Do not execute validation until the operator explicitly approves it.
- Do not broaden ready claims beyond reviewed nested propagation rows.

## Done When

- `MK_editor_core_tests` contains a test where a batch root has a nested prefab preview and `apply_reviewed_nested_prefab_propagation = true`.
- The test covers missing `load_prefab_for_nested_propagation`, a loader returning `std::nullopt`, and a loader returning a mismatched prefab that blocks nested replan.
- Both cases produce blocked batch plans and an empty `make_scene_prefab_instance_refresh_batch_action`.
- The live `SceneAuthoringDocument` remains unchanged after each attempted action.

## Validation Evidence

- Passed: `cmake --build --preset dev --target MK_editor_core_tests` (2026-05-12).
- Passed: `ctest --preset dev --output-on-failure -R MK_editor_core_tests` (2026-05-12; final run after plan status and logical selection fixes).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` (2026-05-12; after returning `currentActivePlan` to the master plan).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (2026-05-12).

## Next Candidate After Validation

- Completed follow-up: `editor-scene-nested-prefab-propagation-two-level-batch-v1`.
- Next selection returns to `recommendedNextPlan.id=next-production-gap-selection`.
