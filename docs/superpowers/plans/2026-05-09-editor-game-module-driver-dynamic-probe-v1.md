# Editor Game Module Driver Dynamic Probe Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the reviewed editor game-module driver contract with a real Windows dynamic-library probe that exports `mirakana_create_editor_game_module_driver_v1` and drives the existing isolated Play-In-Editor session through the loaded function table.

**Architecture:** Keep OS dynamic-library loading in `MK_platform` and the optional test/visible-host layer; `MK_editor_core` continues to own only the reviewed ABI/function-table contract and isolated session adapter. The probe is a Windows test fixture, not a production plugin ABI promise, and it keeps active-session hot reload, stable third-party ABI, `DesktopGameRunner` embedding, package scripts, renderer/RHI handles, native handles, and package streaming unsupported.

**Tech Stack:** C++23, `MK_editor_core`, `MK_platform`, CMake shared-library test fixture, CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Plan ID:** `editor-game-module-driver-dynamic-probe-v1`

**Status:** Completed.

---

## Context

- Active master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Selected gap: `editor-productization`.
- Previous slice: `docs/superpowers/plans/2026-05-09-editor-game-module-driver-contract-metadata-review-v1.md`.
- The current editor driver path validates load/reload review models and function tables, and the visible shell owns `mirakana::DynamicLibrary` lifetime, but there is no focused test that loads an actual game-module DLL exporting the reviewed factory symbol and then runs the resulting driver through `EditorPlaySession`.
- This slice adds that evidence without broadening the ready claim to stable plugin ABI, active hot reload, or runtime host embedding.

## Constraints

- Do not add production hot reload, active-session reload/unload, or automatic module replacement.
- Do not claim a stable third-party binary ABI; this remains same-engine-build evidence.
- Do not embed `DesktopGameRunner`, execute package scripts or validation recipes, expose renderer/RHI/native handles, stream packages, or move OS loading into `editor/core`.
- Keep the dynamic-library probe fixture under existing test locations; do not create new top-level folders.
- Gate the real dynamic-library test to Windows because `mirakana::load_dynamic_library` currently reports non-Windows hosts as unsupported.

## Files

- Create: `tests/fixtures/editor_game_module_driver_probe.cpp`
  - Export `mirakana_create_editor_game_module_driver_v1`.
  - Return an `EditorGameModuleDriverApi` with required `tick` and `destroy` callbacks plus optional `begin` and `end`.
  - Store deterministic static counters for test observation through additional safe exported probe functions.
- Create: `tests/unit/editor_game_module_driver_load_tests.cpp`
  - Load the probe DLL with `mirakana::load_dynamic_library`.
  - Resolve `mirakana_create_editor_game_module_driver_v1` through `mirakana::resolve_dynamic_library_symbol`.
  - Create an `IEditorPlaySessionDriver` through `make_editor_game_module_driver_from_symbol`.
  - Begin/tick/stop an `EditorPlaySession` and assert callback counters, simulation-scene isolation, and destroy-on-driver-reset behavior.
- Modify: `CMakeLists.txt`
  - Add Windows-only `MK_editor_game_module_driver_probe` shared-library fixture.
  - Add Windows-only `MK_editor_game_module_driver_load_tests` linked to `MK_editor_core` and `MK_platform`.
  - Pass `MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH` via target compile definitions and add the fixture dependency.
- Modify: `docs/editor.md`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`
  - Document the real DLL probe evidence and keep unsupported boundaries explicit.
- Modify: `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`
  - Keep Codex and Claude editor guidance aligned.
- Modify: `engine/agent/manifest.json`
  - Point `currentActivePlan` at this plan while active and add probe evidence to `editor-productization` notes without promoting the gap.
- Modify: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`
  - Add sentinels for the new probe target, exported factory symbol, test name, docs, skills, manifest, and plan.
- Modify: `docs/superpowers/plans/README.md`
  - Track this plan as the active slice.
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update the current selected-gap pointer and verdict.

