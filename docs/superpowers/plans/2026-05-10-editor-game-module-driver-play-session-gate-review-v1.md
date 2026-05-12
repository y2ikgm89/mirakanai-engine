# Editor Game Module Driver Play Session Gate Review v1 (2026-05-10)

**Plan ID:** `editor-game-module-driver-play-session-gate-review-v1`  
**Gap:** `editor-productization`  
**Status:** Implementation plan (execution slice).

## Goal

Centralize Play-In-Editor and driver-loaded gate semantics for **load**, **reload**, and **unload** in `MK_editor_core` review models so retained `MK_ui` rows document the same blockers the visible `MK_editor` shell enforces, without claiming active-session hot reload, stable third-party ABI, broader embedding, package scripts, renderer/RHI handles, native handles, package streaming, or broad editor productization.

## Context

- `EditorGameModuleDriverReloadDesc` already carries `play_session_active`; reload review blocks active sessions.
- `MK_editor` duplicated Play-In-Editor checks for load/unload in imperative code while `make_editor_game_module_driver_load_model` omitted play-session and already-loaded gates.
- Master plan and manifest require a dated focused plan before implementation (`production-completion-master-plan-v1`).

## Constraints

- Keep `editor/core` free of SDL3, Dear ImGui, OS handles, and process execution.
- Do not run CTest from editor UI; probe evidence remains label-only.
- Do not weaken unsupported-claim boundaries from prior driver slices.

## Done When

- `EditorGameModuleDriverLoadDesc` includes `play_session_active` and `driver_already_loaded`; `make_editor_game_module_driver_load_model` records consistent blockers and `can_load` reflects shell gates.
- `EditorGameModuleDriverUnloadDesc`, `EditorGameModuleDriverUnloadModel`, `make_editor_game_module_driver_unload_model`, and `make_editor_game_module_driver_unload_ui_model` exist with root id `play_in_editor.game_module_driver.unload`.
- Visible shell uses review models for load/unload enablement (no duplicate gate logic).
- Unit tests cover load/unload gate outcomes and retained element ids.
- `engine/agent/manifest.json`, plan registry, master plan Current Verdict, `docs/editor.md` / `docs/current-capabilities.md` as needed, static checks, and Codex/Claude editor skills stay aligned.

## Validation Evidence

| Check | Command / artifact |
| --- | --- |
| Unit | `MK_editor_core_tests` cases for load play-session / already-loaded and unload gates + UI ids |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` |
