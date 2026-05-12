# Editor Dynamic Game Module Driver Load Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the visible editor explicitly load a reviewed dynamic game-module driver and use it as the `IEditorPlaySessionDriver` for the existing isolated Play-In-Editor in-process runtime-host path.

**Architecture:** Keep dynamic loading behind `mirakana_platform` and keep editor state/lifecycle behind `mirakana_editor_core` models. The first production slice uses an absolute-path, explicit-symbol, Windows `LoadLibraryExW` path with `LOAD_LIBRARY_SEARCH_*` flags and a narrow `GameEngine.EditorGameModuleDriver.v1` function-table ABI, then the optional `mirakana_editor` shell owns the actual load/unload action and starts the already-reviewed `EditorPlaySession` driver path. This does not add hot reload, stable third-party binary ABI promises, `DesktopGameRunner` embedding, package scripts, renderer/RHI uploads or handles, package streaming, or broad editor productization.

**Tech Stack:** C++23, `mirakana_platform`, `mirakana_editor_core`, `mirakana_editor`, Win32 `LoadLibraryExW` / `GetProcAddress` / `FreeLibrary`, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Plan ID:** `editor-dynamic-game-module-driver-load-v1`

**Status:** Completed.

---

## Context

- Active master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Selected gap: `editor-productization`.
- Previous completed slice: `docs/superpowers/plans/2026-05-09-editor-runtime-scene-package-validation-execution-v1.md`.
- Existing editor work already supports `IEditorPlaySessionDriver`, isolated `EditorPlaySession`, external runtime-host process review, and linked-driver in-process runtime-host review.
- The stock visible editor still reports `linked_gameplay_driver_available=false`; it has no reviewed path for loading a game module into the editor process.
- Microsoft DLL security guidance recommends fully qualified paths and `LOAD_LIBRARY_SEARCH` flags instead of implicit current-directory/PATH search. References:
  - <https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-security>
  - <https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order>
  - <https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw>
  - <https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress>
  - <https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-freelibrary>

## Constraints

- Require absolute module paths for executable dynamic-library loads; do not search the current directory or `PATH`.
- Use `LoadLibraryExW` with `LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR` and `LOAD_LIBRARY_SEARCH_SYSTEM32` on Windows.
- Keep native module handles private to `mirakana_platform`; public APIs may expose only first-party RAII/status/symbol abstractions.
- Use a narrow function-table ABI with an explicit version and named factory symbol, not host-side `delete` across module boundaries.
- Do not claim stable third-party binary compatibility; this is a same-engine-build editor game-module handoff.
- Do not add automatic load on project open, hot reload, unload while active, package script execution, validation recipe execution, renderer/RHI uploads, renderer/RHI handle exposure, package streaming, or native handle exposure.

## Files

- Create: `engine/platform/include/mirakana/platform/dynamic_library.hpp`
  - Public RAII dynamic library handle, load status/result, symbol result, and safe symbol-name validation.
- Create: `engine/platform/src/dynamic_library.cpp`
  - Windows `LoadLibraryExW` / `GetProcAddress` / `FreeLibrary` implementation and non-Windows explicit unsupported diagnostics.
- Modify: `engine/platform/CMakeLists.txt`
  - Register the new platform source.
- Create: `editor/core/include/mirakana/editor/game_module_driver.hpp`
  - `GameEngine.EditorGameModuleDriver.v1` ABI constants/function-table types, reviewed load model, UI rows, and driver adapter declarations.
- Create: `editor/core/src/game_module_driver.cpp`
  - Validate load descriptors, wrap a loaded function-table as `IEditorPlaySessionDriver`, and retain `play_in_editor.game_module_driver` rows.
- Modify: `editor/CMakeLists.txt`
  - Register the new editor-core source.
- Modify: `editor/src/main.cpp`
  - Add visible explicit dynamic game-module driver load/unload controls and wire a loaded driver into `Begin In-Process Runtime Host`.
- Create: `tests/fixtures/dynamic_library_probe.cpp`
  - Tiny exported function for Windows dynamic-library loader tests.
- Modify: `tests/unit/platform_process_tests.cpp`
  - Add focused Windows platform dynamic-library load/symbol/failure tests.
- Modify: `tests/unit/editor_core_tests.cpp`
  - Add focused editor-core ABI/model/driver adapter tests.
- Modify: root `CMakeLists.txt`
  - Build the probe shared library and pass its absolute path to the platform test target.
- Modify: `docs/editor.md`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`, `docs/architecture.md`
  - Document the reviewed dynamic game-module driver load path and unsupported boundaries.
- Modify: `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`
  - Keep Codex/Claude editor guidance aligned.
- Modify: `engine/agent/manifest.json`
  - Set this plan as active and describe the new capability honestly under editor/platform guidance and `editor-productization` notes.
- Modify: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`
  - Add sentinel checks for new types, retained ids, docs, manifest, plan, and safe dynamic-library wording.
- Modify: `docs/superpowers/plans/README.md`
  - Track this plan as the active slice.
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update the current verdict and selected-gap evidence.

## Done When

