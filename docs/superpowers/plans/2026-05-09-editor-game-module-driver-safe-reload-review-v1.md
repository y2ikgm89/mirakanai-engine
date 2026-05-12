# Editor Game Module Driver Safe Reload Review Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Replace implicit stopped-state game-module reload with an explicit reviewed safe reload path that unloads and reloads only when Play-In-Editor is stopped.

**Architecture:** Keep reload review in `mirakana_editor_core` as deterministic model and retained UI rows. The optional `mirakana_editor` shell owns `DynamicLibrary` lifetime and exposes separate load, reload, and unload commands while keeping active-session hot reload, stable third-party ABI, package scripts, validation recipes, renderer/RHI handles, and package streaming unsupported.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_editor`, first-party `mirakana_ui` retained models, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Plan ID:** `editor-game-module-driver-safe-reload-review-v1`

**Status:** Completed.

---

## Context

- Active master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Selected gap: `editor-productization`.
- Previous slice: `docs/superpowers/plans/2026-05-09-editor-nested-prefab-refresh-resolution-v1.md`.
- `Editor Dynamic Game Module Driver Load v1` added reviewed module load, safe symbol/function-table validation, and visible load/unload controls.
- The visible shell currently lets `Load Game Module Driver` reset an already loaded stopped driver before loading again. This is an implicit reload path without a separate reviewed reload model.
- This slice makes reload explicit and stopped-only. It does not add active-session hot reload, stable third-party ABI, `DesktopGameRunner` embedding, package scripts, validation recipe execution, renderer/RHI uploads or handles, native handles, package streaming, or full editor productization.

## Files

- Modify: `editor/core/include/mirakana/editor/game_module_driver.hpp`
  - Add `EditorGameModuleDriverReloadDesc`.
  - Add `EditorGameModuleDriverReloadModel`.
  - Add `make_editor_game_module_driver_reload_model`.
  - Add `make_editor_game_module_driver_reload_ui_model`.
- Modify: `editor/core/src/game_module_driver.cpp`
  - Reuse load review for absolute path and safe symbol validation.
  - Add stopped-only reload blockers and unsupported-claim diagnostics.
  - Add retained `play_in_editor.game_module_driver.reload` rows.
- Modify: `tests/unit/editor_core_tests.cpp`
  - Add failing coverage for ready stopped reload, missing loaded driver, active-session blocker, unsupported hot-reload/stable-ABI claims, reused path/symbol blockers, and retained reload UI rows.
- Modify: `editor/src/main.cpp`
  - Prevent `Load Game Module Driver` from replacing an already loaded driver.
  - Add visible `Reload Game Module Driver` that works only when a driver is already loaded and Play-In-Editor is stopped.
  - Keep `Unload Game Module Driver` blocked while active.
- Modify: `docs/editor.md`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`
  - Document explicit stopped-state reload review and unsupported active hot reload boundaries.
- Modify: `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`
  - Keep Codex and Claude editor guidance aligned.
- Modify: `engine/agent/manifest.json`
  - Update `currentActivePlan`, `recommendedNextPlan`, and editor-productization notes.
- Modify: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`
  - Add sentinels for safe reload review model, UI rows, docs, skills, and manifest text.
- Modify: `docs/superpowers/plans/README.md`
  - Track this plan as the active slice.
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update current verdict and selected-gap evidence.

## Done When

- `Load Game Module Driver` no longer performs implicit replacement of an already loaded stopped driver.
- `EditorGameModuleDriverReloadModel` reports ready only when a driver is loaded, the reviewed load inputs are valid, and Play-In-Editor is not active.
- Active Play-In-Editor reload is blocked with explicit diagnostics and remains outside the ready claim.
- Unsupported reload claims, including hot reload, stable third-party ABI, `DesktopGameRunner` embedding, package scripts, validation recipes, renderer/RHI uploads or handles, package streaming, and broad editor productization, are rejected deterministically.
- Retained UI rows include `play_in_editor.game_module_driver.reload`.
- The visible editor exposes `Reload Game Module Driver` separately from load/unload.
- Focused `mirakana_editor_core_tests` pass.
- GUI build passes because `editor/src/main.cpp` changes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `git diff --check`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Tasks

### Task 1: Add failing reload review coverage

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `editor game module driver reload model reviews stopped safe reload`.
- [x] Assert a loaded, stopped, absolute-path reload model is ready and exposes `can_reload`.
- [x] Assert missing loaded driver blocks with `loaded-driver-required`.
- [x] Assert active Play-In-Editor blocks with `play-session-active`.
- [x] Assert relative path and unsafe symbol blockers are inherited from load review.
- [x] Assert unsupported hot-reload/stable-ABI/broad-editor claims are deterministic.
- [x] Assert retained UI rows include `play_in_editor.game_module_driver.reload`.
- [x] Build `mirakana_editor_core_tests` and verify the new test fails before implementation.

Run:

```powershell
cmake --build --preset dev --target mirakana_editor_core_tests
```

Expected before implementation: compile failure for the new reload desc/model/functions.

### Task 2: Implement editor-core reload review

**Files:**
- Modify: `editor/core/include/mirakana/editor/game_module_driver.hpp`
- Modify: `editor/core/src/game_module_driver.cpp`

- [x] Add reload desc/model public structs.
- [x] Add `make_editor_game_module_driver_reload_model`.
- [x] Reuse `make_editor_game_module_driver_load_model` for path/symbol/unsupported load-claim validation.
- [x] Add `loaded-driver-required` and `play-session-active` blockers.
- [x] Add deterministic unsupported reload claim ordering.
- [x] Add `make_editor_game_module_driver_reload_ui_model` with retained reload rows.

### Task 3: Wire visible shell commands

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add a visible reload review model beside the load review.
- [x] Make `load_game_module_driver` reject already loaded drivers instead of replacing them.
- [x] Add `reload_game_module_driver` that validates reload review, resets the existing driver/library, and invokes the existing stopped load path.
- [x] Add a `Reload Game Module Driver` button enabled only when the reload model is ready.
- [x] Keep all load, reload, and unload paths blocked while Play-In-Editor is active.

### Task 4: Synchronize docs, manifest, skills, and static checks

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

- [x] Document safe stopped reload as explicit reviewed reload only.
- [x] Keep unsupported claims explicit: no active-session hot reload, stable third-party ABI, `DesktopGameRunner` embedding, package/validation execution, renderer/RHI handles, native handles, or package streaming.
- [x] Add static sentinels for reload desc/model/functions, retained rows, visible command label, docs, skills, and manifest text.

### Task 5: Validate and close the slice

**Files:**
- Modify: this plan file.

- [x] Run focused editor-core build and tests.
- [x] Run GUI build.
- [x] Run relevant static checks.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact validation evidence in this plan.
- [x] Set this plan status to `Completed`, move `currentActivePlan` back to the master plan, and update the plan registry latest completed slice.

## Validation Evidence

- RED: `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation with missing `EditorGameModuleDriverReloadDesc`, `EditorGameModuleDriverReloadModel`, `make_editor_game_module_driver_reload_model`, and `make_editor_game_module_driver_reload_ui_model`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after adding the exact `Reload Game Module Driver` roadmap sentinel.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- PASS: `cmake --build --preset dev --target mirakana_editor_core_tests`.
- PASS: `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` (`1/1` tests passed).
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- PASS: `git diff --check` with line-ending warnings only.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` (`46/46` GUI-preset tests passed).
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (`29/29` default-preset tests passed; diagnostic-only host gates remain Metal/Apple on this Windows host).

## Status

Completed.
