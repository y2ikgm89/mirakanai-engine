# Editor Project Native Dialog v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add reviewed native `.geproject` open/save-as dialog models and visible `MK_editor` wiring for project bundles without moving SDL3, native handles, or filesystem path conversion into `editor/core`.

**Architecture:** Extend GUI-independent project IO with a small project file-dialog request/review model that mirrors the existing Profiler, Scene, and Prefab Variant dialog contracts. The optional `MK_editor` shell owns `mirakana::IFileDialogService` / `mirakana::SdlFileDialogService`, converts accepted in-store `.geproject` selections to safe store-relative paths, infers the companion workspace/startup-scene paths, and reuses existing `load_project_bundle` / `save_project_bundle`.

**Tech Stack:** C++23, `MK_editor_core`, `MK_platform` file dialog contracts, `MK_editor` Dear ImGui adapter, `MK_editor_core_tests`, `desktop-gui` validation lane.

---

## Goal

Close a narrow editor-productization gap:

- Add `.geproject` native open/save-as request helpers.
- Review native dialog results into deterministic retained rows under `project_file_dialog.open` and `project_file_dialog.save`.
- Let the visible File menu open a project bundle from an accepted in-store `.geproject` selection.
- Let the visible File menu save the current project bundle to an accepted in-store `.geproject` selection, with the companion workspace file inferred from the selected project file stem.
- Keep project document parsing, workspace IO, scene IO, and path conversion on existing reviewed paths.

## Context

- `ProjectDocument`, `ProjectBundlePaths`, `load_project_bundle`, and `save_project_bundle` already own deterministic project bundle IO.
- The visible editor already has native file-dialog infrastructure for Profiler Trace JSON, Scene `.scene`, and Prefab Variant `.prefabvariant`.
- The master plan still lists broader editor native save/open dialogs outside Profiler, Scene, and Prefab Variant as follow-up editor productization work.

## Constraints

- Do not add SDL3, Dear ImGui, native handles, or absolute-path conversion to `editor/core`.
- Do not allow project selections outside the `FileTextStore` root.
- Do not create broad project migration, package execution, validation execution, dynamic runtime-host Play-In-Editor, or manifest editing behavior.
- Keep active Play-In-Editor stopped when replacing the loaded project through the same scene replacement path.
- Update Codex and Claude editor guidance together.

## Done When

- RED `MK_editor_core_tests` proves project native file dialog symbols are missing.
- `MK_editor_core` exposes `make_project_open_dialog_request`, `make_project_save_dialog_request`, `make_project_open_dialog_model`, `make_project_save_dialog_model`, and `make_project_file_dialog_ui_model`.
- The visible editor File menu includes `Open Project...` and `Save Project As...` native dialog paths; accepted `.geproject` selections load/save project bundles through existing helpers.
- Project Settings renders retained open/save dialog rows for operator review.
- Docs, master plan, registry, manifest, editor skills, and AI integration checks record the boundary.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor project native file dialogs review results and retained rows")`.
- [x] Assert open/save requests use `FileDialogKind::open_file` / `save_file`, title `Open Project` / `Save Project`, one `Project` filter with pattern `geproject`, no multi-select, and labels `Open` / `Save`.
- [x] Assert accepted/canceled/failed/empty/multi/wrong-extension results produce deterministic `EditorProjectFileDialogModel` status labels, diagnostics, rows, and retained `project_file_dialog.<mode>.*` UI ids.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests` and confirm it fails before implementation because project file-dialog symbols do not exist.

### Task 2: Editor-Core Project Dialog Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/project.hpp`
- Modify: `editor/core/src/project.cpp`

- [x] Add `EditorProjectFileDialogMode`, `EditorProjectFileDialogRow`, and `EditorProjectFileDialogModel`.
- [x] Add request helpers for open/save `.geproject` dialogs.
- [x] Add result review helpers that reject empty selections, multi-selection, and non-`.geproject` paths.
- [x] Add retained `MK_ui` output under `project_file_dialog.open` and `project_file_dialog.save`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Visible Editor Wiring

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add project open/save dialog state, polling, and visible Project Settings review tables.
- [x] Change File menu command labels to `Open Project...` and add `Save Project As...`.
- [x] Convert accepted native `.geproject` selections to safe store-relative paths before IO.
- [x] For open, infer companion workspace and startup-scene paths, call `load_project_bundle`, reset project/workspace/settings/state, and replace the scene through the existing path.
- [x] For save-as, infer the companion workspace path from the selected `.geproject` stem, reuse the existing startup scene path, update `project_paths_`, and call `save_project_bundle`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Guidance, Static Checks

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

- [x] Record Editor Project Native Dialog v1, retained `project_file_dialog` ids, and visible File menu wiring.
- [x] Keep dynamic game-module/runtime-host Play-In-Editor execution, package scripts, validation execution, arbitrary manifest edits, native handles, and broad editor productization unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Passed | `cmake --build --preset dev --target MK_editor_core_tests` failed as expected before implementation because project file-dialog symbols were missing. |
| Focused `MK_editor_core_tests` | Passed | `cmake --build --preset dev --target MK_editor_core_tests`; `ctest --preset dev --output-on-failure -R MK_editor_core_tests`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed | Built `MK_editor`; desktop-gui CTest passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Added project native dialog manifest/static guidance; rerun passed after recording `mirakana::SdlFileDialogService` in this plan. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Existing unsupported gap classification remains intentional; `editor-productization` stays partly-ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Initial failure in `editor/core/src/project.cpp` was fixed by `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`; rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public API boundary policy accepted the new editor-core dialog helpers. |
| `git diff --check` | Passed | Whitespace check clean; Git reported only CRLF normalization warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full repository validation passed, including 29/29 CTest tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Full dev build passed after validation. |
| Slice-closing commit | Passed | Staged only Editor Project Native Dialog v1 files and created `feat: add project native dialogs`. |