## Done When

- A Windows CMake build produces `MK_editor_game_module_driver_probe` as a shared test module.
- `MK_editor_game_module_driver_load_tests` proves the probe DLL loads, resolves `mirakana_create_editor_game_module_driver_v1`, creates the existing editor driver adapter, mutates only the isolated simulation scene, and destroys probe state when the loaded driver is reset.
- The evidence remains explicitly same-engine-build and test-fixture scoped.
- `editor/core` still does not depend on `MK_platform` dynamic-library APIs.
- Focused build/test evidence for the new target passes.
- Docs, skills, manifest, plan registry, master plan, and static checks describe the same boundary.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/tool blocker.

## Tasks

### Task 1: Add failing real DLL probe coverage

**Files:**
- Create: `tests/unit/editor_game_module_driver_load_tests.cpp`

- [x] Add `editor game module driver loads real dynamic probe and ticks isolated session`.
- [x] Use `MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH` as the reviewed absolute module path.
- [x] Assert `make_editor_game_module_driver_load_model` is ready for that path and factory symbol.
- [x] Load the DLL, resolve `mirakana_create_editor_game_module_driver_v1`, and create a driver through `make_editor_game_module_driver_from_symbol`.
- [x] Begin/tick/stop an `EditorPlaySession` over a source `SceneAuthoringDocument`.
- [x] Assert probe begin/tick/end counters and frame/delta evidence.
- [x] Assert source-scene node count is unchanged while the simulation scene is mutated.
- [x] Assert destroy counter increments when the driver is reset.
- [x] Build the new test target and verify it fails before the probe fixture/CMake target exists.

Run:

```powershell
cmake --build --preset dev --target MK_editor_game_module_driver_load_tests
```

Expected before implementation: CMake target or compile-definition/probe symbol failure.

### Task 2: Add the Windows probe fixture and CMake target

**Files:**
- Create: `tests/fixtures/editor_game_module_driver_probe.cpp`
- Modify: `CMakeLists.txt`

- [x] Export `mirakana_create_editor_game_module_driver_v1` from the probe DLL.
- [x] Export counter reset/read helpers with safe symbol names.
- [x] Add Windows-only CMake shared-library fixture and test executable.
- [x] Link the test executable to `MK_editor_core` and `MK_platform`.
- [x] Keep fixture dependency explicit with `add_dependencies`.

### Task 3: Synchronize docs, manifest, skills, and static checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/roadmap.md`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

- [x] Record real DLL probe evidence under `editor-productization`.
- [x] Keep unsupported hot reload, active-session reload, stable third-party ABI, `DesktopGameRunner` embedding, package scripts, renderer/RHI handles, native handles, package streaming, and broad editor productization explicit.
- [x] Add static sentinels for target names, exported symbols, test names, docs, skills, manifest, and this plan.

### Task 4: Validate and close the slice

**Files:**
- Modify: this plan file.

- [x] Run focused build and CTest for `MK_editor_game_module_driver_load_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact validation evidence.
- [x] Set this plan status to `Completed`, move `currentActivePlan` back to the master plan, and update the plan registry latest completed slice.

## Validation Evidence

- RED: `ctest --preset dev --output-on-failure -R MK_editor_game_module_driver_load_tests` failed before the probe fixture existed because the temporary RED target loaded `MK_dynamic_library_probe` and could not resolve `mirakana_create_editor_game_module_driver_v1`.
- PASS: `cmake --build --preset dev --target MK_editor_game_module_driver_load_tests`.
- PASS: `ctest --preset dev --output-on-failure -R MK_editor_game_module_driver_load_tests` (`1/1` tests passed).
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (`30/30` default-preset tests passed; diagnostic-only host gates remain Metal/Apple on this Windows host, Android release signing not configured, and Android device smoke not connected).
- PASS after closeout patch: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS after closeout patch: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS after closeout patch: `git diff --check` with line-ending warnings only.

## Status

Completed.
