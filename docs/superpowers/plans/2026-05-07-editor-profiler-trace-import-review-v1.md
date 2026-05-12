# Editor Profiler Trace Import Review v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `editor-profiler-trace-import-review-v1`

**Goal:** Add a safe structured review path for pasted Chrome Trace Event JSON so the editor Profiler can inspect trace files exported by `mirakana::export_diagnostics_trace_json` without adding file import, flame graphs, or third-party telemetry tooling.

**Architecture:** `MK_core` owns the trace format and will expose a dependency-free structured review API for the supported Trace Event JSON subset. `MK_editor_core` adapts that review into retained `profiler.trace_import` rows, and the optional Dear ImGui shell provides a paste-and-review UI without reading files or executing tools.

**Tech Stack:** C++23, `MK_core`, `MK_editor_core`, `MK_editor`, Dear ImGui, `MK_ui`, CMake/CTest and PowerShell 7 `tools/` validation.

---

## Goal

Close the next narrow profiler productization gap:

- review a pasted Trace Event JSON payload using a structured parser for the supported subset;
- report payload bytes, total trace events, metadata events, instant diagnostic events, counter events, profile events, and unknown phase counts;
- expose retained `profiler.trace_import` rows and diagnostics from `editor/core`;
- render a visible `Trace JSON Review` paste buffer plus `Review Trace JSON` action in `MK_editor`;
- keep file-open dialogs, project file import, arbitrary JSON conversion, telemetry upload, crash-report backends, GPU/backend timestamps, allocator diagnostics, native handles, and production flame graphs out of scope.

## Context

- `MK_core` already exports deterministic Chrome Trace Event JSON through `mirakana::export_diagnostics_trace_json`.
- `MK_editor_core` already exposes trace export, project-relative trace file save, and telemetry handoff rows.
- The master plan still lists trace import follow-ups. This slice handles only safe review/counting for pasted JSON and does not reconstruct `DiagnosticCapture`.

## Constraints

- Keep `engine/core` and `editor/core` standard-library-only and GUI-independent.
- Do not introduce a third-party JSON dependency in this slice.
- Do not use substring counting for event classification; parse JSON tokens and object fields structurally.
- Accept only a top-level object with a `traceEvents` array. Return deterministic diagnostics for empty input, malformed JSON, missing `traceEvents`, or non-object trace event entries.
- Do not read from disk, write files, upload telemetry, launch external tools, or expose native handles.

## Done When

- A RED `MK_core_tests` test fails first on the missing trace review surface.
- `mirakana::review_diagnostics_trace_json` validates the supported top-level shape and classifies `ph` values `M`, `i`, `C`, `X`, and unknown phases.
- A RED/GREEN `MK_editor_core_tests` test covers `EditorProfilerTraceImportReviewModel`, retained `profiler.trace_import` ids, and malformed/empty diagnostics.
- `MK_editor` Profiler panel exposes a paste buffer and `Review Trace JSON` action, then renders the latest review rows.
- Docs, manifest, plan registry, editor skills, and static integration checks describe the narrow trace import review contract and remaining unsupported work.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Core Trace Review Test

**Files:**
- Modify: `tests/unit/core_tests.cpp`

- [x] Add `MK_TEST("diagnostics trace import review classifies trace event json")`.
- [x] Use `mirakana::export_diagnostics_trace_json` with metadata enabled for a capture containing one diagnostic event, one counter, and one profile.
- [x] Call `mirakana::review_diagnostics_trace_json(json)`.
- [x] Assert:
  - `review.valid`;
  - `review.payload_bytes == json.size()`;
  - `review.trace_event_count == 5` including two metadata rows;
  - `review.metadata_event_count == 2`;
  - `review.instant_event_count == 1`;
  - `review.counter_event_count == 1`;
  - `review.profile_event_count == 1`;
  - `review.unknown_event_count == 0`;
  - `review.diagnostics.empty()`.
- [x] Add malformed/unsupported shape assertions for empty input, missing `traceEvents`, invalid JSON, and a non-object event row.
- [x] Run `cmake --build --preset dev --target MK_core_tests`.
- [x] Record the expected RED failure in Validation Evidence.

