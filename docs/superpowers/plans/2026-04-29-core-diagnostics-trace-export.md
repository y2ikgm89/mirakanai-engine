# Core Diagnostics Trace Export Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a dependency-free trace export foundation to `mirakana_core` so captured diagnostic events, counters, and CPU profile samples can be written as deterministic Chrome-trace-like JSON without exposing runtime, renderer, platform, SDL3, OS, GPU, Dear ImGui, or editor APIs to game code.

**Architecture:** Keep trace serialization in standard-library-only `mirakana_core` beside `DiagnosticsRecorder`. The export function consumes a `DiagnosticCapture` value and returns a stable JSON document with `traceEvents`. This slice does not add file IO, telemetry upload, crash reporting, allocator hooks, GPU markers, backend timestamp queries, or editor profiler UI.

**Tech Stack:** C++23, existing `mirakana_core`, `core_tests`, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Context

- `mirakana_core` already owns `DiagnosticEvent`, `CounterSample`, `ProfileSample`, `DiagnosticCapture`, `DiagnosticsRecorder`, profile clocks, scoped CPU profile zones, and diagnostic summary aggregation.
- `mirakana_runtime_host` can now capture `runtime_host.frame` profile samples and backend-neutral renderer counters into a host-supplied recorder.
- At the start of this slice, the production gap analysis listed trace export and recorder-backed editor profiler panels as missing observability follow-up work.
- A small deterministic JSON export gives samples, tests, CI, editor-core, and AI workflows a first engine-owned format to inspect captures before any native profiler or telemetry adapter is designed.

## Constraints

- Keep public API names in `mirakana::`.
- Do not add third-party dependencies.
- Keep `engine/core` standard-library-only.
- Do not expose SDL3, OS window handles, D3D12/Vulkan/Metal handles, GPU handles, RHI backend internals, Dear ImGui, or editor APIs through game public APIs.
- Do not add file IO to the core serializer; callers can decide where to write the returned string.
- Add tests before implementation and verify focused core diagnostics coverage.

## Done When

- [x] `mirakana_core` exposes a deterministic trace JSON serializer for `DiagnosticCapture`.
- [x] Events, counters, and profile samples map into stable `traceEvents` records with escaped strings and frame/depth metadata.
- [x] Core tests cover profile/counter/event export plus JSON string escaping.
- [x] Plan registry, roadmap, gap analysis, manifest, skills, and Codex/Claude subagent guidance describe the new capability honestly and keep GPU markers, telemetry, crash reports, allocator diagnostics, and recorder-backed editor profiler panels as follow-up.
- [x] Focused validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host/toolchain blockers are recorded.

---

### Task 1: Trace Export Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`

- [x] **Step 1: Add failing export tests**

Add tests for `serialize_diagnostics_trace_json` that expect:

- a top-level `traceEvents` array;
- diagnostic events as instant events with severity/category/message/frame metadata;
- counters as Chrome trace counter events with value/frame metadata;
- CPU profile samples as complete events with microsecond timestamps/durations and depth/frame metadata;
- JSON escaping for quotes and backslashes.

- [x] **Step 2: Verify red**

Run a focused core test build or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`.

Expected: FAIL before implementation because the serializer is not declared or defined.

Initial RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed while compiling `mirakana_core_tests` with MSVC `C2039`/`C3861` because `DiagnosticsTraceExportOptions` and `export_diagnostics_trace_json` were not members of `ge`.

### Task 2: Core Serializer Implementation

**Files:**
- Modify: `engine/core/include/mirakana/core/diagnostics.hpp`
- Modify: `engine/core/src/diagnostics.cpp`

- [x] **Step 1: Add public serializer declaration**

Declare:

```cpp
[[nodiscard]] std::string serialize_diagnostics_trace_json(const DiagnosticCapture& capture);
```

- [x] **Step 2: Implement deterministic JSON export**

Implement a small internal JSON writer helper for strings and numbers. Map records to trace events:

- `DiagnosticEvent`: instant event (`ph: "i"`) with `cat` derived from severity and category, `args.message`, and `args.frame`.
- `CounterSample`: counter event (`ph: "C"`) with `args.value` and `args.frame`.
- `ProfileSample`: complete event (`ph: "X"`) with `ts`/`dur` in integer microseconds, `args.frame`, and `args.depth`.

Keep process/thread ids deterministic (`pid: 1`, `tid: 0`).

- [x] **Step 3: Verify green**

Run focused core diagnostics tests.

Green evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after implementation, including `mirakana_core_tests`.

### Task 3: Documentation And Agent Guidance

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`
- Modify: `.codex/agents/engine-architect.toml`
- Modify: `.claude/agents/engine-architect.md`

- [x] **Step 1: Synchronize capability claims**

Document that deterministic trace export is implemented in `mirakana_core`, while GPU markers, telemetry upload, crash reports, allocator diagnostics, and recorder-backed editor profiler panels remain follow-up work.

- [x] **Step 2: Verify docs/contracts**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: PASS.

Evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.

### Task 4: Final Validation

**Files:**
- No additional implementation files.

- [x] **Step 1: Run focused and boundary checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

Expected: PASS.

Evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed with 18/18 CTest tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` passed, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.

- [x] **Step 2: Run final validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: PASS. Host-gated Metal/Apple/mobile diagnostics remain diagnostic-only where applicable.

Evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Diagnostic-only host blockers remained explicit: Metal shader/library checks require `metal` and `metallib`; Apple packaging requires macOS/Xcode (`xcodebuild`/`xcrun`); Android release signing is not configured and device smoke is not connected; strict tidy analysis reports the Visual Studio generator compile database limitation before continuing.

## Validation Evidence

- Initial RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed while compiling `mirakana_core_tests` because `DiagnosticsTraceExportOptions` and `export_diagnostics_trace_json` were not declared.
- Focused green: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed, 18/18 CTest tests passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS.
