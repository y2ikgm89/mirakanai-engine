# Play-In-Editor Gameplay Driver v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the Play-In-Editor editor-core contract so a reviewed gameplay simulation driver can run against the isolated simulation scene without mutating the authored source scene.

**Architecture:** Build on `EditorPlaySession` in `mirakana_editor_core`. The session owns the simulation scene copy, while a caller-supplied `IEditorPlaySessionDriver` receives begin/tick/end callbacks with explicit tick context and may mutate only the simulation scene. The editor-core model must not load game binaries, run package scripts, invoke validation recipes, depend on SDL3/Dear ImGui, create renderer/RHI resources, or expose native handles.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_scene`, `SceneAuthoringDocument`, `mirakana_editor_core_tests`, manifest/docs/static checks.

---

## Context

- Play-In-Editor Session Isolation v1 added `EditorPlaySession`, state/status labels, reports, isolated simulation scene copies, source-scene edit blocking, and manual tick counting.
- The master plan still lists gameplay execution inside Play-In-Editor as unsupported.
- A narrow gameplay-driver callback contract can prove editor-core execution semantics without coupling the editor to runtime hosts, SDL3, Dear ImGui, package scripts, renderer/RHI backends, or dynamically loaded game modules.

## Constraints

- Keep `editor/core` GUI-independent and default-preset buildable.
- Keep gameplay driver callbacks explicit and caller-owned; editor core must not own dynamic module loading, package script execution, shell commands, validation recipe execution, renderer/RHI uploads, hot reload, or package streaming.
- The source `SceneAuthoringDocument` must remain unchanged by gameplay callbacks.
- Driver `on_play_end` must run before the simulation scene is discarded.
- Add tests before implementation.

## Done When

- `tests/unit/editor_core_tests.cpp` proves driver begin/tick/end lifecycle, tick context, simulation-scene mutation, source-scene isolation, pause rejection, invalid delta rejection, and driver detach after stop.
- `editor/core/include/mirakana/editor/play_in_editor.hpp` and `.cpp` expose `IEditorPlaySessionDriver`, `EditorPlaySessionTickContext`, driver-aware begin/tick behavior, and driver report fields.
- Docs, plan registry, master plan, and `engine/agent/manifest.json` describe gameplay-driver execution as a narrow editor-core contract while keeping full Play-In-Editor/game-module/runtime-host/editor productization claims unsupported.
- Relevant validation passes, including `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, focused build/test, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `git diff --check`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- The slice closes with a validated commit checkpoint after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passes, staging only files owned by this slice.

## Commit Policy

- Use one slice-closing commit for this narrow plan after all code, docs, manifest, and validation evidence are complete.
- Do not commit RED-test or otherwise known-broken intermediate states.
- Keep unrelated user changes out of the commit.

## Tasks

### Task 1: RED Tests

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add a `RecordingEditorPlaySessionDriver` test helper that records begin/tick/end counts, captures `EditorPlaySessionTickContext`, and mutates the provided simulation `Scene`.
- [x] Add a test that starts `EditorPlaySession` with the driver, ticks with an explicit positive delta, verifies the driver mutates only the simulation scene, verifies the source document node count is unchanged, and verifies `stop()` calls driver end before discard.
- [x] Add a test that rejects driver tick while paused, rejects non-positive/non-finite deltas, reports the attached driver while active, and reports no driver after stop.
- [x] Run focused build/test and confirm failure because the driver types and tick overloads do not exist yet.

### Task 2: Editor-Core Driver Contract

**Files:**
- Modify: `editor/core/include/mirakana/editor/play_in_editor.hpp`
- Modify: `editor/core/src/play_in_editor.cpp`

- [x] Add `EditorPlaySessionTickContext` with `frame_index` and `delta_seconds`.
- [x] Add `IEditorPlaySessionDriver` with `on_play_begin(Scene&)`, `on_play_tick(Scene&, const EditorPlaySessionTickContext&)`, and `on_play_end(Scene&)`.
- [x] Extend `EditorPlaySessionActionStatus` with `rejected_invalid_delta` and update labels.
- [x] Add a driver-aware `begin(const SceneAuthoringDocument&, IEditorPlaySessionDriver&)` overload while keeping the existing driverless begin path.
- [x] Add `gameplay_driver_attached()`, `last_delta_seconds()`, and report fields.
- [x] Update `tick(double delta_seconds)` to validate finite positive deltas, dispatch driver ticks against the isolated simulation scene, then increment the frame count.
- [x] Update `stop()` to call `on_play_end` before discarding the simulation scene and detaching the driver.

### Task 3: Documentation And Manifest

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

- [x] Describe Play-In-Editor Gameplay Driver v1 as callback-driven gameplay simulation over the isolated scene.
- [x] Keep non-ready claims explicit: no dynamic game module loading, runtime host embedding, package scripts, validation recipe execution, renderer/RHI uploads, hot reload, package streaming, native handles, or full editor productization.
- [x] Add static checks that fail if driver code/docs/manifest markers drift.

### Task 4: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS | `Remove-Item Env:PATH -ErrorAction SilentlyContinue; cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation because `IEditorPlaySessionDriver`, `EditorPlaySessionTickContext`, driver begin/tick overloads, `rejected_invalid_delta`, and report fields did not exist. |
| Focused `mirakana_editor_core_tests` | PASS | `Remove-Item Env:PATH -ErrorAction SilentlyContinue; cmake --build --preset dev --target mirakana_editor_core_tests`; `Remove-Item Env:PATH -ErrorAction SilentlyContinue; ctest --preset dev --output-on-failure -R mirakana_editor_core_tests`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` after the initial format check reported clang-format drift, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Static manifest and AI integration checks include the driver markers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `editor-productization` remains `partly-ready`; dynamic game-module/runtime-host Play-In-Editor execution remains unsupported. |
| `git diff --check` | PASS | Exit code 0; CRLF warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed on Windows; 29/29 CTest tests passed. Metal/Apple checks remain diagnostic/host-gated on this host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default dev preset build passed after validation. |
| Slice-closing commit | PASS | Commit staged only the files owned by the Play-In-Editor isolation/driver slice and synchronized AI guidance. |
