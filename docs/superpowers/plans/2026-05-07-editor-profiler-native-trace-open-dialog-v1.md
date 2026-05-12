# Editor Profiler Native Trace Open Dialog v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development for the behavior change and keep this plan updated as evidence lands.

**Plan ID:** `editor-profiler-native-trace-open-dialog-v1`

**Goal:** Add a narrow native file-open dialog path for selecting Profiler Trace Event JSON files and feeding safe project-relative selections into the existing trace import review workflow.

**Architecture:** `editor/core` builds a first-party `mirakana::FileDialogRequest` and reviews `mirakana::FileDialogResult` rows without SDL, Dear ImGui, OS handles, or file reads. The optional `MK_editor` shell owns `mirakana::SdlFileDialogService`, polls the async result, maps accepted selections under the current project store to safe `.json` paths, then calls `import_editor_profiler_trace_json`.

**Tech Stack:** C++23, `MK_platform` file dialog contracts, `MK_editor_core`, `MK_editor`, SDL3, Dear ImGui, CMake/CTest and PowerShell 7 `tools/` validation.

---

## Goal

Close the next narrow Profiler observability UX gap:

- build a deterministic Trace JSON open dialog request from editor core;
- review accepted/canceled/failed async file dialog results into retained rows and diagnostics;
- expose a visible `Browse Trace JSON` action in the Profiler panel;
- poll the SDL-backed native dialog from `MK_editor`;
- convert selected files inside the project store into safe project-relative `.json` import paths;
- reuse `import_editor_profiler_trace_json` for actual review/import.

## Context

- `Editor Profiler Trace File Import Review v1` already reads safe project-relative `.json` paths through `ITextStore`.
- `MK_platform` already exposes first-party asynchronous file dialog contracts and `MK_platform_sdl3` maps them to SDL3 native dialogs.
- The master plan still lists native file-open dialogs as a follow-up; this slice handles only the Profiler Trace JSON open/import path.

## Constraints

- Keep `editor/core` GUI-independent and free of SDL3, Dear ImGui, native handles, and filesystem reads.
- Use `mirakana::FileDialogRequest`, `mirakana::IFileDialogService`, and `mirakana::SdlFileDialogService`; do not call SDL dialog APIs directly from random panel code.
- Accept only one `.json` trace selection and import only after the GUI converts it to a safe project-relative store path.
- Do not add arbitrary JSON conversion, `DiagnosticCapture` reconstruction, telemetry upload, crash-report backends, native handle exposure, or flame graph UI.

## Done When

- A RED `MK_editor_core_tests` test fails first on the missing dialog request/result surface.
- `editor/core` exposes a deterministic trace-open dialog request and retained dialog review rows.
- `MK_editor` owns/polls the SDL file dialog service and wires `Browse Trace JSON` into the Profiler panel.
- Accepted in-project `.json` selections update the Trace Import Path and call the existing file import review path.
- Docs, manifest, plan registry, editor skills, and static integration checks describe the narrow native open dialog contract.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Editor-Core Dialog Request/Result Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor profiler native trace open dialog reviews selection")`.
- [x] Assert `make_editor_profiler_trace_open_dialog_request()` returns an open-file request titled `Open Trace JSON`, with a `json` filter, default `diagnostics`, and `allow_many == false`.
- [x] Assert accepted one-file `.json` results produce accepted status, selected path, and retained rows.
- [x] Assert canceled and failed results produce deterministic status/diagnostics.
- [x] Assert accepted zero-file, multi-file, and non-`.json` results are blocked before import.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests` and record the expected RED failure.

### Task 2: Editor-Core Dialog Model/API

**Files:**
- Modify: `editor/core/include/mirakana/editor/profiler.hpp`
- Modify: `editor/core/src/profiler.cpp`

- [x] Add `EditorProfilerTraceOpenDialogModel` with accepted flag, status label, selected path, rows, and diagnostics.
- [x] Declare and implement `make_editor_profiler_trace_open_dialog_request(std::string_view default_location = "diagnostics")`.
- [x] Declare and implement `make_editor_profiler_trace_open_dialog_model(const mirakana::FileDialogResult& result)`.
- [x] Add retained `profiler.trace_open_dialog` rows and diagnostics to `EditorProfilerPanelModel` / `make_profiler_ui_model`.
- [x] Run focused build/test and record GREEN evidence.

### Task 3: Visible Profiler Native Open Dialog UI

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Include and construct `mirakana::SdlFileDialogService` in `main`.
- [x] Pass `mirakana::IFileDialogService*` into `EditorState`.
- [x] Add pending dialog id and retained dialog model fields.
- [x] Render `Browse Trace JSON` beside `Import Trace JSON`.
- [x] Poll pending dialog results, map accepted in-project selections to generic project-relative paths, update `Trace Import Path`, and call `import_editor_profiler_trace_json`.
- [x] Surface canceled, failed, outside-project, and unsafe selection diagnostics without importing.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, And Static Checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`

- [x] Record `Editor Profiler Native Trace Open Dialog v1`, `EditorProfilerTraceOpenDialogModel`, `make_editor_profiler_trace_open_dialog_request`, `make_editor_profiler_trace_open_dialog_model`, retained `profiler.trace_open_dialog` ids, and visible `Browse Trace JSON`.
- [x] State that arbitrary JSON conversion, capture reconstruction, flame graphs, telemetry SDK/upload execution, crash-report backends, GPU/backend timestamps, allocator diagnostics, and native handles remain unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused editor build/test | PASS | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation on missing `make_editor_profiler_trace_open_dialog_request`, `make_editor_profiler_trace_open_dialog_model`, `EditorProfilerTraceOpenDialogModel`, and `trace_open_dialog`, as expected. |
| Focused editor tests | PASS | `cmake --build --preset dev --target MK_editor_core_tests` rebuilt `profiler.cpp` and `editor_core_tests.cpp`; `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed 1/1. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Reconfigured/built `desktop-gui`; `MK_editor` rebuilt with `SdlFileDialogService`; desktop-gui CTest passed 46/46. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `tools/check-ai-integration.ps1` passed after manifest/docs/skills/static needles for native trace open dialog. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Unsupported gap audit passed with 11 tracked gaps and no ready-claim contradiction. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial run found clang-format deltas in `editor/src/main.cpp` and `tests/unit/editor_core_tests.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` was applied and the rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passed after adding the editor-core trace-open dialog API. |
| `git diff --check` | PASS | Whitespace check passed; Git reported only existing LF/CRLF conversion warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed with default CTest 29/29; Metal/Apple diagnostics remained host-gated/diagnostic-only on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default dev build passed. |
| Final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Commit-prep gate passed after formatting, docs, manifest, and static-check updates. |
| Slice-closing commit | Pending | Stage only this slice's files and leave unrelated dirty guidance files unstaged. |
