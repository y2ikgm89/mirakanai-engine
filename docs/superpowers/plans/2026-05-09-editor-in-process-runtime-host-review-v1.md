# Editor In-Process Runtime Host Review Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed editor-core in-process runtime-host handoff that can start an isolated Play-In-Editor session with a caller-supplied linked gameplay driver.

**Architecture:** Keep dynamic module loading out of scope. `mirakana_editor_core` will review an already-linked `IEditorPlaySessionDriver` capability, expose retained `play_in_editor.in_process_runtime_host` rows, and start only the existing isolated `EditorPlaySession` path when the reviewed model is ready. The optional `mirakana_editor` shell may render the review rows, but it must not load game DLLs, execute package scripts, expose renderer/RHI handles, or run runtime hosts from editor core.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_editor`, first-party `mirakana_ui` retained models, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Plan ID:** `editor-in-process-runtime-host-review-v1`

**Status:** Completed.

---

## Context

- Active master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Selected gap: `editor-productization`.
- Previous slice: `docs/superpowers/plans/2026-05-09-editor-prefab-instance-stale-node-refresh-resolution-v1.md`.
- Existing editor work supports isolated Play-In-Editor sessions through `EditorPlaySession` and caller-supplied `IEditorPlaySessionDriver`, plus reviewed external runtime-host process launch through `EditorRuntimeHostPlaytestLaunchModel`.
- This slice narrows the in-process runtime-host gap by adding a reviewed, explicit, already-linked gameplay-driver handoff. It does not add dynamic game-module loading, hot reload, in-process desktop runtime embedding, package script execution, validation recipe execution, renderer/RHI uploads, renderer/RHI handle exposure, package streaming, or native handles.

## Files

- Modify: `editor/core/include/mirakana/editor/play_in_editor.hpp`
  - Add in-process runtime-host review status, descriptor, model, begin result, UI model, and reviewed begin helper declarations.
- Modify: `editor/core/src/play_in_editor.cpp`
  - Build deterministic review rows for linked gameplay driver availability, unsupported claims, active session state, and retained UI output.
  - Start an `EditorPlaySession` with a caller-supplied `IEditorPlaySessionDriver` only when the reviewed model is ready.
- Modify: `tests/unit/editor_core_tests.cpp`
  - Add failing coverage for the reviewed in-process runtime-host handoff, retained UI rows, blocked unsupported claims, and no-driver diagnostics.
- Modify: `editor/src/main.cpp`
  - Render a visible read-only in-process runtime-host review section that reports no linked driver in the stock editor shell.
- Modify: `docs/editor.md`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`
  - Document the narrowed capability and unsupported boundaries.
- Modify: `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`
  - Keep Codex and Claude editor guidance aligned.
- Modify: `engine/agent/manifest.json`
  - Update `currentActivePlan`, `recommendedNextPlan`, and editor-productization capability text.
