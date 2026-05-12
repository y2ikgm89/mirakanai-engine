# Editor Game Module Driver Active Session DLL Barriers Policy UI v1 (2026-05-11)

**Plan ID:** `editor-game-module-driver-active-session-dll-barriers-policy-ui-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Program stream:** [2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md](2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md)

## Goal

Expose **fail-closed, read-only policy and barrier rows** under the existing `play_in_editor.game_module_driver.session` MK_ui contract so operators and AI agents see explicit acknowledgement that **DLL surface mutation (load / unload / reload) is blocked while Play-In-Editor is active**, and that **active-session hot reload and silent mid-play DLL replacement remain unsupported** with no operator override in this slice.

## Context

- Load / unload / reload review models already append `play-session-active` blockers in `make_editor_game_module_driver_*_model` ([editor/core/src/game_module_driver.cpp](../../editor/core/src/game_module_driver.cpp)).
- Session phase vocabulary lives in [docs/specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md](../specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md).
- This slice **documents the barrier in retained MK_ui** and mirrors key lines in `MK_editor` for human operators; it does not add hot reload, active-session reload execution, or policy toggles.

## Constraints

- No silent DLL replacement during play; no new execution paths that mutate `DynamicLibrary` while `play_session_active`.
- No compatibility shims: new element ids are additive under `play_in_editor.game_module_driver.session`.
- Manifest / `check-ai-integration` needles must list new contract strings when gap `notes` are audited.

## Done when

- `EditorGameModuleDriverHostSessionSnapshot` carries deterministic barrier/policy strings derived only from `play_session_active` / `driver_loaded` (and phase), with retained MK_ui labels:
  - `play_in_editor.game_module_driver.session.barriers_contract_label` → `ge.editor.editor_game_module_driver_host_session_dll_barriers.v1`
  - `play_in_editor.game_module_driver.session.barrier.play_dll_surface_mutation.status`
  - `play_in_editor.game_module_driver.session.policy.active_session_hot_reload`
  - `play_in_editor.game_module_driver.session.policy.stopped_state_reload_scope`
- [docs/specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md](../specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md) lists the new MK_ui rows.
- `MK_editor_core_tests` covers serialized session UI for the new ids (at least idle + play-active snapshots).
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes (or recorded toolchain blocker).

## Out of scope

- Implementing active-session hot reload, stable third-party ABI, `DesktopGameRunner` embedding, or automatic validation execution.
- Changing default load/reload/unload gate ordering beyond documenting it in session UI.

## Validation evidence

| Step | Command / artifact |
| --- | --- |
| Format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` when C++ touched |
| Unit | `ctest --preset dev -R editor_core_tests --output-on-failure` (or full `MK_editor_core_tests` selection) |
| Repo gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` |
