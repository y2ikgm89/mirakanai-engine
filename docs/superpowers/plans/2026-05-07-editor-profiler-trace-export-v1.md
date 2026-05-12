# Editor Profiler Trace Export v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic Profiler trace-export model and visible `MK_editor` copy action for the current diagnostics capture.

**Architecture:** Reuse `mirakana::export_diagnostics_trace_json` from `MK_core`; do not invent a second trace format. `editor/core` will produce a GUI-independent `EditorProfilerTraceExportModel` with retained `MK_ui` rows and a JSON payload, while the optional Dear ImGui shell will only copy the current payload to the clipboard and record transient status text.

**Tech Stack:** C++23, `MK_core` diagnostics, `MK_editor_core`, retained `MK_ui`, Dear ImGui adapter in `MK_editor`, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the next narrow profiler productization gap:

- expose `EditorProfilerTraceExportModel` over the current `mirakana::DiagnosticCapture`;
- generate Chrome Trace Event JSON with `mirakana::export_diagnostics_trace_json`;
- retain `profiler.trace_export` rows for status, format, producer, byte count, and sample counts;
- add a visible `Copy Trace JSON` action in the Profiler panel for non-empty captures;
- keep file save dialogs, trace import, telemetry upload, crash-report backends, GPU/backend timestamps, allocator diagnostics, native handles, and production flame graphs out of scope.

## Context

- `MK_core` already owns deterministic diagnostics capture, summary aggregation, `DiagnosticsOpsPlan`, and `export_diagnostics_trace_json`.
- `MK_editor_core` already adapts `DiagnosticCapture` into `EditorProfilerPanelModel` summary/profile/counter/event rows.
- `MK_editor` already records `editor.frame` profile zones and stable editor counters through `DiagnosticsRecorder`.
- The master plan still lists profiler follow-ups; this slice implements only a local trace JSON copy path.

## Constraints

- Do not add a new trace format or serializer in editor code.
- Do not perform file IO from `editor/core`.
- Do not upload telemetry or introduce network/back-end dependencies.
- Do not expose RHI, OS, Dear ImGui, or clipboard handles through `editor/core`.
- Keep empty captures non-exportable in the visible action, while retaining deterministic diagnostics.

## Done When

- RED `MK_editor_core_tests` proves the trace-export model surface is missing.
- `EditorProfilerTraceExportModel` exposes deterministic payload/status/rows/diagnostics.
- `make_profiler_ui_model` emits retained `profiler.trace_export` rows.
- The visible Profiler panel exposes `Copy Trace JSON` only when the current capture is exportable.
- Docs, master plan, registry, manifest, skills, and static checks record the boundary truthfully.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Profiler Trace Export Tests

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor profiler trace export model exposes deterministic trace json")`.
- [x] Build a `mirakana::DiagnosticCapture` with one warning event, one counter, and one profile sample.
- [x] Call `mirakana::editor::make_editor_profiler_trace_export_model(capture)`.
- [x] Assert:
  - `can_export == true`,
  - `status_label == "Trace export ready"`,
  - `format == "Chrome Trace Event JSON"`,
  - `producer == "mirakana::export_diagnostics_trace_json"`,
  - `payload` contains `"traceEvents"`, `"slow frame"`, `"renderer.frames_started"`, and `"editor.frame"`,
  - `payload_bytes == payload.size()`,
  - retained rows include `status`, `format`, `producer`, `payload_bytes`, `events`, `counters`, and `profiles`.
- [x] Extend the existing profiler panel test to assert `model.trace_export.can_export` and `profiler.trace_export.payload_bytes` in `make_profiler_ui_model`.
- [x] Extend the empty capture test to assert `!model.trace_export.can_export`, diagnostic text, and no visible export button state.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`; confirm it fails before implementation because the trace-export model symbols are missing.

### Task 2: Editor-Core Trace Export Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/profiler.hpp`
- Modify: `editor/core/src/profiler.cpp`

- [x] Add:

