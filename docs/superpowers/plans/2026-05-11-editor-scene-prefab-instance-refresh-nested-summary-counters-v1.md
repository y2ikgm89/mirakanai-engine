# Editor Scene Prefab Instance Refresh Nested Summary Counters v1 (2026-05-11)

**Plan ID:** `editor-scene-prefab-instance-refresh-nested-summary-counters-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Orchestration:** [2026-05-10-unsupported-production-gaps-orchestration-program-v1.md](2026-05-10-unsupported-production-gaps-orchestration-program-v1.md)  
**Status:** Completed  

## Goal

Add deterministic **nested prefab review summary counters** to `ScenePrefabInstanceRefreshPlan` and retained `scene_prefab_instance_refresh.summary.*` UI rows so operators can see **unsupported nested prefab instance** row volume separately from **keep_nested_prefab_instance** reviewed rows, without implementing automatic nested propagation, merge/rebase UX, or multi-root refresh.

## Context

- `plan_scene_prefab_instance_refresh` already emits per-node rows with `keep_nested_prefab_instance` or `unsupported_nested_prefab_instance`.
- Summary labels exposed `keep_nested_prefab_instance_count` but did not count blocked unsupported nested rows explicitly.

## Constraints

- No change to apply/refresh graph semantics beyond additive plan fields and UI labels.
- Do not claim nested prefab propagation, automatic merge, or `scene-component-prefab-schema-v2` production editing completion.

## Done when

- `ScenePrefabInstanceRefreshPlan` carries `unsupported_nested_prefab_instance_count` incremented from plan rows.
- `make_scene_prefab_instance_refresh_ui_model` exposes retained `scene_prefab_instance_refresh.summary.unsupported_nested`.
- `MK_editor_core_tests` cover default blocked nested plan and keep-nested warning plan counters plus UI element ids.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the development host.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
| Targeted tests | `out/build/dev/Debug/MK_editor_core_tests.exe` | PASS |
