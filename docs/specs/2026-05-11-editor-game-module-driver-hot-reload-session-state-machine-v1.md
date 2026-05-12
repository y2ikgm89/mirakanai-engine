# Editor Game Module Driver Hot Reload Session State Machine v1 (2026-05-11)

## Purpose

Define a **deterministic, host-independent session phase model** for combining:

1. Whether **Play-In-Editor** is active (`play_session_active`).
2. Whether a **`GameEngine.EditorGameModuleDriver.v1`** dynamic module is **resident** (`driver_loaded`: editor holds `DynamicLibrary` + `IEditorPlaySessionDriver`).

This model is the specification backbone for **stopped-state** load / unload / reload reviews (`EditorGameModuleDriverLoadModel`, `EditorGameModuleDriverUnloadModel`, `EditorGameModuleDriverReloadModel`). It explicitly **does not** specify active-session DLL replacement or hot reload while play is running.

## Authority

- Implementation plan: [`docs/superpowers/plans/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-spec-v1.md`](../superpowers/plans/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-spec-v1.md)
- Parent program stream: [`docs/superpowers/plans/2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md`](../superpowers/plans/2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md)

## Session phases

Boolean inputs: `play_session_active`, `driver_loaded`.

| Phase id | play_session_active | driver_loaded | Meaning |
| --- | --- | --- | --- |
| `idle_no_driver_play_stopped` | false | false | Editor can review **Load** when path/symbol validation passes; no resident DLL. |
| `driver_resident_play_stopped` | false | true | **Stopped-state** territory: explicit unload or stopped-state reload tracks apply; **Load** is blocked because a driver is already resident. |
| `play_active_without_driver` | true | false | Play uses non–game-module drivers only; **Load** / **Unload** / **Reload** reviews that mutate the DLL surface remain blocked while play is active. |
| `play_active_with_driver` | true | true | Play is active **and** the game module driver is resident; **Load** / **Unload** / **Reload** mutation paths stay blocked until play stops. |

## Barriers (fail-closed)

These barriers align with existing review models and shell gates:

- **Play barrier:** While `play_session_active`, load / unload / reload reviews that change DLL residency must stay **blocked** (undo stacks and simulation isolation stay authoritative; no silent replacement mid-play).
- **Single residency barrier:** While `driver_loaded`, implicit **second Load** is rejected; operator must use explicit **Unload** or reviewed **Reload** (stopped-state only).
- **Hot reload:** Replacing the DLL **during** an active play session is **out of scope** for this phase table; future hot-reload work must extend this spec with additional states and explicit policy UI.

## MK_ui contract

Retained rows under root id **`play_in_editor.game_module_driver.session`** expose:

- `phase_id` — stable identifier from the table above.
- `summary` — single-line human summary for operators and AI contracts.
- `contract_label` — `ge.editor.editor_game_module_driver_host_session.v1`.
- `play_session_active` / `driver_loaded` — boolean diagnostics as `true` / `false` text.
- `barriers_contract_label` — `ge.editor.editor_game_module_driver_host_session_dll_barriers.v1` (read-only DLL barrier/policy bundle).
- `barrier.play_dll_surface_mutation.status` — `enforced_block_load_unload_reload` while Play-In-Editor is active; `inactive_play_stopped` when play is stopped.
- `policy.active_session_hot_reload` — `unsupported_no_silent_dll_replacement_mid_play` (no mid-play silent replacement; no operator override in this slice).
- `policy.stopped_state_reload_scope` — `reload_and_duplicate_load_reviews_require_play_stopped_explicit_paths`.

## API mapping

- `make_editor_game_module_driver_host_session_snapshot(play_session_active, driver_loaded)` returns `EditorGameModuleDriverHostSessionSnapshot`.
- `make_editor_game_module_driver_host_session_ui_model(snapshot)` serializes retained MK_ui labels for the session panel.

## DLL mutation order guidance (fail-closed slice, 2026-05-11)

`EditorGameModuleDriverHostSessionSnapshot::policy_dll_mutation_order_guidance` is a **stable machine string** summarizing the **canonical operator order** for same-process `GameEngine.EditorGameModuleDriver.v1` residency on Windows-class hosts: finish play, destroy the driver adapter (so DLL exports are not called), release `DynamicLibrary` handles, then reload or load. It does **not** authorize mid-play mutation; barriers remain as in the phase table above.

Retained MK_ui row: `play_in_editor.game_module_driver.session.policy.dll_mutation_order_guidance`.

Implementation plan: [`2026-05-11-editor-game-module-driver-active-session-hot-reload-fail-closed-order-v1.md`](../superpowers/plans/2026-05-11-editor-game-module-driver-active-session-hot-reload-fail-closed-order-v1.md).

## Future work (not in this spec revision)

- Active-session hot reload state splits, reload barriers, and validation recipes per [`2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md`](../superpowers/plans/2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md).
- Stable third-party ABI track remains separate from same-engine-build residency phases.
