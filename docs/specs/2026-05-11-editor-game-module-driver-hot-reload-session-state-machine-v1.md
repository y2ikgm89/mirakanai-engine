# Editor Game Module Driver Hot Reload Session State Machine v1 (2026-05-11)

## Status

Accepted narrow specification.

Current implementation truth lives in editor/core tests and `engine/agent/manifest.json`; this spec owns the deterministic stopped-state session model only.

## Purpose

Define a **deterministic, host-independent session phase model** for combining:

1. Whether **Play-In-Editor** is active (`play_session_active`).
2. Whether a **`GameEngine.EditorGameModuleDriver`** dynamic module is **resident** (`driver_loaded`: editor holds `DynamicLibrary` + `IEditorPlaySessionDriver`).

This model is the specification backbone for **stopped-state** load / unload / reload reviews (`EditorGameModuleDriverLoadModel`, `EditorGameModuleDriverUnloadModel`, `EditorGameModuleDriverReloadModel`). It explicitly **does not** specify active-session DLL replacement or hot reload while play is running.

## Authority

- Implementation evidence: `Editor Game Module Driver Hot Reload Session State Machine Spec v1` is retained in the historical evidence archive.
- Parent program stream: `Editor Productization Hot Reload Stable ABI Stream v1` is completed historical evidence, not an active plan file.

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
- `contract_label` — `ge.editor.editor_game_module_driver_host_session`.
- `play_session_active` / `driver_loaded` — boolean diagnostics as `true` / `false` text.
- `barriers_contract_label` — `ge.editor.editor_game_module_driver_host_session_dll_barriers` (read-only DLL barrier/policy bundle).
- `barrier.play_dll_surface_mutation.status` — `enforced_block_load_unload_reload` while Play-In-Editor is active; `inactive_play_stopped` when play is stopped.
- `policy.active_session_hot_reload` — `unsupported_no_silent_dll_replacement_mid_play` (no mid-play silent replacement; no operator override in this slice).
- `policy.stopped_state_reload_scope` — `reload_and_duplicate_load_reviews_require_play_stopped_explicit_paths`.

## API mapping

- `make_editor_game_module_driver_host_session_snapshot(play_session_active, driver_loaded)` returns `EditorGameModuleDriverHostSessionSnapshot`.
- `make_editor_game_module_driver_host_session_ui_model(snapshot)` serializes retained MK_ui labels for the session panel.

## DLL mutation order guidance (fail-closed slice, 2026-05-11)

`EditorGameModuleDriverHostSessionSnapshot::policy_dll_mutation_order_guidance` is a **stable machine string** summarizing the **canonical operator order** for same-process `GameEngine.EditorGameModuleDriver` residency on Windows-class hosts: finish play, destroy the driver adapter (so DLL exports are not called), release `DynamicLibrary` handles, then reload or load. It does **not** authorize mid-play mutation; barriers remain as in the phase table above.

Retained MK_ui row: `play_in_editor.game_module_driver.session.policy.dll_mutation_order_guidance`.

Implementation evidence: `Editor Game Module Driver Active Session Hot Reload Fail Closed Order v1` is retained in the historical evidence archive.

## Future work (not in this spec revision)

- Active-session hot reload state splits, reload barriers, and validation recipes remain future work beyond the completed `Editor Productization Hot Reload Stable ABI Stream v1` historical evidence.
- Stable third-party ABI track remains separate from same-engine-build residency phases.
