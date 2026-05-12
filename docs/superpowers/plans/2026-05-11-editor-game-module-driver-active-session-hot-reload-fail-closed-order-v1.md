# Editor Game Module Driver Active Session Hot Reload Fail-Closed Order v1 (2026-05-11)

**Plan ID:** `editor-game-module-driver-active-session-hot-reload-fail-closed-order-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md](2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md)

## Goal

Ship the first **verifiable, fail-closed** vertical toward the active-session hot-reload program: a **canonical host DLL mutation order** exposed on `EditorGameModuleDriverHostSessionSnapshot` and retained MK_ui (`play_in_editor.game_module_driver.session.policy.dll_mutation_order_guidance`), plus **table-driven tests** that tie the four session phases to load/reload/unload review blockers. This slice **does not** implement mid-play DLL replacement, silent hot reload, or operator override while play is active.

## Context

- [Hot reload session state machine v1](../specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md) blocks DLL surface mutation while `play_session_active`.
- Windows: callers must finish using a module and release `HMODULE` references before `FreeLibrary` can unload it (see Microsoft Learn `FreeLibrary` remarks).

## Constraints

- No change to `requiredBeforeReadyClaim` semantics for `editor-productization`; no ready-claim expansion.
- Stable snake_case machine strings for `policy_dll_mutation_order_guidance` values.
- Fragment-edit `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` then compose; do not hand-edit `engine/agent/manifest.json`.

## Done when

- `policy_dll_mutation_order_guidance` populated per phase; MK_ui row present; `MK_editor_core_tests` covers all four phases and a play-active barrier matrix for load/reload/unload models.
- `tools/check-ai-integration.ps1` needles include the new retained row id.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` run after fragment updates.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `tools/build.ps1` pass (or recorded host blocker).

## Validation evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` — exit code 0 (includes `check-ai-integration.ps1`, `check-json-contracts.ps1`, `build.ps1`, `ctest --preset dev`; 47/47 tests passed including `MK_editor_core_tests` and `MK_editor_game_module_driver_load_tests`).

## References (host DLL lifetime)

- Microsoft Learn: [FreeLibrary function (libloaderapi.h)](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-freelibrary) — decrements the module reference count; unload occurs when the count reaches zero; `DllMain` receives `DLL_PROCESS_DETACH` before unload.
- Microsoft Learn: [FreeLibrary and AfxFreeLibrary (C++ build)](https://learn.microsoft.com/en-us/cpp/build/freelibrary-and-afxfreelibrary?view=msvc-170) — paired `LoadLibrary`/`FreeLibrary` usage expectations.
