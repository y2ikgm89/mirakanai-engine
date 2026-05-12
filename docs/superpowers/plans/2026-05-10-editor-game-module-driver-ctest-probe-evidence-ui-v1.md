# Editor Game Module Driver CTest Probe Evidence UI v1 (2026-05-10)

**Plan ID:** `editor-game-module-driver-ctest-probe-evidence-ui-v1`  
**Status:** Completed.

## Goal

Expose deterministic editor-core UI rows that tie the visible Play-In-Editor game-module driver lane to the Windows CTest dynamic-library probe evidence (`MK_editor_game_module_driver_probe`, `MK_editor_game_module_driver_load_tests`) without claiming active-session hot reload, stable third-party ABI, editor-executed tests, or broad editor productization.

## Context

- Master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Gap: `editor-productization`.
- Previous slice: `docs/superpowers/plans/2026-05-09-editor-game-module-driver-dynamic-probe-v1.md`.

## Constraints

- Do not run CTest or load the probe DLL from editor UI; retain evidence as reviewed labels only.
- Keep same-engine-build boundary; no stable plugin ABI or hot reload claims.
- Do not add `MK_platform` usage inside `editor/core` beyond existing patterns.

## Done When

- `EditorGameModuleDriverCtestProbeEvidenceModel` and `make_editor_game_module_driver_ctest_probe_evidence_ui_model` exist with stable `play_in_editor.game_module_driver.ctest_probe_evidence` element ids.
- Unit tests assert required UI element ids.
- Visible editor viewport toolbar shows the same probe/test target names and boundary copy for operators.
- Manifest `editor-productization` notes, plan registry, master plan verdict, docs, skills, and static checks reference this slice; `currentActivePlan` returns to the master plan with `recommendedNextPlan.id=next-production-gap-selection`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Validation evidence

| Check | Result |
| --- | --- |
| `MK_editor_core` tests | `MK_editor_core_tests` covers new UI ids |
| validate.ps1 | Run at slice close |
