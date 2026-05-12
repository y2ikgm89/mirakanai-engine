# Editor Profiler Recorder Panel Model Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the editor Profiler panel's ad hoc status-only data path with a GUI-independent `mirakana_editor_core` profiler model backed by `mirakana_core` diagnostic captures.

**Architecture:** Keep the durable profiler interpretation in `mirakana_editor_core`. Consume value-only `mirakana::DiagnosticCapture`, `DiagnosticSummary`, `CounterSample`, `ProfileSample`, and `DiagnosticEvent` data from `mirakana_core`, derive deterministic summary rows and sorted profile/counter/event rows, and expose the model to the optional Dear ImGui shell as an adapter. Do not add SDL3, Dear ImGui, OS, GPU, RHI backend, telemetry, allocator, crash-report, or GPU marker dependencies to `editor/core`.

**Tech Stack:** C++23, `mirakana_core` diagnostics, `mirakana_editor_core`, retained `mirakana_editor_ui` model patterns, optional `mirakana_editor` ImGui adapter, focused `mirakana_editor_core_tests`.

---

## Context

- `mirakana_core` already owns `DiagnosticsRecorder`, `DiagnosticCapture`, summary aggregation, scoped CPU profile samples, and deterministic Trace Event JSON subset export.
- `mirakana_runtime_host` can record `runtime_host.frame` profile samples and renderer counters when a host supplies a recorder.
- The current editor Profiler panel is an ImGui-only status/debug panel that reports log count, undo/redo depth, asset import count, hot reload event count, shader compile count, dirty flag, and revision.
- Docs and manifest correctly say recorder-backed editor profiler panels are still missing. This slice should close that specific host-feasible gap while keeping GPU markers, allocator diagnostics, telemetry, crash reports, backend timestamps, and trace import/export workflows as follow-up work.

## Constraints

- Add editor model behavior in `mirakana_editor_core` first; the Dear ImGui panel is only an adapter.
- Do not add new third-party dependencies.
- Do not make runtime, renderer, RHI, platform, SDL3, Dear ImGui, or native handles dependencies of the profiler model.
- Keep outputs deterministic for tests and AI-facing diagnostics.
- Keep trace JSON export in `mirakana_core`; this slice may expose trace availability/status in the editor model but must not invent a second trace format.
- Keep Codex and Claude guidance synchronized.

## Done When

- [x] `mirakana_editor_core` exposes a profiler panel model built from `mirakana::DiagnosticCapture` plus editor status counters.
- [x] The model includes deterministic summary rows, profile rows, counter rows, event rows, empty-capture state, and trace-export availability/status text.
- [x] `mirakana_editor` Profiler panel renders the model instead of directly assembling ad hoc status text.
- [x] Tests prove profile/counter/event ordering, duration formatting, empty capture behavior, and editor status rows without requiring GUI dependencies.
- [x] Docs, roadmap, architecture, gap analysis, manifest, editor skill guidance, and Codex/Claude subagent guidance describe recorder-backed editor profiler models honestly while keeping native/GPU/telemetry/crash/allocator work as follow-up.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact blockers are recorded.

---

### Task 1: RED Profiler Model Tests

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `editor/core/include/mirakana/editor/ui_model.hpp` or add `editor/core/include/mirakana/editor/profiler.hpp`

- [x] **Step 1: Add failing model tests**

Add tests that build a `mirakana::DiagnosticCapture` with:

- one warning event and one info event
- two counters with different frame indices
- two profile samples with different durations/depths
- editor status values for log records, undo/redo counts, imports, hot reload events, shader compiles, dirty state, and revision

Expected model behavior:

- summary row counts match `mirakana::summarize_diagnostics`
- profile rows sort by frame index, start time, depth, then name
- counter rows sort by frame index, then name
- event rows sort by frame index, severity, category, then message
- durations format deterministically in milliseconds
- empty captures produce a clear empty-state row

- [x] **Step 2: Verify RED**

Run:

```powershell
cmake --build --preset dev --target mirakana_editor_core_tests
ctest --preset dev --output-on-failure -R mirakana_editor_core_tests
```

Expected before implementation: compile failure for missing profiler model API or test failure for missing behavior.

### Task 2: Implement Editor-Core Profiler Model

**Files:**
- Add or modify: `editor/core/include/mirakana/editor/profiler.hpp`
- Add or modify: `editor/core/src/profiler.cpp`
- Modify: `editor/CMakeLists.txt`

- [x] **Step 1: Add model types**

Define value types such as:

- `EditorProfilerStatus`
- `EditorProfilerSummaryRow`
- `EditorProfilerProfileRow`
- `EditorProfilerCounterRow`
- `EditorProfilerEventRow`
- `EditorProfilerPanelModel`

Use strings for labels/values so the ImGui adapter does not duplicate formatting rules.

- [x] **Step 2: Build the model from `DiagnosticCapture`**

Implement `make_editor_profiler_panel_model(const mirakana::DiagnosticCapture&, const EditorProfilerStatus&)`.

Use `mirakana::summarize_diagnostics` for summary counts. Derive sorted rows deterministically. Reject or sanitize unsafe label fields consistently with existing editor UI model rules.

- [x] **Step 3: Optional retained UI model**

If it stays small, add `make_profiler_ui_model(const EditorProfilerPanelModel&)` so future retained editor panels can consume the same model through `mirakana_ui`.

### Task 3: Wire ImGui Profiler Adapter

**Files:**
- Modify: `editor/src/main.cpp`

- [x] **Step 1: Add editor recorder members**

Add `mirakana::DiagnosticsRecorder` and `mirakana::SteadyProfileClock` members to the editor application shell. Record at least an `editor.frame` profile zone and stable editor status counters each frame without exposing GUI/native details to `editor/core`.

- [x] **Step 2: Render model rows**

Change `draw_profiler_panel()` to build `EditorProfilerStatus`, snapshot the recorder, call the model builder, and render summary/status/profile/counter/event rows. Preserve the existing status/debug information as model status rows.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/editor.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/editor-change/SKILL.md`
- Modify: `.codex/agents/*` and `.claude/agents/*` only if reviewer/explorer guidance changes

- [x] **Step 1: Synchronize capability claims**

Describe the editor Profiler as recorder-backed at the model layer after implementation.

- [x] **Step 2: Keep future work honest**

Keep GPU markers, allocator diagnostics, backend timestamps, telemetry upload, crash reporting, trace import/export workflows, and production flame graphs listed as follow-up.

- [x] **Step 3: Fix nearby docs drift**

Update the workspace example in `docs/editor.md` to include `panel.timeline`.

### Task 5: Verification

- [x] Run focused `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Validation Evidence

- `mirakana_editor_core_tests`: PASS through `cmake --build --preset dev --target mirakana_editor_core_tests` and `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`: PASS after rerunning with approved escalation because sandboxed vcpkg 7zip extraction hit `CreateFileW stdin failed with 5 (Access is denied.)`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS.
- Diagnostic-only blockers remain host/toolchain gated: Metal `metal` / `metallib` missing, Apple packaging requires macOS/Xcode with `xcodebuild` / `xcrun`, Android release signing is not configured, Android device smoke is not connected, and strict clang-tidy is diagnostic-only for the current Visual Studio compile database condition.
