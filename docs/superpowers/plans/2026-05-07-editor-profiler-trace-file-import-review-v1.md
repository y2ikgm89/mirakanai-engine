# Editor Profiler Trace File Import Review v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development for the behavior change and keep this plan updated as evidence lands.

**Plan ID:** `editor-profiler-trace-file-import-review-v1`

**Goal:** Add a safe project-relative trace JSON file import review path so the editor Profiler can read a saved trace from the project `ITextStore` and review it through the existing structured Trace Event JSON reviewer.

**Architecture:** Reuse `mirakana::review_diagnostics_trace_json` and `EditorProfilerTraceImportReviewModel`. `editor/core` validates a caller-supplied project-relative `.json` path and reads it through `ITextStore`; the optional Dear ImGui shell exposes a path field plus explicit import action. No native file dialogs, arbitrary JSON conversion, `DiagnosticCapture` reconstruction, flame graph UI, or telemetry upload execution are included.

**Tech Stack:** C++23, `MK_core`, `MK_editor_core`, `MK_editor`, Dear ImGui, `MK_ui`, CMake/CTest and PowerShell 7 `tools/` validation.

---

## Goal

Close the next narrow profiler productization gap:

- validate a caller-supplied project-relative `.json` trace import path;
- read the file through `ITextStore`;
- review the loaded payload with `make_editor_profiler_trace_import_review_model`;
- expose deterministic import rows and diagnostics from `editor/core`;
- render a visible `Trace Import Path` plus `Import Trace JSON` action in `MK_editor`;
- keep native file-open dialogs, directory traversal, absolute paths, arbitrary JSON conversion, capture reconstruction, telemetry upload, crash-report backends, native handles, and flame graphs out of scope.

## Context

- `Editor Profiler Trace Import Review v1` already added pasted payload review and retained `profiler.trace_import` rows.
- `Editor Profiler Trace File Save v1` already validates safe project-relative `.json` output paths and writes through `ITextStore`.
- The master plan previously listed project/file trace import as a follow-up. This slice closes only reviewed project-file loading of the same supported JSON subset; native file-open dialogs, arbitrary conversion, and capture reconstruction remain follow-ups.

## Constraints

- Keep `editor/core` standard-library-only and GUI-independent.
- Reuse the existing path safety policy for trace `.json` files; do not add native file dialogs or OS path probing to editor core.
- Read only through `ITextStore`.
- Do not mutate files, write imported traces, upload telemetry, launch tools, or expose native handles.
- Do not reconstruct `DiagnosticCapture` or build a flame graph.

## Done When

- A RED `MK_editor_core_tests` test fails first on the missing file import review surface.
- `import_editor_profiler_trace_json` validates path, reads via `ITextStore`, reviews the payload, and reports deterministic status rows and diagnostics.
- Existing pasted review behavior remains unchanged.
- `MK_editor` Profiler panel exposes `Trace Import Path` and `Import Trace JSON`, then renders the imported review rows.
- Docs, manifest, plan registry, editor skills, and static integration checks describe the narrow project-file import review contract and remaining unsupported work.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Editor-Core File Import Review Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor profiler trace file import review reads project relative json")`.
- [x] Store a valid payload from `mirakana::export_diagnostics_trace_json` under `diagnostics/editor-trace.json` in `MemoryTextStore`.
- [x] Call `mirakana::editor::import_editor_profiler_trace_json(store, request)`.
- [x] Assert imported status, path, payload bytes, retained rows, and nested review counts.
- [x] Assert empty/missing/malformed payload diagnostics are surfaced deterministically.
- [x] Assert invalid paths such as `../trace.json`, `/tmp/trace.json`, `diagnostics/trace.txt`, and `diagnostics/bad=name.json` are rejected before store reads.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests` and record the expected RED failure.

### Task 2: Editor-Core Import API

**Files:**
- Modify: `editor/core/include/mirakana/editor/profiler.hpp`
- Modify: `editor/core/src/profiler.cpp`

- [x] Add `EditorProfilerTraceFileImportRequest` with a default safe path.
- [x] Add `EditorProfilerTraceFileImportResult` with imported flag, status label, input path, payload bytes, rows, diagnostics, and nested `EditorProfilerTraceImportReviewModel`.
- [x] Declare `import_editor_profiler_trace_json(ITextStore& store, const EditorProfilerTraceFileImportRequest& request = {})`.
- [x] Reuse/rename the trace `.json` path validator so save and import share the same safety contract.
- [x] Read the payload through `ITextStore::read_text` only after path validation.
- [x] Review the payload through `make_editor_profiler_trace_import_review_model`.
- [x] Return deterministic diagnostics for invalid paths, missing/read failures, empty payloads, malformed JSON, and unsupported shapes.
- [x] Emit retained `profiler.trace_file_import` rows and diagnostics from `make_profiler_ui_model`.
- [x] Run focused build/test and record GREEN evidence.

### Task 3: Visible Profiler File Import Review UI

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add a fixed-size import path buffer and retained status/model fields.
- [x] Render `Trace Import Path` plus `Import Trace JSON`.
- [x] On import, call `import_editor_profiler_trace_json(project_store_, request)`.
- [x] Render the imported review rows and diagnostics through the existing trace import table.
- [x] Do not add native file-open dialogs, file write, upload buttons, capture reconstruction, or flame graph UI.
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

- [x] Record `Editor Profiler Trace File Import Review v1`, `EditorProfilerTraceFileImportRequest`, `EditorProfilerTraceFileImportResult`, `import_editor_profiler_trace_json`, retained `profiler.trace_file_import` ids, and visible `Import Trace JSON`.
- [x] State that native file-open dialogs, arbitrary JSON conversion, capture reconstruction, flame graphs, telemetry SDK/upload execution, crash-report backends, GPU/backend timestamps, allocator diagnostics, and native handles remain unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Final Validation And Commit

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
| RED focused editor build/test | PASS | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation on missing `EditorProfilerTraceFileImportRequest`, `import_editor_profiler_trace_json`, and `trace_file_import`, as expected. |
| Focused editor tests | PASS | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Optional GUI build plus desktop-gui CTest passed after wiring `Trace Import Path` / `Import Trace JSON`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `tools/check-ai-integration.ps1` records the trace file import review contract across editor core, GUI, tests, docs, manifest, plans, and skills. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Production readiness audit passed after master-plan and manifest wording moved project-file import review into the completed narrow editor contract. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial run found clang-format drift in `editor/core/src/profiler.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` fixed it and the rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passed after adding the editor-core trace file import review API. |
| `git diff --check` | PASS | Whitespace check passed; only existing LF/CRLF working-copy warnings were reported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed with 29/29 default CTest tests passing; Metal/Apple host checks remained diagnostic-only host gates on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Full default build passed. |
| Final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Commit-prep gate passed after docs, manifest, static checks, and plan evidence were settled. |
| Slice-closing commit | PASS | Stage only this slice's files and leave unrelated dirty guidance files unstaged. |
