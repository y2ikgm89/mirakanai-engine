# Editor Profiler Telemetry Handoff v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `editor-profiler-telemetry-handoff-v1`

**Goal:** Expose the existing dependency-free `DiagnosticsOpsPlan` telemetry upload handoff state in the editor Profiler panel without uploading telemetry or adding a backend dependency.

**Architecture:** `MK_core` remains the source of truth for diagnostics operation readiness through `build_diagnostics_ops_plan`. `MK_editor_core` adapts only the `telemetry_upload` artifact into deterministic retained rows, and the optional Dear ImGui shell renders those rows as read-only host/back-end handoff evidence.

**Tech Stack:** C++23, `MK_core` diagnostics, `MK_editor_core`, `MK_editor`, Dear ImGui, `MK_ui`, CMake/CTest and PowerShell 7 `tools/` validation.

---

## Goal

Close the next narrow profiler productization gap:

- show whether the current diagnostics capture has a caller-provided telemetry backend handoff path;
- report telemetry artifact kind, status, producer, format, blocker, and sample counts through deterministic editor-core rows;
- expose retained `profiler.telemetry` rows in the `MK_ui` profiler document;
- render a visible read-only "Profiler Telemetry Handoff" table in `MK_editor`;
- keep telemetry SDKs, network upload execution, crash-report backends, native dump writing, trace import, native handles, and production flame graphs out of scope.

## Context

- `Crash Telemetry Trace Ops v1` added `DiagnosticsOpsPlan`, including a `telemetry_upload` artifact that is `unsupported` until the caller reports a configured telemetry backend.
- `Editor Profiler Trace Export v1` and `Editor Profiler Trace File Save v1` already expose deterministic Chrome Trace Event JSON rows and safe project-relative file writes.
- The master plan still lists telemetry follow-ups beyond trace JSON export/save; this slice surfaces the existing handoff state only.

## Constraints

- Keep `editor/core` GUI-independent and free of SDL3, Dear ImGui, OS handles, telemetry SDKs, network clients, and native diagnostic APIs.
- Do not upload telemetry, write telemetry files, launch tools, or create backend credentials/configuration.
- Do not claim production telemetry readiness when `DiagnosticsOpsHostStatus::telemetry_backend_configured` is false.
- Use `DiagnosticsOpsPlan` and `diagnostics_ops_artifact_*_label` helpers instead of inventing editor-local status strings.
- Preserve existing trace export and trace file save behavior.

## Done When

- A RED `MK_editor_core_tests` test fails first on the missing telemetry handoff surface.
- `make_editor_profiler_telemetry_handoff_model` adapts the `telemetry_upload` artifact from `build_diagnostics_ops_plan`.
- `make_editor_profiler_panel_model` includes telemetry rows, and `make_profiler_ui_model` emits retained `profiler.telemetry` row ids.
- `MK_editor` renders a read-only telemetry handoff table from the model.
- Docs, manifest, plan registry, editor skills, and static integration checks describe the new narrow telemetry handoff contract and remaining unsupported work.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Editor-Core Telemetry Handoff Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor profiler telemetry handoff reports backend readiness")`.
- [x] Build a non-empty `mirakana::DiagnosticCapture` with one event, one counter, and one profile.
- [x] Call `mirakana::editor::make_editor_profiler_telemetry_handoff_model(capture)`.
- [x] Assert default state:
  - `!model.ready`;
  - `model.status_label == "unsupported"`;
  - one diagnostic/blocker contains `No telemetry backend`;
  - rows include `kind`, `status`, `format`, `blocker`, `events`, `counters`, and `profiles`.
- [x] Call the same function with `DiagnosticsOpsPlanOptions` where `host_status.telemetry_backend_configured = true`.
- [x] Assert ready state:
  - `ready_model.ready`;
  - `ready_model.status_label == "ready"`;
  - `ready_model.producer == "caller-provided telemetry backend"`;
  - `ready_model.diagnostics.empty()`.
- [x] Extend the profiler retained UI test to assert `profiler.telemetry.status.value`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Record the expected RED failure in Validation Evidence.

### Task 2: Editor-Core Telemetry Handoff Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/profiler.hpp`
- Modify: `editor/core/src/profiler.cpp`

- [x] Add `EditorProfilerTelemetryHandoffModel` with `ready`, `status_label`, `kind`, `producer`, `format`, `blocker`, `rows`, and `diagnostics`.
- [x] Declare `make_editor_profiler_telemetry_handoff_model(const mirakana::DiagnosticCapture&, const mirakana::DiagnosticsOpsPlanOptions& = {})`.
- [x] Implement the function by calling `mirakana::build_diagnostics_ops_plan(capture, options)` and selecting the `DiagnosticsOpsArtifactKind::telemetry_upload` artifact.
- [x] Set `ready` only when the selected artifact status is `DiagnosticsOpsArtifactStatus::ready`.
- [x] Populate deterministic rows for `kind`, `status`, `producer`, `format`, `blocker`, `events`, `counters`, and `profiles`.
- [x] Add the model to `EditorProfilerPanelModel` and populate it from `make_editor_profiler_panel_model`.
- [x] Emit retained rows under `profiler.telemetry` and diagnostics under `profiler.telemetry.diagnostics.N`.
- [x] Run focused build/test and record GREEN evidence.

### Task 3: Visible Profiler Telemetry Table

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Render `Profiler Telemetry Handoff` as a two-column read-only table after trace export/file save status.
- [x] Populate it from `model.telemetry.rows`.
- [x] If `model.telemetry.diagnostics` is non-empty, render the diagnostics as wrapped text below the table.
- [x] Do not add upload buttons, backend configuration controls, file writes, or network execution.
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

- [x] Record `Editor Profiler Telemetry Handoff v1`, `EditorProfilerTelemetryHandoffModel`, `make_editor_profiler_telemetry_handoff_model`, retained `profiler.telemetry` ids, and visible read-only telemetry handoff rows.
- [x] State that telemetry SDKs/uploads/backends, crash-report backends, native dump writing, trace import, native handles, and production flame graphs remain unsupported.
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
| RED focused build/test | PASS (expected failure) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation on missing `EditorProfilerPanelModel::telemetry`, `EditorProfilerTelemetryHandoffModel`, and `make_editor_profiler_telemetry_handoff_model` symbols. |
| Focused `MK_editor_core_tests` | PASS | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after the telemetry handoff model implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Built `MK_editor`; GUI preset reported 46/46 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `tools/check-ai-integration.ps1` records the profiler telemetry handoff contract across code, docs, manifest, and skills. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Reported `unsupported_gaps=11` and preserved `editor-productization` as `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Repository formatting check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passed after adding the editor-core telemetry handoff surface. |
| `git diff --check` | PASS | No whitespace errors; Git emitted only existing LF-to-CRLF working-copy warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed; CTest reported 29/29 tests passed. Host-gated Metal and Apple diagnostics remained diagnostic-only on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev preset configured and built all targets successfully. |
| Final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Commit-prep gate passed; `validate` reported 29/29 CTest tests and `build` completed the dev preset. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the Editor Profiler Telemetry Handoff v1 files and leave unrelated dirty guidance files unstaged. |