- `mirakana_platform` exposes a move-only `DynamicLibrary` abstraction that loads only absolute module paths and resolves named function symbols without exposing native handles.
- Windows uses `LoadLibraryExW` with explicit `LOAD_LIBRARY_SEARCH_*` flags and reports deterministic diagnostics for missing modules, relative paths, unsafe symbol names, and missing symbols.
- Non-Windows hosts compile and report explicit unsupported dynamic-library loading diagnostics until host-specific loaders are accepted.
- `mirakana_editor_core` exposes `EditorGameModuleDriverLoadDesc`, `EditorGameModuleDriverLoadModel`, `EditorGameModuleDriverCreateResult`, `make_editor_game_module_driver_load_model`, `make_editor_game_module_driver_from_api`, `make_editor_game_module_driver_from_symbol`, and `make_editor_game_module_driver_load_ui_model`.
- The function-table ABI requires `GameEngine.EditorGameModuleDriver.v1`, an explicit ABI version, a tick callback, and a destroy callback; begin/end callbacks are optional.
- The visible `mirakana_editor` shell can explicitly load a reviewed module path, resolve `mirakana_create_editor_game_module_driver_v1`, store transient load evidence, unload only while no play session is active, and begin the in-process runtime host with the loaded driver.
- Unsupported claims for hot reload, `DesktopGameRunner` embedding, package scripts, validation recipes, arbitrary shell, renderer/RHI uploads or handle exposure, package streaming, stable third-party ABI, and broad editor productization are rejected deterministically.
- Focused platform/editor tests pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `git diff --check`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete host/tool blockers.

## Tasks

### Task 1: Select and register the active child slice

**Files:**
- Create: this plan file.
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`

- [x] Create this dated plan with Goal, Context, Constraints, Done When, tasks, and validation evidence.
- [x] Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan.
- [x] Set the plan registry Active slice row to this plan.
- [x] Keep `editor-productization` as the selected active gap burn-down.

### Task 2: Add platform dynamic-library foundation

**Files:**
- Create: `engine/platform/include/mirakana/platform/dynamic_library.hpp`
- Create: `engine/platform/src/dynamic_library.cpp`
- Modify: `engine/platform/CMakeLists.txt`
- Create: `tests/fixtures/dynamic_library_probe.cpp`
- Modify: `tests/unit/platform_process_tests.cpp`
- Modify: root `CMakeLists.txt`

- [x] Write the failing platform test for loading the probe shared library by absolute path and resolving `mirakana_dynamic_library_probe_add`.
- [x] Add negative tests for relative module path rejection, missing module failure, unsafe symbol-name rejection, and missing symbol failure.
- [x] Implement the move-only `DynamicLibrary` RAII abstraction and Windows loader.
- [x] Keep non-Windows builds compiling with explicit unsupported diagnostics.
- [x] Build and run the focused platform test target.

### Task 3: Add editor game-module driver ABI and model

**Files:**
- Create: `editor/core/include/mirakana/editor/game_module_driver.hpp`
- Create: `editor/core/src/game_module_driver.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Write failing editor-core tests for a valid function-table driver mutating only the isolated simulation scene through `EditorPlaySession`.
- [x] Add tests for invalid ABI version, missing tick callback, missing destroy callback, unsafe module path, unsafe symbol name, unsupported claims, and retained `play_in_editor.game_module_driver` rows.
- [x] Implement the v1 function-table ABI constants and adapter.
- [x] Implement the reviewed load model and retained UI model.
- [x] Build and run `mirakana_editor_core_tests`.

### Task 4: Wire the visible editor shell

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add transient game-module path text, load status, loaded library, loaded driver, and diagnostics state.
- [x] Render `Game Module Driver` controls near the in-process runtime-host controls.
- [x] Execute explicit load through `mirakana::load_dynamic_library`, resolve the reviewed factory symbol, and build the editor driver adapter.
- [x] Make `make_in_process_runtime_host_model` report `linked_gameplay_driver_available=true` only while a driver is loaded.
- [x] Make `Begin In-Process Runtime Host` call `begin_editor_in_process_runtime_host_session` with the loaded driver.
- [x] Block unload while a play session is active.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 5: Synchronize docs, manifest, skills, and checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

- [x] Document the reviewed dynamic game-module driver load path and official DLL security policy.
- [x] Keep hot reload, stable third-party ABI, `DesktopGameRunner` embedding, package scripts, validation recipes, renderer/RHI uploads or handles, package streaming, Metal readiness, and broad editor productization unsupported.
- [x] Add static sentinels for new types, retained ids, visible labels, official-policy wording, docs, skills, manifest, and this plan.

### Task 6: Validate and close the slice

**Files:**
- Modify: this plan file.
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`

- [x] Run focused platform/editor builds and tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact validation evidence here.
- [x] Mark this plan `Completed` and move `currentActivePlan` back to the master plan with `recommendedNextPlan.id=next-production-gap-selection`.

## Validation Evidence

- `cmake --build --preset dev --target mirakana_platform_process_tests` passed.
- `ctest --preset dev --output-on-failure -R mirakana_platform_process_tests` passed.
- `cmake --build --preset dev --target mirakana_editor_core_tests` passed.
- `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` passed, including 46/46 `desktop-gui` CTest tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after closeout pointer sync.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after closeout pointer sync; generated dry-run game directories were absent after the check.
- `git diff --check` passed with only line-ending warnings and no whitespace errors.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 29/29 `dev` CTest tests. Metal/Apple diagnostics remained host-gated on this Windows host and were reported as diagnostic-only / host-gated, not ready claims.
