# Editor Scene Nested Prefab Propagation Local Child Policy v1 (2026-05-12)

**Plan ID:** `editor-scene-nested-prefab-propagation-local-child-policy-v1`  
**Status:** Completed  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `editor-productization`

## Goal

Prove that reviewed nested prefab propagation carries the explicit `keep_local_children` policy into nested refreshes, preserving local child subtrees under descendant prefab instances across apply, undo, and redo.

## Context

- Local child preservation is covered for a direct prefab instance refresh.
- Reviewed nested propagation is covered for single-target, multi-target, triple-disjoint, fail-closed loader, two-level propagation, and selected nested source-node remapping.
- The remaining policy propagation risk is that nested refreshes could drop `keep_local_children` while still applying reviewed `apply_reviewed_nested_prefab_propagation`.

## Constraints

- Keep this slice test-only unless the regression exposes a real implementation failure.
- Preserve opt-in reviewed propagation semantics; do not add automatic merge/rebase UX or fuzzy matching.
- Keep editor/core GUI-independent and all mutation behind `UndoableAction`.

## Done When

- `MK_editor_core_tests` creates a nested `weapon.prefab` instance with a local child subtree.
- A batch refresh with `keep_local_children=true`, `keep_nested_prefab_instances=true`, and `apply_reviewed_nested_prefab_propagation=true` applies both root and nested source additions.
- The local subtree remains local, selected, and parented under the nested weapon hierarchy after apply, undo, and redo.
- Validation evidence is recorded for focused tests and the repository slice gate.

## Validation Evidence

- Passed: `cmake --build --preset dev --target MK_editor_core_tests` (2026-05-12).
- Passed: `ctest --preset dev --output-on-failure -R MK_editor_core_tests` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Next Candidate After Validation

- Loader drift atomicity is complete in [2026-05-12-editor-scene-nested-prefab-propagation-loader-drift-atomicity-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-loader-drift-atomicity-v1.md). Return to `recommendedNextPlan.id=next-production-gap-selection` and pick the next `editor-productization` wedge from the manifest-selected order.
