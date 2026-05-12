# Play-In-Editor Visible Viewport Wiring v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wire the visible `mirakana_editor` Run/Viewport controls to `EditorPlaySession` so the editor shell displays and advances the isolated simulation scene while source-scene mutations stay blocked during play.

**Architecture:** Add a small GUI-independent controls model in `mirakana_editor_core` for Play/Pause/Resume/Stop availability and session reporting, then adapt the existing Dear ImGui shell to that model. Keep `editor/core` independent of Dear ImGui, SDL3, runtime hosts, dynamic game modules, package scripts, validation recipes, renderer/RHI uploads, hot reload, package streaming, and native handles.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_editor`, Dear ImGui adapter, `EditorPlaySession`, `SceneAuthoringDocument`, `mirakana_editor_core_tests`, `desktop-gui` build lane.

---

## Context

- Play-In-Editor Session Isolation v1 added `EditorPlaySession` as a GUI-independent source/simulation scene isolation model.
- Play-In-Editor Gameplay Driver v1 added caller-supplied driver callbacks and tick context over the isolated simulation scene.
- The current visible `mirakana_editor` shell has Run menu and Viewport toolbar controls, but they still use `ViewportState::play/pause/resume/stop` directly and render the source scene.
- The next narrow editor-productization slice should connect the visible shell to the reviewed play-session contract without claiming dynamic game-module/runtime-host Play-In-Editor execution.

## Constraints

- Add tests before production code.
- Keep the default `mirakana_editor_core` build GUI-free.
- Do not expose Dear ImGui, SDL3, renderer, RHI, OS, or dynamic module handles through editor-core contracts.
- Do not save, serialize, or mutate the isolated simulation scene through source-scene save/project paths.
- Block source-scene edit actions while a play session is active.
- Keep gameplay execution limited to existing caller-supplied `IEditorPlaySessionDriver`; this slice does not add dynamic game loading or runtime host embedding.

## Done When

- `mirakana_editor_core` exposes a deterministic `EditorPlaySessionControlsModel` with Play/Pause/Resume/Stop command rows, enabled flags, session report, and `viewport_uses_simulation_scene`.
- Tests prove controls are correct in edit, play, paused, stopped, and empty-source states.
- `mirakana_editor` Run menu and Viewport toolbar query the controls model, start/pause/resume/stop `EditorPlaySession`, tick it once per viewport frame while in play mode, and render the simulation scene while active.
- `mirakana_editor` source-scene authoring actions and undo/redo are rejected while `EditorPlaySession::source_scene_edits_blocked()` is true.
- Docs, registry, master plan, manifest, and static checks describe visible viewport wiring without claiming dynamic game-module/runtime-host Play-In-Editor execution.
- Relevant validation passes, including focused `mirakana_editor_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- The slice closes with a validated commit checkpoint after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passes, staging only files owned by this slice.

## Commit Policy

- Use one slice-closing commit after code, docs, manifest, static checks, GUI build, and validation evidence are complete.
- Do not commit RED-test or otherwise known-broken intermediate states.
- Keep unrelated user changes out of the commit.

## Efficiency Policy

- Use focused build/test loops while implementing: `mirakana_editor_core_tests` for editor-core behavior, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` only after the visible shell wiring compiles locally, and static checks only when their owned files change.
- Batch docs, manifest, skills, and static-check marker updates after the code behavior is GREEN.
- Run full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` once at the slice-closing gate unless a later edit invalidates that evidence.
- Keep validation evidence as command/result rows instead of long progress prose.

## Tasks

### Task 1: RED Tests For Controls Model

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add tests for `make_editor_play_session_controls_model` in edit state, proving Play is enabled, Pause/Resume/Stop are disabled, `viewport_uses_simulation_scene` is false, and the report state is `edit`.
- [x] Add tests for active play and paused states, proving enabled controls match the legal transitions and `viewport_uses_simulation_scene` is true while the session is active.
- [x] Add tests for stop and empty-source states, proving stop returns to edit controls and empty source disables Play.
- [x] Run focused build/test and confirm failure because `EditorPlaySessionControlsModel`, `EditorPlaySessionControlCommand`, and `make_editor_play_session_controls_model` do not exist yet.

### Task 2: Editor-Core Controls Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/play_in_editor.hpp`
- Modify: `editor/core/src/play_in_editor.cpp`

- [x] Add `EditorPlaySessionControlCommand` values `play`, `pause`, `resume`, and `stop`.
- [x] Add `EditorPlaySessionControlRow` with command, stable id, label, and enabled flag.
- [x] Add `EditorPlaySessionControlsModel` with `EditorPlaySessionReport`, command rows, and `viewport_uses_simulation_scene`.
- [x] Add `editor_play_session_control_command_id`, `editor_play_session_control_command_label`, and `make_editor_play_session_controls_model`.
- [x] Enable Play only when the source scene has at least one node and the session is inactive; enable Pause only in play; enable Resume only in paused; enable Stop whenever active.

### Task 3: Visible Editor Wiring

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add an `EditorPlaySession` member to the desktop editor shell.
- [x] Keep `active_scene()` as the source scene and add a separate viewport scene accessor that returns the simulation scene while the session is active.
- [x] Use `make_editor_play_session_controls_model` for the Run menu and Viewport toolbar enabled state.
- [x] Start, pause, resume, and stop the play session from existing Run commands, then mirror the accepted state into `ViewportState`.
- [x] Tick the play session once per viewport frame while the session is in play mode and mirror accepted ticks into `ViewportState::mark_simulation_tick`.
- [x] Render `viewport_scene()` instead of `active_scene()` in the viewport surface and overlay.
- [x] Reject source-scene undo/redo and `execute_scene_authoring_action` while `source_scene_edits_blocked()` is true.

### Task 4: Docs, Manifest, Static Checks

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

- [x] Describe Play-In-Editor Visible Viewport Wiring v1 as visible Run/Viewport controls over the existing play-session contract.
- [x] Keep non-ready claims explicit: no dynamic game module loading, runtime host embedding, package scripts, validation recipe execution, renderer/RHI uploads, hot reload, package streaming, public native handles, or full Unity/UE-like editor workflow.
- [x] Add static checks for controls model, visible shell wiring, docs, manifest, and AI guidance markers.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Failed as expected | Temporary detached worktree `C:\tmp\GameEngine-pie-red` with only `tests/unit/editor_core_tests.cpp` patch; `cmake --build --preset dev --target mirakana_editor_core_tests -- /m:1` failed on missing `EditorPlaySessionControlRow`, `EditorPlaySessionControlsModel`, `EditorPlaySessionControlCommand`, and `make_editor_play_session_controls_model`. |
| Focused `mirakana_editor_core_tests` | Pass | `cmake --build --preset dev --target mirakana_editor_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed 1/1. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | `format-check: ok` after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-readiness-audit-check: ok`; 11 non-ready gap rows remain audited. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Pass | Built `desktop-gui` and passed 46/46 CTest entries including `mirakana_editor_core_tests`, `mirakana_d3d12_rhi_tests`, and desktop runtime sample smokes. |
| `git diff --check` | Pass | No whitespace errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Exit 0. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Dev preset configured and built. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the Play-In-Editor Visible Viewport Wiring v1 files; leave unrelated pre-existing guidance changes unstaged. |