```cpp
struct EditorProfilerTraceExportModel {
    bool can_export{false};
    std::string status_label;
    std::string format;
    std::string producer;
    std::string payload;
    std::size_t payload_bytes{0};
    std::vector<EditorProfilerKeyValueRow> rows;
    std::vector<std::string> diagnostics;
};
```

- [x] Add `EditorProfilerTraceExportModel trace_export;` to `EditorProfilerPanelModel`.
- [x] Add declaration:

```cpp
[[nodiscard]] EditorProfilerTraceExportModel
make_editor_profiler_trace_export_model(const mirakana::DiagnosticCapture& capture,
                                        const mirakana::DiagnosticsTraceExportOptions& options = {});
```

- [x] Implement `make_editor_profiler_trace_export_model`:
  - compute `capture_empty` from events/counters/profiles;
  - set `format = "Chrome Trace Event JSON"`;
  - set `producer = "mirakana::export_diagnostics_trace_json"`;
  - when empty, set `status_label = "Trace export empty"`, `can_export = false`, and diagnostic `trace export requires at least one diagnostic sample`;
  - when non-empty, set `payload = mirakana::export_diagnostics_trace_json(capture, options)`, `payload_bytes = payload.size()`, `can_export = true`, and `status_label = "Trace export ready"`;
  - add rows for `status`, `format`, `producer`, `payload_bytes`, `events`, `counters`, and `profiles`.
- [x] Have `make_editor_profiler_panel_model` set `model.trace_export = make_editor_profiler_trace_export_model(capture)`.
- [x] Have `make_profiler_ui_model` emit:
  - root list `profiler.trace_export`,
  - `profiler.trace_export.status.value`,
  - `profiler.trace_export.format.value`,
  - `profiler.trace_export.producer.value`,
  - `profiler.trace_export.payload_bytes.value`,
  - diagnostics under `profiler.trace_export.diagnostics.N`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Visible Profiler Copy Action

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add a small transient member near the profiler state:

```cpp
std::string profiler_trace_export_status_;
```

- [x] In `draw_profiler_panel`, render `model.trace_export.rows` in a `Profiler Trace Export` table after the summary table.
- [x] Show `Copy Trace JSON` only when `model.trace_export.can_export`.
- [x] On click, call `ImGui::SetClipboardText(model.trace_export.payload.c_str())` and set `profiler_trace_export_status_` to `Trace JSON copied (<bytes> bytes)`.
- [x] When not exportable, show the model diagnostics as wrapped text.
- [x] Keep the action transient: no file writing, no telemetry upload, no trace import, no native handle exposure.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Guidance, Static Checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

- [x] Record `Editor Profiler Trace Export v1`, `EditorProfilerTraceExportModel`, `make_editor_profiler_trace_export_model`, retained `profiler.trace_export` ids, and visible `Copy Trace JSON`.
- [x] State that file save dialogs, trace import, telemetry upload, crash-report backends, GPU/backend timestamps, allocator diagnostics, native handles, and production flame graphs remain unsupported.
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
| RED focused build/test | PASS (expected failure) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation on missing `trace_export` and `make_editor_profiler_trace_export_model` symbols. |
| Focused `MK_editor_core_tests` | PASS | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after the trace export model implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Built `MK_editor`; GUI preset reported 46/46 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `tools/check-ai-integration.ps1` records the profiler trace export contract across code, docs, manifest, and skills. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Reported `unsupported_gaps=11` and preserved `editor-productization` as `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial check found clang-format drift in `editor/core/src/profiler.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting and the rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passed after adding the editor-core trace export model surface. |
| `git diff --check` | PASS | No whitespace errors; Git emitted only existing LF-to-CRLF working-copy warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed; CTest reported 29/29 tests passed. Host-gated Metal and Apple diagnostics remained diagnostic-only on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev preset configured and built all targets successfully. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the Editor Profiler Trace Export v1 files; leave unrelated pre-existing guidance changes unstaged. |
