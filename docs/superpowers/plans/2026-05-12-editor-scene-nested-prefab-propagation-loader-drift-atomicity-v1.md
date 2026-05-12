# Editor Scene Nested Prefab Propagation Loader Drift Atomicity v1 (2026-05-12)

**Plan ID:** `editor-scene-nested-prefab-propagation-loader-drift-atomicity-v1`  
**Status:** Completed  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `editor-productization`

## Goal

Prove that reviewed nested prefab propagation remains atomic when the nested prefab loader succeeds during validation but fails during the later apply path.

## Context

- Missing, unresolved, and mismatched loaders are already covered when validation sees the failure up front.
- Reviewed nested propagation validates by simulating root and nested refreshes, then applies through an undoable batch action against a working scene clone.
- A stateful or drifting loader can report a valid nested prefab during validation and then fail before actual nested apply.

## Constraints

- Keep this slice test-only unless the regression exposes a real implementation failure.
- Do not weaken fail-closed behavior or commit partial root refreshes when a later nested apply fails.
- Keep all mutation behind `UndoableAction` and avoid automatic recovery, fuzzy matching, or merge/rebase UX.

## Done When

- `MK_editor_core_tests` uses a stateful nested prefab loader that returns `weapon.prefab` for validation calls and `std::nullopt` for the later apply call.
- `make_scene_prefab_instance_refresh_batch_action` returns an empty action / `UndoStack::execute` fails without changing undo or redo counts.
- The live scene, source-node counts, nested parent relation, and selected node remain unchanged after the failed action.
- Validation evidence is recorded for focused tests and the repository slice gate.

## Validation Evidence

- Passed: `cmake --build --preset dev --target MK_editor_core_tests` (2026-05-12).
- Passed: `ctest --preset dev --output-on-failure -R MK_editor_core_tests` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Next Candidate After Validation

- Return to `recommendedNextPlan.id=next-production-gap-selection` and pick the next `editor-productization` wedge from the manifest-selected order.
