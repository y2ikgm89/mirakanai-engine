# Editor Nested Prefab Propagation Candidate Dry Run v1 (2026-05-11)

**Plan ID:** `editor-nested-prefab-propagation-candidate-dry-run-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md)  
**Status:** Completed  

## Goal

Add deterministic **dry-run-only** counters on `ScenePrefabInstanceRefreshPlan` for **descendant linked prefab instance roots** (nested prefab roots under the selected refresh subtree that reference a different prefab identity than the outer instance) plus a **distinct nested prefab asset** count, exposed through retained `scene_prefab_instance_refresh.summary.*` UI rows. This is planning visibility for a future **chained refresh** workflow and does **not** change `apply_scene_prefab_instance_refresh` behavior or implement propagation/merge UX.

## Context

- `scene_prefab_refresh_is_nested_prefab_root` already classifies nested roots for per-row review.
- [2026-05-11-editor-scene-prefab-instance-refresh-nested-summary-counters-v1.md](2026-05-11-editor-scene-prefab-instance-refresh-nested-summary-counters-v1.md) added unsupported nested row summary counts.

## Constraints

- No new apply paths, no automatic multi-target refresh, no merge/rebase automation.
- Counters must be derivable from the same `Scene`, `instance_root`, `root_link`, and `subtree` inputs as the existing planner (deterministic, no filesystem IO).
- New retained row ids must be registered in `tools/check-ai-integration.ps1` if that script enforces explicit UI id lists for this surface (match existing `scene_prefab_instance_refresh.summary.*` pattern).

## Done when

- Plan fields populated on all non-early-return paths where `subtree` is available; early invalid plans keep counts at zero.
- `make_scene_prefab_instance_refresh_ui_model` exposes the new summary labels.
- `MK_editor_core_tests` asserts representative nested prefab fixture counts.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Targeted tests | `out/build/dev/Debug/MK_editor_core_tests.exe` | PASS |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
