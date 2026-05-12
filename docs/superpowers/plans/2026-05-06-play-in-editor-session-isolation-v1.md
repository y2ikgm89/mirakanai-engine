# Play-In-Editor Session Isolation v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a GUI-independent Play-In-Editor session isolation model so editor play mode runs against a mutable simulation scene copy and stopping play discards simulation edits while preserving the authored source scene.

**Architecture:** Keep the contract in `mirakana_editor_core` so the optional Dear ImGui shell can adapt it later without owning the semantics. The model snapshots a `SceneAuthoringDocument` source scene into an isolated simulation scene, exposes play/pause/resume/tick/stop rows, rejects source-scene edits while a session is active, and never executes runtime package scripts, shell commands, renderer/RHI handles, or game code.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_scene`, `mirakana::editor::ViewportState`, `mirakana_editor_core_tests`, repository static docs checks.

---

## Context

- The production master plan still lists `editor-productization` as planned and calls out `play-in-editor-isolation-v1`.
- `ViewportState` already tracks edit/play/paused mode and simulation frame counts, but the editor has no source-vs-simulation scene isolation contract.
- `SceneAuthoringDocument` already owns source-scene edits and undo/redo snapshots without GUI dependencies.

## Constraints

- Do not make `mirakana_editor_core` depend on SDL3, Dear ImGui, renderer/RHI backends, package scripts, or runtime game APIs.
- Do not expose native handles.
- Keep this slice narrow: isolated session state only, not full gameplay execution, physics/audio ticking, hot-reload, package streaming, or GUI wiring.
- Add tests before implementation.

## Done When

- `tests/unit/editor_core_tests.cpp` proves source scene isolation, session state transitions, simulation tick counting, and blocked source-edit claims.
- `editor/core/include/mirakana/editor/play_in_editor.hpp` and `.cpp` expose the GUI-independent model and are registered in `editor/CMakeLists.txt`.
- Docs, plan registry, master plan, and `engine/agent/manifest.json` describe the new narrow editor-productization evidence without claiming full editor productization.
- Relevant validation passes, including `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, focused build/test, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: RED Tests

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`
- Create: `editor/core/include/mirakana/editor/play_in_editor.hpp`

- [x] Add tests for starting a session from `SceneAuthoringDocument`, mutating only the simulation scene, pausing/resuming/ticking, stopping back to edit mode, and reporting that source-scene mutation is blocked while active.
- [x] Run focused build/test and confirm failure because the new header/model does not exist yet.

### Task 2: Editor-Core Model

**Files:**
- Create: `editor/core/include/mirakana/editor/play_in_editor.hpp`
- Create: `editor/core/src/play_in_editor.cpp`
- Modify: `editor/CMakeLists.txt`

- [x] Implement `EditorPlaySessionState`, `EditorPlaySessionActionStatus`, `EditorPlaySessionReport`, `EditorPlaySession`, and `make_editor_play_session_report`.
- [x] Keep the simulation scene as an internal copy and expose it through explicit accessors.
- [x] Reject begin when the source scene is empty and reject begin/pause/resume/tick/stop transitions that do not match the state machine.
- [x] Keep stop discard-only: source scene is never overwritten by simulation edits.

### Task 3: Documentation And Manifest

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- No change needed: `tools/check-production-readiness-audit.ps1` already allows the existing `partly-ready` status vocabulary.

- [x] Update editor-productization status vocabulary and manifest notes to reflect the narrow implemented play-session isolation model.
- [x] Keep non-ready claims explicit: no full editor productization, game-code execution, package scripts, renderer/RHI handle exposure, or renderer quality.
- [x] Add static checks that fail if the model/docs/manifest markers drift.

### Task 4: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS | `Remove-Item Env:PATH -ErrorAction SilentlyContinue; cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation because `mirakana/editor/play_in_editor.hpp` did not exist. |
| Focused `mirakana_editor_core_tests` | PASS | `Remove-Item Env:PATH -ErrorAction SilentlyContinue; cmake --build --preset dev --target mirakana_editor_core_tests`; `Remove-Item Env:PATH -ErrorAction SilentlyContinue; ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial run caught clang-format wrapping in `play_in_editor.hpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` corrected it, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public editor-core header registration remains inside allowed API boundaries. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Static AI integration checks include play-in-editor isolation markers in code, docs, and manifest. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `editor-productization` is audited as `partly-ready` with remaining unsupported claims. |
| `git diff --check` | PASS | No whitespace errors; Git reported only CRLF normalization warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation completed with 29/29 CTest tests passing; Apple/Metal host checks remain diagnostic host-gated on this Windows host. |
