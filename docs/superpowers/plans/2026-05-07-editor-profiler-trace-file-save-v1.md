# Editor Profiler Trace File Save v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `editor-profiler-trace-file-save-v1`

**Goal:** Add an explicit editor Profiler action that saves the current diagnostics Chrome Trace Event JSON to a safe project-relative `.json` file.

**Architecture:** Reuse `EditorProfilerTraceExportModel` and `mirakana::export_diagnostics_trace_json`; do not introduce a second trace format. `editor/core` will validate a caller-supplied project-relative output path and write through `ITextStore`, while the optional Dear ImGui shell will expose a small path input plus an explicit `Save Trace JSON` action over the current capture.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, Dear ImGui, `ITextStore`, `MK_ui`, CMake/CTest and PowerShell 7 `tools/` validation.

---

## Goal

Close the next narrow profiler productization gap:

- save the current non-empty diagnostics capture as Chrome Trace Event JSON;
- require a safe project-relative `.json` output path;
- report saved path, payload bytes, and diagnostics through a deterministic editor-core result;
- expose a visible `Save Trace JSON` button in the Profiler panel;
- keep native file save dialogs, trace import, telemetry upload, crash-report backends, GPU/backend timestamps, allocator diagnostics, native handles, and production flame graphs out of scope.

## Context

- `Editor Profiler Trace Export v1` already added `EditorProfilerTraceExportModel`, retained `profiler.trace_export` rows, and visible `Copy Trace JSON`.
- `editor/core` already has `ITextStore`, `MemoryTextStore`, and `FileTextStore` for deterministic project-relative text IO.
- The master plan still lists profiler file save/import and telemetry follow-ups; this slice implements only explicit project-relative trace JSON save.

## Constraints

- Keep `editor/core` GUI-independent and free of SDL3, Dear ImGui, OS file dialogs, and native handles.
- Do not upload telemetry or introduce network/back-end dependencies.
- Do not parse/import trace files or add flame graph UI.
- Do not write outside the project text store; reject absolute paths, parent-relative segments, unsafe characters, and non-`.json` outputs before calling `ITextStore::write_text`.
- Preserve current clipboard export behavior.

## Done When

- A RED `MK_editor_core_tests` test fails first on the missing trace file save surface.
- `save_editor_profiler_trace_json` writes deterministic trace JSON through `ITextStore` for non-empty captures and rejects invalid/empty requests without writing.
- `make_profiler_ui_model` exposes retained trace file save rows under `profiler.trace_file_save`.
- `MK_editor` Profiler panel exposes `Trace Path` and `Save Trace JSON`, writes through `project_store_`, and reports transient saved/diagnostic status.
- Docs, manifest, plan registry, and editor skills describe the new narrow file-save contract and remaining unsupported work.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Editor-Core Trace File Save Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor profiler trace file save writes project relative json")`.
- [x] Use `mirakana::editor::MemoryTextStore`.
- [x] Build a non-empty `mirakana::DiagnosticCapture`.
- [x] Call `mirakana::editor::save_editor_profiler_trace_json(store, capture, request)` with `request.output_path = "diagnostics/editor-trace.json"`.
- [x] Assert:
  - `result.saved`;
  - `result.output_path == "diagnostics/editor-trace.json"`;
  - `result.payload_bytes == store.read_text("diagnostics/editor-trace.json").size()`;
  - saved text contains `"traceEvents"` and the sample name/message.
- [x] Add invalid/empty capture assertions:
  - empty capture returns `!saved` and a diagnostic;
  - `../trace.json`, `/tmp/trace.json`, `diagnostics/trace.txt`, and `diagnostics/bad=name.json` return `!saved`;
  - rejected requests do not create the target path.
- [x] Extend the profiler retained UI test to assert `profiler.trace_file_save.output_path.value`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Record the expected RED failure in Validation Evidence.

### Task 2: Editor-Core Trace File Save Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/profiler.hpp`
- Modify: `editor/core/src/profiler.cpp`

