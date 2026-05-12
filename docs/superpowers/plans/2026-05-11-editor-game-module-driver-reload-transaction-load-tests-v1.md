# Editor Game Module Driver Reload Transaction Load Tests v1 (2026-05-11)

**Plan ID:** `editor-game-module-driver-reload-transaction-load-tests-v1`  
**Gap:** `editor-productization`  
**Parent stream:** [2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md](2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md) (child slice 2 — reload transaction tests)  
**Parent roadmap:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Prove **stopped-state** editor game module driver **reload transaction** behavior on Windows using the real probe DLL: destroy adapter, release `DynamicLibrary`, then load again and run a second isolated `EditorPlaySession` with clean probe counters. Add an additional test that **two** `load_dynamic_library` results on the same absolute path compose with Windows refcount semantics (first `FreeLibrary` does not unload while a second handle lives).

## Context

- `mirakana::load_dynamic_library` / `DynamicLibrary` live in `engine/platform/src/dynamic_library.cpp`.
- Reviewed load/reload **policy** remains in `make_editor_game_module_driver_load_model` / `make_editor_game_module_driver_reload_model` (`driver-already-loaded`, play-session gates); this slice adds **integration evidence** in `MK_editor_game_module_driver_load_tests`.
- Does **not** implement active-session hot reload, stable third-party ABI, or shell `reload_game_module_driver` wiring tests.

## Constraints

- Windows-only test target (existing `MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH` contract).
- No new public APIs; tests only.

## Done when

- `MK_editor_game_module_driver_load_tests` covers full unload + second load session and paired-handle refcount ordering.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on a Windows dev host with the probe target built.

## Validation evidence

| Command | Result |
| --- | --- |
| `cmake --build --preset dev --target MK_editor_game_module_driver_probe MK_editor_game_module_driver_load_tests` | Pass (2026-05-11, Windows) |
| `ctest --preset dev -R MK_editor_game_module_driver_load_tests --output-on-failure` | Pass (2026-05-11) |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass (2026-05-11) |
