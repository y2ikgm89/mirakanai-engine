# Editor Scene Nested Prefab Propagation Selected Added Node Remap v1 (2026-05-12)

**Plan ID:** `editor-scene-nested-prefab-propagation-selected-added-node-remap-v1`  
**Status:** Completed  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `editor-productization`

## Goal

Prove that reviewed nested prefab propagation keeps editor selection on a logically selected nested prefab source node when that node is remapped through an inner nested refresh instead of preserved as the selected nested root.

## Context

- Multi-target and triple-disjoint reviewed nested propagation batch apply are covered.
- Two-level `player -> weapon -> gem` propagation is covered for a preserved selected nested `gem.prefab` root.
- The remaining selection risk is an inner source node, such as `weapon.prefab` `Blade`, that is remapped through a nested refresh and then must stay selected while later nested propagation steps run.
- Matching source nodes intentionally preserve local editor display names and component/transform overrides while refreshing their prefab source metadata.

## Constraints

- Keep this slice test-only unless the new regression test exposes a real implementation failure.
- Keep `editor/core` GUI-independent and preserve reviewed `apply_reviewed_nested_prefab_propagation` semantics.
- Do not add automatic merge/rebase UX, fuzzy matching, or broader ready claims.

## Done When

- `MK_editor_core_tests` selects a nested `weapon.prefab` `Blade` node before a reviewed two-level propagation batch.
- Batch apply adds `Shield`, `Grip`, and `Sparkle` rows through reviewed propagation.
- The selected node after apply is the remapped `weapon.prefab` `Blade` source node under the refreshed weapon hierarchy, not a stale id or a nested root fallback.
- The selected node keeps its local display name override across apply, undo, and redo while still reporting `weapon.prefab` `Blade` source metadata.

## Validation Evidence

- RED observed: `ctest --preset dev --output-on-failure -R MK_editor_core_tests` failed on the initial over-strict expectation that refreshed matching source nodes should drop local display-name overrides; test expectation was corrected to the existing reviewed refresh contract.
- Passed: `cmake --build --preset dev --target MK_editor_core_tests` (2026-05-12).
- Passed: `ctest --preset dev --output-on-failure -R MK_editor_core_tests` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` (2026-05-12).
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Next Candidate After Validation

- Local-child policy coverage during nested propagation is complete in [2026-05-12-editor-scene-nested-prefab-propagation-local-child-policy-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-local-child-policy-v1.md). Return to `recommendedNextPlan.id=next-production-gap-selection`; the next Windows-default candidate is loader drift atomicity unless the manifest-selected order changes.