- [x] Forward-declare `class ITextStore;` in the profiler header.
- [x] Add:
  - `EditorProfilerTraceFileSaveRequest { std::string output_path{"diagnostics/editor-profiler-trace.json"}; mirakana::DiagnosticsTraceExportOptions options; }`;
  - `EditorProfilerTraceFileSaveResult { bool saved; std::string status_label; std::string output_path; std::size_t payload_bytes; std::vector<EditorProfilerKeyValueRow> rows; std::vector<std::string> diagnostics; }`.
- [x] Add `EditorProfilerTraceFileSaveResult trace_file_save;` to `EditorProfilerPanelModel`.
- [x] Declare `save_editor_profiler_trace_json(ITextStore& store, const mirakana::DiagnosticCapture& capture, const EditorProfilerTraceFileSaveRequest& request = {})`.
- [x] Implement local validation:
  - path is non-empty;
  - no `\r`, `\n`, or `=`;
  - no absolute-like path (`/`, `\`, or Windows drive);
  - no `..` segment;
  - extension is `.json`.
- [x] If capture is empty, return diagnostic `trace file save requires at least one diagnostic sample`.
- [x] If path is invalid, return deterministic diagnostic and do not write.
- [x] If write succeeds, call `store.write_text(output_path, payload)`, set `saved = true`, `status_label = "Trace file saved"`, and record payload bytes.
- [x] If `ITextStore` throws, catch `std::exception` and return `status_label = "Trace file save failed"` plus the exception text.
- [x] Have `make_editor_profiler_panel_model` populate default `trace_file_save` rows for the current capture.
- [x] Have `make_profiler_ui_model` emit root `profiler.trace_file_save`, row values, and diagnostics under `profiler.trace_file_save.diagnostics.N`.
- [x] Run focused build/test and record GREEN evidence.

### Task 3: Visible Profiler Save Action

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Store a GUI path buffer:

```cpp
char profiler_trace_export_path_[256]{"diagnostics/editor-profiler-trace.json"};
```

- [x] In `draw_profiler_panel`, snapshot diagnostics once and pass the same capture to the model and save action.
- [x] Render `ImGui::InputText("Trace Path", profiler_trace_export_path_, sizeof(profiler_trace_export_path_));` near the trace export rows.
- [x] Show `Save Trace JSON` only when `model.trace_export.can_export`.
- [x] On click, call `save_editor_profiler_trace_json(project_store_, capture, request)` using the buffer path.
- [x] Report `Trace JSON saved to <path> (<bytes> bytes)` on success; otherwise report the first diagnostic.
- [x] Keep `Copy Trace JSON` unchanged.
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

- [x] Record `Editor Profiler Trace File Save v1`, `save_editor_profiler_trace_json`, retained `profiler.trace_file_save` ids, and visible `Save Trace JSON`.
- [x] State that native file save dialogs, trace import, telemetry upload, crash-report backends, GPU/backend timestamps, allocator diagnostics, native handles, and production flame graphs remain unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

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
| RED focused build/test | PASS (expected failure) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation on missing `trace_file_save`, `EditorProfilerTraceFileSaveRequest`, and `save_editor_profiler_trace_json` symbols. |
| Focused `MK_editor_core_tests` | PASS | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after the trace file save implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Built `MK_editor`; GUI preset reported 46/46 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `tools/check-ai-integration.ps1` records the profiler trace file save contract across code, docs, manifest, and skills. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Reported `unsupported_gaps=11` and preserved `editor-productization` as `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial check found clang-format drift in `editor/core/src/profiler.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting and the rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passed after adding the editor-core trace file save surface. |
| `git diff --check` | PASS | No whitespace errors; Git emitted only existing LF-to-CRLF working-copy warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed; CTest reported 29/29 tests passed. Host-gated Metal and Apple diagnostics remained diagnostic-only on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev preset configured and built all targets successfully. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the Editor Profiler Trace File Save v1 files; leave unrelated pre-existing guidance changes unstaged. |