- Modify: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`
  - Add sentinel checks for the reviewed in-process runtime-host contract.
- Modify: `docs/superpowers/plans/README.md`
  - Track this plan as the active slice, then latest completed evidence at closeout.
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update current verdict and selected-gap evidence after validation.

## Done When

- `mirakana_editor_core` exposes `EditorInProcessRuntimeHostDesc`, `EditorInProcessRuntimeHostModel`, `EditorInProcessRuntimeHostBeginResult`, `make_editor_in_process_runtime_host_model`, `begin_editor_in_process_runtime_host_session`, and `make_editor_in_process_runtime_host_ui_model`.
- The model is ready only when the source scene is non-empty, no session is already active, and the caller reports an already-linked gameplay driver.
- The reviewed begin helper starts the existing isolated `EditorPlaySession` with a caller-supplied `IEditorPlaySessionDriver`, and normal tick/stop lifecycle continues to call driver callbacks.
- Unsupported claims for dynamic game-module loading, external process execution, package scripts, validation recipes, hot reload, package streaming, renderer/RHI uploads, renderer/RHI handles, and editor-core execution are blocked with deterministic diagnostics.
- The visible editor shows the retained in-process runtime-host review rows but does not invent or load a stock dynamic driver.
- Focused `mirakana_editor_core_tests` pass.
- GUI build passes because `editor/src/main.cpp` changes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `git diff --check -- <touched files>`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Tasks

### Task 1: Add failing editor-core coverage

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add a test named `editor in process runtime host review starts linked gameplay driver sessions`.
- [x] Assert ready review rows require a non-empty source scene and `linked_gameplay_driver_available=true`.
- [x] Assert retained `play_in_editor.in_process_runtime_host` UI rows exist for status, driver availability, blocked claims, and diagnostics.
- [x] Assert `begin_editor_in_process_runtime_host_session` starts an isolated `EditorPlaySession` with the supplied `IEditorPlaySessionDriver`, and tick/stop call the driver callbacks through the existing session lifecycle.
- [x] Assert dynamic module loading, renderer/RHI handle exposure, and missing linked driver requests are blocked.
- [x] Build `mirakana_editor_core_tests` and verify the test fails before implementation.

Run:

```powershell
cmake --build --preset dev --target mirakana_editor_core_tests
```

Expected before implementation: compile failure for the new in-process runtime-host types and functions.

### Task 2: Implement editor-core review and begin helper

**Files:**
- Modify: `editor/core/include/mirakana/editor/play_in_editor.hpp`
- Modify: `editor/core/src/play_in_editor.cpp`

- [x] Add `EditorInProcessRuntimeHostStatus`.
- [x] Add `EditorInProcessRuntimeHostDesc` with id, label, linked driver availability, and unsupported-claim flags.
- [x] Add `EditorInProcessRuntimeHostModel` with status, status label, `can_begin`, `can_tick`, `can_stop`, source scene and simulation state, blocked rows, unsupported claims, and diagnostics.
- [x] Add `EditorInProcessRuntimeHostBeginResult`.
- [x] Implement `make_editor_in_process_runtime_host_model` over an `EditorPlaySession`, `SceneAuthoringDocument`, and descriptor.
- [x] Implement `begin_editor_in_process_runtime_host_session` so it calls `EditorPlaySession::begin(document, driver)` only when the reviewed model can begin.
- [x] Reuse the existing `EditorPlaySession` lifecycle; do not add runtime-host execution inside `editor/core`.

### Task 3: Add retained UI rows and visible shell diagnostics

**Files:**
- Modify: `editor/core/src/play_in_editor.cpp`
- Modify: `editor/src/main.cpp`

- [x] Add `make_editor_in_process_runtime_host_ui_model` with retained `play_in_editor.in_process_runtime_host` ids.
- [x] Add a visible `In-Process Runtime Host` section near existing Play-In-Editor / runtime-host controls.
- [x] In the stock visible editor, report `linked_gameplay_driver_available=false` until a future host supplies a linked gameplay driver.
- [x] Keep visible mutation behind reviewed model readiness; do not load modules or execute package scripts.

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

- [x] Document in-process runtime-host review as an explicit linked-driver handoff.
- [x] Keep unsupported claims explicit: no dynamic game-module loading, hot reload, in-process desktop runtime embedding, package/validation execution, renderer/RHI uploads, renderer/RHI handles, native handles, or package streaming.
- [x] Add sentinels for the new model names, retained row ids, docs, skills, visible shell text, and manifest text.

### Task 5: Validate and close the slice

**Files:**
- Modify: this plan file.

- [x] Run focused editor-core build and tests.
- [x] Run GUI build with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run relevant static checks.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact validation evidence in this plan.
- [x] Set this plan status to `Completed`, move `currentActivePlan` back to the master plan, and update the plan registry latest completed slice.

## Validation Evidence

- RED: `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation with missing `EditorInProcessRuntimeHost*`, `make_editor_in_process_runtime_host_model`, `begin_editor_in_process_runtime_host_session`, and `make_editor_in_process_runtime_host_ui_model` symbols.
- GREEN focused build: `cmake --build --preset dev --target mirakana_editor_core_tests` passed.
- GREEN focused test: `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed, 1/1 test.
- GUI build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` passed, including `mirakana_editor` rebuild and 46/46 desktop GUI tests.
- API/static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- Diff hygiene: `git diff --check -- <touched files>` exited 0 with only existing LF-to-CRLF Git working-copy warnings.
- Slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; CTest reported 29/29 tests passed, `production-readiness-audit-check` still reports `unsupported_gaps=11` with `editor-productization` partly-ready, and Apple/Metal lanes remain host-gated on this Windows host.

## Status

Completed.
