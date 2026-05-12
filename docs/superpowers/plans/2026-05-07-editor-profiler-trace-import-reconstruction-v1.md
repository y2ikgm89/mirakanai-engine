# Editor Profiler Trace Import Reconstruction v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `editor-profiler-trace-import-reconstruction-v1`
**Status:** Completed
**Goal:** Reconstruct a narrow `mirakana::DiagnosticCapture` from GameEngine-exported Chrome Trace Event JSON so the editor Profiler import path can hand callers reviewed capture rows instead of review counts only.
**Architecture:** Keep JSON parsing and reconstruction in `mirakana_core`; keep file IO, retained rows, and GUI-independent Profiler models in `mirakana_editor_core`; keep SDL3/Dear ImGui/native handles out of core models. The importer accepts only the first-party exported subset: metadata (`M`) is skipped, instant diagnostics (`i`), counters (`C`), and duration profiles (`X`) are reconstructed when their exported fields are present and finite.
**Tech Stack:** C++23, `mirakana_core`, `mirakana_editor_core`, existing `MemoryTextStore`, existing retained `mirakana_ui` Profiler model, PowerShell validation wrappers.

---

## Context

- Parent roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Recent completed slices added Profiler Trace JSON review, project-relative file import, native open dialog, and native save dialog.
- The current gap text still excludes capture reconstruction. This slice narrows that unsupported claim only for first-party exported Trace Event JSON.

## Constraints

- Do not add third-party JSON, trace, telemetry, crash-report, or UI dependencies.
- Do not claim arbitrary Chrome trace conversion, flame graphs, native dump import, telemetry upload, or capture-tool execution.
- Do not expose SDL3, Dear ImGui, file dialog handles, OS handles, or RHI/backend handles through `mirakana_core` or `mirakana_editor_core`.
- Preserve existing review diagnostics and counts for unsupported or malformed JSON.
- Treat non-metadata trace events outside the first-party exported subset as not reconstructable.

## Done When

- `mirakana::import_diagnostics_trace_json` reconstructs events, counters, and profile samples from `mirakana::export_diagnostics_trace_json` output and reports deterministic diagnostics for unsupported reconstruction fields.
- `EditorProfilerTraceImportReviewModel`, `EditorProfilerTraceFileImportResult`, `mirakana::editor::make_editor_profiler_trace_import_review_model`, and `import_editor_profiler_trace_json` expose reconstructed capture counts and capture data through retained Profiler rows.
- `make_profiler_ui_model` exposes retained `profiler.trace_import.reconstructed_*` rows.
- Current docs, manifest, and static checks describe the narrowed support boundary honestly.
- Validation evidence is recorded below.

## Tasks

- [x] Add RED `mirakana_core` test for exported Trace Event JSON reconstruction into `DiagnosticCapture`.
- [x] Add RED `mirakana_editor_core` test for reconstructed Profiler import rows and file-import capture data.
- [x] Implement `DiagnosticsTraceImportResult` and `import_diagnostics_trace_json` in `engine/core`.
- [x] Extend Profiler import models and retained UI output in `editor/core`.
- [x] Update docs, master plan, plan registry, manifest, and AI integration static checks.
- [x] Run focused build/tests, formatting/static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_core_tests mirakana_editor_core_tests` | Failed as expected | RED check failed on missing `mirakana::import_diagnostics_trace_json` before implementation. |
| `cmake --build --preset dev --target mirakana_core_tests mirakana_editor_core_tests` | Passed | Re-run after implementation and formatting. |
| `ctest --preset dev --output-on-failure -R "mirakana_core_tests\|mirakana_editor_core_tests"` | Passed | 2/2 tests passed after fixing retained row id conflicts. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Unsupported gap/status audit passed with `unsupported_gaps=11`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Manifest/docs/static sync passed after adding reconstruction needles. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Initial check found C++ test formatting; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting, then re-check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public `mirakana_core` / `mirakana_editor_core` headers changed. |
| `git diff --check` | Passed | Line-ending warnings only; no whitespace errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed; CTest reported 29/29 tests passed. Metal/Apple host lanes remained diagnostic/host-gated on Windows as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit gate build completed successfully. |
