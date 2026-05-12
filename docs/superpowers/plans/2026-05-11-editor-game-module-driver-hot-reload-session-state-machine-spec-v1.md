# Editor Game Module Driver Hot Reload Session State Machine Spec v1 (2026-05-11)

**Plan ID:** `editor-game-module-driver-hot-reload-session-state-machine-spec-v1`  
**Gap:** `editor-productization`  
**Parent:** [`2026-05-03-production-completion-master-plan-v1.md`](2026-05-03-production-completion-master-plan-v1.md)  
**Spec:** [`docs/specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md`](../specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md)  
**Status:** Completed (spec + API + MK_ui + tests + manifest needles)

## Goal

Land the **hot reload stream item 1** deliverable: a formal session **state machine spec** plus minimal **review-facing API**, **retained MK_ui** rows, and **MK_editor_core_tests** coverage so agents and operators share one deterministic phase vocabulary before any active-session hot reload implementation.

## Context

- Reload transaction tests are complete ([`2026-05-11-editor-game-module-driver-reload-transaction-load-tests-v1.md`](2026-05-11-editor-game-module-driver-reload-transaction-load-tests-v1.md)).
- Load / reload / unload review models already encode play-session and residency gates; this slice makes the **combined phase** explicit and machine-readable.

## Constraints

- No active-session DLL swap; no stable third-party ABI claims.
- Preserve fail-closed alignment with `EditorGameModuleDriverLoadDesc`, `EditorGameModuleDriverUnloadDesc`, `EditorGameModuleDriverReloadDesc`.
- English prose for plan headings; ASCII for code identifiers.

## Done when

- Spec file exists under `docs/specs/` with phase table and barriers.
- `EditorGameModuleDriverHostSessionPhase`, `EditorGameModuleDriverHostSessionSnapshot`, `make_editor_game_module_driver_host_session_snapshot`, `make_editor_game_module_driver_host_session_ui_model` shipped in `MK_editor_core`.
- Retained root `play_in_editor.game_module_driver.session` with contract label `ge.editor.editor_game_module_driver_host_session.v1`.
- `MK_editor_core_tests` asserts serialized element ids for all four phase combinations.
- `tools/check-json-contracts.ps1` needles and `014-gameCodeGuidance.json` include new symbols; `editor-productization` gap notes mention session rows; manifest composed.

## Validation evidence

| Check | Result |
| --- | --- |
| Build | `cmake --build --preset dev --target MK_editor_core_tests` |
| Test | `ctest --preset dev --output-on-failure -R MK_editor_core_tests` |
| Repo | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` |