### Task 2: Core Structured Trace Review API

**Files:**
- Modify: `engine/core/include/mirakana/core/diagnostics.hpp`
- Modify: `engine/core/src/diagnostics.cpp`

- [x] Add `DiagnosticsTraceImportReview` with validity, payload byte count, event counts, and diagnostics.
- [x] Declare `review_diagnostics_trace_json(std::string_view json)`.
- [x] Implement a small internal JSON token reader that can parse strings, numbers, literals, arrays, and objects enough to skip unsupported values safely.
- [x] Require top-level object and `traceEvents` array.
- [x] For each trace event object, read the `ph` string field and classify `M`, `i`, `C`, `X`, and unknown phases.
- [x] Return deterministic diagnostics for empty input, malformed JSON, missing `traceEvents`, and non-object trace event entries.
- [x] Run focused build/test and record GREEN evidence.

### Task 3: Editor-Core Trace Import Review Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/profiler.hpp`
- Modify: `editor/core/src/profiler.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `EditorProfilerTraceImportReviewModel` with `valid`, `status_label`, `payload_bytes`, `rows`, and `diagnostics`.
- [x] Declare `make_editor_profiler_trace_import_review_model(std::string_view payload)`.
- [x] Populate rows for status, format, payload bytes, events, metadata, instant events, counters, profiles, and unknown events.
- [x] Add `trace_import` to `EditorProfilerPanelModel` with a default empty review.
- [x] Emit retained `profiler.trace_import` rows and `profiler.trace_import.diagnostics.N`.
- [x] Add `MK_TEST("editor profiler trace import review model reports pasted trace json")`.
- [x] Run focused build/test and record GREEN evidence.

### Task 4: Visible Profiler Trace Review UI

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add a fixed-size paste buffer and retained status/model fields for trace review.
- [x] Render `Trace JSON Review` as an input area plus `Review Trace JSON` button.
- [x] On review, call `make_editor_profiler_trace_import_review_model` with the buffer contents.
- [x] Render the latest review rows and diagnostics.
- [x] Do not add file-open dialogs, file reads, file writes, upload buttons, or flame graph UI.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 5: Docs, Manifest, And Static Checks

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

- [x] Record `Editor Profiler Trace Import Review v1`, `DiagnosticsTraceImportReview`, `review_diagnostics_trace_json`, `EditorProfilerTraceImportReviewModel`, `make_editor_profiler_trace_import_review_model`, retained `profiler.trace_import` ids, and visible `Review Trace JSON`.
- [x] State that file import/open dialogs, arbitrary JSON conversion, flame graphs, telemetry SDK/upload execution, crash-report backends, GPU/backend timestamps, allocator diagnostics, and native handles remain unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 6: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused core build/test | PASS | `cmake --build --preset dev --target MK_core_tests` failed before implementation on missing `mirakana::review_diagnostics_trace_json`, as expected. |
| Focused core/editor tests | PASS | `cmake --build --preset dev --target MK_core_tests`, `cmake --build --preset dev --target MK_editor_core_tests`, and `ctest --preset dev --output-on-failure -R "MK_core_tests|MK_editor_core_tests"` passed after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Optional Dear ImGui editor build and GUI test lane passed after visible Profiler review UI wiring. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `tools/check-ai-integration.ps1` records the trace import review contract across core, editor, docs, manifest, plans, and skills. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Existing unsupported gap count/status audit passed with the updated master plan wording. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial run reported `editor/core/src/profiler.cpp` needed clang-format; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied it, and rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passed after adding the core trace review and editor trace import review surfaces. |
| `git diff --check` | PASS | Whitespace check passed; Git reported only existing LF-to-CRLF working-copy warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed, including static checks, toolchain diagnostics, API boundary check, tidy check, dev build, and 29/29 CTest tests. Apple/Metal lanes remained diagnostic/host-gated as expected on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev preset configure/build completed after validation. |
| Final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Commit-prep gate passed after full validation and dev build. |
| Slice-closing commit | PASS | Commit staged only this slice's trace import review files and left unrelated dirty guidance files unstaged. |
